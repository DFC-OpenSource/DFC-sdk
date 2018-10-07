
#include <stdio.h>
#include <stdint.h>

#include "platform.h"

#include "pcie.h"
#include "common.h"

#include "nvme_bio.h"
#include "nvme.h"

#include "dma_mgr.h"
#include "./bdbm_ftl/bdbm_drv.h"

static NvmeBioTarget *g_BioTarget;

extern NvmeCtrl g_NvmeCtrl;
#ifdef QDMA
QdmaCtrl *g_QdmaCtrl;
#endif

extern void init_df_dma_mgr (FpgaCtrl *fpga);
extern void deinit_df_dma_mgr (void);
extern void init_dma_mgr(FpgaCtrl *fpga, DmaCtrl *dma);

inline int nvme_bio_mmap_prps (nvme_bio *bio)
{
	int fd = 0;
	uint64_t cnt = 0;
	uint64_t vAddr = 1;
	uint64_t *prp = bio->prp;
	uint64_t *vrp = bio->prp;

	for (; cnt < bio->nprps && vAddr; cnt++, prp++, vrp++) {
		vAddr = mmap_prp_addr (g_NvmeCtrl.host.io_mem.addr, \
				(uint64_t)(*prp), 1, &fd, bio->ns_num);
		*vrp = vAddr;
	}

	return fd;
}

#define GET_PPA_FROM_LPA(lpa) ((DDR_PAGE_SIZE >> 3) * (lpa >> 1))

/**
 * @Func	: pba_calc
 * @Brief	: calculate the physical address of DDR interfaced with FPGA for the 
 *		  given logical address and put them into two different way of addressing structures.
 *  @input	: lba - Logical block address for which the physical address need to be calculated.
 * @return 	: Retuns the union object, which will contains the physical address of DDR
 * 		  (both splitted and combined address structures).
 */
static inline ddr_addr_u ppa_calc (uint64_t lpa)
{
	ddr_addr_u ddr_addr;
	ddr_addr.phy.addr = GET_PPA_FROM_LPA (lpa);
	return ddr_addr;
}

extern struct bdbm_drv_info* _bdi;

int nvme_bio_request (nvme_bio *bio)
{
	int ret = 0;
	uint8_t is_full_page;
	uint64_t *prp = bio->prp;
	uint64_t data_size = bio->size;

	if (g_NvmeCtrl.target.mem_type[bio->ns_num] & LS2_DDR_NS) {
		
		uint64_t ramdisk_addr = (bio->slba << BDRV_SECTOR_BITS) + \
					g_NvmeCtrl.target.ramdisk.mem.addr;
		is_full_page = data_size >> PAGE_SHIFT;
		uint64_t nlp = (data_size >> PAGE_SHIFT) + ((data_size%PAGE_SIZE) ? 1 : 0);
#ifdef QDMA
		qdma_bio *q_bio = malloc (sizeof(qdma_bio));
		q_bio->size = bio->size;
		q_bio->req_type = bio->req_type;
		q_bio->req = bio->req_info;
		q_bio->nlb = nlp;
		q_bio->ualign = 0;
#endif
#ifdef CONTINUOUS_PRP
#ifndef QDMA
		uint64_t *prp_def = NULL;
		int count, nlba_def;
		int i = 0;
		nlba_def = nlp;
		int size = 0;
		while (is_full_page) {
			g_NvmeCtrl.dma.io_processor_st = 1; /*stage 1 */
			count = 1;
			prp_def = prp;
			for(; i < (nlba_def - 1); i++, count++, prp_def++) {
				if(((*(prp_def + 1)) - (*prp_def)) != 0x1000) {
					i++;
					break;
				}
			}
			size = count << PAGE_SHIFT;
			size = MIN(data_size, size);
			ramdisk_prp_rw ((void *)(*prp), \
					ramdisk_addr, bio->req_type, size);
			prp = prp_def + 1;
			ramdisk_addr += size;
			data_size -= size;
			is_full_page = data_size >> PAGE_SHIFT;
		}
#endif
#else
		int i = 0;
		while (is_full_page) {
#ifdef QDMA
			q_bio->prp[i] = *prp;
			q_bio->ddr_addr[i] = ramdisk_addr;
			i++;
#else
			ramdisk_prp_rw ((void *)(*prp), \
					ramdisk_addr, bio->req_type, PAGE_SIZE);
#endif
			prp++;
			ramdisk_addr += PAGE_SIZE;
			data_size -= PAGE_SIZE;
			is_full_page = data_size >> PAGE_SHIFT;
		}
#endif
		if(data_size) {
#ifdef QDMA
			q_bio->prp[i] = *prp;
			q_bio->ddr_addr[i] = ramdisk_addr;
#else
			ramdisk_prp_rw ((void *)(*prp), ramdisk_addr, \
				bio->req_type, data_size % PAGE_SIZE);
#endif
		}
#ifndef QDMA
		nvme_rw_cb (bio->req_info, 0);
#else
		enqueue_bio_to_mq(q_bio, g_QdmaCtrl->qdma_mq_txid);
#endif

	} else if (g_NvmeCtrl.target.mem_type[bio->ns_num] & FPGA_DDR_NS) {
#if 1
		uint64_t lpa = (bio->slba << BDRV_SECTOR_BITS);
		uint64_t nlp = (data_size >> PAGE_SHIFT) + ((data_size%PAGE_SIZE) ? 1 : 0);
#else
		uint64_t nSecPerPg = DDR_PAGE_SIZE / KERNEL_SECTOR_SIZE;
		uint64_t lpa = (bio->slba / nSecPerPg);
		uint64_t nlp = (bio->slba + bio->nlb + nSecPerPg - 1) / nSecPerPg - lpa;
#endif
		is_full_page = data_size >> PAGE_SHIFT;
#ifdef CONTINUOUS_PRP
		uint64_t *prp_def = prp;
		int count = 0;
		int i = 0;
		uint64_t nlba_def = nlp;
		uint64_t size = 0;

		while (is_full_page) {
			g_NvmeCtrl.dma.io_processor_st = 1; /*stage 1 */
			count = 1;
			for(; i < (nlba_def - 1); i++, count++, prp_def++, --nlp) {
				if(((*(prp_def + 1)) - (*prp_def)) != 0x1000) {
					prp_def++;
					i++;
					break;
				}
                        }
			size = MIN(data_size, count << PAGE_SHIFT);
                        ret = make_dma_desc (*prp, lpa, size >> 2, bio->req_info, bio->req_type, !--nlp);
                        prp = prp_def;
                        lpa += size;
                        data_size -= size;
                        is_full_page = data_size >> PAGE_SHIFT;
                }
#else
		while (is_full_page) {
			g_NvmeCtrl.dma.io_processor_st = 1; /*stage 1 */
			ret = make_dma_desc (*prp, lpa, PAGE_SIZE >> 2, bio->req_info, bio->req_type, !--nlp);
			prp++;
			lpa += PAGE_SIZE;
			data_size -= PAGE_SIZE;
			is_full_page = data_size >> PAGE_SHIFT;
		}
#endif
		if (data_size) {
			ret = make_dma_desc (*prp, lpa, data_size >> 2, bio->req_info, bio->req_type, 1);
		}
	} else if (g_NvmeCtrl.target.mem_type[bio->ns_num] & FPGA_NAND_NS) {
		return host_block_make_req (_bdi, bio);
	} else {
		printf("ERROR %s incorrect namespace type\n", __func__);
		return -1;
		/*Add nand bio-dma description calls here -TODO*/
		/*return nand_bio_request ();*/
	}

	return ret;
}

int nvme_bio_init (NvmeCtrl *n)
{

	int ret = 0;
	NvmeBioTarget *target = &n->target;
#ifdef QDMA
	g_QdmaCtrl = &n->qdmactrl;
#endif
	/*Get Mountable memory (DDR|NAND) info -TODO*/
	if(n->ns_check == DDR_ALONE) {
		init_ddr_dma_mgr (&n->fpga, &n->dma);
		target->mem_type[1] = FPGA_DDR_NS;
		n->dma.mem_type = NVME_BIO_DDR;
	} else if( n->ns_check == NAND_ALONE) {
		init_df_dma_mgr(&n->fpga);
		target->mem_type[1] = FPGA_NAND_NS;
		n->dma.mem_type = NVME_BIO_NAND;
		syslog(LOG_INFO,"SELECTED MEM_TYPE IS NAND");
	} else if (n->ns_check == DDR_NAND) {
		init_ddr_dma_mgr (&n->fpga, &n->dma);
		init_df_dma_mgr(&n->fpga);
		target->mem_type[1] = FPGA_DDR_NS;
		/*n->dma.mem_type = NVME_BIO_DDR;*/
		target->mem_type[2] = FPGA_NAND_NS;
		n->dma.mem_type = NVME_BIO_TWO;
	}
	
	/*We use ramdisk as target memory.. Setup ramdisk*/
	JUMP_ON_ERROR (open_ramdisk (NULL, RAMDISK_MEM_SIZE,		\
				     &target->ramdisk), ret, " ");
	
	target->mem_type[0] = LS2_DDR_NS;

	syslog(LOG_INFO,"RAMDISK SIZE: %lu Bytes\n", RAMDISK_MEM_SIZE);

	g_BioTarget = target;

	ret = pthread_create (&n->io_processor_tid, NULL, io_processor, (void *)n);
	if(ret) perror("io_processor");

	return 0;

CLEAN_EXIT:
	return ret;
}

inline void nvme_bio_deinit (NvmeCtrl *n)
{
	/*Should we flush or wait for flush in memory before ejecting? -TODO*/
	NvmeBioTarget *target = &n->target;

	if(n->ns_check == DDR_ALONE) {
		/*deinit ddr interface and close the ddr access channels -TODO*/
		deinit_ddr_dma_mgr (); 
	} else if( n->ns_check == NAND_ALONE) {
		deinit_df_dma_mgr (); 
		/*deinit nand interface and close the nand access channels -TODO*/
	} else if (n->ns_check == DDR_NAND) {
		deinit_ddr_dma_mgr (); 
		deinit_df_dma_mgr (); 
	}
	if (target->mem_type[0] & LS2_DDR_NS) {
		close_ramdisk (&target->ramdisk);
	}
	memset (target, 0, sizeof (NvmeBioTarget));
	THREAD_CANCEL(n->io_processor_tid);
}

