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
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched.h>

#include "debug.h"
#include "platform.h"
#include "params.h"
#include "bdbm_drv.h"
#include "llm_mq2.h"
#include "queue/queue.h"

/*#define ENABLE_SEQ_DBG*/


/* llm interface */
struct bdbm_llm_inf_t _llm_mq_inf = {
	.ptr_private = NULL,
	.create = llm_mq_create,
	.destroy = llm_mq_destroy,
	.make_req = llm_mq_make_req,
	.flush = llm_mq_flush,
	.end_req = llm_mq_end_req,
};

/* private */
struct bdbm_llm_mq_private {
	uint64_t nr_punits;
	bdbm_completion* punit_locks;
	struct bdbm_queue_t* q;

	/* for thread management */
	bdbm_completion llm_thread_done;
#ifdef ENABLE_SEQ_DBG
	bdbm_completion dbg_seq;
#endif
	struct task_struct* llm_thread;
	wait_queue_head_t llm_wq;
};


int __llm_mq_thread (void* arg)
{
	struct bdbm_drv_info* bdi = (struct bdbm_drv_info*)arg;
	struct bdbm_llm_mq_private* p = (struct bdbm_llm_mq_private*)BDBM_LLM_PRIV(bdi);
	DECLARE_WAITQUEUE (wait, current);
	uint64_t loop;
	void* ptr_req;

	bdbm_daemonize ("llm_mq_thread");
	allow_signal (SIGKILL); 

	add_wait_queue (&p->llm_wq, &wait);

	bdbm_wait_for_completion (p->llm_thread_done);
	bdbm_reinit_completion (p->llm_thread_done);

	for (;;) {
		set_current_state (TASK_INTERRUPTIBLE);

		/* send reqs until Q becomes empty */
		if (bdbm_queue_is_all_empty (p->q)) {
			schedule();	/* sleep */
			if (signal_pending (current)) {
				/* SIGKILL */
				break;
			}
		}

		set_current_state (TASK_RUNNING);

		for (loop = 0; loop < p->nr_punits; loop++) {
			/* if nothing is in Q, then go to the next punit */
			if (bdbm_queue_is_empty (p->q, loop))
				continue;

			/* if pu is busy, then go to the next pnit */
			if (!bdbm_try_wait_for_completion (p->punit_locks[loop]))
				continue;

			/* send req to dm */
			bdbm_reinit_completion (p->punit_locks[loop]);
			if ((ptr_req = bdbm_queue_dequeue (p->q, loop)) == NULL) {
				bdbm_complete (p->punit_locks[loop]);
				continue;
			}

			if (bdi->ptr_dm_inf->make_req (bdi, (struct bdbm_llm_req_t*)ptr_req)) {
				bdbm_complete (p->punit_locks[loop]);
				/* TODO: I do not check whether it works well or not */
				bdi->ptr_llm_inf->end_req (bdi, (struct bdbm_llm_req_t*)ptr_req);
				bdbm_warning ("oops! make_req failed");
			}
		}
	}

	set_current_state (TASK_RUNNING);
	remove_wait_queue (&p->llm_wq, &wait);

	bdbm_complete (p->llm_thread_done);

	return 0;
}

void __llm_mq_create_fail (struct bdbm_llm_mq_private* p)
{
	if (!p) 
		return;
	if (p->punit_locks)
		bdbm_free_atomic (p->punit_locks);
	if (p->q)
		bdbm_queue_destroy (p->q);
	bdbm_free_atomic (p);
}

uint32_t llm_mq_create (struct bdbm_drv_info* bdi)
{
	struct bdbm_llm_mq_private* p;
	uint64_t loop;

	/* create a private info for llm_nt */
	if ((p = (struct bdbm_llm_mq_private*)bdbm_malloc_atomic
			(sizeof (struct bdbm_llm_mq_private))) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		return -1;
	}

	/* get the total number of parallel units */
	p->nr_punits =
		bdi->ptr_bdbm_params->nand.nr_channels *
		bdi->ptr_bdbm_params->nand.nr_chips_per_channel;

	/* create queue */
	if ((p->q = bdbm_queue_create (p->nr_punits, INFINITE_QUEUE)) == NULL) {
		bdbm_error ("bdbm_queue_create failed");
		__llm_mq_create_fail (p);
		return -1;
	}

	/* create completion locks for parallel units */
	if ((p->punit_locks = (bdbm_completion*)bdbm_malloc_atomic
			(sizeof (bdbm_completion) * p->nr_punits)) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		__llm_mq_create_fail (p);
		return -1;
	}
	for (loop = 0; loop < p->nr_punits; loop++) {
		bdbm_init_completion (p->punit_locks[loop]);
		bdbm_complete (p->punit_locks[loop]);
	}

	/* keep the private structures for llm_nt */
	bdi->ptr_llm_inf->ptr_private = (void*)p;

	/* create & run a thread */
	bdbm_init_completion (p->llm_thread_done);
	bdbm_complete (p->llm_thread_done);
	init_waitqueue_head (&p->llm_wq);
	if ((p->llm_thread = kthread_create (
			__llm_mq_thread, bdi, "__llm_mq_thread")) == NULL) {
		bdbm_error ("kthread_create failed");
		__llm_mq_create_fail (p);
		return -1;
	}
	wake_up_process (p->llm_thread);

#ifdef ENABLE_SEQ_DBG
	bdbm_init_completion (p->dbg_seq);
	bdbm_complete (p->dbg_seq);
#endif

	return 0;
}

/* NOTE: we assume that all of the host requests are completely served.
 * the host adapter must be first closed before this function is called.
 * if not, it would work improperly. */
void llm_mq_destroy (struct bdbm_drv_info* bdi)
{
	uint64_t loop;
	struct bdbm_llm_mq_private* p = (struct bdbm_llm_mq_private*)BDBM_LLM_PRIV(bdi);

	/* wait until Q becomes empty */
	while (!bdbm_queue_is_all_empty (p->q)) {
		msleep (1);
	}

	/* kill kthread */
	send_sig (SIGKILL, p->llm_thread, 0);
	bdbm_wait_for_completion (p->llm_thread_done);

	for (loop = 0; loop < p->nr_punits; loop++) {
		bdbm_wait_for_completion (p->punit_locks[loop]);
	}

	/* release all the relevant data structures */
	bdbm_queue_destroy (p->q);
	bdbm_free_atomic (p);
}

uint32_t llm_mq_make_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* llm_req)
{
	uint32_t ret;
	uint64_t punit_id;
	struct bdbm_llm_mq_private* p = (struct bdbm_llm_mq_private*)BDBM_LLM_PRIV(bdi);

#ifdef ENABLE_SEQ_DBG
	bdbm_wait_for_completion (p->dbg_seq);
	bdbm_reinit_completion (p->dbg_seq);
#endif

	/* get a parallel unit ID */
	punit_id = llm_req->phyaddr->channel_no * 
		BDBM_GET_NAND_PARAMS (bdi)->nr_chips_per_channel +
		llm_req->phyaddr->chip_no;

	while (bdbm_queue_get_nr_items (p->q) >= 256) {
		yield ();
	}

	/* put a request into Q */
	if ((ret = bdbm_queue_enqueue (p->q, punit_id, (void*)llm_req))) {
		bdbm_msg ("bdbm_queue_enqueue failed");
	}

	/* wake up thread if it sleeps */
	wake_up_interruptible (&p->llm_wq); 

	return ret;
}

void llm_mq_flush (struct bdbm_drv_info* bdi)
{
	struct bdbm_llm_mq_private* llm_p = (struct bdbm_llm_mq_private*)BDBM_LLM_PRIV(bdi);

	while (bdbm_queue_is_all_empty (llm_p->q) != 1) {
		cond_resched ();
	}
}

void llm_mq_end_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* llm_req)
{
	uint64_t punit_id;
	struct bdbm_llm_mq_private* p = (struct bdbm_llm_mq_private*)BDBM_LLM_PRIV(bdi);

	switch (llm_req->req_type) {
	case REQTYPE_RMW_READ:
		/* get a parallel unit ID */
		punit_id = llm_req->phyaddr->channel_no * 
			BDBM_GET_NAND_PARAMS (bdi)->nr_chips_per_channel +
			llm_req->phyaddr->chip_no;
		bdbm_complete (p->punit_locks[punit_id]);

		/* change its type to WRITE if req_type is RMW */
		llm_req->phyaddr = &llm_req->phyaddr_w;
		llm_req->req_type = REQTYPE_RMW_WRITE;

		/*bdbm_msg ("REQTYPE_RMW_WRITE");*/
		punit_id = llm_req->phyaddr->channel_no * 
			BDBM_GET_NAND_PARAMS (bdi)->nr_chips_per_channel +
			llm_req->phyaddr->chip_no;

		/* put it to Q again */
		if (bdbm_queue_enqueue_top (p->q, punit_id, (void*)llm_req)) {
			bdbm_msg ("bdbm_queue_enqueue failed");
			bdbm_bug_on (1);
		}

		/* wake up thread if it sleeps */
		wake_up_interruptible (&p->llm_wq); 
		break;

	case REQTYPE_READ:
	case REQTYPE_READ_DUMMY:
	case REQTYPE_WRITE:
	case REQTYPE_RMW_WRITE:
	case REQTYPE_GC_READ:
	case REQTYPE_GC_WRITE:
	case REQTYPE_GC_ERASE:
	case REQTYPE_TRIM:
		/* get a parallel unit ID */
		punit_id = llm_req->phyaddr->channel_no * 
			BDBM_GET_NAND_PARAMS (bdi)->nr_chips_per_channel +
			llm_req->phyaddr->chip_no;
		/* complete a lock */
		bdbm_complete (p->punit_locks[punit_id]);
		/* finish a request */
		bdi->ptr_hlm_inf->end_req (bdi, llm_req);

#ifdef ENABLE_SEQ_DBG
		bdbm_complete (p->dbg_seq);          
#endif
		break;

	default:
		bdbm_error ("invalid req-type (%u)", llm_req->req_type);
		break;
	}
}

