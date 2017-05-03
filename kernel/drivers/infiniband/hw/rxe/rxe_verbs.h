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
 *	   Redistribution and use in source and binary forms, with or
 *	   without modification, are permitted provided that the following
 *	   conditions are met:
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

#ifndef RXE_VERBS_H
#define RXE_VERBS_H

#include <linux/interrupt.h>
#include "rxe_pool.h"
#include "rxe_task.h"

static inline int pkey_match(u16 key1, u16 key2)
{
	return (((key1 & 0x7fff) != 0) &&
		((key1 & 0x7fff) == (key2 & 0x7fff)) &&
		((key1 & 0x8000) || (key2 & 0x8000))) ? 1 : 0;
}

/* Return >0 if psn_a > psn_b
 *	   0 if psn_a == psn_b
 *	  <0 if psn_a < psn_b
 */
static inline int psn_compare(u32 psn_a, u32 psn_b)
{
	s32 diff;

	diff = (psn_a - psn_b) << 8;
	return diff;
}

struct rxe_ucontext {
	struct rxe_pool_entry	pelem;
	struct ib_ucontext	ibuc;
};

struct rxe_pd {
	struct rxe_pool_entry	pelem;
	struct ib_pd		ibpd;
};

#define RXE_LL_ADDR_LEN		(16)

struct rxe_av {
	struct ib_ah_attr	attr;
	u8			ll_addr[RXE_LL_ADDR_LEN];
	u8			network_type;
	union {
		struct sockaddr		_sockaddr;
		struct sockaddr_in	_sockaddr_in;
		struct sockaddr_in6	_sockaddr_in6;
	} sgid_addr, dgid_addr;
};

struct rxe_ah {
	struct rxe_pool_entry	pelem;
	struct ib_ah		ibah;
	struct rxe_pd		*pd;
	struct rxe_av		av;
};

struct rxe_cqe {
	union {
		struct ib_wc		ibwc;
		struct ib_uverbs_wc	uibwc;
	};
};

struct rxe_cq {
	struct rxe_pool_entry	pelem;
	struct ib_cq		ibcq;
	struct rxe_queue	*queue;
	spinlock_t		cq_lock; /* cq lock */
	u8			notify;
	u8			special;
	int			is_user;
	struct tasklet_struct	comp_task;
};

struct rxe_dma_info {
	__u32			length;
	__u32			resid;
	__u32			cur_sge;
	__u32			num_sge;
	__u32			sge_offset;
	union {
		u8		inline_data[0];
		struct ib_sge	sge[0];
	};
};

enum wqe_state {
	wqe_state_posted,
	wqe_state_processing,
	wqe_state_pending,
	wqe_state_done,
	wqe_state_error,
};

/* must match corresponding data structure in librxe */
struct rxe_send_wqe {
	struct ib_send_wr	ibwr;
	struct rxe_av		av;	/* UD only */
	u32			status;
	u32			state;
	u64			iova;
	u32			mask;
	u32			first_psn;
	u32			last_psn;
	u32			ack_length;
	u32			ssn;
	u32			has_rd_atomic;
	struct rxe_dma_info	dma;	/* must go last */
};

struct rxe_recv_wqe {
	__u64			wr_id;
	__u32			num_sge;
	__u32			padding;
	struct rxe_dma_info	dma;
};

struct rxe_sq {
	int			max_wr;
	int			max_sge;
	int			max_inline;
	struct ib_wc		next_wc;
	spinlock_t		sq_lock; /* sq lock */
	struct rxe_queue	*queue;
};

struct rxe_rq {
	int			max_wr;
	int			max_sge;
	struct ib_wc		next_wc;
	spinlock_t		producer_lock; /* producer lock */
	spinlock_t		consumer_lock; /* consumer lock */
	struct rxe_queue	*queue;
};

struct rxe_srq {
	struct rxe_pool_entry	pelem;
	struct ib_srq		ibsrq;
	struct rxe_pd		*pd;
	struct rxe_cq		*cq;
	struct rxe_rq		rq;
	u32			srq_num;

	void			(*event_handler)(
					struct ib_event *, void *);
	void			*context;

	int			limit;
	int			error;
};

enum rxe_qp_state {
	QP_STATE_RESET,
	QP_STATE_INIT,
	QP_STATE_READY,
	QP_STATE_DRAIN,		/* req only */
	QP_STATE_DRAINED,	/* req only */
	QP_STATE_ERROR
};

extern char *rxe_qp_state_name[];

struct rxe_req_info {
	enum rxe_qp_state	state;
	int			wqe_index;
	u32			psn;
	int			opcode;
	atomic_t		rd_atomic;
	int			wait_fence;
	int			need_rd_atomic;
	int			wait_psn;
	int			need_retry;
	int			noack_pkts;
	struct rxe_task		task;
};

struct rxe_comp_info {
	u32			psn;
	int			opcode;
	int			timeout;
	int			timeout_retry;
	u32			retry_cnt;
	u32			rnr_retry;
	struct rxe_task		task;
};

enum rdatm_res_state {
	rdatm_res_state_next,
	rdatm_res_state_new,
	rdatm_res_state_replay,
};

struct resp_res {
	int			type;
	u32			first_psn;
	u32			last_psn;
	u32			cur_psn;
	enum rdatm_res_state	state;

	union {
		struct {
			struct sk_buff	*skb;
		} atomic;
		struct {
			struct rxe_mem	*mr;
			u64		va_org;
			u32		rkey;
			u32		length;
			u64		va;
			u32		resid;
		} read;
	};
};

struct rxe_resp_info {
	enum rxe_qp_state	state;
	u32			msn;
	u32			psn;
	int			opcode;
	int			drop_msg;
	int			goto_error;
	int			sent_psn_nak;
	enum ib_wc_status	status;
	u8			aeth_syndrome;

	/* Receive only */
	struct rxe_recv_wqe	*wqe;

	/* RDMA read / atomic only */
	u64			va;
	struct rxe_mem		*mr;
	u32			resid;
	u32			rkey;
	u64			atomic_orig;

	/* SRQ only */
	struct {
		struct rxe_recv_wqe	wqe;
		struct ib_sge		sge[RXE_MAX_SGE];
	} srq_wqe;

	/* Responder resources. It's a circular list where the oldest
	 * resource is dropped first. */
	struct resp_res		*resources;
	unsigned int		res_head;
	unsigned int		res_tail;
	struct resp_res		*res;
	struct rxe_task		task;
	u32 			rnr_nak_cur_psn;
	u32 			rnr_nak_first_psn;
	u32 			rnr_nak_last_psn;
	int 			sent_psn_rnr_nak;
};

struct rxe_qp {
	struct rxe_pool_entry	pelem;
	struct ib_qp		ibqp;
	struct ib_qp_attr	attr;
	unsigned int		valid;
	unsigned int		mtu;
	int			is_user;

	struct rxe_pd		*pd;
	struct rxe_srq		*srq;
	struct rxe_cq		*scq;
	struct rxe_cq		*rcq;

	enum ib_sig_type	sq_sig_type;

	struct rxe_sq		sq;
	struct rxe_rq		rq;

	struct rxe_av		pri_av;
	struct rxe_av		alt_av;

	/* list of mcast groups qp has joined
	   (for cleanup) */
	struct list_head	grp_list;
	spinlock_t		grp_lock; /* grp lock */

	struct sk_buff_head	req_pkts;
	struct sk_buff_head	resp_pkts;
	struct sk_buff_head	send_pkts;

	struct rxe_req_info	req;
	struct rxe_comp_info	comp;
	struct rxe_resp_info	resp;

	struct ib_udata		*udata;

	struct list_head	arbiter_list;
	atomic_t		ssn;
	atomic_t		req_skb_in;
	atomic_t		resp_skb_in;
	atomic_t		req_skb_out;
	atomic_t		resp_skb_out;
	int			need_req_skb;

	/* Timer for retranmitting packet when ACKs have been lost. RC
	 * only. The requester sets it when it is not already
	 * started. The responder resets it whenever an ack is
	 * received. */
	struct timer_list retrans_timer;
	u64 qp_timeout_jiffies;

	/* Timer for handling RNR NAKS. */
	struct timer_list rnr_nak_timer;

	spinlock_t		state_lock; /* state lock */
};

enum rxe_mem_state {
	RXE_MEM_STATE_ZOMBIE,
	RXE_MEM_STATE_INVALID,
	RXE_MEM_STATE_FREE,
	RXE_MEM_STATE_VALID,
};

enum rxe_mem_type {
	RXE_MEM_TYPE_NONE,
	RXE_MEM_TYPE_DMA,
	RXE_MEM_TYPE_MR,
	RXE_MEM_TYPE_FMR,
	RXE_MEM_TYPE_MW,
};

#define RXE_BUF_PER_MAP		(PAGE_SIZE/sizeof(struct ib_phys_buf))

struct rxe_map {
	struct ib_phys_buf	buf[RXE_BUF_PER_MAP];
};

struct rxe_mem {
	struct rxe_pool_entry	pelem;
	union {
		struct ib_mr		ibmr;
		struct ib_fmr		ibfmr;
		struct ib_mw		ibmw;
	};

	struct rxe_pd		*pd;
	struct ib_umem		*umem;

	u32			lkey;
	u32			rkey;

	enum rxe_mem_state	state;
	enum rxe_mem_type	type;
	u64			va;
	u64			iova;
	size_t			length;
	u32			offset;
	int			access;

	int			page_shift;
	int			page_mask;
	int			map_shift;
	int			map_mask;

	u32			num_buf;

	u32			max_buf;
	u32			num_map;

	struct rxe_map		**map;
};

struct rxe_fast_reg_page_list {
	struct ib_fast_reg_page_list	ibfrpl;
};

struct rxe_mc_grp {
	struct rxe_pool_entry	pelem;
	spinlock_t		mcg_lock; /* mcg lock */
	struct rxe_dev		*rxe;
	struct list_head	qp_list;
	union ib_gid		mgid;
	int			num_qp;
	u32			qkey;
	u16			mlid;
	u16			pkey;
};

struct rxe_mc_elem {
	struct rxe_pool_entry	pelem;
	struct list_head	qp_list;
	struct list_head	grp_list;
	struct rxe_qp		*qp;
	struct rxe_mc_grp	*grp;
};

struct rxe_port {
	struct ib_port_attr	attr;
	u16			*pkey_tbl;
	__be64			*guid_tbl;
	__be64			subnet_prefix;

	/* rate control */
	/* TODO */

	spinlock_t		port_lock; /* port lock */

	unsigned int		mtu_cap;

	/* special QPs */
	u32			qp_smi_index;
	u32			qp_gsi_index;
};

struct rxe_arbiter {
	struct rxe_task		task;
	struct list_head	qp_list;
	spinlock_t		list_lock; /* list lock */
	struct timer_list	timer;
	int			skb_count;
	int			queue_stalled;
};

/* callbacks from ib_rxe to network interface layer */
struct rxe_ifc_ops {
	void (*release)(struct rxe_dev *rxe);
	__be64 (*node_guid)(struct rxe_dev *rxe);
	__be64 (*port_guid)(struct rxe_dev *rxe, unsigned int port_num);
	struct device *(*dma_device)(struct rxe_dev *rxe);
	int (*mcast_add)(struct rxe_dev *rxe, union ib_gid *mgid);
	int (*mcast_delete)(struct rxe_dev *rxe, union ib_gid *mgid);
	int (*send)(struct rxe_dev *rxe, struct sk_buff *skb);
	int (*loopback)(struct sk_buff *skb);
	struct sk_buff *(*init_packet)(struct rxe_dev *rxe, struct rxe_av *av,
				       int paylen);
	int (*init_av)(struct rxe_dev *rxe, struct ib_ah_attr *attr,
		       struct rxe_av *av);
	char *(*parent_name)(struct rxe_dev *rxe, unsigned int port_num);
	enum rdma_link_layer (*link_layer)(struct rxe_dev *rxe,
					   unsigned int port_num);
};

struct rxe_dev {
	struct ib_device	ib_dev;
	struct ib_device_attr	attr;
	int			max_ucontext;
	int			max_inline_data;
	struct kref		ref_cnt;

	struct rxe_ifc_ops	*ifc_ops;

	struct net_device	*ndev;

	struct rxe_arbiter	arbiter;

	atomic_t		ind;

	atomic_t		req_skb_in;
	atomic_t		resp_skb_in;
	atomic_t		req_skb_out;
	atomic_t		resp_skb_out;

	int			xmit_errors;

	struct rxe_pool		uc_pool;
	struct rxe_pool		pd_pool;
	struct rxe_pool		ah_pool;
	struct rxe_pool		srq_pool;
	struct rxe_pool		qp_pool;
	struct rxe_pool		cq_pool;
	struct rxe_pool		mr_pool;
	struct rxe_pool		mw_pool;
	struct rxe_pool		fmr_pool;
	struct rxe_pool		mc_grp_pool;
	struct rxe_pool		mc_elem_pool;

	spinlock_t		pending_lock; /* pending lock */
	struct list_head	pending_mmaps;

	spinlock_t		mmap_offset_lock; /* mmap offset lock */
	int			mmap_offset;

	u8			num_ports;
	struct rxe_port		*port;
	u64 watermark_depth; // roce added
	u64 line_speed; // roce added
	u64 line_accumulation_per_jiffy; // roce added
	u64 burst_data_len; // roce added
	u64 last_known_jiffy; // roce added
	u64 control_method; // roce added (1 -Bandwidh, 2 - Watermark, 0 -None ,others --ignore and use as none)
};

static inline struct rxe_dev *to_rdev(struct ib_device *dev)
{
	return dev ? container_of(dev, struct rxe_dev, ib_dev) : NULL;
}

static inline struct rxe_ucontext *to_ruc(struct ib_ucontext *uc)
{
	return uc ? container_of(uc, struct rxe_ucontext, ibuc) : NULL;
}

static inline struct rxe_pd *to_rpd(struct ib_pd *pd)
{
	return pd ? container_of(pd, struct rxe_pd, ibpd) : NULL;
}

static inline struct rxe_ah *to_rah(struct ib_ah *ah)
{
	return ah ? container_of(ah, struct rxe_ah, ibah) : NULL;
}

static inline struct rxe_srq *to_rsrq(struct ib_srq *srq)
{
	return srq ? container_of(srq, struct rxe_srq, ibsrq) : NULL;
}

static inline struct rxe_qp *to_rqp(struct ib_qp *qp)
{
	return qp ? container_of(qp, struct rxe_qp, ibqp) : NULL;
}

static inline struct rxe_cq *to_rcq(struct ib_cq *cq)
{
	return cq ? container_of(cq, struct rxe_cq, ibcq) : NULL;
}

static inline struct rxe_mem *to_rmr(struct ib_mr *mr)
{
	return mr ? container_of(mr, struct rxe_mem, ibmr) : NULL;
}

static inline struct rxe_mem *to_rfmr(struct ib_fmr *fmr)
{
	return fmr ? container_of(fmr, struct rxe_mem, ibfmr) : NULL;
}

static inline struct rxe_mem *to_rmw(struct ib_mw *mw)
{
	return mw ? container_of(mw, struct rxe_mem, ibmw) : NULL;
}

static inline struct rxe_fast_reg_page_list *to_rfrpl(
	struct ib_fast_reg_page_list *frpl)
{
	return frpl ? container_of(frpl,
		struct rxe_fast_reg_page_list, ibfrpl) : NULL;
}

int rxe_register_device(struct rxe_dev *rxe);
int rxe_unregister_device(struct rxe_dev *rxe);

void rxe_mc_cleanup(void *arg);

#endif /* RXE_VERBS_H */
