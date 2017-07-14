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
#include "bdbm_drv.h"
#include "debug.h"
#include "platform.h"
#include "host_block.h"
#include "params.h"
#include "ioctl.h"
#include "types.h"
#include "../nvme.h"
/*#include "utils/time.h" */

/*#define ENABLE_DISPLAY*/
extern void nvme_bio_cb(nvme_bio *bio,int ret);

unsigned int host_block_open (struct bdbm_drv_info* bdi);
void host_block_close (struct bdbm_drv_info* bdi);
int host_block_make_req (struct bdbm_drv_info* bdi, struct nvme_bio *bio);
void host_block_end_req (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* hlm_req);

/* interface for host */
struct bdbm_host_inf_t _host_block_inf = {
	.ptr_private = NULL,
	.open = host_block_open,
	.close = host_block_close,
	.make_req = host_block_make_req,
	.end_req = host_block_end_req,
};

#if 0
static struct bdbm_device_t {
	struct gendisk *gd;
	struct request_queue *queue;
	bdbm_completion make_request_lock;
} bdbm_device;

static unsigned int bdbm_device_major_num = 0;
static struct block_device_operations bdops = {
	.owner = THIS_MODULE,
	//.ioctl = bdbm_blk_ioctl,
	//.getgeo = bdbm_blk_getgeo,
};
#endif
/* global data structure */
extern struct bdbm_drv_info* _bdi;


static struct bdbm_hlm_req_t* __host_block_create_hlm_trim_req (
	struct bdbm_drv_info* bdi, 
	struct nvme_bio* bio)
{
	struct bdbm_hlm_req_t* hlm_req = NULL;
#if 0
	struct nand_params* np = &bdi->ptr_bdbm_params->nand;
	struct driver_params* dp = &bdi->ptr_bdbm_params->driver;
	unsigned long nr_secs_per_fp = 0;

	nr_secs_per_fp = np->page_main_size >> KERNEL_SECTOR_SZ_SHIFT;
#endif
	/* create bdbm_hm_req_t */
	if ((hlm_req = (struct bdbm_hlm_req_t*)bdbm_malloc_atomic
			(sizeof (struct bdbm_hlm_req_t))) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		return NULL;
	}

	/* make a high-level request for TRIM */
	hlm_req->req_type = REQTYPE_TRIM;
	hlm_req->is_seq_lpa = bio->is_seq;

#if 0
	if (dp->mapping_type == MAPPING_POLICY_SEGMENT) {
		hlm_req->lpa = bio->slba / nr_secs_per_fp;
		hlm_req->len = bio->n_sectors / nr_secs_per_fp;
		if (hlm_req->len == 0) 
			hlm_req->len = 1;
	} else {
		hlm_req->lpa = (bio->slba + nr_secs_per_fp - 1) / nr_secs_per_fp;

		if ((hlm_req->lpa * nr_secs_per_fp - bio->slba) > bio->n_sectors) {
			bdbm_error ("'hlm_req->lpa (%lu) * nr_secs_per_fp (%lu) - bio->slba (%lu)' (%lu) > bio->nlb (%u)",
					hlm_req->lpa, nr_secs_per_fp, bio->slba,
					hlm_req->lpa * nr_secs_per_fp - bio->slba,
					bio->n_sectors);
			hlm_req->len = 0;
		} else {
			hlm_req->len = (bio->slba + bio->n_sectors + nr_secs_per_fp - 1) / nr_secs_per_fp - hlm_req->lpa;
		}
	}
#endif
	hlm_req->lpa = &bio->slba;
	hlm_req->len = bio->nlb;
	hlm_req->nr_llm_reqs = 0;
	hlm_req->nr_done_reqs = 0;
	hlm_req->kpg_flags = NULL;
	hlm_req->pptr_kpgs = NULL;	/* no data */
	hlm_req->ptr_host_req = (void*)bio;
	hlm_req->ret = 0;

	return hlm_req;
}

static struct bdbm_hlm_req_t* __host_block_create_hlm_rq_req (
	struct bdbm_drv_info* bdi, 
	struct nvme_bio* bio)
{
/*	struct bio_vec *bvec = NULL; */
	struct bdbm_hlm_req_t* hlm_req = NULL;
	struct nand_params* np = &bdi->ptr_bdbm_params->nand;

	uint64_t loop = 0;
	uint64_t slba = 0;
	unsigned int kpg_loop = 0;
#if 0
	unsigned int bvec_offset = 0;
	unsigned long nr_secs_per_fp = 0;
#endif
	unsigned long nr_secs_per_kp = 0;
	unsigned int nr_kp_per_fp = 0;
	int nprps;
	uint64_t *prp = bio->prp;
	/* get # of sectors per flash page */
	nr_secs_per_kp = KERNEL_PAGE_SIZE >> KERNEL_SECTOR_SZ_SHIFT;
	nr_kp_per_fp = np->page_main_size >> KERNEL_PAGE_SHIFT;	/* e.g., 2 = 8 KB / 4 KB */

	/* create bdbm_hm_req_t */
	if ((hlm_req = (struct bdbm_hlm_req_t*)bdbm_malloc_atomic
			(sizeof (struct bdbm_hlm_req_t))) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		return NULL;
	}
#if 0
	/* get a bio direction */
	if (bio_data_dir (bio) == READ || bio_data_dir (bio) == READA) {
		hlm_req->req_type = REQTYPE_READ;
	} else if (bio_data_dir (bio) == WRITE) {
		hlm_req->req_type = REQTYPE_WRITE;
	} else {
		bdbm_error ("the direction of a bio is invalid (%lu)", bio_data_dir (bio));
		goto fail_req;
	}
#endif
	if ((bio->req_type != REQTYPE_WRITE) && (bio->req_type != REQTYPE_READ)) {
		bdbm_error ("invalid request type: %d\n", bio->req_type);
		goto fail_req;
	}
	hlm_req->req_type = bio->req_type;

	if (bio->req_type == REQTYPE_READ) {
		hlm_req->len = bio->nlb;
	} else {
		hlm_req->len = bio->nlb + ((nr_kp_per_fp * (!!(bio->nlb & (nr_kp_per_fp-1)))) - (bio->nlb & (nr_kp_per_fp-1)));
	}
	/* make a high-level request for READ or WRITE */
	//hlm_req->lpa = (bio->slba / nr_secs_per_fp);
	//hlm_req->offset = bio->offset;
	//hlm_req->len = (bio->slba + bio->n_sectors + nr_secs_per_fp - 1) / nr_secs_per_fp - hlm_req->lpa;
	hlm_req->lpa = bdbm_malloc (sizeof(hlm_req->lpa) * hlm_req->len);
	if (bio->is_seq) {
		//slba = (uint64_t)bio->slba/nr_secs_per_kp;/*TODO need to remove once after the testing without cache is done*/
		slba = (uint64_t)bio->slba>>3;/*TODO need to remove once after the testing without cache is done*/
		for (loop = 0; loop < bio->nlb; loop++,slba++) {
			hlm_req->lpa[loop] = slba;
		}
	} else {
		uint64_t *lpa;
		lpa = (uint64_t *)bio->slba;
		for (loop = 0; loop < bio->nlb; loop++) {
			/*hlm_req->lpa[loop] = lpa[loop]/nr_secs_per_kp;*/
			hlm_req->lpa[loop] = lpa[loop]>>3;
		}
	}
	hlm_req->is_seq_lpa = bio->is_seq;
	hlm_req->nr_done_reqs = 0;
	hlm_req->nr_llm_reqs = 0;
	hlm_req->ptr_host_req = (void*)bio;
	hlm_req->ret = 0;
/*	bdbm_spin_lock_init (&hlm_req->lock); */
	if ((hlm_req->pptr_kpgs = (unsigned char**)bdbm_malloc_atomic
			(sizeof(unsigned char*) * hlm_req->len)) == NULL) { 
		bdbm_error ("bdbm_malloc_atomic failed"); 
		goto fail_req;
	}
	if ((hlm_req->kpg_flags = (unsigned char*)bdbm_malloc_atomic
			(sizeof(unsigned char) * hlm_req->len)) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		goto fail_flags;
	}
	/* kpg_flags is set to MEMFLAG_NOT_SET (0) */

	/* get or alloc pages */
	//for(nprps=0 ; nprps < bio->n_sectors ; nprps++) {
#if 0
	for(nprps=0 ; nprps < bio->nprps ; nprps++) {
		/* check some error cases */
#if 0
		if (bvec->bv_offset != 0) {
			bdbm_warning ("'bv_offset' is not 0 (%d)", bvec->bv_offset);
			/*goto fail_grab_pages;*/
		}
#endif
next_kpg:
		/* assign a new page */
		if ((hlm_req->lpa * nr_kp_per_fp + kpg_loop) != (bio->slba + bvec_offset) / nr_secs_per_kp) {
			hlm_req->pptr_kpgs[kpg_loop] = (unsigned char*)bdbm_malloc_atomic (KERNEL_PAGE_SIZE);
			hlm_req->kpg_flags[kpg_loop] = MEMFLAG_FRAG_PAGE;
			kpg_loop++;
			goto next_kpg;
		}

		if ((hlm_req->pptr_kpgs[kpg_loop] = (unsigned char*) (*prp)) != NULL) {
			hlm_req->kpg_flags[kpg_loop] = MEMFLAG_KMAP_PAGE;
		} else {
			bdbm_error ("kmap failed");
			goto fail_grab_pages;
		}

		bvec_offset += nr_secs_per_kp;
		kpg_loop++;
		prp++;
	}
#endif
	for(nprps=0 ; nprps < bio->nlb ; nprps++) {
		if ((hlm_req->pptr_kpgs[kpg_loop] = (unsigned char*) (*prp)) != NULL) {
			hlm_req->kpg_flags[kpg_loop] = MEMFLAG_KMAP_PAGE;
		} else {
			bdbm_error ("kmap failed");
			goto fail_grab_pages;
		}
		kpg_loop++;
		prp++;
	}
	/* get additional free pages if necessary */
	while (kpg_loop < hlm_req->len) {
		hlm_req->lpa[kpg_loop] = -1ULL;
		hlm_req->pptr_kpgs[kpg_loop] = (unsigned char*)bdbm_malloc_atomic (KERNEL_PAGE_SIZE);
		hlm_req->kpg_flags[kpg_loop] = MEMFLAG_FRAG_PAGE;
		kpg_loop++;
	}
	return hlm_req;

fail_grab_pages:
	printf ("fail_grab_pages\n");
	/* release grabbed pages */
	for (kpg_loop = 0; kpg_loop < hlm_req->len; kpg_loop++) {
		if (hlm_req->kpg_flags[kpg_loop] == MEMFLAG_FRAG_PAGE) {
			bdbm_free_atomic (hlm_req->pptr_kpgs[kpg_loop]);
		} else if (hlm_req->kpg_flags[kpg_loop] == MEMFLAG_KMAP_PAGE) {
		} else if (hlm_req->kpg_flags[kpg_loop] != MEMFLAG_NOT_SET) {
			bdbm_error ("invalid flags (kpg_flags[%u]=%u)", kpg_loop, hlm_req->kpg_flags[kpg_loop]);
		}
	}
	bdbm_free_atomic (hlm_req->kpg_flags);

fail_flags:
	bdbm_free_atomic (hlm_req->pptr_kpgs);

fail_req:
	bdbm_free_atomic (hlm_req);

	return NULL;
}

static struct bdbm_hlm_req_t* __host_block_create_hlm_req (
	struct bdbm_drv_info* bdi, 
	struct nvme_bio* bio)
{
	struct bdbm_hlm_req_t* hlm_req = NULL;
	struct nand_params* np = &bdi->ptr_bdbm_params->nand;
#if 0
	unsigned long nr_secs_per_kp = 0;

	/* get # of sectors per flash page */
	nr_secs_per_kp = KERNEL_PAGE_SIZE >> KERNEL_SECTOR_SZ_SHIFT;
	/* see if some error cases */
	if (bio->bi_sector % nr_secs_per_kp != 0) {
		bdbm_warning ("kernel pages are not aligned with disk sectors (%lu mod %lu != 0)",
			bio->bi_sector, nr_secs_per_kp);
		/* go ahead */
	}
#endif
	if (KERNEL_PAGE_SIZE > np->page_main_size) {
		bdbm_error ("kernel page (%d) is larger than flash page (%lu)",
			KERNEL_PAGE_SIZE, np->page_main_size);
		return NULL;
	}

	/* create 'hlm_req' */
	/* make a high-level request for READ or WRITE */
	if (bio->req_type == REQTYPE_TRIM) {
		hlm_req = __host_block_create_hlm_trim_req (bdi, bio);
	} else {
		hlm_req = __host_block_create_hlm_rq_req (bdi, bio);
	}

#if 0
	/* start a stopwatch */
	if (hlm_req) {
		bdbm_stopwatch_start (&hlm_req->sw);
	}
#endif
	if (!hlm_req) {
		printf ("!hlm_req\n");
	}
	return hlm_req;
}

static void __host_block_delete_hlm_req (
	struct bdbm_drv_info* bdi, 
	struct bdbm_hlm_req_t* hlm_req)
{
	unsigned int kpg_loop = 0;
#if 0
	struct nand_params* np = NULL;
	unsigned int nr_kp_per_fp = 0;

	np = &bdi->ptr_bdbm_params->nand;
	nr_kp_per_fp = np->page_main_size >> KERNEL_PAGE_SHIFT;	/* e.g., 2 = 8 KB / 4 KB */
#endif
	/* temp */
	if (hlm_req->org_pptr_kpgs) {
		hlm_req->pptr_kpgs = hlm_req->org_pptr_kpgs;
		hlm_req->kpg_flags = hlm_req->org_kpg_flags;
		hlm_req->lpa--;
		hlm_req->len++;
	}
	/* end */

	/* free or unmap pages */
	if (hlm_req->kpg_flags != NULL && hlm_req->pptr_kpgs != NULL) {
		for (kpg_loop = 0; kpg_loop < hlm_req->len; kpg_loop++) {
			if (hlm_req->kpg_flags[kpg_loop] == MEMFLAG_FRAG_PAGE_DONE) {
				bdbm_free_atomic (hlm_req->pptr_kpgs[kpg_loop]);
			} else if (hlm_req->kpg_flags[kpg_loop] == MEMFLAG_KMAP_PAGE_DONE) {
			} else if (hlm_req->kpg_flags[kpg_loop] != MEMFLAG_NOT_SET) {
				bdbm_error ("invalid flags (kpg_flags[%u]=%u)", kpg_loop, hlm_req->kpg_flags[kpg_loop]);
			}
		}
	}

	/* release other stuff */
	if ((hlm_req->req_type != REQTYPE_TRIM) && (hlm_req->lpa != NULL)) {
		bdbm_free (hlm_req->lpa);
	}
	if (hlm_req->kpg_flags != NULL) 
		bdbm_free_atomic (hlm_req->kpg_flags);
	if (hlm_req->pptr_kpgs != NULL) 
		bdbm_free_atomic (hlm_req->pptr_kpgs);
	bdbm_free_atomic (hlm_req);
}

#ifdef ENABLE_DISPLAY
static void __host_block_display_req (
	struct bdbm_drv_info* bdi, 
	struct bdbm_hlm_req_t* hlm_req)
{
	struct bdbm_ftl_inf_t* ftl = (struct bdbm_ftl_inf_t*)BDBM_GET_FTL_INF(bdi);
	unsigned long seg_no = 0;

	if (ftl->get_segno) {
		seg_no = ftl->get_segno (bdi, hlm_req->lpa);
	}

	switch (hlm_req->req_type) {
		case REQTYPE_TRIM:
			bdbm_msg ("[%lu] TRIM\t%lu\t%lu", seg_no, hlm_req->lpa, hlm_req->len);
			break;
		case REQTYPE_READ:
			/*bdbm_msg ("[%lu] READ\t%lu\t%lu", seg_no, hlm_req->lpa, hlm_req->len);*/
			break;
		case REQTYPE_WRITE:
			/*bdbm_msg ("[%lu] WRITE\t%lu\t%lu", seg_no, hlm_req->lpa, hlm_req->len);*/
			break;
		default:
			bdbm_error ("invalid REQTYPE (%u)", hlm_req->req_type);
			break;
	}
}
#endif
#if 0
int nvme_bio_req (struct nvme_bio *bio)
{
#if 0
	/* see if q or bio is valid or not */
	if (q == NULL || bio == NULL) {
		bdbm_msg ("q or bio is NULL; ignore incoming requests");
		return;
	}
	/* grab the lock until a host request is sent to hlm */
	bdbm_wait_for_completion (bdbm_device.make_request_lock);
	bdbm_reinit_completion (bdbm_device.make_request_lock);

#endif
	return host_block_make_req (_bdi, bio);

/*	return 0;*/ /*FIXME*/
	/* free the lock*/
/*	bdbm_complete (bdbm_device.make_request_lock);           */
}
#endif
#if 0
static unsigned int __host_block_register_block_device (struct bdbm_drv_info* bdi)
{
	struct bdbm_params* p = bdi->ptr_bdbm_params;

	/* create a completion lock */
	bdbm_init_completion (bdbm_device.make_request_lock);
	bdbm_complete (bdbm_device.make_request_lock);

	/* create a blk queue */
	if (!(bdbm_device.queue = blk_alloc_queue (GFP_KERNEL))) {
		bdbm_error ("blk_alloc_queue failed");
		return -ENOMEM;
	}
	blk_queue_make_request (bdbm_device.queue, __host_block_make_request);
	blk_queue_logical_block_size (bdbm_device.queue, p->driver.kernel_sector_size);
	blk_queue_io_min (bdbm_device.queue, p->nand.page_main_size);
	blk_queue_io_opt (bdbm_device.queue, p->nand.page_main_size);

	/*blk_limits_max_hw_sectors (&bdbm_device.queue->limits, 16);*/

	/* see if a TRIM command is used or not */
	if (p->driver.trim == TRIM_ENABLE) {
		bdbm_device.queue->limits.discard_granularity = KERNEL_PAGE_SIZE;
		bdbm_device.queue->limits.max_discard_sectors = UINT_MAX;
		/*bdbm_device.queue->limits.discard_zeroes_data = 1;*/
		queue_flag_set_unlocked (QUEUE_FLAG_DISCARD, bdbm_device.queue);
		bdbm_msg ("TRIM is enabled");
	} else {
		bdbm_msg ("TRIM is disabled");
	}

	/* register a blk device */
	if ((bdbm_device_major_num = register_blkdev (bdbm_device_major_num, "blueDBM")) < 0) {
		bdbm_msg ("register_blkdev failed (%d)", bdbm_device_major_num);
		return bdbm_device_major_num;
	}
	if (!(bdbm_device.gd = alloc_disk (1))) {
		bdbm_msg ("alloc_disk failed");
		unregister_blkdev (bdbm_device_major_num, "blueDBM");
		return -ENOMEM;
	}
	bdbm_device.gd->major = bdbm_device_major_num;
	bdbm_device.gd->first_minor = 0;
	bdbm_device.gd->fops = &bdops;
	bdbm_device.gd->queue = bdbm_device.queue;
	bdbm_device.gd->private_data = NULL;
	strcpy (bdbm_device.gd->disk_name, "blueDBM");
	set_capacity (bdbm_device.gd, p->nand.device_capacity_in_byte / KERNEL_SECTOR_SIZE);
	add_disk (bdbm_device.gd);

	return 0;
}

void __host_block_unregister_block_device (struct bdbm_drv_info* bdi)
{
	/* unregister a BlueDBM device driver */
	del_gendisk (bdbm_device.gd);
	put_disk (bdbm_device.gd);
	unregister_blkdev (bdbm_device_major_num, "blueDBM");
	blk_cleanup_queue (bdbm_device.queue);
}

#endif

unsigned int host_block_open (struct bdbm_drv_info* bdi)
{
	/*unsigned int ret;*/
	struct bdbm_host_block_private* p;

	/* create a private data structure */
	if ((p = (struct bdbm_host_block_private*)bdbm_malloc_atomic
			(sizeof (struct bdbm_host_block_private))) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		return 1;
	}
	p->nr_host_reqs = 0;
// 	bdbm_spin_lock_init (&p->lock); 
	pthread_spin_init(&p->lock, PTHREAD_PROCESS_SHARED);
	bdi->ptr_host_inf->ptr_private = (void*)p;

#if 0
	/* register blueDBM */
	if ((ret = __host_block_register_block_device (bdi)) != 0) {
		bdbm_error ("failed to register blueDBM");
		bdbm_free_atomic (p);
		return 1;
	}
#endif

	return 0;
}

void host_block_close (struct bdbm_drv_info* bdi)
{
	/*unsigned long flags;*/
	struct bdbm_host_block_private* p = NULL;

	p = (struct bdbm_host_block_private*)BDBM_HOST_PRIV(bdi);

	/* wait for host reqs to finish */
	bdbm_msg ("wait for host reqs to finish");
#if 0
	for (;;) {
		bdbm_spin_lock_irqsave (&p->lock, flags);
		if (p->nr_host_reqs == 0) {
			bdbm_spin_unlock_irqrestore (&p->lock, flags);
			break;
		}
		bdbm_spin_unlock_irqrestore (&p->lock, flags);
		schedule (); /* sleep */
	}
#endif

	/* unregister a block device */
// 	__host_block_unregister_block_device (bdi);
	pthread_spin_destroy(&p->lock);

	/* free private */
	bdbm_free_atomic (p);
}

int host_block_allow_gc (struct bdbm_drv_info* bdi)
{
	struct bdbm_ftl_inf_t *ftl = BDBM_GET_FTL_INF(bdi);
	/* see if gc is needed, then signal GC thread to work it's way*/
	/* TODO: lock */

	if (ftl->is_gc_needed != NULL && ftl->is_gc_needed (bdi)) {
		return ftl->do_gc (bdi);
	}
	/* TODO: release */

	return 0;
}

int host_block_make_req (struct bdbm_drv_info* bdi, struct nvme_bio *bio)
{
	/*unsigned long flags;*/
	struct nand_params* np = NULL;
	struct bdbm_hlm_req_t* hlm_req = NULL;
	struct bdbm_host_block_private* p = NULL;

	np = &bdi->ptr_bdbm_params->nand;
	if (!np) {
		printf ("!np\n");
		return -1;
	}
	p = (struct bdbm_host_block_private*)BDBM_HOST_PRIV(bdi);
#if 0
	/* see if the address range of bio is beyond storage space */
	if (bio->bi_sector + bio_sectors (bio) > np->device_capacity_in_byte / KERNEL_SECTOR_SIZE) {
		bdbm_error ("bio is beyond storage space (%lu > %lu)",
			bio->bi_sector + bio_sectors (bio),
			np->device_capacity_in_byte / KERNEL_SECTOR_SIZE);
		bio_io_error (bio);
		return;
	}
#endif
	/* create a hlm_req using a bio */
	if ((hlm_req = __host_block_create_hlm_req (bdi, bio)) == NULL) {
		bdbm_error ("the creation of hlm_req failed");
/*		bio_io_error (bio); */
		return -1;
	}

	/* display req info */
#ifdef ENABLE_DISPLAY
	__host_block_display_req (bdi, hlm_req);
#endif
#if 0
	/* if success, increase # of host reqs */
	bdbm_spin_lock_irqsave (&p->lock, flags);
#endif
	pthread_spin_lock (&p->lock);
	p->nr_host_reqs++;
	pthread_spin_unlock (&p->lock);
#if 0
	bdbm_spin_unlock_irqrestore (&p->lock, flags);
#endif
	/* NOTE: it would be possible that 'hlm_req' becomes NULL 
	 * if 'bdi->ptr_hlm_inf->make_req' is success. */
	if (bdi->ptr_hlm_inf->make_req (bdi, hlm_req) != 0) {
		printf("bio %p %d %p %ld", bio, bio->is_seq, bio->nlb, bio->req_info, bio->slba);
		bdbm_error ("'bdi->ptr_hlm_inf->make_req' failed");
#if 0
		/* decreate # of reqs */
		bdbm_spin_lock_irqsave (&p->lock, flags);
#endif
		pthread_spin_lock (&p->lock);
		if (p->nr_host_reqs > 0)
			p->nr_host_reqs--;
		else
			bdbm_error ("p->nr_host_reqs is 0");
		pthread_spin_unlock (&p->lock);
#if 0
		bdbm_spin_unlock_irqrestore (&p->lock, flags);
#endif
		/* finish a bio */
		__host_block_delete_hlm_req (bdi, hlm_req);
		return -1;
		/*bio_io_error (bio);*/
	}
	return 0;
}

void host_block_end_req (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* hlm_req)
{
	unsigned int ret;
	/*unsigned long flags;*/
	struct nvme_bio *bio = NULL;
	struct bdbm_host_block_private* p = NULL;
	/*NvmeRequest *req = NULL;*/


	/* get a bio from hlm_req */
	bio = (struct nvme_bio*)(hlm_req->ptr_host_req);
	p = (struct bdbm_host_block_private*)BDBM_HOST_PRIV(bdi);
	ret = hlm_req->ret;
	/*req = (NvmeRequest*)(bio->req_info);*/

	/* destroy hlm_req */
	__host_block_delete_hlm_req (bdi, hlm_req);
	if(bio->req_type != REQTYPE_TRIM){
		nvme_bio_cb(bio, ret);
	}
#if 0
	/* get the result and end a bio */
	if (bio != NULL) {
		if (ret == 0)
			bio_endio (bio, 0);
		else
			bio_io_error (bio);
	}
#endif
	/* decreate # of reqs */
/*	bdbm_spin_lock_irqsave (&p->lock, flags);*/
	pthread_spin_lock (&p->lock);
	if (p->nr_host_reqs > 0)
		p->nr_host_reqs--;
	else
		bdbm_bug_on (1);
	pthread_spin_unlock (&p->lock);
/*	bdbm_spin_unlock_irqrestore (&p->lock, flags); */
}

