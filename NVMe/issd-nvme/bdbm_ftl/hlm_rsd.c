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
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched.h>

#include "debug.h"
#include "params.h"
#include "bdbm_drv.h"
#include "hlm_nobuf.h"
#include "hlm_rsd.h"

#include "ftl/no_ftl.h"
#include "ftl/block_ftl.h"
#include "ftl/page_ftl.h"
#include "queue/queue.h"
#include "3rd/uthash.h"


/* interface for hlm_rsd */
struct bdbm_hlm_inf_t _hlm_rsd_inf = {
	.ptr_private = NULL,
	.create = hlm_rsd_create,
	.destroy = hlm_rsd_destroy,
	.make_req = hlm_rsd_make_req,
	.end_req = hlm_rsd_end_req,
};

struct segment_buf {
	uint32_t seg_no;
	struct bdbm_hlm_req_t* hlm_req;
	UT_hash_handle hh;	/* hash header */
};

/* data structures for hlm_rsd */
struct bdbm_hlm_rsd_private {
	struct bdbm_ftl_inf_t* ptr_ftl_inf;	/* for hlm_nobuff (it must be on top of this structure) */

	/* buffers for segments; 
	 * NOTE: the contents of buffers must be materialized to NAND flash
	 * when a flush command arrives.
	 */
	struct segment_buf* seg_buf;
};

/* interface functions for hlm_rsd */
uint32_t hlm_rsd_create (struct bdbm_drv_info* bdi)
{
	struct bdbm_hlm_rsd_private* p = NULL;
	struct driver_params* parms = BDBM_GET_DRIVER_PARAMS(bdi);

	/* create private */
	if ((p = (struct bdbm_hlm_rsd_private*)bdbm_malloc_atomic
			(sizeof(struct bdbm_hlm_rsd_private))) == NULL) {
		bdbm_error ("bdbm_malloc failed");
		return 1;
	}

	/* setup FTL function pointers */
	if ((p->ptr_ftl_inf = BDBM_GET_FTL_INF (bdi)) == NULL) {
		bdbm_error ("ftl is not valid");
		return 1;
	}
	if (parms->mapping_type != MAPPING_POLICY_SEGMENT) {
		bdbm_warning ("ftl is not for RSD!!!");
	}

	/* keep the private structure */
	bdi->ptr_hlm_inf->ptr_private = (void*)p;

	return 0;
}

void hlm_rsd_destroy (struct bdbm_drv_info* bdi)
{
	struct bdbm_hlm_rsd_private* p = (struct bdbm_hlm_rsd_private*)BDBM_HLM_PRIV(bdi);

	bdbm_free_atomic (p);
}

struct bdbm_hlm_req_t* __hlm_rsd_duplicate_hlm_req (
	struct bdbm_drv_info* bdi, 
	struct bdbm_hlm_req_t* hlm_req)
{
	struct bdbm_hlm_req_t* new_hlm_req = NULL;
	struct nand_params* np = &bdi->ptr_bdbm_params->nand;
	uint32_t nr_kp_per_fp = np->page_main_size / KERNEL_PAGE_SIZE;
	uint32_t i = 0;

	if (hlm_req->req_type != REQTYPE_WRITE) {
		bdbm_bug_on (1);
	}

	if ((new_hlm_req = (struct bdbm_hlm_req_t*)bdbm_malloc_atomic
			(sizeof (struct bdbm_hlm_req_t))) == NULL) {
		bdbm_bug_on (1);
	}

	new_hlm_req->req_type = hlm_req->req_type;
	new_hlm_req->lpa = hlm_req->lpa;
	new_hlm_req->len = hlm_req->len;
	new_hlm_req->nr_done_reqs = 0;
	new_hlm_req->ptr_host_req = NULL;
	new_hlm_req->ret = 0;
	new_hlm_req->queued = 1;	/* mark it queued */
	new_hlm_req->sw = hlm_req->sw;

	if ((new_hlm_req->pptr_kpgs = (uint8_t**)bdbm_malloc_atomic
			(sizeof(uint8_t*) * new_hlm_req->len * nr_kp_per_fp)) == NULL) {
		bdbm_bug_on (1);
	}
	if ((new_hlm_req->kpg_flags = (uint8_t*)bdbm_malloc_atomic
			(sizeof(uint8_t) * new_hlm_req->len * nr_kp_per_fp)) == NULL) {
		bdbm_bug_on (1);
	}
	for (i = 0; i < new_hlm_req->len * nr_kp_per_fp; i++) {
		new_hlm_req->kpg_flags[i] = hlm_req->kpg_flags[i];
		if ((new_hlm_req->pptr_kpgs[i] = (uint8_t*)bdbm_malloc_atomic (KERNEL_PAGE_SIZE)) == NULL) {
			bdbm_bug_on (1);
		}
		bdbm_memcpy (
			new_hlm_req->pptr_kpgs[i], 
			hlm_req->pptr_kpgs[i], 
			KERNEL_PAGE_SIZE);
	}

	return new_hlm_req;
}

void __hlm_rsd_delete_hlm_req (
	struct bdbm_drv_info* bdi, 
	struct bdbm_hlm_req_t* hlm_req)
{
	struct nand_params* np = &bdi->ptr_bdbm_params->nand;
	uint32_t nr_kp_per_fp = np->page_main_size / KERNEL_PAGE_SIZE;
	uint32_t i = 0;

	for (i = 0; i < hlm_req->len * nr_kp_per_fp; i++) {
		bdbm_free_atomic (hlm_req->pptr_kpgs[i]);
	}
	bdbm_free_atomic (hlm_req->pptr_kpgs);
	bdbm_free_atomic (hlm_req->kpg_flags);
	bdbm_free_atomic (hlm_req);
}

uint32_t __hlm_rsd_make_rm_seg (
	struct bdbm_drv_info* bdi, 
	uint32_t seg_no)
{
	struct bdbm_hlm_rsd_private* p = (struct bdbm_hlm_rsd_private*)BDBM_HLM_PRIV(bdi);
	struct segment_buf* b = NULL;

	HASH_FIND_INT (p->seg_buf, &seg_no, b);
	if (b != NULL) {
		HASH_DEL (p->seg_buf, b);
		__hlm_rsd_delete_hlm_req (bdi, b->hlm_req);
		bdbm_free_atomic (b);
	}

	return 0;
}

uint32_t __hlm_rsd_make_req_r (
	struct bdbm_drv_info* bdi, 
	struct bdbm_hlm_req_t* new_hlm_req)
{
	struct bdbm_hlm_rsd_private* p = (struct bdbm_hlm_rsd_private*)BDBM_HLM_PRIV(bdi);
	struct bdbm_ftl_inf_t* ftl = (struct bdbm_ftl_inf_t*)BDBM_GET_FTL_INF(bdi);
	struct nand_params* np = (struct nand_params*)BDBM_GET_NAND_PARAMS(bdi);
	struct bdbm_hlm_req_t* old_hlm_req = NULL;
	struct segment_buf* b = NULL;
	uint64_t nr_kp_per_fp = np->page_main_size / KERNEL_PAGE_SIZE;
	uint32_t seg_no;

	/* if get_segno is not available, handle reqs normally */
	if (ftl->get_segno == NULL) {
		goto done;
	}

	/* [step1] is there a req in seg buff? */
	seg_no = ftl->get_segno (bdi, new_hlm_req->lpa);

	HASH_FIND_INT (p->seg_buf, &seg_no, b);

	if (b != NULL) {
		uint64_t lpa_new, lpa_old, new_ofs, old_ofs, i;

		/* see if 'old_hlm_req' is valid or not */
		if (b->hlm_req == NULL) {
			bdbm_warning ("old_hlm_req is NULL");
			bdbm_bug_on (1);
		}
		if (b->seg_no != seg_no) {
			bdbm_warning ("seg_no is different (%u %u)", b->seg_no, seg_no);
			bdbm_bug_on (1);
		}
		old_hlm_req = b->hlm_req;

		if ((new_hlm_req->lpa + new_hlm_req->len - 1) < old_hlm_req->lpa) {
			goto done;
		}

		if (new_hlm_req->lpa > (old_hlm_req->lpa + old_hlm_req->len - 1)) {
			goto done;
		}

		/* copy the contents of the buffer to new hlm */
		for (lpa_new = new_hlm_req->lpa; lpa_new < new_hlm_req->lpa + new_hlm_req->len; lpa_new++) {
			for (lpa_old = old_hlm_req->lpa; lpa_old < old_hlm_req->lpa + old_hlm_req->len; lpa_old++) {
				if (lpa_new != lpa_old)
					continue;

#if 0
				bdbm_msg ("\t[%u] CACHE-READ-HIT: %llu(%llu)", seg_no, new_hlm_req->lpa, new_hlm_req->len);
#endif

				new_ofs = (lpa_new - new_hlm_req->lpa) * nr_kp_per_fp;
				old_ofs = (lpa_old - old_hlm_req->lpa) * nr_kp_per_fp;

				for (i = 0; i < nr_kp_per_fp; i++) {
					if (old_hlm_req->kpg_flags[old_ofs + i] == MEMFLAG_KMAP_PAGE) {
						bdbm_memcpy (
							new_hlm_req->pptr_kpgs[new_ofs + i],
							old_hlm_req->pptr_kpgs[old_ofs + i],
							KERNEL_PAGE_SIZE);

						new_hlm_req->kpg_flags[new_ofs + i] |= MEMFLAG_DONE;
					}
				}

#if 0
				for (i = 0; i < nr_kp_per_fp; i++) {
					if (old_hlm_req->kpg_flags[(lpa_old - old_hlm_req->lpa) * nr_kp_per_fp + i] == MEMFLAG_KMAP_PAGE) {
						bdbm_memcpy (
							new_hlm_req->pptr_kpgs[(lpa_new - new_hlm_req->lpa) * nr_kp_per_fp + i],
							old_hlm_req->pptr_kpgs[(lpa_old - old_hlm_req->lpa) * nr_kp_per_fp + i],
							KERNEL_PAGE_SIZE);

						new_hlm_req->kpg_flags[(lpa_new - new_hlm_req->lpa) * nr_kp_per_fp + i] |= MEMFLAG_DONE;
					}
				}
#endif
			}
		}
	}

done:
	return hlm_nobuf_make_req (bdi, new_hlm_req);
}

uint32_t __hlm_rsd_make_req_w (
	struct bdbm_drv_info* bdi, 
	struct bdbm_hlm_req_t* new_hlm_req)
{
	struct bdbm_hlm_rsd_private* p = (struct bdbm_hlm_rsd_private*)BDBM_HLM_PRIV(bdi);
	struct bdbm_ftl_inf_t* ftl = (struct bdbm_ftl_inf_t*)BDBM_GET_FTL_INF(bdi);
	struct nand_params* np = (struct nand_params*)BDBM_GET_NAND_PARAMS(bdi);
	struct bdbm_hlm_req_t* old_hlm_req = NULL;
	struct segment_buf* b = NULL;
	uint32_t ret = 0;

	uint64_t nr_kp_per_fp = np->page_main_size / KERNEL_PAGE_SIZE;
	uint32_t seg_no;

	/* if get_segno is not available, handle reqs normally */
	if (ftl->get_segno == NULL) {
		ret = hlm_nobuf_make_req (bdi, new_hlm_req);
		goto done;
	}

	/* [step1] is there a req in seg buff? */
	seg_no = ftl->get_segno (bdi, new_hlm_req->lpa);

	HASH_FIND_INT (p->seg_buf, &seg_no, b);

	if (b != NULL) {
		/* see if 'old_hlm_req' is valid or not */
		if (b->hlm_req == NULL) {
			bdbm_warning ("old_hlm_req is NULL");
			bdbm_bug_on (1);
		}
		if (b->seg_no != seg_no) {
			bdbm_warning ("seg_no is different (%u %u)", b->seg_no, seg_no);
			bdbm_bug_on (1);
		}
		old_hlm_req = b->hlm_req;

		/* see if new and old ones can be merged or not */
		if ((old_hlm_req->lpa + old_hlm_req->len - 1) == new_hlm_req->lpa) {
			uint32_t i = 0;
			uint32_t ofs = (old_hlm_req->len - 1) * nr_kp_per_fp;

#if 0
			bdbm_msg ("\t[%u] CACHE-WRITE-HIT: %llu(%llu)", seg_no, new_hlm_req->lpa, new_hlm_req->len);
#endif

			/* merge old and new requests */
			for (i = 0; i < nr_kp_per_fp; i++) {
				if (old_hlm_req->kpg_flags[ofs+i] == MEMFLAG_FRAG_PAGE &&
					new_hlm_req->kpg_flags[i] == MEMFLAG_KMAP_PAGE) {
					bdbm_memcpy (
						old_hlm_req->pptr_kpgs[ofs+i],
						new_hlm_req->pptr_kpgs[i],
						KERNEL_PAGE_SIZE);

#if 0
					bdbm_msg ("\t\tmemcpy: new (%u) => old (%u)", ofs+i, i);
#endif
				}
				new_hlm_req->kpg_flags[i] |= MEMFLAG_DONE;
			}

			/* finish the new req if its length is 1 */
			if (new_hlm_req->len == 1) {
#if 0
				bdbm_msg ("\t[%u] DIRECT-WRITE-IGNORE: %llu(%llu)", seg_no, new_hlm_req->lpa, new_hlm_req->len);
#endif
				bdi->ptr_host_inf->end_req (bdi, new_hlm_req);
				new_hlm_req = NULL;
			} else {
				/* otherwise, reduce its length by 1 */
				new_hlm_req->lpa++;
				new_hlm_req->len--;

				new_hlm_req->org_kpg_flags = new_hlm_req->kpg_flags;
				new_hlm_req->org_pptr_kpgs = new_hlm_req->pptr_kpgs;

				new_hlm_req->pptr_kpgs+=nr_kp_per_fp;
				new_hlm_req->kpg_flags+=nr_kp_per_fp;
			}
		} else {
			/* This is not a common case, but this is not error as well.
			 * For RISA, we submit it before writing new req */
		}

		/* remove it from the hash table */
		HASH_DEL (p->seg_buf, b);
		bdbm_free_atomic (b);
	}

	/* [step2] submit a prev req if it is available */
	if (old_hlm_req) {
#if 0
		bdbm_msg ("\t[%u] CACHE-WRITE: %llu(%llu) [%x %x %x ... %x]", 
			seg_no,
			old_hlm_req->lpa, 
			old_hlm_req->len,
			old_hlm_req->pptr_kpgs[1][0],
			old_hlm_req->pptr_kpgs[1][1],
			old_hlm_req->pptr_kpgs[1][2],
			old_hlm_req->pptr_kpgs[1][511]);
#endif

		if ((ret = hlm_nobuf_make_req (bdi, old_hlm_req)) != 0) {
			bdbm_error ("hlm_nobuf_make_req failed");
			__hlm_rsd_delete_hlm_req (bdi, old_hlm_req);
			goto done;
		}
	}

	/* [step3] submit or keep the req */
	if (new_hlm_req) {
		if (new_hlm_req->kpg_flags[new_hlm_req->len * nr_kp_per_fp - 1] == MEMFLAG_FRAG_PAGE) {
			uint32_t i = 0;
#if 0
			bdbm_msg ("\t[%u] CACHE: %llu(%llu) [%x %x %x ... %x]", 
				seg_no,
				new_hlm_req->lpa, 
				new_hlm_req->len, 
				new_hlm_req->pptr_kpgs[1][0],
				new_hlm_req->pptr_kpgs[1][1],
				new_hlm_req->pptr_kpgs[1][2],
				new_hlm_req->pptr_kpgs[1][511]);
#endif
			/* put the req into the buffer */
			if ((b = (struct segment_buf*)bdbm_malloc_atomic
					(sizeof (struct segment_buf))) == NULL) {
				bdbm_bug_on (1);
			}
			b->seg_no = seg_no;
			b->hlm_req = __hlm_rsd_duplicate_hlm_req (bdi, new_hlm_req);
			HASH_ADD_INT (p->seg_buf, seg_no, b);

			/* finish the req */
			for (i = 0; i < new_hlm_req->len * nr_kp_per_fp; i++)
				new_hlm_req->kpg_flags[i] |= MEMFLAG_DONE;
			bdi->ptr_host_inf->end_req (bdi, new_hlm_req);
		} else {
			/* submit the req to NAND flash */
			ret = hlm_nobuf_make_req (bdi, new_hlm_req);
		}
	}

done:
	return ret;
}

uint32_t hlm_rsd_make_req (
	struct bdbm_drv_info* bdi, 
	struct bdbm_hlm_req_t* hlm_req)
{
	uint32_t ret = 1;

	switch (hlm_req->req_type) {
	case REQTYPE_READ:
		ret = __hlm_rsd_make_req_r (bdi, hlm_req);
		break;
	case REQTYPE_WRITE:
		ret = __hlm_rsd_make_req_w (bdi, hlm_req);
		break;
	case REQTYPE_TRIM:
		ret = hlm_nobuf_make_req (bdi, hlm_req);
		break;
	default:
		bdbm_warning ("unknown req_type (%u)", hlm_req->req_type);
		break;
	}

	return ret;
}

void __hlm_rsd_end_req (
	struct bdbm_drv_info* bdi, 
	struct bdbm_llm_req_t* ptr_llm_req)
{
	struct bdbm_hlm_req_t* ptr_hlm_req = (struct bdbm_hlm_req_t* )ptr_llm_req->ptr_hlm_req;

	/* free oob space & ptr_llm_req */
	if (ptr_llm_req->ptr_oob != NULL) {
#if 0
		/* LPA stored on OOB must be the same as  */
		if (ptr_llm_req->req_type == REQTYPE_READ) {
			uint64_t lpa = ((uint64_t*)ptr_llm_req->ptr_oob)[0];
			if (lpa != ptr_llm_req->lpa) {
				bdbm_warning ("%llu != %llu (%llX)", ptr_llm_req->lpa, lpa, lpa);
			}	
		}
#endif
		bdbm_free_atomic (ptr_llm_req->ptr_oob);
	}
	bdbm_free_atomic (ptr_llm_req);

	/* increase # of reqs finished */
	ptr_hlm_req->nr_done_reqs++; 
	if (ptr_hlm_req->nr_done_reqs == ptr_hlm_req->len) {
		__hlm_rsd_delete_hlm_req (bdi, ptr_hlm_req);
	}
}

void hlm_rsd_end_req (
	struct bdbm_drv_info* bdi, 
	struct bdbm_llm_req_t* ptr_llm_req)
{
	struct bdbm_hlm_req_t* hlm_req = (struct bdbm_hlm_req_t* )ptr_llm_req->ptr_hlm_req;

	if (hlm_req->queued == 1) {
		__hlm_rsd_end_req (bdi, ptr_llm_req);
	} else {
		hlm_nobuf_end_req (bdi, ptr_llm_req);
	}
}

