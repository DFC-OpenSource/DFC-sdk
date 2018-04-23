#include <stdint.h>
#include <errno.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "nvme.h"
#include "unaligned_cache.h"
#include "common.h"
#include "ls2_cache.h"
#include "qdma.h"

nvme_cache *g_nvme_cache;
NvmeCtrl	*c_NvmeCtrl;
int kpages_per_fpage_shift;
int sectors_per_page_shift;

#ifdef QDMA
QdmaCtrl *g_QdmaCtrl;
#endif
void nvme_bio_cb(nvme_bio *bio,int ret);
int nvme_uncache_cb(nvme_bio *bio);
static int qdma_write_to_unaligned_cache(nvme_bio *bio, int upgs, void *req);
static int memcpy_write_to_unaligned_cache(nvme_bio *bio, int upgs);
static int ualgn_bio_write(nvme_bio *bio, int upgs);
static int ualgn_bio_read(nvme_bio *bio);
int ualgn_pg_handler_init(NvmeCtrl *n);
int ualgn_pg_handler_deinit(nvme_cache *cache);
int write_read_host_ls2(uint64_t prp, uint64_t len, uint64_t *cache_addr, int is_write);

void enqueue_bio_to_mq(void *bio, int mq_txid)
{
	uint64_t addr;
	int ret;
	addr = (uint64_t)(bio);
	do {
		if ((ret = mq_send(mq_txid, (char *)&addr, sizeof (uint64_t), 1)) < 0) {
			perror("mq_send");
		}
	} while (ret < 0);
}

int flush_to_nand()
{
	nvme_cache *cache = g_nvme_cache;
	unaligned_cache_ddr *cache_page = g_nvme_cache->cache_table;
	uint16_t f_idx;
	int cnt =0,i;

	if(cache->page_boundary) {
		nvme_bio *f_bio = malloc(sizeof(nvme_bio));
		uint64_t *lba = malloc(sizeof(uint64_t) << kpages_per_fpage_shift);
		for(i=0; i < KPAGES_PER_FPAGE; i++){
			f_idx = cache->f_idx;
			mutex_lock(&cache_page[f_idx].page_lock);
			if(cache_page[f_idx].status & UNALIGNED_USED){
				lba[i] = cache_page[f_idx].lba;
				f_bio->prp[i] = cache_page[f_idx].phy_addr;
				cache_page[f_idx].status = UNDER_PROCESS;
				mutex_unlock(&cache_page[f_idx].page_lock);
				cnt++;
				CIRCULAR_INCR(cache->f_idx , UNALIGNED_CACHE_PAGES);
			} else {
				mutex_unlock(&cache_page[f_idx].page_lock);
				break;
			}
		}
		if (cnt) {
			f_bio->slba = (uint64_t)lba;
			f_bio->nlb = cnt;
			f_bio->is_seq = 0;
			f_bio->req_type = NVME_REQ_WRITE;
			f_bio->req_info = f_bio; /*Since the memory need to be freed */
			if(c_NvmeCtrl->ns_check == DDR_NAND){
				f_bio->ns_num = 2;
			} else if(c_NvmeCtrl->ns_check == NAND_ALONE){
				f_bio->ns_num = 1;
			}
			mutex_lock(&cache->bdry_lock);
			cache->page_boundary = cache->page_boundary - cnt;
			mutex_unlock(&cache->bdry_lock);
			enqueue_bio_to_mq(f_bio,c_NvmeCtrl->nand_mq_txid);
		}

	}
	f_idx = (f_idx) ? f_idx : UNALIGNED_CACHE_PAGES;
	while (cache_page[f_idx - 1].status != UNALIGNED_FREED) { usleep(2); }
	for(i=0; i < UNALIGNED_CACHE_PAGES; i++){
		cache_page[i].lba = -1;
	}
	return 0;
}

int nvme_uncache_cb(nvme_bio *bio)
{
	nvme_cache *cache = g_nvme_cache;
	unaligned_cache_ddr *cache_page = cache->cache_table;
	uint16_t idx = 0, i;
	//int status = 0;

	idx = cache->tail_idx;
	for(i=0; i< bio->nlb; i++){
		mutex_lock(&cache_page[idx].page_lock);
		//status = cache_page[idx].status;
		cache_page[idx].status = UNALIGNED_FREED;
		cache_page[idx].lba = -1;
		mutex_unlock(&cache_page[idx].page_lock);
		CIRCULAR_INCR(idx, UNALIGNED_CACHE_PAGES);

#ifdef QDMA
		mutex_lock(&cache->ls2_mem_lock);
		cache->ls2_memory_status[cache->mem_tail] = MEM_FREED;
		mutex_unlock(&cache->ls2_mem_lock);

		CIRCULAR_INCR(cache->mem_tail, UNALIGNED_CACHE_PAGES);
#endif
	}
	cache->tail_idx = idx;
	free((void *)bio->slba);
	free(bio);
	return 0;
}

void nvme_bio_cb(nvme_bio *bio,int ret)
{
	NvmeRequest *req = NULL;
	req = (NvmeRequest*)(bio->req_info);

	if (req) {
		if(bio->is_seq)	{
			nvme_rw_cb(req, ret);
		} else {
			if (bio->req_type == REQTYPE_WRITE) {
				nvme_uncache_cb(bio);
			} else {
				free((void *)bio->slba);
				nvme_rw_cb(req, ret);
			}
		}
	} else {
		free(bio);
	}
}

int write_read_host_ls2(uint64_t prp, uint64_t len, uint64_t *cache_addr, int is_write)
{
    uint64_t *addr = NULL;

   int fd = open (DEV_MEM, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror(DEV_MEM);
        return 1;
    }

	addr = (uint64_t *)mmap (0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, prp);
	if(addr == (uint64_t *)MAP_FAILED) {
        perror("mmap");
        close(fd);
        fd = 0;
        return 1;
    }
    if (is_write) {
        memcpy(cache_addr, addr, len);
    } else {
        memcpy(addr, cache_addr, len);
    }
    munmap ((void*)((uint64_t)addr & 0xFFFFFFFFFFFFF000), getpagesize());
    close (fd);

        return 0;
}

static int qdma_write_to_unaligned_cache(nvme_bio *bio, int upgs, void *req)
{
	nvme_cache *cache = g_nvme_cache;
	unaligned_cache_ddr *cache_page = cache->cache_table;
	uint16_t i;
	uint64_t size = bio->size - (bio->nlb << PAGE_SHIFT);
	uint64_t ad = 0;
	int iter = bio->nlb;

	qdma_bio *q_bio = malloc (sizeof(qdma_bio));
	q_bio->size = size;
	q_bio->req_type = REQTYPE_WRITE;
	q_bio->req = req;
	q_bio->nlb = upgs;
	q_bio->ualign = 1;

	q_bio->lba = malloc(sizeof(uint64_t) * q_bio->nlb);
	for(i=0; i<upgs ; i++, iter++){
		do {
			mutex_lock(&cache->ls2_mem_lock);
			if(cache->ls2_memory_status[cache->mem_head] & MEM_FREED){
				break;
			} else { 
				mutex_unlock(&cache->ls2_mem_lock);
				if (!ad) {
					struct mq_attr attr[4];
					mq_getattr(c_NvmeCtrl->trim_mq_txid, &attr[0]);
					mq_getattr(c_NvmeCtrl->nand_mq_txid, &attr[1]);
					mq_getattr(c_NvmeCtrl->qdmactrl.qdma_mq_txid, &attr[2]);
					mq_getattr(c_NvmeCtrl->unalign_mq_txid, &attr[3]);
					ad++;
#if 0
					for(j = 0; j < UNALIGNED_CACHE_PAGES; j++) {
						mutex_lock(&cache_page[j].page_lock);
						//syslog(LOG_INFO, "%d %d %ld\n", j, cache_page[j].status, cache_page[j].lba);
						mutex_unlock(&cache_page[j].page_lock);
					}
#endif
				}
				usleep(1);
			}
		}while(1);
		cache->ls2_memory_status[cache->mem_head] = MEM_USED;
		mutex_unlock(&cache->ls2_mem_lock);
		q_bio->prp[i] = bio->prp[iter];

		q_bio->ddr_addr[i] = cache->ls2_phy_addr[cache->mem_head];

		q_bio->lba[i] = bio->slba + (iter << sectors_per_page_shift);

		CIRCULAR_INCR(cache->mem_head , UNALIGNED_CACHE_PAGES);
	}
#ifdef QDMA
	enqueue_bio_to_mq(q_bio, g_QdmaCtrl->qdma_mq_txid);
#endif
	return 0;
}

static int memcpy_write_to_unaligned_cache(nvme_bio *bio, int upgs)
{
	nvme_cache *cache = g_nvme_cache;
	unaligned_cache_ddr *cache_page = cache->cache_table;
	/*LS2_Cache *ddr_cache = &c_NvmeCtrl->ls2_cache;
	  LBA_Track *lba_table = ddr_cache->lba_track_table;*/
	uint16_t cache_idx =0,i;
	uint64_t size = bio->size - (bio->nlb << PAGE_SHIFT);
	uint64_t sz = 0;
	uint64_t slba;

	uint64_t *host_addr = NULL;
	int iter = bio->nlb;
	//mutex_lock(&cache->f_idx_lock);
	cache_idx = cache->head_idx;
	for(i=0; i<upgs ; i++, iter++){
		do {
			mutex_lock(&cache_page[cache_idx].page_lock);
			if(cache_page[cache_idx].status & UNALIGNED_FREED){
				break;
			} else {
				mutex_unlock(&cache_page[cache_idx].page_lock);
				usleep(1);
			}
		}while(1);
		cache_page[cache_idx].lba = bio->slba + (iter << sectors_per_page_shift);
		cache_page[cache_idx].status = UNALIGNED_USED;
		sz = MIN(PAGE_SIZE, size);
#if (CUR_SETUP == STANDALONE_SETUP)
		        write_read_host_ls2(bio->prp[iter], sz, cache_page[cache_idx].virt_addr, 1);
#else
		host_addr = (uint64_t *)(c_NvmeCtrl->host.io_mem.addr + bio->prp[iter] - HOST_OUTBOUND_ADDR);
		memcpy(cache_page[cache_idx].virt_addr, host_addr, sz);
#endif
		size -= sz;
		mutex_unlock(&cache_page[cache_idx].page_lock);
		/*slba = bio->f_lba + iter;
		  cache_page[cache_idx].slba = slba;
		  *mutex_lock(&(ddr_cache->lba_track_table_mutex));
		  lba_table[slba].status =  LBA_IN_UNALIGNED_CACHE;
		  lba_table[slba].cache_idx = cache_idx;
		  mutex_unlock(&(ddr_cache->lba_track_table_mutex));*/
		CIRCULAR_INCR(cache_idx , UNALIGNED_CACHE_PAGES);
	}
	mutex_lock(&cache->bdry_lock);
	cache->page_boundary+=upgs;
	mutex_unlock(&cache->bdry_lock);
	cache->head_idx = cache_idx;
	//mutex_unlock(&cache->f_idx_lock);
	return 0;
}

static int ualgn_bio_write(nvme_bio *bio, int upgs)
{
	nvme_cache *cache = g_nvme_cache;
	unaligned_cache_ddr *cache_page = cache->cache_table;
	uint32_t i,f_idx, pages;

	bio->nlb = bio->nlb - upgs;
	if(bio->nlb) {
		enqueue_bio_to_mq(bio,c_NvmeCtrl->nand_mq_txid);
	}
#ifdef QDMA
		if(!bio->nlb){
			qdma_write_to_unaligned_cache(bio, upgs, bio->req_info);
		} else {
			qdma_write_to_unaligned_cache(bio, upgs, NULL);
		}
#endif

#ifndef QDMA
		memcpy_write_to_unaligned_cache(bio, upgs);
		if((!bio->nlb) && (upgs)){
			nvme_rw_cb(bio->req_info, 0);
		}

		mutex_lock(&cache->bdry_lock);
		pages = cache->page_boundary;
		mutex_unlock(&cache->bdry_lock);

		if(pages >= KPAGES_PER_FPAGE) {
			nvme_bio *f_bio = malloc(sizeof(nvme_bio));
			uint64_t *lba = malloc(sizeof(uint64_t) << kpages_per_fpage_shift);
			//mutex_lock(&cache->f_idx_lock);
			f_idx = cache->f_idx;
			for(i=0; i < KPAGES_PER_FPAGE; i++){
				mutex_lock(&cache_page[f_idx].page_lock);
				lba[i] = cache_page[f_idx].lba;
				f_bio->prp[i] = cache_page[f_idx].phy_addr;
				cache_page[f_idx].status = UNDER_PROCESS;
				mutex_unlock(&cache_page[f_idx].page_lock);
				CIRCULAR_INCR(f_idx , UNALIGNED_CACHE_PAGES);
			}
			cache->f_idx = f_idx;
			//mutex_unlock(&cache->f_idx_lock);
			mutex_lock(&cache->bdry_lock);
			cache->page_boundary = cache->page_boundary - KPAGES_PER_FPAGE;
			pages = cache->page_boundary;
			mutex_unlock(&cache->bdry_lock);
			f_bio->slba = (uint64_t)lba;
			f_bio->nlb = KPAGES_PER_FPAGE;
			f_bio->is_seq = 0;
			f_bio->req_type = NVME_REQ_WRITE;
			f_bio->req_info = f_bio; /*Since the memory need to be freed */
			if(c_NvmeCtrl->ns_check == DDR_NAND){
				f_bio->ns_num = 2;
			} else if(c_NvmeCtrl->ns_check == NAND_ALONE){
				f_bio->ns_num = 1;
			}
			enqueue_bio_to_mq(f_bio, c_NvmeCtrl->nand_mq_txid);
		}
#endif

		return 0;
}

static int ualgn_bio_read(nvme_bio *bio)
{
	nvme_cache *cache = g_nvme_cache;
	unaligned_cache_ddr *cache_page = cache->cache_table;
	//LBA_Track *lba_table = c_NvmeCtrl->ls2_cache.lba_track_table;
	//LS2_Cache *ddr_cache = &c_NvmeCtrl->ls2_cache;
	int i,j=0;

	mutex_lock(&cache->f_idx_lock);
	int pages = cache->page_boundary;

	int limit = (cache->head_idx >= cache->f_idx) ? (cache->head_idx - cache->f_idx) : ((cache->head_idx + UNALIGNED_CACHE_PAGES) - cache->f_idx);
	if (limit) {
		uint64_t start_slba = bio->slba;
		uint64_t end_slba = bio->slba + ((bio->nlb - 1) << sectors_per_page_shift);
		int index = 0, idx = cache->f_idx;

		uint32_t nlb = bio->nlb;
		uint64_t *lba = malloc(sizeof(uint64_t) * bio->nlb);
		uint64_t *host_addr = NULL;

		for(i = 0; i < bio->nlb; i++) {
			lba[i] = bio->slba + (i << sectors_per_page_shift);
		}
		for(i = 0; i < limit; i++) {
			if (nlb) {
				mutex_lock(&cache_page[idx].page_lock);
				if ((cache_page[idx].lba >= start_slba) && (cache_page[idx].lba <= end_slba)) {
					index = (cache_page[idx].lba - bio->slba) >> sectors_per_page_shift;
					nlb--;
#ifndef QDMA
#if (CUR_SETUP == STANDALONE_SETUP)
					write_read_host_ls2(bio->prp[index], PAGE_SIZE, cache_page[idx].virt_addr, 0);
#else
					host_addr = (uint64_t *)(c_NvmeCtrl->host.io_mem.addr + bio->prp[index] - HOST_OUTBOUND_ADDR);
					memcpy(host_addr, cache_page[idx].virt_addr , PAGE_SIZE);
#endif
#else
					qdma_bio *q_bio = malloc (sizeof(qdma_bio));
					q_bio->req_type = NVME_REQ_READ;
					q_bio->req = (!nlb) ? bio->req_info : NULL;
					q_bio->nlb = 1;
					q_bio->prp[0] = bio->prp[index];
					q_bio->ddr_addr[0] = cache_page[idx].phy_addr;
					q_bio->size = PAGE_SIZE;
					q_bio->ualign = 0;
					q_bio->lba = 0;
					enqueue_bio_to_mq(q_bio, g_QdmaCtrl->qdma_mq_txid);
#endif
					lba[index] = INVALID;
				}
				mutex_unlock(&cache_page[idx].page_lock);
			} else {
				break;
			}
			CIRCULAR_INCR(idx, UNALIGNED_CACHE_PAGES);
		}
		//	mutex_unlock(&cache->bdry_lock);
		mutex_unlock(&cache->f_idx_lock);
		if (nlb) {
			if (nlb != bio->nlb) {
				uint64_t *slba = malloc(sizeof(uint64_t) * nlb);
				for(i = 0; i < nlb; i++) {
					for(; j < bio->nlb; j++) {
						if(lba[j] != INVALID) {
							slba[i] = lba[j];
							bio->prp[i] = bio->prp[j];
							j++;
							break;
						}
					}
				}
				//bio->n_sectors = nlb << (bio->data_shift - BDRV_SECTOR_BITS);
				bio->slba = (uint64_t)slba;
				bio->nlb = nlb;
				bio->is_seq = 0;
				enqueue_bio_to_mq(bio, c_NvmeCtrl->nand_mq_txid);
			} else {
				enqueue_bio_to_mq(bio, c_NvmeCtrl->nand_mq_txid);
			}
		} 
#if 1
		else {
			/*m*/ nvme_rw_cb(bio->req_info,0);
		}
#endif
		free(lba);
	} else {
		mutex_unlock(&cache->f_idx_lock);
		//		mutex_unlock(&cache->bdry_lock);
		enqueue_bio_to_mq(bio, c_NvmeCtrl->nand_mq_txid);
	}
	return 0;
}

static int update_cache_id(nvme_cache *cache, int status)
{
	unaligned_cache_ddr *cache_page = cache->cache_table;
	int idx = cache->tail_idx;
	int i;

	for (i = 0; i < UNALIGNED_CACHE_PAGES; i++) {
		mutex_lock(&cache_page[idx].page_lock);
		if(cache_page[idx].status & status) {
			mutex_unlock(&cache_page[idx].page_lock);
			break;
		}
		mutex_unlock(&cache_page[idx].page_lock);
		CIRCULAR_INCR(idx, UNALIGNED_CACHE_PAGES);
	}

	return idx;
}

void remove_the_hole (nvme_cache *cache)
{
	int i=0, j=0, k=0;
	uint32_t idx = cache->f_idx;
	mutex_lock(&cache->f_idx_lock);
	int limit = (cache->head_idx > cache->f_idx) ? (cache->head_idx - cache->f_idx) : ((cache->head_idx + UNALIGNED_CACHE_PAGES) - cache->f_idx);/* Don't change this */
	unaligned_cache_ddr *cache_page = cache->cache_table;
	for(i = 0; i < limit; i++){
		mutex_lock(&cache_page[idx].page_lock);
		if(cache_page[idx].status & UNALIGNED_USED) {
			mutex_unlock(&cache_page[idx].page_lock);
		} else {
			j = idx;
			CIRCULAR_INCR(j, UNALIGNED_CACHE_PAGES);
			if (j != cache->tail_idx) {
				for (k = 0; k < (limit - 1 - i) ;k++){
					mutex_lock(&cache_page[j].page_lock);

					if(cache_page[j].status & UNALIGNED_USED) {
#ifndef QDMA 
						memcpy(cache_page[idx].virt_addr,cache_page[j].virt_addr, PAGE_SIZE);
#else
						qdma_bio *q_bio = malloc (sizeof(qdma_bio));
						q_bio->req_type = NVME_REQ_READ;
						q_bio->req = NULL;
						q_bio->nlb = 1;
						q_bio->prp[0] = cache_page[idx].phy_addr;
						q_bio->ddr_addr[0] = cache_page[j].phy_addr;
						q_bio->size = PAGE_SIZE;
						q_bio->ualign = 0;
						enqueue_bio_to_mq(q_bio, g_QdmaCtrl->qdma_mq_txid);
#endif
						cache_page[idx].status = cache_page[j].status;
						cache_page[idx].lba = cache_page[j].lba;
						cache_page[j].lba = -1;
						cache_page[j].status = UNALIGNED_FREED;
						mutex_unlock(&cache_page[j].page_lock);
						break;
					}
					mutex_unlock(&cache_page[j].page_lock);
					CIRCULAR_INCR(j, UNALIGNED_CACHE_PAGES);
				}
			}
			mutex_unlock(&cache_page[idx].page_lock);
		}
		CIRCULAR_INCR(idx, UNALIGNED_CACHE_PAGES);
	}
	cache->f_idx = update_cache_id(cache, UNALIGNED_USED);
	cache->head_idx = update_cache_id(cache, UNALIGNED_FREED);
	mutex_unlock(&cache->f_idx_lock);
#ifdef QDMA
	limit = (cache->mem_head > cache->mem_tail) ? (cache->mem_head - cache->mem_tail) : ((cache->mem_head + UNALIGNED_CACHE_PAGES) - cache->mem_tail);/* Don't change this */
	idx = cache->mem_tail;
	for(i = 0; i < limit; i++){
		mutex_lock(&cache->ls2_mem_lock);
		if (cache->ls2_memory_status[idx] & MEM_USED) {
			mutex_unlock(&cache->ls2_mem_lock);
		} else {
			j = idx;
			CIRCULAR_INCR(j, UNALIGNED_CACHE_PAGES);
			if (j != cache->mem_tail) {
				for (k = 0; k < (limit - 1 - i) ;k++){
					if (cache->ls2_memory_status[j] & MEM_USED) {
						cache->ls2_memory_status[idx] = cache->ls2_memory_status[j];
						cache->ls2_memory_status[j] = MEM_FREED;
						break;
					}
					CIRCULAR_INCR(j, UNALIGNED_CACHE_PAGES);
				}
			}
			mutex_unlock(&cache->ls2_mem_lock);
		}
		CIRCULAR_INCR(idx, UNALIGNED_CACHE_PAGES);
	}
#endif
}

int trim_cache(NvmeRequest *req)
{
	nvme_bio *bio = &req->bio;
	nvme_cache *cache = g_nvme_cache;
	unaligned_cache_ddr *cache_page = cache->cache_table;
	uint32_t i, plugged=0, pages=0;
	uint32_t idx = 0;
	uint64_t start_lba = 0, end_lba = 0;


	start_lba = bio->slba;
	end_lba = bio->slba + ((bio->nlb - 1) << sectors_per_page_shift);

	mutex_lock(&cache->bdry_lock);
	pages = cache->page_boundary;
	mutex_unlock(&cache->bdry_lock);

	idx = cache->f_idx;
	for (i = 0; i < pages; i++) {
		mutex_lock(&cache_page[idx].page_lock);
		if ((cache_page[idx].lba >= start_lba) && (cache_page[idx].lba <= end_lba)){
#ifdef QDMA
				uint32_t mem_idx = (cache_page[idx].phy_addr - UNALIGNED_CACHE_PHY_ADDR) >> PAGE_SHIFT;
				mutex_lock(&cache->ls2_mem_lock);
				cache->ls2_memory_status[mem_idx] = MEM_FREED;
				mutex_unlock(&cache->ls2_mem_lock);
#endif
				cache_page[idx].status = UNALIGNED_FREED;
				cache_page[idx].lba = -1;
				mutex_lock(&cache->bdry_lock);
				cache->page_boundary--;
				mutex_unlock(&cache->bdry_lock);
				plugged++;
			}
			mutex_unlock(&cache_page[idx].page_lock);
			CIRCULAR_INCR(idx, UNALIGNED_CACHE_PAGES);
		}

		bio->req_info = NULL;
		enqueue_bio_to_mq(bio,c_NvmeCtrl->nand_mq_txid);
		mutex_lock(&cache->bdry_lock);
		pages = cache->page_boundary;
		mutex_unlock(&cache->bdry_lock);

		if (plugged) {
			if (pages) {
				remove_the_hole(cache);
			} else {
				mutex_lock(&cache->f_idx_lock);
				cache->head_idx = cache->f_idx;
				mutex_unlock(&cache->f_idx_lock);
			}
#ifdef QDMA
			idx = cache->mem_tail;
			for (i = 0; i < UNALIGNED_CACHE_PAGES; i++) {
				mutex_lock(&cache->ls2_mem_lock);
				if (cache->ls2_memory_status[idx] & MEM_FREED) {
					mutex_unlock(&cache->ls2_mem_lock);
					break;
				}
				mutex_unlock(&cache->ls2_mem_lock);
				CIRCULAR_INCR(idx, UNALIGNED_CACHE_PAGES);
			}
			cache->mem_head = idx;
#endif
		}
		//mutex_unlock(&cache->bdry_lock);
		return 0;
}

void *unalign_bio_process(void *arg)
{
	NvmeCtrl *n = (NvmeCtrl *)arg;
	int upgs = 0;
	uint64_t rev_addr;
	nvme_bio *bio = NULL;
	set_thread_affinity (3, pthread_self ());

	while(1) {

		rev_addr = 0;
		if((mq_receive (n->unalign_mq_txid, (char *)&rev_addr, sizeof (uint64_t), NULL)) > 0) {
			if ((bio = (nvme_bio *)rev_addr) != NULL) {
				if(bio->req_type == REQTYPE_WRITE) {
					upgs = bio->nlb & (KPAGES_PER_FPAGE - 1);
					if (upgs) {
						ualgn_bio_write(bio,upgs);
					} else {
						enqueue_bio_to_mq(bio, n->nand_mq_txid);
					}
				} else if(bio->req_type == REQTYPE_READ) {
					ualgn_bio_read(bio);
				} else {
#ifdef QDMA
					enqueue_bio_to_mq(bio, n->trim_mq_txid);
#else
					trim_cache((NvmeRequest *)bio->req_info);
#endif
				}
			}
		}
	}
}

int ualgn_pg_handler_init(NvmeCtrl *n)
{
	int iter, val;
	c_NvmeCtrl = n;
	g_nvme_cache = &n->cache;
	nvme_cache *cache = g_nvme_cache;
#ifdef QDMA
	g_QdmaCtrl = &n->qdmactrl;
#endif
	cache->ls2_mem_fd = open("/dev/mem", O_RDWR | O_DSYNC);
	if(cache->ls2_mem_fd < 0) {
		perror("open");
		return errno;
	}

	cache->ls2_virt_addr = (uint64_t) mmap (0, UNALIGNED_CACHE_PAGES * getpagesize(), PROT_READ | PROT_WRITE, \
						MAP_SHARED, cache->ls2_mem_fd, UNALIGNED_CACHE_PHY_ADDR);
	if(cache->ls2_virt_addr == (uint64_t)MAP_FAILED) {
		printf("Ls2 memory mapping failed\n");
		return errno;
	}

	cache->cache_table = (unaligned_cache_ddr *)calloc(1, UNALIGNED_CACHE_PAGES * sizeof(unaligned_cache_ddr));
	cache->head_idx = cache->tail_idx = 0, cache->f_idx=0,    cache->mem_head = 0 , cache->mem_tail = 0;

	for(iter = 0; iter < UNALIGNED_CACHE_PAGES ; iter++) {
		cache->cache_table[iter].lba = -1;
		cache->cache_table[iter].status = UNALIGNED_FREED;
#ifndef QDMA
		cache->cache_table[iter].phy_addr = UNALIGNED_CACHE_PHY_ADDR + (iter << PAGE_SHIFT);
#endif
		cache->cache_table[iter].virt_addr = (uint64_t *)(cache->ls2_virt_addr + (iter << PAGE_SHIFT));
#ifdef QDMA
		cache->ls2_memory_status[iter] = MEM_FREED;
		cache->ls2_phy_addr[iter] = UNALIGNED_CACHE_PHY_ADDR + (iter << PAGE_SHIFT);
#endif
		mutex_init(&cache->cache_table[iter].page_lock,NULL);
	}
	mutex_init((pthread_mutex_t *)&cache->bdry_lock,NULL);
	mutex_init((pthread_mutex_t *)&cache->f_idx_lock,NULL);
	mutex_init((pthread_mutex_t *)&cache->ls2_mem_lock,NULL);

	val = SECTORS_PER_PAGE;
	POWER_FACTER(val, sectors_per_page_shift); /* defines on SECTORS_PER_PAGE */
	val = KPAGES_PER_FPAGE;
	POWER_FACTER(val, kpages_per_fpage_shift); /* defines on  KPAGES_PER_FPAGE */

	return 0;
}

int ualgn_pg_handler_deinit(nvme_cache *cache)
{
	int iter;
	for(iter = 0; iter < UNALIGNED_CACHE_PAGES ; iter++) {
		mutex_destroy(&cache->cache_table[iter].page_lock);
	}
	mutex_destroy((pthread_mutex_t *)&cache->bdry_lock);
	mutex_destroy((pthread_mutex_t *)&cache->f_idx_lock);
	mutex_destroy((pthread_mutex_t *)&cache->ls2_mem_lock);
	free((void *)cache->cache_table);
	munmap((void *)cache->ls2_virt_addr,UNALIGNED_CACHE_PAGES * getpagesize());
	close(cache->ls2_mem_fd);
	return 0;
}
