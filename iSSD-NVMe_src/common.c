#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>

#include "common.h"
#include "nvme.h"

#define DEV_MEM   "/dev/mem"
#define DEV_PPMAP "/dev/ppmap"
#define DEV_UIO0  "/dev/uio0"
#define DEV_UIO1  "/dev/uio8"

#if (CUR_SETUP == TARGET_SETUP || CUR_SETUP == TARGET_RD_SETUP )
#define UIO
#define HOST_OUTBOUND_ADDR  0x1400000000        /*Outbound iATU Address*/
#define HOST_OUTBOUND_SIZE  8ULL*1024*1024*1024 /*Outbound iATU size 4GB*/
#define PCI_CONF_ADDR       0x3600000           /*PCIe3 config space address - According to LS2 DS*/
#endif

#ifdef UIO
#define DEV_FPGA_MMAP DEV_UIO0
#define DEV_FPGA_MMAP1 DEV_UIO1
#else
#define DEV_FPGA_MMAP DEV_MEM
#endif

#if (CUR_SETUP == LS2_TEST_SETUP || CUR_SETUP == STANDALONE_SETUP)
#define DEV_MMAP DEV_MEM
#elif (CUR_SETUP == X86_TEST_SETUP)
#define DEV_MMAP DEV_PPMAP
#endif

#if (CUR_SETUP == LS2_TEST_SETUP || CUR_SETUP == STANDALONE_SETUP)
#define DEV_QMEM DEV_MEM
#else
#define DEV_QMEM DEV_PPMAP
#endif

#define LS2_MEM 0x8200000000

extern unsigned long long int gc_phy_addr[70];

void munmap_fpga_bars (FpgaCtrl *fpga);
void munmap_host_mem (HostCtrl *host);

/*Ctrl+C Handler to clean EXIT*/
/*This is not a part of nvme controller;*/
volatile uint8_t nvmeKilled = 0;
void sig_handler (int signo)
{
	if (signo == SIGINT) {
		syslog(LOG_NOTICE,"received SIGINT\n");
		nvmeKilled = 1;
	}
}

int setup_ctrl_c_handler (void)
{
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		perror ("signal():");
	}
	return errno;
}

int open_ramdisk (char *file_path, uint64_t nMB, RamdiskCtrl *ramdisk)
{
#if (CUR_SETUP == X86_TEST_SETUP)
	int fd = open (file_path, O_RDWR);
#else
	int fd = open ("/dev/mem", O_RDWR);
#endif
	if (fd < 0) {
		perror ("open:");
		return errno;
	}

	ramdisk->fd_mem = fd;

#if (CUR_SETUP == X86_TEST_SETUP)
	ramdisk->mem.addr = (uint64_t)mmap (0, nMB, PROT_READ | PROT_WRITE, \
						MAP_SHARED, ramdisk->fd_mem, 0);
#else
	ramdisk->mem.addr = (uint64_t)mmap (0, nMB, PROT_READ | PROT_WRITE, \
						MAP_SHARED, ramdisk->fd_mem, LS2_MEM);
#endif
	if (ramdisk->mem.addr == (uint64_t)MAP_FAILED) {
		perror ("mmap");
		ramdisk->mem.addr = 0;
		SAFE_CLOSE (ramdisk->fd_mem);
		return errno;
	} else {
		ramdisk->mem.is_valid = 1;
		ramdisk->mem.size = nMB;
	}

	syslog(LOG_INFO,"ramdisk address is %p\n", (void *)ramdisk->mem.addr);

	return 0;
}

inline void close_ramdisk (RamdiskCtrl *ramdisk)
{
	if (!ramdisk->mem.is_valid) return;

	munmap ((void *)ramdisk->mem.addr, ramdisk->mem.size);
	SAFE_CLOSE (ramdisk->fd_mem);
	memset (ramdisk, 0, sizeof (RamdiskCtrl));
}

inline void ramdisk_prp_rw (void *prp, uint64_t ramdisk_addr, uint32_t req_type, uint64_t len)
{
	if (req_type == NVME_REQ_READ) {
		memcpy (prp, (void *)ramdisk_addr, len);
	} else {
		memcpy ((void *)ramdisk_addr, prp, len);
	}
}

#ifndef UIO
#if (CUR_SETUP == LS2_TEST_SETUP || CUR_SETUP == STANDALONE_SETUP)
/*Hard-coded FPGA BAR2 and BAR4 registers address;*/
#define PEX2_HARD_BAR2_OFF   0x1246000000	/*nvme reg and dma table*/
#define PEX2_HARD_BAR4_OFF   0x1246600000	/*fifo*/
#define PEX4_HARD_BAR0_OFF   0x1646008000	/*nvme reg and dma table*/
#define PEX4_HARD_BAR2_OFF   0x124600c000	/*fifo*/
#elif (CUR_SETUP == X86_TEST_SETUP)
#define HARD_BAR2_OFF   0xF4000000
#define HARD_BAR4_OFF   0xFBFF8000
#endif
#endif

int mmap_fpga_bars (FpgaCtrl *fpga, int *fd_uio)
{
	int ret = 0;
	int i;
	int eps = 0;
	int nPages[PCIE_MAX_EPS][PCIE_MAX_BARS] = {{1, 1, 1024, 1, 16, 1},{1,1,4,1,8,1}}; /*pages for each bar as required*/
	int fd_mem[2];
	MemoryRegion *bar;
	PCIDevice *ep = fpga->ep;

	fd_mem[0] = open (DEV_FPGA_MMAP, O_RDWR);
	PERROR_ON_ERROR ((fd_mem[0] < 0), ret, DEV_FPGA_MMAP": %s\n", strerror (errno));

#ifdef UIO
#if (CUR_SETUP == TARGET_SETUP)
	for (;eps < 2; eps++, ep++) {
		if(eps ==1 ){
			fd_mem[1] = open (DEV_FPGA_MMAP1, O_RDWR);
		    PERROR_ON_ERROR ((fd_mem[1] < 0), ret, DEV_FPGA_MMAP1": %s\n", strerror (errno));
		}
#endif
		bar = ep->bar;
		for (i=0; i < PCIE_MAX_BARS; i++) {
			bar[i].size  = nPages[eps][i] * getpagesize();
			bar[i].addr = (uint64_t)mmap (0, bar[i].size, \
					PROT_READ | PROT_WRITE, MAP_SHARED, \
					fd_mem[eps], i * getpagesize());
			if (bar[i].addr == (uint64_t)MAP_FAILED) {
				memset (&bar[i].addr, 0, sizeof (uint64_t));
				bar[i].is_valid = 0;
			} else {
				bar[i].is_valid = 1;
			}
		}
#if (CUR_SETUP == TARGET_SETUP)
	}
#endif
#else
	bar = ep->bar;
	bar[2].size = nPages[0][2] * getpagesize();
	bar[2].addr = (uint64_t)mmap (0, bar[2].size, \
			PROT_READ | PROT_WRITE, \
			MAP_SHARED, fd_mem[0], PEX2_HARD_BAR2_OFF);
	PERROR_ON_ERROR ((bar[2].addr == 0), ret, "bar2: %s\n", strerror (errno));
	bar[2].is_valid = 1;

	bar[4].size = nPages[0][4] * getpagesize();
	bar[4].addr = (uint64_t)mmap (0, bar[4].size, \
			PROT_READ | PROT_WRITE, \
			MAP_SHARED, fd_mem[0], PEX2_HARD_BAR4_OFF);
	PERROR_ON_ERROR ((bar[4].addr == 0), ret, "bar4: %s\n", strerror (errno));
	bar[4].is_valid = 1;
#if (CUR_SETUP == STANDALONE_SETUP)
	ep++;
	bar = ep->bar;
	bar[0].size = nPages[1][0] * getpagesize();
	bar[0].addr = (uint64_t)mmap (0, bar[0].size, \
			PROT_READ | PROT_WRITE, \
			MAP_SHARED, fd_mem[0], PEX4_HARD_BAR0_OFF);
	PERROR_ON_ERROR ((bar[0].addr == 0), ret, "bar0: %s\n", strerror (errno));
	bar[0].is_valid = 1;

	bar[2].size = nPages[1][2] * getpagesize();
	bar[2].addr = (uint64_t)mmap (0, bar[2].size, \
			PROT_READ | PROT_WRITE, \
			MAP_SHARED, fd_mem[0], PEX4_HARD_BAR2_OFF);
	PERROR_ON_ERROR ((bar[2].addr == 0), ret, "bar2: %s\n", strerror (errno));
	bar[2].is_valid = 1;
	
#endif
#endif

	/*Now verify the required BARs are mapped properly*/
	RETURN_ON_ERROR (!fpga->ep[0].bar[2].is_valid, ret, "Bad FPGA-BAR0");
#if (CUR_SETUP == TARGET_SETUP)
	//RETURN_ON_ERROR (!fpga->ep[0].bar[1].is_valid, ret, "Bad FPGA-BAR1");
#ifdef UIO
	RETURN_ON_ERROR (!fpga->ep[1].bar[2].is_valid, ret, "Bad FPGA-BAR2");
	RETURN_ON_ERROR (!fpga->ep[1].bar[4].is_valid, ret, "Bad FPGA-BAR4");
#endif
#endif
#ifdef UIO
	RETURN_ON_ERROR (!fpga->ep[0].bar[4].is_valid, ret, "Bad FPGA-BAR4");
#else
	RETURN_ON_ERROR (!fpga->ep[0].bar[4].is_valid, ret, "Bad FPGA-BAR4");
#endif
	/*Offset for LS2 NVMe register access permissions*/

	*fd_uio = fd_mem[0];
	fd_uio++;
	*fd_uio = fd_mem[1];
	return 0;

CLEAN_EXIT:
	munmap_fpga_bars (fpga);
	return ret;
}
void munmap_fpga_bars (FpgaCtrl *fpga)
{
	PCIDevice *ep = fpga->ep;
	int eps = 0;
	int i = 0;
	MemoryRegion *bar;

#if (CUR_SETUP == TARGET_SETUP || CUR_SETUP == STANDALONE_SETUP)
	for (; eps < 2; eps++, ep++) {
#endif
		bar = ep->bar;
		for (i = 0; i < PCIE_MAX_BARS; i++) {
			if (bar[i].is_valid) {
				munmap ((void *)bar[i].addr, bar[i].size);
			}
		}
#if (CUR_SETUP == TARGET_SETUP || CUR_SETUP == STANDALONE_SETUP)
	}
#endif
	memset (fpga, 0, sizeof (FpgaCtrl));
}

int setupFpgaCtrlRegs (FpgaCtrl *fpga)
{
	int i;
	PCIDevice *ep = fpga->ep;
	DmaRegs *dma_regs = &fpga->dma_regs;
	Nand_DmaRegs *nand_dma_regs = &fpga->nand_dma_regs;
	/*Pointers to registers from BARs mapped/stored here -TODO*/
	fpga->nvme_regs = (typeof (fpga->nvme_regs))(ep[0].bar[2].addr + 0x2000 + \
			FPGA_OFFS_NVME_REGS);

	fpga->fifo_reg = (uint32_t *)ep[0].bar[4].addr + FPGA_OFFS_FIFO_ENTRY;
	fpga->fifo_count = (uint32_t *)ep[0].bar[4].addr + FPGA_OFFS_FIFO_COUNT;
	fpga->fifo_reset = (uint32_t *)ep[0].bar[4].addr + FPGA_OFFS_FIFO_RESET;
	fpga->fifo_msi = (uint32_t *)ep[0].bar[4].addr + FPGA_OFFS_FIFO_IRQ_CSR;
	fpga->iosqdb_bits = (uint32_t *)ep[0].bar[4].addr + FPGA_OFFS_IOSQDBST;
	fpga->gpio_csr = (uint32_t *)ep[0].bar[4].addr + FPGA_OFFS_GPIO_CSR;

	for (i = 0; i < 1; i++) {
		dma_regs->icr[i] = (uint32_t *)ep[i].bar[4].addr + FPGA_OFFS_ICR;
		fpga->icr[i] = dma_regs->icr[i];
		dma_regs->table_sz[i] = (uint32_t *)ep[i].bar[4].addr + FPGA_OFFS_DMA_TABLE_SZ;
		dma_regs->csr[i] = (uint32_t *)ep[i].bar[4].addr + FPGA_OFFS_DMA_CSR;
		dma_regs->table[i] = (uint64_t *)ep[i].bar[2].addr + FPGA_OFFS_DMA_TABLE;
	}

	for (i = 0; i < NAND_TABLE_COUNT; i++) {
		nand_dma_regs->csr[i] = (uint32_t *)ep[1].bar[4].addr + (i * NAND_CSR_OFFSET);
		nand_dma_regs->table_sz[i] = (uint32_t *)ep[1].bar[4].addr + (i * NAND_CSR_OFFSET) + NAND_TBL_SZ_SHIFT;
		nand_dma_regs->table[i] = (uint32_t *)ep[1].bar[2].addr + (i * NAND_TABLE_OFFSET);
	}

	syslog(LOG_INFO,"bar4:0x%lx icr:%p table_sz:%p csr:%p table:%p \n", \
				ep[0].bar[4].addr, dma_regs->icr[0], dma_regs->table_sz[0], dma_regs->csr[0], dma_regs->table[0]);

	syslog(LOG_INFO,"bar4:0x%lx table_sz:%p csr:%p table:%p \n", \
				ep[1].bar[0].addr, nand_dma_regs->table_sz[0], nand_dma_regs->csr[0], nand_dma_regs->table[0]);
	return 0;
}

int mmap_host_mem (HostCtrl *host)
{
	int ret = 0;
	int fd_host_mem = 0;

#if (CUR_SETUP == TARGET_SETUP || CUR_SETUP == TARGET_RD_SETUP)
	void *mem_addr;

	uint64_t msix_mem_addr = PCI_CONF_ADDR;

	memset (host, 0, sizeof (HostCtrl));

	fd_host_mem = open(DEV_MEM, O_RDWR);
	PERROR_ON_ERROR ((fd_host_mem < 0), ret, \
			DEV_MEM": %s\n", strerror (errno));
	host->fd_mem = fd_host_mem;

	syslog(LOG_INFO,"outbound addr: 0x%lx  host size: 0x%llx\n", \
			(uint64_t)HOST_OUTBOUND_ADDR, HOST_OUTBOUND_SIZE);

	/*Host io memory*/
	mem_addr = mmap(0, HOST_OUTBOUND_SIZE, PROT_READ | PROT_WRITE, \
			MAP_SHARED, fd_host_mem, HOST_OUTBOUND_ADDR);

	JUMP_ON_ERROR ((mem_addr == NULL), ret, "mem_addr: %s\n", strerror(errno));

	syslog(LOG_INFO,"host io_mem_addr : %p\n", mem_addr);

	host->io_mem.addr = (uint64_t)mem_addr;
	host->io_mem.size = HOST_OUTBOUND_SIZE;
	host->io_mem.is_valid = 1;

	/*Host msi(x) table info memory*/
	mem_addr = mmap(0, getpagesize (), PROT_READ | PROT_WRITE, \
			MAP_SHARED, fd_host_mem, msix_mem_addr);

	JUMP_ON_ERROR ((mem_addr == NULL), ret, "mem_addr: %s\n", strerror(errno));

	syslog(LOG_INFO,"host msix_mem_addr : %p\n", mem_addr);

	host->msix_mem.addr = (uint64_t)mem_addr;
	host->msix_mem.size = getpagesize ();
	host->msix_mem.is_valid = 1;

	host->msix.addr_ptr = mem_addr + 0x948;
	host->msix.msix_en_ptr = mem_addr + 0xb2;

	host->msi.msi_en_ptr = mem_addr + 0x52;
	host->msi.addr_ptr = mem_addr + 0x54;
	host->msi.data_ptr = mem_addr + 0x5c;

	return 0;

CLEAN_EXIT:
	munmap_host_mem (host);
#endif

	return ret;
}
inline void munmap_host_mem (HostCtrl *host)
{
	if (host->io_mem.addr) {
		munmap ((void *)host->io_mem.addr, host->io_mem.size);
	}
	if (host->msix_mem.addr) {
		munmap ((void *)host->msix_mem.addr, host->msix_mem.size);
	}
	close (host->fd_mem);
	memset (host, 0, sizeof (HostCtrl));
}

uint64_t mmap_prp_addr (uint64_t base_addr, uint64_t off_addr, uint8_t nPages, int *fd)
{
#if (CUR_SETUP == TARGET_SETUP)
	return (HOST_OUTBOUND_ADDR + off_addr);
#elif (CUR_SETUP == TARGET_RD_SETUP)
	return (base_addr + off_addr);

#else
	uint64_t vAddr = 0;
	if (!*fd) {
		*fd = open (DEV_MMAP, O_RDWR | O_SYNC);
		if (*fd < 0) {
			perror (DEV_MMAP);
			return 0;
		}
	}
	vAddr = (uint64_t)mmap (0, nPages*getpagesize(), \
			PROT_READ|PROT_WRITE, \
			MAP_SHARED, *fd, off_addr - (off_addr % getpagesize()));
	if (vAddr == (uint64_t)MAP_FAILED) {
		perror ("prp_map: ");
		close(*fd);
		return 0;
	}

	return (vAddr + (off_addr % getpagesize()));

#endif
}

struct ad_trans {
    unsigned long long int phy_addr[70];
};

uint8_t read_mem_addr()
{
	struct ad_trans buf;

	uint8_t fd,ret;

	fd = open("/dev/uio_rc",O_RDWR);
    if (fd < 0) {
        perror ("open:");
        return -1;
    }

    memset(&buf,0,sizeof(uint64_t)*70);
    ret =read(fd,(struct ad_trans *)&buf,sizeof(unsigned long long int)*70);
    if(ret == -1){
        perror("read");
		close (fd);
        return -1;
    }
	
	memcpy(gc_phy_addr,buf.phy_addr,sizeof(unsigned long long int) *70);

	close(fd);
	return 0;
}

uint8_t* mmap_oob_addr(uint32_t oob_mem_size,int idx)
{
	uint8_t *virt_addr = NULL;
	uint8_t fd;
	uint64_t offset = gc_phy_addr[idx];

	fd = open("/dev/mem",O_RDWR);
	if (fd < 0) {
		perror ("open:");
        return NULL;
    }

	virt_addr = (uint8_t*)mmap (0, (oob_mem_size)*getpagesize(),PROT_READ|PROT_WRITE,MAP_SHARED,fd,offset);
    if (virt_addr == (uint8_t *)MAP_FAILED) {
        perror ("oob_map: ");
        close(fd);
        return NULL;
    }
	return virt_addr;
}

inline void munmap_prp_addr (void *addr, uint8_t nPages, int *fd)
{
#if (CUR_SETUP != TARGET_SETUP && CUR_SETUP != TARGET_RD_SETUP)
	munmap ((void*)((uint64_t)addr & 0xFFFFFFFFFFFFF000), nPages);
	SAFE_CLOSE (*fd);
#endif
}

uint64_t mmap_queue_addr (uint64_t host_base, uint64_t prp, uint16_t qid, int *fd)
{
#if (CUR_SETUP == TARGET_SETUP || CUR_SETUP == TARGET_RD_SETUP)
	return host_base + prp;
#else
	uint64_t addr = 0;

	*fd = open (DEV_QMEM, O_RDWR | O_SYNC);
	if (*fd < 0) {
		perror(DEV_QMEM);
		return 0;
	}

	addr = (uint64_t)mmap (0, ((qid<1)?2:16)*getpagesize(), \
			PROT_READ|PROT_WRITE, MAP_SHARED, *fd, prp);
	if(addr == (uint64_t)MAP_FAILED) {
		perror("mmap");
		close(*fd);
		*fd = 0;
		return 0;
	}
	return addr;
#endif
}

uint64_t prp_read_write (uint64_t host_base, uint64_t prp_ptr, uint64_t *list, uint64_t list_len)
{
	uint64_t prp_addr = 0;

	if(list_len > PAGE_SIZE){
		list_len = PAGE_SIZE;
	}

#if (CUR_SETUP == TARGET_SETUP || CUR_SETUP == TARGET_RD_SETUP)
	prp_addr = host_base + prp_ptr;
#else
	int fd = 0;
	if (!fd) {
		fd = open (DEV_MMAP, O_RDWR | O_SYNC);
		if (fd < 0) {
			perror (DEV_MMAP);
			return 0;
		}
	}
	prp_addr = (uint64_t)mmap (0, getpagesize(), \
			PROT_READ|PROT_WRITE, \
			MAP_SHARED, fd, prp_ptr - (prp_ptr % getpagesize()));
	if (prp_addr == (uint64_t)MAP_FAILED) {
		perror ("mmap");
		close(fd);
		return 0;
	}
	prp_addr += (prp_ptr % getpagesize());
#endif

	memcpy ((void *)list, (void *)prp_addr, list_len);

#if (CUR_SETUP != TARGET_SETUP && CUR_SETUP != TARGET_RD_SETUP)
	munmap((void *)(prp_addr& 0xFFFFFFFFFFFFF000), getpagesize());
	close(fd);
#endif
	return NVME_SUCCESS;
}

inline void set_thread_affinity (int cpuid, pthread_t thread)
{
	cpu_set_t cpuset;
	int ret_val;

	CPU_ZERO (&cpuset); /* Initializing the CPU set to be the empty set */
	CPU_SET (cpuid , &cpuset);  /* setting CPU on cpuset */

	ret_val = pthread_setaffinity_np (thread, sizeof (cpu_set_t), &cpuset);
	if (ret_val != 0) {
		perror ("pthread_setset_thread_affinity_np");
	}
	ret_val = pthread_getaffinity_np (thread, sizeof (cpu_set_t), &cpuset);
	if (ret_val != 0) {
		perror ("pthread_getset_thread_affinity_np");
	}
}

//#define NVME_SIG SIGUSR1

NvmeTimer *create_nvme_timer (void *timerCB, void *arg, uint16_t expiry_ns, int signo)
{
	NvmeTimer *newTimer = NULL;

	if (!timerCB) {
		syslog(LOG_ERR,"No timer callback specified!\r\n");
		return NULL;
	}

	newTimer = calloc (1, sizeof (NvmeTimer));
	if (!newTimer) {
		syslog(LOG_ERR,"Unable to alloc new timer\r\n");
		return NULL;
	}

	newTimer->sa.sa_flags = SA_SIGINFO;
	newTimer->sa.sa_sigaction = timerCB;
	sigemptyset(&newTimer->sa.sa_mask);
	sigaction(signo, &newTimer->sa, NULL);
	newTimer->sev.sigev_notify = SIGEV_SIGNAL;
	newTimer->sev.sigev_signo = signo;
	newTimer->sev.sigev_value.sival_ptr = arg;

	newTimer->its.it_value.tv_sec = expiry_ns / 10^9;
	newTimer->its.it_value.tv_nsec = expiry_ns % 10^9;

	if (timer_create (CLOCK_REALTIME, &newTimer->sev, &newTimer->timerID)) {
		syslog(LOG_ERR,"timer creation failed\r\n");
		FREE_VALID (newTimer);
		return NULL;
	}

	return newTimer;
}

inline int nvme_timer_expired (NvmeTimer *timer)
{
	struct itimerspec cur;

	if (timer_gettime (timer->timerID, &cur)) {
		perror ("timer_gettime:");
		syslog(LOG_ERR,"timer_gettime failed\r\n");
		return errno;
	}

	return (cur.it_value.tv_nsec | (cur.it_value.tv_sec == 0));
}

inline int start_nvme_timer (NvmeTimer *timer)
{
	if (timer_settime (timer->timerID, 0, &timer->its, NULL)) {
		perror ("timer_settime:");
		syslog(LOG_ERR,"%s: timer_settime failed\r\n",__func__);
		return errno;
	}

	return 0;
}

inline void stop_nvme_timer (NvmeTimer *timer)
{
	struct itimerspec sits = {{0}};

	if (timer_settime (timer->timerID, 0, &sits, NULL)) {
		perror ("timer_settime");
		syslog(LOG_ERR,"%s: timer_settime failed\r\n",__func__);
	}
}

inline void delete_nvme_timer_fn (NvmeTimer **timer)
{
	if (*timer) {
		(void)timer_delete ((*timer)->timerID);
		FREE_VALID (*timer);
		//*timer = NULL;
	}
}

