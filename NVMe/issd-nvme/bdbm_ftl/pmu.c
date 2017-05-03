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

#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#include "debug.h"
#include "params.h"
#include "bdbm_drv.h"
#include "platform.h"


#ifdef USE_PMU
void pmu_create (struct bdbm_drv_info* bdi)
{
	uint64_t i, punit;
	struct nand_params* np = (struct nand_params*)BDBM_GET_NAND_PARAMS(bdi);

	bdbm_spin_lock_init (&bdi->pm.pmu_lock);

	/* # of I/O operations */
	atomic64_set (&bdi->pm.exetime_us, time_get_timestamp_in_us ());
	atomic64_set (&bdi->pm.page_read_cnt, 0);
	atomic64_set (&bdi->pm.page_write_cnt, 0);
	atomic64_set (&bdi->pm.rmw_read_cnt, 0);
	atomic64_set (&bdi->pm.rmw_write_cnt, 0);
	atomic64_set (&bdi->pm.gc_cnt, 0);
	atomic64_set (&bdi->pm.gc_erase_cnt, 0);
	atomic64_set (&bdi->pm.gc_read_cnt, 0);
	atomic64_set (&bdi->pm.gc_write_cnt, 0);

	/* elapsed times taken to handle normal I/Os */
	bdi->pm.time_r_sw = 0;
	bdi->pm.time_r_q = 0;
	bdi->pm.time_r_tot = 0;

	bdi->pm.time_w_sw = 0;
	bdi->pm.time_w_q = 0;
	bdi->pm.time_w_tot = 0;

	bdi->pm.time_rmw_sw = 0;
	bdi->pm.time_rmw_q = 0;
	bdi->pm.time_rmw_tot = 0;

	/* elapsed times taken to handle gc I/Os */
	bdi->pm.time_gc_sw = 0;
	bdi->pm.time_gc_q = 0;
	bdi->pm.time_gc_tot = 0;

	/* channel / chip utilization */
	punit = np->nr_chips_per_channel * np->nr_channels;
	bdi->pm.util_r = bdbm_malloc_atomic (punit * sizeof (atomic64_t));
	if (bdi->pm.util_r)  {
		for (i = 0; i < punit; i++) 
			atomic64_set (&bdi->pm.util_r[i], 0);
	}

	bdi->pm.util_w = bdbm_malloc_atomic (punit * sizeof (atomic64_t));
	if (bdi->pm.util_w) {
		for (i = 0; i < punit; i++) 
			atomic64_set (&bdi->pm.util_w[i], 0);
	}
}

void pmu_destory (struct bdbm_drv_info* bdi)
{
	if (bdi->pm.util_r)
		bdbm_free_atomic (bdi->pm.util_r);
	if (bdi->pm.util_w)
		bdbm_free_atomic (bdi->pm.util_w);
}

/* 
 * increase the number of I/O operations according to their types 
 */
void pmu_inc (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* llm_req)
{
	struct nand_params* np = (struct nand_params*)BDBM_GET_NAND_PARAMS(bdi);
	uint64_t pid = 
		np->nr_chips_per_channel * 
		llm_req->phyaddr->channel_no +
		llm_req->phyaddr->chip_no;

	switch (llm_req->req_type) {
	case REQTYPE_READ:
		pmu_inc_read (bdi);
		pmu_inc_util_r (bdi, pid);
		break;
	case REQTYPE_WRITE:
		pmu_inc_write (bdi);
		pmu_inc_util_w (bdi, pid);
		break;
	case REQTYPE_RMW_READ:
		pmu_inc_rmw_read (bdi);
		pmu_inc_util_r (bdi, pid);
		break;
	case REQTYPE_RMW_WRITE:
		pmu_inc_rmw_write (bdi);
		pmu_inc_util_w (bdi, pid);
		break;
	case REQTYPE_GC_READ:
		pmu_inc_gc_read (bdi);
		pmu_inc_util_r (bdi, pid);
		break;
	case REQTYPE_GC_WRITE:
		pmu_inc_gc_write (bdi);
		pmu_inc_util_w (bdi, pid);
		break;
	case REQTYPE_GC_ERASE:
		pmu_inc_gc_erase (bdi);
		break;
	default:
		break;
	}
}

void pmu_inc_read (struct bdbm_drv_info* bdi) 
{
	atomic64_inc (&bdi->pm.page_read_cnt);
}

void pmu_inc_write (struct bdbm_drv_info* bdi) 
{
	atomic64_inc (&bdi->pm.page_write_cnt);
}

void pmu_inc_rmw_read (struct bdbm_drv_info* bdi) 
{
	atomic64_inc (&bdi->pm.rmw_read_cnt);
}

void pmu_inc_rmw_write (struct bdbm_drv_info* bdi) 
{
	atomic64_inc (&bdi->pm.rmw_write_cnt);
}

void pmu_inc_gc (struct bdbm_drv_info* bdi)
{
	atomic64_inc (&bdi->pm.gc_cnt);
}

void pmu_inc_gc_erase (struct bdbm_drv_info* bdi)
{
	atomic64_inc (&bdi->pm.gc_erase_cnt);
}

void pmu_inc_gc_read (struct bdbm_drv_info* bdi)
{
	atomic64_inc (&bdi->pm.gc_read_cnt);
}

void pmu_inc_gc_write (struct bdbm_drv_info* bdi)
{
	atomic64_inc (&bdi->pm.gc_write_cnt);
}

void pmu_inc_util_r (struct bdbm_drv_info* bdi, uint64_t id)
{
	atomic64_inc (&bdi->pm.util_r[id]);
}

void pmu_inc_util_w (struct bdbm_drv_info* bdi, uint64_t id)
{
	atomic64_inc (&bdi->pm.util_w[id]);
}

/* update the time taken to run sw algorithms */
void pmu_update_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) 
{
	switch (req->req_type) {
	case REQTYPE_READ:
		pmu_update_r_sw (bdi, req);
		break;
	case REQTYPE_WRITE:
		pmu_update_w_sw (bdi, req);
		break;
	case REQTYPE_RMW_READ:
		pmu_update_rmw_sw (bdi, req);
		break;
	}
}

void pmu_update_r_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) 
{
	unsigned long flags;
	struct bdbm_hlm_req_t* h = (struct bdbm_hlm_req_t*)req->ptr_hlm_req;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (&h->sw);
	int64_t n = atomic64_read (&bdi->pm.page_read_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_r_sw = (bdi->pm.time_r_sw * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}

void pmu_update_w_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) 
{
	unsigned long flags;
	struct bdbm_hlm_req_t* h = (struct bdbm_hlm_req_t*)req->ptr_hlm_req;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (&h->sw);
	int64_t n = atomic64_read (&bdi->pm.page_write_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_w_sw = (bdi->pm.time_w_sw * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}

void pmu_update_rmw_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req)
{
	unsigned long flags;
	struct bdbm_hlm_req_t* h = (struct bdbm_hlm_req_t*)req->ptr_hlm_req;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (&h->sw);
	int64_t n = atomic64_read (&bdi->pm.rmw_read_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_rmw_sw = (bdi->pm.time_rmw_sw * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}

void pmu_update_gc_sw (struct bdbm_drv_info* bdi, struct bdbm_stopwatch* sw) 
{
	unsigned long flags;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (sw);
	int64_t n = atomic64_read (&bdi->pm.gc_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_gc_sw = (bdi->pm.time_gc_sw * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}


/* 
 * update the time taken to stay in the queue 
 **/
void pmu_update_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req)
{
	switch (req->req_type) {
	case REQTYPE_READ:
		pmu_update_r_q (bdi, req);
		break;
	case REQTYPE_WRITE:
		pmu_update_w_q (bdi, req);
		break;
	case REQTYPE_RMW_READ:
		pmu_update_rmw_q (bdi, req);
		break;
	}
}

void pmu_update_r_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) 
{
	unsigned long flags;
	struct bdbm_hlm_req_t* h = (struct bdbm_hlm_req_t*)req->ptr_hlm_req;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (&h->sw);
	int64_t n = atomic64_read (&bdi->pm.page_read_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_r_q = (bdi->pm.time_r_q * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}

void pmu_update_w_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) 
{
	unsigned long flags;
	struct bdbm_hlm_req_t* h = (struct bdbm_hlm_req_t*)req->ptr_hlm_req;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (&h->sw);
	int64_t n = atomic64_read (&bdi->pm.page_write_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_w_q = (bdi->pm.time_w_q * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}

void pmu_update_rmw_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req)
{
	unsigned long flags;
	struct bdbm_hlm_req_t* h = (struct bdbm_hlm_req_t*)req->ptr_hlm_req;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (&h->sw);
	int64_t n = atomic64_read (&bdi->pm.rmw_read_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_rmw_q = (bdi->pm.time_rmw_q * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}

void pmu_update_gc_q (struct bdbm_drv_info* bdi, struct bdbm_stopwatch* sw)
{
	unsigned long flags;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (sw);
	int64_t n = atomic64_read (&bdi->pm.gc_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_gc_q = (bdi->pm.time_gc_q * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}


/* 
 * update the time taken for NAND devices to handle reqs 
 **/
void pmu_update_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req)
{
	switch (req->req_type) {
	case REQTYPE_READ:
		pmu_update_r_tot (bdi, req);
		break;
	case REQTYPE_WRITE:
		pmu_update_w_tot (bdi, req);
		break;
	case REQTYPE_RMW_WRITE:
		pmu_update_rmw_tot (bdi, req);
		break;
	}
}

void pmu_update_r_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) 
{
	unsigned long flags;
	struct bdbm_hlm_req_t* h = (struct bdbm_hlm_req_t*)req->ptr_hlm_req;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (&h->sw);
	int64_t n = atomic64_read (&bdi->pm.page_read_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_r_tot = (bdi->pm.time_r_tot * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}

void pmu_update_w_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) 
{
	unsigned long flags;
	struct bdbm_hlm_req_t* h = (struct bdbm_hlm_req_t*)req->ptr_hlm_req;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (&h->sw);
	int64_t n = atomic64_read (&bdi->pm.page_write_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_w_tot = (bdi->pm.time_w_tot * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}

void pmu_update_rmw_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req)
{
	unsigned long flags;
	struct bdbm_hlm_req_t* h = (struct bdbm_hlm_req_t*)req->ptr_hlm_req;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (&h->sw);
	int64_t n = atomic64_read (&bdi->pm.rmw_read_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_rmw_tot = (bdi->pm.time_rmw_q * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}

void pmu_update_gc_tot (struct bdbm_drv_info* bdi, struct bdbm_stopwatch* sw)
{
	unsigned long flags;
	int64_t delta = bdbm_stopwatch_get_elapsed_time_us (sw);
	int64_t n = atomic64_read (&bdi->pm.gc_cnt);
	bdbm_spin_lock_irqsave (&bdi->pm.pmu_lock, flags);
	bdi->pm.time_gc_tot = (bdi->pm.time_gc_tot * n + delta) / (n + 1);
	bdbm_spin_unlock_irqrestore (&bdi->pm.pmu_lock, flags);
}


/* display performance results */
char format[1024];
char str[1024];

void pmu_display (struct bdbm_drv_info* bdi) 
{
	uint64_t i, j;
	uint32_t exetime;
	struct nand_params* np = (struct nand_params*)BDBM_GET_NAND_PARAMS(bdi);

	bdbm_msg ("-----------------------------------------------");
	bdbm_msg ("< PERFORMANCE SUMMARY >");

	exetime = time_get_timestamp_in_us ();
	bdbm_msg ("(0) Execute Time (us): %ld.%ld", 
		(exetime - atomic64_read (&bdi->pm.exetime_us)) / 1000000,
		(exetime - atomic64_read (&bdi->pm.exetime_us)) % 1000000);
	bdbm_msg ("");

	bdbm_msg ("(1) Total I/Os");
	bdbm_msg ("# of page reads: %ld", 
		atomic64_read (&bdi->pm.page_read_cnt) + 
		atomic64_read (&bdi->pm.rmw_read_cnt) + 
		atomic64_read (&bdi->pm.gc_read_cnt));
	bdbm_msg ("# of page writes: %ld", 
		atomic64_read (&bdi->pm.page_write_cnt) +
		atomic64_read (&bdi->pm.rmw_write_cnt) +
		atomic64_read (&bdi->pm.gc_write_cnt));
	bdbm_msg ("# of block erase: %ld", 
		atomic64_read (&bdi->pm.gc_erase_cnt));
	bdbm_msg ("");

	bdbm_msg ("(2) Normal I/Os");
	bdbm_msg ("# of page reads: %ld", 
		atomic64_read (&bdi->pm.page_read_cnt));
	bdbm_msg ("# of page writes: %ld", 
		atomic64_read (&bdi->pm.page_write_cnt));
	bdbm_msg ("# of page rmw reads: %ld", 
		atomic64_read (&bdi->pm.rmw_read_cnt));
	bdbm_msg ("# of page rmw writes: %ld", 
		atomic64_read (&bdi->pm.rmw_write_cnt));
	bdbm_msg ("");


	bdbm_msg ("(3) GC I/Os");
	bdbm_msg ("# of GC invocation: %ld",
		atomic64_read (&bdi->pm.gc_cnt));
	bdbm_msg ("# of page reads: %ld",
		atomic64_read (&bdi->pm.gc_read_cnt));
	bdbm_msg ("# of page writes: %ld",
		atomic64_read (&bdi->pm.gc_write_cnt));
	bdbm_msg ("# of block erase: %ld", 
		atomic64_read (&bdi->pm.gc_erase_cnt));
	bdbm_msg ("");


	bdbm_msg ("(4) Elapsed Time");
	bdbm_msg ("page read (us): %llu (S:%llu + Q:%llu + D:%llu)",
		bdi->pm.time_r_tot, 
		bdi->pm.time_r_sw,
		bdi->pm.time_r_q - bdi->pm.time_r_sw,
		bdi->pm.time_r_tot - bdi->pm.time_r_q);
	bdbm_msg ("page write (us): %llu (S:%llu + Q:%llu + D:%llu)",
		bdi->pm.time_w_tot, 
		bdi->pm.time_w_sw,
		bdi->pm.time_w_q - bdi->pm.time_w_sw,
		bdi->pm.time_w_tot - bdi->pm.time_w_q);
	bdbm_msg ("rmw (us): %llu (S:%llu + Q:%llu + D:%llu)",
		bdi->pm.time_rmw_tot, 
		bdi->pm.time_rmw_sw,
		bdi->pm.time_rmw_q - bdi->pm.time_rmw_sw,
		bdi->pm.time_rmw_tot - bdi->pm.time_rmw_q);
	bdbm_msg ("");

	bdbm_msg ("(5) Utilization (R)");
	for (i = 0; i < np->nr_chips_per_channel; i++) {
		for (j = 0; j < np->nr_channels; j++) {
			sprintf (str, "% 8ld ", atomic64_read (&bdi->pm.util_r[j*np->nr_chips_per_channel+i]));
			strcat (format, str);
		}
		bdbm_msg ("%s", format);
		bdbm_memset (format, 0x00, sizeof (format));
	}
	bdbm_msg ("");

	bdbm_msg ("(6) Utilization (W)");
	for (i = 0; i < np->nr_chips_per_channel; i++) {
		for (j = 0; j < np->nr_channels; j++) {
			sprintf (str, "% 8ld ", atomic64_read (&bdi->pm.util_w[j*np->nr_chips_per_channel+i]));
			strcat (format, str);
		}
		bdbm_msg ("%s", format);
		bdbm_memset (format, 0x00, sizeof (format));
	}

	bdbm_msg ("-----------------------------------------------");
	bdbm_msg ("-----------------------------------------------");
}

#else

void pmu_create (struct bdbm_drv_info* bdi) {}
void pmu_destroy (struct bdbm_drv_info* bdi) {}
void pmu_display (struct bdbm_drv_info* bdi) {}

void pmu_inc (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* llm_req) {}
void pmu_inc_read (struct bdbm_drv_info* bdi) {}
void pmu_inc_write (struct bdbm_drv_info* bdi) {}
void pmu_inc_rmw_read (struct bdbm_drv_info* bdi) {}
void pmu_inc_rmw_write (struct bdbm_drv_info* bdi) {}
void pmu_inc_gc (struct bdbm_drv_info* bdi) {}
void pmu_inc_gc_erase (struct bdbm_drv_info* bdi) {}
void pmu_inc_gc_read (struct bdbm_drv_info* bdi) {}
void pmu_inc_gc_write (struct bdbm_drv_info* bdi) {}
void pmu_inc_util_r (struct bdbm_drv_info* bdi, uint64_t id) {}
void pmu_inc_util_w (struct bdbm_drv_info* bdi, uint64_t id) {}

void pmu_update_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_r_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_w_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_rmw_sw (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_gc_sw (struct bdbm_drv_info* bdi, struct bdbm_stopwatch* sw) {}

void pmu_update_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_r_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_w_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_rmw_q (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}

void pmu_update_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_r_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_w_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_rmw_tot (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req) {}
void pmu_update_gc_tot (struct bdbm_drv_info* bdi, struct bdbm_stopwatch* sw) {}

#endif
