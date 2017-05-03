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
/*
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/log2.h>
*/

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

#include "../bdbm_drv.h"
#include "../params.h"
#include "../debug.h"
#include <pthread.h>
#include <sched.h>
/*
#include "utils/time.h"
#include "/utils/file.h"
*/
#include "abm.h"
#include "page_ftl.h"
#include "../../uatomic.h"
#include <syslog.h>

pthread_mutex_t gc_lock; 
extern unsigned long long int gc_phy_addr[70];
extern atomic_t act_desc_cnt;
extern uint8_t* ls2_virt_addr[5];
extern uint8_t ftl_initialised,ftl_stored;
/* FTL interface */
struct bdbm_ftl_inf_t _ftl_page_ftl = {
	.ptr_private = NULL,
	.create = bdbm_page_ftl_create,
	.destroy = bdbm_page_ftl_destroy,
	.get_free_ppa = bdbm_page_ftl_get_free_ppa,
	.get_ppa = bdbm_page_ftl_get_ppa,
	.map_lpa_to_ppa = bdbm_page_ftl_map_lpa_to_ppa,
	.invalidate_lpa = bdbm_page_ftl_invalidate_lpa,
#if GC_THREAD_ENABLED
	.do_gc = bdbm_page_ftl_signal_gc,
#else
	.do_gc = bdbm_page_ftl_do_gc,
#endif
	.is_gc_needed = bdbm_page_ftl_is_gc_needed,
	.scan_badblocks = bdbm_page_badblock_scan,
	.load = bdbm_page_ftl_load,
	.store = bdbm_page_ftl_store,
	.get_segno = NULL,
};

void *gc_thread (void *arg);

/* data structures for block-level FTL */
enum BDBM_PFTL_PAGE_STATUS {
	PFTL_PAGE_NOT_ALLOCATED = 0,
	PFTL_PAGE_VALID,
	PFTL_PAGE_INVALID,
	PFTL_PAGE_INVALID_ADDR = -1,
};

struct bdbm_page_mapping_entry {
	uint8_t status; /* BDBM_PFTL_PAGE_STATUS */
	struct bdbm_phyaddr_t phyaddr; /* physical location */
	uint8_t sp_off;
};

struct bdbm_page_ftl_private {
	struct bdbm_abm_info* bai;
	struct bdbm_page_mapping_entry* ptr_mapping_table;
	/*bdbm_spinlock_t ftl_lock; */
	uint64_t nr_punits;	

	/* for the management of active blocks */
	uint64_t curr_puid;
	uint64_t curr_page_ofs;
	struct bdbm_abm_block_t** ac_bab;

	/* reserved for gc (reused whenever gc is invoked) */
	struct bdbm_abm_block_t** gc_bab;
	struct bdbm_hlm_req_gc_t gc_hlm;
	struct bdbm_hlm_req_gc_t gc_hlm_w;

	/* for bad-block scanning */
	/*bdbm_completion badblk; */

#if GC_THREAD_ENABLED
	sem_t     gc_sem;
	pthread_t gc_tid;
	int       bdbmIsAlive;
#endif
};


struct bdbm_page_mapping_entry* __bdbm_page_ftl_create_mapping_table (struct nand_params* np)
{
	struct bdbm_page_mapping_entry* me;
	uint64_t loop;

	/* create a page-level mapping table */
	if ((me = (struct bdbm_page_mapping_entry*)bdbm_zmalloc 
			(sizeof (struct bdbm_page_mapping_entry) * np->nr_subpages_per_ssd)) == NULL) {
		return NULL;
	}

	/* initialize a page-level mapping table */
	for (loop = 0; loop < np->nr_subpages_per_ssd; loop++) {
		me[loop].status = PFTL_PAGE_NOT_ALLOCATED;
		me[loop].phyaddr.channel_no = PFTL_PAGE_INVALID_ADDR;
		me[loop].phyaddr.chip_no = PFTL_PAGE_INVALID_ADDR;
		me[loop].phyaddr.block_no = PFTL_PAGE_INVALID_ADDR;
		me[loop].phyaddr.page_no = PFTL_PAGE_INVALID_ADDR;
		me[loop].sp_off = -1;
	}

	/* return a set of mapping entries */
	return me;
}

void __bdbm_page_ftl_destroy_mapping_table (
	struct bdbm_page_mapping_entry* me)
{
	if (me == NULL)
		return;
	bdbm_free (me);
}

uint32_t __bdbm_page_ftl_get_active_blocks (
	struct nand_params* np,
	struct bdbm_abm_info* bai,
	struct bdbm_abm_block_t** bab)
{
	uint64_t i, j;

	/* get a set of free blocks for active blocks */
	for (i = 0; i < np->nr_channels; i++) {
		for (j = 0; j < np->nr_chips_per_channel; j++) {
			/* prepare & commit free blocks */
			if ((*bab = bdbm_abm_get_free_block_prepare (bai, i, j))) {
				bdbm_abm_get_free_block_commit (bai, *bab);
				/*bdbm_msg ("active blk = %p", *bab);*/
				bab++;
			} else {
				bdbm_error ("free_block_prepare failed @ channel:%lu chip:%lu",i,j);
				return 1;
			}
		}
	}

	return 0;
}

struct bdbm_abm_block_t** __bdbm_page_ftl_create_active_blocks (
	struct nand_params* np,
	struct bdbm_abm_info* bai)
{
	uint64_t nr_punits;
	struct bdbm_abm_block_t** bab = NULL;

	nr_punits = np->nr_chips_per_channel * np->nr_channels;

	/*bdbm_msg ("nr_punits: %lu", nr_punits);*/

	/* create a set of active blocks */
	if ((bab = (struct bdbm_abm_block_t**)bdbm_zmalloc 
			(sizeof (struct bdbm_abm_block_t*) * nr_punits)) == NULL) {
		bdbm_error ("bdbm_zmalloc failed");
		goto fail;
	}
#if 0
	/* get a set of free blocks for active blocks */
	if (__bdbm_page_ftl_get_active_blocks (np, bai, bab) != 0) {
		bdbm_error ("__bdbm_page_ftl_get_active_blocks failed");
		goto fail;
	}
#endif
	return bab;

fail:
	if (bab)
		bdbm_free (bab);
	return NULL;
}

void __bdbm_page_ftl_destroy_active_blocks (
	struct bdbm_abm_block_t** bab)
{
	if (bab == NULL)
		return;

	/* TODO: it might be required to save the status of active blocks 
	 * in order to support rebooting */
	bdbm_free (bab);
}

uint32_t bdbm_page_ftl_create (struct bdbm_drv_info* bdi)
{
	struct bdbm_page_ftl_private* p = NULL;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	uint64_t i = 0, j = 0;
	uint64_t nr_kp_per_fp = np->page_main_size / KERNEL_PAGE_SIZE;	/* e.g., 2 = 8 KB / 4 KB */
	int pge_phy_idx = 5, oob_phy_idx = 1;
	uint32_t pge_addr_cnt = 0, oob_addr_cnt = 0, max_pge_cnt, max_oob_cnt;
	uint64_t gc_llm_cnt = 0;

	max_pge_cnt = 0x400000/KERNEL_PAGE_SIZE;
	max_oob_cnt = 0x400000/np->page_oob_size;

	/* create a private data structure */
	if ((p = (struct bdbm_page_ftl_private*)bdbm_zmalloc 
			(sizeof (struct bdbm_page_ftl_private))) == NULL) {
		bdbm_error ("bdbm_malloc failed");
		return 1;
	}
	p->curr_puid = 0;
	p->curr_page_ofs = 0;
	p->nr_punits = np->nr_chips_per_channel * np->nr_channels;
	/*bdbm_spin_lock_init (&p->ftl_lock);*/
	_ftl_page_ftl.ptr_private = (void*)p;
	/* create 'bdbm_abm_info' with pst */
	if ((p->bai = bdbm_abm_create (np, 1)) == NULL) {
		bdbm_error ("bdbm_abm_create failed");
		bdbm_page_ftl_destroy (bdi);
		return 1;
	}

	/* create a mapping table */
	if ((p->ptr_mapping_table = __bdbm_page_ftl_create_mapping_table (np)) == NULL) {
		bdbm_error ("__bdbm_page_ftl_create_mapping_table failed");
		bdbm_page_ftl_destroy (bdi);
		return 1;
	}

	/* allocate active blocks */
	if ((p->ac_bab = __bdbm_page_ftl_create_active_blocks (np, p->bai)) == NULL) {
		bdbm_error ("__bdbm_page_ftl_create_active_blocks failed");
		bdbm_page_ftl_destroy (bdi);
		return 1;
	}

	/* allocate gc stuffs */
	if ((p->gc_bab = (struct bdbm_abm_block_t**)bdbm_zmalloc 
			(sizeof (struct bdbm_abm_block_t*) * p->nr_punits)) == NULL) {
		bdbm_error ("bdbm_zmalloc failed");
		bdbm_page_ftl_destroy (bdi);
		return 1;
	}
	gc_llm_cnt = p->nr_punits * np->nr_pages_per_block;
	if ((p->gc_hlm.llm_reqs = (struct bdbm_llm_req_t**)bdbm_zmalloc
			(sizeof (struct bdbm_llm_req_t*) * gc_llm_cnt)) == NULL) {
		bdbm_error ("bdbm_zmalloc failed");
		bdbm_page_ftl_destroy (bdi);
		return 1;
	}

	for (i=0; i < gc_llm_cnt; i++) {
		struct bdbm_llm_req_t* r = NULL;
		/* create a low-level request */
		if ((r = (struct bdbm_llm_req_t*)bdbm_zmalloc 
					(sizeof (struct bdbm_llm_req_t))) == NULL) {
			bdbm_error ("bdbm_malloc_atomic failed");
		}
		p->gc_hlm.llm_reqs[i] = r;
	}

	sem_init (&p->gc_hlm.gc_done, 0, 0);

	i=0;
	while (i < p->nr_punits * np->nr_pages_per_block) {
		struct bdbm_llm_req_t* r = p->gc_hlm.llm_reqs[i];
		r->lpa = NULL;
		r->kpg_flags = (uint8_t*)bdbm_malloc_atomic (sizeof(uint8_t) * nr_kp_per_fp);
		r->pptr_kpgs = (uint8_t**)bdbm_malloc_atomic (sizeof(uint8_t*) * nr_kp_per_fp);
		if (max_pge_cnt == pge_addr_cnt) {
			pge_phy_idx++;
			pge_addr_cnt = 0;
		}

		for (j = 0; j < nr_kp_per_fp; j++) {
			//r->pptr_kpgs[j] = (uint8_t*)get_zeroed_page (GFP_KERNEL); 
			r->pptr_kpgs[j] = (uint8_t*)gc_phy_addr[pge_phy_idx]+(KERNEL_PAGE_SIZE*pge_addr_cnt);
			pge_addr_cnt++;
		}
		//r->ptr_oob = (uint8_t*)bdbm_malloc_atomic (sizeof (uint8_t) * np->page_oob_size);
		r->phy_ptr_oob = (uint8_t*)gc_phy_addr[oob_phy_idx]+(np->page_oob_size * oob_addr_cnt);
		r->ptr_oob = (uint8_t*)ls2_virt_addr[oob_phy_idx]+(np->page_oob_size * oob_addr_cnt);
		oob_addr_cnt++;
		if (max_oob_cnt == oob_addr_cnt) {
			oob_phy_idx++;
			oob_addr_cnt = 0;
		}
		i++;
	}
		/*	bdbm_init_completion (p->gc_hlm.gc_done);
			bdbm_complete (p->gc_hlm.gc_done);
			*/

	if ((p->gc_hlm_w.llm_reqs = (struct bdbm_llm_req_t**)bdbm_zmalloc
			(sizeof (struct bdbm_llm_req_t*) * gc_llm_cnt)) == NULL) {
		bdbm_error ("bdbm_zmalloc failed");
		bdbm_page_ftl_destroy (bdi);
		return 1;
	}
	
	oob_addr_cnt = 0;
	for (i=0; i < gc_llm_cnt; i++) {
		struct bdbm_llm_req_t* r = NULL;
		/* create a low-level request */
		if ((r = (struct bdbm_llm_req_t*)bdbm_zmalloc 
					(sizeof (struct bdbm_llm_req_t))) == NULL) {
			bdbm_error ("bdbm_malloc_atomic failed");
		}
		r->lpa = (int64_t*)bdbm_malloc_atomic (sizeof(int64_t) * nr_kp_per_fp);
		r->kpg_flags = (uint8_t*)bdbm_malloc_atomic (sizeof(uint8_t) * nr_kp_per_fp);
		r->pptr_kpgs = (uint8_t**)bdbm_malloc_atomic (sizeof(uint8_t*) * nr_kp_per_fp);
		r->phy_ptr_oob = (uint8_t*)gc_phy_addr[oob_phy_idx]+(np->page_oob_size * oob_addr_cnt);
		r->ptr_oob = (uint8_t*)ls2_virt_addr[oob_phy_idx]+(np->page_oob_size * oob_addr_cnt);
		oob_addr_cnt++;
		if (max_oob_cnt == oob_addr_cnt) {
			oob_phy_idx++;
			oob_addr_cnt = 0;
		}
		p->gc_hlm_w.llm_reqs[i] = r;
	}

#if GC_THREAD_ENABLED
	int ret;

	/*Create (unnamed) semaphore for GC activation signalling*/
	ret = sem_init (&p->gc_sem, 0, 0);
	if (ret < 0) {
		perror ("gc_sem");
		bdbm_page_ftl_destroy (bdi);
		return 1;
	}

	p->bdbmIsAlive = 1;
	pthread_mutex_init(&gc_lock,NULL);

	/*Create GC thread*/
	ret = pthread_create (&p->gc_tid, NULL, gc_thread, (void*)bdi);
	if (ret < 0) {
		perror ("gc_tid");
		bdbm_page_ftl_destroy (bdi);
		return 1;
	}
#endif
	return 0;
}

void bdbm_page_ftl_destroy (struct bdbm_drv_info* bdi)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	uint64_t nr_kp_per_fp = np->page_main_size / KERNEL_PAGE_SIZE;	/* e.g., 2 = 8 KB / 4 KB */
	uint64_t i = 0, j = 0;

	if (!p)
		return;

#if GC_THREAD_ENABLED
	/*kill GC thread*/
	p->bdbmIsAlive = 0;
	pthread_cancel (p->gc_tid);
	pthread_mutex_destroy(&gc_lock);
	pthread_join (p->gc_tid,NULL);
	sem_destroy (&p->gc_sem);
#endif

	sem_destroy (&p->gc_hlm.gc_done);

	if (p->gc_hlm.llm_reqs) {
		i = 0;
		j = 0;
		while (i < p->nr_punits * np->nr_pages_per_block) {
			struct bdbm_llm_req_t* r = p->gc_hlm.llm_reqs[i];
#if 0
			for (j = 0; j < nr_kp_per_fp; j++)
				bdbm_free_atomic (r->pptr_kpgs[j]); 
#endif
			bdbm_free_atomic (r->pptr_kpgs);
			bdbm_free_atomic (r->kpg_flags);
			bdbm_free (r);
#if 0
			bdbm_free_atomic (r->ptr_oob);
#endif
			i++;
		}
		bdbm_free (p->gc_hlm.llm_reqs);
	}
	if (p->gc_hlm_w.llm_reqs) {
		i = 0;
		j = 0;
		while (i < p->nr_punits * np->nr_pages_per_block) {
			struct bdbm_llm_req_t* r = p->gc_hlm_w.llm_reqs[i];
#if 0
			for (j = 0; j < nr_kp_per_fp; j++)
				bdbm_free_atomic (r->pptr_kpgs[j]); 
#endif
			bdbm_free_atomic (r->pptr_kpgs);
			bdbm_free_atomic (r->kpg_flags);
			bdbm_free_atomic (r->lpa);
			bdbm_free (r);
#if 0
			bdbm_free_atomic (r->ptr_oob);
#endif
			i++;
		}
		bdbm_free (p->gc_hlm_w.llm_reqs);
	}
	if (p->gc_bab)
		bdbm_free (p->gc_bab);
	if (p->ac_bab)
		__bdbm_page_ftl_destroy_active_blocks (p->ac_bab);
	if (p->ptr_mapping_table)
		__bdbm_page_ftl_destroy_mapping_table (p->ptr_mapping_table);
	if (p->bai)
		bdbm_abm_destroy (p->bai);
	bdbm_free (p);
}

uint32_t bdbm_page_ftl_get_free_ppa (struct bdbm_drv_info* bdi, uint64_t *lpa, struct bdbm_phyaddr_t* ppa)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_abm_block_t* b = NULL;
	uint64_t curr_channel;
	uint64_t curr_chip;
	/* get the channel & chip numbers */
	curr_channel = p->curr_puid % np->nr_channels;
	curr_chip = p->curr_puid / np->nr_channels;
	/* get the physical offset of the active blocks */
	/*b = &p->ac_bab[curr_channel][curr_chip];*/
	b = p->ac_bab[curr_channel * np->nr_chips_per_channel + curr_chip];
	ppa->channel_no =  b->channel_no;
	ppa->chip_no = b->chip_no;
	ppa->block_no = b->block_no;
	ppa->page_no = p->curr_page_ofs;
	/* check some error cases before returning the physical address */
	bdbm_bug_on (ppa->channel_no != curr_channel);
	bdbm_bug_on (ppa->chip_no != curr_chip);
	bdbm_bug_on (ppa->page_no >= np->nr_pages_per_block);

	/* go to the next parallel unit */
	if ((p->curr_puid + 1) == p->nr_punits) {
		p->curr_puid = 0;
		p->curr_page_ofs++;	/* go to the next page */
		/* see if there are sufficient free pages or not */
		if (p->curr_page_ofs == np->nr_pages_per_block) {
			/* get active blocks */
			if (__bdbm_page_ftl_get_active_blocks (np, p->bai, p->ac_bab) != 0) {
				bdbm_error ("__bdbm_page_ftl_get_active_blocks failed");
				return 1;
			}
			/* ok; go ahead with 0 offset */
			/*bdbm_msg ("curr_puid = %lu", p->curr_puid);*/
			p->curr_page_ofs = 0;
		}
	} else {
		/*bdbm_msg ("curr_puid = %lu", p->curr_puid);*/
		p->curr_puid++;
	}
	//printf("Total:%lu Free:%lu Free_pre:%lu Clean:%lu Dirty:%lu Bad:%lu\n",p->bai->nr_total_blks,p->bai->nr_free_blks,p->bai->nr_free_blks_prepared,p->bai->nr_clean_blks,p->bai->nr_dirty_blks,p->bai->nr_bad_blks);

	return 0;
}

uint32_t bdbm_page_ftl_map_lpa_to_ppa (struct bdbm_drv_info* bdi, uint64_t *lpa, struct bdbm_phyaddr_t* phyaddr)
{
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct bdbm_page_mapping_entry* me = NULL;
	int k;

	/* is it a valid logical address */
	for (k = 0; k < np->nr_subpages_per_page; k++) {
		if (lpa[k] == -1ULL) {
			/* the correpsonding subpage must be set to invalid for gc */
			bdbm_abm_invalidate_page (
				p->bai, 
				phyaddr->channel_no, 
				phyaddr->chip_no,
				phyaddr->block_no,
				phyaddr->page_no,
				k
			);
			continue;
		}

		if (lpa[k] >= np->nr_subpages_per_ssd) {
			bdbm_error ("LPA is beyond logical space (%llX)", lpa[k]);
			return 1;
		}

		/* get the mapping entry for lpa */
		me = &p->ptr_mapping_table[lpa[k]];
		bdbm_bug_on (me == NULL);

		/* update the mapping table */
		if (me->status == PFTL_PAGE_VALID) {
			bdbm_abm_invalidate_page (
				p->bai, 
				me->phyaddr.channel_no, 
				me->phyaddr.chip_no,
				me->phyaddr.block_no,
				me->phyaddr.page_no,
				me->sp_off
			);
		}
		me->status = PFTL_PAGE_VALID;
		me->phyaddr.channel_no = phyaddr->channel_no;
		me->phyaddr.chip_no = phyaddr->chip_no;
		me->phyaddr.block_no = phyaddr->block_no;
		me->phyaddr.page_no = phyaddr->page_no;
		me->sp_off = k;
	}

	return 0;
}

uint32_t bdbm_page_ftl_get_ppa (
	struct bdbm_drv_info* bdi, 
	uint64_t lpa,
	struct bdbm_phyaddr_t* ppa,
	uint8_t* sp_off)
{
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct bdbm_page_mapping_entry* me = NULL;
	uint32_t ret;
	/* is it a valid logical address */
	if (lpa >= np->nr_subpages_per_ssd) {
		bdbm_error ("A given lpa is beyond logical space (%lu)", lpa);
		return 1;
	}

	/* get the mapping entry for lpa */
	me = &p->ptr_mapping_table[lpa];

	/* NOTE: sometimes a file system attempts to read 
	 * a logical address that was not written before.
	 * in that case, we return 'address 0' */
	if (me->status != PFTL_PAGE_VALID) {
		ppa->channel_no = 0;
		ppa->chip_no = 0;
		ppa->block_no = 0;
		ppa->page_no = 0;
		*sp_off = 0;
		ret = 1;
	} else {
		ppa->channel_no = me->phyaddr.channel_no;
		ppa->chip_no = me->phyaddr.chip_no;
		ppa->block_no = me->phyaddr.block_no;
		ppa->page_no = me->phyaddr.page_no;
		*sp_off = me->sp_off;
		ret = 0;
	}

	return ret;
}

uint32_t bdbm_page_ftl_invalidate_lpa (struct bdbm_drv_info* bdi, uint64_t lpa, uint64_t len)
{	
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct bdbm_page_mapping_entry* me = NULL;
	uint64_t loop;

	/* check the range of input addresses */
	if ((lpa + len) > np->nr_subpages_per_ssd) {
		bdbm_warning ("LPA is beyond logical space (%lu = %lu+%lu) %lu", 
			lpa+len, lpa, len, np->nr_subpages_per_ssd);
		return 1;
	}

	/* make them invalid */
	for (loop = lpa; loop < (lpa + len); loop++) {
		me = &p->ptr_mapping_table[loop];
		if (me->status == PFTL_PAGE_VALID) {
			bdbm_abm_invalidate_page (
				p->bai, 
				me->phyaddr.channel_no, 
				me->phyaddr.chip_no,
				me->phyaddr.block_no,
				me->phyaddr.page_no,
				me->sp_off
			);
			me->status = PFTL_PAGE_INVALID;
		}
	}

	return 0;
}

uint8_t bdbm_page_ftl_is_gc_needed (struct bdbm_drv_info* bdi)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	uint64_t nr_total_blks = bdbm_abm_get_nr_total_blocks (p->bai);
	uint64_t nr_free_blks = bdbm_abm_get_nr_free_blocks (p->bai);

	/* invoke gc when remaining free blocks are less than 1% of total blocks */
	if ((nr_free_blks * 100 / nr_total_blks) <= 4) {
		return 1;
	}

	/* invoke gc when there is only one dirty block (for debugging) */
	/*
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	if (bdbm_abm_get_nr_dirty_blocks (p->bai) > 1) {
		return 1;
	}
	*/

	return 0;
}

/* VICTIM SELECTION - First Selection:
 * select the first dirty block in a list */
struct bdbm_abm_block_t* __bdbm_page_ftl_victim_selection (
	struct bdbm_drv_info* bdi,
	uint64_t channel_no,
	uint64_t chip_no)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_abm_block_t* a = NULL;
	struct bdbm_abm_block_t* b = NULL;
	struct list_head* pos = NULL;

	a = p->ac_bab[channel_no*np->nr_chips_per_channel + chip_no];
	bdbm_abm_list_for_each_dirty_block (pos, p->bai, channel_no, chip_no) {
		b = bdbm_abm_fetch_dirty_block (pos);
		if (a != b)
			break;
		b = NULL;
	}

	return b;
}

/* VICTIM SELECTION - Greedy:
 * select a dirty block with a small number of valid pages */
struct bdbm_abm_block_t* __bdbm_page_ftl_victim_selection_greedy (
	struct bdbm_drv_info* bdi,
	uint64_t channel_no,
	uint64_t chip_no)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_abm_block_t* a = NULL;
	struct bdbm_abm_block_t* b = NULL;
	struct bdbm_abm_block_t* v = NULL;
	struct list_head* pos = NULL;

	a = p->ac_bab[channel_no*np->nr_chips_per_channel + chip_no];

	bdbm_abm_list_for_each_dirty_block (pos, p->bai, channel_no, chip_no) {
		b = bdbm_abm_fetch_dirty_block (pos);
		if (a == b)
			continue;
		if (b->nr_invalid_subpages == np->nr_subpages_per_block) {
			v = b;
			break;
		}
		if (v == NULL) {
			v = b;
			continue;
		}
		if (b->nr_invalid_subpages > v->nr_invalid_subpages)
			v = b;
	}

	return v;
}
uint8_t bdbm_page_ftl_gc_max_threshold (struct bdbm_drv_info* bdi)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	uint64_t nr_total_blks = bdbm_abm_get_nr_total_blocks (p->bai);
	uint64_t nr_free_blks = bdbm_abm_get_nr_free_blocks (p->bai);

	/* invoke gc when remaining free blocks are less than 1% of total blocks */
	if ((nr_free_blks * 100 / nr_total_blks) > 5) {
		return 1;
	}

	return 0;
}
#if GC_THREAD_ENABLED
uint32_t bdbm_page_ftl_signal_gc (struct bdbm_drv_info* bdi)
{
    struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
    sem_post (&p->gc_sem);
}
#endif

#if GC_THREAD_ENABLED
void *gc_thread (void *arg)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct bdbm_drv_info* bdi = (struct bdbm_drv_info *)arg;
	cpu_set_t cpuset;
	int ret_val;
	CPU_ZERO (&cpuset);
	CPU_SET (3 , &cpuset);
	ret_val = pthread_setaffinity_np (pthread_self (),sizeof (cpu_set_t),&cpuset);
	if (ret_val != 0) {
		perror ("pthread_setset_thread_affinity_np");
	}

	while (p->bdbmIsAlive) {
		sem_wait (&p->gc_sem);
		if (!bdbm_page_ftl_gc_max_threshold(bdi)) {
			bdbm_page_ftl_do_gc (bdi);
		}
		/* While GC's working, multiple gc activation signals might arrive;
		 * Just remove them all without blocking */
		while (!sem_trywait (&p->gc_sem));
	}

	pthread_exit (NULL);
}
#endif

static void hlm_reqs_write_compaction (
		struct bdbm_hlm_req_gc_t* dst,
		struct bdbm_hlm_req_gc_t* src,
		nand_params_t* np)
{
	uint64_t dst_loop = 0, dst_kp = 0, src_kp = 0, i = 0;
	uint64_t nr_punits = np->nr_chips_per_channel * np->nr_channels;

	struct bdbm_llm_req_t* dst_r = NULL;
	struct bdbm_llm_req_t* src_r = NULL;

	dst->nr_reqs = 0;

	dst_r = dst->llm_reqs[0];
	bdbm_memset (dst_r->kpg_flags, MEMFLAG_FRAG_PAGE, sizeof (uint8_t) * np->nr_subpages_per_page);
	dst->nr_reqs = 1;
	for (i = 0; i < nr_punits * np->nr_pages_per_block; i++) {
		src_r = src->llm_reqs[i];

		for (src_kp = 0; src_kp < np->nr_subpages_per_page; src_kp++) {
			if (src_r->kpg_flags[src_kp] == MEMFLAG_KMAP_PAGE) {
				/* if src has data, copy it to dst */
				dst_r->kpg_flags[dst_kp] = src_r->kpg_flags[src_kp];
				dst_r->pptr_kpgs[dst_kp] = src_r->pptr_kpgs[src_kp];
				((int64_t*)dst_r->ptr_oob)[dst_kp] = ((int64_t*)src_r->ptr_oob)[src_kp];
			} else {
				/* otherwise, skip it */
				continue;
			}

			/* goto the next llm if all kps are full */
			dst_kp++;
			if (dst_kp == np->nr_subpages_per_page) {
				dst_kp = 0;
				dst_loop++;
				//dst_r++;
				dst_r = dst->llm_reqs[dst_loop];
				bdbm_memset (dst_r->kpg_flags, MEMFLAG_FRAG_PAGE, sizeof (uint8_t) * np->nr_subpages_per_page);
				dst->nr_reqs++;
			}
		}
	}
}

/* TODO: need to improve it for background gc */
uint32_t bdbm_page_ftl_do_gc (struct bdbm_drv_info* bdi)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_hlm_req_gc_t* hlm_gc = &p->gc_hlm;
	struct bdbm_hlm_req_gc_t* hlm_gc_w = &p->gc_hlm_w;
	uint64_t nr_gc_blks = 0;
	uint64_t nr_llm_reqs = 0;
	uint64_t nr_punits = 0;
	uint64_t i, j, k;

/*	struct bdbm_stopwatch sw; */

	nr_punits = np->nr_channels * np->nr_chips_per_channel;

	/* choose victim blocks for individual parallel units */
	bdbm_memset (p->gc_bab, 0x00, sizeof (struct bdbm_abm_block_t*) * nr_punits);
/*	bdbm_stopwatch_start (&sw); */
#if GC_THREAD_ENABLED
	pthread_mutex_lock(&gc_lock);
#endif


	while (atomic_read(&act_desc_cnt)) {
		usleep(100);
	}
	for (i = 0, nr_gc_blks = 0; i < np->nr_channels; i++) {
		for (j = 0; j < np->nr_chips_per_channel; j++) {
			struct bdbm_abm_block_t* b; 
			if ((b = __bdbm_page_ftl_victim_selection_greedy (bdi, i, j))) {
				p->gc_bab[nr_gc_blks] = b;
				nr_gc_blks++;
			}
		}
	}
	if (nr_gc_blks < nr_punits) {
		/* TODO: we need to implement a load balancing feature to avoid this */
		/*bdbm_warning ("TODO: this warning will be removed with load-balancing");*/
		printf("nr_gc_blks < nr_punits\n");
		pthread_mutex_unlock(&gc_lock);
		return 0;
	}

	/* build hlm_req_gc for reads */
	for (i = 0, nr_llm_reqs = 0; i < nr_gc_blks; i++) {
		struct bdbm_abm_block_t* b = p->gc_bab[i];
		if (b == NULL)
			break;
		for (j = 0; j < np->nr_pages_per_block; j++) {
			struct bdbm_llm_req_t* r = hlm_gc->llm_reqs[nr_llm_reqs];
			int has_valid = 0;
			for (k = 0; k < np->nr_subpages_per_page; k++) {
				if (b->pst[j*np->nr_subpages_per_page+k] != BDBM_ABM_SUBPAGE_INVALID) {
					has_valid = 1;
					//r->lpa[k] = -1; /* the subpage contains new data */
					r->kpg_flags[k] = MEMFLAG_KMAP_PAGE;
				} else {
					//r->lpa[k] = -1;	/* the subpage contains obsolate data */
					r->kpg_flags[k] = MEMFLAG_FRAG_PAGE;
				}
			}
			if (has_valid) {
				r->req_type = REQTYPE_GC_READ;
				r->ptr_hlm_req = (void*)hlm_gc;
				r->phyaddr = &r->phyaddr_r;
				r->phyaddr->channel_no = b->channel_no;
				r->phyaddr->chip_no = b->chip_no;
				r->phyaddr->block_no = b->block_no;
				r->phyaddr->page_no = j;                       // vvdn every block page number starts from zero again //
				r->ret = 0;
				nr_llm_reqs++;
			}
		}
	}

	/*
	bdbm_msg ("----------------------------------------------");
	bdbm_msg ("gc-victim: %lu pages, %lu blocks, %lu us", 
		nr_llm_reqs, nr_gc_blks, bdbm_stopwatch_get_elapsed_time_us (&sw));
	*/

	/* wait until Q in llm becomes empty 
	 * TODO: it might be possible to further optimize this */
	bdi->ptr_llm_inf->flush (bdi);

	if (nr_llm_reqs == 0) 
		goto erase_blks;

	/* send read reqs to llm */
	hlm_gc->req_type = REQTYPE_GC_READ;
	hlm_gc->nr_done_reqs = 0;
	hlm_gc->nr_reqs = nr_llm_reqs;
/*	bdbm_reinit_completion (hlm_gc->gc_done);*/
#if 0
	for (i = 0; i < nr_llm_reqs; i++) {
		if ((bdi->ptr_llm_inf->make_req (bdi, &hlm_gc->llm_reqs[i])) != 0) {
			bdbm_error ("llm_make_req failed");
			bdbm_bug_on (1);
		}
	}
#endif
	bdi->ptr_llm_inf->make_req (bdi, hlm_gc->llm_reqs);
	sem_wait (&hlm_gc->gc_done);
/*	bdbm_wait_for_completion (hlm_gc->gc_done); */

	/* build hlm_req_gc for writes */
	hlm_reqs_write_compaction (hlm_gc_w, hlm_gc, np);
	nr_llm_reqs = hlm_gc_w->nr_reqs;

	for (i = 0; i < nr_llm_reqs; i++) {
		struct bdbm_llm_req_t* r = hlm_gc_w->llm_reqs[i];
		r->req_type = REQTYPE_GC_WRITE;	/* change to write */

		for (k = 0; k < np->nr_subpages_per_page; k++) {
			/* move subpages that contain new data */
			if (r->kpg_flags[k] == MEMFLAG_KMAP_PAGE) {
				r->lpa[k] = ((uint64_t*)r->ptr_oob)[k];
			} else if (r->kpg_flags[k] == MEMFLAG_FRAG_PAGE) {
				((uint64_t*)r->ptr_oob)[k] = -1;
				r->lpa[k] = -1;
			} else {
				bdbm_bug_on (1);
			}
		}
		r->ptr_hlm_req = (void*)hlm_gc_w;
		r->phyaddr = &r->phyaddr_w;

		if (bdbm_page_ftl_get_free_ppa (bdi, r->lpa, r->phyaddr) != 0) {
			bdbm_error ("bdbm_page_ftl_get_free_ppa failed");
			bdbm_bug_on (1);
		}
		if (bdbm_page_ftl_map_lpa_to_ppa (bdi, r->lpa, r->phyaddr) != 0) {
			bdbm_error ("bdbm_page_ftl_map_lpa_to_ppa failed");
			bdbm_bug_on (1);
		}
	}

	/* send write reqs to llm */
	hlm_gc_w->req_type = REQTYPE_GC_WRITE;
	hlm_gc_w->nr_done_reqs = 0;
	hlm_gc_w->nr_reqs = nr_llm_reqs;
/*	bdbm_reinit_completion (hlm_gc->gc_done); */
#if 0
	for (i = 0; i < nr_llm_reqs; i++) {
		if ((bdi->ptr_llm_inf->make_req (bdi, &hlm_gc->llm_reqs[i])) != 0) {
			bdbm_error ("llm_make_req failed");
			bdbm_bug_on (1);
		}
	}
#endif
	bdi->ptr_llm_inf->make_req (bdi, hlm_gc_w->llm_reqs);
	sem_wait (&hlm_gc_w->gc_done);
/*	bdbm_wait_for_completion (hlm_gc->gc_done); */

	/* erase blocks */
erase_blks:
	for (i = 0; i < nr_gc_blks; i++) {
		struct bdbm_abm_block_t* b = p->gc_bab[i];
		struct bdbm_llm_req_t* r = hlm_gc->llm_reqs[i];
		r->req_type = REQTYPE_GC_ERASE;
		r->lpa = NULL; /* lpa is not available now */
		r->ptr_hlm_req = (void*)hlm_gc;
		r->phyaddr = &r->phyaddr_w;
		r->phyaddr->channel_no = b->channel_no;
		r->phyaddr->chip_no = b->chip_no;
		r->phyaddr->block_no = b->block_no;
		r->phyaddr->page_no = 0;
		r->ret = 0;
	}

	/* send erase reqs to llm */
	hlm_gc->req_type = REQTYPE_GC_ERASE;
	hlm_gc->nr_done_reqs = 0;
	hlm_gc->nr_reqs = nr_gc_blks;
/*	bdbm_reinit_completion (hlm_gc->gc_done); */
#if 0
	for (i = 0; i < nr_gc_blks; i++) {
		if ((bdi->ptr_llm_inf->make_req (bdi, &hlm_gc->llm_reqs[i])) != 0) {
			bdbm_error ("llm_make_req failed");
			bdbm_bug_on (1);
		}
	}
#endif
	bdi->ptr_llm_inf->make_req (bdi, hlm_gc->llm_reqs);
/*	bdbm_wait_for_completion (hlm_gc->gc_done); */
	sem_wait (&hlm_gc->gc_done);

	/* FIXME: what happens if block erasure fails */
	for (i = 0; i < nr_gc_blks; i++) {
		uint8_t ret = 0;
		struct bdbm_abm_block_t* b = p->gc_bab[i];
		if (hlm_gc->llm_reqs[i]->ret != 0) 
			ret = 1;	/* bad block */
/*
#ifdef EMULATE_BAD_BLOCKS
		{
			struct bdbm_llm_req_t* r = (struct bdbm_llm_req_t*)&hlm_gc->llm_reqs[i];
			if (r->phyaddr->block_no % 8 == 0) {
				bdbm_msg (" -- FTL: b:%lu c:%lu b:%lu (ret=%u)",
						r->phyaddr->channel_no, 
						r->phyaddr->chip_no, 
						r->phyaddr->block_no,
						r->ret);
			}
		}
#endif
*/
		bdbm_abm_erase_block (p->bai, b->channel_no, b->chip_no, b->block_no, ret);
	}

	/* measure gc elapsed time */
/*	bdbm_complete (hlm_gc->gc_done);           */
#if GC_THREAD_ENABLED
	pthread_mutex_unlock(&gc_lock);
#endif
	return 0;
}

/* for snapshot */
uint32_t bdbm_page_ftl_load (struct bdbm_drv_info* bdi, const char* fn)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	uint8_t ret;
	uint8_t load_from_nand = 0;
#ifdef MGMT_DATA_ON_NAND
	uint8_t load_from_nand = 1;
#endif

	if (ftl_initialised == 1 && ftl_stored == 1) {
		goto get_act_blks;
	} else if (ftl_initialised == 1 && ftl_stored == 0) {
		return 0;
	}
	if ((np->device_type == DEVICE_TYPE_DRAGON_FIRE) && load_from_nand) {
		ret = bdi->ptr_dm_inf->load(bdi, (void*)p->ptr_mapping_table, (void*)p->bai);
	} else {
		struct bdbm_page_mapping_entry* me;
		//struct file* fp = NULL;
		//uint64_t i, pos = 0;
		FILE *fp = NULL;
		uint64_t i;
		ret = bdi->ptr_dm_inf->load(bdi, (void*)p->ptr_mapping_table, (void*)p->bai);
		/* step1: load abm */
        if (system (" ls /run/ftl_pmt/metaData.tar.gz &> /dev/null")){
            goto get_act_blks;
        }
		system ("cp /run/ftl_pmt/metaData.tar.gz /run/ &> /dev/null");
        ret = system ("tar -xvzf /run/metaData.tar.gz -C /run/ &> /dev/null");
        if(ret) {
            bdbm_error ("PMT TAR Extraction Failed %u\n", ret);
        }

		if (bdbm_abm_load (p->bai, "/run/abm.dat") != 0) {
			//bdbm_error ("bdbm_abm_load failed");
			syslog (LOG_INFO,"Failed to load abm\n");
			goto get_act_blks;
		}

		/* step2: load mapping table */
		//if ((fp = bdbm_fopen (fn, O_RDWR, 0777)) == NULL) {
		if ((fp=fopen(fn,"r")) == NULL) {
			bdbm_error ("bdbm_fopen failed");
			goto get_act_blks;
		}

		me = p->ptr_mapping_table;
		syslog (LOG_INFO,"Reading ftl.dat..............\n");
		for (i = 0; i < np->nr_subpages_per_ssd; i++) {
			//pos += bdbm_fread (fp, pos, (uint8_t*)&me[i], sizeof (struct bdbm_page_mapping_entry));
			fread((uint8_t*)&me[i], 1, sizeof (struct bdbm_page_mapping_entry), fp);
			if (me[i].status != PFTL_PAGE_NOT_ALLOCATED &&
					me[i].status != PFTL_PAGE_VALID &&
					me[i].status != PFTL_PAGE_INVALID &&
					me[i].status != PFTL_PAGE_INVALID_ADDR) {
				bdbm_msg ("snapshot: invalid status = %u", me[i].status);
			}
		}
		//bdbm_fclose (fp);
		fclose(fp);
	}
	/* step3: get active blocks */
get_act_blks:
	syslog (LOG_INFO,"Gathering active blocks from parallel units....\n");
	if (__bdbm_page_ftl_get_active_blocks (np, p->bai, p->ac_bab) != 0) {
		bdbm_error ("__bdbm_page_ftl_get_active_blocks failed");
		//bdbm_fclose (fp);
		//fclose(fp);
		return 1;
	}
	p->curr_puid = 0;
	p->curr_page_ofs = 0;

	return 0;
}

uint32_t bdbm_page_ftl_store (struct bdbm_drv_info* bdi, const char* fn)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_page_mapping_entry* me;
	struct bdbm_abm_block_t* b = NULL;
	//struct file* fp = NULL;
	FILE *fp=NULL;
	//uint64_t pos = 0;
	uint64_t i, j, k;
	uint32_t ret;
	uint8_t store_to_nand = 0;
#ifdef MGMT_DATA_ON_NAND
	uint8_t store_to_nand = 1;
#endif
	/* step1: make active blocks invalid (it's ugly!!!) */
	//if ((fp = bdbm_fopen (fn, O_CREAT | O_WRONLY, 0777)) == NULL) {
	if ((np->device_type != DEVICE_TYPE_DRAGON_FIRE) || (!store_to_nand)) {
		if ((fp = fopen(fn,"w")) == NULL) {
			bdbm_error ("bdbm_fopen failed");
			return 1;
		}
	}

	while (1) {
		/* get the channel & chip numbers */
		i = p->curr_puid % np->nr_channels;
		j = p->curr_puid / np->nr_channels;

		/* get the physical offset of the active blocks */
		b = p->ac_bab[i*np->nr_chips_per_channel + j];

		/* invalidate remaining pages */
		for (k=0; k<np->nr_subpages_per_page; k++) {
			bdbm_abm_invalidate_page (p->bai, 
					b->channel_no, b->chip_no, b->block_no, p->curr_page_ofs, k);
		}
		bdbm_bug_on (b->channel_no != i);
		bdbm_bug_on (b->chip_no != j);

		/* go to the next parallel unit */
		if ((p->curr_puid + 1) == p->nr_punits) {
			p->curr_puid = 0;
			p->curr_page_ofs++;	/* go to the next page */

			/* see if there are sufficient free pages or not */
			if (p->curr_page_ofs == np->nr_pages_per_block) {
				p->curr_page_ofs = 0;
				break;
			}
		} else {
			p->curr_puid++;
		}
	}

	if (np->device_type == DEVICE_TYPE_DRAGON_FIRE && store_to_nand) {
		ret = bdi->ptr_dm_inf->store(bdi);
	} else {
		ret = bdi->ptr_dm_inf->store(bdi);
		/* step2: store mapping table */
		me = p->ptr_mapping_table;
		syslog(LOG_INFO,"Writing to ftl.dat.............\n");
		for (i = 0; i < np->nr_subpages_per_ssd; i++) {
			//pos += bdbm_fwrite (fp, pos, (uint8_t*)&me[i], sizeof (struct bdbm_page_mapping_entry));
			fwrite((uint8_t*)&me[i], 1, sizeof (struct bdbm_page_mapping_entry), fp);
		}
		//bdbm_fsync (fp);
		//bdbm_fclose (fp);
		fclose(fp);

		/* step3: store abm */
		ret = bdbm_abm_store (p->bai, "/run/abm.dat");
        ret = system ("cd /run && tar -cvzf /run/metaData.tar.gz ftl.dat abm.dat &> /dev/null");
        if(ret) {
            bdbm_error ("Making TAR failed %u\n", ret);
        }

		system ("cp /run/metaData.tar.gz /run/ftl_pmt/");
		system ("sync");
		system ("umount /dev/mmcblk0p1");
		system ("rm /run/metaData.tar.gz");
	    system ("rm /run/ftl.dat");
		system ("rm /run/abm.dat");
		system ("sync");
	}
	return ret;
}

void __bdbm_page_badblock_scan_eraseblks (
	struct bdbm_drv_info* bdi,
	uint64_t block_no)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_hlm_req_gc_t* hlm_gc = &p->gc_hlm;
	uint64_t i, j;

	/* setup blocks to erase */
	bdbm_memset (p->gc_bab, 0x00, sizeof (struct bdbm_abm_block_t*) * p->nr_punits);
	for (i = 0; i < np->nr_channels; i++) {
		for (j = 0; j < np->nr_chips_per_channel; j++) {
			struct bdbm_abm_block_t* b = NULL;
			struct bdbm_llm_req_t* r = NULL;
			uint64_t punit_id = i*np->nr_chips_per_channel+j;

			if ((b = bdbm_abm_get_block (p->bai, i, j, block_no)) == NULL) {
				bdbm_error ("oops! bdbm_abm_get_block failed");
				bdbm_bug_on (1);
			}
			p->gc_bab[punit_id] = b;

			r = hlm_gc->llm_reqs[punit_id];
			r->req_type = REQTYPE_GC_ERASE;
			//r->lpa = -1ULL; /* lpa is not available now */
			r->lpa = NULL; /* lpa is not available now */
			r->ptr_hlm_req = (void*)hlm_gc;
			r->phyaddr = &r->phyaddr_w;
			r->phyaddr->channel_no = b->channel_no;
			r->phyaddr->chip_no = b->chip_no;
			r->phyaddr->block_no = b->block_no;
			r->phyaddr->page_no = 0;
			r->ret = 0;
		}
	}

	/* send erase reqs to llm */
	hlm_gc->req_type = REQTYPE_GC_ERASE;
	hlm_gc->nr_done_reqs = 0;
	hlm_gc->nr_reqs = p->nr_punits;
/*	bdbm_reinit_completion (hlm_gc->gc_done); */
#if 0
	for (i = 0; i < p->nr_punits; i++) {
		if ((bdi->ptr_llm_inf->make_req (bdi, &hlm_gc->llm_reqs[i])) != 0) {
			bdbm_error ("llm_make_req failed");
			bdbm_bug_on (1);
		}
	}
#endif
	bdi->ptr_llm_inf->make_req (bdi, hlm_gc->llm_reqs);
/*	bdbm_wait_for_completion (hlm_gc->gc_done); */

	for (i = 0; i < p->nr_punits; i++) {
		uint8_t ret = 0;
		struct bdbm_abm_block_t* b = p->gc_bab[i];

		if (hlm_gc->llm_reqs[i]->ret != 0) {
			ret = 1; /* bad block */
		}

		bdbm_abm_erase_block (p->bai, b->channel_no, b->chip_no, b->block_no, ret);
	}

	/* measure gc elapsed time */
/*	bdbm_complete (hlm_gc->gc_done); */
}

uint32_t bdbm_page_badblock_scan (struct bdbm_drv_info* bdi)
{
	struct bdbm_page_ftl_private* p = _ftl_page_ftl.ptr_private;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_page_mapping_entry* me = NULL;
	uint64_t i = 0;
	uint32_t ret = 0;

	bdbm_msg ("[WARNING] 'bdbm_page_badblock_scan' is called! All of the flash blocks will be erased!!!");

	/* step1: reset the page-level mapping table */
	bdbm_msg ("step1: reset the page-level mapping table");
	me = p->ptr_mapping_table;
	for (i = 0; i < np->nr_pages_per_ssd; i++) {
		me[i].status = PFTL_PAGE_NOT_ALLOCATED;
		me[i].phyaddr.channel_no = PFTL_PAGE_INVALID_ADDR;
		me[i].phyaddr.chip_no = PFTL_PAGE_INVALID_ADDR;
		me[i].phyaddr.block_no = PFTL_PAGE_INVALID_ADDR;
		me[i].phyaddr.page_no = PFTL_PAGE_INVALID_ADDR;
	}

	/* step2: erase all the blocks */
	bdi->ptr_llm_inf->flush (bdi);
	for (i = 0; i < np->nr_blocks_per_chip; i++) {
		__bdbm_page_badblock_scan_eraseblks (bdi, i);
	}

	/* step3: store abm */
	if ((ret = bdbm_abm_store (p->bai, "/usr/share/bdbm_drv/abm.dat"))) {
		bdbm_error ("bdbm_abm_store failed");
		return 1;
	}

	/* step4: get active blocks */
	bdbm_msg ("step2: get active blocks");
	if (__bdbm_page_ftl_get_active_blocks (np, p->bai, p->ac_bab) != 0) {
		bdbm_error ("__bdbm_page_ftl_get_active_blocks failed");
		return 1;
	}
	p->curr_puid = 0;
	p->curr_page_ofs = 0;

	bdbm_msg ("done");
	 
	return 0;
}
struct bdbm_abm_info* bdbm_get_bai(struct bdbm_drv_info* _bdi) {
	struct bdbm_page_ftl_private* p;
	p = (struct bdbm_page_ftl_private*)(_bdi->ptr_ftl_inf->ptr_private);
	struct bdbm_abm_info* bai;
	bai = p->bai;
	return bai;
}

struct bdbm_abm_block_t** bdbm_get_ac_bab(struct bdbm_drv_info* _bdi) {
	struct bdbm_page_ftl_private* p;
	p = (struct bdbm_page_ftl_private*)(_bdi->ptr_ftl_inf->ptr_private);
	struct bdbm_abm_block_t** ac_bab;
	ac_bab = p->ac_bab;
	return ac_bab;
}

uint64_t bdbm_hidden_blk_cnt (struct bdbm_drv_info* _bdi)
{
	struct bdbm_page_ftl_private* p;
	p = (struct bdbm_page_ftl_private*)(_bdi->ptr_ftl_inf->ptr_private);
	return (p->bai->nr_bad_blks + p->bai->nr_mgmt_blks);
}
