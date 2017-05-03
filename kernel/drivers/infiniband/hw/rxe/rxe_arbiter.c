/*
 * Copyright (c) 2009-2011 Mellanox Technologies Ltd. All rights reserved.
 * Copyright (c) 2009-2011 System Fabric Works, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/skbuff.h>

#include "rxe.h"
#include "rxe_loc.h"

static inline void account_skb(struct rxe_dev *rxe, struct rxe_qp *qp,
			       int is_request)
{
	if (is_request & RXE_REQ_MASK) {
		atomic_dec(&rxe->req_skb_out);
		atomic_dec(&qp->req_skb_out);
		if (qp->need_req_skb) {
			if (atomic_read(&qp->req_skb_out) <
					RXE_MAX_INFLIGHT_SKBS_PER_QP)
				rxe_run_task(&qp->req.task, 1);
		}
	} else {
		atomic_dec(&rxe->resp_skb_out);
		atomic_dec(&qp->resp_skb_out);
	}
}

static int xmit_one_packet(struct rxe_dev *rxe, struct rxe_qp *qp,
			   struct sk_buff *skb)
{
	int err;
	struct rxe_pkt_info *pkt = SKB_TO_PKT(skb);
	int is_request = pkt->mask & RXE_REQ_MASK;

	/* drop pkt if qp is in wrong state to send */
	if (!qp->valid)
		goto drop;

	if (is_request) {
		if (qp->req.state != QP_STATE_READY)
			goto drop;
	} else {
		if (qp->resp.state != QP_STATE_READY)
			goto drop;
	}

	if (pkt->mask & RXE_LOOPBACK_MASK)
		err = rxe->ifc_ops->loopback(skb);
	else
		err = rxe->ifc_ops->send(rxe, skb);

	if (err) {
		rxe->xmit_errors++;
		return err;
	}

	goto done;

drop:
	kfree_skb(skb);
	err = 0;
done:
	account_skb(rxe, qp, is_request);
	return err;
}


/*ROCE PRIV CODE START*/

#ifndef __u64
typedef uint64_t __u64;
#endif

#include<../drivers/staging/fsl-dpaa2/ethernet/dpaa2-eth.h>
/*ROCE PRIV CODE END*/



/*
 * choose one packet for sending
 */
int rxe_arbiter(void *arg)
{
	int err;
	unsigned long flags;
	struct rxe_dev *rxe = (struct rxe_dev *)arg;
	struct sk_buff *skb;
	struct list_head *qpl;
	struct rxe_qp *qp;
	int status = 0;


	/* get the next qp's send queue */
	spin_lock_irqsave(&rxe->arbiter.list_lock, flags);
	if (list_empty(&rxe->arbiter.qp_list)) {
		spin_unlock_irqrestore(&rxe->arbiter.list_lock, flags);
		return 1;
	}

	qpl = rxe->arbiter.qp_list.next;
	list_del_init(qpl);
	qp = list_entry(qpl, struct rxe_qp, arbiter_list);
	spin_unlock_irqrestore(&rxe->arbiter.list_lock, flags);

	/* get next packet from queue and try to send it
	   note skb could have already been removed */
	skb = skb_dequeue(&qp->send_pkts);
	if (skb) {
		struct rxe_pkt_info *pkt = SKB_TO_PKT(skb);

		err = xmit_one_packet(rxe, qp, skb);

		/*
		BUG: When udp tunnel queuing fails then on the next run of 
		rxe_arbiter is run from rxe_arbiter_timer() which was set sometime back in past and
		list_empty(&rxe->arbiter.qp_list) will be true in this run , because
		list_add_tail(qpl, &rxe->arbiter.qp_list) is not called on failure
		
		This halts scheduling of arbiter because of two reasons
		1. Timer is not set from now , so rxe_arbiter_timer() will not run again , so 
		   rxe_arbiter is not scheduled
		2. arbiter_skb_queue will schedule arbiter only if arbiter.skb_count is made to 1 from 0,
		   which is not going to occur.
		*/

		/*
		Soft_RoCE_MOD: (FIXING THE BUG) 
	
		1. Perform list_add_tail(qpl, &rxe->arbiter.qp_list) which
		   allows to make qp_list non empty and arbiter will definitely
		   attempt to queue skb to udp_tunel 
		2. set timer which calls rxe_arbiter_timer() on timeout
		
		   Till the next timer is fired udp tunnel queue mostly should have space
		   to accomodate new SKB, and keep on trying on successive rxe_arbiter()
		   scheduled by timer
		*/
		if (err) {
			skb_queue_head(&qp->send_pkts, skb);
			/*rxe_run_task(&rxe->arbiter.task, 1);*/
			/*return 1;*/
			status = 1;
		}

		else {

			#if  0
			__u64 tx_queued_packets;
			__u64 tx_confirmed_packets;
			#endif
			switch(rxe->control_method) {

				case 2:
					#if 0
					tx_queued_packets = dpaa_tx_packets_cnt(rxe->ndev);
			 		tx_confirmed_packets = dpaa_tx_confirmed_cnt(rxe->ndev);
					#endif

					//printk("using watermark\n");
					#if 0
					if((tx_queued_packets - tx_confirmed_packets) < rxe->watermark_depth) { // we are using line_accumulation_per_jiffy as 												  // queue length
						//printk("good: tx_queued_packets : %llu  tx_confirmed_packets : %llu ,  rxe->watermark_depth :%u\n",\
						//		tx_queued_packets,tx_confirmed_packets,rxe->watermark_depth);
						status = 0;	
					}
					else {
						//printk("bad: tx_queued_packets : %llu  tx_confirmed_packets : %llu ,  rxe->watermark_depth :%u\n",\
						//		tx_queued_packets,tx_confirmed_packets,rxe->watermark_depth);
						status = 1;
					}



				#endif
					break;

				case 1:
					//printk("using bandwidth\n");
				#if 1

					if (jiffies > rxe->last_known_jiffy) {
						rxe->burst_data_len = skb->len; // reset burst_data_len
						rxe->last_known_jiffy = jiffies; // move to new jiffies
						status = 0;
					}
					else if (jiffies <  rxe->last_known_jiffy) {
						printk("reset at jiffies (overflow) : %lu  line acc : %llu last_known_jiffy :%llu bursted_till now :%llu bytes\n",
								jiffies,rxe->line_accumulation_per_jiffy,rxe->last_known_jiffy,rxe->burst_data_len);
						rxe->burst_data_len = skb->len; // reset burst_data_len
						rxe->last_known_jiffy = jiffies; // move to new jiffies
						status = 0;
					}
					else if((rxe->burst_data_len >= rxe->line_accumulation_per_jiffy)) {
						// we dont want to schedule again
						status = 1;
					}
					else {
						rxe->burst_data_len +=  skb->len; // add up burst_data_len
						status = 0;
					}
				#endif
					break;

				case 0:	
					//printk("using none\n");
					status = 0;
				
					break;
				default:
					printk("case default\n");
				
			}
		}


		if ((qp_type(qp) != IB_QPT_RC) &&
		    (pkt->mask & RXE_END_MASK)) {
			pkt->wqe->state = wqe_state_done;
			rxe_run_task(&qp->comp.task, 1);
		}
	}

	/* if more work in queue put qp back on the list */
	spin_lock_irqsave(&rxe->arbiter.list_lock, flags);

	/* Soft_RoCE_MOD: adding to qp_list  on failure case to get immediately scheduled on next tick */
	if (list_empty(qpl) && !skb_queue_empty(&qp->send_pkts))
		list_add_tail(qpl, &rxe->arbiter.qp_list);

	/* Soft_RoCE_MOD: Decrement the skb_count only on successful queuing to udp tunnel queue */
	if (skb && (status ==  0))
		rxe->arbiter.skb_count--;

	if (rxe->arbiter.skb_count &&
	    !timer_pending(&rxe->arbiter.timer))
		mod_timer(&rxe->arbiter.timer,
			  jiffies +
			  nsecs_to_jiffies(RXE_NSEC_ARB_TIMER_DELAY));


	spin_unlock_irqrestore(&rxe->arbiter.list_lock, flags);

	/*return 0;*/
	/*
	Soft_RoCE_MOD: Return the status, if skb_count is non zero rxe_arbiter_will 
	run from rxe_arbiter_timer 
	*/
	return status;
}

/*
 * queue a packet for sending from a qp
 */
void arbiter_skb_queue(struct rxe_dev *rxe, struct rxe_qp *qp,
		       struct sk_buff *skb)
{
	unsigned long flags;

	/* add packet to send queue */
	skb_queue_tail(&qp->send_pkts, skb);

	/* if not already there add qp to arbiter list */
	spin_lock_irqsave(&rxe->arbiter.list_lock, flags);
	if (list_empty(&qp->arbiter_list))
		list_add_tail(&qp->arbiter_list, &rxe->arbiter.qp_list);

	if (rxe->arbiter.skb_count)
		rxe->arbiter.skb_count++;
	else {
		rxe->arbiter.skb_count++;
		rxe_run_task(&rxe->arbiter.task, 1);
	}

	spin_unlock_irqrestore(&rxe->arbiter.list_lock, flags);
}

void rxe_arbiter_timer(unsigned long arg)
{
	struct rxe_dev *rxe = (struct rxe_dev *)arg;

	rxe_run_task(&rxe->arbiter.task, 0);
}
