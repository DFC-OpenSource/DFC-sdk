/*
The MIT License (MIT)

Copyright (c) 2014-2015 CSAIL, MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _BLUEDBM_DRV_H
#define _BLUEDBM_DRV_H

#include <semaphore.h>
#include "platform.h"
#include "list.h"
#include "types.h"

#include "../nvme_bio.h"
#include <sys/ipc.h>
#include <sys/msg.h>

#define PAGE_SIZE 	4096
#define KERNEL_PAGE_SIZE	PAGE_SIZE
#define PAGE_OOB_OFF 4
#define MTL
#define SNAPSHOT_ENABLE

/* useful macros */
#define BDBM_KB (1024)
#define BDBM_MB (1024 * 1024)
#define BDBM_GB (1024 * 1024 * 1024)
#define BDBM_TB (1024 * 1024 * 1024 * 1024)

#define BDBM_SIZE_KB(size) (size/BDBM_KB)
#define BDBM_SIZE_MB(size) (size/BDBM_MB)
#define BDBM_SIZE_GB(size) (size/BDBM_GB)
#if 0
/* for performance monitoring */
struct bdbm_perf_monitor {
	bdbm_spinlock_t pmu_lock;

	atomic64_t exetime_us;

	atomic64_t page_read_cnt;
	atomic64_t page_write_cnt;
	atomic64_t rmw_read_cnt;
	atomic64_t rmw_write_cnt;
	atomic64_t gc_cnt;
	atomic64_t gc_erase_cnt;
	atomic64_t gc_read_cnt;
	atomic64_t gc_write_cnt;

	uint64_t time_r_sw;
	uint64_t time_r_q;
	uint64_t time_r_tot;

	uint64_t time_w_sw;
	uint64_t time_w_q;
	uint64_t time_w_tot;

	uint64_t time_rmw_sw;
	uint64_t time_rmw_q;
	uint64_t time_rmw_tot;

	uint64_t time_gc_sw;
	uint64_t time_gc_q;
	uint64_t time_gc_tot;

	atomic64_t* util_r;
	atomic64_t* util_w;
};
#endif
/* the main data-structure for bdbm_drv */
struct bdbm_drv_info {
	void* private_data;
	struct bdbm_params* ptr_bdbm_params;
	struct bdbm_host_inf_t* ptr_host_inf; 
	struct bdbm_dm_inf_t* ptr_dm_inf;
	struct bdbm_hlm_inf_t* ptr_hlm_inf;
	struct bdbm_llm_inf_t* ptr_llm_inf;
	struct bdbm_ftl_inf_t* ptr_ftl_inf;
/*	struct bdbm_perf_monitor pm; */
};

#define BDBM_GET_HOST_INF(bdi) bdi->ptr_host_inf
#define BDBM_GET_DM_INF(bdi) bdi->ptr_dm_inf
#define BDBM_GET_HLM_INF(bdi) bdi->ptr_hlm_inf
#define BDBM_GET_LLM_INF(bdi) bdi->ptr_llm_inf
#define BDBM_GET_NAND_PARAMS(bdi) (&bdi->ptr_bdbm_params->nand)
#define BDBM_GET_DRIVER_PARAMS(bdi) (&bdi->ptr_bdbm_params->driver)
#define BDBM_GET_FTL_INF(bdi) bdi->ptr_ftl_inf

#define BDBM_HOST_PRIV(bdi) bdi->ptr_host_inf->ptr_private
#define BDBM_DM_PRIV(bdi) bdi->ptr_dm_inf->ptr_private
#define BDBM_HLM_PRIV(bdi) bdi->ptr_hlm_inf->ptr_private
#define BDBM_LLM_PRIV(bdi) bdi->ptr_llm_inf->ptr_private
#define BDBM_FTL_PRIV(bdi) bdi->ptr_ftl_inf->ptr_private


/* request types */
enum BDBM_REQTYPE {
	/* reqtype from host */
	REQTYPE_WRITE 		= 0,
	REQTYPE_READ 		= 1,
	REQTYPE_READ_DUMMY 	= 2,
	REQTYPE_RMW_READ 	= 3, 		/* Read-Modify-Write */
	REQTYPE_RMW_WRITE 	= 4, 		/* Read-Modify-Write */
	REQTYPE_GC_READ 	= 5,
	REQTYPE_GC_WRITE 	= 6,
	REQTYPE_GC_ERASE 	= 7,
	REQTYPE_TRIM 		= 8,
};

/* a physical address */
typedef struct bdbm_phyaddr_t {
	uint8_t channel_no;
	uint8_t chip_no;
	uint16_t block_no;
	uint16_t page_no;
}bdbm_phyaddr_t;

/* a high-level memory manager request */
enum BDBM_HLM_MEMFLAG {
	MEMFLAG_NOT_SET = 0,
	MEMFLAG_FRAG_PAGE = 1,
	MEMFLAG_KMAP_PAGE = 2,
	MEMFLAG_DONE = 0x80,
	MEMFLAG_FRAG_PAGE_DONE = MEMFLAG_FRAG_PAGE | MEMFLAG_DONE,
	MEMFLAG_KMAP_PAGE_DONE = MEMFLAG_KMAP_PAGE | MEMFLAG_DONE,
};

typedef struct nvme_drv_params {
	uint64_t npages;
	uint64_t page_size;
	uint8_t  hasNextDrvParams;
} nvme_drv_params;

/* a high-level request */
struct bdbm_hlm_req_t {
	uint32_t req_type; /* read, write, or trim */
	uint64_t *lpa; /* logical page address */
	uint64_t len; /* legnth */
	uint64_t nr_done_reqs;	/* # of llm_reqs served */
	uint8_t* kpg_flags;
	uint8_t** pptr_kpgs; /* data for individual kernel pages */
	void* ptr_host_req; /* struct bio or I/O trace */
	uint8_t ret;
	uint8_t queued;
	void* llm_set_ptr;
	//bdbm_spinlock_t lock; /* spinlock */

	/* for performance monitoring */
/*	struct bdbm_stopwatch sw; */

	/* temp */
	uint8_t* org_kpg_flags;
	uint8_t** org_pptr_kpgs; /* data for individual kernel pages */
	/* end */
	uint32_t nr_llm_reqs;
	uint8_t is_seq_lpa;
};

/* a high-level request for gc */
struct bdbm_hlm_req_gc_t {
	uint32_t req_type;
	uint64_t nr_done_reqs;
	uint64_t nr_reqs;
	struct bdbm_llm_req_t** llm_reqs;
	sem_t gc_done;
/*	bdbm_completion gc_done; */
};

/* a low-level request */
struct bdbm_llm_req_t {
	uint32_t req_type; /* read, write, or erase */
	uint64_t *lpa; /* logical page address */
	struct bdbm_phyaddr_t* phyaddr;	/* current */
	struct bdbm_phyaddr_t phyaddr_r; /* for reads */
	struct bdbm_phyaddr_t phyaddr_w; /* for writes */
	uint8_t* kpg_flags;
	uint8_t** pptr_kpgs; /* from bdbm_hlm_req_t */
	uint8_t* ptr_oob;
	uint8_t *phy_ptr_oob; /*physical address for gc based oob operations*/
	void* ptr_hlm_req;
	struct list_head list;	/* for list management */
	void* ptr_qitem;
	uint8_t ret;	/* old for GC */
	uint8_t offset;
};

/* a generic host interface */
struct bdbm_host_inf_t {
	void* ptr_private;
	uint32_t (*open) (struct bdbm_drv_info* bdi);
	void (*close) (struct bdbm_drv_info* bdi);
	int (*make_req) (struct bdbm_drv_info* bdi, struct nvme_bio *bio);
	void (*end_req) (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* req);
};

/* a generic high-level memory manager interface */
struct bdbm_hlm_inf_t {
	void* ptr_private;
	uint32_t (*create) (struct bdbm_drv_info* bdi);
	void (*destroy) (struct bdbm_drv_info* bdi);
	uint32_t (*make_req) (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* req);
	void (*end_req) (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
};

/* a generic low-level memory manager interface */
struct bdbm_llm_inf_t {
	void* ptr_private;
	uint32_t (*create) (struct bdbm_drv_info* bdi);
	void (*destroy) (struct bdbm_drv_info* bdi);
	uint32_t (*make_req) (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t** req);
	void (*flush) (struct bdbm_drv_info* bdi);
	void (*end_req) (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
};

/* a generic device interface */
struct bdbm_dm_inf_t {
	void* ptr_private;
	uint32_t (*probe) (struct bdbm_drv_info* bdi);
	uint32_t (*open) (struct bdbm_drv_info* bdi);
	void (*close) (struct bdbm_drv_info* bdi);
	uint32_t (*make_req) (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t** req);
	void (*end_req) (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
	uint32_t (*load) (struct bdbm_drv_info* bdi, void *pmt, void *abi);
	uint32_t (*store) (struct bdbm_drv_info* bdi);
};

/* a generic queue interface */
struct bdbm_queue_inf_t {
	void* ptr_private;
	uint32_t (*create) (struct bdbm_drv_info* bdi, uint64_t nr_punits, uint64_t nr_items_per_pu);
	void (*destroy) (struct bdbm_drv_info* bdi);
	uint32_t (*enqueue) (struct bdbm_drv_info* bdi, uint64_t punit, void* req);
	void* (*dequeue) (struct bdbm_drv_info* bdi, uint64_t punit);
	uint8_t (*is_full) (struct bdbm_drv_info* bdi, uint64_t punit);
	uint8_t (*is_empty) (struct bdbm_drv_info* bdi, uint64_t punit);
};

#define GC_THREAD_ENABLED 1

/* a generic FTL interface */
struct bdbm_ftl_inf_t {
	void* ptr_private;
	uint32_t (*create) (struct bdbm_drv_info* bdi);
	void (*destroy) (struct bdbm_drv_info* bdi);
	uint32_t (*get_free_ppa) (struct bdbm_drv_info* bdi, uint64_t *lpa, struct bdbm_phyaddr_t* ppa);
	uint32_t (*get_ppa) (struct bdbm_drv_info* bdi, uint64_t lpa, struct bdbm_phyaddr_t* ppa, uint8_t* sp_off);
	uint32_t (*map_lpa_to_ppa) (struct bdbm_drv_info* bdi, uint64_t *lpa, struct bdbm_phyaddr_t* ppa);
	uint32_t (*invalidate_lpa) (struct bdbm_drv_info* bdi, uint64_t lpa, uint64_t len);
	uint32_t (*do_gc) (struct bdbm_drv_info* bdi);
	uint8_t (*is_gc_needed) (struct bdbm_drv_info* bdi);
	uint32_t (*scan_badblocks) (struct bdbm_drv_info* bdi);
	uint32_t (*load) (struct bdbm_drv_info* bdi, const char* fn);
	uint32_t (*store) (struct bdbm_drv_info* bdi, const char* fn);
	/* custom interfaces for rsd */
	uint64_t (*get_segno) (struct bdbm_drv_info* bdi, uint64_t lpa);
};

struct bdbm_abm_info* bdbm_get_bai(struct bdbm_drv_info*);
struct bdbm_abm_block_t** bdbm_get_ac_bab(struct bdbm_drv_info* _bdi);

#if 0
/* performance monitor functions */
void pmu_create (struct bdbm_drv_info* bdi);
void pmu_destory (struct bdbm_drv_info* bdi);
void pmu_display (struct bdbm_drv_info* bdi);

void pmu_inc (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* llm_req);
void pmu_inc_read (struct bdbm_drv_info* bdi);
void pmu_inc_write (struct bdbm_drv_info* bdi);
void pmu_inc_rmw_read (struct bdbm_drv_info* bdi);
void pmu_inc_rmw_write (struct bdbm_drv_info* bdi);
void pmu_inc_gc (struct bdbm_drv_info* bdi);
void pmu_inc_gc_erase (struct bdbm_drv_info* bdi);
void pmu_inc_gc_read (struct bdbm_drv_info* bdi);
void pmu_inc_gc_write (struct bdbm_drv_info* bdi);
void pmu_inc_util_r (struct bdbm_drv_info* bdi, uint64_t pid);
void pmu_inc_util_w (struct bdbm_drv_info* bdi, uint64_t pid);

void pmu_update_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_r_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_w_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_rmw_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_gc_sw (struct bdbm_drv_info* bdi, struct bdbm_stopwatch* sw);

void pmu_update_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_r_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_w_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_rmw_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);

void pmu_update_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_r_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_w_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_rmw_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void pmu_update_gc_tot (struct bdbm_drv_info* bdi, struct bdbm_stopwatch* sw);
#endif
#endif /* _BLUEDBM_DRV_H */
