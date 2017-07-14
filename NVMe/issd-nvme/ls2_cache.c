#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "nvme.h"
#include "common.h"
#include "nvme_bio.h"
#include "ls2_cache.h"
#include "qdma.h"
#include "qdma_user_space_lib.h"

#ifdef DDR_CACHE

#ifdef QDMA
QdmaCtrl *g_QdmaCtrl;
#endif

LS2_Cache *g_LS2_Cache;

extern int sectors_per_page_shift;
extern void enqueue_bio_to_mq(void *bio, int mq_txid);
extern int write_read_host_ls2(uint64_t prp, uint64_t len, uint64_t *cache_addr, int is_write);

static int is_cache_available(LS2_Cache *cache, uint16_t nlb)
{
	int ret = 1;
	uint32_t head_idx = cache->cache_head_idx;
	uint32_t tail_idx = cache->cache_tail_idx;
	tail_idx = (tail_idx > head_idx) ? tail_idx : (tail_idx + cache->nr_pages_in_cache);

	if ((tail_idx - head_idx) < nlb) {
		ret = 0;
	}
		
	return ret;
}

static inline void update_qDMA_status(LS2_Cache *cache, uint16_t nlb)
{
	int i;
	uint32_t head_idx = cache->qDMA_head_idx;
	uint32_t nr_pages = cache->nr_pages_in_cache;
	uint8_t *qdma_status = cache->qDMA_status;

	mutex_lock(&cache->qDMA_mutex);
	for (i=0; i<nlb; i++) {
		qdma_status[head_idx] = COPIED;
		CIRCULAR_INCR (head_idx, nr_pages);
	}
	mutex_unlock(&cache->qDMA_mutex);
	cache->qDMA_head_idx = head_idx;
}

static void write_to_ddr_cache(LS2_Cache *cache, nvme_bio *bio, uint64_t host_mem_addr, uint64_t lba)
{
	int i = 0;
	uint32_t nr_pages = cache->nr_pages_in_cache;
	LBA_Track *lba_table = cache->lba_track_table;
	uint64_t ls2_addr = cache->ls2_ddr_addr;
	uint64_t *ls2_cache_addr = NULL;
	uint64_t *host_addr = NULL;
	uint64_t size = bio->size;
	uint64_t sz = 0;
	uint64_t idx = cache->cache_head_idx;
	Cache_DDR *ddr_cache = cache->ddr_table;

	for (i = 0; i < bio->nlb; i++, lba++) {
		ls2_cache_addr = (uint64_t *)(ls2_addr + (idx << PAGE_SHIFT));
		sz = MIN(PAGE_SIZE, size);
#if (CUR_SETUP == STANDALONE_SETUP)
		write_read_host_ls2(bio->prp[i], sz, ls2_cache_addr, 1);
#else
		host_addr = (uint64_t *)(host_mem_addr + bio->prp[i] - HOST_OUTBOUND_ADDR);
		memcpy(ls2_cache_addr, host_addr, sz);
#endif
		size -= sz;

		lba_table[lba].status =  LBA_IN_CACHE;
		lba_table[lba].cache_idx = idx;

		mutex_lock(&cache->cache_ddr_mutex);
		ddr_cache[idx].status = USED;
		ddr_cache[idx].lba = lba;
		ddr_cache[idx].bio = NULL;
		bio->prp[i] = ddr_cache[idx].phy_addr;
		mutex_unlock(&cache->cache_ddr_mutex);

		CIRCULAR_INCR (idx, nr_pages);
	}
	cache->cache_head_idx = idx;
	idx = (idx) ? idx : nr_pages;
	mutex_lock(&cache->cache_ddr_mutex);
	ddr_cache[idx - 1].bio = bio;
	mutex_unlock(&cache->cache_ddr_mutex);
	update_qDMA_status(cache, bio->nlb);
}

static void read_from_cache(LS2_Cache *cache, nvme_bio *bio, uint64_t host_mem_addr, uint64_t lba)
{
	LBA_Track *lba_table = cache->lba_track_table;
	uint64_t size = bio->size;
	uint64_t sz = 0;
	int i;

#ifdef QDMA
	qdma_bio *q_bio = malloc (sizeof(qdma_bio));
	q_bio->size = bio->size;
	q_bio->req_type = REQTYPE_READ;
	q_bio->req = bio->req_info;
	q_bio->nlb = bio->nlb;

	for (i=0; i<bio->nlb; i++) {
		q_bio->prp[i] = bio->prp[i];
		q_bio->ddr_addr[i] = DDR_CACHE_MEM_ADDR + ((uint64_t)lba_table[lba + i].cache_idx << PAGE_SHIFT);
	}	
	enqueue_bio_to_mq(q_bio, g_QdmaCtrl->qdma_mq_txid);
#else
	uint64_t *ls2_cache_addr = NULL;
	uint64_t *host_addr = NULL;

	for (i=0; i<bio->nlb; i++) {
		ls2_cache_addr = (uint64_t *)(cache->ls2_ddr_addr + ((uint64_t)lba_table[lba + i].cache_idx << PAGE_SHIFT));
		sz = MIN(PAGE_SIZE, size);
#if (CUR_SETUP == STANDALONE_SETUP)
		write_read_host_ls2(bio->prp[i], sz, ls2_cache_addr, 0);
#else
		host_addr = (uint64_t *)(host_mem_addr + bio->prp[i] - HOST_OUTBOUND_ADDR);
		memcpy(host_addr, ls2_cache_addr, sz);
#endif
		size -= sz;
	}	
#endif
}

int process_write(NvmeCtrl *n, nvme_bio *bio)
{
	int i;
	LS2_Cache *cache = &n->ls2_cache;
	Cache_DDR *ddr_cache = cache->ddr_table;
	LBA_Track *lba_table = cache->lba_track_table;

	uint64_t lba = bio->f_lba;

	if (is_cache_available(cache, bio->nlb)) {
		mutex_lock(&cache->lba_track_table_mutex);
		if (lba_table[lba].status & LBA_AVAIL_IN_CACHE) {
			uint64_t r_lba = lba;
			uint32_t cache_idx = 0;
			mutex_lock(&cache->cache_ddr_mutex);
			for (i=0; i<bio->nlb; i++, r_lba++) {
					cache_idx = lba_table[r_lba].cache_idx;
					ddr_cache[cache_idx].status = NOT_USED;
					ddr_cache[cache_idx].lba = INVALID;
			}
			mutex_unlock(&cache->cache_ddr_mutex);
		}
#ifndef QDMA
		write_to_ddr_cache(cache, bio, n->host.io_mem.addr, lba);
#endif
		mutex_unlock(&cache->lba_track_table_mutex);
#ifdef QDMA
		Cache_DDR *ddr_cache = cache->ddr_table;
		uint64_t idx = cache->cache_head_idx;
		uint64_t phy_addr = 0;
		uint64_t nr_pages = cache->nr_pages_in_cache;

		qdma_bio *q_bio = malloc (sizeof(qdma_bio));
		q_bio->size = bio->size;
		q_bio->req_type = REQTYPE_WRITE;
		q_bio->req = bio->req_info;
		q_bio->nlb = bio->nlb;

		for (i = 0; i < bio->nlb; i++) {
			mutex_lock(&cache->cache_ddr_mutex);
			phy_addr = ddr_cache[idx].phy_addr;
			ddr_cache[idx].status = USED;
			mutex_unlock(&cache->cache_ddr_mutex);
			q_bio->prp[i] = bio->prp[i];
			q_bio->ddr_addr[i] = phy_addr;
			bio->prp[i] = phy_addr;
			CIRCULAR_INCR (idx, nr_pages);
		}
		enqueue_bio_to_mq(q_bio, g_QdmaCtrl->qdma_mq_txid);
		cache->cache_head_idx = idx;
#endif
	} else {
		uint64_t r_lba = lba;
		mutex_lock(&cache->lba_track_table_mutex);
		for (i=0; i<bio->nlb; i++, r_lba++) {
			lba_table[r_lba].cache_idx = INVALID;
			lba_table[r_lba].status = LBA_IN_NAND;
		}
		mutex_unlock(&cache->lba_track_table_mutex);
		enqueue_bio_to_mq(bio, n->nand_mq_txid);
	}
	return 0;
}

int process_read(NvmeCtrl *n, nvme_bio *bio)
{
	int i, j=0;
	int prp_idx = 0;
	LS2_Cache *cache = &n->ls2_cache;
	LBA_Track *lba_table = cache->lba_track_table;
	NvmeRequest *req = bio->req_info;

	uint64_t lba = bio->f_lba;
	uint64_t aio_slba = lba;
	uint32_t nlb = bio->nlb - 1;
	uint32_t count = 0;
	mutex_lock(&cache->lba_track_table_mutex);	
	int status = lba_table[aio_slba].status;
	mutex_unlock(&cache->lba_track_table_mutex);
	i = 1;
	do {
		count = 1;
		mutex_lock(&cache->lba_track_table_mutex);	
		for (;i<bio->nlb; i++, count++, nlb--) {
			if(status != lba_table[lba + i].status) {
				nlb--;
				break;
			}
		}
		mutex_unlock(&cache->lba_track_table_mutex);

		if (status & LBA_IN_CACHE) {
#ifdef QDMA
			while (!avail_qdma_desc(bio->nlb)) { usleep(5); }
#endif
			if (count == bio->nlb) {
				mutex_lock(&cache->lba_track_table_mutex);
				read_from_cache(cache, bio, n->host.io_mem.addr, lba);
				mutex_unlock(&cache->lba_track_table_mutex);
#ifndef QDMA
				nvme_rw_cb(req, 0);
#endif
			} else {
				nvme_bio *new_bio = (nvme_bio *)malloc(sizeof(nvme_bio));
				if (nlb) {
					new_bio->req_info = NULL;
					new_bio->size = count << PAGE_SHIFT;
					bio->size -= new_bio->size; 
				} else {
					new_bio->req_info = req;
					new_bio->size = MIN(count << PAGE_SHIFT, bio->size);
				}

				new_bio->nlb = count;
				for(j=0;j<count;j++) {
					new_bio->prp[j] = bio->prp[prp_idx+j];
				}

				mutex_lock(&cache->lba_track_table_mutex);
				read_from_cache(cache, new_bio, n->host.io_mem.addr, aio_slba);
				mutex_unlock(&cache->lba_track_table_mutex);
#ifndef QDMA
				if (!nlb) {
					nvme_rw_cb(req, 0);
				}
#endif
				free(bio);
			}
		} else if ((status & LBA_IN_UNALIGNED_CACHE)) {
			unaligned_cache_ddr *cache_page = n->cache.cache_table;
			int index = 0, k = 0;
			uint64_t size = 0;
#ifdef QDMA
			NvmeRequest *req = (count == bio->nlb) ? bio->req_info : NULL;
			while (!avail_qdma_desc(req->nlb)) { usleep(5); }
			qdma_bio *q_bio = malloc (sizeof(qdma_bio));
			q_bio->size = 0;
			q_bio->req_type = REQTYPE_READ;
			q_bio->req = req;
			q_bio->nlb = 0;
			int ct = 0;
#endif
			for(j = 0; j < count; j++) {
				for(k = 0; k < UNALIGNED_CACHE_PAGES ; k++) {
					
					mutex_lock(&cache_page[k].page_lock);
					if (cache_page[k].slba == (aio_slba + j)) {
						index = (cache_page[k].slba - lba);
						size = MIN(PAGE_SIZE, bio->size);
#ifndef QDMA
#if (CUR_SETUP == STANDALONE_SETUP)
						write_read_host_ls2(bio->prp[index], size, (uint64_t *)cache_page[j].virt_addr, 0);
#else
						uint64_t *host_addr = (uint64_t *)(n->host.io_mem.addr + \
								bio->prp[index] - HOST_OUTBOUND_ADDR);
						memcpy(host_addr, cache_page[k].virt_addr, size);
#endif
#else
						q_bio->prp[ct] = bio->prp[index];
						q_bio->ddr_addr[ct] = cache_page[k].phy_addr;
						q_bio->size += size;
						bio->size -= size;
						q_bio->nlb++;
						ct++; 
#endif
						mutex_unlock(&cache_page[k].page_lock);
						break;
					}
					mutex_unlock(&cache_page[k].page_lock);
				}
			}
#ifdef QDMA
			enqueue_bio_to_mq(q_bio, g_QdmaCtrl->qdma_mq_txid);
#else
			if (!nlb) {
				nvme_rw_cb(req, 0);
			}
#endif
		} else {
			if (count != bio->nlb) {
				nvme_bio *new_bio = (nvme_bio *)malloc(sizeof(nvme_bio));
				if (nlb) {
					new_bio->req_info = NULL;
					new_bio->size = count << PAGE_SHIFT;
					bio->size -= new_bio->size; 
				} else {
					new_bio->req_info = req;
					new_bio->size = MIN(count << PAGE_SHIFT, bio->size);
				}
				new_bio->req_type = REQTYPE_READ;
				new_bio->ns_num = bio->ns_num;
				//new_bio->n_sectors = count << (bio->data_shift - BDRV_SECTOR_BITS);
				new_bio->slba = aio_slba << (bio->data_shift - BDRV_SECTOR_BITS);
				new_bio->nlb = count;
				new_bio->is_seq = 1;

				for(j=0; j<count; j++) {
					new_bio->prp[j] = bio->prp[prp_idx + j];
				}
				enqueue_bio_to_mq(new_bio, n->nand_mq_txid);
			} else {
				enqueue_bio_to_mq(bio, n->nand_mq_txid);
			}
		}
		aio_slba = lba + i;
		status = lba_table[aio_slba].status;
		prp_idx = i;
		i++;
	} while (nlb);
	return 0;
}

void add_to_list(LS2_Cache *cache, nvme_bio *bio)
{
	bio_list *new = (bio_list *)malloc(sizeof(bio_list));

	new->bio = (nvme_bio *)malloc(sizeof(nvme_bio));
	memcpy(new->bio, bio, sizeof(nvme_bio));

	new->next = NULL;
	if (cache->tail == NULL) {
		cache->head = cache->tail = new;
	} else {
		cache->tail->next = new;
		cache->tail = new;
	}
}

nvme_bio *remove_from_list(LS2_Cache *cache)
{
	bio_list *temp = NULL;
	nvme_bio *bio = NULL;

	if (cache->head != NULL) {
		temp = cache->head;
		bio = temp->bio;
		if (cache->head->next != NULL) {
			cache->head = cache->head->next;
		} else {
			cache->head = cache->tail = NULL;
		}
		free(temp);
	}
	return bio;
}

void update_lba_table(LS2_Cache *cache, nvme_bio *bio, uint64_t idx)
{

	LBA_Track *lba_table = cache->lba_track_table;
	int i = 0;

	if (bio->req_type == REQTYPE_WRITE) {
		uint64_t lba = bio->f_lba;
		uint64_t nr_pages = cache->nr_pages_in_cache;

		mutex_lock(&cache->lba_track_table_mutex);
		for (i=0; i<bio->nlb; i++, lba++) {
			lba_table[lba].status =  LBA_IN_CACHE;
			lba_table[lba].cache_idx = idx;

			CIRCULAR_INCR (idx, nr_pages);
		}
		mutex_unlock(&cache->lba_track_table_mutex);
		add_to_list(cache, bio);
	}
}

void flash_to_nand(LS2_Cache *cache, nvme_bio *bio, int mq_txid)
{
	LBA_Track *lba_table = cache->lba_track_table;
	int i = 0, status = LBA_IN_CACHE;
	uint64_t lba = bio->f_lba;
	uint32_t ddr_idx = (bio->prp[0] - DDR_CACHE_MEM_ADDR) >> PAGE_SHIFT;

	mutex_lock(&cache->lba_track_table_mutex);
	for (i=0; i<bio->nlb; i++) {
		if (lba_table[lba + i].status != LBA_IN_CACHE) {
			status = lba_table[lba + i].status;
			break;
		}
	}

	if ((status & LBA_IN_CACHE) && (ddr_idx == lba_table[lba].cache_idx)) {
		for (i=0; i<bio->nlb; i++) {
			lba_table[lba + i].status = LBA_IN_CACHE_SEND_NAND;
		}
		mutex_unlock(&cache->lba_track_table_mutex);
		bio->req_info = NULL;
		mutex_lock(&cache->cached_bio_mutex);
		cache->nr_cached_bio_to_nand++;
		mutex_unlock(&cache->cached_bio_mutex);
		enqueue_bio_to_mq(bio, mq_txid);
	} else {
		mutex_unlock(&cache->lba_track_table_mutex);

		Cache_DDR *ddr_cache = cache->ddr_table;
		mutex_lock(&cache->cache_ddr_mutex);
		for (i=0; i<bio->nlb; i++) {
			ddr_cache[ddr_idx].status = NOT_USED;
			CIRCULAR_INCR(ddr_idx, cache->nr_pages_in_cache);
		}
		mutex_unlock(&cache->cache_ddr_mutex);
		free(bio);
	}
}

void nvme_cache_cb(nvme_bio *bio, int ret)
{
	int i = 0, status = LBA_IN_CACHE_SEND_NAND;
	LS2_Cache *cache = g_LS2_Cache;
	Cache_DDR *ddr_cache = cache->ddr_table;
	LBA_Track *lba_table = cache->lba_track_table;
	uint64_t lba = bio->f_lba;
	uint32_t ddr_idx = (bio->prp[0] - DDR_CACHE_MEM_ADDR) >> PAGE_SHIFT;

	uint32_t ddr_cache_idx[bio->nlb];

	mutex_lock(&cache->cached_bio_mutex);
	cache->nr_cached_bio_to_nand--;
	mutex_unlock(&cache->cached_bio_mutex);

	mutex_lock(&cache->lba_track_table_mutex);
	for (i=0; i<bio->nlb; i++) {
		if (lba_table[lba + i].status != status) {
			status = lba_table[lba + i].status;
			break;
		}
	}
	if ((status & LBA_IN_CACHE_SEND_NAND) && (ddr_idx == lba_table[lba].cache_idx)) {
		for (i=0; i<bio->nlb; i++, lba++) {
#if 0
			lba_table[lba + i].status = LBA_IN_CACHE_NAND;
#else
			lba_table[lba].status = LBA_IN_NAND;
#endif
			ddr_cache_idx[i] = lba_table[lba].cache_idx;
		}
		mutex_unlock(&cache->lba_track_table_mutex);

		mutex_lock(&cache->cache_ddr_mutex);
		for (i=0; i<bio->nlb; i++) {
			ddr_cache[ddr_cache_idx[i]].status = FLUSHED;
		}
		mutex_unlock(&cache->cache_ddr_mutex);
	} else if (status & LBA_TRIMMED) {
		for (i=0; i<bio->nlb; i++, lba++) {
			lba_table[lba].status = LBA_FREE;
			ddr_cache_idx[i] = lba_table[lba].cache_idx;
			lba_table[lba].cache_idx = INVALID;
		}
		mutex_unlock(&cache->lba_track_table_mutex);

		mutex_lock(&cache->cache_ddr_mutex);
		for (i=0; i<bio->nlb; i++) {
			ddr_cache[ddr_cache_idx[i]].status = NOT_USED;
		}
		mutex_unlock(&cache->cache_ddr_mutex);
	} else {
		mutex_unlock(&cache->lba_track_table_mutex);
		mutex_lock(&cache->cache_ddr_mutex);
		for (i=0; i<bio->nlb; i++) {
			ddr_cache[ddr_idx].status = NOT_USED;
			CIRCULAR_INCR(ddr_idx, cache->nr_pages_in_cache);
		}
		mutex_unlock(&cache->cache_ddr_mutex);
	}
	uint32_t idx = cache->cache_tail_idx;
	for (i=0; i<cache->nr_pages_in_cache;i++) {
		if (ddr_cache[idx].status == USED) {
			break;
		}
		CIRCULAR_INCR(idx, cache->nr_pages_in_cache);
	}
	cache->cache_tail_idx = idx;
	
	free(bio);
}

void lba_flush_to_nand()
{
	int i = 0;
	LS2_Cache *cache = g_LS2_Cache;

	do {
		mutex_lock(&cache->cached_bio_mutex);
		i = cache->nr_cached_bio_to_nand;
		mutex_unlock(&cache->cached_bio_mutex);
		if (!i) {
			break;
		}
		usleep(5);
	} while (1);
}

int delete_cache(NvmeCtrl *n, nvme_bio *bio)
{
	int i=0;
	int status = 0, ddr_cache_idx = 0;
	int j=0, k=0, count=0;
	uint32_t nlb;
	uint64_t aio_slba;
	LS2_Cache *cache = &n->ls2_cache;
	LBA_Track *lba_table = cache->lba_track_table;
	Cache_DDR *ddr_cache = cache->ddr_table;
	nvme_cache *unal_cache = &n->cache;
	unaligned_cache_ddr *cache_page = unal_cache->cache_table;

	aio_slba = bio->f_lba;
	nlb = bio->nlb - 1;
	trim_cache(NULL);
	mutex_lock(&cache->lba_track_table_mutex);	
	status = lba_table[aio_slba].status;
	mutex_unlock(&cache->lba_track_table_mutex);
	i = 1;
	do {
		count = 1;
		nvme_bio *r_bio = (nvme_bio *)malloc(sizeof(nvme_bio));
		r_bio->req_info = NULL;
		r_bio->req_type =	REQTYPE_TRIM;
		r_bio->ns_num = bio->ns_num;
		mutex_lock(&cache->lba_track_table_mutex);	
		for (;i<bio->nlb; i++, count++, nlb--) {
			if(status != lba_table[bio->f_lba + i].status) {
				nlb--;
				break;
			}
		}
		if ((status & LBA_IN_CACHE) || (status & LBA_IN_CACHE_NAND)) {
			mutex_lock(&cache->cache_ddr_mutex);
			for(j=0; j<count; j++) {
				lba_table[(aio_slba + j)].status = LBA_FREE;
				ddr_cache_idx = lba_table[aio_slba + j].cache_idx;
				ddr_cache[ddr_cache_idx].status = NOT_USED;
				ddr_cache[ddr_cache_idx].lba = INVALID;
				lba_table[(aio_slba + j)].cache_idx = INVALID;
			}
			mutex_unlock(&cache->cache_ddr_mutex);
		} else if ((status & LBA_IN_CACHE_SEND_NAND)) {
			for(j=0; j<count; j++) {
				lba_table[(aio_slba + j)].status = LBA_TRIMMED;
			}
		}
		mutex_unlock(&cache->lba_track_table_mutex);
		if ((status & LBA_IN_NAND) || (status & LBA_IN_CACHE_NAND) || (status & LBA_IN_CACHE_SEND_NAND)) {
			//r_bio->n_sectors = count << (bio->data_shift - BDRV_SECTOR_BITS);
			r_bio->slba = (aio_slba << (bio->data_shift - BDRV_SECTOR_BITS));
			r_bio->nlb = count;
			r_bio->is_seq = 1;
			enqueue_bio_to_mq(r_bio, n->nand_mq_txid);
		}
		aio_slba = bio->f_lba + i;
		status = lba_table[aio_slba].status;
		i++;
	} while (nlb);
	return 0;
}

int nvme_init_ls2_cache(NvmeCtrl *n, uint64_t nand_capacity)
{
	uint64_t i;
	printf("\t%s\n",__func__);
	g_LS2_Cache = &n->ls2_cache;
	LS2_Cache *cache = g_LS2_Cache;
#ifdef QDMA
	g_QdmaCtrl = &n->qdmactrl;
#endif

	cache->ls2_mem_fd = open(DEV_MEM, O_RDWR);
	if(cache->ls2_mem_fd < 0) {
		perror("open");
		return errno;
	}

	cache->ls2_ddr_addr = (uint64_t)mmap (0, LS2_DDR_CACHE_SIZE, PROT_READ | PROT_WRITE, \
					      MAP_SHARED, cache->ls2_mem_fd, DDR_CACHE_MEM_ADDR);
	if(cache->ls2_ddr_addr == (uint64_t)MAP_FAILED) {
		printf("Ls2 memory mapping failed\n");
		return errno;
	}

	cache->lba_track_table = (LBA_Track *)calloc(1, (nand_capacity >> PAGE_SHIFT) * sizeof(LBA_Track));
	cache->nr_pages_in_cache = LS2_DDR_CACHE_SIZE >> PAGE_SHIFT;
	cache->ddr_table = (Cache_DDR *)calloc(1, cache->nr_pages_in_cache * sizeof(Cache_DDR));
	cache->qDMA_status = (uint8_t *)calloc(1, cache->nr_pages_in_cache * sizeof(uint8_t));

	memset(cache->lba_track_table, 0, sizeof(*cache->lba_track_table));

	for (i=0; i<cache->nr_pages_in_cache; i++) {
		cache->ddr_table[i].status = NOT_USED;
		cache->ddr_table[i].lba = INVALID;
		cache->ddr_table[i].bio = NULL;
		cache->ddr_table[i].phy_addr = DDR_CACHE_MEM_ADDR + (i << PAGE_SHIFT);
		cache->qDMA_status[i] = FREE;
	}

	for (i=0; i<(nand_capacity >> PAGE_SHIFT); i++) {
		cache->lba_track_table[i].cache_idx = INVALID;
		cache->lba_track_table[i].status = LBA_FREE;
	}

	cache->cache_head_idx = 0;
	cache->qDMA_head_idx = 0;
	cache->qDMA_tail_idx = 0;
	cache->head = NULL;
	cache->tail = NULL;

	mutex_init(&cache->lba_track_table_mutex, NULL);
	mutex_init(&cache->cache_ddr_mutex, NULL);
	mutex_init(&cache->qDMA_mutex, NULL);
	printf("\t%s\n","done");
	return 0;
}

void nvme_deinit_ls2_cache(LS2_Cache *cache)
{
	free((void *)cache->ddr_table);
	free((void *)cache->lba_track_table);
	free((void *)cache->qDMA_status);
	mutex_destroy(&cache->lba_track_table_mutex);
	mutex_destroy(&cache->cache_ddr_mutex);
	mutex_destroy(&cache->qDMA_mutex);
	close(cache->ls2_mem_fd);
}
#endif
