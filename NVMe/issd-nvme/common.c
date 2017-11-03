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
#include <signal.h>
#include <sys/time.h>

#include "common.h"
#include "nvme.h"
#include "qdma.h"

#define DEV_UIO0  "/dev/uio0"
#define DEV_UIO1  "/dev/uio8"
#define NOF_LEN 128

#if (CUR_SETUP == STANDALONE_SETUP)
#define DEV_MMAP DEV_MEM
#define DEV_QMEM DEV_MEM
#endif

extern QdmaCtrl *g_QdmaCtrl;
extern unsigned long long int gc_phy_addr[80];

void munmap_fpga_bars (FpgaCtrl *fpga);
void munmap_host_mem (HostCtrl *host);
int trace_fpga_im_info(NvmeCtrl *n);

char pex2_path[] = "/sys/bus/pci/devices/0000:01:00.0/resource";
char pex4_path[] = "/sys/bus/pci/devices/0001:01:00.0/resource";
/*Ctrl+C Handler to clean EXIT*/
/*This is not a part of nvme controller;*/
volatile uint8_t nvmeKilled = 0;
uint8_t total_eps = 0;

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

int trace_fpga_im_info (NvmeCtrl *n) 
{
	uint32_t *ver_reg;
	uint64_t bar_address = 0 ;
	//uint8_t version_bar = 5;
	int version=0,mem_fd=0,ret=0;
	int row = 15,count=0;

	FILE *file = fopen("/sys/bus/pci/devices/0000:01:00.0/resource", "r");
	if ( file != NULL )
	{
		char line[20];
		while(fgets(line, sizeof line, file)!= NULL)
		{
			if(count == row){
				bar_address = (uint64_t)strtoul(line,NULL,16);
				break;
			}
			else count++;
		}
		fclose(file);
	} else {
		printf("PCI device not found,Check storage card is Configured properly\n");
		return -1;
	}

	mem_fd = open ("/dev/mem", O_RDWR);
	PERROR_ON_ERROR ((mem_fd < 0), ret, "/dev/mem: %s\n", strerror (errno)); 

	ver_reg = (uint32_t*)mmap(0, getpagesize(), PROT_READ | PROT_WRITE,\
			MAP_SHARED, mem_fd, bar_address);
	if (ver_reg == MAP_FAILED) {
		perror("Version-reg MMAP:");
		return -1;
	}
	version = (uint32_t)(*(ver_reg+7));
	
	if(version ==  0x00020502 || version == 0x00020400 || version == 0x00020501){
		n->fpga_version =0;
		n->pex_count = 2;       
	} else if (version == 0x00030100 || version == 0xa3030401 || version == 0xa3030501 || version == 0xa7030500 || version == 0xa3030500 || version == 0xa7030501 || version == 0xa3030502 || version == 0xa7030502 || version == 0xa7030600 || version == 0xa3030600) {
		n->fpga_version =1;
		n->pex_count = 1;
	} else {
		n->pex_count = 0;
		printf(" ERROR:Unsupported FPGA image version");
	}
	total_eps = n->pex_count;

CLEAN_EXIT:
	munmap(ver_reg,getpagesize());
	close(mem_fd);
	return 0;
}   

int open_ramdisk (char *file_path, uint64_t nMB, RamdiskCtrl *ramdisk)
{
#ifdef QDMA
	ramdisk->mem.addr = RAMDISK_MEM_ADDR;
	ramdisk->mem.size = RAMDISK_MEM_SIZE;
	ramdisk->mem.is_valid = 1;
#else
	int ret = 0;
	int fd = open (DEV_MEM, O_RDWR);
	RETURN_ON_ERROR (fd < 0, ret, "/dev/mem : %s", strerror (errno));

	ramdisk->fd_mem = fd;

	ramdisk->mem.addr = (uint64_t)mmap (0, nMB, PROT_READ | PROT_WRITE, \
						MAP_SHARED, ramdisk->fd_mem, RAMDISK_MEM_ADDR);
	if (ramdisk->mem.addr == (uint64_t)MAP_FAILED) {
		perror ("mmap");
		ramdisk->mem.addr = 0;
		SAFE_CLOSE (ramdisk->fd_mem);
		return errno;
	} else {
		ramdisk->mem.is_valid = 1;
		ramdisk->mem.size = nMB;
	}
#endif
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

void ramdisk_prp_rw (void *prp, uint64_t ramdisk_addr, uint32_t req_type, uint64_t len)
{
#ifndef QDMA
#if (CUR_SETUP == STANDALONE_SETUP)
        uint64_t *addr = NULL;

        int fd = open (DEV_MEM, O_RDWR | O_SYNC);
        if (fd < 0) {
                perror(DEV_MEM);
        }
	
        addr = (uint64_t *)mmap (0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, (uint64_t)prp);
        if(addr == (uint64_t *)MAP_FAILED) {
                perror("mmap");
                close(fd);
                fd = 0;
	}
	prp = (void *)addr;
#endif
	if (req_type == NVME_REQ_READ) {
		memcpy (prp, (void *)ramdisk_addr, len);
	} else {
		memcpy ((void *)ramdisk_addr, prp, len);
	}
#if (CUR_SETUP == STANDALONE_SETUP)
        munmap ((void*)((uint64_t)addr & 0xFFFFFFFFFFFFF000), getpagesize());
        close (fd);
#endif
#else
	QdmaCtrl *qdma = g_QdmaCtrl;
	struct qdma_desc *base = NULL; 
	int i = 10000;
	static int chan_no = 0;
	qdma->size_counters[(len >> 12) - 1]++;
START:
	if (req_type == NVME_REQ_READ) {
		base = qdma_transfer(chan_no,(uint64_t)ramdisk_addr,(uint64_t)prp,len);
		while((base == NULL) && i--) {
			base = qdma_transfer(chan_no,(uint64_t)ramdisk_addr,(uint64_t)prp,len);
			usleep(500);
		}
	} else {
		base = qdma_transfer(chan_no,(uint64_t)prp,(uint64_t)ramdisk_addr,len);
		while((base == NULL) && i--) {
			base = qdma_transfer(chan_no,(uint64_t)prp,(uint64_t)ramdisk_addr,len);
			usleep(500);
		}
	}

	if(base != NULL) {
		struct qdma_io *temp = (struct qdma_io *)&(qdma->des_io_table[qdma->producer_index]);
		temp->q_desc[temp->count] = base;
		temp->count++;
		temp->chan_no = chan_no;
		qdma->pi_cnt++;
	} else {
		printf("base is null pointer\n");
		if(display_desc_status(chan_no)) {
			printf("status display failed\n");
		}
		usleep(25000);
		i = 10000;
		chan_no = 0;
		goto START;
	}
	chan_no++;
	if(chan_no > 7) {
		chan_no = 0;
	}

#endif
}

uint64_t get_bar_address(int ep, int bar_no)
{
	int count = 0;
	uint64_t bar_address;
	int lineNumber=3 * bar_no;
	FILE *file = NULL;

	if (!ep) {
		file = fopen(pex2_path, "r");
	} else {
		file = fopen(pex4_path, "r");
	}
	if (file != NULL) {
		char line[20];
		while (fgets(line, sizeof line, file) != NULL) {
			if(count == lineNumber){
				bar_address = (uint64_t)strtol(line, NULL, 16);
				break;
			} else {
				count++;
			}
		}
		fclose(file);
	} else {
		printf("file not opened\n");
	}
	return bar_address;
}

int mmap_fpga_bars (FpgaCtrl *fpga, int *fd_uio)
{
	int ret = 0;
	int nPages[PCIE_MAX_EPS][PCIE_MAX_BARS] = {{1, 1, 1024, 1, 32, 1},{1, 1, 4, 1, 32, 1}}; /*pages for each bar as required*/
	int fd_mem[2];
	MemoryRegion *bar;
	PCIDevice *ep = fpga->ep;
	fd_mem[0] = open(DEV_MEM, O_RDWR | O_SYNC);
	PERROR_ON_ERROR ((fd_mem[0] < 0), ret, DEV_MEM": %s\n", strerror (errno));

	if(total_eps == 1) {
		nPages[0][2] = 32;
	}

	int eps = 0;
	int i;
	fd_mem[1] = fd_mem[0];
	for (;eps < total_eps; eps++, ep++) {
		bar = ep->bar;
		for (i=0; i < PCIE_MAX_BARS; i++) {
			bar[i].size  = nPages[eps][i] * getpagesize();
			bar[i].addr = (uint64_t)mmap (0, bar[i].size,	\
						      PROT_READ | PROT_WRITE, MAP_SHARED, \
						      fd_mem[eps], get_bar_address(eps, i));
			if (bar[i].addr == (uint64_t)MAP_FAILED) {
				memset (&bar[i].addr, 0, sizeof (uint64_t));
				bar[i].is_valid = 0;
			} else {
				bar[i].is_valid = 1;
			}
		}
	}

	/*Now verify the required BARs are mapped properly*/
	RETURN_ON_ERROR (!fpga->ep[0].bar[PEX2_BAR_NVME].is_valid, ret, "Bad FPGA-BAR2");
	if(total_eps == 2) {
		RETURN_ON_ERROR (!fpga->ep[1].bar[PEX4_BAR_DMA].is_valid, ret, "Bad FPGA-BAR2");
		RETURN_ON_ERROR (!fpga->ep[1].bar[PEX4_BAR_DMA_CTRL].is_valid, ret, "Bad FPGA-BAR4");
	}

	RETURN_ON_ERROR (!fpga->ep[0].bar[PEX2_BAR_FIFO].is_valid, ret, "Bad FPGA-BAR4");
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

	for (; eps < total_eps; eps++, ep++) {
		bar = ep->bar;
		for (i = 0; i < PCIE_MAX_BARS; i++) {
			if (bar[i].is_valid) {
				munmap ((void *)bar[i].addr, bar[i].size);
			}
		}
	}
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

	fpga->fifo_reg = (uint32_t *)ep[0].bar[PEX2_BAR_FIFO].addr + FPGA_OFFS_FIFO_ENTRY;
	fpga->fifo_count = (uint32_t *)ep[0].bar[PEX2_BAR_FIFO].addr + FPGA_OFFS_FIFO_COUNT;
	fpga->fifo_reset = (uint32_t *)ep[0].bar[PEX2_BAR_FIFO].addr + FPGA_OFFS_FIFO_RESET;
	fpga->fifo_msi = (uint32_t *)ep[0].bar[PEX2_BAR_FIFO].addr + FPGA_OFFS_FIFO_IRQ_CSR;
	fpga->iosqdb_bits = (uint32_t *)ep[0].bar[PEX2_BAR_IOSQDB].addr + FPGA_OFFS_IOSQDBST;
	fpga->gpio_csr = (uint32_t *)ep[0].bar[PEX2_BAR_FIFO].addr + FPGA_OFFS_GPIO_CSR;
	fpga->gpio_int = (uint32_t *)ep[0].bar[PEX2_GPIO_CR].addr + FPGA_OFFS_GPIO_INT;
	fpga->msi_int = (uint64_t *)ep[0].bar[PEX2_MSI_CR].addr + FPGA_OFFS_MSI_INT;

	if(total_eps == 2) {
		fpga->nand_gpio_int = (uint32_t *)ep[1].bar[PEX4_GPIO_CR].addr + FPGA_OFFS_GPIO_INT;
		for (i = 0; i < 1; i++) {
			dma_regs->icr[i] = (uint32_t *)ep[0].bar[PEX2_BAR_DMA_CTRL].addr + FPGA_OFFS_ICR;
			fpga->icr[i] = dma_regs->icr[i];
			dma_regs->table_sz[i] = (uint32_t *)ep[0].bar[PEX2_BAR_DMA_CTRL].addr + FPGA_OFFS_DMA_TABLE_SZ;
			dma_regs->csr[i] = (uint32_t *)ep[0].bar[PEX2_BAR_DMA_CTRL].addr + FPGA_OFFS_DMA_CSR;
			dma_regs->table[i] = (uint64_t *)ep[0].bar[PEX2_BAR_DMA].addr + FPGA_OFFS_DMA_TABLE;
		}
		for (i = 0; i < NAND_TABLE_COUNT; i++) {
			nand_dma_regs->csr[i] = (uint32_t *)ep[1].bar[PEX4_BAR_DMA_CTRL].addr + (i * NAND_CSR_OFFSET);
			nand_dma_regs->table_sz[i] = (uint32_t *)ep[1].bar[PEX4_BAR_DMA_CTRL].addr + (i * NAND_CSR_OFFSET) + NAND_TBL_SZ_SHIFT;
			nand_dma_regs->table[i] = (uint32_t *)ep[1].bar[PEX4_BAR_DMA].addr + (i * NAND_TABLE_OFFSET);
		}
	} else {
		fpga->nand_gpio_int = (uint32_t *)ep[0].bar[PEX4_GPIO_CR].addr + FPGA_OFFS_GPIO_INT;
		for (i = 0; i < NAND_TABLE_COUNT; i++) {
			nand_dma_regs->csr[i] = (uint32_t *)ep[0].bar[PEX4_BAR_DMA_CTRL].addr + (i * NAND_CSR_OFFSET)+0x1000;
			nand_dma_regs->table_sz[i] = (uint32_t *)ep[0].bar[PEX4_BAR_DMA_CTRL].addr + (i * NAND_CSR_OFFSET) + NAND_TBL_SZ_SHIFT+0x1000;
			nand_dma_regs->table[i] = (uint32_t *)ep[0].bar[PEX4_BAR_DMA].addr + (i * NAND_TABLE_OFFSET)+0x4000;
		}
	}

	syslog(LOG_INFO,"bar4:0x%lx icr:%p table_sz:%p csr:%p table:%p \n", \
	       ep[0].bar[4].addr, dma_regs->icr[0], dma_regs->table_sz[0], dma_regs->csr[0], dma_regs->table[0]);

	return 0;
}

int mmap_host_mem (HostCtrl *host)
{
	int ret = 0;
#if (CUR_SETUP == TARGET_SETUP)
	int fd_host_mem = 0;

	void *mem_addr;

	memset (host, 0, sizeof (HostCtrl));

	fd_host_mem = open(DEV_MEM, O_RDWR);
	PERROR_ON_ERROR ((fd_host_mem < 0), ret,		\
			 DEV_MEM": %s\n", strerror (errno));
	host->fd_mem = fd_host_mem;

	syslog(LOG_INFO,"outbound addr: 0x%lx  host size: 0x%llx\n",	\
	       (uint64_t)HOST_OUTBOUND_ADDR, HOST_OUTBOUND_SIZE);

	/*Host io memory*/
	mem_addr = mmap(0, HOST_OUTBOUND_SIZE, PROT_READ | PROT_WRITE,	\
			MAP_SHARED, fd_host_mem, HOST_OUTBOUND_ADDR);

	JUMP_ON_ERROR ((mem_addr == NULL), ret, "mem_addr: %s\n", strerror(errno));

	syslog(LOG_INFO,"host io_mem_addr : %p\n", mem_addr);

	host->io_mem.addr = (uint64_t)mem_addr;
	host->io_mem.size = HOST_OUTBOUND_SIZE;
	host->io_mem.is_valid = 1;

	/*Host msi(x) table info memory*/
	mem_addr = mmap(0, getpagesize (), PROT_READ | PROT_WRITE,	\
			MAP_SHARED, fd_host_mem, PCI_CONF_ADDR);

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

uint64_t mmap_prp_addr (uint64_t base_addr, uint64_t off_addr, uint8_t nPages, int *fd, int ns_num)
{
#if (CUR_SETUP == TARGET_SETUP)
	if (ns_num) { /*Three Namespace 1.LS2 DDR 2.FPGA DDR 3.FPGA NAND */
		return (HOST_OUTBOUND_ADDR + off_addr);
	} else {
#ifdef QDMA
		return (HOST_OUTBOUND_ADDR + off_addr);
#else
		return (base_addr + off_addr);
#endif
	}
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
	unsigned long long int phy_addr[80];
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

	memset(&buf,0,sizeof(uint64_t)*80);
	ret =read(fd,(struct ad_trans *)&buf,sizeof(unsigned long long int)*80);
	if(ret == -1){
		perror("read");
		close (fd);
		return -1;
	}

	memcpy(gc_phy_addr,buf.phy_addr,sizeof(unsigned long long int) *80);

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
#if (CUR_SETUP != TARGET_SETUP)
	munmap ((void*)((uint64_t)addr & 0xFFFFFFFFFFFFF000), nPages);
	SAFE_CLOSE (*fd);
#endif
}

uint64_t mmap_queue_addr (uint64_t host_base, uint64_t prp, uint16_t qid, int *fd)
{
#if (CUR_SETUP == TARGET_SETUP)
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

#if (CUR_SETUP == TARGET_SETUP)
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

#if (CUR_SETUP != TARGET_SETUP)
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
