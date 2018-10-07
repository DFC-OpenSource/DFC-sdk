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
#include "dmaManager.h"
#include "GeneratedTypes.h" 

#include "bdbm_drv.h"
#include "params.h"
#include "debug.h"
#include "dm_bluesim.h"


static int nandsim_init = 0;

/* for sock_utils.c */
ssize_t xbsv_kernel_read (struct file *f, char __user *arg, size_t len, loff_t *data);
ssize_t xbsv_kernel_write (struct file *f, const char __user *arg, size_t len, loff_t *data);
struct semaphore bsim_start;
int main_program_finished = 0;
int bsim_relay_running;

static struct file_operations pa_fops = {
	.owner = THIS_MODULE,
	.read = xbsv_kernel_read,
	.write = xbsv_kernel_write,
};

static struct miscdevice miscdev = {
	.minor = MISC_DYNAMIC_MINOR,  // Must be < 256!
	.name = "xbsvtest",
	.fops = &pa_fops,
};


/* for kernel threads */
DECLARE_COMPLETION(worker_completion);
static struct task_struct *kid_worker = NULL;
static int trace_memory;// = 1;

#define MAX_INDARRAY 4
static PortalInternal intarr[MAX_INDARRAY];
static sem_t test_sem;
#ifdef BSIM
#define numWords 0x1240000/4 // make sure to allocate at least one entry of each size
#else
#define numWords 0x124000/4
#endif
static long alloc_sz = numWords*sizeof(unsigned int);
static DmaManagerPrivate priv;
size_t numBytes = 1 << 12;

int srcAlloc;
unsigned int *srcBuffer;
unsigned int ref_srcAlloc;


/* data structures for dm_bluesim */
struct bdbm_dm_inf_t _dm_bluesim_inf = {
	.ptr_private = NULL,
	.probe = dm_bluesim_probe,
	.open = dm_bluesim_open,
	.close = dm_bluesim_close,
	.make_req = dm_bluesim_make_req,
	.end_req = dm_bluesim_end_req,
	.load = NULL,
	.store = NULL,
};

extern struct bdbm_drv_info* _bdi;
struct bdbm_llm_req_t* ptr_llm_req = NULL;

struct dm_bdbme_private {

};


/* call-back functions */
void NandSimIndicationWrappereraseDone_cb (  struct PortalInternal *p, const uint32_t tag )
{
	PORTAL_PRINTF( "NandSim_eraseDone(tag = %x)\n", tag);
	sem_post(&test_sem);
}
void NandSimIndicationWrapperwriteDone_cb (  struct PortalInternal *p, const uint32_t tag )
{
	PORTAL_PRINTF( "NandSim_writeDone(tag = %x)\n", tag);
	sem_post(&test_sem);
}
void NandSimIndicationWrapperreadDone_cb (  struct PortalInternal *p, const uint32_t tag )
{
	PORTAL_PRINTF( "NandSim_readDone(tag = %x)\n", tag);
	sem_post(&test_sem);
}
void DmaIndicationWrapperconfigResp_cb (  struct PortalInternal *p, const uint32_t pointer )
{
	PORTAL_PRINTF("DmaIndication_configResp(physAddr=%x)\n", pointer);
	sem_post(&priv.confSem);
}
void DmaIndicationWrapperaddrResponse_cb (  struct PortalInternal *p, const uint64_t physAddr )
{
	PORTAL_PRINTF("DmaIndication_addrResponse(physAddr=%"PRIx64")\n", physAddr);
}
void DmaIndicationWrapperreportStateDbg_cb (  struct PortalInternal *p, const DmaDbgRec rec )
{
	PORTAL_PRINTF("reportStateDbg: {x:%08x y:%08x z:%08x w:%08x}\n", rec.x,rec.y,rec.z,rec.w);
	sem_post(&priv.dbgSem);
}
void DmaIndicationWrapperreportMemoryTraffic_cb (  struct PortalInternal *p, const uint64_t words )
{
	PORTAL_PRINTF("reportMemoryTraffic: words=%"PRIx64"\n", words);
	priv.mtCnt = words;
	sem_post(&priv.mtSem);
}
void DmaIndicationWrapperdmaError_cb (  struct PortalInternal *p, const uint32_t code, const uint32_t pointer, const uint64_t offset, const uint64_t extra ) 
{
	PORTAL_PRINTF("DmaIndication::dmaError(code=%x, pointer=%x, offset=%"PRIx64" extra=%"PRIx64"\n", code, pointer, offset, extra);
}

void manual_event(void)
{
	int i;
	for (i = 0; i < MAX_INDARRAY; i++) {
		PortalInternal *instance = &intarr[i];
		volatile unsigned int *map_base = instance->map_base;
		unsigned int queue_status;
		while ((queue_status= READL(instance, &map_base[IND_REG_QUEUE_STATUS]))) {
			unsigned int int_src = READL(instance, &map_base[IND_REG_INTERRUPT_FLAG]);
			unsigned int int_en  = READL(instance, &map_base[IND_REG_INTERRUPT_MASK]);
			unsigned int ind_count  = READL(instance, &map_base[IND_REG_INTERRUPT_COUNT]);
			PORTAL_PRINTF("(%d:fpga%d) about to receive messages int=%08x en=%08x qs=%08x cnt=%x\n", i, instance->fpga_number, int_src, int_en, queue_status, ind_count);
			instance->handler(instance, queue_status-1);
		}
	}
}

int nandsim_worker_thread_fn (void* arg) 
{
	while (1) {
		manual_event ();
		msleep (0);
		if (kthread_should_stop ()) {
			bdbm_msg ("nandsim_worker_thread_fn ends");
			break;
		}
	}
	complete (&worker_completion);
	return 0;
}

/* interrupt handler */
static void __dm_bluesim_ih (void* arg)
{
	/*
	struct bdbm_llm_req_t* ptr_llm_req = (struct bdbm_llm_req_t*)arg;
	struct bdbm_drv_info* bdi = _bdi;

	bdi->ptr_dm_inf->end_req (bdi, ptr_llm_req);
	*/

	bdbm_bug_on (1);
}

uint32_t dm_bluesim_probe (struct bdbm_drv_info* bdi)
{
	bdbm_msg ("bdbm_nandsim_thread_init minor %d\n", miscdev.minor);

	misc_register (&miscdev);
	if ((kid_worker = kthread_create (nandsim_worker_thread_fn, NULL, "nandsim_worker_thread")) == NULL) {
		bdbm_error ("kthread_create failed");
		goto fail;
	}
	sema_init (&bsim_start, 0);
	nandsim_init = 1;
	return 0;

fail:
	misc_deregister (&miscdev);
	return 1;
}

uint32_t dm_bluesim_open (struct bdbm_drv_info* bdi)
{
	init_portal_internal(&intarr[0], IfcNames_DmaIndication, DmaIndicationWrapper_handleMessage);     // fpga1
	init_portal_internal(&intarr[1], IfcNames_NandSimIndication, NandSimIndicationWrapper_handleMessage); // fpga2
	init_portal_internal(&intarr[2], IfcNames_DmaConfig, DmaConfigProxy_handleMessage);         // fpga3
	init_portal_internal(&intarr[3], IfcNames_NandSimRequest, NandSimRequestProxy_handleMessage);    // fpga4

	bdbm_msg ("dm_bluesim_open-1");

	sem_init(&test_sem, 0, 0);
	DmaManager_init(&priv, &intarr[2]);
	srcAlloc = portalAlloc(alloc_sz);

	bdbm_msg ("dm_bluesim_open-2");

	PORTAL_PRINTF( "Main: creating exec thread\n");
	wake_up_process (kid_worker);
	srcBuffer = (unsigned int *)portalMmap(srcAlloc, alloc_sz);

	bdbm_msg ("dm_bluesim_open-3");

	portalEnableInterrupts(&intarr[0]);
	portalEnableInterrupts(&intarr[1]);
	portalEnableInterrupts(&intarr[2]);
	portalEnableInterrupts(&intarr[3]);

	bdbm_msg ("dm_bluesim_open-4");

	ref_srcAlloc = DmaManager_reference(&priv, srcAlloc);
	portalDCacheFlushInval(srcAlloc, alloc_sz, srcBuffer);

	bdbm_msg ("dm_bluesim_open-5");

	return 0;
}

void dm_bluesim_close (struct bdbm_drv_info* bdi)
{
	if (!nandsim_init)
		return;

	main_program_finished = 1;

	bdbm_msg ("wait for a worker thread to finish");
	kthread_stop (kid_worker);
	wait_for_completion_timeout(&worker_completion, msecs_to_jiffies(2000));

	/*
	if (!bsim_relay_running) {
		printk("TestProgram::pa_exit terminate main program\n");
		main_program_finished = 1;
		printk("[%s:%d] - up\n", __FUNCTION__, __LINE__);
		up(&bsim_start); // in case host never starts
	}
	*/
	misc_deregister (&miscdev);
}

uint64_t __get_page_offset (
	struct nand_params* np, 
	uint64_t channel_no,
	uint64_t chip_no,
	uint64_t block_no,
	uint64_t page_no)
{
	uint64_t page_offset = 0;
	uint64_t page_size = np->page_main_size + np->page_oob_size;
	uint64_t block_size = page_size * np->nr_pages_per_block;
	uint64_t chip_size = block_size * np->nr_blocks_per_chip;
	uint64_t channel_size = chip_size * np->nr_chips_per_channel;

	page_offset += channel_size * channel_no;
	page_offset += chip_size * chip_no;
	page_offset += block_size * block_no;
	page_offset += page_size * page_no;

	return page_offset;
}

uint64_t __get_block_offset (
	struct nand_params* np, 
	uint64_t channel_no,
	uint64_t chip_no,
	uint64_t block_no)
{
	uint64_t block_offset = 0;
	uint64_t page_size = np->page_main_size + np->page_oob_size;
	uint64_t block_size = page_size * np->nr_pages_per_block;
	uint64_t chip_size = block_size * np->nr_blocks_per_chip;
	uint64_t channel_size = chip_size * np->nr_chips_per_channel;

	block_offset += channel_size * channel_no;
	block_offset += chip_size * chip_no;
	block_offset += block_size * block_no;

	return block_offset;
}

uint32_t dm_bluesim_make_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* ptr_llm_req)
{
	struct nand_params* np = BDBM_GET_NAND_PARAMS (bdi);
	uint8_t* dmabuf = (uint8_t*)srcBuffer;
	uint64_t addr;

	/* flush dcache */
	/*portalDCacheFlushInval(srcAlloc, alloc_sz, srcBuffer);*/

	/* send a request to bluesime according to its type */
	switch (ptr_llm_req->req_type) {
	case REQTYPE_WRITE:
	case REQTYPE_GC_WRITE:
		addr = __get_page_offset (np, 
			ptr_llm_req->phyaddr.channel_no,
			ptr_llm_req->phyaddr.chip_no,
			ptr_llm_req->phyaddr.block_no,
			ptr_llm_req->phyaddr.page_no);
		bdbm_msg ("WRITE => addr: %llu, (%llu, %llu, %llu, %llu)", 
			addr,
			ptr_llm_req->phyaddr.channel_no,
			ptr_llm_req->phyaddr.chip_no,
			ptr_llm_req->phyaddr.block_no,
			ptr_llm_req->phyaddr.page_no);
		memcpy (dmabuf, ptr_llm_req->pptr_kpgs[0], KERNEL_PAGE_SIZE);
		memcpy (dmabuf + KERNEL_PAGE_SIZE, ptr_llm_req->ptr_oob, 64);
		NandSimRequestProxy_startWrite (&intarr[3], ref_srcAlloc, 0, addr, KERNEL_PAGE_SIZE + 64, 1);
		sem_wait(&test_sem);
		break;
	case REQTYPE_READ:
	case REQTYPE_GC_READ:
		addr = __get_page_offset (np, 
			ptr_llm_req->phyaddr.channel_no,
			ptr_llm_req->phyaddr.chip_no,
			ptr_llm_req->phyaddr.block_no,
			ptr_llm_req->phyaddr.page_no);
		bdbm_msg ("READ => addr: %llu, (%llu, %llu, %llu, %llu)", 
			addr,
			ptr_llm_req->phyaddr.channel_no,
			ptr_llm_req->phyaddr.chip_no,
			ptr_llm_req->phyaddr.block_no,
			ptr_llm_req->phyaddr.page_no);
		NandSimRequestProxy_startRead (&intarr[3], ref_srcAlloc, 0, addr, KERNEL_PAGE_SIZE + 64, 1);
		sem_wait(&test_sem);
		memcpy (ptr_llm_req->pptr_kpgs[0], dmabuf, KERNEL_PAGE_SIZE);
		memcpy (ptr_llm_req->ptr_oob, dmabuf + KERNEL_PAGE_SIZE, 64);
		break;
	case REQTYPE_GC_ERASE:
		addr = __get_block_offset (np, 
			ptr_llm_req->phyaddr.channel_no,
			ptr_llm_req->phyaddr.chip_no,
			ptr_llm_req->phyaddr.block_no);
		bdbm_msg ("ERASE => addr: %llu, (%llu, %llu, %llu, %llu)", 
			addr,
			ptr_llm_req->phyaddr.channel_no,
			ptr_llm_req->phyaddr.chip_no,
			ptr_llm_req->phyaddr.block_no,
			ptr_llm_req->phyaddr.page_no);
		NandSimRequestProxy_startErase (&intarr[3], addr, KERNEL_PAGE_SIZE + 64);
		sem_wait(&test_sem);
		break;
	case REQTYPE_READ_DUMMY:
		break;
	default:
		break;
	}

	bdbm_msg ("DONE!");

	/* return resp */
	dm_bluesim_end_req (bdi, ptr_llm_req);
	return 0;
}

void dm_bluesim_end_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* ptr_llm_req)
{
	bdi->ptr_llm_inf->end_req (bdi, ptr_llm_req);
}

