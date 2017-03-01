
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "nvme.h"
#include "dma_mgr.h"
#include "dma_ddr.h"
#include "dma_nand.h"

#define GET_PAGE_NUM(x)  ((x) >> 12)
#define NUM_DMA_CONTR	 2
#define DMA_MASK 		 (NUM_DMA_CONTR - 1)
#if (CUR_SETUP != STANDALONE_SETUP)
#define DMA_MSI
#endif

extern uint8_t nvmeKilled;
extern NvmeCtrl g_NvmeCtrl;
extern struct bdbm_drv_info* _bdi;

DmaCtrl *g_DmaCtrl;
DescStatus *g_DescSt;

static inline void reset_dma_descriptors (void)
{
	int iter;
	NvmeCtrl *n = &g_NvmeCtrl;
	csr_reg csr_reset = {0, 0, 1, 1, 1, 0};

	/*((icr_reg *)(n->fpga.dma_regs.icr[0]))->irq_stat = 0;*/

	memcpy ((void *)n->fpga.dma_regs.csr[0], (void *)&csr_reset, sizeof (csr_reg));

	g_DmaCtrl->tail_idx =0;
	g_DmaCtrl->head_idx =0;

	for(iter =0 ;iter< (g_DmaCtrl->total_desc*8);iter++){
		memset((uint32_t*)n->fpga.dma_regs.table[0]+iter, 0,sizeof(uint32_t));
	}
}

//static int check_first  = 0;
//static int check_prp  = 0;
int make_dma_desc (uint64_t prp, uint64_t pa, uint64_t len, void *req_info, uint8_t req_type, uint8_t eoc)
{
	NvmeCtrl *n = &g_NvmeCtrl;
	DdrDmaDesc desc = {0};
	/*icr_reg **icr = (icr_reg **)n->fpga.dma_regs.icr;*/
#if 0
	if(check_first == 0) {
		((csr_reg *)(n->fpga.dma_regs.csr[0]))->reset = 1;
		((csr_reg *)(n->fpga.dma_regs.csr[0]))->loop = 1;
		syslog(LOG_INFO, "reset and loop set\n");
	}
#endif    
	desc.ppa = pa;
	desc.prp = prp;
	desc.opcode = (req_type == DDR_REQTYPE_READ) ? 1 : 0;
	desc.data_len = len;
	desc.eoc = eoc;
	desc.cmd_tag = g_DmaCtrl->head_idx;
	desc.ownedByFpga = 1;
#if (CUR_SETUP == STANDALONE_SETUP)
	desc.irq_en = 0;
#else
	if(eoc) {
		desc.irq_en = 1;
	} else {
		desc.irq_en = 0;
	}
#endif


#ifdef SEMAPHORE_LOCK
	g_DmaCtrl->io_processor_st = IOPROC_ST_WAIT_DESC1;
	while (sem_wait(&g_DescSt[g_DmaCtrl->head_idx].free_desc));
#else

	do {
		mutex_lock (&g_DescSt[g_DmaCtrl->head_idx].available);
		/*if valid == 0, then the descriptor can be used, else wait*/
		if (!g_DescSt[g_DmaCtrl->head_idx].valid) {
			break;
		}
		mutex_unlock (&g_DescSt[g_DmaCtrl->head_idx].available);
		usleep (1);
	} while (1);
#endif
	g_DmaCtrl->io_processor_st = IOPROC_ST_GOT_DESC1;

	/*Handling both dma engines from PEX2 and 4*/
#if 0
	if ((GET_PAGE_NUM(pa) & DMA_MASK) != (g_DmaCtrl->head_idx & DMA_MASK)) {
		/* Since it's planned to have even pa in PEX2 DMA and 
		 * odd pa in PEX4 DMA, just skip this descriptor */
		dptr = g_DescSt[g_DmaCtrl->head_idx].ptr;
		memset (&desc, 0, sizeof (descr_t));
		desc.skip = 1;
		/*memcpy ((void *)g_DescSt[g_DmaCtrl->head_idx].ptr, \
		(void *)&desc, sizeof (new_desc));*/
		memcpy (dptr, ((uint64_t *)&desc), 3*sizeof(uint64_t));
		if(check_first == 0)
		{
			((csr_reg *)(n->fpga.dma_regs.csr[0]))->start = 1;
			check_first++;
		}
#ifdef SEMAPHORE_LOCK
		sem_post(&g_DescSt[g_DmaCtrl->head_idx].used_desc);
#else
		g_DescSt[g_DmaCtrl->head_idx].valid = 1;
		mutex_unlock (&g_DescSt[g_DmaCtrl->head_idx].available);
#endif		
		g_DmaCtrl->io_processor_st = IOPROC_ST_REL_DESC1;
		CIRCULAR_INCR (g_DmaCtrl->head_idx, g_DmaCtrl->total_desc);
		g_DmaCtrl->io_processor_st = IOPROC_ST_WAIT_DESC2;
#ifdef SEMAPHORE_LOCK
		while (sem_wait(&g_DescSt[g_DmaCtrl->head_idx].free_desc)) {
			perror ("free_desc:");
		}
#else
		do {
			mutex_lock (&g_DescSt[g_DmaCtrl->head_idx].available);
			/*if valid == 0, then the descriptor can be used, else wait*/
			if (!g_DescSt[g_DmaCtrl->head_idx].valid) {
				break;
			}
			mutex_unlock (&g_DescSt[g_DmaCtrl->head_idx].available);
			usleep (1);
		} while (1);
#endif		
		g_DmaCtrl->io_processor_st = IOPROC_ST_GOT_DESC2;
	}

	if(check_first == 0) {
		((csr_reg *)(n->fpga.dma_regs.csr[0]))->start = 1;
		syslog(LOG_INFO, "dma engine starts\n");
		check_first++;
	}
#endif
	g_DescSt[g_DmaCtrl->head_idx].req_ptr = req_info;
	memcpy(g_DescSt[g_DmaCtrl->head_idx].ptr, ((uint64_t *)&desc), (3*sizeof(uint64_t)));

#ifdef SEMAPHORE_LOCK
	sem_post(&g_DescSt[g_DmaCtrl->head_idx].used_desc);
#else
	g_DescSt[g_DmaCtrl->head_idx].valid = 1;
	mutex_unlock (&g_DescSt[g_DmaCtrl->head_idx].available);
#endif
	g_DmaCtrl->io_processor_st = IOPROC_ST_REL_DESC2;
	CIRCULAR_INCR (g_DmaCtrl->head_idx, g_DmaCtrl->total_desc);
	if(!(((csr_reg *)(n->fpga.dma_regs.csr[0]))->start)) {
//	if(!g_DmaCtrl->head_idx) {
		((csr_reg *)(n->fpga.dma_regs.csr[0]))->start = 1;
		((csr_reg *)(n->fpga.dma_regs.csr[0]))->loop = 1;
	}

	g_DmaCtrl->io_processor_st = IOPROC_ST_END;

	return 0;
}

static void setup_ddr_desc_array ()
{
	int i = 0;
	uint64_t **table = g_DmaCtrl->regs->table;
	DescStatus *desc_st = g_DescSt;

	for (; i < (g_DmaCtrl->total_desc); i++) {
		desc_st->ptr = (uint64_t *)(table[0] + (i*4));
		desc_st++;
	}
}

static void setup_nand_desc_array ()
{
	/*Setup NAND descriptor array pattern -TODO*/
}

void dma_default (void)
{
	g_DmaCtrl->head_idx=0;
	g_DmaCtrl->mem_type = NVME_BIO_DDR;

	if (g_DmaCtrl->mem_type == NVME_BIO_DDR) {
		setup_ddr_desc_array ();
	} else {
//		setup_nand_desc_array ();
	}
    reset_dma_descriptors();
}

void init_ddr_dma_mgr (FpgaCtrl *fpga, DmaCtrl *dma)
{
	DmaRegs *regs = &fpga->dma_regs;
	int i = 0;

	*(regs->table_sz[0]) =  DESC_TBL_ENTRIES * 32 ;
	dma->regs = regs;
	dma->total_desc =  DESC_TBL_COUNT * DESC_TBL_ENTRIES;

	dma->desc_st = calloc (1, dma->total_desc * sizeof (DescStatus));

	g_DmaCtrl = dma;
	g_DescSt = dma->desc_st;

	for (i = 0; i < dma->total_desc; i++) {
#ifdef SEMAPHORE_LOCK
		if(sem_init(&g_DescSt[i].used_desc,0,0)) {
			perror("sem_init() for used desc");
			return;
		}
		if(sem_init(&g_DescSt[i].free_desc,0,1)) {
			perror("sem_init() for free desc");
			return;
		}
#else
		mutex_init (&g_DescSt[i].available, NULL);
#endif
	}

	dma_default ();

	dma->is_active = 0;
}

void inline deinit_ddr_dma_mgr (void)
{
	int i = 0;
	if (g_DmaCtrl->is_active) {
		syslog(LOG_DEBUG,"DMA is not free!\n");
		return;
	}

	FREE_VALID (g_DmaCtrl->desc_st);
	g_DmaCtrl->head_idx = g_DmaCtrl->tail_idx = 0;
	for(i=0;i<g_DmaCtrl->total_desc;i++) {
#ifdef SEMAPHORE_LOCK
		sem_destroy(&g_DescSt[i].free_desc);
		sem_destroy(&g_DescSt[i].used_desc);
#else
		mutex_destroy(&g_DescSt[i].available);	
#endif
	}

}

static inline void wait_for_dma_irqs (icr_reg *icr)
{
	/*Wait for any dma irq from PCIe2 & PCIe4 and return -TODO*/
}

static inline void abort_active_ios (void)
{
	/*get active cmd_tags and get req-info and call nvme_abort_req -TODO*/
}

#define IS_RD_WR_TOUT_ERR(ec) (ec & (WR_TOUT | RD_TOUT))
#define SLEEP_1NS 1L

void *io_completer (void *arg)
{
	NvmeCtrl *n = arg;
	csf_reg *csf;
	csf_reg *csf_bak = malloc(sizeof(csf_reg));
	csr_reg **csr_ptr;
	uint8_t io_st;
	int err;
#ifdef DMA_MSI
	int rc, fd;
	uint32_t info = 1;
	ssize_t nb;
	struct timeval def_time = {0, 1000};
	struct timeval timeout = {0, 1000};
#endif
	sleep(1);
	csr_ptr = (csr_reg **)n->fpga.dma_regs.csr;

	set_thread_affinity (2, pthread_self ());

	g_DmaCtrl->tail_idx = 0;
	/*reset_dma_descriptors ();*/

#ifdef DMA_MSI
	fd = open("/dev/uio6", O_RDWR);
	if(fd < 0) {
		perror("uio open");
		return (void *)-1;
	}

	fd_set readfds;
	FD_ZERO(&readfds);

	while (1) {
WAIT:
		info = 1;
		nb = write(fd, &info, sizeof(info));
		if (nb < sizeof(info)) {
			perror("config write:");
			continue;
		}

		FD_SET(fd, &readfds);
		rc = select(fd+1, &readfds, NULL, NULL, &timeout);
		if (rc) {
			nb = read(fd, &info, sizeof(info));
			if(nb != sizeof(info)){
				perror("uio Read:");
				continue;
			}
		} else if (!rc) {
			timeout = def_time;
		} else if(rc == -1) {
			timeout = def_time;
			continue;
		}
#endif
		usleep(10);
		while(1) {
			do {
#ifndef DMA_MSI
#ifndef SEMAPHORE_LOCK
WAIT:
#endif
#endif
#ifdef SEMAPHORE_LOCK
#ifdef DMA_MSI
				if((sem_trywait(&g_DescSt[g_DmaCtrl->tail_idx].used_desc)) < 0) {
					goto WAIT;
				}
#else
				while (sem_wait(&g_DescSt[g_DmaCtrl->tail_idx].used_desc));
#endif
				g_DmaCtrl->io_completer_st = IOCOMP_ST_GOT_DESC3;
#else
				mutex_lock (&g_DescSt[g_DmaCtrl->tail_idx].available);
				g_DmaCtrl->io_completer_st = IOCOMP_ST_GOT_DESC3;
				if (!g_DescSt[g_DmaCtrl->tail_idx].valid) {
					mutex_unlock (&g_DescSt[g_DmaCtrl->tail_idx].available);
#ifndef DMA_MSI
					usleep(1);
#endif
					goto WAIT;
				}
#endif
				csf = (csf_reg *)((uint64_t *)g_DescSt[g_DmaCtrl->tail_idx].ptr + DMA_DDR_CSF_OFF);

				while (((*(uint64_t *)csf)) & 0x2000000000000) {
					usleep(1);
				}
				memcpy (((uint64_t *)csf_bak),((uint64_t *)csf), sizeof (uint64_t));

				if(csf_bak->dma_cmp){
					io_st = 0;
					if(!csf_bak->eoc) {
#ifdef SEMAPHORE_LOCK
						sem_post(&g_DescSt[g_DmaCtrl->tail_idx].free_desc);
#else
						g_DescSt[g_DmaCtrl->tail_idx].valid = 0;
						mutex_unlock (&g_DescSt[g_DmaCtrl->tail_idx].available);
#endif
						g_DmaCtrl->io_completer_st = IOCOMP_ST_REL_DESC3;
						CIRCULAR_INCR (g_DmaCtrl->tail_idx, g_DmaCtrl->total_desc);
					}
				}else{
					err = csr_ptr[0]->err_code;
					if(err==1 || err ==2) {
						printf("READ/WRITE error occured\n");
						io_st = 1;
						g_DmaCtrl->tail_idx = 0;
					}
					break;
				}
			} while (!csf_bak->eoc);
			if(g_DescSt[g_DmaCtrl->tail_idx].req_ptr == NULL) {
				if(n->ns_check == DDR_NAND) {
					if (_bdi->ptr_ftl_inf->store) {
						syslog (LOG_INFO,"Storing FTL....\n");
						_bdi->ptr_ftl_inf->store (_bdi, "/run/ftl_pmt/ftl.dat");
					}
					printf("Controller Corrupted. Shutdown and Powerup the Host Machine\n");
				}
				printf("csf:%p id:%u tag: %u \n",csf, g_DmaCtrl->tail_idx, csf_bak->cmd_tag);
			}
			if (io_st) { /*DMA engine's halted(?).. restore it*/
				printf("###################### DMA ENGINE HALTED ############################\n");
#if CLEARALL_ON_ERR && (MEMORY_TYPE == NVME_BIO_DDR)
				abort_active_ios ();
				reset_dma_descriptors ();
#else
				/*Do fail-io for this io request only -TODO*/
#endif
#ifndef SEMAPHORE_LOCK
				mutex_unlock (&g_DescSt[g_DmaCtrl->tail_idx].available);
#endif
			} else {
				nvme_rw_cb ((void *)g_DescSt[g_DmaCtrl->tail_idx].req_ptr, io_st);
				g_DescSt[g_DmaCtrl->tail_idx].req_ptr = NULL;
#ifdef SEMAPHORE_LOCK
				sem_post(&g_DescSt[g_DmaCtrl->tail_idx].free_desc);
#else
				g_DescSt[g_DmaCtrl->tail_idx].valid = 0;
				mutex_unlock (&g_DescSt[g_DmaCtrl->tail_idx].available);
#endif
				g_DmaCtrl->io_completer_st = IOCOMP_ST_REL_END_DESC3;
				CIRCULAR_INCR (g_DmaCtrl->tail_idx, g_DmaCtrl->total_desc);
			}
#ifdef DMA_MSI
		}
#endif
	}
	syslog(LOG_INFO, "Completion thread ends\n");
#ifdef DMA_MSI
	close(fd);
#endif
}
