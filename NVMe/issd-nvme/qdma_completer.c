#include<stdio.h>
#include <stdint.h>
#include<signal.h>
#include<sys/mman.h>
#include <mqueue.h>
#include <stdlib.h>
#include <unistd.h>
#include<sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include "nvme.h"
#include"qdma.h"
#include"qdma_user_space_lib.h"
#include "ls2_cache.h"

#define TIMEOUT 10000000 

#ifdef QDMA

QdmaCtrl *g_QdmaCtrl;

void init_qdma_desc(NvmeCtrl *n)
{
	int i = 0;
	QdmaCtrl *qdma = &n->qdmactrl;
	g_QdmaCtrl = qdma;

	for(i=0;i<IO_TABLE_SIZE;i++) {
		qdma->des_io_table[i].status = QDMA_AVAIL;
		mutex_init((pthread_mutex_t *)&qdma->des_io_table[i].qdesc_mutex, NULL);
	}
	qdma->producer_index = 0;
	qdma->consumer_index = 0;
	qdma->qdma_io_posted = 0;
	qdma->qdma_io_completed = 0;
	qdma->pi_cnt = 0;
	qdma->ci_cnt = 0;
	qdma->dev_error = 0;
	mutex_init(&qdma->qdma_mutex, NULL);
}

int ramdisk_prp_rw_prologue(NvmeRequest *req, qdma_bio *q_bio)
{
	QdmaCtrl *qdma = g_QdmaCtrl;

	struct qdma_io *temp = (struct qdma_io *)(&qdma->des_io_table[qdma->producer_index]);

	mutex_lock(&temp->qdesc_mutex);
	if(temp->status == QDMA_AVAIL) {
		temp->count = 0;
		temp->req = (void *)req;
		temp->q_bio = q_bio;
		//syslog(LOG_INFO, "pro %d %p\n", qdma->producer_index, q_bio);
		return 1;
	}
	mutex_unlock(&temp->qdesc_mutex);

	return 0;
}

int avail_qdma_desc(uint32_t nlb)
{
	QdmaCtrl *qdma = g_QdmaCtrl;

	int diff = qdma->producer_index - qdma->consumer_index;

	diff = (diff < 0) ? (-1 * diff) : diff;
	if ((128 - diff) >= nlb) {
		return 1;
	}
	return 0;
}

void ramdisk_prp_rw_epilogue()
{
	QdmaCtrl *qdma = g_QdmaCtrl;
	struct qdma_io *temp = (struct qdma_io *)(&qdma->des_io_table[qdma->producer_index]);

	gettimeofday((struct timeval *)(&temp->io_post_time), NULL);
	temp->status = QDMA_POSTED;
	mutex_unlock(&temp->qdesc_mutex);
	CIRCULAR_INCR(qdma->producer_index, IO_TABLE_SIZE);

	qdma->qdma_io_posted++;

}

long long int diff_time(struct timeval *newtime,struct timeval *oldtime)
{
	long long int var1,var2,diff;
	var1 = 1000000*(newtime->tv_sec) + (newtime->tv_usec);
	var2 = 1000000*(oldtime->tv_sec) + (oldtime->tv_usec);

	if(var1 >= var2) {
		diff = var1 - var2;
	}
	else {
		diff = var2 - var1;
	}
	return diff;
}

void *qdma_handler(void *arg)
{
	NvmeCtrl *n = (NvmeCtrl *)arg;
	QdmaCtrl *qdma = &n->qdmactrl;
	qdma_bio *q_bio = NULL;
	//NvmeRequest *req = NULL;
	uint64_t rev_addr;
	int i;
	set_thread_affinity (6, pthread_self ());

	while(1) {

		rev_addr = 0;
		if((mq_receive (qdma->qdma_mq_txid, (char *)&rev_addr, sizeof (uint64_t), NULL)) > 0) {
			q_bio = NULL;
			if ((q_bio = (qdma_bio *)rev_addr) != NULL) {
				while(!ramdisk_prp_rw_prologue((NvmeRequest *)q_bio->req, (q_bio->ualign ? q_bio : NULL))) { 
					usleep(1000);
				}
				for (i = 0; i < q_bio->nlb; i++) {
					ramdisk_prp_rw ((void *)q_bio->prp[i], (uint64_t)(q_bio->ddr_addr[i]), q_bio->req_type, MIN(q_bio->size, PAGE_SIZE));
					q_bio->size -= MIN(q_bio->size, PAGE_SIZE);
				}
				ramdisk_prp_rw_epilogue();
			}
			//free(q_bio);
			//usleep(1);
		}
	}
}

void alarm_handler(int sig)
{
	int i = 0;
	QdmaCtrl *qdma = g_QdmaCtrl;


	for(i = 0;i < QDMA_DESC_COUNT;i++) {
		//syslog(LOG_INFO,"size_counters[%d]: %d\n",i,size_counters[i]);
	}
	alarm(50);
}

void display()
{
	QdmaCtrl *qdma = g_QdmaCtrl;
	int i = 0;
	int c = qdma->consumer_index;

//	printf("disp: count: %d\n",qdma->des_io_table[c].count);
	for(i = 0;i < qdma->des_io_table[c].count;i++) {
/*		printf("src: %lx, dest: %lx, len: %x, status: %d\n", 
				qdma->des_io_table[c].q_desc[i]->src_addr,qdma->des_io_table[c].q_desc[i]->dest_addr, 
				qdma->des_io_table[c].q_desc[i]->len,qdma->des_io_table[c].q_desc[i]->status);
*/
	}

}

void cleanup_io(struct qdma_io *temp, int start)
{
	int i = 0;
	QdmaCtrl *qdma = g_QdmaCtrl;

	for(i = start;i < temp->count; i++) {
		qdma_desc_release(temp->q_desc[i]);
	}
	qdma->ci_cnt += temp->count;
}

int check_status(struct qdma_io *temp)
{
	int i;
	for(i = 0; i < temp->count; i++) {
		if(temp->q_desc[i]->status != DMA_DONE) {
			return -1;
		}
	}
	return 0;
}
#endif

void memory_copy(uint64_t src_addr, uint64_t dest_addr, uint32_t len)
{
	uint64_t *saddr = NULL, *daddr = NULL;

	int fd = open (DEV_MEM, O_RDWR | O_SYNC);
	if (fd < 0) {
		perror(DEV_MEM);
	}

	saddr = (uint64_t *)mmap (0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, src_addr);
	if(saddr == (uint64_t *)MAP_FAILED) {
		perror("mmap");
		close(fd);
		fd = 0;
	}
	daddr = (uint64_t *)mmap (0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, dest_addr);
	if(saddr == (uint64_t *)MAP_FAILED) {
		perror("mmap");
		close(fd);
		fd = 0;
	}
	memcpy(daddr, saddr, len);
	munmap ((void*)((uint64_t)saddr & 0xFFFFFFFFFFFFF000), getpagesize());
	munmap ((void*)((uint64_t)daddr & 0xFFFFFFFFFFFFF000), getpagesize());
	close (fd);
}


void *qdma_completer(void *arg)
{
	NvmeCtrl *n = (NvmeCtrl *)arg;
	int ret = 0, i=0;

	set_thread_affinity(6, pthread_self());

#ifdef QDMA
	QdmaCtrl *qdma = &n->qdmactrl;
	struct qdma_io *temp = NULL;
	int sleep_time = 1;
	long long int diff = 0;
	int timedout,io_completed;
	uint32_t f_idx = 0;
	NvmeRequest *req = NULL;
	nvme_cache *cache = &n->cache;
	unaligned_cache_ddr *cache_page = cache->cache_table;
	uint32_t val = KPAGES_PER_FPAGE;
	uint32_t kpages_per_fpage_shift = 0;
	POWER_FACTER(val, kpages_per_fpage_shift); /* defines on  KPAGES_PER_FPAGE */
	struct mq_attr attr;
	uint64_t rev_addr = 0; 

	init_qdma_desc(n);
	signal(SIGALRM,alarm_handler);
	alarm(20);
#endif

	while(1) {

#ifdef QDMA
		temp = (struct qdma_io *)(&qdma->des_io_table[qdma->consumer_index]);
		mutex_lock(&temp->qdesc_mutex);
		if (temp->status == QDMA_POSTED) {
			do {
				io_completed = !check_status(temp);
				if (io_completed) break;
				gettimeofday(&qdma->time2,NULL);
				diff = diff_time(&qdma->time2,(struct timeval *)&(temp->io_post_time));
				timedout = diff > TIMEOUT ? 1 : 0 ;
				usleep(sleep_time);
			} while (!timedout && !io_completed);/*try until timedout and io is not completed*/
			ret = 0;
			if(!io_completed) {
				qdma->dev_error++;            
				display();
				for (i=0; i<temp->count; i++) {
					if(temp->q_desc[i]->status != DMA_DONE) {
						memory_copy(temp->q_desc[i]->src_addr, temp->q_desc[i]->dest_addr, temp->q_desc[i]->len);
						temp->q_desc[i]->status = DMA_DONE;
					}
				}
			}
			cleanup_io(temp, 0);
			req = (temp->req) ? (NvmeRequest *)temp->req : NULL;
			qdma_bio *q_bio = (qdma_bio *)temp->q_bio;
			if (q_bio && q_bio->ualign) {
				//uint64_t *lba = malloc(sizeof(uint64_t) * q_bio->nlb);
				mutex_lock(&cache->f_idx_lock);
				uint32_t idx = cache->head_idx;
				for (i = 0; i < q_bio->nlb; i++) {
					mutex_lock(&cache_page[idx].page_lock);
					cache_page[idx].lba = q_bio->lba[i];
					cache_page[idx].status = UNALIGNED_USED;
					cache_page[idx].phy_addr = q_bio->ddr_addr[i];
					mutex_unlock(&cache_page[idx].page_lock);
					CIRCULAR_INCR(idx, UNALIGNED_CACHE_PAGES);
				}
				mutex_lock(&cache->bdry_lock);
				cache->page_boundary+=q_bio->nlb;
				mutex_unlock(&cache->bdry_lock);
                cache->head_idx = idx;
				mutex_unlock(&cache->f_idx_lock);
				free(q_bio->lba);
				free(q_bio);
#if 1
				if(cache->page_boundary >= KPAGES_PER_FPAGE) {
					nvme_bio *f_bio = malloc(sizeof(nvme_bio));
					uint64_t *lba = malloc(sizeof(uint64_t) << kpages_per_fpage_shift);
					mutex_lock(&cache->f_idx_lock);
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
					mutex_unlock(&cache->f_idx_lock);

					mutex_lock(&cache->bdry_lock);
					cache->page_boundary = cache->page_boundary - KPAGES_PER_FPAGE;
					mutex_unlock(&cache->bdry_lock);
					f_bio->slba = (uint64_t)lba;
					//mutex_unlock(&cache->bdry_lock);
					f_bio->nlb = KPAGES_PER_FPAGE;
					f_bio->is_seq = 0;
					f_bio->req_type = NVME_REQ_WRITE;
					f_bio->req_info = f_bio; /*Since the memory need to be freed */
					if(n->ns_check == DDR_NAND){
						f_bio->ns_num = 2;
					} else if(n->ns_check == NAND_ALONE){
						f_bio->ns_num = 1;
					}
					enqueue_bio_to_mq(f_bio, n->nand_mq_txid);
				}
#endif
			}
			temp->status = QDMA_AVAIL;
			mutex_unlock(&temp->qdesc_mutex);
			if (req) {
				nvme_rw_cb((void *)req, ret);
			}
			CIRCULAR_INCR(qdma->consumer_index, IO_TABLE_SIZE);
			qdma->qdma_io_completed++;
		} else {
			mutex_unlock(&temp->qdesc_mutex);
			usleep(1);
		}
                ret = mq_getattr(n->trim_mq_txid, &attr);
		if((!ret) && (attr.mq_curmsgs)) {
			rev_addr = 0;
			if((mq_receive (n->trim_mq_txid, (char *)&rev_addr, sizeof (uint64_t), NULL)) > 0) {
				nvme_bio *bio = (nvme_bio *)rev_addr;
				if(bio != NULL) {
					trim_cache((NvmeRequest *)bio->req_info);
				}
			}
                }

#endif
	}
}

