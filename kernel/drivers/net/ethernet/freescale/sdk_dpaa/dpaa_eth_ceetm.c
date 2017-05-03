/* Copyright 2008-2016 Freescale Semiconductor Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/init.h>
#include "dpaa_eth_ceetm.h"

#define DPA_CEETM_DESCRIPTION "FSL DPAA CEETM qdisc"

const struct nla_policy ceetm_policy[TCA_CEETM_MAX + 1] = {
	[TCA_CEETM_COPT] = { .len = sizeof(struct tc_ceetm_copt) },
	[TCA_CEETM_QOPS] = { .len = sizeof(struct tc_ceetm_qopt) },
};

struct Qdisc_ops ceetm_qdisc_ops;

/* Obtain the DCP and the SP ids from the FMan port */
static void get_dcp_and_sp(struct net_device *dev, enum qm_dc_portal *dcp_id,
			   unsigned int *sp_id)
{
	uint32_t channel;
	t_LnxWrpFmPortDev *port_dev;
	struct dpa_priv_s *dpa_priv = netdev_priv(dev);
	struct mac_device *mac_dev = dpa_priv->mac_dev;

	port_dev = (t_LnxWrpFmPortDev *)mac_dev->port_dev[TX];
	channel = port_dev->txCh;

	*sp_id = channel & CHANNEL_SP_MASK;
	pr_debug(KBUILD_BASENAME " : FM sub-portal ID %d\n", *sp_id);

	if (channel < DCP0_MAX_CHANNEL) {
		*dcp_id = qm_dc_portal_fman0;
		pr_debug(KBUILD_BASENAME " : DCP ID 0\n");
	} else {
		*dcp_id = qm_dc_portal_fman1;
		pr_debug(KBUILD_BASENAME " : DCP ID 1\n");
	}
}

/* Enqueue Rejection Notification callback */
static void ceetm_ern(struct qman_portal *portal, struct qman_fq *fq,
		      const struct qm_mr_entry *msg)
{
	struct net_device *net_dev;
	struct ceetm_class *cls;
	struct ceetm_class_stats *cstats = NULL;
	const struct dpa_priv_s *dpa_priv;
	struct dpa_percpu_priv_s *dpa_percpu_priv;
	struct sk_buff *skb;
	struct qm_fd fd = msg->ern.fd;

	net_dev = ((struct ceetm_fq *)fq)->net_dev;
	dpa_priv = netdev_priv(net_dev);
	dpa_percpu_priv = raw_cpu_ptr(dpa_priv->percpu_priv);

	/* Increment DPA counters */
	dpa_percpu_priv->stats.tx_dropped++;
	dpa_percpu_priv->stats.tx_fifo_errors++;

	/* Increment CEETM counters */
	cls = ((struct ceetm_fq *)fq)->ceetm_cls;
	switch (cls->type) {
	case CEETM_PRIO:
		cstats = this_cpu_ptr(cls->prio.cstats);
		break;
	case CEETM_WBFS:
		cstats = this_cpu_ptr(cls->wbfs.cstats);
		break;
	}

	if (cstats)
		cstats->ern_drop_count++;

	if (fd.bpid != 0xff) {
		dpa_fd_release(net_dev, &fd);
		return;
	}

	skb = _dpa_cleanup_tx_fd(dpa_priv, &fd);
	dev_kfree_skb_any(skb);
}

/* Congestion State Change Notification callback */
static void ceetm_cscn(struct qm_ceetm_ccg *ccg, void *cb_ctx, int congested)
{
	struct ceetm_fq *ceetm_fq = (struct ceetm_fq *)cb_ctx;
	struct dpa_priv_s *dpa_priv = netdev_priv(ceetm_fq->net_dev);
	struct ceetm_class *cls = ceetm_fq->ceetm_cls;
	struct ceetm_class_stats *cstats = NULL;

	switch (cls->type) {
	case CEETM_PRIO:
		cstats = this_cpu_ptr(cls->prio.cstats);
		break;
	case CEETM_WBFS:
		cstats = this_cpu_ptr(cls->wbfs.cstats);
		break;
	}

	if (congested) {
		dpa_priv->cgr_data.congestion_start_jiffies = jiffies;
		netif_tx_stop_all_queues(dpa_priv->net_dev);
		dpa_priv->cgr_data.cgr_congested_count++;
		if (cstats)
			cstats->congested_count++;
	} else {
		dpa_priv->cgr_data.congested_jiffies +=
			(jiffies - dpa_priv->cgr_data.congestion_start_jiffies);
		netif_tx_wake_all_queues(dpa_priv->net_dev);
	}
}

/* Allocate a ceetm fq */
static int ceetm_alloc_fq(struct ceetm_fq **fq, struct net_device *dev,
			  struct ceetm_class *cls)
{
	*fq = kzalloc(sizeof(**fq), GFP_KERNEL);
	if (!*fq)
		return -ENOMEM;

	(*fq)->net_dev = dev;
	(*fq)->ceetm_cls = cls;
	return 0;
}

/* Configure a ceetm Class Congestion Group */
static int ceetm_config_ccg(struct qm_ceetm_ccg **ccg,
			    struct qm_ceetm_channel *channel, unsigned int id,
			    struct ceetm_fq *fq, u32 if_support)
{
	int err;
	u32 cs_th;
	u16 ccg_mask;
	struct qm_ceetm_ccg_params ccg_params;

	err = qman_ceetm_ccg_claim(ccg, channel, id, ceetm_cscn, fq);
	if (err)
		return err;

	/* Configure the count mode (frames/bytes), enable
	 * notifications, enable tail-drop, and configure the tail-drop
	 * mode and threshold */
	ccg_mask = QM_CCGR_WE_MODE | QM_CCGR_WE_CSCN_EN |
		   QM_CCGR_WE_TD_EN | QM_CCGR_WE_TD_MODE |
		   QM_CCGR_WE_TD_THRES;

	ccg_params.mode = 0; /* count bytes */
	ccg_params.cscn_en = 1; /* generate notifications */
	ccg_params.td_en = 1; /* enable tail-drop */
	ccg_params.td_mode = 1; /* tail-drop on threshold */

	/* Configure the tail-drop threshold according to the link
	 * speed */
	if (if_support & SUPPORTED_10000baseT_Full)
		cs_th = CONFIG_FSL_DPAA_CS_THRESHOLD_10G;
	else
		cs_th = CONFIG_FSL_DPAA_CS_THRESHOLD_1G;
	qm_cgr_cs_thres_set64(&ccg_params.td_thres, cs_th, 1);

	err = qman_ceetm_ccg_set(*ccg, ccg_mask, &ccg_params);
	if (err)
		return err;

	return 0;
}

/* Configure a ceetm Logical Frame Queue */
static int ceetm_config_lfq(struct qm_ceetm_cq *cq, struct ceetm_fq *fq,
			    struct qm_ceetm_lfq **lfq)
{
	int err;
	u64 context_a;
	u32 context_b;

	err = qman_ceetm_lfq_claim(lfq, cq);
	if (err)
		return err;

	/* Get the former contexts in order to preserve context B */
	err = qman_ceetm_lfq_get_context(*lfq, &context_a, &context_b);
	if (err)
		return err;

	context_a = CEETM_CONTEXT_A;
	err = qman_ceetm_lfq_set_context(*lfq, context_a, context_b);
	if (err)
		return err;

	(*lfq)->ern = ceetm_ern;

	err = qman_ceetm_create_fq(*lfq, &fq->fq);
	if (err)
		return err;

	return 0;
}

/* Configure a prio ceetm class */
static int ceetm_config_prio_cls(struct ceetm_class *cls,
				 struct net_device *dev,
				 struct qm_ceetm_channel *channel,
				 unsigned int id)
{
	int err;
	struct dpa_priv_s *dpa_priv = netdev_priv(dev);

	err = ceetm_alloc_fq(&cls->prio.fq, dev, cls);
	if (err)
		return err;

	/* Claim and configure the CCG */
	err = ceetm_config_ccg(&cls->prio.ccg, channel, id, cls->prio.fq,
			       dpa_priv->mac_dev->if_support);
	if (err)
		return err;

	/* Claim and configure the CQ */
	err = qman_ceetm_cq_claim(&cls->prio.cq, channel, id, cls->prio.ccg);
	if (err)
		return err;

	if (cls->shaped) {
		err = qman_ceetm_channel_set_cq_cr_eligibility(channel, id, 1);
		if (err)
			return err;

		err = qman_ceetm_channel_set_cq_er_eligibility(channel, id, 1);
		if (err)
			return err;
	}

	/* Claim and configure a LFQ */
	err = ceetm_config_lfq(cls->prio.cq, cls->prio.fq, &cls->prio.lfq);
	if (err)
		return err;

	return 0;
}

/* Configure a wbfs ceetm class */
static int ceetm_config_wbfs_cls(struct ceetm_class *cls,
				 struct net_device *dev,
				 struct qm_ceetm_channel *channel,
				 unsigned int id, int type)
{
	int err;
	struct dpa_priv_s *dpa_priv = netdev_priv(dev);

	err = ceetm_alloc_fq(&cls->wbfs.fq, dev, cls);
	if (err)
		return err;

	/* Claim and configure the CCG */
	err = ceetm_config_ccg(&cls->wbfs.ccg, channel, id, cls->wbfs.fq,
			       dpa_priv->mac_dev->if_support);
	if (err)
		return err;

	/* Claim and configure the CQ */
	if (type == WBFS_GRP_B)
		err = qman_ceetm_cq_claim_B(&cls->wbfs.cq, channel, id,
					    cls->wbfs.ccg);
	else
		err = qman_ceetm_cq_claim_A(&cls->wbfs.cq, channel, id,
					    cls->wbfs.ccg);
	if (err)
		return err;

	/* Configure the CQ weight: real number mutiplied by 100 to get rid
	 * of the fraction  */
	err = qman_ceetm_set_queue_weight_in_ratio(cls->wbfs.cq,
						   cls->wbfs.weight * 100);
	if (err)
		return err;

	/* Claim and configure a LFQ */
	err = ceetm_config_lfq(cls->wbfs.cq, cls->wbfs.fq, &cls->wbfs.lfq);
	if (err)
		return err;

	return 0;
}

/* Find class in qdisc hash table using given handle */
static inline struct ceetm_class *ceetm_find(u32 handle, struct Qdisc *sch)
{
	struct ceetm_qdisc *priv = qdisc_priv(sch);
	struct Qdisc_class_common *clc;

	pr_debug(KBUILD_BASENAME " : %s : find class %X in qdisc %X\n",
		 __func__, handle, sch->handle);

	clc = qdisc_class_find(&priv->clhash, handle);
	return clc ? container_of(clc, struct ceetm_class, common) : NULL;
}

/* Insert a class in the qdisc's class hash */
static void ceetm_link_class(struct Qdisc *sch,
			     struct Qdisc_class_hash *clhash,
			     struct Qdisc_class_common *common)
{
	sch_tree_lock(sch);
	qdisc_class_hash_insert(clhash, common);
	sch_tree_unlock(sch);
	qdisc_class_hash_grow(sch, clhash);
}

/* Destroy a ceetm class */
static void ceetm_cls_destroy(struct Qdisc *sch, struct ceetm_class *cl)
{
	if (!cl)
		return;

	pr_debug(KBUILD_BASENAME " : %s : destroy class %X from under %X\n",
		 __func__, cl->common.classid, sch->handle);

	switch (cl->type) {
	case CEETM_ROOT:
		if (cl->root.child) {
			qdisc_destroy(cl->root.child);
			cl->root.child = NULL;
		}

		if (cl->root.ch && qman_ceetm_channel_release(cl->root.ch))
			pr_err(KBUILD_BASENAME
			       " : %s : error releasing the channel %d\n",
			       __func__, cl->root.ch->idx);

		break;

	case CEETM_PRIO:
		if (cl->prio.child) {
			qdisc_destroy(cl->prio.child);
			cl->prio.child = NULL;
		}

		if (cl->prio.lfq && qman_ceetm_lfq_release(cl->prio.lfq))
			pr_err(KBUILD_BASENAME
			       " : %s : error releasing the LFQ %d\n",
			       __func__, cl->prio.lfq->idx);

		if (cl->prio.cq && qman_ceetm_cq_release(cl->prio.cq))
			pr_err(KBUILD_BASENAME
			       " : %s : error releasing the CQ %d\n",
			       __func__, cl->prio.cq->idx);

		if (cl->prio.ccg && qman_ceetm_ccg_release(cl->prio.ccg))
			pr_err(KBUILD_BASENAME
			       " : %s : error releasing the CCG %d\n",
			       __func__, cl->prio.ccg->idx);

		kfree(cl->prio.fq);

		if (cl->prio.cstats)
			free_percpu(cl->prio.cstats);

		break;

	case CEETM_WBFS:
		if (cl->wbfs.lfq && qman_ceetm_lfq_release(cl->wbfs.lfq))
			pr_err(KBUILD_BASENAME
			       " : %s : error releasing the LFQ %d\n",
			       __func__, cl->wbfs.lfq->idx);

		if (cl->wbfs.cq && qman_ceetm_cq_release(cl->wbfs.cq))
			pr_err(KBUILD_BASENAME
			       " : %s : error releasing the CQ %d\n",
			       __func__, cl->wbfs.cq->idx);

		if (cl->wbfs.ccg && qman_ceetm_ccg_release(cl->wbfs.ccg))
			pr_err(KBUILD_BASENAME
			       " : %s : error releasing the CCG %d\n",
			       __func__, cl->wbfs.ccg->idx);

		kfree(cl->wbfs.fq);

		if (cl->wbfs.cstats)
			free_percpu(cl->wbfs.cstats);
	}

	tcf_destroy_chain(&cl->filter_list);
	kfree(cl);
}

/* Destroy a ceetm qdisc */
static void ceetm_destroy(struct Qdisc *sch)
{
	unsigned int ntx, i;
	struct hlist_node *next;
	struct ceetm_class *cl;
	struct ceetm_qdisc *priv = qdisc_priv(sch);
	struct net_device *dev = qdisc_dev(sch);

	pr_debug(KBUILD_BASENAME " : %s : destroy qdisc %X\n",
		 __func__, sch->handle);

	/* All filters need to be removed before destroying the classes */
	tcf_destroy_chain(&priv->filter_list);

	for (i = 0; i < priv->clhash.hashsize; i++) {
		hlist_for_each_entry(cl, &priv->clhash.hash[i], common.hnode)
			tcf_destroy_chain(&cl->filter_list);
	}

	for (i = 0; i < priv->clhash.hashsize; i++) {
		hlist_for_each_entry_safe(cl, next, &priv->clhash.hash[i],
					  common.hnode)
			ceetm_cls_destroy(sch, cl);
	}

	qdisc_class_hash_destroy(&priv->clhash);

	switch (priv->type) {
	case CEETM_ROOT:
		dpa_disable_ceetm(dev);

		if (priv->root.lni && qman_ceetm_lni_release(priv->root.lni))
			pr_err(KBUILD_BASENAME
			       " : %s : error releasing the LNI %d\n",
			       __func__, priv->root.lni->idx);

		if (priv->root.sp && qman_ceetm_sp_release(priv->root.sp))
			pr_err(KBUILD_BASENAME
			       " : %s : error releasing the SP %d\n",
			       __func__, priv->root.sp->idx);

		if (priv->root.qstats)
			free_percpu(priv->root.qstats);

		if (!priv->root.qdiscs)
			break;

		/* Remove the pfifo qdiscs */
		for (ntx = 0; ntx < dev->num_tx_queues; ntx++)
			if (priv->root.qdiscs[ntx])
				qdisc_destroy(priv->root.qdiscs[ntx]);

		kfree(priv->root.qdiscs);
		break;

	case CEETM_PRIO:
		if (priv->prio.parent)
			priv->prio.parent->root.child = NULL;
		break;

	case CEETM_WBFS:
		if (priv->wbfs.parent)
			priv->wbfs.parent->prio.child = NULL;
		break;
	}
}

static int ceetm_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct Qdisc *qdisc;
	unsigned int ntx, i;
	struct nlattr *nest;
	struct tc_ceetm_qopt qopt;
	struct ceetm_qdisc_stats *qstats;
	struct net_device *dev = qdisc_dev(sch);
	struct ceetm_qdisc *priv = qdisc_priv(sch);

	pr_debug(KBUILD_BASENAME " : %s : qdisc %X\n", __func__, sch->handle);

	sch_tree_lock(sch);
	memset(&qopt, 0, sizeof(qopt));
	qopt.type = priv->type;
	qopt.shaped = priv->shaped;

	switch (priv->type) {
	case CEETM_ROOT:
		/* Gather statistics from the underlying pfifo qdiscs */
		sch->q.qlen = 0;
		memset(&sch->bstats, 0, sizeof(sch->bstats));
		memset(&sch->qstats, 0, sizeof(sch->qstats));

		for (ntx = 0; ntx < dev->num_tx_queues; ntx++) {
			qdisc = netdev_get_tx_queue(dev, ntx)->qdisc_sleeping;
			sch->q.qlen		+= qdisc->q.qlen;
			sch->bstats.bytes	+= qdisc->bstats.bytes;
			sch->bstats.packets	+= qdisc->bstats.packets;
			sch->qstats.qlen	+= qdisc->qstats.qlen;
			sch->qstats.backlog	+= qdisc->qstats.backlog;
			sch->qstats.drops	+= qdisc->qstats.drops;
			sch->qstats.requeues	+= qdisc->qstats.requeues;
			sch->qstats.overlimits	+= qdisc->qstats.overlimits;
		}

		for_each_online_cpu(i) {
			qstats = per_cpu_ptr(priv->root.qstats, i);
			sch->qstats.drops += qstats->drops;
		}

		qopt.rate = priv->root.rate;
		qopt.ceil = priv->root.ceil;
		qopt.overhead = priv->root.overhead;
		break;

	case CEETM_PRIO:
		qopt.qcount = priv->prio.qcount;
		break;

	case CEETM_WBFS:
		qopt.qcount = priv->wbfs.qcount;
		qopt.cr = priv->wbfs.cr;
		qopt.er = priv->wbfs.er;
		break;

	default:
		pr_err(KBUILD_BASENAME " : %s : invalid qdisc\n", __func__);
		sch_tree_unlock(sch);
		return -EINVAL;
	}

	nest = nla_nest_start(skb, TCA_OPTIONS);
	if (!nest)
		goto nla_put_failure;
	if (nla_put(skb, TCA_CEETM_QOPS, sizeof(qopt), &qopt))
		goto nla_put_failure;
	nla_nest_end(skb, nest);

	sch_tree_unlock(sch);
	return skb->len;

nla_put_failure:
	sch_tree_unlock(sch);
	nla_nest_cancel(skb, nest);
	return -EMSGSIZE;
}

/* Configure a root ceetm qdisc */
static int ceetm_init_root(struct Qdisc *sch, struct ceetm_qdisc *priv,
			   struct tc_ceetm_qopt *qopt)
{
	struct netdev_queue *dev_queue;
	struct Qdisc *qdisc;
	enum qm_dc_portal dcp_id;
	unsigned int i, sp_id;
	int err;
	u64 bps;
	struct qm_ceetm_sp *sp;
	struct qm_ceetm_lni *lni;
	struct net_device *dev = qdisc_dev(sch);
	struct dpa_priv_s *dpa_priv = netdev_priv(dev);
	struct mac_device *mac_dev = dpa_priv->mac_dev;

	pr_debug(KBUILD_BASENAME " : %s : qdisc %X\n", __func__, sch->handle);

	/* Validate inputs */
	if (sch->parent != TC_H_ROOT) {
		pr_err("CEETM: a root ceetm qdisc can not be attached to a class\n");
		tcf_destroy_chain(&priv->filter_list);
		qdisc_class_hash_destroy(&priv->clhash);
		return -EINVAL;
	}

	if (!mac_dev) {
		pr_err("CEETM: the interface is lacking a mac\n");
		err = -EINVAL;
		goto err_init_root;
	}

	/* pre-allocate underlying pfifo qdiscs */
	priv->root.qdiscs = kcalloc(dev->num_tx_queues,
				    sizeof(priv->root.qdiscs[0]),
				    GFP_KERNEL);
	if (!priv->root.qdiscs) {
		err = -ENOMEM;
		goto err_init_root;
	}

	for (i = 0; i < dev->num_tx_queues; i++) {
		dev_queue = netdev_get_tx_queue(dev, i);
		qdisc = qdisc_create_dflt(dev_queue, &pfifo_qdisc_ops,
					  TC_H_MAKE(TC_H_MAJ(sch->handle),
					  TC_H_MIN(i + PFIFO_MIN_OFFSET)));
		if (!qdisc) {
			err = -ENOMEM;
			goto err_init_root;
		}

		priv->root.qdiscs[i] = qdisc;
		qdisc->flags |= TCQ_F_ONETXQUEUE;
	}

	sch->flags |= TCQ_F_MQROOT;

	priv->root.qstats = alloc_percpu(struct ceetm_qdisc_stats);
	if (!priv->root.qstats) {
		pr_err(KBUILD_BASENAME " : %s : alloc_percpu() failed\n",
		       __func__);
		err = -ENOMEM;
		goto err_init_root;
	}

	priv->shaped = qopt->shaped;
	priv->root.rate = qopt->rate;
	priv->root.ceil = qopt->ceil;
	priv->root.overhead = qopt->overhead;

	/* Claim the SP */
	get_dcp_and_sp(dev, &dcp_id, &sp_id);
	err = qman_ceetm_sp_claim(&sp, dcp_id, sp_id);
	if (err) {
		pr_err(KBUILD_BASENAME " : %s : failed to claim the SP\n",
		       __func__);
		goto err_init_root;
	}

	priv->root.sp = sp;

	/* Claim the LNI - will use the same id as the SP id since SPs 0-7
	 * are connected to the TX FMan ports */
	err = qman_ceetm_lni_claim(&lni, dcp_id, sp_id);
	if (err) {
		pr_err(KBUILD_BASENAME " : %s : failed to claim the LNI\n",
		       __func__);
		goto err_init_root;
	}

	priv->root.lni = lni;

	err = qman_ceetm_sp_set_lni(sp, lni);
	if (err) {
		pr_err(KBUILD_BASENAME " : %s : failed to link the SP and LNI\n",
		       __func__);
		goto err_init_root;
	}

	lni->sp = sp;

	/* Configure the LNI shaper */
	if (priv->shaped) {
		err = qman_ceetm_lni_enable_shaper(lni, 1, priv->root.overhead);
		if (err) {
			pr_err(KBUILD_BASENAME " : %s : failed to configure the LNI shaper\n",
			       __func__);
			goto err_init_root;
		}

		bps = priv->root.rate << 3; /* Bps -> bps */
		err = qman_ceetm_lni_set_commit_rate_bps(lni, bps, dev->mtu);
		if (err) {
			pr_err(KBUILD_BASENAME " : %s : failed to configure the LNI shaper\n",
			       __func__);
			goto err_init_root;
		}

		bps = priv->root.ceil << 3; /* Bps -> bps */
		err = qman_ceetm_lni_set_excess_rate_bps(lni, bps, dev->mtu);
		if (err) {
			pr_err(KBUILD_BASENAME " : %s : failed to configure the LNI shaper\n",
			       __func__);
			goto err_init_root;
		}
	}

	/* TODO default configuration */

	dpa_enable_ceetm(dev);
	return 0;

err_init_root:
	ceetm_destroy(sch);
	return err;
}

/* Configure a prio ceetm qdisc */
static int ceetm_init_prio(struct Qdisc *sch, struct ceetm_qdisc *priv,
			   struct tc_ceetm_qopt *qopt)
{
	int err;
	unsigned int i;
	struct ceetm_class *parent_cl, *child_cl;
	struct Qdisc *parent_qdisc;
	struct net_device *dev = qdisc_dev(sch);

	pr_debug(KBUILD_BASENAME " : %s : qdisc %X\n", __func__, sch->handle);

	if (sch->parent == TC_H_ROOT) {
		pr_err("CEETM: a prio ceetm qdisc can not be root\n");
		err = -EINVAL;
		goto err_init_prio;
	}

	parent_qdisc = qdisc_lookup(dev, TC_H_MAJ(sch->parent));
	if (strcmp(parent_qdisc->ops->id, ceetm_qdisc_ops.id)) {
		pr_err("CEETM: a ceetm qdisc can not be attached to other qdisc/class types\n");
		err = -EINVAL;
		goto err_init_prio;
	}

	/* Obtain the parent root ceetm_class */
	parent_cl = ceetm_find(sch->parent, parent_qdisc);

	if (!parent_cl || parent_cl->type != CEETM_ROOT) {
		pr_err("CEETM: a prio ceetm qdiscs can be added only under a root ceetm class\n");
		err = -EINVAL;
		goto err_init_prio;
	}

	priv->prio.parent = parent_cl;
	parent_cl->root.child = sch;

	priv->shaped = parent_cl->shaped;
	priv->prio.qcount = qopt->qcount;

	/* Create and configure qcount child classes */
	for (i = 0; i < priv->prio.qcount; i++) {
		child_cl = kzalloc(sizeof(*child_cl), GFP_KERNEL);
		if (!child_cl) {
			pr_err(KBUILD_BASENAME " : %s : kzalloc() failed\n",
			       __func__);
			err = -ENOMEM;
			goto err_init_prio;
		}

		child_cl->prio.cstats = alloc_percpu(struct ceetm_class_stats);
		if (!child_cl->prio.cstats) {
			pr_err(KBUILD_BASENAME " : %s : alloc_percpu() failed\n",
			       __func__);
			err = -ENOMEM;
			goto err_init_prio_cls;
		}

		child_cl->common.classid = TC_H_MAKE(sch->handle, (i + 1));
		child_cl->refcnt = 1;
		child_cl->parent = sch;
		child_cl->type = CEETM_PRIO;
		child_cl->shaped = priv->shaped;
		child_cl->prio.child = NULL;

		/* All shaped CQs have CR and ER enabled by default */
		child_cl->prio.cr = child_cl->shaped;
		child_cl->prio.er = child_cl->shaped;
		child_cl->prio.fq = NULL;
		child_cl->prio.cq = NULL;

		/* Configure the corresponding hardware CQ */
		err = ceetm_config_prio_cls(child_cl, dev,
					    parent_cl->root.ch, i);
		if (err) {
			pr_err(KBUILD_BASENAME " : %s : failed to configure the ceetm prio class %X\n",
			       __func__, child_cl->common.classid);
			goto err_init_prio_cls;
		}

		/* Add class handle in Qdisc */
		ceetm_link_class(sch, &priv->clhash, &child_cl->common);
		pr_debug(KBUILD_BASENAME " : %s : added ceetm prio class %X associated with CQ %d and CCG %d\n",
			 __func__, child_cl->common.classid,
			child_cl->prio.cq->idx, child_cl->prio.ccg->idx);
	}

	return 0;

err_init_prio_cls:
	ceetm_cls_destroy(sch, child_cl);
err_init_prio:
	ceetm_destroy(sch);
	return err;
}

/* Configure a wbfs ceetm qdisc */
static int ceetm_init_wbfs(struct Qdisc *sch, struct ceetm_qdisc *priv,
			   struct tc_ceetm_qopt *qopt)
{
	int err, group_b, small_group;
	unsigned int i, id, prio_a, prio_b;
	struct ceetm_class *parent_cl, *child_cl, *root_cl;
	struct Qdisc *parent_qdisc;
	struct ceetm_qdisc *parent_priv;
	struct qm_ceetm_channel *channel;
	struct net_device *dev = qdisc_dev(sch);

	pr_debug(KBUILD_BASENAME " : %s : qdisc %X\n", __func__, sch->handle);

	/* Validate inputs */
	if (sch->parent == TC_H_ROOT) {
		pr_err("CEETM: a wbfs ceetm qdiscs can not be root\n");
		err = -EINVAL;
		goto err_init_wbfs;
	}

	/* Obtain the parent prio ceetm qdisc */
	parent_qdisc = qdisc_lookup(dev, TC_H_MAJ(sch->parent));
	if (strcmp(parent_qdisc->ops->id, ceetm_qdisc_ops.id)) {
		pr_err("CEETM: a ceetm qdisc can not be attached to other qdisc/class types\n");
		err = -EINVAL;
		goto err_init_wbfs;
	}

	/* Obtain the parent prio ceetm class */
	parent_cl = ceetm_find(sch->parent, parent_qdisc);
	parent_priv = qdisc_priv(parent_qdisc);

	if (!parent_cl || parent_cl->type != CEETM_PRIO) {
		pr_err("CEETM: a wbfs ceetm qdiscs can be added only under a prio ceetm class\n");
		err = -EINVAL;
		goto err_init_wbfs;
	}

	if (!qopt->qcount || !qopt->qweight[0]) {
		pr_err("CEETM: qcount and qweight are mandatory for a wbfs ceetm qdisc\n");
		err = -EINVAL;
		goto err_init_wbfs;
	}

	priv->shaped = parent_cl->shaped;

	if (!priv->shaped && (qopt->cr || qopt->er)) {
		pr_err("CEETM: CR/ER can be enabled only for shaped wbfs ceetm qdiscs\n");
		err = -EINVAL;
		goto err_init_wbfs;
	}

	if (priv->shaped && !(qopt->cr || qopt->er)) {
		pr_err("CEETM: either CR or ER must be enabled for shaped wbfs ceetm qdiscs\n");
		err = -EINVAL;
		goto err_init_wbfs;
	}

	/* Obtain the parent root ceetm class */
	root_cl = parent_priv->prio.parent;
	if ((root_cl->root.wbfs_grp_a && root_cl->root.wbfs_grp_b) ||
	    root_cl->root.wbfs_grp_large) {
		pr_err("CEETM: no more wbfs classes are available\n");
		err = -EINVAL;
		goto err_init_wbfs;
	}

	if ((root_cl->root.wbfs_grp_a || root_cl->root.wbfs_grp_b) &&
	    qopt->qcount == CEETM_MAX_WBFS_QCOUNT) {
		pr_err("CEETM: only %d wbfs classes are available\n",
		       CEETM_MIN_WBFS_QCOUNT);
		err = -EINVAL;
		goto err_init_wbfs;
	}

	priv->wbfs.parent = parent_cl;
	parent_cl->prio.child = sch;

	priv->wbfs.qcount = qopt->qcount;
	priv->wbfs.cr = qopt->cr;
	priv->wbfs.er = qopt->er;

	channel = root_cl->root.ch;

	/* Configure the hardware wbfs channel groups */
	if (priv->wbfs.qcount == CEETM_MAX_WBFS_QCOUNT) {
		/* Configure the large group A */
		priv->wbfs.group_type = WBFS_GRP_LARGE;
		small_group = false;
		group_b = false;
		prio_a = TC_H_MIN(parent_cl->common.classid) - 1;
		prio_b = prio_a;

	} else if (root_cl->root.wbfs_grp_a) {
		/* Configure the group B */
		priv->wbfs.group_type = WBFS_GRP_B;

		err = qman_ceetm_channel_get_group(channel, &small_group,
						   &prio_a, &prio_b);
		if (err) {
			pr_err(KBUILD_BASENAME " : %s : failed to get group details\n",
			       __func__);
			goto err_init_wbfs;
		}

		small_group = true;
		group_b = true;
		prio_b = TC_H_MIN(parent_cl->common.classid) - 1;
		/* If group A isn't configured, configure it as group B */
		prio_a = prio_a ? : prio_b;

	} else {
		/* Configure the small group A */
		priv->wbfs.group_type = WBFS_GRP_A;

		err = qman_ceetm_channel_get_group(channel, &small_group,
						   &prio_a, &prio_b);
		if (err) {
			pr_err(KBUILD_BASENAME " : %s : failed to get group details\n",
			       __func__);
			goto err_init_wbfs;
		}

		small_group = true;
		group_b = false;
		prio_a = TC_H_MIN(parent_cl->common.classid) - 1;
		/* If group B isn't configured, configure it as group A */
		prio_b = prio_b ? : prio_a;
	}

	err = qman_ceetm_channel_set_group(channel, small_group, prio_a,
					   prio_b);
	if (err)
		goto err_init_wbfs;

	if (priv->shaped) {
		err = qman_ceetm_channel_set_group_cr_eligibility(channel,
								group_b,
								priv->wbfs.cr);
		if (err) {
			pr_err(KBUILD_BASENAME " : %s : failed to set group CR eligibility\n",
			       __func__);
			goto err_init_wbfs;
		}

		err = qman_ceetm_channel_set_group_er_eligibility(channel,
								group_b,
								priv->wbfs.er);
		if (err) {
			pr_err(KBUILD_BASENAME " : %s : failed to set group ER eligibility\n",
			       __func__);
			goto err_init_wbfs;
		}
	}

	/* Create qcount child classes */
	for (i = 0; i < priv->wbfs.qcount; i++) {
		child_cl = kzalloc(sizeof(*child_cl), GFP_KERNEL);
		if (!child_cl) {
			pr_err(KBUILD_BASENAME " : %s : kzalloc() failed\n",
			       __func__);
			err = -ENOMEM;
			goto err_init_wbfs;
		}

		child_cl->wbfs.cstats = alloc_percpu(struct ceetm_class_stats);
		if (!child_cl->wbfs.cstats) {
			pr_err(KBUILD_BASENAME " : %s : alloc_percpu() failed\n",
			       __func__);
			err = -ENOMEM;
			goto err_init_wbfs_cls;
		}

		child_cl->common.classid = TC_H_MAKE(sch->handle, (i + 1));
		child_cl->refcnt = 1;
		child_cl->parent = sch;
		child_cl->type = CEETM_WBFS;
		child_cl->shaped = priv->shaped;
		child_cl->wbfs.fq = NULL;
		child_cl->wbfs.cq = NULL;
		child_cl->wbfs.weight = qopt->qweight[i];

		if (priv->wbfs.group_type == WBFS_GRP_B)
			id = WBFS_GRP_B_OFFSET + i;
		else
			id = WBFS_GRP_A_OFFSET + i;

		err = ceetm_config_wbfs_cls(child_cl, dev, channel, id,
					    priv->wbfs.group_type);
		if (err) {
			pr_err(KBUILD_BASENAME " : %s : failed to configure the ceetm wbfs class %X\n",
			       __func__, child_cl->common.classid);
			goto err_init_wbfs_cls;
		}

		/* Add class handle in Qdisc */
		ceetm_link_class(sch, &priv->clhash, &child_cl->common);
		pr_debug(KBUILD_BASENAME " : %s : added ceetm wbfs class %X associated with CQ %d and CCG %d\n",
			 __func__, child_cl->common.classid,
			 child_cl->wbfs.cq->idx, child_cl->wbfs.ccg->idx);
	}

	/* Signal the root class that a group has been configured */
	switch (priv->wbfs.group_type) {
	case WBFS_GRP_LARGE:
		root_cl->root.wbfs_grp_large = true;
		break;
	case WBFS_GRP_A:
		root_cl->root.wbfs_grp_a = true;
		break;
	case WBFS_GRP_B:
		root_cl->root.wbfs_grp_b = true;
		break;
	}

	return 0;

err_init_wbfs_cls:
	ceetm_cls_destroy(sch, child_cl);
err_init_wbfs:
	ceetm_destroy(sch);
	return err;
}

/* Configure a generic ceetm qdisc */
static int ceetm_init(struct Qdisc *sch, struct nlattr *opt)
{
	struct tc_ceetm_qopt *qopt;
	struct nlattr *tb[TCA_CEETM_QOPS + 1];
	int ret;
	struct ceetm_qdisc *priv = qdisc_priv(sch);
	struct net_device *dev = qdisc_dev(sch);

	pr_debug(KBUILD_BASENAME " : %s : qdisc %X\n", __func__, sch->handle);

	if (!netif_is_multiqueue(dev))
		return -EOPNOTSUPP;

	if (!opt) {
		pr_err(KBUILD_BASENAME " : %s : tc error\n", __func__);
		return -EINVAL;
	}

	ret = nla_parse_nested(tb, TCA_CEETM_QOPS, opt, ceetm_policy);
	if (ret < 0) {
		pr_err(KBUILD_BASENAME " : %s : tc error\n", __func__);
		return ret;
	}

	if (!tb[TCA_CEETM_QOPS]) {
		pr_err(KBUILD_BASENAME " : %s : tc error\n", __func__);
		return -EINVAL;
	}

	if (TC_H_MIN(sch->handle)) {
		pr_err("CEETM: a qdisc should not have a minor\n");
		return -EINVAL;
	}

	qopt = nla_data(tb[TCA_CEETM_QOPS]);

	/* Initialize the class hash list. Each qdisc has its own class hash */
	ret = qdisc_class_hash_init(&priv->clhash);
	if (ret < 0) {
		pr_err(KBUILD_BASENAME " : %s : qdisc_class_hash_init failed\n",
		       __func__);
		return ret;
	}

	priv->type = qopt->type;

	switch (priv->type) {
	case CEETM_ROOT:
		ret = ceetm_init_root(sch, priv, qopt);
		break;
	case CEETM_PRIO:
		ret = ceetm_init_prio(sch, priv, qopt);
		break;
	case CEETM_WBFS:
		ret = ceetm_init_wbfs(sch, priv, qopt);
		break;
	default:
		pr_err(KBUILD_BASENAME " : %s : invalid qdisc\n", __func__);
		ceetm_destroy(sch);
		ret = -EINVAL;
	}

	return ret;
}

/* Edit a root ceetm qdisc */
static int ceetm_change_root(struct Qdisc *sch, struct ceetm_qdisc *priv,
			     struct net_device *dev,
			     struct tc_ceetm_qopt *qopt)
{
	int err = 0;
	u64 bps;

	if (priv->shaped != (bool)qopt->shaped) {
		pr_err("CEETM: qdisc %X is %s\n", sch->handle,
		       priv->shaped ? "shaped" : "unshaped");
		return -EINVAL;
	}

	/* Nothing to modify for unshaped qdiscs */
	if (!priv->shaped)
		return 0;

	/* Configure the LNI shaper */
	if (priv->root.overhead != qopt->overhead) {
		err = qman_ceetm_lni_enable_shaper(priv->root.lni, 1,
						   qopt->overhead);
		if (err)
			goto change_err;
		priv->root.overhead = qopt->overhead;
	}

	if (priv->root.rate != qopt->rate) {
		bps = qopt->rate << 3; /* Bps -> bps */
		err = qman_ceetm_lni_set_commit_rate_bps(priv->root.lni, bps,
							 dev->mtu);
		if (err)
			goto change_err;
		priv->root.rate = qopt->rate;
	}

	if (priv->root.ceil != qopt->ceil) {
		bps = qopt->ceil << 3; /* Bps -> bps */
		err = qman_ceetm_lni_set_excess_rate_bps(priv->root.lni, bps,
							 dev->mtu);
		if (err)
			goto change_err;
		priv->root.ceil = qopt->ceil;
	}

	return 0;

change_err:
	pr_err(KBUILD_BASENAME " : %s : failed to configure the root ceetm qdisc %X\n",
	       __func__, sch->handle);
	return err;
}

/* Edit a wbfs ceetm qdisc */
static int ceetm_change_wbfs(struct Qdisc *sch, struct ceetm_qdisc *priv,
			     struct tc_ceetm_qopt *qopt)
{
	int err;
	bool group_b;
	struct qm_ceetm_channel *channel;
	struct ceetm_class *prio_class, *root_class;
	struct ceetm_qdisc *prio_qdisc;

	if (qopt->qcount) {
		pr_err("CEETM: the qcount can not be modified\n");
		return -EINVAL;
	}

	if (qopt->qweight[0]) {
		pr_err("CEETM: the qweight can be modified through the wbfs classes\n");
		return -EINVAL;
	}

	if (!priv->shaped && (qopt->cr || qopt->er)) {
		pr_err("CEETM: CR/ER can be enabled only for shaped wbfs ceetm qdiscs\n");
		return -EINVAL;
	}

	if (priv->shaped && !(qopt->cr || qopt->er)) {
		pr_err("CEETM: either CR or ER must be enabled for shaped wbfs ceetm qdiscs\n");
		return -EINVAL;
	}

	/* Nothing to modify for unshaped qdiscs */
	if (!priv->shaped)
		return 0;

	prio_class = priv->wbfs.parent;
	prio_qdisc = qdisc_priv(prio_class->parent);
	root_class = prio_qdisc->prio.parent;
	channel = root_class->root.ch;
	group_b = priv->wbfs.group_type == WBFS_GRP_B;

	if (qopt->cr != priv->wbfs.cr) {
		err = qman_ceetm_channel_set_group_cr_eligibility(channel,
								  group_b,
								  qopt->cr);
		if (err)
			goto change_err;
		priv->wbfs.cr = qopt->cr;
	}

	if (qopt->er != priv->wbfs.er) {
		err = qman_ceetm_channel_set_group_er_eligibility(channel,
								  group_b,
								  qopt->er);
		if (err)
			goto change_err;
		priv->wbfs.er = qopt->er;
	}

	return 0;

change_err:
	pr_err(KBUILD_BASENAME " : %s : failed to configure the wbfs ceetm qdisc %X\n",
	       __func__, sch->handle);
	return err;
}

/* Edit a ceetm qdisc */
static int ceetm_change(struct Qdisc *sch, struct nlattr *opt)
{
	struct tc_ceetm_qopt *qopt;
	struct nlattr *tb[TCA_CEETM_QOPS + 1];
	int ret;
	struct ceetm_qdisc *priv = qdisc_priv(sch);
	struct net_device *dev = qdisc_dev(sch);

	pr_debug(KBUILD_BASENAME " : %s : qdisc %X\n", __func__, sch->handle);

	ret = nla_parse_nested(tb, TCA_CEETM_QOPS, opt, ceetm_policy);
	if (ret < 0) {
		pr_err(KBUILD_BASENAME " : %s : tc error\n", __func__);
		return ret;
	}

	if (!tb[TCA_CEETM_QOPS]) {
		pr_err(KBUILD_BASENAME " : %s : tc error\n", __func__);
		return -EINVAL;
	}

	if (TC_H_MIN(sch->handle)) {
		pr_err("CEETM: a qdisc should not have a minor\n");
		return -EINVAL;
	}

	qopt = nla_data(tb[TCA_CEETM_QOPS]);

	if (priv->type != qopt->type) {
		pr_err("CEETM: qdisc %X is not of the provided type\n",
		       sch->handle);
		return -EINVAL;
	}

	switch (priv->type) {
	case CEETM_ROOT:
		ret = ceetm_change_root(sch, priv, dev, qopt);
		break;
	case CEETM_PRIO:
		pr_err("CEETM: prio qdiscs can not be modified\n");
		ret = -EINVAL;
		break;
	case CEETM_WBFS:
		ret = ceetm_change_wbfs(sch, priv, qopt);
		break;
	default:
		pr_err(KBUILD_BASENAME " : %s : invalid qdisc\n", __func__);
		ret = -EINVAL;
	}

	return ret;
}

/* Attach the underlying pfifo qdiscs */
static void ceetm_attach(struct Qdisc *sch)
{
	struct net_device *dev = qdisc_dev(sch);
	struct ceetm_qdisc *priv = qdisc_priv(sch);
	struct Qdisc *qdisc, *old_qdisc;
	unsigned int i;

	pr_debug(KBUILD_BASENAME " : %s : qdisc %X\n", __func__, sch->handle);

	for (i = 0; i < dev->num_tx_queues; i++) {
		qdisc = priv->root.qdiscs[i];
		old_qdisc = dev_graft_qdisc(qdisc->dev_queue, qdisc);
		if (old_qdisc)
			qdisc_destroy(old_qdisc);
	}
}

static unsigned long ceetm_cls_get(struct Qdisc *sch, u32 classid)
{
	struct ceetm_class *cl;

	pr_debug(KBUILD_BASENAME " : %s : classid %X from qdisc %X\n",
		 __func__, classid, sch->handle);
	cl = ceetm_find(classid, sch);

	if (cl)
		cl->refcnt++; /* Will decrement in put() */
	return (unsigned long)cl;
}

static void ceetm_cls_put(struct Qdisc *sch, unsigned long arg)
{
	struct ceetm_class *cl = (struct ceetm_class *)arg;

	pr_debug(KBUILD_BASENAME " : %s : classid %X from qdisc %X\n",
		 __func__, cl->common.classid, sch->handle);
	cl->refcnt--;

	if (cl->refcnt == 0)
		ceetm_cls_destroy(sch, cl);
}

static int ceetm_cls_change_root(struct ceetm_class *cl,
				 struct tc_ceetm_copt *copt,
				 struct net_device *dev)
{
	int err;
	u64 bps;

	if ((bool)copt->shaped != cl->shaped) {
		pr_err("CEETM: class %X is %s\n", cl->common.classid,
		       cl->shaped ? "shaped" : "unshaped");
		return -EINVAL;
	}

	if (cl->shaped && cl->root.rate != copt->rate) {
		bps = copt->rate << 3; /* Bps -> bps */
		err = qman_ceetm_channel_set_commit_rate_bps(cl->root.ch, bps,
							     dev->mtu);
		if (err)
			goto change_cls_err;
		cl->root.rate = copt->rate;
	}

	if (cl->shaped && cl->root.ceil != copt->ceil) {
		bps = copt->ceil << 3; /* Bps -> bps */
		err = qman_ceetm_channel_set_excess_rate_bps(cl->root.ch, bps,
							     dev->mtu);
		if (err)
			goto change_cls_err;
		cl->root.ceil = copt->ceil;
	}

	if (!cl->shaped && cl->root.tbl != copt->tbl) {
		err = qman_ceetm_channel_set_weight(cl->root.ch, copt->tbl);
		if (err)
			goto change_cls_err;
		cl->root.tbl = copt->tbl;
	}

	return 0;

change_cls_err:
	pr_err(KBUILD_BASENAME " : %s : failed to configure the ceetm root class %X\n",
	       __func__, cl->common.classid);
	return err;
}

static int ceetm_cls_change_prio(struct ceetm_class *cl,
				 struct tc_ceetm_copt *copt)
{
	int err;

	if (!cl->shaped && (copt->cr || copt->er)) {
		pr_err("CEETM: only shaped classes can have CR and ER enabled\n");
		return -EINVAL;
	}

	if (cl->prio.cr != (bool)copt->cr) {
		err = qman_ceetm_channel_set_cq_cr_eligibility(
						cl->prio.cq->parent,
						cl->prio.cq->idx,
						copt->cr);
		if (err)
			goto change_cls_err;
		cl->prio.cr = copt->cr;
	}

	if (cl->prio.er != (bool)copt->er) {
		err = qman_ceetm_channel_set_cq_er_eligibility(
						cl->prio.cq->parent,
						cl->prio.cq->idx,
						copt->er);
		if (err)
			goto change_cls_err;
		cl->prio.er = copt->er;
	}

	return 0;

change_cls_err:
	pr_err(KBUILD_BASENAME " : %s : failed to configure the ceetm prio class %X\n",
	       __func__, cl->common.classid);
	return err;
}

static int ceetm_cls_change_wbfs(struct ceetm_class *cl,
				 struct tc_ceetm_copt *copt)
{
	int err;

	if (copt->weight != cl->wbfs.weight) {
		/* Configure the CQ weight: real number multiplied by 100 to
		 * get rid of the fraction
		 */
		err = qman_ceetm_set_queue_weight_in_ratio(cl->wbfs.cq,
							   copt->weight * 100);

		if (err) {
			pr_err(KBUILD_BASENAME " : %s : failed to configure the ceetm wbfs class %X\n",
			       __func__, cl->common.classid);
			return err;
		}

		cl->wbfs.weight = copt->weight;
	}

	return 0;
}

/* Add a ceetm root class or configure a ceetm root/prio/wbfs class */
static int ceetm_cls_change(struct Qdisc *sch, u32 classid, u32 parentid,
			    struct nlattr **tca, unsigned long *arg)
{
	int err;
	u64 bps;
	struct ceetm_qdisc *priv;
	struct ceetm_class *cl = (struct ceetm_class *)*arg;
	struct nlattr *opt = tca[TCA_OPTIONS];
	struct nlattr *tb[__TCA_CEETM_MAX];
	struct tc_ceetm_copt *copt;
	struct qm_ceetm_channel *channel;
	struct net_device *dev = qdisc_dev(sch);

	pr_debug(KBUILD_BASENAME " : %s : classid %X under qdisc %X\n",
		 __func__, classid, sch->handle);

	if (strcmp(sch->ops->id, ceetm_qdisc_ops.id)) {
		pr_err("CEETM: a ceetm class can not be attached to other qdisc/class types\n");
		return -EINVAL;
	}

	priv = qdisc_priv(sch);

	if (!opt) {
		pr_err(KBUILD_BASENAME " : %s : tc error\n", __func__);
		return -EINVAL;
	}

	if (!cl && sch->handle != parentid) {
		pr_err("CEETM: classes can be attached to the root ceetm qdisc only\n");
		return -EINVAL;
	}

	if (!cl && priv->type != CEETM_ROOT) {
		pr_err("CEETM: only root ceetm classes can be attached to the root ceetm qdisc\n");
		return -EINVAL;
	}

	err = nla_parse_nested(tb, TCA_CEETM_COPT, opt, ceetm_policy);
	if (err < 0) {
		pr_err(KBUILD_BASENAME " : %s : tc error\n", __func__);
		return -EINVAL;
	}

	if (!tb[TCA_CEETM_COPT]) {
		pr_err(KBUILD_BASENAME " : %s : tc error\n", __func__);
		return -EINVAL;
	}

	if (TC_H_MIN(classid) >= PFIFO_MIN_OFFSET) {
		pr_err("CEETM: only minors 0x01 to 0x20 can be used for ceetm root classes\n");
		return -EINVAL;
	}

	copt = nla_data(tb[TCA_CEETM_COPT]);

	/* Configure an existing ceetm class */
	if (cl) {
		if (copt->type != cl->type) {
			pr_err("CEETM: class %X is not of the provided type\n",
			       cl->common.classid);
			return -EINVAL;
		}

		switch (copt->type) {
		case CEETM_ROOT:
			return ceetm_cls_change_root(cl, copt, dev);

		case CEETM_PRIO:
			return ceetm_cls_change_prio(cl, copt);

		case CEETM_WBFS:
			return ceetm_cls_change_wbfs(cl, copt);

		default:
			pr_err(KBUILD_BASENAME " : %s : invalid class\n",
			       __func__);
			return -EINVAL;
		}
	}

	/* Add a new root ceetm class */
	if (copt->type != CEETM_ROOT) {
		pr_err("CEETM: only root ceetm classes can be attached to the root ceetm qdisc\n");
		return -EINVAL;
	}

	if (copt->shaped && !priv->shaped) {
		pr_err("CEETM: can not add a shaped ceetm root class under an unshaped ceetm root qdisc\n");
		return -EINVAL;
	}

	cl = kzalloc(sizeof(*cl), GFP_KERNEL);
	if (!cl)
		return -ENOMEM;

	cl->type = copt->type;
	cl->shaped = copt->shaped;
	cl->root.rate = copt->rate;
	cl->root.ceil = copt->ceil;
	cl->root.tbl = copt->tbl;

	cl->common.classid = classid;
	cl->refcnt = 1;
	cl->parent = sch;
	cl->root.child = NULL;
	cl->root.wbfs_grp_a = false;
	cl->root.wbfs_grp_b = false;
	cl->root.wbfs_grp_large = false;

	/* Claim a CEETM channel */
	err = qman_ceetm_channel_claim(&channel, priv->root.lni);
	if (err) {
		pr_err(KBUILD_BASENAME " : %s : failed to claim a channel\n",
		       __func__);
		goto claim_err;
	}

	cl->root.ch = channel;

	if (cl->shaped) {
		/* Configure the channel shaper */
		err = qman_ceetm_channel_enable_shaper(channel, 1);
		if (err)
			goto channel_err;

		bps = cl->root.rate << 3; /* Bps -> bps */
		err = qman_ceetm_channel_set_commit_rate_bps(channel, bps,
							     dev->mtu);
		if (err)
			goto channel_err;

		bps = cl->root.ceil << 3; /* Bps -> bps */
		err = qman_ceetm_channel_set_excess_rate_bps(channel, bps,
							     dev->mtu);
		if (err)
			goto channel_err;

	} else {
		/* Configure the uFQ algorithm */
		err = qman_ceetm_channel_set_weight(channel, cl->root.tbl);
		if (err)
			goto channel_err;
	}

	/* Add class handle in Qdisc */
	ceetm_link_class(sch, &priv->clhash, &cl->common);

	pr_debug(KBUILD_BASENAME " : %s : configured class %X associated with channel %d\n",
		 __func__, classid, channel->idx);
	*arg = (unsigned long)cl;
	return 0;

channel_err:
	pr_err(KBUILD_BASENAME " : %s : failed to configure the channel %d\n",
	       __func__, channel->idx);
	if (qman_ceetm_channel_release(channel))
		pr_err(KBUILD_BASENAME " : %s : failed to release the channel %d\n",
		       __func__, channel->idx);
claim_err:
	kfree(cl);
	return err;
}

static void ceetm_cls_walk(struct Qdisc *sch, struct qdisc_walker *arg)
{
	struct ceetm_qdisc *priv = qdisc_priv(sch);
	struct ceetm_class *cl;
	unsigned int i;

	pr_debug(KBUILD_BASENAME " : %s : qdisc %X\n", __func__, sch->handle);

	if (arg->stop)
		return;

	for (i = 0; i < priv->clhash.hashsize; i++) {
		hlist_for_each_entry(cl, &priv->clhash.hash[i], common.hnode) {
			if (arg->count < arg->skip) {
				arg->count++;
				continue;
			}
			if (arg->fn(sch, (unsigned long)cl, arg) < 0) {
				arg->stop = 1;
				return;
			}
			arg->count++;
		}
	}
}

static int ceetm_cls_dump(struct Qdisc *sch, unsigned long arg,
			  struct sk_buff *skb, struct tcmsg *tcm)
{
	struct ceetm_class *cl = (struct ceetm_class *)arg;
	struct nlattr *nest;
	struct tc_ceetm_copt copt;

	pr_debug(KBUILD_BASENAME " : %s : class %X under qdisc %X\n",
		 __func__, cl->common.classid, sch->handle);

	sch_tree_lock(sch);

	tcm->tcm_parent = ((struct Qdisc *)cl->parent)->handle;
	tcm->tcm_handle = cl->common.classid;

	memset(&copt, 0, sizeof(copt));

	copt.shaped = cl->shaped;
	copt.type = cl->type;

	switch (cl->type) {
	case CEETM_ROOT:
		if (cl->root.child)
			tcm->tcm_info = cl->root.child->handle;

		copt.rate = cl->root.rate;
		copt.ceil = cl->root.ceil;
		copt.tbl = cl->root.tbl;
		break;

	case CEETM_PRIO:
		if (cl->prio.child)
			tcm->tcm_info = cl->prio.child->handle;

		copt.cr = cl->prio.cr;
		copt.er = cl->prio.er;
		break;

	case CEETM_WBFS:
		copt.weight = cl->wbfs.weight;
		break;
	}

	nest = nla_nest_start(skb, TCA_OPTIONS);
	if (!nest)
		goto nla_put_failure;
	if (nla_put(skb, TCA_CEETM_COPT, sizeof(copt), &copt))
		goto nla_put_failure;
	nla_nest_end(skb, nest);
	sch_tree_unlock(sch);
	return skb->len;

nla_put_failure:
	sch_tree_unlock(sch);
	nla_nest_cancel(skb, nest);
	return -EMSGSIZE;
}

static int ceetm_cls_delete(struct Qdisc *sch, unsigned long arg)
{
	struct ceetm_qdisc *priv = qdisc_priv(sch);
	struct ceetm_class *cl = (struct ceetm_class *)arg;

	pr_debug(KBUILD_BASENAME " : %s : class %X under qdisc %X\n",
		 __func__, cl->common.classid, sch->handle);

	sch_tree_lock(sch);
	qdisc_class_hash_remove(&priv->clhash, &cl->common);
	cl->refcnt--;

	/* The refcnt should be at least 1 since we have incremented it in
	   get(). Will decrement again in put() where we will call destroy()
	   to actually free the memory if it reaches 0. */
	WARN_ON(cl->refcnt == 0);

	sch_tree_unlock(sch);
	return 0;
}

/* Get the class' child qdisc, if any */
static struct Qdisc *ceetm_cls_leaf(struct Qdisc *sch, unsigned long arg)
{
	struct ceetm_class *cl = (struct ceetm_class *)arg;

	pr_debug(KBUILD_BASENAME " : %s : class %X under qdisc %X\n",
		 __func__, cl->common.classid, sch->handle);

	switch (cl->type) {
	case CEETM_ROOT:
		return cl->root.child;

	case CEETM_PRIO:
		return cl->prio.child;
	}

	return NULL;
}

static int ceetm_cls_graft(struct Qdisc *sch, unsigned long arg,
			   struct Qdisc *new, struct Qdisc **old)
{
	if (new && strcmp(new->ops->id, ceetm_qdisc_ops.id)) {
		pr_err("CEETM: only ceetm qdiscs can be attached to ceetm classes\n");
		return -EOPNOTSUPP;
	}

	return 0;
}

static int ceetm_cls_dump_stats(struct Qdisc *sch, unsigned long arg,
				struct gnet_dump *d)
{
	unsigned int i;
	struct ceetm_class *cl = (struct ceetm_class *)arg;
	struct gnet_stats_basic_packed tmp_bstats;
	struct ceetm_class_stats *cstats = NULL;
	struct qm_ceetm_cq *cq = NULL;
	struct tc_ceetm_xstats xstats;

	memset(&xstats, 0, sizeof(xstats));
	memset(&tmp_bstats, 0, sizeof(tmp_bstats));

	switch (cl->type) {
	case CEETM_ROOT:
		return 0;
	case CEETM_PRIO:
		cq = cl->prio.cq;
		break;
	case CEETM_WBFS:
		cq = cl->wbfs.cq;
		break;
	}

	for_each_online_cpu(i) {
		switch (cl->type) {
		case CEETM_PRIO:
			cstats = per_cpu_ptr(cl->prio.cstats, i);
			break;
		case CEETM_WBFS:
			cstats = per_cpu_ptr(cl->wbfs.cstats, i);
			break;
		}

		if (cstats) {
			xstats.ern_drop_count += cstats->ern_drop_count;
			xstats.congested_count += cstats->congested_count;
			tmp_bstats.bytes += cstats->bstats.bytes;
			tmp_bstats.packets += cstats->bstats.packets;
		}
	}

	if (gnet_stats_copy_basic(d, NULL, &tmp_bstats) < 0)
		return -1;

	if (cq && qman_ceetm_cq_get_dequeue_statistics(cq, 0,
						       &xstats.frame_count,
						       &xstats.byte_count))
		return -1;

	return gnet_stats_copy_app(d, &xstats, sizeof(xstats));
}

static struct tcf_proto **ceetm_tcf_chain(struct Qdisc *sch, unsigned long arg)
{
	struct ceetm_qdisc *priv = qdisc_priv(sch);
	struct ceetm_class *cl = (struct ceetm_class *)arg;
	struct tcf_proto **fl = cl ? &cl->filter_list : &priv->filter_list;

	pr_debug(KBUILD_BASENAME " : %s : class %X under qdisc %X\n", __func__,
		 cl ? cl->common.classid : 0, sch->handle);
	return fl;
}

static unsigned long ceetm_tcf_bind(struct Qdisc *sch, unsigned long parent,
				    u32 classid)
{
	struct ceetm_class *cl = ceetm_find(classid, sch);

	pr_debug(KBUILD_BASENAME " : %s : class %X under qdisc %X\n", __func__,
		 cl ? cl->common.classid : 0, sch->handle);
	return (unsigned long)cl;
}

static void ceetm_tcf_unbind(struct Qdisc *sch, unsigned long arg)
{
	struct ceetm_class *cl = (struct ceetm_class *)arg;

	pr_debug(KBUILD_BASENAME " : %s : class %X under qdisc %X\n", __func__,
		 cl ? cl->common.classid : 0, sch->handle);
}

const struct Qdisc_class_ops ceetm_cls_ops = {
	.graft		=	ceetm_cls_graft,
	.leaf		=	ceetm_cls_leaf,
	.get		=	ceetm_cls_get,
	.put		=	ceetm_cls_put,
	.change		=	ceetm_cls_change,
	.delete		=	ceetm_cls_delete,
	.walk		=	ceetm_cls_walk,
	.tcf_chain	=	ceetm_tcf_chain,
	.bind_tcf	=	ceetm_tcf_bind,
	.unbind_tcf	=	ceetm_tcf_unbind,
	.dump		=	ceetm_cls_dump,
	.dump_stats	=	ceetm_cls_dump_stats,
};

struct Qdisc_ops ceetm_qdisc_ops __read_mostly = {
	.id		=	"ceetm",
	.priv_size	=	sizeof(struct ceetm_qdisc),
	.cl_ops		=	&ceetm_cls_ops,
	.init		=	ceetm_init,
	.destroy	=	ceetm_destroy,
	.change		=	ceetm_change,
	.dump		=	ceetm_dump,
	.attach		=	ceetm_attach,
	.owner		=	THIS_MODULE,
};

/* Run the filters and classifiers attached to the qdisc on the provided skb */
static struct ceetm_class *ceetm_classify(struct sk_buff *skb,
					  struct Qdisc *sch, int *qerr,
					  bool *act_drop)
{
	struct ceetm_qdisc *priv = qdisc_priv(sch);
	struct ceetm_class *cl = NULL, *wbfs_cl;
	struct tcf_result res;
	struct tcf_proto *tcf;
	int result;

	*qerr = NET_XMIT_SUCCESS | __NET_XMIT_BYPASS;
	tcf = priv->filter_list;
	while (tcf && (result = tc_classify(skb, tcf, &res)) >= 0) {
#ifdef CONFIG_NET_CLS_ACT
		switch (result) {
		case TC_ACT_QUEUED:
		case TC_ACT_STOLEN:
			*qerr = NET_XMIT_SUCCESS | __NET_XMIT_STOLEN;
		case TC_ACT_SHOT:
			/* No valid class found due to action */
			*act_drop = true;
			return NULL;
		}
#endif
		cl = (void *)res.class;
		if (!cl) {
			if (res.classid == sch->handle) {
				/* The filter leads to the qdisc */
				/* TODO default qdisc */
				return NULL;
			}

			cl = ceetm_find(res.classid, sch);
			if (!cl)
				/* The filter leads to an invalid class */
				break;
		}

		/* The class might have its own filters attached */
		tcf = cl->filter_list;
	}

	if (!cl) {
		/* No valid class found */
		/* TODO default qdisc */
		return NULL;
	}

	switch (cl->type) {
	case CEETM_ROOT:
		if (cl->root.child) {
			/* Run the prio qdisc classifiers */
			return ceetm_classify(skb, cl->root.child, qerr,
						act_drop);
		} else {
			/* The root class does not have a child prio qdisc */
			/* TODO default qdisc */
			return NULL;
		}
	case CEETM_PRIO:
		if (cl->prio.child) {
			/* If filters lead to a wbfs class, return it.
			 * Otherwise, return the prio class */
			wbfs_cl = ceetm_classify(skb, cl->prio.child, qerr,
						 act_drop);
			/* A NULL result might indicate either an erroneous
			 * filter, or no filters at all. We will assume the
			 * latter */
			return wbfs_cl ? : cl;
		}
	}

	/* For wbfs and childless prio classes, return the class directly */
	return cl;
}

int __hot ceetm_tx(struct sk_buff *skb, struct net_device *net_dev)
{
	int ret;
	bool act_drop = false;
	struct Qdisc *sch = net_dev->qdisc;
	struct ceetm_class *cl;
	struct dpa_priv_s *priv_dpa;
	struct qman_fq *egress_fq, *conf_fq;
	struct ceetm_qdisc *priv = qdisc_priv(sch);
	struct ceetm_qdisc_stats *qstats = this_cpu_ptr(priv->root.qstats);
	struct ceetm_class_stats *cstats;
	const int queue_mapping = dpa_get_queue_mapping(skb);
	spinlock_t *root_lock = qdisc_lock(sch);

	spin_lock(root_lock);
	cl = ceetm_classify(skb, sch, &ret, &act_drop);
	spin_unlock(root_lock);

#ifdef CONFIG_NET_CLS_ACT
	if (act_drop) {
		if (ret & __NET_XMIT_BYPASS)
			qstats->drops++;
		goto drop;
	}
#endif
	/* TODO default class */
	if (unlikely(!cl)) {
		qstats->drops++;
		goto drop;
	}

	priv_dpa = netdev_priv(net_dev);
	conf_fq = priv_dpa->conf_fqs[queue_mapping];

	/* Choose the proper tx fq and update the basic stats (bytes and
	 * packets sent by the class) */
	switch (cl->type) {
	case CEETM_PRIO:
		egress_fq = &(cl->prio.fq->fq);
		cstats = this_cpu_ptr(cl->prio.cstats);
		break;
	case CEETM_WBFS:
		egress_fq = &(cl->wbfs.fq->fq);
		cstats = this_cpu_ptr(cl->wbfs.cstats);
		break;
	default:
		qstats->drops++;
		goto drop;
	}

	bstats_update(&cstats->bstats, skb);
	return dpa_tx_extended(skb, net_dev, egress_fq, conf_fq);

drop:
	dev_kfree_skb_any(skb);
	return NET_XMIT_SUCCESS;
}

static int __init ceetm_register(void)
{
	int _errno = 0;

	pr_info(KBUILD_MODNAME ": " DPA_CEETM_DESCRIPTION "\n");

	_errno = register_qdisc(&ceetm_qdisc_ops);
	if (unlikely(_errno))
		pr_err(KBUILD_MODNAME
		       ": %s:%hu:%s(): register_qdisc() = %d\n",
		       KBUILD_BASENAME".c", __LINE__, __func__, _errno);

	return _errno;
}

static void __exit ceetm_unregister(void)
{
	pr_debug(KBUILD_MODNAME ": %s:%s() ->\n",
		 KBUILD_BASENAME".c", __func__);

	unregister_qdisc(&ceetm_qdisc_ops);
}

module_init(ceetm_register);
module_exit(ceetm_unregister);
