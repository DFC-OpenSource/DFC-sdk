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
#include <linux/blkdev.h>
*/
#include <pthread.h>
#include "debug.h"
#include "params.h"
#include "bdbm_drv.h"
#include "hlm_nobuf.h"

#include "ftl/no_ftl.h"
#include "ftl/block_ftl.h"
#include "ftl/page_ftl.h"
/*#include "utils/time.h" */

#if GC_THREAD_ENABLED
extern pthread_mutex_t gc_lock;
#endif
/* interface for hlm_nobuf */
struct bdbm_hlm_inf_t _hlm_nobuf_inf = {
	.ptr_private = NULL,
	.create = hlm_nobuf_create,
	.destroy = hlm_nobuf_destroy,
	.make_req = hlm_nobuf_make_req,
	.end_req = hlm_nobuf_end_req,
	/*.load = hlm_nobuf_load,*/
	/*.store = hlm_nobuf_store,*/
};

/* data structures for hlm_nobuf */
struct bdbm_hlm_nobuf_private {
	struct bdbm_ftl_inf_t* ptr_ftl_inf;
};

/* functions for hlm_nobuf */
uint32_t hlm_nobuf_create (struct bdbm_drv_info* bdi)
{
	struct bdbm_hlm_nobuf_private* p;

	/* create private */
	if ((p = (struct bdbm_hlm_nobuf_private*)bdbm_malloc_atomic
			(sizeof(struct bdbm_hlm_nobuf_private))) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		return 1;
	}

	/* setup FTL function pointers */
	if ((p->ptr_ftl_inf = BDBM_GET_FTL_INF (bdi)) == NULL) {
		bdbm_error ("ftl is not valid");
		return 1;
	}

	/* keep the private structure */
	bdi->ptr_hlm_inf->ptr_private = (void*)p;

	return 0;
}

void hlm_nobuf_destroy (struct bdbm_drv_info* bdi)
{
	struct bdbm_hlm_nobuf_private* p = (struct bdbm_hlm_nobuf_private*)BDBM_HLM_PRIV(bdi);

	/* free priv */
	bdbm_free_atomic (p);
}

uint32_t __hlm_nobuf_make_trim_req (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* ptr_hlm_req)
{
	struct bdbm_ftl_inf_t* ftl = (struct bdbm_ftl_inf_t*)BDBM_GET_FTL_INF(bdi);
	uint64_t i;
	unsigned long nr_secs_per_kp = 0;
	
	nr_secs_per_kp = KERNEL_PAGE_SIZE >> KERNEL_SECTOR_SZ_SHIFT;
	
	if (!ptr_hlm_req->is_seq_lpa) {
		for (i = 0; i < ptr_hlm_req->len; i++) {
			ftl->invalidate_lpa (bdi, ptr_hlm_req->lpa[i], 1);
		}
	} else {
		uint64_t lpa = ptr_hlm_req->lpa[0];
		lpa = lpa/nr_secs_per_kp;/*TODO to remove once after the code without cache is verfied*/
		for (i = 0; i < ptr_hlm_req->len; i++,lpa++) {
			ftl->invalidate_lpa (bdi, lpa, 1);
		}
	}
	return 0;
}

uint32_t __hlm_nobuf_get_req_type (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* ptr_hlm_req, uint32_t index)
{
	struct driver_params* dp = (struct driver_params*)BDBM_GET_DRIVER_PARAMS(bdi);
	struct nand_params* np = (struct nand_params*)BDBM_GET_NAND_PARAMS(bdi);
	uint32_t nr_kp_per_fp, req_type, j;

	nr_kp_per_fp = np->page_main_size >> KERNEL_PAGE_SHIFT;	/* e.g., 2 = 8 KB / 4 KB */

	/* temp */
	if (dp->mapping_policy == MAPPING_POLICY_SEGMENT)
		return ptr_hlm_req->req_type;
	/* end */

	req_type = ptr_hlm_req->req_type;
	if (ptr_hlm_req->req_type == REQTYPE_WRITE) {
		for (j = 0; j < nr_kp_per_fp; j++) {
			if (ptr_hlm_req->kpg_flags[index * nr_kp_per_fp + j] == MEMFLAG_FRAG_PAGE) {
				req_type = REQTYPE_RMW_READ;
				break;
			}
		}
	}

	return req_type;
}

uint32_t __hlm_nobuf_make_wr_req (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* ptr_hlm_req)
{
	struct bdbm_ftl_inf_t* ftl = (struct bdbm_ftl_inf_t*)BDBM_GET_FTL_INF(bdi);
	struct nand_params* np = (struct nand_params*)BDBM_GET_NAND_PARAMS(bdi);
	struct bdbm_llm_req_t** pptr_llm_req;
	uint32_t nr_kp_per_fp;
	uint32_t hlm_len, llm_cnt = 0;
	uint32_t ret = 0;
	uint32_t i,sub_page, offset;
	/*uint64_t *lpa;*/

	nr_kp_per_fp = np->page_main_size >> KERNEL_PAGE_SHIFT;	/* e.g., 2 = 8 KB / 4 KB */
	hlm_len = ptr_hlm_req->len;
	llm_cnt = hlm_len / nr_kp_per_fp;
	/*lpa = ptr_hlm_req->lpa;*/
	/* create a set of llm_req */
	if ((pptr_llm_req = (struct bdbm_llm_req_t**)bdbm_malloc_atomic
			(sizeof (struct bdbm_llm_req_t*) * llm_cnt)) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		return -1;
	}
	ptr_hlm_req->llm_set_ptr = pptr_llm_req;
	ptr_hlm_req->nr_llm_reqs = llm_cnt;
	/* divide a hlm_req into multiple llm_reqs */
	for (i = 0; i < llm_cnt; i++) {
		struct bdbm_llm_req_t* r = NULL;
		offset = i * nr_kp_per_fp;

		/* create a low-level request */
		if ((r = (struct bdbm_llm_req_t*)bdbm_malloc_atomic 
				(sizeof (struct bdbm_llm_req_t))) == NULL) {
			bdbm_error ("bdbm_malloc_atomic failed");
			goto fail;
		}
		pptr_llm_req[i] = r;
	
		/* setup llm_req */
		r->req_type = REQTYPE_WRITE;
		r->lpa = ptr_hlm_req->lpa + offset;  /* each request should be lpa size */
		r->kpg_flags = ptr_hlm_req->kpg_flags + offset;
		r->pptr_kpgs = ptr_hlm_req->pptr_kpgs + offset;
		r->ptr_hlm_req = (void*)ptr_hlm_req;

		r->phyaddr = &r->phyaddr_w;
		/* get the physical addr where new page will be written */

		if (ftl->get_free_ppa (bdi, r->lpa, r->phyaddr) != 0) {
			bdbm_error ("`ftl->get_free_ppa' failed");
			goto fail;
		}
		if (ftl->map_lpa_to_ppa (bdi, r->lpa, r->phyaddr) != 0) {
			bdbm_error ("`ftl->map_lpa_to_ppa' failed");
			goto fail;
		}
		
		if (np->page_oob_size > 0) {
			if ((r->ptr_oob = (uint8_t*)bdbm_malloc_atomic 
					(sizeof (uint8_t) * np->page_oob_size)) == NULL) {
				bdbm_error ("bdbm_malloc_atomic failed");
				goto fail;
			}
		} else {
			r->ptr_oob = NULL;
		}
		r->offset = NULL;
		/* keep some metadata in OOB if it is write (e.g., LPA) */
		if (r->ptr_oob) {
			for (sub_page = 0; sub_page < np->nr_subpages_per_page; sub_page++) {
				if (sub_page >= 4) { 
					((uint64_t*)(r->ptr_oob + (np->page_oob_size >> 1)))[sub_page & 3] = r->lpa[sub_page];
				} else {
					((uint64_t*)r->ptr_oob)[sub_page] = r->lpa[sub_page];
				}
			}
		}
	}

	if ((ret = bdi->ptr_dm_inf->make_req (bdi, pptr_llm_req)) != 0) {
		bdbm_error ("llm_make_req failed");
	}

	/* free pptr_llm_req */
	//bdbm_free_atomic (pptr_llm_req);

	return 0;

fail:
	printf ("fail\n");
	/* free llm_req */
	for (i = 0; i < llm_cnt; i++) {
		struct bdbm_llm_req_t* r = pptr_llm_req[i];
		if (r != NULL) {
			if (r->ptr_oob != NULL)
				bdbm_free_atomic (r->ptr_oob);
			bdbm_free_atomic (r);
		}
	}
	bdbm_free_atomic (pptr_llm_req);

	return 1;
}

uint32_t __hlm_nobuf_make_rd_req (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* ptr_hlm_req)
{
	struct bdbm_ftl_inf_t* ftl = (struct bdbm_ftl_inf_t*)BDBM_GET_FTL_INF(bdi);
	struct nand_params* np = (struct nand_params*)BDBM_GET_NAND_PARAMS(bdi);
	struct bdbm_llm_req_t** pptr_llm_req;
	struct bdbm_phyaddr_t curr_phy, prev_phy; 
	uint32_t nr_kp_per_fp;
	uint32_t hlm_len;
	uint32_t ret = 0;
	uint32_t i;
	uint32_t req_type; 
	uint8_t offset = 0, prev_off = 0, chk_PrevPhy = 0, j=0, k=0;

	nr_kp_per_fp = np->page_main_size >> KERNEL_PAGE_SHIFT;	/* e.g., 2 = 8 KB / 4 KB */
	hlm_len = ptr_hlm_req->len;

	/* create a set of llm_req */
	if ((pptr_llm_req = (struct bdbm_llm_req_t**)bdbm_malloc_atomic
			(sizeof (struct bdbm_llm_req_t*) * ptr_hlm_req->len)) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		return -1;
	}
	ptr_hlm_req->llm_set_ptr = pptr_llm_req;
	ptr_hlm_req->nr_llm_reqs = hlm_len;
	/* divide a hlm_req into multiple llm_reqs */
	for (i = 0; i < hlm_len; i++) {
		struct bdbm_llm_req_t* r;

		if (ftl->get_ppa (bdi, ptr_hlm_req->lpa[i], &curr_phy, &offset) != 0) {
			req_type = REQTYPE_READ_DUMMY; /* reads for unwritten pages */
			chk_PrevPhy = 0;
		} else if (chk_PrevPhy) {
			if ((curr_phy.channel_no == prev_phy.channel_no) \
					&& (curr_phy.chip_no == prev_phy.chip_no) \
					&& (curr_phy.block_no == prev_phy.block_no) \
					&& (curr_phy.page_no == prev_phy.page_no) \
					&& (offset == prev_off + 1)) {
				ptr_hlm_req->nr_llm_reqs--;
				j++;
				r->offset[j] = offset;
				prev_off = offset;
				r->nr_subpgs++;
				continue;
			}
		} else {
			chk_PrevPhy = 1;
			req_type = REQTYPE_READ;
		}
		j=0;
		prev_phy = curr_phy;
		prev_off = offset;
		/* create a low-level request */
		r = NULL;
		if ((r = (struct bdbm_llm_req_t*)bdbm_malloc_atomic 
				(sizeof (struct bdbm_llm_req_t))) == NULL) {
			bdbm_error ("bdbm_malloc_atomic failed");
			goto fail;
		}
		pptr_llm_req[k] = r;
	
		/* setup llm_req */
		r->req_type = req_type;
		r->lpa = ptr_hlm_req->lpa + i;  /* each request should be lpa size */

		/* get the physical addr for reads */
		r->phyaddr = &r->phyaddr_r;
		r->phyaddr_r = curr_phy;
		r->offset = bdbm_malloc (nr_kp_per_fp * sizeof(uint8_t));
		bdbm_memset (r->offset, -1U, nr_kp_per_fp * sizeof(uint8_t));
		r->offset[j] = offset;
		r->nr_subpgs = 1;

		r->kpg_flags = ptr_hlm_req->kpg_flags + i;
		r->pptr_kpgs = ptr_hlm_req->pptr_kpgs + i;
		r->ptr_hlm_req = (void*)ptr_hlm_req;
		k++;
	}
	if ((ret = bdi->ptr_dm_inf->make_req (bdi, pptr_llm_req)) != 0) {
		bdbm_error ("llm_make_req failed");
	}

	/* free pptr_llm_req */
	//bdbm_free_atomic (pptr_llm_req);

	return 0;

fail:
	printf ("fail\n");
	/* free llm_req */
	for (i = 0; i < ptr_hlm_req->len; i++) {
		struct bdbm_llm_req_t* r = pptr_llm_req[i];
		if (r != NULL) {
			if (r->ptr_oob != NULL)
				bdbm_free_atomic (r->ptr_oob);
			bdbm_free_atomic (r);
		}
	}
	bdbm_free_atomic (pptr_llm_req);

	return 1;
}

#if 0
uint32_t __hlm_nobuf_make_rw_req (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* ptr_hlm_req)
{
	struct bdbm_ftl_inf_t* ftl = (struct bdbm_ftl_inf_t*)BDBM_GET_FTL_INF(bdi);
	struct nand_params* np = (struct nand_params*)BDBM_GET_NAND_PARAMS(bdi);
	struct bdbm_llm_req_t** pptr_llm_req;
	uint32_t nr_kp_per_fp;
	uint32_t hlm_len;
	uint32_t ret = 0;
	uint32_t i;

	nr_kp_per_fp = np->page_main_size >> KERNEL_PAGE_SHIFT;	/* e.g., 2 = 8 KB / 4 KB */
	hlm_len = ptr_hlm_req->len;

	/* create a set of llm_req */
	if ((pptr_llm_req = (struct bdbm_llm_req_t**)bdbm_malloc_atomic
			(sizeof (struct bdbm_llm_req_t*) * ptr_hlm_req->len)) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		return -1;
	}
	ptr_hlm_req->llm_set_ptr = pptr_llm_req;
	/* divide a hlm_req into multiple llm_reqs */
	for (i = 0; i < hlm_len; i++) {
		struct bdbm_llm_req_t* r = NULL;

		/* create a low-level request */
		if ((r = (struct bdbm_llm_req_t*)bdbm_malloc_atomic 
				(sizeof (struct bdbm_llm_req_t))) == NULL) {
			bdbm_error ("bdbm_malloc_atomic failed");
			goto fail;
		}
		pptr_llm_req[i] = r;
	
		/* setup llm_req */
		r->req_type = __hlm_nobuf_get_req_type (bdi, ptr_hlm_req, i);
		r->lpa = ptr_hlm_req->lpa + i;  /* each request should be lpa size */
		switch (r->req_type) {
		case REQTYPE_READ:
			/* get the physical addr for reads */
			r->phyaddr = &r->phyaddr_r;
			if (ftl->get_ppa (bdi, r->lpa, r->phyaddr) != 0)
				r->req_type = REQTYPE_READ_DUMMY; /* reads for unwritten pages */
			break;
		case REQTYPE_RMW_READ:
			/* get the physical addr for original pages */
			r->phyaddr = &r->phyaddr_r;
			if (ftl->get_ppa (bdi, r->lpa, r->phyaddr) != 0)
				r->req_type = REQTYPE_WRITE;
			/* go ahead! */
		case REQTYPE_WRITE:
			r->phyaddr = &r->phyaddr_w;
			/* get the physical addr where new page will be written */

			if (ftl->get_free_ppa (bdi, r->lpa, r->phyaddr) != 0) {
				bdbm_error ("`ftl->get_free_ppa' failed");
				goto fail;
			}
			if (ftl->map_lpa_to_ppa (bdi, r->lpa, r->phyaddr) != 0) {
				bdbm_error ("`ftl->map_lpa_to_ppa' failed");
				goto fail;
			}
			break;
		default:
			bdbm_error ("invalid request type (%u)", r->req_type);
			bdbm_bug_on (1);
			break;

		}

		if (r->req_type == REQTYPE_RMW_READ)
			r->phyaddr = &r->phyaddr_r;
		r->kpg_flags = ptr_hlm_req->kpg_flags + (i * nr_kp_per_fp);
		r->pptr_kpgs = ptr_hlm_req->pptr_kpgs + (i * nr_kp_per_fp);
		r->ptr_hlm_req = (void*)ptr_hlm_req;
		if (np->page_oob_size > 0) {
			if ((r->ptr_oob = (uint8_t*)bdbm_malloc_atomic 
					(sizeof (uint8_t) * np->page_oob_size)) == NULL) {
				bdbm_error ("bdbm_malloc_atomic failed");
				goto fail;
			}
		} else
			r->ptr_oob = NULL;

		/* keep some metadata in OOB if it is write (e.g., LPA) */
		if ((r->req_type == REQTYPE_WRITE || r->req_type == REQTYPE_RMW_READ)) {
			if (r->ptr_oob)
				((uint64_t*)r->ptr_oob)[0] = r->lpa;
		}
		r->offset = ((i!=0) ? 0 : ptr_hlm_req->offset);
		/* set elapsed time */
	}

	/* TODO: we assume that 'ptr_llm_inf->make_req' always returns success.
	 * It must be improved to handle the following two cases later
	 * (1) when some of llm_reqs fail
	 * (2) when all of llm_reqs fail */
#if 0
	for (i = 0; i < hlm_len; i++) {
		if ((ret = bdi->ptr_llm_inf->make_req (bdi, pptr_llm_req[i])) != 0) {
			bdbm_error ("llm_make_req failed");
		}
	}
#endif
	bdi->ptr_llm_inf->make_req (bdi, pptr_llm_req);

	/* free pptr_llm_req */
	//bdbm_free_atomic (pptr_llm_req);

	return 0;

fail:
	printf ("fail\n");
	/* free llm_req */
	for (i = 0; i < ptr_hlm_req->len; i++) {
		struct bdbm_llm_req_t* r = pptr_llm_req[i];
		if (r != NULL) {
			if (r->ptr_oob != NULL)
				bdbm_free_atomic (r->ptr_oob);
			bdbm_free_atomic (r);
		}
	}
	bdbm_free_atomic (pptr_llm_req);

	return 1;
}
#endif
uint32_t hlm_nobuf_make_req (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* ptr_hlm_req)
{
	struct bdbm_ftl_inf_t* ftl = (struct bdbm_ftl_inf_t*)BDBM_GET_FTL_INF(bdi);
	uint32_t ret;

	/* see if gc is needed or not */
	/* TODO: lock */
	if (ptr_hlm_req->req_type == REQTYPE_WRITE && 
		ftl->is_gc_needed != NULL && 
		ftl->is_gc_needed (bdi)) {
		ftl->do_gc (bdi);
#if GC_THREAD_ENABLED
		while (pthread_mutex_trylock(&gc_lock)==0) {
			pthread_mutex_unlock(&gc_lock);
		}
#endif
	}
#if GC_THREAD_ENABLED
	pthread_mutex_lock(&gc_lock);
	pthread_mutex_unlock(&gc_lock);
#endif
	/* TODO: release */

	switch (ptr_hlm_req->req_type) {
	case REQTYPE_TRIM:
		if ((ret = __hlm_nobuf_make_trim_req (bdi, ptr_hlm_req)) == 0) {
			/* call 'ptr_host_inf->end_req' directly */
			bdi->ptr_host_inf->end_req (bdi, ptr_hlm_req);
			/* ptr_hlm_req is now NULL */
		}
		break;
	case REQTYPE_READ:
		ret = __hlm_nobuf_make_rd_req (bdi, ptr_hlm_req);
		break;
	case REQTYPE_WRITE:
		ret = __hlm_nobuf_make_wr_req (bdi, ptr_hlm_req);
		break;
	default:
		bdbm_error ("invalid REQTYPE (%u)", ptr_hlm_req->req_type);
		bdbm_bug_on (1);
		ret = 1;
		break;
	}

	return ret;
}

void __hlm_nobuf_end_host_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* ptr_llm_req)
{
	struct bdbm_hlm_req_t* ptr_hlm_req = (struct bdbm_hlm_req_t* )ptr_llm_req->ptr_hlm_req;

	/* see if 'ptr_hlm_req' is NULL or not */
	if (ptr_hlm_req == NULL) {
		bdbm_error ("ptr_hlm_req is NULL");
		return;
	}

	/* see if llm's lpa is correct or not */
#if 0
	if (ptr_hlm_req->lpa > ptr_llm_req->lpa || 
		ptr_hlm_req->lpa + ptr_hlm_req->len <= ptr_llm_req->lpa) {
		bdbm_error ("hlm_req->lpa: %lu-%lu, llm_req->lpa: %lu", 
			ptr_hlm_req->lpa, 
			ptr_hlm_req->lpa + ptr_hlm_req->len, 
			ptr_llm_req->lpa);
		return;
	}
#endif
	/* change flags of hlm */
	/*bdbm_spin_lock (&ptr_hlm_req->lock);*/
	if (ptr_llm_req->kpg_flags != NULL) {
		struct nand_params* np;
		uint32_t nr_sub_reqs, ofs, loop;

		np = &bdi->ptr_bdbm_params->nand;
		nr_sub_reqs = (ptr_llm_req->req_type == REQTYPE_WRITE) ? np->nr_subpages_per_page : ptr_llm_req->nr_subpgs;

		for (loop = 0; loop < nr_sub_reqs; loop++) {
			/* change the status of kernel pages */
			if (ptr_llm_req->kpg_flags[loop] != MEMFLAG_FRAG_PAGE &&
				ptr_llm_req->kpg_flags[loop] != MEMFLAG_KMAP_PAGE &&
				ptr_llm_req->kpg_flags[loop] != MEMFLAG_FRAG_PAGE_DONE &&
				ptr_llm_req->kpg_flags[loop] != MEMFLAG_KMAP_PAGE_DONE) {
				bdbm_error ("kpg_flags is not valid");
				/*bdbm_spin_unlock (&ptr_hlm_req->lock);*/
				/*return;*/
			}
			ptr_llm_req->kpg_flags[loop] |= MEMFLAG_DONE;
		}
	}
	ptr_hlm_req->ret |= ptr_llm_req->ret;
	/*bdbm_spin_unlock (&ptr_hlm_req->lock);*/

	/* free oob space & ptr_llm_req */
	if (ptr_llm_req->ptr_oob != NULL) {
		/* LPA stored on OOB must be the same as  */
		/*
		if (ptr_llm_req->req_type == REQTYPE_READ) {
			uint64_t lpa = ((uint64_t*)ptr_llm_req->ptr_oob)[0];
			if (lpa != ptr_llm_req->lpa) {
				bdbm_warning ("%lu != %lu (%llX)", ptr_llm_req->lpa, lpa, lpa);
			}	
		}
		*/
		bdbm_free_atomic (ptr_llm_req->ptr_oob);
	}
	if (ptr_llm_req->offset != NULL) {
		bdbm_free (ptr_llm_req->offset);
	}
	bdbm_free_atomic (ptr_llm_req);

	/* increase # of reqs finished */
	/*bdbm_spin_lock (&ptr_hlm_req->lock);*/
	ptr_hlm_req->nr_done_reqs++; 
	/*bdbm_spin_unlock (&ptr_hlm_req->lock);*/
	if (ptr_hlm_req->nr_done_reqs == ptr_hlm_req->nr_llm_reqs) {
		/* finish the host request */
		bdbm_free_atomic (ptr_hlm_req->llm_set_ptr);
		bdi->ptr_host_inf->end_req (bdi, ptr_hlm_req);
	}
}

void __hlm_nobuf_end_gc_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* llm_req)
{
	struct bdbm_hlm_req_gc_t* hlm_req = (struct bdbm_hlm_req_gc_t* )llm_req->ptr_hlm_req;

	hlm_req->nr_done_reqs++;
	if (hlm_req->nr_reqs == hlm_req->nr_done_reqs) {
		/*bdbm_msg ("gc_req done (%lu)", hlm_req->nr_done_reqs);*/
		sem_post (&hlm_req->gc_done);
/*		bdbm_complete (hlm_req->gc_done); */
	}
}

void hlm_nobuf_end_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* llm_req)
{
	switch (llm_req->req_type) {
	case REQTYPE_READ:
	case REQTYPE_READ_DUMMY:
	case REQTYPE_WRITE:
	case REQTYPE_RMW_READ:
	case REQTYPE_RMW_WRITE:
		__hlm_nobuf_end_host_req (bdi, llm_req);
		break;
	case REQTYPE_GC_ERASE:
	case REQTYPE_GC_READ:
	case REQTYPE_GC_WRITE:
		__hlm_nobuf_end_gc_req (bdi, llm_req);
		break;
	case REQTYPE_TRIM:
	default:
		bdbm_error ("hlm_nobuf_end_req got an invalid llm_req%p req_typ:%u lpa:%u blkno: %d", llm_req, llm_req->req_type, llm_req->lpa, llm_req->phyaddr->block_no);
		bdbm_bug_on (1);
		break;
	}
}

