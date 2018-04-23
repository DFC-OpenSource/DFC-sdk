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
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/delay.h> /* mdelay */

#include "portal.h"  
#include "portalmem.h"
#include "dmaManager.h"
#include "GeneratedTypes.h" 

#include "bdbm_drv.h"
#include "params.h"
#include "debug.h"
#include "dm_bluedbm.h"
#include "platform.h"


/*#define BDBM_DBG*/
#define MAX_INDARRAY 4

struct bdbm_dm_inf_t _dm_bluedbm_inf = {
	.ptr_private = NULL,
	.probe = dm_bluedbm_probe,
	.open = dm_bluedbm_open,
	.close = dm_bluedbm_close,
	.make_req = dm_bluedbm_make_req,
	.end_req = dm_bluedbm_end_req,
	.load = dm_bluedbm_load,
	.store = dm_bluedbm_store,
};

struct dm_bluedbm_private {
	bdbm_completion event_handler_completion;
	bdbm_completion connectal_completion;
	bdbm_completion connectal_completion_init;

	/* for Connectal */
	DmaManagerPrivate dma;
	PortalInternal intarr[MAX_INDARRAY];
	sem_t flash_sem;

	/* for thread management */
	struct task_struct *event_handler;
	struct task_struct *connectal_handler;

	/* for tag management */
	bdbm_spinlock_t lock;
	struct bdbm_llm_req_t** llm_reqs;
	uint8_t** rbuf;
	uint8_t** wbuf;
};

extern struct bdbm_drv_info* _bdi;

void __copy_bio_to_dma (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* r);
void __copy_dma_to_bio (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* r);


/**
 * the implementation of call-back functions for Connectal 
 **/
void FlashIndicationreadDone_cb (struct PortalInternal *p, const uint32_t tag)
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (_bdi);
	struct bdbm_llm_req_t* r = NULL;
	unsigned long flags;

	bdbm_spin_lock_irqsave (&priv->lock, flags);
	if ((r = priv->llm_reqs[tag]) == NULL) {
		bdbm_spin_unlock_irqrestore (&priv->lock, flags);
		bdbm_bug_on (1);
	}
	priv->llm_reqs[tag] = NULL;
	__copy_dma_to_bio (_bdi, r);
	_bdi->ptr_dm_inf->end_req (_bdi, r);
	bdbm_spin_unlock_irqrestore (&priv->lock, flags);
}

void FlashIndicationwriteDone_cb (  struct PortalInternal *p, const uint32_t tag )
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (_bdi);
	struct bdbm_llm_req_t* r = NULL;
	unsigned long flags;

	bdbm_spin_lock_irqsave (&priv->lock, flags);
	if ((r = priv->llm_reqs[tag]) == NULL) {
		bdbm_spin_unlock_irqrestore (&priv->lock, flags);
		bdbm_bug_on (1);
	}
	priv->llm_reqs[tag] = NULL;
	_bdi->ptr_dm_inf->end_req (_bdi, r);
	bdbm_spin_unlock_irqrestore (&priv->lock, flags);
}

void FlashIndicationeraseDone_cb (  struct PortalInternal *p, const uint32_t tag, const uint32_t status )
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (_bdi);
	struct bdbm_llm_req_t* r = NULL;
	unsigned long flags;

	bdbm_spin_lock_irqsave (&priv->lock, flags);
	if ((r = priv->llm_reqs[tag]) == NULL) {
		bdbm_spin_unlock_irqrestore (&priv->lock, flags);
		bdbm_bug_on (1);
	}
	priv->llm_reqs[tag] = NULL;
	if (status != 0) {
		bdbm_msg ("*** bad block detected! ***");
		r->ret = 1; /* oops! it is a bad block */
	}
	_bdi->ptr_dm_inf->end_req (_bdi, r);
	bdbm_spin_unlock_irqrestore (&priv->lock, flags);
}

void FlashIndicationdebugDumpResp_cb (  struct PortalInternal *p, const uint32_t debug0, const uint32_t debug1, const uint32_t debug2, const uint32_t debug3, const uint32_t debug4, const uint32_t debug5 )
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (_bdi);

	sem_post (&priv->flash_sem);
}

void MMUIndicationWrapperidResponse_cb (  struct PortalInternal *p, const uint32_t sglId ) 
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (_bdi);

	priv->dma.sglId = sglId;
	sem_post (&priv->dma.sglIdSem);
}

void MMUIndicationWrapperconfigResp_cb (  struct PortalInternal *p, const uint32_t pointer )
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (_bdi);

	sem_post (&priv->dma.confSem);
}

void MMUIndicationWrappererror_cb (  struct PortalInternal *p, const uint32_t code, const uint32_t pointer, const uint64_t offset, const uint64_t extra ) 
{
	PORTAL_PRINTF ("cb: MMUConfigIndicationWrappererror_cb\n");
}

void manual_event (struct dm_bluedbm_private* priv)
{
	int i;
	for (i = 0; i < MAX_INDARRAY; i++)
		portalCheckIndication(&priv->intarr[i]);
}

int event_handler_fn (void* arg) 
{
	struct bdbm_drv_info* bdi = (struct bdbm_drv_info*)arg;
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (bdi);

	while (1) {
		manual_event (priv);
		yield ();
		/*msleep (0);*/
		if (kthread_should_stop ()) {
			bdbm_msg ("event_handler_fn ends");
			break;
		}
	}
	bdbm_msg ("event_handler_fn is going to finish");
	bdbm_complete (priv->event_handler_completion);
	return 0;
}

MMUIndicationCb MMUIndication_cbTable = {
	MMUIndicationWrapperidResponse_cb,
	MMUIndicationWrapperconfigResp_cb,
	MMUIndicationWrappererror_cb,
};

FlashIndicationCb FlashIndication_cbTable = {
	FlashIndicationreadDone_cb,
	FlashIndicationwriteDone_cb,
	FlashIndicationeraseDone_cb,
	FlashIndicationdebugDumpResp_cb,
};

int connectal_handler_fn (void* arg) 
{
	#define BDBM_FLASH_PAGE_SIZE (8192*2)
	#define BDBM_NUM_TAGS 128

	uint32_t t = 0;
	struct bdbm_drv_info* bdi = (struct bdbm_drv_info*)arg;
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (bdi);

	uint32_t src_alloc;
	uint32_t dst_alloc;
	size_t dst_alloc_sz = BDBM_FLASH_PAGE_SIZE * BDBM_NUM_TAGS *sizeof(uint8_t);
	size_t src_alloc_sz = BDBM_FLASH_PAGE_SIZE * BDBM_NUM_TAGS *sizeof(uint8_t);
	uint32_t ref_dst_alloc; 
	uint32_t ref_src_alloc; 
	uint32_t* dst_buffer;
	uint32_t* src_buffer;

	/* create portals in FPGA */
#if 0
	init_portal_internal (&priv->intarr[2], IfcNames_HostMMUConfigRequest, NULL); // fpga3
	init_portal_internal (&priv->intarr[0], IfcNames_HostMMUConfigIndication, MMUConfigIndicationWrapper_handleMessage); // fpga1
	init_portal_internal (&priv->intarr[3], IfcNames_FlashRequest, NULL); // fpga4
	init_portal_internal (&priv->intarr[1], IfcNames_FlashIndication, FlashIndicationWrapper_handleMessage); // fpga2
#endif
	init_portal_internal (&priv->intarr[2], IfcNames_HostMMURequest, NULL, NULL, NULL, NULL, MMURequest_reqinfo); // fpga3
	init_portal_internal (&priv->intarr[0], IfcNames_HostMMUIndication, MMUIndication_handleMessage, &MMUIndication_cbTable, NULL, NULL, MMUIndication_reqinfo); // fpga1
	init_portal_internal (&priv->intarr[3], IfcNames_FlashRequest, NULL, NULL, NULL, NULL, FlashRequest_reqinfo); // fpga4
	init_portal_internal (&priv->intarr[1], IfcNames_FlashIndication, FlashIndication_handleMessage, &FlashIndication_cbTable, NULL, NULL, FlashIndication_reqinfo); // fpga2

	DmaManager_init (&priv->dma, NULL, &priv->intarr[2]);
	sem_init (&priv->flash_sem, 0, 0);

	/* create and run a thread for message handling */
	wake_up_process (priv->event_handler);

	/* get the mapped-memory from Connectal */
	src_alloc = portalAlloc (src_alloc_sz);
	dst_alloc = portalAlloc (dst_alloc_sz);
	src_buffer = (uint32_t*)portalMmap (src_alloc, src_alloc_sz);
	dst_buffer = (uint32_t*)portalMmap (dst_alloc, dst_alloc_sz);

	portalDCacheFlushInval (dst_alloc, dst_alloc_sz, dst_buffer);
	portalDCacheFlushInval (src_alloc, src_alloc_sz, src_buffer);
	ref_dst_alloc = DmaManager_reference (&priv->dma, dst_alloc);
	ref_src_alloc = DmaManager_reference (&priv->dma, src_alloc);

	/* assign the mapped-memory to device */
	for (t = 0; t < BDBM_NUM_TAGS; t++) {
		uint32_t byte_offset = t * BDBM_FLASH_PAGE_SIZE;
		FlashRequest_addDmaWriteRefs (&priv->intarr[3], ref_dst_alloc, byte_offset, t);
		FlashRequest_addDmaReadRefs (&priv->intarr[3], ref_src_alloc, byte_offset, t);
		priv->rbuf[t] = (uint8_t*)(dst_buffer + byte_offset/sizeof(uint32_t));
		priv->wbuf[t] = (uint8_t*)(src_buffer + byte_offset/sizeof(uint32_t));
	}

	/* initialize a device */
	FlashRequest_start (&priv->intarr[3], 0);
	FlashRequest_setDebugVals (&priv->intarr[3], 0, 0);
	FlashRequest_debugDumpReq (&priv->intarr[3], 0);
	sem_wait (&priv->flash_sem);
	FlashRequest_debugDumpReq (&priv->intarr[3], 0);
	sem_wait (&priv->flash_sem);

	/* everything has done */
	bdbm_daemonize ("connectal_handler_fn");
	allow_signal (SIGKILL); 
	bdbm_complete (priv->connectal_completion_init);

	/* sleep until the device driver is unloaded */
	set_current_state (TASK_INTERRUPTIBLE);
	schedule ();	/* go to sleep */
	set_current_state (TASK_RUNNING);

	/* free mapped-memory */
	DmaManager_dereference (&priv->dma, ref_dst_alloc);
	DmaManager_dereference (&priv->dma, ref_src_alloc);
	portalmem_dmabuffer_destroy (src_alloc);
	portalmem_dmabuffer_destroy (dst_alloc);

	bdbm_complete (priv->connectal_completion);

	return 0;
}


/**
 * the implementation of call-back functions for bdbm_drv
 **/
uint32_t dm_bluedbm_probe (struct bdbm_drv_info* bdi)
{
	struct dm_bluedbm_private* priv = NULL;
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	uint32_t nr_punit = np->nr_channels * np->nr_chips_per_channel;

	/* create a private for bluedbm */
	if ((priv = bdbm_zmalloc (sizeof (struct dm_bluedbm_private))) == NULL)	{
		bdbm_warning ("bdbm_zmalloc failed");
		goto fail;
	}
	bdi->ptr_dm_inf->ptr_private = (void*)priv;

	/* initialize some internal data structures */
	bdbm_spin_lock_init (&priv->lock);
	if ((priv->llm_reqs = (struct bdbm_llm_req_t**)bdbm_zmalloc (
			sizeof (struct bdbm_llm_req_t*) * nr_punit)) == NULL) {
		bdbm_warning ("bdbm_zmalloc failed");
		goto fail;
	}
	if ((priv->rbuf = (uint8_t**)bdbm_zmalloc (
			sizeof (uint8_t*) * nr_punit)) == NULL) {
		bdbm_warning ("bdbm_zmalloc failed");
		goto fail;
	}
	if ((priv->wbuf = (uint8_t**)bdbm_zmalloc (
			sizeof (uint8_t*) * nr_punit)) == NULL) {
		bdbm_warning ("bdbm_zmalloc failed");
		goto fail;
	}
	bdbm_init_completion (priv->event_handler_completion);
	bdbm_init_completion (priv->connectal_completion);
	bdbm_init_completion (priv->connectal_completion_init);

	/* register a bdbm device */
	if ((priv->event_handler = kthread_create (
			event_handler_fn, (void*)bdi, "event_handler_fn")) == NULL) {
		bdbm_error ("kthread_create failed");
		goto fail;
	}
	if ((priv->connectal_handler = kthread_create (
			connectal_handler_fn, (void*)bdi, "connectal_handler_fn")) == NULL) {
		bdbm_error ("kthread_create failed");
		goto fail;
	}

	return 0;

fail:
	bdbm_error ("dm_bluedbm_probe failed!");
	bdi->ptr_dm_inf->close (bdi);
	return 1;
}

/* NOTE: To prevent the mapped-memory from Connectal from being freed,
 * we have to run connectal_handler in a different thread */
uint32_t dm_bluedbm_open (struct bdbm_drv_info* bdi)
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (bdi);

	/* start the connectal handler */
	wake_up_process (priv->connectal_handler);

	/* wait until the connectal handler finishes it jobs */
	wait_for_completion (&priv->connectal_completion_init);

	return 0;
}

void dm_bluedbm_close (struct bdbm_drv_info* bdi)
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (bdi);

	if (priv == NULL)
		return;

	if (priv->event_handler) {
		kthread_stop (priv->event_handler);
		wait_for_completion_timeout (
			&priv->event_handler_completion, 
			msecs_to_jiffies(2000));
		bdbm_msg ("event_handler done");
	}
	if (priv->connectal_handler) {
		send_sig (SIGKILL, priv->connectal_handler, 0);
		wait_for_completion_timeout (
			&priv->connectal_completion, 
			msecs_to_jiffies(2000));
	}
	if (priv->llm_reqs)
		bdbm_free (priv->llm_reqs);
	if (priv->rbuf)
		bdbm_free (priv->rbuf);
	if (priv->wbuf)
		bdbm_free (priv->wbuf);
	bdbm_free (priv);
}

void __copy_dma_to_bio (
	struct bdbm_drv_info* bdi,
	struct bdbm_llm_req_t* r)
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (bdi);
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	uint8_t* ptr_dma_addr = NULL;
	uint32_t nr_pages = 0;
	uint32_t loop = 0;
	uint32_t punit_id;

	punit_id = r->phyaddr->channel_no * 
		np->nr_chips_per_channel + r->phyaddr->chip_no;

	nr_pages = np->page_main_size / KERNEL_PAGE_SIZE;
	ptr_dma_addr = (uint8_t*)priv->rbuf[punit_id];

	/* copy the main page data to a buffer */
	for (loop = 0; loop < nr_pages; loop++) {
		if (r->req_type == REQTYPE_RMW_READ && 
			r->kpg_flags[loop] == MEMFLAG_KMAP_PAGE) {
			continue;
		}

		bdbm_memcpy (
			r->pptr_kpgs[loop],
			ptr_dma_addr + KERNEL_PAGE_SIZE * loop, 
			KERNEL_PAGE_SIZE
		);
	}

	/* copy the OOB data to a buffer */
	if (r->req_type != REQTYPE_RMW_READ && 
		r->ptr_oob != NULL &&
		np->page_oob_size != 0) {
		bdbm_memcpy (r->ptr_oob, 
			ptr_dma_addr + np->page_main_size,
			np->page_oob_size
		);
	}
}

void __copy_bio_to_dma (
	struct bdbm_drv_info* bdi,
	struct bdbm_llm_req_t* r)
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (bdi);
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
 	uint8_t* ptr_dma_addr = NULL;
	uint32_t nr_pages = 0;
	uint32_t loop = 0;
	uint32_t punit_id;

	punit_id = r->phyaddr->channel_no * 
		np->nr_chips_per_channel + r->phyaddr->chip_no;

	nr_pages = np->page_main_size / KERNEL_PAGE_SIZE;
	ptr_dma_addr = (uint8_t*)priv->wbuf[punit_id];

	/* copy the main page data to a buffer */
	for (loop = 0; loop < nr_pages; loop++) {
		bdbm_memcpy (
			ptr_dma_addr + KERNEL_PAGE_SIZE * loop, 
			r->pptr_kpgs[loop], 
			KERNEL_PAGE_SIZE
		);
	}

	/* copy the OOB data to a buffer */
	if (np->page_oob_size != 0 && r->ptr_oob != NULL) {
		bdbm_memcpy (ptr_dma_addr + np->page_main_size,
			r->ptr_oob, np->page_oob_size
		);
	}
}

uint32_t dm_bluedbm_make_req (
	struct bdbm_drv_info* bdi, 
	struct bdbm_llm_req_t* ptr_llm_req)
{
	struct dm_bluedbm_private* priv = BDBM_DM_PRIV (bdi);
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	struct bdbm_phyaddr_t* phyaddr = ptr_llm_req->phyaddr;
	unsigned long flags;
	uint32_t punit_id;

	if (ptr_llm_req->req_type == REQTYPE_READ_DUMMY) {
		_bdi->ptr_dm_inf->end_req (bdi, ptr_llm_req);
		return 0;
	}

	/* check punit (= tags) */
	punit_id = phyaddr->channel_no * 
		np->nr_chips_per_channel + phyaddr->chip_no;

	spin_lock_irqsave (&priv->lock, flags);
	if (priv->llm_reqs[punit_id] != NULL) {
		spin_unlock_irqrestore (&priv->lock, flags);
		bdbm_error ("punit_id (%u) is busy...", punit_id);
		bdbm_bug_on (1);
	} else
		priv->llm_reqs[punit_id] = ptr_llm_req;
	spin_unlock_irqrestore (&priv->lock, flags);

#ifdef BDBM_DBG
	switch (ptr_llm_req->req_type) {
	case REQTYPE_WRITE:
		bdbm_msg ("[W-NM] %llu (%llu,%llu,%llu,%llu)", ptr_llm_req->lpa,phyaddr->channel_no,phyaddr->chip_no,phyaddr->block_no,phyaddr->page_no);
		break;
	case REQTYPE_RMW_WRITE:
		bdbm_msg ("[W-RW] %llu (%llu,%llu,%llu,%llu)", ptr_llm_req->lpa,phyaddr->channel_no,phyaddr->chip_no,phyaddr->block_no,phyaddr->page_no);
		break;
	case REQTYPE_GC_WRITE:
		bdbm_msg ("[W-GC] %llu (%llu,%llu,%llu,%llu)", ptr_llm_req->lpa,phyaddr->channel_no,phyaddr->chip_no,phyaddr->block_no,phyaddr->page_no);
		break;
	case REQTYPE_READ:
		bdbm_msg ("[R-NM] %llu (%llu,%llu,%llu,%llu)", ptr_llm_req->lpa,phyaddr->channel_no,phyaddr->chip_no,phyaddr->block_no,phyaddr->page_no);
		break;
	case REQTYPE_RMW_READ:
		bdbm_msg ("[R-RW] %llu (%llu,%llu,%llu,%llu)", ptr_llm_req->lpa,phyaddr->channel_no,phyaddr->chip_no,phyaddr->block_no,phyaddr->page_no);
		break;
	case REQTYPE_GC_READ:
		bdbm_msg ("[R-GC] %llu (%llu,%llu,%llu,%llu)", ptr_llm_req->lpa,phyaddr->channel_no,phyaddr->chip_no,phyaddr->block_no,phyaddr->page_no);
		break;
	default:
		break;
	}
#endif

	/* submit reqs to the device */
	switch (ptr_llm_req->req_type) {
	case REQTYPE_WRITE:
	case REQTYPE_RMW_WRITE:
	case REQTYPE_GC_WRITE:
		__copy_bio_to_dma (bdi, ptr_llm_req);
		FlashRequest_writePage (
			&priv->intarr[3], 
			phyaddr->channel_no, 
			phyaddr->chip_no, 
			phyaddr->block_no+BLKOFS, 
			phyaddr->page_no, 
			punit_id);
		break;

	case REQTYPE_READ:
	case REQTYPE_RMW_READ:
	case REQTYPE_GC_READ:
		FlashRequest_readPage (
			&priv->intarr[3], 
			phyaddr->channel_no, 
			phyaddr->chip_no, 
			phyaddr->block_no+BLKOFS, 
			phyaddr->page_no, 
			punit_id);
		break;

	case REQTYPE_GC_ERASE:
		FlashRequest_eraseBlock (
			&priv->intarr[3], 
			phyaddr->channel_no, 
			phyaddr->chip_no, 
			phyaddr->block_no+BLKOFS, 
			punit_id);
		break;

	default:
		break;
	}

	return 0;
}

void dm_bluedbm_end_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* ptr_llm_req)
{
	bdi->ptr_llm_inf->end_req (bdi, ptr_llm_req);
}

uint32_t dm_bluedbm_load (struct bdbm_drv_info* bdi, const char* fn)
{
	/* do nothing */
	return 0;
}

uint32_t dm_bluedbm_store (struct bdbm_drv_info* bdi, const char* fn)
{
	/* do nothing */
	return 0;
}

