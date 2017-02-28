/*
 * drivers/dma/dpaa2-qdma/dpaa2-qdma.c
 *
 * Copyright 2015-2017 NXP Semiconductor, Inc.
 * Author: Changming Huang <jerry.huang@nxp.com>
 *
 * Driver for the NXP QDMA engine with QMan mode.
 * Channel virtualization is supported through enqueuing of DMA jobs to,
 * or dequeuing DMA jobs from different work queues with QMan portal.
 * This module can be found on NXP LS2 SoCs.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_dma.h>
#include <linux/types.h>
#include <linux/delay.h>

#include "../virt-dma.h"

#include "../../../drivers/staging/fsl-mc/include/mc.h"
#include "../../../drivers/staging/fsl-mc/include/fsl_dpaa2_io.h"
#include "../../../drivers/staging/fsl-mc/include/fsl_dpaa2_fd.h"
#include "fsl_dpdmai_cmd.h"
#include "fsl_dpdmai.h"
#include "dpaa2-qdma.h"

static struct dpaa2_qdma_chan *to_dpaa2_qdma_chan(struct dma_chan *chan)
{
	return container_of(chan, struct dpaa2_qdma_chan, vchan.chan);
}

static struct dpaa2_qdma_comp *to_fsl_qdma_comp(struct virt_dma_desc *vd)
{
	return container_of(vd, struct dpaa2_qdma_comp, vdesc);
}

static int dpaa2_qdma_alloc_chan_resources(struct dma_chan *chan)
{
	return 0;
}

static void dpaa2_qdma_free_chan_resources(struct dma_chan *chan)
{
	struct dpaa2_qdma_chan *dpaa2_chan = to_dpaa2_qdma_chan(chan);
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock_irqsave(&dpaa2_chan->vchan.lock, flags);
	vchan_get_all_descriptors(&dpaa2_chan->vchan, &head);
	spin_unlock_irqrestore(&dpaa2_chan->vchan.lock, flags);

	vchan_dma_desc_free_list(&dpaa2_chan->vchan, &head);
}

/*
 * Request a command descriptor for enqueue.
 */
static struct dpaa2_qdma_comp *
dpaa2_qdma_request_desc(struct dpaa2_qdma_chan *dpaa2_chan)
{
	struct dpaa2_qdma_comp *comp_temp = NULL;
	unsigned long flags;

	spin_lock_irqsave(&dpaa2_chan->queue_lock, flags);
	if (list_empty(&dpaa2_chan->comp_free)) {
		spin_unlock_irqrestore(&dpaa2_chan->queue_lock, flags);
		comp_temp = kzalloc(sizeof(*comp_temp), GFP_KERNEL);
		if (!comp_temp)
			goto err;
		comp_temp->fd_virt_addr = dma_pool_alloc(dpaa2_chan->fd_pool,
				GFP_NOWAIT, &comp_temp->fd_bus_addr);
		if (!comp_temp->fd_virt_addr)
			goto err;

		comp_temp->fl_virt_addr =
			(void *)((struct dpaa2_fd *)
				comp_temp->fd_virt_addr + 1);
		comp_temp->fl_bus_addr = comp_temp->fd_bus_addr +
					sizeof(struct dpaa2_fd);
		comp_temp->desc_virt_addr =
			(void *)((struct dpaa2_frame_list *)
				comp_temp->fl_virt_addr + 3);
		comp_temp->desc_bus_addr = comp_temp->fl_bus_addr +
				sizeof(struct dpaa2_frame_list) * 3;

		comp_temp->qchan = dpaa2_chan;
		comp_temp->sg_blk_num = 0;
		INIT_LIST_HEAD(&comp_temp->sg_src_head);
		INIT_LIST_HEAD(&comp_temp->sg_dst_head);
		return comp_temp;
	}
	comp_temp = list_first_entry(&dpaa2_chan->comp_free,
			struct dpaa2_qdma_comp, list);
	list_del(&comp_temp->list);
	spin_unlock_irqrestore(&dpaa2_chan->queue_lock, flags);

	comp_temp->qchan = dpaa2_chan;
err:
	return comp_temp;
}

static void dpaa2_qdma_populate_fd(uint32_t format,
		struct dpaa2_qdma_comp *dpaa2_comp)
{
	struct dpaa2_fd *fd;

	fd = (struct dpaa2_fd *)dpaa2_comp->fd_virt_addr;
	memset(fd, 0, sizeof(struct dpaa2_fd));

	/* fd populated */
	fd->simple.addr_lo = dpaa2_comp->fl_bus_addr;
	fd->simple.addr_hi = (dpaa2_comp->fl_bus_addr >> 32);
	/* Bypass memory translation, Frame list format, short length disable */
	fd->simple.bpid_offset = QMAN_FD_FMT_ENABLE |
				QMAN_FD_BMT_ENABLE | QMAN_FD_SL_DISABLE;

	fd->simple.frc = format | QDMA_SER_CTX | QDMA_FD_SPF_ENALBE;
	fd->simple.ctrl = QMAN_FD_VA_DISABLE |
			QMAN_FD_CBMT_ENABLE | QMAN_FD_SC_DISABLE;
}

/* first frame list for descriptor buffer */
static void dpaa2_qdma_populate_first_framel(
		struct dpaa2_frame_list *f_list,
		struct dpaa2_qdma_comp *dpaa2_comp)
{
	struct dpaa2_qdma_sd_d *sdd;

	sdd = (struct dpaa2_qdma_sd_d *)dpaa2_comp->desc_virt_addr;
	/* source and destination descriptor */
	sdd->cmd = CMD_TTYPE_RW; /* source descriptor CMD */
	sdd++;
	sdd->cmd = CMD_TTYPE_RW; /* destination descriptor CMD */

	memset(f_list, 0, sizeof(struct dpaa2_frame_list));
	/* first frame list to source descriptor */
	f_list->addr_lo = dpaa2_comp->desc_bus_addr;
	f_list->addr_hi = (dpaa2_comp->desc_bus_addr >> 32);
	f_list->data_len.data_len_sl0 = 0x20; /* source/destination desc len */
	f_list->fmt = QDMA_FL_FMT_SBF; /* single buffer frame */
	f_list->bmt = QDMA_FL_BMT_ENABLE; /* bypass memory translation */
	f_list->sl = QDMA_FL_SL_LONG; /* long length */
	f_list->f = 0; /* not the last frame list */
}

/* source and destination frame list */
static void dpaa2_qdma_populate_frames(struct dpaa2_frame_list *f_list,
		dma_addr_t dst, dma_addr_t src, size_t len, uint8_t fmt)
{
	/* source frame list to source buffer */
	memset(f_list, 0, sizeof(struct dpaa2_frame_list));
	f_list->addr_lo = src;
	f_list->addr_hi = (src >> 32);
	f_list->data_len.data_len_sl0 = len;
	f_list->fmt = fmt; /* single buffer frame or scatter gather frame */
	f_list->bmt = QDMA_FL_BMT_ENABLE; /* bypass memory translation */
	f_list->sl = QDMA_FL_SL_LONG; /* long length */
	f_list->f = 0; /* not the last frame list */

	f_list++;
	/* destination frame list to destination buffer */
	memset(f_list, 0, sizeof(struct dpaa2_frame_list));
	f_list->addr_lo = dst;
	f_list->addr_hi = (dst >> 32);
	f_list->data_len.data_len_sl0 = len;
	f_list->fmt = fmt; /* single buffer frame or scatter gather frame */
	f_list->bmt = QDMA_FL_BMT_ENABLE; /* bypass memory translation */
	f_list->sl = QDMA_FL_SL_LONG; /* long length */
	f_list->f = QDMA_FL_F; /* Final bit: 1, for last frame list */
}

static struct dma_async_tx_descriptor *dpaa2_qdma_prep_memcpy(
		struct dma_chan *chan, dma_addr_t dst,
		dma_addr_t src, size_t len, unsigned long flags)
{
	struct dpaa2_qdma_chan *dpaa2_chan = to_dpaa2_qdma_chan(chan);
	struct dpaa2_qdma_comp *dpaa2_comp;
	struct dpaa2_frame_list *f_list;
	uint32_t format;

	dpaa2_comp = dpaa2_qdma_request_desc(dpaa2_chan);

#ifdef LONG_FORMAT
	format = QDMA_FD_LONG_FORMAT;
#else
	format = QDMA_FD_SHORT_FORMAT;
#endif
	/* populate Frame descriptor */
	dpaa2_qdma_populate_fd(format, dpaa2_comp);

	f_list = (struct dpaa2_frame_list *)dpaa2_comp->fl_virt_addr;

#ifdef LONG_FORMAT
	/* first frame list for descriptor buffer (logn format) */
	dpaa2_qdma_populate_first_framel(f_list, dpaa2_comp);

	f_list++;
#endif

	dpaa2_qdma_populate_frames(f_list, dst, src, len, QDMA_FL_FMT_SBF);

	return vchan_tx_prep(&dpaa2_chan->vchan, &dpaa2_comp->vdesc, flags);
}

static struct qdma_sg_blk *dpaa2_qdma_get_sg_blk(
	struct dpaa2_qdma_comp *dpaa2_comp,
	struct dpaa2_qdma_chan *dpaa2_chan)
{
	struct qdma_sg_blk *sg_blk = NULL;
	dma_addr_t phy_sgb;
	unsigned long flags;

	spin_lock_irqsave(&dpaa2_chan->queue_lock, flags);
	if (list_empty(&dpaa2_chan->sgb_free)) {
		sg_blk = (struct qdma_sg_blk *)dma_pool_alloc(
				dpaa2_chan->sg_blk_pool,
				GFP_NOWAIT, &phy_sgb);
		if (!sg_blk) {
			spin_unlock_irqrestore(&dpaa2_chan->queue_lock, flags);
			return sg_blk;
		}
		sg_blk->blk_virt_addr = (void *)(sg_blk + 1);
		sg_blk->blk_bus_addr = phy_sgb + sizeof(*sg_blk);
	} else {
		sg_blk = list_first_entry(&dpaa2_chan->sgb_free,
			struct qdma_sg_blk, list);
		list_del(&sg_blk->list);
	}
	spin_unlock_irqrestore(&dpaa2_chan->queue_lock, flags);

	return sg_blk;
}

static uint32_t dpaa2_qdma_populate_sg(struct device *dev,
		struct dpaa2_qdma_chan *dpaa2_chan,
		struct dpaa2_qdma_comp *dpaa2_comp,
		struct scatterlist *dst_sg, u32 dst_nents,
		struct scatterlist *src_sg, u32 src_nents)
{
	struct dpaa2_qdma_sg *src_sge;
	struct dpaa2_qdma_sg *dst_sge;
	struct qdma_sg_blk *sg_blk;
	struct qdma_sg_blk *sg_blk_dst;
	dma_addr_t src;
	dma_addr_t dst;
	uint32_t num;
	uint32_t blocks;
	uint32_t len = 0;
	uint32_t total_len = 0;
	int i, j = 0;

	num = min(dst_nents, src_nents);
	blocks = num / (NUM_SG_PER_BLK - 1);
	if (num % (NUM_SG_PER_BLK - 1))
		blocks += 1;
	if (dpaa2_comp->sg_blk_num < blocks) {
		len = blocks - dpaa2_comp->sg_blk_num;
		for (i = 0; i < len; i++) {
			/* source sg blocks */
			sg_blk = dpaa2_qdma_get_sg_blk(dpaa2_comp, dpaa2_chan);
			if (!sg_blk)
				return 0;
			list_add_tail(&sg_blk->list, &dpaa2_comp->sg_src_head);
			/* destination sg blocks */
			sg_blk = dpaa2_qdma_get_sg_blk(dpaa2_comp, dpaa2_chan);
			if (!sg_blk)
				return 0;
			list_add_tail(&sg_blk->list, &dpaa2_comp->sg_dst_head);
		}
	} else {
		len = dpaa2_comp->sg_blk_num - blocks;
		for (i = 0; i < len; i++) {
			spin_lock(&dpaa2_chan->queue_lock);
			/* handle source sg blocks */
			sg_blk = list_first_entry(&dpaa2_comp->sg_src_head,
					struct qdma_sg_blk, list);
			list_del(&sg_blk->list);
			list_add_tail(&sg_blk->list, &dpaa2_chan->sgb_free);
			/* handle destination sg blocks */
			sg_blk = list_first_entry(&dpaa2_comp->sg_dst_head,
					struct qdma_sg_blk, list);
			list_del(&sg_blk->list);
			list_add_tail(&sg_blk->list, &dpaa2_chan->sgb_free);
			spin_unlock(&dpaa2_chan->queue_lock);
		}
	}
	dpaa2_comp->sg_blk_num = blocks;

	/* get the first source sg phy address */
	sg_blk = list_first_entry(&dpaa2_comp->sg_src_head,
				struct qdma_sg_blk, list);
	dpaa2_comp->sge_src_bus_addr = sg_blk->blk_bus_addr;
	/* get the first destinaiton sg phy address */
	sg_blk_dst = list_first_entry(&dpaa2_comp->sg_dst_head,
				struct qdma_sg_blk, list);
	dpaa2_comp->sge_dst_bus_addr = sg_blk_dst->blk_bus_addr;

	for (i = 0; i < blocks; i++) {
		src_sge = (struct dpaa2_qdma_sg *)sg_blk->blk_virt_addr;
		dst_sge = (struct dpaa2_qdma_sg *)sg_blk_dst->blk_virt_addr;

		for (j = 0; j < (NUM_SG_PER_BLK - 1); j++) {
			len = min(sg_dma_len(dst_sg), sg_dma_len(src_sg));
			if (0 == len)
				goto fetch;
			total_len += len;
			src = sg_dma_address(src_sg);
			dst = sg_dma_address(dst_sg);

			/* source SG */
			src_sge->addr_lo = src;
			src_sge->addr_hi = (src >> 32);
			src_sge->data_len.data_len_sl0 = len;
			src_sge->ctrl.sl = QDMA_SG_SL_LONG;
			src_sge->ctrl.fmt = QDMA_SG_FMT_SDB;
			/* destination SG */
			dst_sge->addr_lo = dst;
			dst_sge->addr_hi = (dst >> 32);
			dst_sge->data_len.data_len_sl0 = len;
			dst_sge->ctrl.sl = QDMA_SG_SL_LONG;
			dst_sge->ctrl.fmt = QDMA_SG_FMT_SDB;
fetch:
			num--;
			if (0 == num) {
				src_sge->ctrl.f = QDMA_SG_F;
				dst_sge->ctrl.f = QDMA_SG_F;
				goto end;
			}
			dst_sg = sg_next(dst_sg);
			src_sg = sg_next(src_sg);
			src_sge++;
			dst_sge++;
			if (j == (NUM_SG_PER_BLK - 2)) {
				/* for next blocks, extension */
				sg_blk = list_next_entry(sg_blk, list);
				sg_blk_dst = list_next_entry(sg_blk_dst, list);
				src_sge->addr_lo = sg_blk->blk_bus_addr;
				src_sge->addr_hi = sg_blk->blk_bus_addr >> 32;
				src_sge->ctrl.sl = QDMA_SG_SL_LONG;
				src_sge->ctrl.fmt = QDMA_SG_FMT_SGTE;
				dst_sge->addr_lo = sg_blk_dst->blk_bus_addr;
				dst_sge->addr_hi =
					sg_blk_dst->blk_bus_addr >> 32;
				dst_sge->ctrl.sl = QDMA_SG_SL_LONG;
				dst_sge->ctrl.fmt = QDMA_SG_FMT_SGTE;
			}
		}
	}

end:
	return total_len;
}

static struct dma_async_tx_descriptor *dpaa2_qdma_prep_sg(
		struct dma_chan *chan,
		struct scatterlist *dst_sg, u32 dst_nents,
		struct scatterlist *src_sg, u32 src_nents,
		unsigned long flags)
{
	struct dpaa2_qdma_chan *dpaa2_chan = to_dpaa2_qdma_chan(chan);
	struct dpaa2_qdma_comp *dpaa2_comp;
	struct dpaa2_frame_list *f_list;
	struct device *dev = dpaa2_chan->qdma->priv->dev;
	uint32_t total_len = 0;

	/* basic sanity checks */
	if (dst_nents == 0 || src_nents == 0)
		return NULL;

	if (dst_sg == NULL || src_sg == NULL)
		return NULL;

	/* get the descriptors required */
	dpaa2_comp = dpaa2_qdma_request_desc(dpaa2_chan);

	/* populate Frame descriptor */
	dpaa2_qdma_populate_fd(QDMA_FD_LONG_FORMAT, dpaa2_comp);

	/* prepare Scatter gather entry for source and destination */
	total_len = dpaa2_qdma_populate_sg(dev, dpaa2_chan,
			dpaa2_comp, dst_sg, dst_nents, src_sg, src_nents);

	f_list = (struct dpaa2_frame_list *)dpaa2_comp->fl_virt_addr;
	/* first frame list for descriptor buffer */
	dpaa2_qdma_populate_first_framel(f_list, dpaa2_comp);
	f_list++;
	/* prepare Scatter gather entry for source and destination */
	/* populate source and destination frame list table */
	dpaa2_qdma_populate_frames(f_list, dpaa2_comp->sge_dst_bus_addr,
			dpaa2_comp->sge_src_bus_addr,
			total_len, QDMA_FL_FMT_SGE);

	return vchan_tx_prep(&dpaa2_chan->vchan, &dpaa2_comp->vdesc, flags);
}

static enum dma_status dpaa2_qdma_tx_status(struct dma_chan *chan,
		dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	return dma_cookie_status(chan, cookie, txstate);
}

static void dpaa2_qdma_free_desc(struct virt_dma_desc *vdesc)
{
}

static void dpaa2_qdma_issue_pending(struct dma_chan *chan)
{
	struct dpaa2_qdma_comp *dpaa2_comp;
	struct dpaa2_qdma_chan *dpaa2_chan = to_dpaa2_qdma_chan(chan);
	struct dpaa2_qdma_engine *dpaa2_qdma = dpaa2_chan->qdma;
	struct dpaa2_qdma_priv *priv = dpaa2_qdma->priv;
	struct virt_dma_desc *vdesc;
	struct dpaa2_fd *fd;
	int err;
	unsigned long flags;

	spin_lock_irqsave(&dpaa2_chan->queue_lock, flags);
	spin_lock(&dpaa2_chan->vchan.lock);
	if (vchan_issue_pending(&dpaa2_chan->vchan)) {
		vdesc = vchan_next_desc(&dpaa2_chan->vchan);
		if (!vdesc)
			goto err_enqueue;
		dpaa2_comp = to_fsl_qdma_comp(vdesc);

		fd = (struct dpaa2_fd *)dpaa2_comp->fd_virt_addr;

		list_del(&vdesc->node);
		list_add_tail(&dpaa2_comp->list, &dpaa2_chan->comp_used);

		/* TOBO: priority hard-coded to zero */
		err = dpaa2_io_service_enqueue_fq(NULL,
				priv->tx_queue_attr[0].fqid, fd);
		if (err) {
			list_del(&dpaa2_comp->list);
			list_add_tail(&dpaa2_comp->list,
				&dpaa2_chan->comp_free);
		}

	}
err_enqueue:
	spin_unlock(&dpaa2_chan->vchan.lock);
	spin_unlock_irqrestore(&dpaa2_chan->queue_lock, flags);
}

static int __cold dpaa2_qdma_setup(struct fsl_mc_device *ls_dev)
{
	struct device *dev = &ls_dev->dev;
	struct dpaa2_qdma_priv *priv;
	struct dpaa2_qdma_priv_per_prio *ppriv;
	uint8_t prio_def = DPDMAI_PRIO_NUM;
	int err;
	int i;

	priv = dev_get_drvdata(dev);

	priv->dev = dev;
	priv->dpqdma_id = ls_dev->obj_desc.id;

	/*Get the handle for the DPDMAI this interface is associate with */
	err = dpdmai_open(priv->mc_io, 0, priv->dpqdma_id, &ls_dev->mc_handle);
	if (err) {
		dev_err(dev, "dpdmai_open() failed\n");
		return err;
	}
	dev_info(dev, "Opened dpdmai object successfully\n");

	err = dpdmai_get_attributes(priv->mc_io, 0, ls_dev->mc_handle,
				&priv->dpdmai_attr);
	if (err) {
		dev_err(dev, "dpdmai_get_attributes() failed\n");
		return err;
	}

	if (priv->dpdmai_attr.version.major > DPDMAI_VER_MAJOR) {
		dev_err(dev, "DPDMAI major version mismatch\n"
			     "Found %u.%u, supported version is %u.%u\n",
				priv->dpdmai_attr.version.major,
				priv->dpdmai_attr.version.minor,
				DPDMAI_VER_MAJOR, DPDMAI_VER_MINOR);
	}

	if (priv->dpdmai_attr.version.minor > DPDMAI_VER_MINOR) {
		dev_err(dev, "DPDMAI minor version mismatch\n"
			     "Found %u.%u, supported version is %u.%u\n",
				priv->dpdmai_attr.version.major,
				priv->dpdmai_attr.version.minor,
				DPDMAI_VER_MAJOR, DPDMAI_VER_MINOR);
	}

	priv->num_pairs = min(priv->dpdmai_attr.num_of_priorities, prio_def);
	ppriv = kcalloc(priv->num_pairs, sizeof(*ppriv), GFP_KERNEL);
	if (!ppriv) {
		dev_err(dev, "kzalloc for ppriv failed\n");
		return -1;
	}
	priv->ppriv = ppriv;

	for (i = 0; i < priv->num_pairs; i++) {
		err = dpdmai_get_rx_queue(priv->mc_io, 0, ls_dev->mc_handle,
				i, &priv->rx_queue_attr[i]);
		if (err) {
			dev_err(dev, "dpdmai_get_rx_queue() failed\n");
			return err;
		}
		ppriv->rsp_fqid = priv->rx_queue_attr[i].fqid;

		err = dpdmai_get_tx_queue(priv->mc_io, 0, ls_dev->mc_handle,
				i, &priv->tx_queue_attr[i]);
		if (err) {
			dev_err(dev, "dpdmai_get_tx_queue() failed\n");
			return err;
		}
		ppriv->req_fqid = priv->tx_queue_attr[i].fqid;
		ppriv->prio = i;
		ppriv->priv = priv;
		ppriv++;
	}

	return 0;
}

static void dpaa2_qdma_fqdan_cb(struct dpaa2_io_notification_ctx *ctx)
{
	struct dpaa2_qdma_priv_per_prio *ppriv = container_of(ctx,
			struct dpaa2_qdma_priv_per_prio, nctx);
	struct dpaa2_qdma_priv *priv = ppriv->priv;
	struct dpaa2_qdma_comp *dpaa2_comp, *_comp_tmp;
	struct dpaa2_qdma_chan *qchan;
	const struct dpaa2_fd *fd;
	const struct dpaa2_fd *fd_eq;
	struct dpaa2_dq *dq;
	int err;
	int is_last = 0;
	uint8_t status;
	int i;
	int found;
	uint32_t n_chans = priv->dpaa2_qdma->n_chans;

	do {
		err = dpaa2_io_service_pull_fq(NULL, ppriv->rsp_fqid,
						ppriv->store);
	} while (err);

	while (!is_last) {
		do {
			dq = dpaa2_io_store_next(ppriv->store, &is_last);
		} while (!is_last && !dq);
		if (!dq) {
			dev_err(priv->dev, "FQID returned no valid frames!\n");
			continue;
		}

		/* obtain FD and process the error */
		fd = dpaa2_dq_fd(dq);
		status = fd->simple.ctrl & 0xff;
		if (status)
			dev_err(priv->dev, "FD error occurred\n");
		found = 0;
		for (i = 0; i < n_chans; i++) {
			qchan = &priv->dpaa2_qdma->chans[i];
			spin_lock(&qchan->queue_lock);
			if (list_empty(&qchan->comp_used)) {
				spin_unlock(&qchan->queue_lock);
				continue;
			}
			list_for_each_entry_safe(dpaa2_comp, _comp_tmp,
				&qchan->comp_used, list) {
				fd_eq = (struct dpaa2_fd *)
					dpaa2_comp->fd_virt_addr;

				if ((fd_eq->simple.addr_lo ==
					fd->simple.addr_lo) &&
					(fd_eq->simple.addr_hi ==
					fd->simple.addr_hi)) {

					list_del(&dpaa2_comp->list);
					list_add_tail(&dpaa2_comp->list,
							&qchan->comp_free);

					spin_lock(&qchan->vchan.lock);
					vchan_cookie_complete_for_qdma(
						&dpaa2_comp->vdesc);
					spin_unlock(&qchan->vchan.lock);
					found = 1;
					break;
				}
			}
			spin_unlock(&qchan->queue_lock);
			if (found)
				break;
		}
	}

	dpaa2_io_service_rearm(NULL, ctx);
}

static int __cold dpaa2_qdma_dpio_setup(struct dpaa2_qdma_priv *priv)
{
	int err, i, num;
	struct device *dev = priv->dev;
	struct dpaa2_qdma_priv_per_prio *ppriv;

	num = priv->num_pairs;
	ppriv = priv->ppriv;
	for (i = 0; i < num; i++) {
		ppriv->nctx.is_cdan = 0;
		ppriv->nctx.desired_cpu = -1;
		ppriv->nctx.id = ppriv->rsp_fqid;
		ppriv->nctx.cb = dpaa2_qdma_fqdan_cb;
		err = dpaa2_io_service_register(NULL, &ppriv->nctx);
		if (err) {
			dev_err(dev, "Notification register failed\n");
			goto err_service;
		}

		ppriv->store = dpaa2_io_store_create(DPAA2_QDMA_STORE_SIZE,
						dev);
		if (!ppriv->store) {
			dev_err(dev, "dpaa2_io_store_create() failed\n");
			goto err_store;
		}

		ppriv++;
	}
	return 0;

err_store:
	dpaa2_io_service_deregister(NULL, &ppriv->nctx);
err_service:
	ppriv--;
	while (ppriv >= priv->ppriv) {
		dpaa2_io_service_deregister(NULL, &ppriv->nctx);
		dpaa2_io_store_destroy(ppriv->store);
		ppriv--;
	}
	return -1;
}

static void __cold dpaa2_dpmai_store_free(struct dpaa2_qdma_priv *priv)
{
	struct dpaa2_qdma_priv_per_prio *ppriv = priv->ppriv;
	int i;

	for (i = 0; i < priv->num_pairs; i++) {
		dpaa2_io_store_destroy(ppriv->store);
		ppriv++;
	}
}

static void __cold dpaa2_dpdmai_dpio_free(struct dpaa2_qdma_priv *priv)
{
	struct dpaa2_qdma_priv_per_prio *ppriv = priv->ppriv;
	int i;

	for (i = 0; i < priv->num_pairs; i++) {
		dpaa2_io_service_deregister(NULL, &ppriv->nctx);
		ppriv++;
	}
}

static int __cold dpaa2_dpdmai_bind(struct dpaa2_qdma_priv *priv)
{
	int err;
	struct dpdmai_rx_queue_cfg rx_queue_cfg;
	struct device *dev = priv->dev;
	struct dpaa2_qdma_priv_per_prio *ppriv;
	struct fsl_mc_device *ls_dev = to_fsl_mc_device(dev);
	int i, num;

	num = priv->num_pairs;
	ppriv = priv->ppriv;
	for (i = 0; i < num; i++) {
		rx_queue_cfg.options = DPDMAI_QUEUE_OPT_USER_CTX |
					DPDMAI_QUEUE_OPT_DEST;
		rx_queue_cfg.user_ctx = ppriv->nctx.qman64;
		rx_queue_cfg.dest_cfg.dest_type = DPDMAI_DEST_DPIO;
		rx_queue_cfg.dest_cfg.dest_id = ppriv->nctx.dpio_id;
		rx_queue_cfg.dest_cfg.priority = ppriv->prio;
		err = dpdmai_set_rx_queue(priv->mc_io, 0, ls_dev->mc_handle,
				rx_queue_cfg.dest_cfg.priority, &rx_queue_cfg);
		if (err) {
			dev_err(dev, "dpdmai_set_rx_queue() failed\n");
			return err;
		}

		ppriv++;
	}

	return 0;
}

static int __cold dpaa2_dpdmai_dpio_unbind(struct dpaa2_qdma_priv *priv)
{
	int err = 0;
	struct device *dev = priv->dev;
	struct fsl_mc_device *ls_dev = to_fsl_mc_device(dev);
	struct dpaa2_qdma_priv_per_prio *ppriv = priv->ppriv;
	int i;

	for (i = 0; i < priv->num_pairs; i++) {
		ppriv->nctx.qman64 = 0;
		ppriv->nctx.dpio_id = 0;
		ppriv++;
	}

	err = dpdmai_reset(priv->mc_io, 0, ls_dev->mc_handle);
	if (err)
		dev_err(dev, "dpdmai_reset() failed\n");

	return err;
}

static void __cold dpaa2_dpdmai_free_pool(struct dpaa2_qdma_chan *qchan,
							struct list_head *head)
{
	struct qdma_sg_blk *sgb_tmp, *_sgb_tmp;
	/* free the QDMA SG pool block */
	list_for_each_entry_safe(sgb_tmp, _sgb_tmp, head, list) {
		sgb_tmp->blk_virt_addr = (void *)((struct qdma_sg_blk *)
						sgb_tmp->blk_virt_addr - 1);
		sgb_tmp->blk_bus_addr = sgb_tmp->blk_bus_addr
							- sizeof(*sgb_tmp);
		dma_pool_free(qchan->sg_blk_pool, sgb_tmp->blk_virt_addr,
						sgb_tmp->blk_bus_addr);
	}

}

static void __cold dpaa2_dpdmai_free_comp(struct dpaa2_qdma_chan *qchan,
						struct list_head *head)
{
	struct dpaa2_qdma_comp *comp_tmp, *_comp_tmp;
	/* free the QDMA comp resource */
	list_for_each_entry_safe(comp_tmp, _comp_tmp,
			head, list) {
		dma_pool_free(qchan->fd_pool,
			comp_tmp->fd_virt_addr,
			comp_tmp->fd_bus_addr);
		/* free the SG source block on comp */
		dpaa2_dpdmai_free_pool(qchan, &comp_tmp->sg_src_head);
		/* free the SG destination block on comp */
		dpaa2_dpdmai_free_pool(qchan, &comp_tmp->sg_dst_head);
		list_del(&comp_tmp->list);
		kfree(comp_tmp);
	}

}

static void __cold dpaa2_dpdmai_free_channels(
		struct dpaa2_qdma_engine *dpaa2_qdma)
{
	struct dpaa2_qdma_chan *qchan;
	int num, i;

	num = dpaa2_qdma->n_chans;
	for (i = 0; i < num; i++) {
		qchan = &dpaa2_qdma->chans[i];
		dpaa2_dpdmai_free_comp(qchan, &qchan->comp_used);
		dpaa2_dpdmai_free_comp(qchan, &qchan->comp_free);
		dpaa2_dpdmai_free_pool(qchan, &qchan->sgb_free);
		dma_pool_destroy(qchan->fd_pool);
		dma_pool_destroy(qchan->sg_blk_pool);
	}
}

static int dpaa2_dpdmai_alloc_channels(struct dpaa2_qdma_engine *dpaa2_qdma)
{
	struct dpaa2_qdma_chan *dpaa2_chan;
	struct device *dev = &dpaa2_qdma->priv->dpdmai_dev->dev;
	int i;

	INIT_LIST_HEAD(&dpaa2_qdma->dma_dev.channels);
	for (i = 0; i < dpaa2_qdma->n_chans; i++) {
		dpaa2_chan = &dpaa2_qdma->chans[i];
		dpaa2_chan->qdma = dpaa2_qdma;
		dpaa2_chan->vchan.desc_free = dpaa2_qdma_free_desc;
		vchan_init(&dpaa2_chan->vchan, &dpaa2_qdma->dma_dev);

		dpaa2_chan->fd_pool = dma_pool_create("fd_pool",
					dev, FD_POOL_SIZE, 32, 0);
		if (!dpaa2_chan->fd_pool)
			return -1;
		dpaa2_chan->sg_blk_pool = dma_pool_create("sg_blk_pool",
					dev, SG_POOL_SIZE, 32, 0);
		if (!dpaa2_chan->sg_blk_pool)
			return -1;

		spin_lock_init(&dpaa2_chan->queue_lock);
		INIT_LIST_HEAD(&dpaa2_chan->comp_used);
		INIT_LIST_HEAD(&dpaa2_chan->comp_free);
		INIT_LIST_HEAD(&dpaa2_chan->sgb_free);
	}
	return 0;
}

static int dpaa2_qdma_probe(struct fsl_mc_device *dpdmai_dev)
{
	struct dpaa2_qdma_priv *priv;
	struct device *dev = &dpdmai_dev->dev;
	struct dpaa2_qdma_engine *dpaa2_qdma;
	int err;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	dev_set_drvdata(dev, priv);
	priv->dpdmai_dev = dpdmai_dev;

	/* obtain a MC portal */
	err = fsl_mc_portal_allocate(dpdmai_dev, 0, &priv->mc_io);
	if (err) {
		dev_err(dev, "MC portal allocation failed\n");
		goto err_mcportal;
	}

	/* DPDMAI initialization */
	err = dpaa2_qdma_setup(dpdmai_dev);
	if (err) {
		dev_err(dev, "dpaa2_dpdmai_setup() failed\n");
		goto err_dpdmai_setup;
	}

	/* DPIO */
	err = dpaa2_qdma_dpio_setup(priv);
	if (err) {
		dev_err(dev, "dpaa2_dpdmai_dpio_setup() failed\n");
		goto err_dpio_setup;
	}

	/* DPDMAI binding to DPIO */
	err = dpaa2_dpdmai_bind(priv);
	if (err) {
		dev_err(dev, "dpaa2_dpdmai_bind() failed\n");
		goto err_bind;
	}

	/* DPDMAI enable */
	err = dpdmai_enable(priv->mc_io, 0, dpdmai_dev->mc_handle);
	if (err) {
		dev_err(dev, "dpdmai_enable() faile\n");
		goto err_enable;
	}

	dpaa2_qdma = kzalloc(sizeof(*dpaa2_qdma), GFP_KERNEL);
	if (!dpaa2_qdma) {
		err = -ENOMEM;
		goto err_eng;
	}

	priv->dpaa2_qdma = dpaa2_qdma;
	dpaa2_qdma->priv = priv;

	dpaa2_qdma->n_chans = NUM_CH;

	err = dpaa2_dpdmai_alloc_channels(dpaa2_qdma);
	if (err) {
		dev_err(dev, "QDMA alloc channels faile\n");
		goto err_reg;
	}

	dma_cap_set(DMA_PRIVATE, dpaa2_qdma->dma_dev.cap_mask);
	dma_cap_set(DMA_SLAVE, dpaa2_qdma->dma_dev.cap_mask);
	dma_cap_set(DMA_MEMCPY, dpaa2_qdma->dma_dev.cap_mask);
	dma_cap_set(DMA_SG, dpaa2_qdma->dma_dev.cap_mask);

	dpaa2_qdma->dma_dev.dev = dev;
	dpaa2_qdma->dma_dev.device_alloc_chan_resources
		= dpaa2_qdma_alloc_chan_resources;
	dpaa2_qdma->dma_dev.device_free_chan_resources
		= dpaa2_qdma_free_chan_resources;
	dpaa2_qdma->dma_dev.device_tx_status = dpaa2_qdma_tx_status;
	dpaa2_qdma->dma_dev.device_prep_dma_memcpy = dpaa2_qdma_prep_memcpy;
	dpaa2_qdma->dma_dev.device_prep_dma_sg = dpaa2_qdma_prep_sg;
	dpaa2_qdma->dma_dev.device_issue_pending = dpaa2_qdma_issue_pending;

	err = dma_async_device_register(&dpaa2_qdma->dma_dev);
	if (err) {
		dev_err(dev, "Can't register NXP QDMA engine.\n");
		goto err_reg;
	}

	return 0;

err_reg:
	dpaa2_dpdmai_free_channels(dpaa2_qdma);
	kfree(dpaa2_qdma);
err_eng:
	dpdmai_disable(priv->mc_io, 0, dpdmai_dev->mc_handle);
err_enable:
	dpaa2_dpdmai_dpio_unbind(priv);
err_bind:
	dpaa2_dpmai_store_free(priv);
	dpaa2_dpdmai_dpio_free(priv);
err_dpio_setup:
	dpdmai_close(priv->mc_io, 0, dpdmai_dev->mc_handle);
err_dpdmai_setup:
	fsl_mc_portal_free(priv->mc_io);
err_mcportal:
	kfree(priv->ppriv);
	kfree(priv);
	dev_set_drvdata(dev, NULL);
	return err;
}

static int dpaa2_qdma_remove(struct fsl_mc_device *ls_dev)
{
	struct device *dev;
	struct dpaa2_qdma_priv *priv;
	struct dpaa2_qdma_engine *dpaa2_qdma;

	dev = &ls_dev->dev;
	priv = dev_get_drvdata(dev);
	dpaa2_qdma = priv->dpaa2_qdma;

	dpdmai_disable(priv->mc_io, 0, ls_dev->mc_handle);
	dpaa2_dpdmai_dpio_unbind(priv);
	dpaa2_dpmai_store_free(priv);
	dpaa2_dpdmai_dpio_free(priv);
	dpdmai_close(priv->mc_io, 0, ls_dev->mc_handle);
	fsl_mc_portal_free(priv->mc_io);
	dev_set_drvdata(dev, NULL);
	dpaa2_dpdmai_free_channels(dpaa2_qdma);

	dma_async_device_unregister(&dpaa2_qdma->dma_dev);
	kfree(priv);
	kfree(dpaa2_qdma);

	return 0;
}

static const struct fsl_mc_device_match_id dpaa2_qdma_id_table[] = {
	{
		.vendor = FSL_MC_VENDOR_FREESCALE,
		.obj_type = "dpdmai",
	},
	{ .vendor = 0x0 }
};

static struct fsl_mc_driver dpaa2_qdma_driver = {
	.driver		= {
		.name	= "dpaa2-qdma",
		.owner  = THIS_MODULE,
	},
	.probe          = dpaa2_qdma_probe,
	.remove		= dpaa2_qdma_remove,
	.match_id_table	= dpaa2_qdma_id_table
};

static int __init dpaa2_qdma_driver_init(void)
{
	return fsl_mc_driver_register(&(dpaa2_qdma_driver));
}
late_initcall(dpaa2_qdma_driver_init);

static void __exit fsl_qdma_exit(void)
{
	fsl_mc_driver_unregister(&(dpaa2_qdma_driver));
}
module_exit(fsl_qdma_exit);

MODULE_DESCRIPTION("NXP DPAA2 qDMA driver");
MODULE_LICENSE("Dual BSD/GPL");
