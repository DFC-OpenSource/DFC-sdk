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
#include <linux/slab.h>
#include <linux/log2.h>

#include "../bdbm_drv.h"
#include "../params.h"
#include "../debug.h"

#include "abm.h"
#include "block_ftl.h"


#define DBG_ALLOW_INPLACE_UPDATE
/*#define ENABLE_LOG*/


/* FTL interface */
struct bdbm_ftl_inf_t _ftl_block_ftl = {
	.ptr_private = NULL,
	.create = bdbm_block_ftl_create,
	.destroy = bdbm_block_ftl_destroy,
	.get_free_ppa = bdbm_block_ftl_get_free_ppa,
	.get_ppa = bdbm_block_ftl_get_ppa,
	.map_lpa_to_ppa = bdbm_block_ftl_map_lpa_to_ppa,
	.invalidate_lpa = bdbm_block_ftl_invalidate_lpa,
	.do_gc = bdbm_block_ftl_do_gc,
	.is_gc_needed = bdbm_block_ftl_is_gc_needed,	

	.scan_badblocks = bdbm_block_ftl_badblock_scan,
	.load = NULL,
	.store = NULL,
	/*.load = bdbm_block_ftl_load,*/
	/*.store = bdbm_block_ftl_store,*/

	/* custom interfaces for rsd */
	.get_segno = bdbm_block_ftl_get_segno,
};


/* data structures for block-level FTL */
enum BDBM_BFTL_BLOCK_STATUS {
	BFTL_NOT_ALLOCATED = 0,
	BFTL_ALLOCATED,
	BFTL_DEAD,
};

struct bdbm_block_mapping_entry {
	uint8_t status;	/* BDBM_BFTL_BLOCK_STATUS */
	uint64_t channel_no;
	uint64_t chip_no;
	uint64_t block_no;
	uint64_t rw_pg_ofs; /* recently-written page offset */
	uint8_t* pst;	/* status of pages in a block */
};

struct bdbm_block_ftl_private {
	uint64_t nr_segments;	/* a segment is the unit of mapping */
	uint64_t nr_pages_per_segment;	/* how many pages belong to a segment */
	uint64_t nr_blocks_per_segment;	/* how many blocks belong to a segment */

	bdbm_spinlock_t ftl_lock;
	struct bdbm_abm_info* abm;
	struct bdbm_block_mapping_entry** ptr_mapping_table;
	struct bdbm_abm_block_t** gc_bab;
	struct bdbm_hlm_req_gc_t gc_hlm;

	uint64_t* nr_trimmed_pages;
	int64_t nr_dead_segments;
};


uint32_t __hlm_rsd_make_rm_seg (
	struct bdbm_drv_info* bdi, 
	uint32_t seg_no);

/* some internal functions */
static inline
uint64_t __bdbm_block_ftl_get_segment_no (
	struct bdbm_block_ftl_private *p, 
	uint64_t lpa) 
{
	return lpa / p->nr_pages_per_segment;
}

static inline
uint64_t __bdbm_block_ftl_get_block_no (
	struct bdbm_block_ftl_private *p, 
	uint64_t lpa) 
{
	return (lpa % p->nr_pages_per_segment) % p->nr_blocks_per_segment;
}

static inline
uint64_t __bdbm_block_ftl_get_page_ofs (
	struct bdbm_block_ftl_private *p, 
	uint64_t lpa) 
{
	return (lpa % p->nr_pages_per_segment) / p->nr_blocks_per_segment;
}

uint32_t __bdbm_block_ftl_do_gc_segment (
	struct bdbm_drv_info* bdi,
	uint64_t seg_no);

/* functions for block-level FTL */
uint32_t bdbm_block_ftl_create (struct bdbm_drv_info* bdi)
{
	struct bdbm_abm_info* abm = NULL;
	struct bdbm_block_ftl_private* p = NULL;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);

	uint64_t nr_segments;
	uint64_t nr_blocks_per_segment;
	uint64_t nr_pages_per_segment;
	uint64_t i, j;

	/* create 'bdbm_abm_info' */
	if ((abm = bdbm_abm_create (np, 0)) == NULL) {
		bdbm_error ("bdbm_abm_create failed");
		return 1;
	}

	/* create a private data structure */
	if ((p = (struct bdbm_block_ftl_private*)bdbm_zmalloc 
			(sizeof (struct bdbm_block_ftl_private))) == NULL) {
		bdbm_error ("bdbm_malloc failed");
		return 1;
	}
	_ftl_block_ftl.ptr_private = (void*)p;

	/* calculate # of mapping entries */
	nr_blocks_per_segment = np->nr_chips_per_channel * np->nr_channels;
	nr_pages_per_segment = np->nr_pages_per_block * nr_blocks_per_segment;
	nr_segments = np->nr_blocks_per_chip;

	/* intiailize variables for ftl */
	p->nr_segments = nr_segments;
	p->nr_dead_segments = 0;
	p->nr_blocks_per_segment = nr_blocks_per_segment;
	p->nr_pages_per_segment = nr_pages_per_segment;
	p->abm = abm;

	bdbm_spin_lock_init (&p->ftl_lock);

	/* initialize a mapping table */
	if ((p->ptr_mapping_table = (struct bdbm_block_mapping_entry**)bdbm_zmalloc 
			(sizeof (struct bdbm_block_mapping_entry*) * p->nr_segments)) == NULL) {
		bdbm_error ("bdbm_malloc failed");
		goto fail;
	}
	for (i= 0; i < p->nr_segments; i++) {
		if ((p->ptr_mapping_table[i] = (struct bdbm_block_mapping_entry*)bdbm_zmalloc
				(sizeof (struct bdbm_block_mapping_entry) * 
				p->nr_blocks_per_segment)) == NULL) {
			bdbm_error ("bdbm_malloc failed");
			goto fail;
		}
		for (j = 0; j < p->nr_blocks_per_segment; j++) {
			p->ptr_mapping_table[i][j].status = BFTL_NOT_ALLOCATED;
			p->ptr_mapping_table[i][j].channel_no = -1;
			p->ptr_mapping_table[i][j].chip_no = -1;
			p->ptr_mapping_table[i][j].block_no = -1;
			p->ptr_mapping_table[i][j].rw_pg_ofs = -1;
			p->ptr_mapping_table[i][j].pst = 
				(uint8_t*)bdbm_zmalloc (sizeof (uint8_t) * np->nr_pages_per_block);
		}
	}
	if ((p->nr_trimmed_pages = (uint64_t*)bdbm_zmalloc 
			(sizeof (uint64_t) * p->nr_segments)) == NULL) {
		bdbm_error ("bdbm_malloc failed");
		goto fail;
	}

	/* initialize gc_hlm */
	if ((p->gc_bab = (struct bdbm_abm_block_t**)bdbm_zmalloc 
			(sizeof (struct bdbm_abm_block_t*) * p->nr_blocks_per_segment)) == NULL) {
		bdbm_error ("bdbm_zmalloc failed");
		goto fail;
	}
	if ((p->gc_hlm.llm_reqs = (struct bdbm_llm_req_t*)bdbm_zmalloc
			(sizeof (struct bdbm_llm_req_t) * p->nr_blocks_per_segment)) == NULL) {
		bdbm_error ("bdbm_zmalloc failed");
		goto fail;
	}
	bdbm_init_completion (p->gc_hlm.gc_done);
	bdbm_complete (p->gc_hlm.gc_done);

	bdbm_msg ("nr_segments = %llu, nr_blocks_per_segment = %llu, nr_pages_per_segment = %llu",
		p->nr_segments, p->nr_blocks_per_segment, p->nr_pages_per_segment);

	return 0;

fail:
	bdbm_block_ftl_destroy (bdi);
	return 1;
}

void bdbm_block_ftl_destroy (
	struct bdbm_drv_info* bdi)
{
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);
	uint64_t i, j;

	if (p == NULL)
		return;

	if (p->nr_trimmed_pages != NULL)
		bdbm_free (p->nr_trimmed_pages);

	if (p->gc_bab)
		bdbm_free (p->gc_bab);

	if (p->gc_hlm.llm_reqs)
		bdbm_free (p->gc_hlm.llm_reqs);

	if (p->ptr_mapping_table != NULL) {
		for (i = 0; i < p->nr_segments; i++) {
			if (p->ptr_mapping_table[i] != NULL) {
				for (j = 0; j < p->nr_blocks_per_segment; j++) {
					if (p->ptr_mapping_table[i][j].pst)
						bdbm_free (p->ptr_mapping_table[i][j].pst);
				}
				bdbm_free (p->ptr_mapping_table[i]);
			}
		}
		bdbm_free (p->ptr_mapping_table);
	}

	if (p->abm != NULL)
		bdbm_abm_destroy (p->abm);

	bdbm_free (p);
}

uint32_t bdbm_block_ftl_get_ppa (
	struct bdbm_drv_info* bdi, 
	uint64_t lpa,
	struct bdbm_phyaddr_t* ppa)
{
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);
	struct bdbm_block_mapping_entry* e = NULL;
	uint64_t segment_no;
	uint64_t block_no;
	uint64_t page_ofs;

	segment_no = __bdbm_block_ftl_get_segment_no (p, lpa);
	block_no = __bdbm_block_ftl_get_block_no (p, lpa);
	page_ofs = __bdbm_block_ftl_get_page_ofs (p, lpa);
	e = &p->ptr_mapping_table[segment_no][block_no];

	if (e->status == BFTL_NOT_ALLOCATED) {
		/* NOTE: the host could send reads to not-allocated pages,
		 * especially when initialzing a file system;
		 * in that case, we just ignore requests */
		ppa->channel_no = 0;
		ppa->chip_no = 0;
		ppa->block_no = 0;
		ppa->page_no = 0;
		return 0;
	} 

	/* see if the mapping entry has valid physical locations */ 
	bdbm_bug_on (e->channel_no == -1);
	bdbm_bug_on (e->chip_no == -1);
	bdbm_bug_on (e->block_no == -1);

	/* return a phyical page address */
	ppa->channel_no = e->channel_no;
	ppa->chip_no = e->chip_no;
	ppa->block_no = e->block_no;
	ppa->page_no = page_ofs;

	return 0;
}

static uint64_t problem_seg_no = -1;

uint32_t bdbm_block_ftl_get_free_ppa (
	struct bdbm_drv_info* bdi, 
	uint64_t lpa,
	struct bdbm_phyaddr_t* ppa)
{
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);
	struct bdbm_block_mapping_entry* e = NULL;
	uint64_t segment_no;
	uint64_t block_no;
	uint64_t page_ofs;
	uint32_t ret = 1;

	segment_no = __bdbm_block_ftl_get_segment_no (p, lpa);
	block_no = __bdbm_block_ftl_get_block_no (p, lpa);
	page_ofs = __bdbm_block_ftl_get_page_ofs (p, lpa);
	e = &p->ptr_mapping_table[segment_no][block_no];

	if (e == NULL) {
		bdbm_msg ("[BUG] lpa: %llu (seg: %llu, blk: %llu, ofs: %llu",
			lpa, segment_no, block_no, page_ofs);
		bdbm_bug_on (1);
	}

	/* is it dead? */
	if (p->nr_trimmed_pages[segment_no] == p->nr_pages_per_segment) {
		/* perform gc for the segment */
		__bdbm_block_ftl_do_gc_segment (bdi, segment_no);
#ifdef ENABLE_LOG
		bdbm_msg ("E: [%llu] erase", segment_no);
#endif
	}

#ifdef ENABLE_LOG
	bdbm_msg ("W: [%llu] lpa: %llu ofs: %llu", segment_no, lpa, page_ofs);
#endif

	/* is it already mapped? */
	if (e->status == BFTL_ALLOCATED) {
		/* if so, see if the target block is writable or not */
		if (e->rw_pg_ofs < page_ofs) {
			/* [CASE 1] it is a writable block */
			ppa->channel_no = e->channel_no;
			ppa->chip_no = e->chip_no;
			ppa->block_no = e->block_no;
			ppa->page_no = page_ofs;
			ret = 0;

			if ((e->rw_pg_ofs + 1) != page_ofs) {
				bdbm_msg ("INFO: seg: %llu, %llu %llu", segment_no, (e->rw_pg_ofs + 1), page_ofs);
			}

		} else {
			/* [CASE 2] it is a not-writable block */
#ifdef DBG_ALLOW_INPLACE_UPDATE
			/* TODO: it will be an error case in our final implementation,
			 * but we temporarly allows this case */
			ppa->channel_no = e->channel_no;
			ppa->chip_no = e->chip_no;
			ppa->block_no = e->block_no;
			ppa->page_no = page_ofs;
			ret = 0;

			problem_seg_no = segment_no;

			/*#else*/
			bdbm_msg ("[%llu] [OVERWRITE] this should not occur (rw_pg_ofs:%llu page_ofs:%llu)", 
				segment_no, e->rw_pg_ofs, page_ofs);

			bdbm_msg ("[%llu] [# of trimmed pages = %llu, lpa = %llu",
				segment_no,
				p->nr_trimmed_pages[segment_no],
				lpa);
			/*bdbm_bug_on (1);*/
#endif
		}
	} else if (e->status == BFTL_NOT_ALLOCATED) {
		/* [CASE 3] allocate a new free block */
		uint64_t channel_no;
		uint64_t chip_no;
		struct bdbm_abm_block_t* b = NULL;
		struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);

		channel_no = block_no % np->nr_channels;
		chip_no = block_no / np->nr_channels;
		
		if ((b = bdbm_abm_get_free_block_prepare (p->abm, channel_no, chip_no)) != NULL) {
			bdbm_abm_get_free_block_commit (p->abm, b);

			ppa->channel_no = b->channel_no;
			ppa->chip_no = b->chip_no;
			ppa->block_no = b->block_no;
			ppa->page_no = page_ofs;
			ret = 0;
		} else {
			bdbm_error ("oops! bdbm_abm_get_free_block_prepare failed (%llu %llu)", channel_no, chip_no);
			bdbm_bug_on (1);
		}
	} else {
		bdbm_error ("'e->status' is not valid (%u)", e->status);
		bdbm_bug_on (1);
	}

	return ret;
}

uint32_t bdbm_block_ftl_map_lpa_to_ppa (
	struct bdbm_drv_info* bdi, 
	uint64_t lpa, 
	struct bdbm_phyaddr_t* ppa)
{
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);
	struct bdbm_block_mapping_entry* e = NULL;
	uint64_t segment_no;
	uint64_t block_no;
	uint64_t page_ofs;

	segment_no = __bdbm_block_ftl_get_segment_no (p, lpa);
	block_no = __bdbm_block_ftl_get_block_no (p, lpa);
	page_ofs = __bdbm_block_ftl_get_page_ofs (p, lpa);
	e = &p->ptr_mapping_table[segment_no][block_no];

	/* is it already mapped? */
	if (e->status == BFTL_ALLOCATED) {
		/* if so, see if the mapping information is valid or not */
		bdbm_bug_on (e->channel_no != ppa->channel_no);	
		bdbm_bug_on (e->chip_no != ppa->chip_no);
		bdbm_bug_on (e->block_no != ppa->block_no);

		/* update the offset of the recently written page */
		e->rw_pg_ofs = page_ofs;
	
		return 0;
	}

	/* mapping lpa to ppa */
	e->status = BFTL_ALLOCATED;
	e->channel_no = ppa->channel_no;
	e->chip_no = ppa->chip_no;
	e->block_no = ppa->block_no;
	e->rw_pg_ofs = page_ofs;

	return 0;
}

uint32_t bdbm_block_ftl_invalidate_lpa (
	struct bdbm_drv_info* bdi, 
	uint64_t lpa,
	uint64_t len)
{
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);
	struct bdbm_block_mapping_entry* e = NULL;
	uint64_t segment_no;
	uint64_t block_no;
	uint64_t page_ofs;

	segment_no = __bdbm_block_ftl_get_segment_no (p, lpa);
	block_no = __bdbm_block_ftl_get_block_no (p, lpa);
	page_ofs = __bdbm_block_ftl_get_page_ofs (p, lpa);
	e = &p->ptr_mapping_table[segment_no][block_no];

	/* ignore trim requests for a free segment */
	if (e->status == BFTL_NOT_ALLOCATED)
		return 0;

	/* ignore trim requests if it is already invalid */
	if (e->pst[page_ofs] == 1)
		return 0;

#ifdef ENABLE_LOG
	bdbm_msg ("T: [%llu] lpa: %llu (# of invalid pages: %llu)", segment_no, lpa, p->nr_trimmed_pages[segment_no]);
#endif

	/* mark a corresponding page an invalid status */
	e->pst[page_ofs] = 1; 
	p->nr_trimmed_pages[segment_no]++;

	if (p->nr_trimmed_pages[segment_no] == p->nr_pages_per_segment) {
		/* increate # of dead segments */
		__hlm_rsd_make_rm_seg (bdi, segment_no);
		p->nr_dead_segments++;
	}

	return 0;
}

uint8_t bdbm_block_ftl_is_gc_needed (
	struct bdbm_drv_info* bdi)
{
	/*
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);

	if (p->nr_dead_segments >= 1) {
		return 1;
	}

	return 0;
	*/

	return 0;
}

uint32_t __bdbm_block_ftl_erase_block (
	struct bdbm_drv_info* bdi,
	uint64_t seg_no)
{
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);
	struct bdbm_hlm_req_gc_t* hlm_gc = &p->gc_hlm;
	uint64_t i;

	bdbm_memset (p->gc_bab, 0x00, sizeof (struct bdbm_abm_block_t*) * p->nr_blocks_per_segment);

	/* setup a set of reqs */
	for (i = 0; i < p->nr_blocks_per_segment; i++) {
		struct bdbm_abm_block_t* b = NULL;
		struct bdbm_llm_req_t* r = NULL;

		if ((b = bdbm_abm_get_block (p->abm, 
				p->ptr_mapping_table[seg_no][i].channel_no, 
				p->ptr_mapping_table[seg_no][i].chip_no, 
				p->ptr_mapping_table[seg_no][i].block_no)) == NULL) {
			bdbm_error ("oops! bdbm_abm_get_block failed (%llu %llu %llu)", 
				p->ptr_mapping_table[seg_no][i].channel_no, 
				p->ptr_mapping_table[seg_no][i].chip_no, 
				p->ptr_mapping_table[seg_no][i].block_no);
			bdbm_bug_on (1);
		}
		p->gc_bab[i] = b;

		r = &hlm_gc->llm_reqs[i];
		r->req_type = REQTYPE_GC_ERASE;
		r->lpa = -1ULL; 
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
	hlm_gc->nr_reqs = p->nr_blocks_per_segment;
	bdbm_reinit_completion (hlm_gc->gc_done);
	for (i = 0; i < p->nr_blocks_per_segment; i++) {
		if ((bdi->ptr_llm_inf->make_req (bdi, &hlm_gc->llm_reqs[i])) != 0) {
			bdbm_error ("llm_make_req failed");
			bdbm_bug_on (1);
		}
	}
	bdbm_wait_for_completion (hlm_gc->gc_done);

	for (i = 0; i < p->nr_blocks_per_segment; i++) {
		uint8_t is_bad = 0;
		struct bdbm_abm_block_t* b = p->gc_bab[i];

		if (hlm_gc->llm_reqs[i].ret != 0)
			is_bad = 1; /* bad block */

		bdbm_abm_erase_block (p->abm, b->channel_no, b->chip_no, b->block_no, is_bad);
	}

	/* measure gc elapsed time */
	bdbm_complete (hlm_gc->gc_done);

	return 0;
}

uint32_t __bdbm_block_ftl_do_gc_segment (
	struct bdbm_drv_info* bdi,
	uint64_t seg_no)
{
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);
	struct bdbm_block_mapping_entry* e = NULL;
	uint64_t i;

	/* step 3: erase all the blocks that belong to the victim */
	if (__bdbm_block_ftl_erase_block (bdi, seg_no) != 0) {
		bdbm_error ("__bdbm_block_ftl_erase_block failed");
		return 1;
	}

	/* step 4: reset the victim segment */
	for (i = 0; i < p->nr_blocks_per_segment; i++) {
		e = &p->ptr_mapping_table[seg_no][i];

		if (e->channel_no == -1 || e->chip_no == -1 || e->block_no == -1) {
			bdbm_msg ("oops! %lld %lld %lld", e->channel_no, e->chip_no, e->block_no);
		}

		/* reset all the variables */
		e->status = BFTL_NOT_ALLOCATED;
		e->channel_no = -1;
		e->chip_no = -1;
		e->block_no = -1;
		e->rw_pg_ofs = -1;
		bdbm_memset (e->pst, 0x00, sizeof (uint8_t) * np->nr_pages_per_block);
	}
	if (p->nr_dead_segments <= 0) {
		bdbm_error ("oops! p->nr_dead_segments is %lld", p->nr_dead_segments);
		bdbm_bug_on (1);
	}
	p->nr_trimmed_pages[seg_no] = 0;
	p->nr_dead_segments--;

	__hlm_rsd_make_rm_seg (bdi, seg_no);

	return 0;
}

uint32_t bdbm_block_ftl_do_gc (
	struct bdbm_drv_info* bdi)
{
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);
	uint64_t seg_no = -1; /* victim */
	uint64_t i;
	uint32_t ret;

	/* step 1: do we really need gc now? */
	if (bdbm_block_ftl_is_gc_needed (bdi) == 0)
		return 1;

	/* step 2: select a victim segment 
	 * TODO: need to improve it with a dead block list */
	for (i = 0; i < p->nr_segments; i++) {
		if (p->nr_trimmed_pages[i] == p->nr_pages_per_segment) {
			seg_no = i;
			break;
		}
	}

	/* it would be possible that there is no victim segment */
	if (seg_no == -1)
		return 1;

	/* step 3: do gc for a victim segment */
	ret = __bdbm_block_ftl_do_gc_segment (bdi, seg_no);

	return ret;
}

uint64_t bdbm_block_ftl_get_segno (struct bdbm_drv_info* bdi, uint64_t lpa)
{
	return __bdbm_block_ftl_get_segment_no (BDBM_FTL_PRIV (bdi), lpa);
}


#if 0
struct bdbm_block_mapping_entry {
	uint8_t status;	/* BDBM_BFTL_BLOCK_STATUS */
	uint64_t channel_no;
	uint64_t chip_no;
	uint64_t block_no;
	uint64_t rw_pg_ofs; /* recently-written page offset */
	uint8_t* pst;	/* status of pages in a block */
};
#endif

uint32_t bdbm_block_ftl_load (struct bdbm_drv_info* bdi, const char* fn)
{
	return 0;
}

uint32_t bdbm_block_ftl_store (struct bdbm_drv_info* bdi, const char* fn)
{
	return 0;
}

void __bdbm_block_ftl_badblock_scan_eraseblks (struct bdbm_drv_info* bdi, uint64_t block_no)
{
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_hlm_req_gc_t* hlm_gc = &p->gc_hlm;
	uint64_t i, j;

	bdbm_memset (p->gc_bab, 0x00, sizeof (struct bdbm_abm_block_t*) * p->nr_blocks_per_segment);

	/* setup a set of reqs */
	for (i = 0; i < np->nr_channels; i++) {
		for (j = 0; j < np->nr_chips_per_channel; j++) {
			struct bdbm_abm_block_t* b = NULL;
			struct bdbm_llm_req_t* r = NULL;
			uint64_t punit_id = i*np->nr_chips_per_channel+j;

			if ((b = bdbm_abm_get_block (p->abm, i, j, block_no)) == NULL) {
				bdbm_error ("oops! bdbm_abm_get_block failed");
				bdbm_bug_on (1);
			}
			p->gc_bab[punit_id] = b;

			r = &hlm_gc->llm_reqs[punit_id];
			r->req_type = REQTYPE_GC_ERASE;
			r->lpa = -1ULL; /* lpa is not available now */
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
	hlm_gc->nr_reqs = p->nr_blocks_per_segment;
	bdbm_reinit_completion (hlm_gc->gc_done);
	for (i = 0; i < p->nr_blocks_per_segment; i++) {
		if ((bdi->ptr_llm_inf->make_req (bdi, &hlm_gc->llm_reqs[i])) != 0) {
			bdbm_error ("llm_make_req failed");
			bdbm_bug_on (1);
		}
	}
	bdbm_wait_for_completion (hlm_gc->gc_done);

	for (i = 0; i < p->nr_blocks_per_segment; i++) {
		uint8_t is_bad = 0;
		struct bdbm_abm_block_t* b = p->gc_bab[i];

		if (hlm_gc->llm_reqs[i].ret != 0)
			is_bad = 1; /* bad block */

		/*
		bdbm_msg ("erase: %llu %llu %llu (%llu)", 
			b->channel_no, 
			b->chip_no, 
			b->block_no, 
			is_bad);
		*/

		bdbm_abm_erase_block (p->abm, b->channel_no, b->chip_no, b->block_no, is_bad);
	}

	/* measure gc elapsed time */
	bdbm_complete (hlm_gc->gc_done);
}

uint32_t bdbm_block_ftl_badblock_scan (struct bdbm_drv_info* bdi)
{
	struct bdbm_block_ftl_private* p = BDBM_FTL_PRIV (bdi);
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_block_mapping_entry* me = NULL;
	uint64_t i = 0, j = 0;
	uint32_t ret = 0;

	bdbm_msg ("[WARNING] 'bdbm_block_ftl_badblock_scan' is called! All of the flash blocks will be erased!!!");

	/* step1: reset the page-level mapping table */
	bdbm_msg ("step1: reset the block-level mapping table");
	for (i = 0; i < p->nr_segments; i++) {
		me = p->ptr_mapping_table[i];
		for (j = 0; j < p->nr_blocks_per_segment; j++) {
			me[j].status = BFTL_NOT_ALLOCATED;
			me[j].channel_no = -1;
			me[j].chip_no = -1;
			me[j].block_no = -1;
			me[j].rw_pg_ofs = -1;
			bdbm_memset ((uint8_t*)me[j].pst, 0x00, sizeof (uint8_t) * np->nr_pages_per_block);
		}
		p->nr_trimmed_pages[i] = 0;
	}
	p->nr_dead_segments = 0;

	/* step2: erase all the blocks */
	bdbm_msg ("step2: erase all the blocks");
	bdi->ptr_llm_inf->flush (bdi);
	for (i = 0; i < np->nr_blocks_per_chip; i++) {
		__bdbm_block_ftl_badblock_scan_eraseblks (bdi, i);
	}

	/* step3: store abm */
	/*
	if ((ret = bdbm_abm_store (p->bai, "/usr/share/bdbm_drv/abm.dat"))) {
		bdbm_error ("bdbm_abm_store failed");
		return 1;
	}
	*/

	bdbm_msg ("done");
	 
	return ret;
}

