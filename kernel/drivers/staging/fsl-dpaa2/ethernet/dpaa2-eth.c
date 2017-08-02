/* Copyright 2014-2015 Freescale Semiconductor Inc.
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
#include <inic_config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/etherdevice.h>
#include <linux/of_net.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/net_tstamp.h>

#include "../../fsl-mc/include/mc.h"
#include "../../fsl-mc/include/mc-sys.h"
#include "dpaa2-eth.h"

#if FSLU_NVME_INIC
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include<linux/hardirq.h>
#include<linux/irqdomain.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
unsigned long irq_start = 40;
uint64_t txpacket_count,drop_count;
static struct global_data
{
	uint64_t pci_config;
	uint64_t dma_virt;
	uint64_t dreg_config;
	uint64_t pci_outbound;
	uint64_t dma_msix;
	uint64_t dma_netreg;
	uint64_t priv[10];
	dma_addr_t dma_handle,dma_bkp;
#if FSLU_NVME_INIC_QDMA
	struct dma_chan *dma_chan;/*to store qdma engine dma channel */
	struct dma_device *dma_device;/*to store device having multiple channels*/
#endif
}global_mem;
static struct global_data *global = &global_mem;
#if FSLU_NVME_INIC_QDMA
/*rx side dma descriptor for call back function*/
struct rx_dma_desc {
	uint64_t src_addr;
	uint64_t dest_addr;
	uint64_t len_offset_flag;
	int rx_desc_offset;
	struct dpaa2_eth_priv *priv;
};
#endif

#endif /* FSLU_NVME_INIC */


/* CREATE_TRACE_POINTS only needs to be defined once. Other dpa files
 * using trace events only need to #include <trace/events/sched.h>
 */
#define CREATE_TRACE_POINTS
#include "dpaa2-eth-trace.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Freescale Semiconductor, Inc");
MODULE_DESCRIPTION("Freescale DPAA2 Ethernet Driver");


#if FSLU_NVME_INIC
#define SHARED_CONF_SIZE (128*1024*1024)
#define PEX3 0x3600000
/* Ring and Frame size -- these must match between the drivers */
#if FSLU_NVME_INIC_QDMA
#define PH_RING_SIZE    (16800)
#define RX_BD_RING_SIZE (16800)
#define RX_BUF_REUSE_COUNT 700
#endif
#if FSLU_NVME_INIC_DPAA2
#define PH_RING_SIZE    (168000)
#define RX_BD_RING_SIZE (84000)
#endif
#define REFILL_RING_SIZE 30
#define REFILL_POOL_SIZE 350
#define REFILL_THRESHOLD 350
#define MTU_INIC 4000
#define TX_ERROR 0x8
#define TX_CLEAR 0x4
/* Buffer Descriptor Registers */
#define INTERFACE_REG           0x300000
#define PCINET_TXBD_BASE        0x800
#define PCINET_RXBD_BASE        0x200000
#define PCINET_TXCONF_BASE      0x7fd00
#define BUF_CONF                0x5
#define BUF_RELEASE             0x6
#define IFACE_IDLE              0x7
#define IFACE_UP                0x8
#define IFACE_READY             0x9
#define IFACE_DOWN              0xa
#define IFACE_STOP              0xb
#define MAC_SET           0x02
#define MAC_NOTSET        0x00
/* MSI */
#define MSI_DISABLE             0x2
#define MSI_ENABLE              0x1
#define REFILL_CLEAR_REQUEST    0x1
#define REFILL_REQUEST          0x2
/* Buffer Descriptor Status */
#define BD_MEM_READY     0x1
#define BD_MEM_DIRTY     0x2
#define BD_MEM_FREE      0x4
#define BD_MEM_CONSUME   0x8
#define BD_IRQ_SET       0x4
#define BD_IRQ_MASK      0x8
#define BD_IRQ_CLEAR     0x2
#define LINK_STATE_UP    0x8
#define LINK_STATE_DOWN  0x4
#define LINK_STATUS  0x2
//#if FSLU_NVME_INIC_DPAA2
#define BP_PULL_SET  0x1
#define BP_PULL_DONE 0x2
#define BP_PULL_CLEAR 0x3
//#endif
#define PEX_OFFSET 0x3000000000
#define DPBP_TIMEOUT     600 /* 10 Min */
#define NUM_NET_DEV 4
static struct of_device_id nic_of_genirq_match[] = {
	{ .compatible = "arm,gic-v3", },
	{ },
};
static void  nic_open(struct work_struct *work);
static void  nic_stop(struct work_struct *work);
static int if_count=0;
/*----------------------------------------------------------------------------*/
/* VVDN gen MSI                                                               */
/*----------------------------------------------------------------------------*/

static inline void generate_msi(struct dpaa2_eth_priv *priv)
{

	iowrite32(priv->msix_value, priv->msix_addr);

}
/*----------------------------------------------------------------------------*/
/* Buffer Descriptor Accessor Helpers                                         */
/*----------------------------------------------------------------------------*/

static inline void cbd_write(void __iomem *addr, u32 val)
{
	iowrite32(val, addr);
}

static inline u32 cbd_read(void __iomem *addr)
{
	return ioread32(addr);
}

static inline void cbd_write16(void __iomem *addr, u16 val)
{
	iowrite16(val, addr);
}

static int dpaa2_dpbp_refill_custom(struct dpaa2_eth_priv *priv, uint16_t bpid);
#endif /* FSLU_NVME_INIC */



static void validate_rx_csum(struct dpaa2_eth_priv *priv,
		u32 fd_status,
		struct sk_buff *skb)
{
	skb_checksum_none_assert(skb);

	/* HW checksum validation is disabled, nothing to do here */
	if (!(priv->net_dev->features & NETIF_F_RXCSUM))
		return;

	/* Read checksum validation bits */
	if (!((fd_status & DPAA2_FAS_L3CV) &&
				(fd_status & DPAA2_FAS_L4CV)))
		return;

	/* Inform the stack there's no need to compute L3/L4 csum anymore */
	skb->ip_summed = CHECKSUM_UNNECESSARY;
}

/* Free a received FD.
 * Not to be used for Tx conf FDs or on any other paths.
 */
static void free_rx_fd(struct dpaa2_eth_priv *priv,
		const struct dpaa2_fd *fd,
		void *vaddr)
{
	struct device *dev = priv->net_dev->dev.parent;
	dma_addr_t addr = dpaa2_fd_get_addr(fd);
	u8 fd_format = dpaa2_fd_get_format(fd);
	struct dpaa2_sg_entry *sgt;
	void *sg_vaddr;
	int i;

	/* If single buffer frame, just free the data buffer */
	if (fd_format == dpaa2_fd_single)
		goto free_buf;

	/* For S/G frames, we first need to free all SG entries */
	sgt = vaddr + dpaa2_fd_get_offset(fd);
	for (i = 0; i < DPAA2_ETH_MAX_SG_ENTRIES; i++) {
		dpaa2_sg_le_to_cpu(&sgt[i]);

		addr = dpaa2_sg_get_addr(&sgt[i]);
		dma_unmap_single(dev, addr, DPAA2_ETH_RX_BUF_SIZE,
				DMA_FROM_DEVICE);

		sg_vaddr = phys_to_virt(addr);
		put_page(virt_to_head_page(sg_vaddr));

		if (dpaa2_sg_is_final(&sgt[i]))
			break;
	}

free_buf:
	put_page(virt_to_head_page(vaddr));
}

/* Build a linear skb based on a single-buffer frame descriptor */
static struct sk_buff *build_linear_skb(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_channel *ch,
		const struct dpaa2_fd *fd,
		void *fd_vaddr)
{
	struct sk_buff *skb = NULL;
	u16 fd_offset = dpaa2_fd_get_offset(fd);
	u32 fd_length = dpaa2_fd_get_len(fd);

	/* Leave enough extra space in the headroom to make sure the skb is
	 * not realloc'd in forwarding scenarios. This has been previously
	 * allocated when seeding the buffer pools.
	 */
	skb = build_skb(fd_vaddr - priv->rx_extra_head,
			DPAA2_ETH_SKB_SIZE(priv));
	if (unlikely(!skb))
		return NULL;

	skb_reserve(skb, fd_offset + priv->rx_extra_head);
	skb_put(skb, fd_length);

	ch->buf_count--;

	return skb;
}

/* Build a non linear (fragmented) skb based on a S/G table */
static struct sk_buff *build_frag_skb(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_channel *ch,
		struct dpaa2_sg_entry *sgt)
{
	struct sk_buff *skb = NULL;
	struct device *dev = priv->net_dev->dev.parent;
	void *sg_vaddr;
	dma_addr_t sg_addr;
	u16 sg_offset;
	u32 sg_length;
	struct page *page, *head_page;
	int page_offset;
	int i;

	for (i = 0; i < DPAA2_ETH_MAX_SG_ENTRIES; i++) {
		struct dpaa2_sg_entry *sge = &sgt[i];

		dpaa2_sg_le_to_cpu(sge);

		/* NOTE: We only support SG entries in dpaa2_sg_single format,
		 * but this is the only format we may receive from HW anyway
		 */

		/* Get the address and length from the S/G entry */
		sg_addr = dpaa2_sg_get_addr(sge);
		dma_unmap_single(dev, sg_addr, DPAA2_ETH_RX_BUF_SIZE,
				DMA_FROM_DEVICE);

		sg_vaddr = phys_to_virt(sg_addr);
		sg_length = dpaa2_sg_get_len(sge);

		if (i == 0) {
			/* We build the skb around the first data buffer */
			skb = build_skb(sg_vaddr, DPAA2_ETH_SKB_SIZE(priv));
			if (unlikely(!skb))
				return NULL;

			sg_offset = dpaa2_sg_get_offset(sge);
			skb_reserve(skb, sg_offset);
			skb_put(skb, sg_length);
		} else {
			/* Rest of the data buffers are stored as skb frags */
			page = virt_to_page(sg_vaddr);
			head_page = virt_to_head_page(sg_vaddr);

			/* Offset in page (which may be compound).
			 * Data in subsequent SG entries is stored from the
			 * beginning of the buffer, so we don't need to add the
			 * sg_offset.
			 */
			page_offset = ((unsigned long)sg_vaddr &
					(PAGE_SIZE - 1)) +
				(page_address(page) - page_address(head_page));

			skb_add_rx_frag(skb, i - 1, head_page, page_offset,
					sg_length, DPAA2_ETH_RX_BUF_SIZE);
		}

		if (dpaa2_sg_is_final(sge))
			break;
	}

	/* Count all data buffers + SG table buffer */
	ch->buf_count -= i + 2;

	return skb;
}

#if FSLU_NVME_INIC_QDMA
void rx_qdma_callback(void *arg)
{

	struct rx_dma_desc *ptd =(struct rx_dma_desc*)arg;
	spin_lock(&ptd->priv->rx_dma_lock);
	struct circ_buf_desc *bdp;
	ptd->len_offset_flag|=BD_MEM_DIRTY;
	bdp = (ptd->priv->rx_base + ptd->rx_desc_offset);
	cbd_write(&bdp->len_offset_flag,ptd->len_offset_flag);
	ptd->priv->rx_buf[ptd->priv->rx_buf_count++] = ptd->src_addr;
	/*refill  TODO  delaying callback should be minimal*/
	if(ptd->priv->rx_buf_count>=RX_BUF_REUSE_COUNT)
	{
		/*refill buff pool and reset the count */
		ptd->priv->rx_buf_count=0;
		dpaa2_dpbp_refill_custom(ptd->priv, ptd->priv->dpbp_attrs.bpid);
	}
	/*end*/
	if(ptd->priv->bman_buf->napi_msi != MSI_DISABLE) {
		generate_msi(ptd->priv);
	}
	kfree(ptd);
	spin_unlock(&ptd->priv->rx_dma_lock);
	return;
}
#endif

/* Main Rx frame processing routine */
static void dpaa2_eth_rx(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_channel *ch,
		const struct dpaa2_fd *fd,
		struct napi_struct *napi,
		u16 queue_id)
{

#if FSLU_NVME_INIC
#if FSLU_NVME_INIC_DPAA2
	struct rtnl_link_stats64 *percpu_stats;
	struct circ_buf_desc *bdp;
	uint32_t len_offset_flag =0;
	uint64_t addr = 0;
	spin_lock(&priv->rx_desc_lock);
	priv->rx_count++;
	bdp = priv->cur_rx;
	if(priv->rx_count >= REFILL_THRESHOLD)
	{
		dpaa2_dpbp_refill_custom(priv, priv->dpbp_attrs.bpid);
		priv->rx_count =0;
	}

	addr=(((uint64_t)dpaa2_fd_get_addr(fd))-(PEX_OFFSET));
	addr |=addr>>32;

	len_offset_flag=((uint32_t)dpaa2_fd_get_len(fd)<<16);
	len_offset_flag |= (dpaa2_fd_get_offset(fd)<<4);
	len_offset_flag |= BD_MEM_DIRTY;
	writeq(((uint64_t)addr<<32)|(len_offset_flag),&bdp->len_offset_flag);

	if ((bdp - priv->rx_base) == (RX_BD_RING_SIZE - 1)) {
		bdp = priv->rx_base;
	} else {
		bdp++;
	}
	priv->cur_rx = bdp;

	percpu_stats = this_cpu_ptr(priv->percpu_stats);
	percpu_stats->rx_packets++;

	if(priv->bman_buf->napi_msi != MSI_DISABLE) {
		generate_msi(priv);
	}

	spin_unlock(&priv->rx_desc_lock);
	return;
#endif/*FSLU_NVME_INIC_DPAA2*/
#if FSLU_NVME_INIC_QDMA
	struct rtnl_link_stats64 *percpu_stats;
	int i = 0;
	struct circ_buf_desc *bdp;
	spin_lock(&priv->rx_desc_lock);
	int len=0,offset=0;
	bdp = priv->cur_rx;
	struct dma_async_tx_descriptor *txd;
	dma_cookie_t cookie;
	struct rx_dma_desc *ptd;
	ptd=kzalloc(sizeof(struct rx_dma_desc),GFP_ATOMIC);
#if 1
	ptd->dest_addr=readq(&bdp->len_offset_flag);
	/* traffic exceeds ring size LS2 can't drop packets TODO */
	if ((ptd->dest_addr & 0xf) != BD_MEM_READY) {
		printk( "PCIE:%s RX packets are  overflow  \n",__func__);

	}
#endif

	ptd->dest_addr = (uint32_t)(ptd->dest_addr>>32);
	ptd->dest_addr |=((uint64_t)(ptd->dest_addr&0xf)<<32);
	ptd->dest_addr  = (uint64_t)(ptd->dest_addr & 0xfffffffffffffff0);
	ptd->dest_addr = ptd->dest_addr + PEX_OFFSET;
	ptd->src_addr = ((uint64_t)dpaa2_fd_get_addr(fd));
	len=dpaa2_fd_get_len(fd);
	offset=dpaa2_fd_get_offset(fd);
	ptd->rx_desc_offset=(bdp - priv->rx_base);
	ptd->priv=priv;
	ptd->len_offset_flag=((uint32_t)len<<16);
	ptd->len_offset_flag |= (offset<<4);

	txd = priv->dma_device->device_prep_dma_memcpy(priv->dma_chan,(ptd->dest_addr),ptd->src_addr,(len+offset),DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if (unlikely(!txd))
	{
		uint64_t dest_addr;
		dest_addr=ptd->dest_addr-PEX_OFFSET;
		dest_addr +=priv->g_outaddr;
		memcpy(dest_addr,phys_to_virt(ptd->src_addr),(len+offset));
		printk("%s qdma mapping error doing cpu memcpy \n",__func__);
		rx_qdma_callback(ptd);
		goto rx_dma_err;
	}
	txd->callback = rx_qdma_callback;
	txd->callback_param = ptd;
	cookie = dmaengine_submit(txd);
	if (unlikely(dma_submit_error(cookie))) {
		uint64_t dest_addr;
		dest_addr=ptd->dest_addr-PEX_OFFSET;
		dest_addr +=priv->g_outaddr;
		memcpy(dest_addr,phys_to_virt(ptd->src_addr),(len+offset));
		printk("%s qdma dma submit error doing cpu memcpy \n",__func__);
		rx_qdma_callback(ptd);
		goto rx_dma_err;
	}
	dma_async_issue_pending(priv->dma_chan);

rx_dma_err:
	if ((bdp - priv->rx_base) == (RX_BD_RING_SIZE - 1)) {
		bdp = priv->rx_base;
	} else {
		bdp++;
	}
	priv->cur_rx = bdp;

	percpu_stats = this_cpu_ptr(priv->percpu_stats);
	percpu_stats->rx_packets++;

	spin_unlock(&priv->rx_desc_lock);
	return;
#endif /*FSLU_NVME_INIC_QDMA*/

#else
	dma_addr_t addr = dpaa2_fd_get_addr(fd);
	u8 fd_format = dpaa2_fd_get_format(fd);
	void *vaddr;
	struct sk_buff *skb;
	struct rtnl_link_stats64 *percpu_stats;
	struct dpaa2_eth_drv_stats *percpu_extras;
	struct device *dev = priv->net_dev->dev.parent;
	struct dpaa2_fas *fas;
	u32 status = 0;

	/* Tracing point */
	trace_dpaa2_rx_fd(priv->net_dev, fd);

	dma_unmap_single(dev, addr, DPAA2_ETH_RX_BUF_SIZE, DMA_FROM_DEVICE);
	vaddr = phys_to_virt(addr);

	/* HWA - FAS, timestamp */
	prefetch(vaddr + priv->buf_layout.private_data_size);
	/* data / SG table */
	prefetch(vaddr + dpaa2_fd_get_offset(fd));

	percpu_stats = this_cpu_ptr(priv->percpu_stats);
	percpu_extras = this_cpu_ptr(priv->percpu_extras);

	if (fd_format == dpaa2_fd_single) {
		skb = build_linear_skb(priv, ch, fd, vaddr);
	} else if (fd_format == dpaa2_fd_sg) {
		struct dpaa2_sg_entry *sgt =
			vaddr + dpaa2_fd_get_offset(fd);
		skb = build_frag_skb(priv, ch, sgt);

		/* prefetch newly built skb data */
		prefetch(skb->data);

		put_page(virt_to_head_page(vaddr));
		percpu_extras->rx_sg_frames++;
		percpu_extras->rx_sg_bytes += dpaa2_fd_get_len(fd);
	} else {
		/* We don't support any other format */
		goto err_frame_format;
	}

	if (unlikely(!skb))
		goto err_build_skb;

	/* Get the timestamp value */
	if (priv->ts_rx_en) {
		struct skb_shared_hwtstamps *shhwtstamps = skb_hwtstamps(skb);
		u64 *ns = (u64 *)(vaddr +
				priv->buf_layout.private_data_size +
				sizeof(struct dpaa2_fas));

		*ns = DPAA2_PTP_NOMINAL_FREQ_PERIOD_NS * le64_to_cpup(ns);
		memset(shhwtstamps, 0, sizeof(*shhwtstamps));
		shhwtstamps->hwtstamp = ns_to_ktime(*ns);
	}

	/* Check if we need to validate the L4 csum */
	if (likely(fd->simple.frc & DPAA2_FD_FRC_FASV)) {
		fas = (struct dpaa2_fas *)
			(vaddr + priv->buf_layout.private_data_size);
		status = le32_to_cpu(fas->status);
		validate_rx_csum(priv, status, skb);
	}

	skb->protocol = eth_type_trans(skb, priv->net_dev);

	/* Record Rx queue - this will be used when picking a Tx queue to
	 * forward the frames. We're keeping flow affinity through the
	 * network stack.
	 */
	skb_record_rx_queue(skb, queue_id);

	percpu_stats->rx_packets++;
	percpu_stats->rx_bytes += skb->len;

	if (priv->net_dev->features & NETIF_F_GRO)
		napi_gro_receive(napi, skb);
	else
		netif_receive_skb(skb);

	return;
err_frame_format:
err_build_skb:
	free_rx_fd(priv, fd, vaddr);
	percpu_stats->rx_dropped++;
#endif /* FSLU_NVME_INIC */
}

#ifdef CONFIG_FSL_DPAA2_ETH_USE_ERR_QUEUE
/* Processing of Rx frames received on the error FQ
 * We check and print the error bits and then free the frame
 */
static void dpaa2_eth_rx_err(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_channel *ch,
		const struct dpaa2_fd *fd,
		struct napi_struct *napi __always_unused,
		u16 queue_id __always_unused)
{
	struct device *dev = priv->net_dev->dev.parent;
	dma_addr_t addr = dpaa2_fd_get_addr(fd);
	void *vaddr;
	struct rtnl_link_stats64 *percpu_stats;
	struct dpaa2_fas *fas;
	u32 status = 0;
	bool check_fas_errors = false;

	dma_unmap_single(dev, addr, DPAA2_ETH_RX_BUF_SIZE, DMA_FROM_DEVICE);
	vaddr = phys_to_virt(addr);

	/* check frame errors in the FD field */
	if (fd->simple.ctrl & DPAA2_FD_RX_ERR_MASK) {
		check_fas_errors = !!(fd->simple.ctrl & DPAA2_FD_CTRL_FAERR) &&
			!!(fd->simple.frc & DPAA2_FD_FRC_FASV);
		if (net_ratelimit())
			netdev_dbg(priv->net_dev, "Rx frame FD err: %x08\n",
					fd->simple.ctrl & DPAA2_FD_RX_ERR_MASK);
	}

	/* check frame errors in the FAS field */
	if (check_fas_errors) {
		fas = (struct dpaa2_fas *)
			(vaddr + priv->buf_layout.private_data_size);
		status = le32_to_cpu(fas->status);
		if (net_ratelimit())
			netdev_dbg(priv->net_dev, "Rx frame FAS err: 0x%08x\n",
					status & DPAA2_FAS_RX_ERR_MASK);
	}
	free_rx_fd(priv, fd, vaddr);

	percpu_stats = this_cpu_ptr(priv->percpu_stats);
	percpu_stats->rx_errors++;
}
#endif

/* Consume all frames pull-dequeued into the store. This is the simplest way to
 * make sure we don't accidentally issue another volatile dequeue which would
 * overwrite (leak) frames already in the store.
 *
 * The number of frames is returned using the last 2 output arguments,
 * separately for Rx and Tx confirmations.
 *
 * Observance of NAPI budget is not our concern, leaving that to the caller.
 */
static bool consume_frames(struct dpaa2_eth_channel *ch, int *rx_cleaned,
		int *tx_conf_cleaned)
{
	struct dpaa2_eth_priv *priv = ch->priv;
	struct dpaa2_eth_fq *fq;
	struct dpaa2_dq *dq;
	const struct dpaa2_fd *fd;
	bool has_cleaned = false;
	int is_last;

	do {
		dq = dpaa2_io_store_next(ch->store, &is_last);
		if (unlikely(!dq)) {
			/* If we're here, we *must* have placed a
			 * volatile dequeue comnmand, so keep reading through
			 * the store until we get some sort of valid response
			 * token (either a valid frame or an "empty dequeue")
			 */
			continue;
		}

		fd = dpaa2_dq_fd(dq);

		/* prefetch the frame descriptor */
		prefetch(fd);

		fq = (struct dpaa2_eth_fq *)dpaa2_dq_fqd_ctx(dq);
		fq->stats.frames++;

		fq->consume(priv, ch, fd, &ch->napi, fq->flowid);
		has_cleaned = true;

		if (fq->type == DPAA2_TX_CONF_FQ)
			(*tx_conf_cleaned)++;
		else
			(*rx_cleaned)++;
	} while (!is_last);

	return has_cleaned;
}

#if !(FSLU_NVME_INIC)
/* Configure the egress frame annotation for timestamp update */
static void enable_tx_tstamp(struct dpaa2_fd *fd, void *hwa_start)
{
	struct dpaa2_faead *faead;
	u32 ctrl;
	u32 frc;

	/* Mark the egress frame annotation area as valid */
	frc = dpaa2_fd_get_frc(fd);
	dpaa2_fd_set_frc(fd, frc | DPAA2_FD_FRC_FAEADV);

	/* enable UPD (update prepanded data) bit in FAEAD field of
	 * hardware frame annotation area
	 */
	ctrl = DPAA2_FAEAD_A2V | DPAA2_FAEAD_UPDV | DPAA2_FAEAD_UPD;
	faead = hwa_start + DPAA2_FAEAD_OFFSET;
	faead->ctrl = cpu_to_le32(ctrl);
}

/* Create a frame descriptor based on a fragmented skb */
static int build_sg_fd(struct dpaa2_eth_priv *priv,
		struct sk_buff *skb,
		struct dpaa2_fd *fd)
{
	struct device *dev = priv->net_dev->dev.parent;
	void *sgt_buf = NULL;
	void *hwa;
	dma_addr_t addr;
	int nr_frags = skb_shinfo(skb)->nr_frags;
	struct dpaa2_sg_entry *sgt;
	int i, j, err;
	int sgt_buf_size;
	struct scatterlist *scl, *crt_scl;
	int num_sg;
	int num_dma_bufs;
	struct dpaa2_eth_swa *swa;

	/* Create and map scatterlist.
	 * We don't advertise NETIF_F_FRAGLIST, so skb_to_sgvec() will not have
	 * to go beyond nr_frags+1.
	 * Note: We don't support chained scatterlists
	 */
	if (unlikely(PAGE_SIZE / sizeof(struct scatterlist) < nr_frags + 1))
		return -EINVAL;

	scl = kcalloc(nr_frags + 1, sizeof(struct scatterlist), GFP_ATOMIC);
	if (unlikely(!scl))
		return -ENOMEM;

	sg_init_table(scl, nr_frags + 1);
	num_sg = skb_to_sgvec(skb, scl, 0, skb->len);
	num_dma_bufs = dma_map_sg(dev, scl, num_sg, DMA_TO_DEVICE);
	if (unlikely(!num_dma_bufs)) {
		err = -ENOMEM;
		goto dma_map_sg_failed;
	}

	/* Prepare the HW SGT structure */
	sgt_buf_size = priv->tx_data_offset +
		sizeof(struct dpaa2_sg_entry) * (1 + num_dma_bufs);
	sgt_buf = kzalloc(sgt_buf_size + DPAA2_ETH_TX_BUF_ALIGN, GFP_ATOMIC);
	if (unlikely(!sgt_buf)) {
		err = -ENOMEM;
		goto sgt_buf_alloc_failed;
	}
	sgt_buf = PTR_ALIGN(sgt_buf, DPAA2_ETH_TX_BUF_ALIGN);

	/* PTA from egress side is passed as is to the confirmation side so
	 * we need to clear some fields here in order to find consistent values
	 * on TX confirmation. We are clearing FAS (Frame Annotation Status)
	 * field here.
	 */
	hwa = sgt_buf + priv->buf_layout.private_data_size;
	memset(hwa, 0, 8);

	sgt = (struct dpaa2_sg_entry *)(sgt_buf + priv->tx_data_offset);

	/* Fill in the HW SGT structure.
	 *
	 * sgt_buf is zeroed out, so the following fields are implicit
	 * in all sgt entries:
	 *   - offset is 0
	 *   - format is 'dpaa2_sg_single'
	 */
	for_each_sg(scl, crt_scl, num_dma_bufs, i) {
		dpaa2_sg_set_addr(&sgt[i], sg_dma_address(crt_scl));
		dpaa2_sg_set_len(&sgt[i], sg_dma_len(crt_scl));
	}
	dpaa2_sg_set_final(&sgt[i - 1], true);

	/* Store the skb backpointer in the SGT buffer.
	 * Fit the scatterlist and the number of buffers alongside the
	 * skb backpointer in the SWA. We'll need all of them on Tx Conf.
	 */
	swa = (struct dpaa2_eth_swa *)sgt_buf;
	swa->skb = skb;
	swa->scl = scl;
	swa->num_sg = num_sg;
	swa->num_dma_bufs = num_dma_bufs;

	/* Hardware expects the SG table to be in little endian format */
	for (j = 0; j < i; j++)
		dpaa2_sg_cpu_to_le(&sgt[j]);

	/* Separately map the SGT buffer */
	addr = dma_map_single(dev, sgt_buf, sgt_buf_size, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev, addr))) {
		err = -ENOMEM;
		goto dma_map_single_failed;
	}
	dpaa2_fd_set_offset(fd, priv->tx_data_offset);
	dpaa2_fd_set_format(fd, dpaa2_fd_sg);
	dpaa2_fd_set_addr(fd, addr);
	dpaa2_fd_set_len(fd, skb->len);

	fd->simple.ctrl = DPAA2_FD_CTRL_ASAL | DPAA2_FD_CTRL_PTA |
		DPAA2_FD_CTRL_PTV1;

	if (priv->ts_tx_en && skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP)
		enable_tx_tstamp(fd, hwa);

	return 0;

dma_map_single_failed:
	kfree(sgt_buf);
sgt_buf_alloc_failed:
	dma_unmap_sg(dev, scl, num_sg, DMA_TO_DEVICE);
dma_map_sg_failed:
	kfree(scl);
	return err;
}

/* Create a frame descriptor based on a linear skb */
static int build_single_fd(struct dpaa2_eth_priv *priv,
		struct sk_buff *skb,
		struct dpaa2_fd *fd)
{
	struct device *dev = priv->net_dev->dev.parent;
	u8 *buffer_start;
	struct sk_buff **skbh;
	dma_addr_t addr;
	void *hwa;

	buffer_start = PTR_ALIGN(skb->data - priv->tx_data_offset -
			DPAA2_ETH_TX_BUF_ALIGN,
			DPAA2_ETH_TX_BUF_ALIGN);

	/* PTA from egress side is passed as is to the confirmation side so
	 * we need to clear some fields here in order to find consistent values
	 * on TX confirmation. We are clearing FAS (Frame Annotation Status)
	 * field here
	 */
	hwa = buffer_start + priv->buf_layout.private_data_size;
	memset(hwa, 0, 8);

	/* Store a backpointer to the skb at the beginning of the buffer
	 * (in the private data area) such that we can release it
	 * on Tx confirm
	 */
	skbh = (struct sk_buff **)buffer_start;
	*skbh = skb;

	addr = dma_map_single(dev, buffer_start,
			skb_tail_pointer(skb) - buffer_start,
			DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev, addr)))
		return -ENOMEM;

	dpaa2_fd_set_addr(fd, addr);
	dpaa2_fd_set_offset(fd, (u16)(skb->data - buffer_start));
	dpaa2_fd_set_len(fd, skb->len);
	dpaa2_fd_set_format(fd, dpaa2_fd_single);

	fd->simple.ctrl = DPAA2_FD_CTRL_ASAL | DPAA2_FD_CTRL_PTA |
		DPAA2_FD_CTRL_PTV1;

	if (priv->ts_tx_en && skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP)
		enable_tx_tstamp(fd, hwa);

	return 0;
}

/* FD freeing routine on the Tx path
 *
 * DMA-unmap and free FD and possibly SGT buffer allocated on Tx. The skb
 * back-pointed to is also freed.
 * This can be called either from dpaa2_eth_tx_conf() or on the error path of
 * dpaa2_eth_tx().
 * Optionally, return the frame annotation status word (FAS), which needs
 * to be checked if we're on the confirmation path.
 */
static void free_tx_fd(const struct dpaa2_eth_priv *priv,
		const struct dpaa2_fd *fd,
		u32 *status)
{
	struct device *dev = priv->net_dev->dev.parent;
	dma_addr_t fd_addr;
	struct sk_buff **skbh, *skb;
	unsigned char *buffer_start;
	int unmap_size;
	struct scatterlist *scl;
	int num_sg, num_dma_bufs;
	struct dpaa2_eth_swa *swa;
	bool fd_single;
	struct dpaa2_fas *fas;

	fd_addr = dpaa2_fd_get_addr(fd);
	skbh = phys_to_virt(fd_addr);
	fd_single = (dpaa2_fd_get_format(fd) == dpaa2_fd_single);

	/* HWA - FAS, timestamp (for Tx confirmation frames) */
	prefetch((void*) skbh + priv->buf_layout.private_data_size);

	if (fd_single) {
		skb = *skbh;
		buffer_start = (unsigned char *)skbh;
		/* Accessing the skb buffer is safe before dma unmap, because
		 * we didn't map the actual skb shell.
		 */
		dma_unmap_single(dev, fd_addr,
				skb_tail_pointer(skb) - buffer_start,
				DMA_TO_DEVICE);
	} else {
		swa = (struct dpaa2_eth_swa *)skbh;
		skb = swa->skb;
		scl = swa->scl;
		num_sg = swa->num_sg;
		num_dma_bufs = swa->num_dma_bufs;

		/* Unmap the scatterlist */
		dma_unmap_sg(dev, scl, num_sg, DMA_TO_DEVICE);
		kfree(scl);

		/* Unmap the SGT buffer */
		unmap_size = priv->tx_data_offset +
			sizeof(struct dpaa2_sg_entry) * (1 + num_dma_bufs);
		dma_unmap_single(dev, fd_addr, unmap_size, DMA_TO_DEVICE);
	}

	/* Get the timestamp value */
	if (priv->ts_tx_en && skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) {
		struct skb_shared_hwtstamps shhwtstamps;
		u64 *ns;

		memset(&shhwtstamps, 0, sizeof(shhwtstamps));

		ns = (u64 *)((void *)skbh +
				priv->buf_layout.private_data_size +
				sizeof(struct dpaa2_fas));
		*ns = DPAA2_PTP_NOMINAL_FREQ_PERIOD_NS * le64_to_cpup(ns);
		shhwtstamps.hwtstamp = ns_to_ktime(*ns);
		skb_tstamp_tx(skb, &shhwtstamps);
	}

	/* Read the status from the Frame Annotation after we unmap the first
	 * buffer but before we free it. The caller function is responsible
	 * for checking the status value.
	 */
	if (status) {
		fas = (struct dpaa2_fas *)
			((void *)skbh + priv->buf_layout.private_data_size);
		*status = le32_to_cpu(fas->status);
	}

	/* Free SGT buffer kmalloc'ed on tx */
	if (!fd_single)
		kfree(skbh);

	/* Move on with skb release */
	dev_kfree_skb(skb);
}

static int dpaa2_eth_tx(struct sk_buff *skb, struct net_device *net_dev)
{
	struct dpaa2_eth_priv *priv = netdev_priv(net_dev);
	struct dpaa2_fd fd;
	struct rtnl_link_stats64 *percpu_stats;
	struct dpaa2_eth_drv_stats *percpu_extras;
	struct dpaa2_eth_fq fq;
	u16 queue_mapping = skb_get_queue_mapping(skb);
	int err, i;

	percpu_stats = this_cpu_ptr(priv->percpu_stats);
	percpu_extras = this_cpu_ptr(priv->percpu_extras);

	if (unlikely(skb_headroom(skb) < DPAA2_ETH_NEEDED_HEADROOM(priv))) {
		struct sk_buff *ns;

		ns = skb_realloc_headroom(skb, DPAA2_ETH_NEEDED_HEADROOM(priv));
		if (unlikely(!ns)) {
			percpu_stats->tx_dropped++;
			goto err_alloc_headroom;
		}
		dev_kfree_skb(skb);
		skb = ns;
	}

	/* We'll be holding a back-reference to the skb until Tx Confirmation;
	 * we don't want that overwritten by a concurrent Tx with a cloned skb.
	 */
	skb = skb_unshare(skb, GFP_ATOMIC);
	if (unlikely(!skb)) {
		/* skb_unshare() has already freed the skb */
		percpu_stats->tx_dropped++;
		return NETDEV_TX_OK;
	}

	/* Setup the FD fields */
	memset(&fd, 0, sizeof(fd));

	if (skb_is_nonlinear(skb)) {
		err = build_sg_fd(priv, skb, &fd);
		percpu_extras->tx_sg_frames++;
		percpu_extras->tx_sg_bytes += skb->len;
	} else {
		err = build_single_fd(priv, skb, &fd);
	}

	if (unlikely(err)) {
		percpu_stats->tx_dropped++;
		goto err_build_fd;
	}

	/* Tracing point */
	trace_dpaa2_tx_fd(net_dev, &fd);

	fq = priv->fq[queue_mapping];
	for (i = 0; i < (DPAA2_ETH_MAX_TX_QUEUES << 1); i++) {
		err = dpaa2_io_service_enqueue_qd(NULL, priv->tx_qdid, 0,
				fq.tx_qdbin, &fd);
		/* TODO: This doesn't work. Check on simulator.
		 * err = dpaa2_io_service_enqueue_fq(NULL,
		 *			priv->fq[0].fqid_tx, &fd);
		 */
		if (err != -EBUSY)
			break;
	}
	percpu_extras->tx_portal_busy += i;
	if (unlikely(err < 0)) {
		percpu_stats->tx_errors++;
		/* Clean up everything, including freeing the skb */
		free_tx_fd(priv, &fd, NULL);
	} else {
		percpu_stats->tx_packets++;
		percpu_stats->tx_bytes += skb->len;
	}

	return NETDEV_TX_OK;

err_build_fd:
err_alloc_headroom:
	dev_kfree_skb(skb);

	return NETDEV_TX_OK;
}
#endif

#if FSLU_NVME_INIC_QDMA
void tx_qdma_callback(void *arg)
{

	struct dpaa2_eth_priv *priv=(struct dpaa2_eth_priv*)arg;
	spin_lock(&priv->tx_desc_lock);

	int err,i;
	struct rtnl_link_stats64 *percpu_stats;
	struct dpaa2_fd fd;
	uint64_t dest_addr;
	uint16_t len=0;
	memset(&fd, 0, sizeof(fd));

	dest_addr=priv->tx_buf[priv->tx_buf_dma]->addr;
	priv->tx_buf[priv->tx_buf_dma]->flag=BD_MEM_DIRTY;
	len=priv->cur_tx_dma->len_offset_flag>>16;
	priv->cur_tx_dma->len_offset_flag=BD_MEM_FREE;/*dma copy done now host can free its packet*/

	/* Setup the FD fields */
	dpaa2_fd_set_addr(&fd,dest_addr);
	dpaa2_fd_set_offset(&fd,priv->tx_data_offset);
	dpaa2_fd_set_bpid(&fd, priv->dpbp_attrs.bpid);
	dpaa2_fd_set_len(&fd, len);
	dpaa2_fd_set_format(&fd, dpaa2_fd_single);
	(&fd)->simple.ctrl = DPAA2_FD_CTRL_ASAL | DPAA2_FD_CTRL_PTA | DPAA2_FD_CTRL_PTV1;

	percpu_stats = this_cpu_ptr(priv->percpu_stats);

	for (i = 0; i < (DPAA2_ETH_MAX_TX_QUEUES<<1); i++) {
		err = dpaa2_io_service_enqueue_qd(NULL, priv->tx_qdid,
				0, priv->fq[0].tx_qdbin, &fd);
		if (err != -EBUSY)
		{
			break;
		}
	}

	if (unlikely(err < 0)) {
		percpu_stats->tx_errors++;
		printk("PCIE: %s enqueu error\n",__func__);
		priv->tx_buf[priv->tx_buf_dma]->flag=BD_MEM_READY;
		goto enque_err;
	}

	percpu_stats->tx_packets++;

enque_err:

	if(priv->tx_buf_dma==(DPAA2_ETH_NUM_TX_BUFS-1))
	{
		priv->tx_buf_dma=0;
	}
	else
	{
		priv->tx_buf_dma++;
	}

	if ((priv->cur_tx_dma - priv->tx_base) == (PH_RING_SIZE - 1))
	{
		priv->cur_tx_dma = priv->tx_base;
	}
	else
	{
		priv->cur_tx_dma++;
	}

	spin_unlock(&priv->tx_desc_lock);
	return;

}
#endif/*FSLU_NVME_INIC_QDMA*/

#if FSLU_NVME_INIC
static int nic_tx_napi(struct napi_struct *napi, int budget)
{

	struct dpaa2_eth_priv *priv = container_of(napi, struct dpaa2_eth_priv, napi);
	int received = 0;

#if FSLU_NVME_INIC_DPAA2
	int i,err;
	struct circ_buf_desc *bdp;
	struct rtnl_link_stats64 *percpu_stats;
	uint64_t addr_binfo=0;
#if FSLU_NVME_INIC_FPGA_IRQ
	uint32_t reg=0;
#endif
	bdp = priv->cur_tx;
	while (received < budget) {


		struct dpaa2_fd fd;

		if ((bdp->len_offset_flag & 0xf) != BD_MEM_DIRTY) {
			break;
		}

		/* Setup the FD fields */
		memset(&fd, 0, sizeof(fd));
		addr_binfo = bdp->addr_lower;
		addr_binfo |= ((uint64_t)(addr_binfo&0x20)<<27);
#if FSLU_NVME_INIC_SG
		if(addr_binfo & 0x10)
		{
			dpaa2_fd_set_format(&fd, dpaa2_fd_sg);
		}
		else
		{
#endif
			dpaa2_fd_set_format(&fd, dpaa2_fd_single);
#if FSLU_NVME_INIC_SG
		}
#endif
		addr_binfo = ((uint64_t)addr_binfo & 0xffffffffffffffc0);
		dpaa2_fd_set_addr(&fd,(dma_addr_t)((addr_binfo) + PEX_OFFSET));
		dpaa2_fd_set_offset(&fd, (uint16_t)((bdp->len_offset_flag>>4)&0x0fff));
		dpaa2_fd_set_bpid(&fd, priv->dpbp_attrs.bpid);
		dpaa2_fd_set_len(&fd, (bdp->len_offset_flag>>16));
		(&fd)->simple.ctrl = DPAA2_FD_CTRL_ASAL | DPAA2_FD_CTRL_PTA | DPAA2_FD_CTRL_PTV1;
		percpu_stats = this_cpu_ptr(priv->percpu_stats);
		/* FIXME Ugly hack, and not even cpu hotplug-friendly*/
		bdp->len_offset_flag=BD_MEM_CONSUME;
		mb();
		for (i = 0; i < (DPAA2_ETH_MAX_TX_QUEUES<<1); i++) {
			/*single queue */
			err = dpaa2_io_service_enqueue_qd(NULL, priv->tx_qdid,
					0, priv->fq[0].tx_qdbin, &fd);
			if (err != -EBUSY)
			{
				break;
			}
		}
		if (unlikely(err < 0)) {
			printk("enque_err\n");
			percpu_stats->tx_errors++;
			bdp->len_offset_flag=0xf0;
			bdp->len_offset_flag|=BD_MEM_FREE;
			goto enque_err;
		}

		percpu_stats->tx_packets++;
enque_err:

		if ((bdp - priv->tx_base) == (PH_RING_SIZE - 1))
		{
			bdp = priv->tx_base;
		}
		else
		{
			bdp++;
		}


		received++;
	}
#endif/*FSLU_NVME_INIC_DPAA2*/
#if FSLU_NVME_INIC_QDMA
	int i,err,len=0;
	struct circ_buf_desc *bdp;
	struct device *dev = priv->net_dev->dev.parent;
	dma_addr_t addr;
	bdp = priv->cur_tx;
	uint64_t src_addr=0,dest_addr=0,temp_addr;
	uint16_t offset=0;
	struct dma_async_tx_descriptor *txd;
	while (received < budget) {
		dma_cookie_t cookie;
		if ((bdp->len_offset_flag & 0xf) != BD_MEM_DIRTY) {
			break;
		}
		if(priv->tx_buf[priv->tx_buf_ready]->flag!=BD_MEM_READY)
		{
			printk("PCIE:%s tx buffers not available to process  and flag: %p \n",__func__,priv->tx_buf[priv->tx_buf_ready]->flag);
			break;
		}

		dest_addr=priv->tx_buf[priv->tx_buf_ready]->addr;
		priv->tx_buf[priv->tx_buf_ready]->flag=BD_MEM_CONSUME;
		memset(phys_to_virt(dest_addr + priv->buf_layout.private_data_size), 0, 8); /*doing annotation memset here to reduce callback work*/

		offset=(uint16_t)((bdp->len_offset_flag>>4)&0x0fff);
		len=bdp->len_offset_flag>>16;
		bdp->len_offset_flag |= BD_MEM_CONSUME;/*after dma copy done we will set it to free just to avoid loop whole descriptor again if it is dirty*/
		/*disable queue selection over head always use queue zero*/

		src_addr = bdp->addr_lower;
		src_addr |= ((uint64_t)(src_addr&0x20)<<27);
		src_addr  = (src_addr & 0xffffffffffffffc0);
		src_addr = PEX_OFFSET + src_addr+offset;

		txd = priv->dma_device->device_prep_dma_memcpy(priv->dma_chan,(dest_addr+priv->tx_data_offset),src_addr, len,DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
		if (!txd)
		{
			src_addr-=PEX_OFFSET;
			src_addr=priv->g_outaddr+offset;/*phys to virt*/
			memcpy(phys_to_virt(dest_addr+priv->tx_data_offset),src_addr,len);
			printk("%s qdma mapping error doing cpu memcpy \n",__func__);
			tx_qdma_callback(priv);
			goto dma_err;
		}
		txd->callback = tx_qdma_callback;
		txd->callback_param = priv;
		cookie = dmaengine_submit(txd);
		if (dma_submit_error(cookie)) {
			src_addr-=PEX_OFFSET;
			src_addr=priv->g_outaddr+offset;/*phys to virt*/
			memcpy(phys_to_virt(dest_addr+priv->tx_data_offset),src_addr,len);
			printk("%s qdma dma submit error doing cpu memcpy \n",__func__);
			tx_qdma_callback(priv);
			goto dma_err;
		}
		dma_async_issue_pending(priv->dma_chan);

dma_err:
		if(priv->tx_buf_ready==(DPAA2_ETH_NUM_TX_BUFS-1))
		{
			priv->tx_buf_ready=0;
		}
		else
		{
			priv->tx_buf_ready++;
		}
		/* if there is enqueu or dma error  don't update tx_buf count and set flag to ready*/

		if ((bdp - priv->tx_base) == (PH_RING_SIZE - 1))
		{
			bdp = priv->tx_base;
		}
		else
		{
			bdp++;
		}


		received++;
	}
#endif

	priv->cur_tx = bdp;
#if FSLU_NVME_INIC_POLL
	received =65;
#endif
	if (received < budget) {
		napi_complete(napi);
#if FSLU_NVME_INIC_QDMA_IRQ
		iowrite32(BD_IRQ_CLEAR,&priv->flags_ptr->tx_irq_flag);
#endif
#if FSLU_NVME_INIC_FPGA_IRQ
		enable_irq(priv->fpga_irq);
		reg=ioread32(priv->fpga_reg+0x3800);
		iowrite32(0x0,priv->fpga_reg + 0x3800);
		reg = (reg & (~(1<<priv->interface_id)));
		iowrite32(reg,priv->fpga_reg + 0x3800);
#endif
	}

	return received;

}

static int dpaa2_eth_tx(struct sk_buff *skb, struct net_device *net_dev)
{
	return NETDEV_TX_OK;

}
#endif /* FSLU_NVME_INIC */

/* Tx confirmation frame processing routine */
static void dpaa2_eth_tx_conf(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_channel *ch,
		const struct dpaa2_fd *fd,
		struct napi_struct *napi __always_unused,
		u16 queue_id __always_unused)
{

#if FSLU_NVME_INIC
#if FSLU_NVME_INIC_DPAA2
	struct rtnl_link_stats64 *percpu_stats;
	struct circ_buf_desc *bdp;
	spin_lock(&priv->tx_conf_lock);
	bdp = priv->cur_conf;

	percpu_stats = this_cpu_ptr(priv->percpu_stats);
	while((bdp->len_offset_flag & 0xf) != BD_MEM_CONSUME)
	{
		if ((bdp - priv->tx_base) == (PH_RING_SIZE - 1)) {
			bdp = priv->tx_base;
		} else {
			bdp++;
		}

		percpu_stats->tx_dropped++;
	}
	bdp->len_offset_flag = BD_MEM_FREE;
	if ((bdp - priv->tx_base) == (PH_RING_SIZE - 1)) {
		bdp = priv->tx_base;
	} else {
		bdp++;
	}

	percpu_stats->tx_errors++;

	priv->cur_conf = bdp;
	mb();
	spin_unlock(&priv->tx_conf_lock);
	return ;
#endif/*FSLU_NVME_INIC_DPAA2*/
#if FSLU_NVME_INIC_QDMA
	struct rtnl_link_stats64 *percpu_stats;
	spin_lock(&priv->tx_conf_lock);
	dma_addr_t fd_addr;
	uint32_t status = 0,len=0;
	uint64_t addr_binfo=0;
	struct dpaa2_fas *fas;
	struct device *dev = priv->net_dev->dev.parent;
	percpu_stats = this_cpu_ptr(priv->percpu_stats);

	fd_addr = dpaa2_fd_get_addr(fd);
	addr_binfo = phys_to_virt(fd_addr);

	len=dpaa2_fd_get_len(fd);

	/*tx_error checking*/
	fas = (struct dpaa2_fas *)((addr_binfo) + priv->buf_layout.private_data_size);
	status = le32_to_cpu(fas->status);
	if (status & DPAA2_FAS_TX_ERR_MASK) {
		printk("TxConf frame error(s): 0x%08x\n",status & DPAA2_FAS_TX_ERR_MASK);
		/* Tx-conf logically pertains to the egress path. */
		percpu_stats->tx_errors++;
	}


	if(priv->tx_buf[priv->tx_buf_free]->flag!=BD_MEM_DIRTY)
	{
		printk("PCEI : %s wrong conformation \n");
	}
	else
	{
		priv->tx_buf[priv->tx_buf_free]->flag=BD_MEM_READY;
	}

	if(priv->tx_buf_free==(DPAA2_ETH_NUM_TX_BUFS - 1))
	{
		priv->tx_buf_free=0;
	}
	else
	{
		priv->tx_buf_free++;
	}
	wmb();

	percpu_stats->tx_dropped++;
	spin_unlock(&priv->tx_conf_lock);
	return ;

#endif /*FSLU_NVME_INIC_QDMA*/
#else

	struct rtnl_link_stats64 *percpu_stats;
	struct dpaa2_eth_drv_stats *percpu_extras;
	u32 status = 0;
	bool errors = !!(fd->simple.ctrl & DPAA2_FD_TX_ERR_MASK);
	bool check_fas_errors = false;

	/* Tracing point */
	trace_dpaa2_tx_conf_fd(priv->net_dev, fd);

	percpu_extras = this_cpu_ptr(priv->percpu_extras);
	percpu_extras->tx_conf_frames++;
	percpu_extras->tx_conf_bytes += dpaa2_fd_get_len(fd);

	/* check frame errors in the FD field */
	if (unlikely(errors)) {
		check_fas_errors = !!(fd->simple.ctrl & DPAA2_FD_CTRL_FAERR) &&
			!!(fd->simple.frc & DPAA2_FD_FRC_FASV);
		if (net_ratelimit())
			netdev_dbg(priv->net_dev, "Tx frame FD err: %x08\n",
					fd->simple.ctrl & DPAA2_FD_TX_ERR_MASK);
	}

	free_tx_fd(priv, fd, check_fas_errors ? &status : NULL);

	/* if there are no errors, we're done */
	if (likely(!errors))
		return;

	percpu_stats = this_cpu_ptr(priv->percpu_stats);
	/* Tx-conf logically pertains to the egress path. */
	percpu_stats->tx_errors++;

	if (net_ratelimit())
		netdev_dbg(priv->net_dev, "Tx frame FAS err: %x08\n",
				status & DPAA2_FAS_TX_ERR_MASK);
#endif
}

static int set_rx_csum(struct dpaa2_eth_priv *priv, bool enable)
{
	int err;

	err = dpni_set_offload(priv->mc_io, 0, priv->mc_token,
			DPNI_OFF_RX_L3_CSUM, enable);
	if (err) {
		netdev_err(priv->net_dev,
				"dpni_set_offload() DPNI_OFF_RX_L3_CSUM failed\n");
		return err;
	}

	err = dpni_set_offload(priv->mc_io, 0, priv->mc_token,
			DPNI_OFF_RX_L4_CSUM, enable);
	if (err) {
		netdev_err(priv->net_dev,
				"dpni_set_offload() DPNI_OFF_RX_L4_CSUM failed\n");
		return err;
	}

	return 0;
}

static int set_tx_csum(struct dpaa2_eth_priv *priv, bool enable)
{
	int err;

	err = dpni_set_offload(priv->mc_io, 0, priv->mc_token,
			DPNI_OFF_TX_L3_CSUM, enable);
	if (err) {
		netdev_err(priv->net_dev,
				"dpni_set_offload() DPNI_OFF_RX_L3_CSUM failed\n");
		return err;
	}

	err = dpni_set_offload(priv->mc_io, 0, priv->mc_token,
			DPNI_OFF_TX_L4_CSUM, enable);
	if (err) {
		netdev_err(priv->net_dev,
				"dpni_set_offload() DPNI_OFF_RX_L4_CSUM failed\n");
		return err;
	}

	return 0;
}

/* Perform a single release command to add buffers
 * to the specified buffer pool
 */
#if FSLU_NVME_INIC_DPAA2
static int dpaa2_bp_add_7(struct dpaa2_eth_priv *priv, u16 bpid, void __iomem *array_addr)
#else
static int add_bufs(struct dpaa2_eth_priv *priv, u16 bpid)
#endif
{
	struct device *dev = priv->net_dev->dev.parent;
	u64 buf_array[DPAA2_ETH_BUFS_PER_CMD];
	void *buf;
	dma_addr_t addr;
	int i;
#if FSLU_NVME_INIC_DPAA2
	uint64_t paddr;
#endif /* FSLU_NVME_INIC_DPAA2 */

	for (i = 0; i < DPAA2_ETH_BUFS_PER_CMD; i++) {
		/* Allocate buffer visible to WRIOP + skb shared info +
		 * alignment padding.
		 */
#if !(FSLU_NVME_INIC_DPAA2)
		buf = napi_alloc_frag(DPAA2_ETH_BUF_RAW_SIZE(priv));
		if (unlikely(!buf))
			goto err_alloc;

		/* Leave extra IP headroom in front of the actual
		 * area the device is using.
		 */
		buf = PTR_ALIGN(buf + priv->rx_extra_head,
				DPAA2_ETH_RX_BUF_ALIGN);
#if !(FSLU_NVME_INIC_QDMA)
		addr = dma_map_single(dev, buf, DPAA2_ETH_RX_BUF_SIZE,
				DMA_FROM_DEVICE);
		if (unlikely(dma_mapping_error(dev, addr)))
			goto err_map;
#else
		addr=virt_to_phys(buf);
#endif
		buf_array[i] = addr;
#else
		paddr =  ioread32(array_addr);
		paddr |=((uint64_t)(paddr&0xf)<<32);
		paddr = (paddr & 0xfffffffffffffff0);
		paddr = PEX_OFFSET + paddr;
		buf_array[i] = paddr;
		array_addr = array_addr + 0x04;
#endif /* FSLU_NVME_INIC_DPAA2 */

		/* tracing point */
		trace_dpaa2_eth_buf_seed(priv->net_dev,
				buf, DPAA2_ETH_BUF_RAW_SIZE(priv),
				addr, DPAA2_ETH_RX_BUF_SIZE,
				bpid);
	}

release_bufs:
	/* In case the portal is busy, retry until successful.
	 * The buffer release function would only fail if the QBMan portal
	 * was busy, which implies portal contention (i.e. more CPUs than
	 * portals, i.e. GPPs w/o affine DPIOs). For all practical purposes,
	 * there is little we can realistically do, short of giving up -
	 * in which case we'd risk depleting the buffer pool and never again
	 * receiving the Rx interrupt which would kick-start the refill logic.
	 * So just keep retrying, at the risk of being moved to ksoftirqd.
	 */
	while (dpaa2_io_service_release(NULL, bpid, buf_array, i))
		cpu_relax();
	return i;

err_map:
	put_page(virt_to_head_page(buf));
err_alloc:
	if (i)
		goto release_bufs;

	return 0;
}

static int seed_pool(struct dpaa2_eth_priv *priv, u16 bpid)
{
	int i, j;
	int new_count;

#if FSLU_NVME_INIC_DPAA2
	void __iomem *array_addr;
	struct buf_pool_desc *bman_pool;
	bman_pool=priv->bman_buf;
	array_addr = bman_pool->pool_addr + priv->g_outaddr;
#endif
	/* This is the lazy seeding of Rx buffer pools.
	 * dpaa2_add_bufs() is also used on the Rx hotpath and calls
	 * napi_alloc_frag(). The trouble with that is that it in turn ends up
	 * calling this_cpu_ptr(), which mandates execution in atomic context.
	 * Rather than splitting up the code, do a one-off preempt disable.
	 */
	preempt_disable();
#if FSLU_NVME_INIC_DPAA2
	for (i = 0; i < DPAA2_ETH_NUM_BUFS;
			i += DPAA2_ETH_BUFS_PER_CMD) {
		new_count = dpaa2_bp_add_7(priv, bpid, array_addr);
		if (unlikely(new_count < 7))
			goto out_of_memory;
		array_addr = array_addr + (new_count * 0x04);
	}
#else
	for (j = 0; j < priv->num_channels; j++) {
		for (i = 0; i < DPAA2_ETH_NUM_BUFS;
				i += DPAA2_ETH_BUFS_PER_CMD) {
			new_count = add_bufs(priv, bpid);
			priv->channel[j]->buf_count += new_count;

			if (new_count < DPAA2_ETH_BUFS_PER_CMD) {
				preempt_enable();
				goto out_of_memory;
			}
		}
	}
#endif
	preempt_enable();

	return 0;
out_of_memory:
	return -ENOMEM;
}

/**
 * Drain the specified number of buffers from the DPNI's private buffer pool.
 * @count must not exceeed DPAA2_ETH_BUFS_PER_CMD
 */
#if FSLU_NVME_INIC_DPAA2
static uint32_t *bp_pull_index;
static uint32_t  bp_pull_count;
#endif/*FSLU_NVME_INIC_DPAA2*/
static void drain_bufs(struct dpaa2_eth_priv *priv, int count)
{
	struct device *dev = priv->net_dev->dev.parent;
	u64 buf_array[DPAA2_ETH_BUFS_PER_CMD];
	void *vaddr;
	int ret, i;

#if FSLU_NVME_INIC_DPAA2
	uint64_t addr;
#endif

	do {
		ret = dpaa2_io_service_acquire(NULL, priv->dpbp_attrs.bpid,
				buf_array, count);
		if (ret < 0) {
			netdev_err(priv->net_dev, "dpaa2_io_service_acquire() failed\n");
			return;
		}
		for (i = 0; i < ret; i++) {
#if FSLU_NVME_INIC_DPAA2
			addr=buf_array[i];
			addr=(addr-PEX_OFFSET);
			addr|=(addr>>32);
			cbd_write(bp_pull_index,(uint32_t)addr);
			bp_pull_index++;
			bp_pull_count++;
#else
			/* Same logic as on regular Rx path */
			dma_unmap_single(dev, buf_array[i],
					DPAA2_ETH_RX_BUF_SIZE,
					DMA_FROM_DEVICE);
			vaddr = phys_to_virt(buf_array[i]);
			put_page(virt_to_head_page(vaddr));
#endif
		}
	} while (ret);
}

static void drain_pool(struct dpaa2_eth_priv *priv)
{
	int i;
#if  FSLU_NVME_INIC_DPAA2
	bp_pull_index = priv->bman_buf->pool_addr + priv->g_outaddr;
	bp_pull_count = 0;
#endif
	drain_bufs(priv, DPAA2_ETH_BUFS_PER_CMD);
	drain_bufs(priv, 1);
#if FSLU_NVME_INIC_DPAA2
	priv->bman_buf->pool_len=bp_pull_count;
#endif

	for (i = 0; i < priv->num_channels; i++)
		priv->channel[i]->buf_count = 0;
}

#if FSLU_NVME_INIC
static int dpaa2_dpbp_refill_custom(struct dpaa2_eth_priv *priv, uint16_t bpid)
{
#if FSLU_NVME_INIC_DPAA2
	int new_count;
	struct refill_mem_pool *mem_pool;
	int i;
	void __iomem *array_addr;
	mem_pool=priv->mem_pool_cur;

	for(i=0;i<200000;i++)/*UGLY HACK  but this doesn't happen*/
	{
		if(cbd_read(&mem_pool->pool_flag)==BD_MEM_READY)
		{
			break;
		}
		if(i==199999)
		{
			printk("PCIE:refill_fail\n");
			return 0;
		}
	}
	array_addr = cbd_read(&mem_pool->pool_addr) + priv->g_outaddr;

	for (i = 0; i <REFILL_POOL_SIZE ; i += 7) {
		new_count = dpaa2_bp_add_7(priv, bpid, array_addr);
		array_addr = array_addr + (new_count * 0x04);
	}

	cbd_write(&mem_pool->pool_flag,BD_MEM_FREE);
	if((mem_pool - priv->mem_pool_base) == (REFILL_RING_SIZE-1))
	{
		mem_pool = priv->mem_pool_base;
	}
	else
	{
		mem_pool++;
	}
	priv->mem_pool_cur = mem_pool;
	return 0;
#endif/*FSLU_NVME_INIC_DPAA2*/
#if FSLU_NVME_INIC_QDMA

	int i=0,j=0;
	struct device *dev = priv->net_dev->dev.parent;
	uint64_t buf_array[7];
	uint64_t addr;

	for(i=0;i<RX_BUF_REUSE_COUNT;i++)
	{
		buf_array[j++]=priv->rx_buf[i];
		if(j==7)
		{
			while (dpaa2_io_service_release(NULL, bpid, buf_array, j))
				cpu_relax();
			j=0;
		}
	}

	return 0;

#endif/*FSLU_NVME_INIC_QDMA*/
}

#endif/*FSLU_NVME_INIC*/

/* Function is called from softirq context only, so we don't need to guard
 * the access to percpu count
 */
static int refill_pool(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_channel *ch,
		u16 bpid)
{
#if FSLU_NVME_INIC
	dump_stack();
	return 0;
#else
	int new_count;

	if (likely(ch->buf_count >= DPAA2_ETH_REFILL_THRESH))
		return 0;

	do {
		new_count = add_bufs(priv, bpid);
		if (unlikely(!new_count)) {
			/* Out of memory; abort for now, we'll try later on */
			break;
		}
		ch->buf_count += new_count;
	} while (ch->buf_count < DPAA2_ETH_NUM_BUFS);

	if (unlikely(ch->buf_count < DPAA2_ETH_NUM_BUFS))
		return -ENOMEM;

	return 0;
#endif
}

#if FSLU_NVME_INIC
#if FSLU_NVME_INIC_DPAA2
static irqreturn_t nic_irq_handler(int irq, void *data)
{

	int i;
	struct global_data *global =(struct global_data*)data; 
	uint32_t reg=0;
	struct dpaa2_eth_priv *priv =(struct dpaa2_eth_priv*)data; 
#if FSLU_NVME_INIC_QDMA_IRQ
	//iowrite32(0x10,global->dreg_config + 0x390104);
	iowrite32(0x12,global->dreg_config + 0x390104);
	iowrite32(0x0,global->dreg_config + 0x390100); 
#endif
#if FSLU_NVME_INIC_QDMA_IRQ
	for(i=0;i<NUM_NET_DEV;i++) 
	{
		struct dpaa2_eth_priv *priv = (struct dpaa2_eth_priv*)global->priv[i];
#endif
#if 1
		/* support for non blocking boot init moved from probe to irq handler since some flags need to be initialiazed before calling nic_open*/
		if(unlikely(priv->bman_buf->pool_set==BD_MEM_READY))
		{

			/*do init at only first time not for every time interface up is called*/
			priv->rx_base = (priv->bman_buf->rx_desc_base + priv->g_outaddr + 0x400);/*space for refill mem pool*/ 
			priv->flags_ptr =  ((unsigned long)priv->bman_buf->rx_desc_base + priv->g_outaddr);//host memory having tx_irq and link state flags
			priv->mem_pool_base=(priv->bman_buf->rx_desc_base + priv->g_outaddr+ 0x40);//host memory
			priv->mem_pool_cur= priv->mem_pool_base;//host memory
			/*fill the buffer pool once*/
			if (seed_pool(priv, priv->dpbp_attrs.bpid)) {
				printk("Buffer seeding failed for DPBP %d (bpid=%d)\n",priv->dpbp_dev->obj_desc.id, priv->dpbp_attrs.bpid);
			}

			priv->bman_buf->pool_set=BD_MEM_FREE;
		}
#endif

		if(cbd_read(&priv->flags_ptr->tx_irq_flag) == BD_IRQ_MASK) 
		{  

			if(unlikely(priv->bman_buf->iface_status == IFACE_UP)) {
				schedule_work(&priv->net_start_work);
#if FSLU_NVME_INIC_QDMA_IRQ
				iowrite32(BD_IRQ_CLEAR,&priv->flags_ptr->tx_irq_flag);
#endif   
#if FSLU_NVME_INIC_FPGA_IRQ
				reg=ioread32(priv->fpga_reg+0x3800);
				reg = (reg & (~(1<<priv->interface_id)));  
				iowrite32(reg,priv->fpga_reg + 0x3800);
#endif
				iowrite32(BD_IRQ_CLEAR,&priv->flags_ptr->tx_irq_flag);
			} else if(unlikely(priv->bman_buf->iface_status  == IFACE_DOWN)) {
				schedule_work(&priv->net_stop_work);
#if FSLU_NVME_INIC_QDMA_IRQ
				iowrite32(BD_IRQ_CLEAR,&priv->flags_ptr->tx_irq_flag); 
#endif 
#if FSLU_NVME_INIC_FPGA_IRQ
				reg=ioread32(priv->fpga_reg+0x3800);
				reg = (reg & (~(1<<priv->interface_id)));  
				iowrite32(reg,priv->fpga_reg + 0x3800);
#endif
			} else if(unlikely(priv->bman_buf->pull_rx_buffers==BP_PULL_SET)){

				drain_pool(priv);
				priv->bman_buf->pull_rx_buffers=BP_PULL_DONE;
#if FSLU_NVME_INIC_QDMA_IRQ
				iowrite32(BD_IRQ_CLEAR,&priv->flags_ptr->tx_irq_flag);
#endif 

			}

			else if(napi_schedule_prep(&priv->napi)){  
#if FSLU_NVME_INIC_FPGA_IRQ
				disable_irq_nosync(priv->fpga_irq);
#endif
#if FSLU_NVME_INIC_QDMA_IRQ
				iowrite32(BD_IRQ_CLEAR,&priv->flags_ptr->tx_irq_flag);
#endif 
				__napi_schedule(&priv->napi);
			}
#if FSLU_NVME_INIC_QDMA_IRQ
		}
#endif
	}
	return IRQ_HANDLED;
}
#endif/*FSLU_NVME_INIC_DPAA2*/

static int inic_poll_host_work(void *data)
{
	struct dpaa2_eth_priv *priv=(struct dpaa2_eth_priv*)data;

	printk("kthread executing on cpu %d\n",smp_processor_id());
	while(1)
	{
		/* support for non blocking boot init moved from probe to irq handler since some flags need to be initialiazed before calling nic_open*/
		if(unlikely(priv->bman_buf->pool_set==BD_MEM_READY))
		{

			/*do init at only first time not for every time interface up is called*/
			priv->rx_base = (priv->bman_buf->rx_desc_base + priv->g_outaddr + 0x400);/*space for refill mem pool*/
			priv->flags_ptr =  ((unsigned long)priv->bman_buf->rx_desc_base + priv->g_outaddr);//host memory having tx_irq and link state flags

#if FSLU_NVME_INIC_DPAA2

			priv->mem_pool_base=(priv->bman_buf->rx_desc_base + priv->g_outaddr+ 0x40);//host memory
			priv->mem_pool_cur= priv->mem_pool_base;//host memory

			/*fill the buffer pool once*/

			if (seed_pool(priv, priv->dpbp_attrs.bpid)) {
				printk("Buffer seeding failed for DPBP %d (bpid=%d)\n",priv->dpbp_dev->obj_desc.id, priv->dpbp_attrs.bpid);
			}
#endif
			priv->bman_buf->pool_set=BD_MEM_FREE;
		}

		if(priv->bman_buf->pool_set==BD_MEM_FREE)
		{

			if(unlikely(cbd_read(&priv->flags_ptr->tx_irq_flag) == BD_IRQ_MASK))
			{

				if(unlikely(priv->bman_buf->iface_status == IFACE_UP)) {
					schedule_work(&priv->net_start_work);
					iowrite32(BD_IRQ_CLEAR,&priv->flags_ptr->tx_irq_flag);
				} else if(unlikely(priv->bman_buf->iface_status  == IFACE_DOWN)) {
					schedule_work(&priv->net_stop_work);
					iowrite32(BD_IRQ_CLEAR,&priv->flags_ptr->tx_irq_flag);
				}
				else if(unlikely(priv->bman_buf->pull_rx_buffers==BP_PULL_SET)){

					drain_pool(priv);
					priv->bman_buf->pull_rx_buffers=BP_PULL_DONE;
					iowrite32(BD_IRQ_CLEAR,&priv->flags_ptr->tx_irq_flag);

				}
				else if(napi_schedule_prep(&priv->napi)){
					iowrite32(BD_IRQ_CLEAR,&priv->flags_ptr->tx_irq_flag);
					__napi_schedule(&priv->napi);
				}

			}
		}
		msleep(1000);
	}
	return 0;
}
#endif /*FSLU_NVME_INIC*/

static int pull_channel(struct dpaa2_eth_channel *ch)
{
	int err;
	int dequeues = -1;

	/* Retry while portal is busy */
	do {
		err = dpaa2_io_service_pull_channel(NULL, ch->ch_id, ch->store);
		dequeues++;
		cpu_relax();
	} while (err == -EBUSY);

	ch->stats.dequeue_portal_busy += dequeues;
	if (unlikely(err))
		ch->stats.pull_err++;

	return err;
}

/* NAPI poll routine
 *
 * Frames are dequeued from the QMan channel associated with this NAPI context.
 * Rx and (if configured) Rx error frames count towards the NAPI budget. Tx
 * confirmation frames are limited by a threshold per NAPI poll cycle.
 */
static int dpaa2_eth_poll(struct napi_struct *napi, int budget)
{
	struct dpaa2_eth_channel *ch;
	int cleaned, rx_cleaned = 0, tx_conf_cleaned = 0;
	bool store_cleaned;
	struct dpaa2_eth_priv *priv;
	int err;

	ch = container_of(napi, struct dpaa2_eth_channel, napi);
	priv = ch->priv;

	do {
		err = pull_channel(ch);
		if (unlikely(err))
			break;
#if !(FSLU_NVME_INIC)
		/* Refill pool if appropriate */
		refill_pool(priv, ch, priv->dpbp_attrs.bpid);
#endif

		store_cleaned = consume_frames(ch, &rx_cleaned,
				&tx_conf_cleaned);

		/* If we've either consumed the budget with Rx frames,
		 * or reached the Tx conf threshold, we're done.
		 */
		if (rx_cleaned >= budget ||
				tx_conf_cleaned >= TX_CONF_PER_NAPI_POLL)
			break;
	} while (store_cleaned);

	if (rx_cleaned >= budget || tx_conf_cleaned >= TX_CONF_PER_NAPI_POLL)
		cleaned = budget;
	else
		cleaned = max(rx_cleaned, 1);

	if (cleaned < budget) {
		napi_complete_done(napi, cleaned);
		/* Re-enable data available notifications */
		do {
			err = dpaa2_io_service_rearm(NULL, &ch->nctx);
			cpu_relax();
		} while (err == -EBUSY);
	}

	ch->stats.frames += rx_cleaned + tx_conf_cleaned;

	return cleaned;
}

static void enable_ch_napi(struct dpaa2_eth_priv *priv)
{
	struct dpaa2_eth_channel *ch;
	int i;

	for (i = 0; i < priv->num_channels; i++) {
		ch = priv->channel[i];
		napi_enable(&ch->napi);
	}
#if FSLU_NVME_INIC
	napi_enable(&priv->napi);
#endif /* FSLU_NVME_INIC */

}

static void disable_ch_napi(struct dpaa2_eth_priv *priv)
{
	struct dpaa2_eth_channel *ch;
	int i;

	for (i = 0; i < priv->num_channels; i++) {
		ch = priv->channel[i];
		napi_disable(&ch->napi);
	}
#if FSLU_NVME_INIC
	napi_disable(&priv->napi);
#endif /* FSLU_NVME_INIC */
}

static int link_state_update(struct dpaa2_eth_priv *priv)
{
	struct dpni_link_state state;
	int err;

	err = dpni_get_link_state(priv->mc_io, 0, priv->mc_token, &state);
	if (unlikely(err)) {
		netdev_err(priv->net_dev,
				"dpni_get_link_state() failed\n");
		return err;
	}

	/* Chech link state; speed / duplex changes are not treated yet */
	if (priv->link_state.up == state.up)
		return 0;

	priv->link_state = state;
	if (state.up) {
		netif_carrier_on(priv->net_dev);
		netif_tx_start_all_queues(priv->net_dev);
#if FSLU_NVME_INIC
		if((priv->interface_id >=0) && (priv->interface_id<NUM_NET_DEV))
		{
			cbd_write(&priv->flags_ptr->link_state_update,LINK_STATUS);
			cbd_write(&priv->flags_ptr->link_state,LINK_STATE_UP);
			generate_msi(priv);
		}
#endif
	} else {
		netif_tx_stop_all_queues(priv->net_dev);
		netif_carrier_off(priv->net_dev);
#if FSLU_NVME_INIC
		if((priv->interface_id >=0) && (priv->interface_id<NUM_NET_DEV))
		{
			cbd_write(&priv->flags_ptr->link_state_update,LINK_STATUS);
			cbd_write(&priv->flags_ptr->link_state,LINK_STATE_DOWN);
			generate_msi(priv);
		}
#endif
	}

	netdev_info(priv->net_dev, "Link Event: state %s",
			state.up ? "up" : "down");

	return 0;
}

#if FSLU_NVME_INIC
static void  nic_open(struct work_struct *work)
{
	struct dpaa2_eth_priv *priv = container_of(work, struct dpaa2_eth_priv,
			net_start_work);
	int err;

	/* We'll only start the txqs when the link is actually ready; make sure
	 * we don't race against the link up notification, which may come
	 * immediately after dpni_enable();
	 *
	 * FIXME beware of race conditions
	 */
	/*reset all discriptors */
	priv->cur_rx=priv->rx_base;
	priv->cur_tx=priv->tx_base;
#if FSLU_NVME_INIC_DPAA2
	priv->cur_conf = priv->tx_base;
#endif
#if FSLU_NVME_INIC_QDMA
	priv->cur_tx_dma = priv->tx_base;
#endif

	netif_tx_stop_all_queues(priv->net_dev);

	enable_ch_napi(priv);
	err = dpni_enable(priv->mc_io,0, priv->mc_token);
	if (err < 0) {
		printk("dpni_enable() failed\n");
	}

	/* Set the maximum Rx frame length to match the transmit side;
	 * account for L2 headers when computing the MFL
	 */
	err = dpni_set_max_frame_length(priv->mc_io,0, priv->mc_token,
			(uint16_t)DPAA2_ETH_L2_MAX_FRM(MTU_INIC));
	if (err) {
		printk("PCIE:dpni_set_mfl() failed\n");

	}

	priv->net_dev->mtu = MTU_INIC;

	priv->bman_buf->iface_status = IFACE_READY;
	return;
}

#endif /* FSLU_NVME_INIC */


static int dpaa2_eth_open(struct net_device *net_dev)
{
#if FSLU_NVME_INIC
	dump_stack();
	return 0;
#else
	struct dpaa2_eth_priv *priv = netdev_priv(net_dev);
	int err;

	err = seed_pool(priv, priv->dpbp_attrs.bpid);
	if (err) {
		/* Not much to do; the buffer pool, though not filled up,
		 * may still contain some buffers which would enable us
		 * to limp on.
		 */
		netdev_err(net_dev, "Buffer seeding failed for DPBP %d (bpid=%d)\n",
				priv->dpbp_dev->obj_desc.id, priv->dpbp_attrs.bpid);
	}

	/* We'll only start the txqs when the link is actually ready; make sure
	 * we don't race against the link up notification, which may come
	 * immediately after dpni_enable();
	 */
	netif_tx_stop_all_queues(net_dev);
	enable_ch_napi(priv);
	/* Also, explicitly set carrier off, otherwise netif_carrier_ok() will
	 * return true and cause 'ip link show' to report the LOWER_UP flag,
	 * even though the link notification wasn't even received.
	 */
	netif_carrier_off(net_dev);

	err = dpni_enable(priv->mc_io, 0, priv->mc_token);
	if (err < 0) {
		netdev_err(net_dev, "dpni_enable() failed\n");
		goto enable_err;
	}

	/* If the DPMAC object has already processed the link up interrupt,
	 * we have to learn the link state ourselves.
	 */
	err = link_state_update(priv);
	if (err < 0) {
		netdev_err(net_dev, "Can't update link state\n");
		goto link_state_err;
	}

	return 0;

link_state_err:
enable_err:
	disable_ch_napi(priv);
	drain_pool(priv);
	return err;
#endif
}

#if FSLU_NVME_INIC
static void nic_stop(struct work_struct *work)
{
	struct dpaa2_eth_priv *priv = container_of(work, struct dpaa2_eth_priv,
			net_stop_work);

	dpni_disable(priv->mc_io,0, priv->mc_token);

	/* TOO: Make sure queues are drained before if down is complete! */
	msleep(100);

	disable_ch_napi(priv);
	msleep(100);


	priv->bman_buf->iface_status = IFACE_STOP;

	return;
}
#endif /* FSLU_NVME_INIC */

/* The DPIO store must be empty when we call this,
 * at the end of every NAPI cycle.
 */
#if !(FSLU_NVME_INIC)
static int drain_channel(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_channel *ch)
{
	int rx_drained = 0, tx_conf_drained = 0;
	bool has_drained;

	do {
		pull_channel(ch);
		has_drained = consume_frames(ch, &rx_drained, &tx_conf_drained);
	} while (has_drained);

	return rx_drained + tx_conf_drained;
}

static int drain_ingress_frames(struct dpaa2_eth_priv *priv)
{
	struct dpaa2_eth_channel *ch;
	int i;
	int drained = 0;

	for (i = 0; i < priv->num_channels; i++) {
		ch = priv->channel[i];
		drained += drain_channel(priv, ch);
	}

	return drained;
}
#endif

static int dpaa2_eth_stop(struct net_device *net_dev)
{
#if FSLU_NVME_INIC
	return 0;
#else
	struct dpaa2_eth_priv *priv = netdev_priv(net_dev);
	int dpni_enabled;
	int retries = 10;
	int drained;

	netif_tx_stop_all_queues(net_dev);
	netif_carrier_off(net_dev);

	/* Loop while dpni_disable() attempts to drain the egress FQs
	 * and confirm them back to us.
	 */
	do {
		dpni_disable(priv->mc_io, 0, priv->mc_token);
		dpni_is_enabled(priv->mc_io, 0, priv->mc_token, &dpni_enabled);
		if (dpni_enabled)
			/* Allow the MC some slack */
			msleep(100);
	} while (dpni_enabled && --retries);
	if (!retries) {
		netdev_warn(net_dev, "Retry count exceeded disabling DPNI\n");
		/* Must go on and disable NAPI nonetheless, so we don't crash at
		 * the next "ifconfig up"
		 */
	}

	/* Wait for NAPI to complete on every core and disable it.
	 * In particular, this will also prevent NAPI from being rescheduled if
	 * a new CDAN is serviced, effectively discarding the CDAN. We therefore
	 * don't even need to disarm the channels, except perhaps for the case
	 * of a huge coalescing value.
	 */
	disable_ch_napi(priv);

	/* Manually drain the Rx and TxConf queues */
	drained = drain_ingress_frames(priv);
	if (drained)
		netdev_dbg(net_dev, "Drained %d frames.\n", drained);

	/* Empty the buffer pool */
	drain_pool(priv);

	return 0;
#endif
}

static int dpaa2_eth_init(struct net_device *net_dev)
{
	u64 supported = 0;
	u64 not_supported = 0;
	struct dpaa2_eth_priv *priv = netdev_priv(net_dev);
	u32 options = priv->dpni_attrs.options;

	/* Capabilities listing */
	supported |= IFF_LIVE_ADDR_CHANGE;

	if (options & DPNI_OPT_NO_MAC_FILTER)
		not_supported |= IFF_UNICAST_FLT;
	else
		supported |= IFF_UNICAST_FLT;

	net_dev->priv_flags |= supported;
	net_dev->priv_flags &= ~not_supported;

	/* Features */
#if FSLU_NVME_INIC
	net_dev->features = NETIF_F_RXCSUM |
		NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
		NETIF_F_SG | NETIF_F_HIGHDMA;
#else
	net_dev->features = NETIF_F_RXCSUM |
		NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
		NETIF_F_SG | NETIF_F_HIGHDMA |
		NETIF_F_LLTX;
#endif
	net_dev->hw_features = net_dev->features;

	return 0;
}

static int dpaa2_eth_set_addr(struct net_device *net_dev, void *addr)
{
	struct dpaa2_eth_priv *priv = netdev_priv(net_dev);
	struct device *dev = net_dev->dev.parent;
	int err;

	err = eth_mac_addr(net_dev, addr);
	if (err < 0) {
		dev_err(dev, "eth_mac_addr() failed (%d)\n", err);
		return err;
	}

	err = dpni_set_primary_mac_addr(priv->mc_io, 0, priv->mc_token,
			net_dev->dev_addr);
	if (err) {
		dev_err(dev, "dpni_set_primary_mac_addr() failed (%d)\n", err);
		return err;
	}

	return 0;
}

/** Fill in counters maintained by the GPP driver. These may be different from
 * the hardware counters obtained by ethtool.
 */
	static struct rtnl_link_stats64
*dpaa2_eth_get_stats(struct net_device *net_dev,
		struct rtnl_link_stats64 *stats)
{
	struct dpaa2_eth_priv *priv = netdev_priv(net_dev);
	struct rtnl_link_stats64 *percpu_stats;
	u64 *cpustats;
	u64 *netstats = (u64 *)stats;
	int i, j;
	int num = sizeof(struct rtnl_link_stats64) / sizeof(u64);

	for_each_possible_cpu(i) {
		percpu_stats = per_cpu_ptr(priv->percpu_stats, i);
		cpustats = (u64 *)percpu_stats;
		for (j = 0; j < num; j++)
			netstats[j] += cpustats[j];
	}

	return stats;
}

static int dpaa2_eth_change_mtu(struct net_device *net_dev, int mtu)
{
	struct dpaa2_eth_priv *priv = netdev_priv(net_dev);
	int err;

	if (mtu < 68 || mtu > DPAA2_ETH_MAX_MTU) {
		netdev_err(net_dev, "Invalid MTU %d. Valid range is: 68..%d\n",
				mtu, DPAA2_ETH_MAX_MTU);
		return -EINVAL;
	}

	/* Set the maximum Rx frame length to match the transmit side;
	 * account for L2 headers when computing the MFL
	 */
	err = dpni_set_max_frame_length(priv->mc_io, 0, priv->mc_token,
			(u16)DPAA2_ETH_L2_MAX_FRM(mtu));
	if (err) {
		netdev_err(net_dev, "dpni_set_max_frame_length() failed\n");
		return err;
	}

	net_dev->mtu = mtu;
	return 0;
}

/* Copy mac unicast addresses from @net_dev to @priv.
 * Its sole purpose is to make dpaa2_eth_set_rx_mode() more readable.
 */
static void add_uc_hw_addr(const struct net_device *net_dev,
		struct dpaa2_eth_priv *priv)
{
	struct netdev_hw_addr *ha;
	int err;

	netdev_for_each_uc_addr(ha, net_dev) {
		err = dpni_add_mac_addr(priv->mc_io, 0, priv->mc_token,
				ha->addr);
		if (err)
			netdev_warn(priv->net_dev,
					"Could not add ucast MAC %pM to the filtering table (err %d)\n",
					ha->addr, err);
	}
}

/* Copy mac multicast addresses from @net_dev to @priv
 * Its sole purpose is to make dpaa2_eth_set_rx_mode() more readable.
 */
static void add_mc_hw_addr(const struct net_device *net_dev,
		struct dpaa2_eth_priv *priv)
{
	struct netdev_hw_addr *ha;
	int err;

	netdev_for_each_mc_addr(ha, net_dev) {
		err = dpni_add_mac_addr(priv->mc_io, 0, priv->mc_token,
				ha->addr);
		if (err)
			netdev_warn(priv->net_dev,
					"Could not add mcast MAC %pM to the filtering table (err %d)\n",
					ha->addr, err);
	}
}

static void dpaa2_eth_set_rx_mode(struct net_device *net_dev)
{
	struct dpaa2_eth_priv *priv = netdev_priv(net_dev);
	int uc_count = netdev_uc_count(net_dev);
	int mc_count = netdev_mc_count(net_dev);
	u8 max_mac = priv->dpni_attrs.mac_filter_entries;
	u32 options = priv->dpni_attrs.options;
	u16 mc_token = priv->mc_token;
	struct fsl_mc_io *mc_io = priv->mc_io;
	int err;

	/* Basic sanity checks; these probably indicate a misconfiguration */
	if (options & DPNI_OPT_NO_MAC_FILTER && max_mac != 0)
		netdev_info(net_dev,
				"mac_filter_entries=%d, DPNI_OPT_NO_MAC_FILTER option must be disabled\n",
				max_mac);

	/* Force promiscuous if the uc or mc counts exceed our capabilities. */
	if (uc_count > max_mac) {
		netdev_info(net_dev,
				"Unicast addr count reached %d, max allowed is %d; forcing promisc\n",
				uc_count, max_mac);
		goto force_promisc;
	}
	if (mc_count + uc_count > max_mac) {
		netdev_info(net_dev,
				"Unicast + Multicast addr count reached %d, max allowed is %d; forcing promisc\n",
				uc_count + mc_count, max_mac);
		goto force_mc_promisc;
	}

	/* Adjust promisc settings due to flag combinations */
	if (net_dev->flags & IFF_PROMISC)
		goto force_promisc;
	if (net_dev->flags & IFF_ALLMULTI) {
		/* First, rebuild unicast filtering table. This should be done
		 * in promisc mode, in order to avoid frame loss while we
		 * progressively add entries to the table.
		 * We don't know whether we had been in promisc already, and
		 * making an MC call to find out is expensive; so set uc promisc
		 * nonetheless.
		 */
		err = dpni_set_unicast_promisc(mc_io, 0, mc_token, 1);
		if (err)
			netdev_warn(net_dev, "Can't set uc promisc\n");

		/* Actual uc table reconstruction. */
		err = dpni_clear_mac_filters(mc_io, 0, mc_token, 1, 0);
		if (err)
			netdev_warn(net_dev, "Can't clear uc filters\n");
		add_uc_hw_addr(net_dev, priv);

		/* Finally, clear uc promisc and set mc promisc as requested. */
		err = dpni_set_unicast_promisc(mc_io, 0, mc_token, 0);
		if (err)
			netdev_warn(net_dev, "Can't clear uc promisc\n");
		goto force_mc_promisc;
	}

	/* Neither unicast, nor multicast promisc will be on... eventually.
	 * For now, rebuild mac filtering tables while forcing both of them on.
	 */
	err = dpni_set_unicast_promisc(mc_io, 0, mc_token, 1);
	if (err)
		netdev_warn(net_dev, "Can't set uc promisc (%d)\n", err);
	err = dpni_set_multicast_promisc(mc_io, 0, mc_token, 1);
	if (err)
		netdev_warn(net_dev, "Can't set mc promisc (%d)\n", err);

	/* Actual mac filtering tables reconstruction */
	err = dpni_clear_mac_filters(mc_io, 0, mc_token, 1, 1);
	if (err)
		netdev_warn(net_dev, "Can't clear mac filters\n");
	add_mc_hw_addr(net_dev, priv);
	add_uc_hw_addr(net_dev, priv);

	/* Now we can clear both ucast and mcast promisc, without risking
	 * to drop legitimate frames anymore.
	 */
	err = dpni_set_unicast_promisc(mc_io, 0, mc_token, 0);
	if (err)
		netdev_warn(net_dev, "Can't clear ucast promisc\n");
	err = dpni_set_multicast_promisc(mc_io, 0, mc_token, 0);
	if (err)
		netdev_warn(net_dev, "Can't clear mcast promisc\n");

	return;

force_promisc:
	err = dpni_set_unicast_promisc(mc_io, 0, mc_token, 1);
	if (err)
		netdev_warn(net_dev, "Can't set ucast promisc\n");
force_mc_promisc:
	err = dpni_set_multicast_promisc(mc_io, 0, mc_token, 1);
	if (err)
		netdev_warn(net_dev, "Can't set mcast promisc\n");
}

static int dpaa2_eth_set_features(struct net_device *net_dev,
		netdev_features_t features)
{
	struct dpaa2_eth_priv *priv = netdev_priv(net_dev);
	netdev_features_t changed = features ^ net_dev->features;
	bool enable;
	int err;

	if (changed & NETIF_F_RXCSUM) {
		enable = !!(features & NETIF_F_RXCSUM);
		err = set_rx_csum(priv, enable);
		if (err)
			return err;
	}

	if (changed & (NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM)) {
		enable = !!(features & (NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM));
		err = set_tx_csum(priv, enable);
		if (err)
			return err;
	}

	return 0;
}

#if !(FSLU_NVME_INIC)
static int dpaa2_eth_ts_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct dpaa2_eth_priv *priv = netdev_priv(dev);
	struct hwtstamp_config config;

	if (copy_from_user(&config, rq->ifr_data, sizeof(config)))
		return -EFAULT;

	switch (config.tx_type) {
		case HWTSTAMP_TX_OFF:
			priv->ts_tx_en = false;
			break;
		case HWTSTAMP_TX_ON:
			priv->ts_tx_en = true;
			break;
		default:
			return -ERANGE;
	}

	if (config.rx_filter == HWTSTAMP_FILTER_NONE) {
		priv->ts_rx_en = false;
	} else {
		priv->ts_rx_en = true;
		/* TS is set for all frame types, not only those requested */
		config.rx_filter = HWTSTAMP_FILTER_ALL;
	}

	return copy_to_user(rq->ifr_data, &config, sizeof(config)) ?
		-EFAULT : 0;
}

static int dpaa2_eth_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	if (cmd == SIOCSHWTSTAMP)
		return dpaa2_eth_ts_ioctl(dev, rq, cmd);

	return -EINVAL;
}
#endif

static const struct net_device_ops dpaa2_eth_ops = {
	.ndo_open = dpaa2_eth_open,
	.ndo_start_xmit = dpaa2_eth_tx,
	.ndo_stop = dpaa2_eth_stop,
	.ndo_init = dpaa2_eth_init,
	.ndo_set_mac_address = dpaa2_eth_set_addr,
	.ndo_get_stats64 = dpaa2_eth_get_stats,
	.ndo_change_mtu = dpaa2_eth_change_mtu,
	.ndo_set_rx_mode = dpaa2_eth_set_rx_mode,
	.ndo_set_features = dpaa2_eth_set_features,
#if !(FSLU_NVME_INIC)
	.ndo_do_ioctl = dpaa2_eth_ioctl,
#endif
};

static void cdan_cb(struct dpaa2_io_notification_ctx *ctx)
{
	struct dpaa2_eth_channel *ch;

	ch = container_of(ctx, struct dpaa2_eth_channel, nctx);

	/* Update NAPI statistics */
	ch->stats.cdan++;

	napi_schedule_irqoff(&ch->napi);
}

/* Allocate and configure a DPCON object */
static struct fsl_mc_device *setup_dpcon(struct dpaa2_eth_priv *priv)
{
	struct fsl_mc_device *dpcon;
	struct device *dev = priv->net_dev->dev.parent;
	struct dpcon_attr attrs;
	int err;

	err = fsl_mc_object_allocate(to_fsl_mc_device(dev),
			FSL_MC_POOL_DPCON, &dpcon);
	if (err) {
		dev_info(dev, "Not enough DPCONs, will go on as-is\n");
		return NULL;
	}

	err = dpcon_open(priv->mc_io, 0, dpcon->obj_desc.id, &dpcon->mc_handle);
	if (err) {
		dev_err(dev, "dpcon_open() failed\n");
		goto err_open;
	}

	err = dpcon_reset(priv->mc_io, 0, dpcon->mc_handle);
	if (err) {
		dev_err(dev, "dpcon_reset() failed\n");
		goto err_reset;
	}

	err = dpcon_get_attributes(priv->mc_io, 0, dpcon->mc_handle, &attrs);
	if (err) {
		dev_err(dev, "dpcon_get_attributes() failed\n");
		goto err_get_attr;
	}

	err = dpcon_enable(priv->mc_io, 0, dpcon->mc_handle);
	if (err) {
		dev_err(dev, "dpcon_enable() failed\n");
		goto err_enable;
	}

	return dpcon;

err_enable:
err_get_attr:
err_reset:
	dpcon_close(priv->mc_io, 0, dpcon->mc_handle);
err_open:
	fsl_mc_object_free(dpcon);

	return NULL;
}

static void free_dpcon(struct dpaa2_eth_priv *priv,
		struct fsl_mc_device *dpcon)
{
	dpcon_disable(priv->mc_io, 0, dpcon->mc_handle);
	dpcon_close(priv->mc_io, 0, dpcon->mc_handle);
	fsl_mc_object_free(dpcon);
}

	static struct dpaa2_eth_channel *
alloc_channel(struct dpaa2_eth_priv *priv)
{
	struct dpaa2_eth_channel *channel;
	struct dpcon_attr attr;
	struct device *dev = priv->net_dev->dev.parent;
	int err;

	channel = kzalloc(sizeof(*channel), GFP_ATOMIC);
	if (!channel)
		return NULL;

	channel->dpcon = setup_dpcon(priv);
	if (!channel->dpcon)
		goto err_setup;

	err = dpcon_get_attributes(priv->mc_io, 0, channel->dpcon->mc_handle,
			&attr);
	if (err) {
		dev_err(dev, "dpcon_get_attributes() failed\n");
		goto err_get_attr;
	}

	channel->dpcon_id = attr.id;
	channel->ch_id = attr.qbman_ch_id;
	channel->priv = priv;

	return channel;

err_get_attr:
	free_dpcon(priv, channel->dpcon);
err_setup:
	kfree(channel);
	return NULL;
}

static void free_channel(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_channel *channel)
{
	free_dpcon(priv, channel->dpcon);
	kfree(channel);
}

/* DPIO setup: allocate and configure QBMan channels, setup core affinity
 * and register data availability notifications
 */
static int setup_dpio(struct dpaa2_eth_priv *priv)
{
	struct dpaa2_io_notification_ctx *nctx;
	struct dpaa2_eth_channel *channel;
	struct dpcon_notification_cfg dpcon_notif_cfg;
	struct device *dev = priv->net_dev->dev.parent;
	int i, err;

	/* Don't allocate more channels than strictly necessary and assign
	 * them to cores starting from the first one available in
	 * cpu_online_mask.
	 * If the number of channels is lower than the number of cores,
	 * there will be no rx/tx conf processing on the last cores in the mask.
	 */
	cpumask_clear(&priv->dpio_cpumask);
	for_each_online_cpu(i) {
		/* Try to allocate a channel */
		channel = alloc_channel(priv);
		if (!channel)
			goto err_alloc_ch;

		priv->channel[priv->num_channels] = channel;

		nctx = &channel->nctx;
		nctx->is_cdan = 1;
		nctx->cb = cdan_cb;
		nctx->id = channel->ch_id;
		nctx->desired_cpu = i;

		/* Register the new context */
		err = dpaa2_io_service_register(NULL, nctx);
		if (err) {
			dev_info(dev, "No affine DPIO for core %d\n", i);
			/* This core doesn't have an affine DPIO, but there's
			 * a chance another one does, so keep trying
			 */
			free_channel(priv, channel);
			continue;
		}

		/* Register DPCON notification with MC */
		dpcon_notif_cfg.dpio_id = nctx->dpio_id;
		dpcon_notif_cfg.priority = 0;
		dpcon_notif_cfg.user_ctx = nctx->qman64;
		err = dpcon_set_notification(priv->mc_io, 0,
				channel->dpcon->mc_handle,
				&dpcon_notif_cfg);
		if (err) {
			dev_err(dev, "dpcon_set_notification failed()\n");
			goto err_set_cdan;
		}

		/* If we managed to allocate a channel and also found an affine
		 * DPIO for this core, add it to the final mask
		 */
		cpumask_set_cpu(i, &priv->dpio_cpumask);
		priv->num_channels++;

		if (priv->num_channels == dpaa2_eth_max_channels(priv))
			break;
	}

	/* Tx confirmation queues can only be serviced by cpus
	 * with an affine DPIO/channel
	 */
	cpumask_copy(&priv->txconf_cpumask, &priv->dpio_cpumask);

	return 0;

err_set_cdan:
	dpaa2_io_service_deregister(NULL, nctx);
	free_channel(priv, channel);
err_alloc_ch:
	if (cpumask_empty(&priv->dpio_cpumask)) {
		dev_err(dev, "No cpu with an affine DPIO/DPCON\n");
		return -ENODEV;
	}
	cpumask_copy(&priv->txconf_cpumask, &priv->dpio_cpumask);

	return 0;
}

static void free_dpio(struct dpaa2_eth_priv *priv)
{
	int i;
	struct dpaa2_eth_channel *ch;

	/* deregister CDAN notifications and free channels */
	for (i = 0; i < priv->num_channels; i++) {
		ch = priv->channel[i];
		dpaa2_io_service_deregister(NULL, &ch->nctx);
		free_channel(priv, ch);
	}
}

static struct dpaa2_eth_channel *get_affine_channel(struct dpaa2_eth_priv *priv,
		int cpu)
{
	struct device *dev = priv->net_dev->dev.parent;
	int i;

	for (i = 0; i < priv->num_channels; i++)
		if (priv->channel[i]->nctx.desired_cpu == cpu)
			return priv->channel[i];

	/* We should never get here. Issue a warning and return
	 * the first channel, because it's still better than nothing
	 */
	dev_warn(dev, "No affine channel found for cpu %d\n", cpu);

	return priv->channel[0];
}

static void set_fq_affinity(struct dpaa2_eth_priv *priv)
{
	struct device *dev = priv->net_dev->dev.parent;
	struct dpaa2_eth_fq *fq;
	int rx_cpu, txc_cpu;
	int i;

	/* For each FQ, pick one channel/CPU to deliver frames to.
	 * This may well change at runtime, either through irqbalance or
	 * through direct user intervention.
	 */
	rx_cpu = cpumask_first(&priv->dpio_cpumask);
	txc_cpu = cpumask_first(&priv->txconf_cpumask);

	for (i = 0; i < priv->num_fqs; i++) {
		fq = &priv->fq[i];
		switch (fq->type) {
			case DPAA2_RX_FQ:
			case DPAA2_RX_ERR_FQ:
				fq->target_cpu = rx_cpu;
				rx_cpu = cpumask_next(rx_cpu, &priv->dpio_cpumask);
				if (rx_cpu >= nr_cpu_ids)
					rx_cpu = cpumask_first(&priv->dpio_cpumask);
				break;
			case DPAA2_TX_CONF_FQ:
				fq->target_cpu = txc_cpu;
				txc_cpu = cpumask_next(txc_cpu, &priv->txconf_cpumask);
				if (txc_cpu >= nr_cpu_ids)
					txc_cpu = cpumask_first(&priv->txconf_cpumask);
				break;
			default:
				dev_err(dev, "Unknown FQ type: %d\n", fq->type);
		}
		fq->channel = get_affine_channel(priv, fq->target_cpu);
	}
}

static void setup_fqs(struct dpaa2_eth_priv *priv)
{
	int i;

	/* We have one TxConf FQ per Tx flow. Tx queues MUST be at the
	 * beginning of the queue array.
	 * Number of Rx and Tx queues are the same.
	 * We only support one traffic class for now.
	 */
	for (i = 0; i < dpaa2_eth_queue_count(priv); i++) {
		priv->fq[priv->num_fqs].type = DPAA2_TX_CONF_FQ;
		priv->fq[priv->num_fqs].consume = dpaa2_eth_tx_conf;
		priv->fq[priv->num_fqs++].flowid = (u16)i;
	}

	for (i = 0; i < dpaa2_eth_queue_count(priv); i++) {
		priv->fq[priv->num_fqs].type = DPAA2_RX_FQ;
		priv->fq[priv->num_fqs].consume = dpaa2_eth_rx;
		priv->fq[priv->num_fqs++].flowid = (u16)i;
	}

#ifdef CONFIG_FSL_DPAA2_ETH_USE_ERR_QUEUE
	/* We have exactly one Rx error queue per DPNI */
	priv->fq[priv->num_fqs].type = DPAA2_RX_ERR_FQ;
	priv->fq[priv->num_fqs++].consume = dpaa2_eth_rx_err;
#endif

	/* For each FQ, decide on which core to process incoming frames */
	set_fq_affinity(priv);
}

/* Allocate and configure one buffer pool for each interface */
static int setup_dpbp(struct dpaa2_eth_priv *priv)
{
	int err;
	struct fsl_mc_device *dpbp_dev;
	struct device *dev = priv->net_dev->dev.parent;

	err = fsl_mc_object_allocate(to_fsl_mc_device(dev), FSL_MC_POOL_DPBP,
			&dpbp_dev);
	if (err) {
		dev_err(dev, "DPBP device allocation failed\n");
		return err;
	}

	priv->dpbp_dev = dpbp_dev;

	err = dpbp_open(priv->mc_io, 0, priv->dpbp_dev->obj_desc.id,
			&dpbp_dev->mc_handle);
	if (err) {
		dev_err(dev, "dpbp_open() failed\n");
		goto err_open;
	}

	err = dpbp_reset(priv->mc_io, 0, dpbp_dev->mc_handle);
	if (err) {
		dev_err(dev, "dpbp_reset() failed\n");
		goto err_reset;
	}

	err = dpbp_enable(priv->mc_io, 0, dpbp_dev->mc_handle);
	if (err) {
		dev_err(dev, "dpbp_enable() failed\n");
		goto err_enable;
	}

	err = dpbp_get_attributes(priv->mc_io, 0, dpbp_dev->mc_handle,
			&priv->dpbp_attrs);
	if (err) {
		dev_err(dev, "dpbp_get_attributes() failed\n");
		goto err_get_attr;
	}
#if FSLU_NVME_INIC
#if FSLU_NVME_INIC_QDMA/*implementing non blocking boot support*/
	if((if_count >=0) && (if_count< NUM_NET_DEV))
	{
		err = seed_pool(priv, priv->dpbp_attrs.bpid);
		if (err) {
			/* Not much to do; the buffer pool, though not filled up,
			 * may still contain some buffers which would enable us
			 * to limp on.
			 */
			printk("Buffer seeding failed for DPBP\n");
		}
	}
#endif
#endif
	return 0;

err_get_attr:
	dpbp_disable(priv->mc_io, 0, dpbp_dev->mc_handle);
err_enable:
err_reset:
	dpbp_close(priv->mc_io, 0, dpbp_dev->mc_handle);
err_open:
	fsl_mc_object_free(dpbp_dev);

	return err;
}

static void free_dpbp(struct dpaa2_eth_priv *priv)
{
	drain_pool(priv);
	dpbp_disable(priv->mc_io, 0, priv->dpbp_dev->mc_handle);
	dpbp_close(priv->mc_io, 0, priv->dpbp_dev->mc_handle);
	fsl_mc_object_free(priv->dpbp_dev);
}

/* Configure the DPNI object this interface is associated with */
static int setup_dpni(struct fsl_mc_device *ls_dev)
{
	struct device *dev = &ls_dev->dev;
	struct dpaa2_eth_priv *priv;
	struct net_device *net_dev;
	int err;

	net_dev = dev_get_drvdata(dev);
	priv = netdev_priv(net_dev);

	priv->dpni_id = ls_dev->obj_desc.id;

	/* get a handle for the DPNI object */
	err = dpni_open(priv->mc_io, 0, priv->dpni_id, &priv->mc_token);
	if (err) {
		dev_err(dev, "dpni_open() failed\n");
		goto err_open;
	}

	ls_dev->mc_io = priv->mc_io;
	ls_dev->mc_handle = priv->mc_token;

	err = dpni_reset(priv->mc_io, 0, priv->mc_token);
	if (err) {
		dev_err(dev, "dpni_reset() failed\n");
		goto err_reset;
	}

	err = dpni_get_attributes(priv->mc_io, 0, priv->mc_token,
			&priv->dpni_attrs);

	if (err) {
		dev_err(dev, "dpni_get_attributes() failed (err=%d)\n", err);
		goto err_get_attr;
	}

	/* TODO: keep these prints for debug. Remove them in the future */
	#if 0
	dev_info(dev, "attributes: options = %08x\n", priv->dpni_attrs.options);
	dev_info(dev, "attributes: num_queues = %d\n", priv->dpni_attrs.num_queues);
	dev_info(dev, "attributes: num_tcs = %d\n", priv->dpni_attrs.num_tcs);
	dev_info(dev, "attributes: mac_filter_entries = %d\n", priv->dpni_attrs.mac_filter_entries);
	dev_info(dev, "attributes: vlan_filter_entries = %d\n", priv->dpni_attrs.vlan_filter_entries);
	dev_info(dev, "attributes: qos_entries = %d\n", priv->dpni_attrs.qos_entries);
	dev_info(dev, "attributes: fs_entries = %d\n", priv->dpni_attrs.fs_entries);
	dev_info(dev, "attributes: qos_key_size = %d\n", priv->dpni_attrs.qos_key_size);
	dev_info(dev, "attributes: fs_key_size = %d\n", priv->dpni_attrs.fs_key_size);
	#endif
	/* Update number of logical FQs in netdev */
	err = netif_set_real_num_tx_queues(net_dev,
			dpaa2_eth_queue_count(priv));
	if (err) {
		dev_err(dev, "netif_set_real_num_tx_queues failed (%d)\n", err);
		goto err_set_tx_queues;
	}

	err = netif_set_real_num_rx_queues(net_dev,
			dpaa2_eth_queue_count(priv));
	if (err) {
		dev_err(dev, "netif_set_real_num_rx_queues failed (%d)\n", err);
		goto err_set_rx_queues;
	}

	/* Configure our buffers' layout */
	priv->buf_layout.options = DPNI_BUF_LAYOUT_OPT_PARSER_RESULT |
		DPNI_BUF_LAYOUT_OPT_FRAME_STATUS |
		DPNI_BUF_LAYOUT_OPT_PRIVATE_DATA_SIZE |
		DPNI_BUF_LAYOUT_OPT_DATA_ALIGN;
	priv->buf_layout.pass_parser_result = true;
	priv->buf_layout.pass_frame_status = true;
	priv->buf_layout.private_data_size = DPAA2_ETH_SWA_SIZE;
	/* HW erratum mandates data alignment in multiples of 256 */
	priv->buf_layout.data_align = DPAA2_ETH_RX_BUF_ALIGN;

	/* rx buffer */
	err = dpni_set_buffer_layout(priv->mc_io, 0, priv->mc_token,
			DPNI_QUEUE_RX, &priv->buf_layout);
	if (err) {
		dev_err(dev,
				"dpni_set_buffer_layout() DPNI_QUEUE_RX failed (%d)\n",
				err);
		goto err_buf_layout;
	}

	/* tx buffer: remove Rx-only options */
	priv->buf_layout.options &= ~(DPNI_BUF_LAYOUT_OPT_DATA_ALIGN |
			DPNI_BUF_LAYOUT_OPT_PARSER_RESULT);
	err = dpni_set_buffer_layout(priv->mc_io, 0, priv->mc_token,
			DPNI_QUEUE_TX, &priv->buf_layout);
	if (err) {
		dev_err(dev,
				"dpni_set_buffer_layout() DPNI_QUEUE_TX failed (%d)\n",
				err);
		goto err_buf_layout;
	}

	/* tx-confirm: same options as tx */
	priv->buf_layout.options &= ~DPNI_BUF_LAYOUT_OPT_PRIVATE_DATA_SIZE;
	priv->buf_layout.options |= DPNI_BUF_LAYOUT_OPT_TIMESTAMP;
	priv->buf_layout.pass_timestamp = 1;
	err = dpni_set_buffer_layout(priv->mc_io, 0, priv->mc_token,
			DPNI_QUEUE_TX_CONFIRM, &priv->buf_layout);
	if (err) {
		dev_err(dev,
				"dpni_set_buffer_layout() DPNI_QUEUE_TX_CONFIRM failed (%d)\n",
				err);
		goto err_buf_layout;
	}

	/* Now that we've set our tx buffer layout, retrieve the minimum
	 * required tx data offset.
	 */
	err = dpni_get_tx_data_offset(priv->mc_io, 0, priv->mc_token,
			&priv->tx_data_offset);
	if (err) {
		dev_err(dev, "dpni_get_tx_data_offset() failed (%d)\n", err);
		goto err_data_offset;
	}

	if ((priv->tx_data_offset % 64) != 0)
		dev_warn(dev, "Tx data offset (%d) not a multiple of 64B",
				priv->tx_data_offset);

	/* Accommodate SWA space. */
	priv->tx_data_offset += DPAA2_ETH_SWA_SIZE;

	/* allocate classification rule space */
	priv->cls_rule = kzalloc(sizeof(*priv->cls_rule) *
			dpaa2_eth_fs_count(priv), GFP_KERNEL);
	if (!priv->cls_rule)
		goto err_cls_rule;

	return 0;

err_cls_rule:
err_data_offset:
err_buf_layout:
err_set_rx_queues:
err_set_tx_queues:
err_get_attr:
err_reset:
	dpni_close(priv->mc_io, 0, priv->mc_token);
err_open:
	return err;
}

static void free_dpni(struct dpaa2_eth_priv *priv)
{
	int err;

	err = dpni_reset(priv->mc_io, 0, priv->mc_token);
	if (err)
		netdev_warn(priv->net_dev, "dpni_reset() failed (err %d)\n",
				err);

	dpni_close(priv->mc_io, 0, priv->mc_token);
}

static int setup_rx_flow(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_fq *fq)
{
	struct device *dev = priv->net_dev->dev.parent;
	struct dpni_queue q = { { 0 } };
	struct dpni_queue_id qid;
	u8 q_opt = DPNI_QUEUE_OPT_USER_CTX | DPNI_QUEUE_OPT_DEST;
	struct dpni_taildrop td;
	int err;

	err = dpni_get_queue(priv->mc_io, 0, priv->mc_token,
			DPNI_QUEUE_RX, 0, fq->flowid, &q, &qid);
	if (err) {
		dev_err(dev, "dpni_get_queue() failed (%d)\n", err);
		return err;
	}

	//dev_info(dev, "Rx q->fqid %08x\n", qid.fqid); /*m*/
	fq->fqid = qid.fqid;

	q.destination.id = fq->channel->dpcon_id;
	q.destination.type = DPNI_DEST_DPCON;
	q.destination.priority = 1;
	q.user_context = (u64)fq;
	err = dpni_set_queue(priv->mc_io, 0, priv->mc_token,
			DPNI_QUEUE_RX, 0, fq->flowid, q_opt, &q);
	if (err) {
		dev_err(dev, "dpni_set_queue() failed (%d)\n", err);
		return err;
	}

	td.enable = 1;
	td.threshold = DPAA2_ETH_TAILDROP_THRESH;
	err = dpni_set_taildrop(priv->mc_io, 0, priv->mc_token, DPNI_CP_QUEUE,
			DPNI_QUEUE_RX, 0, fq->flowid, &td);
	if (err) {
		dev_err(dev, "dpni_set_taildrop() failed (%d)\n", err);
		return err;
	}

	return 0;
}

static int setup_tx_flow(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_fq *fq)
{
	struct device *dev = priv->net_dev->dev.parent;
	struct dpni_queue q = { { 0 } };
	struct dpni_queue_id qid;
	u8 q_opt = DPNI_QUEUE_OPT_USER_CTX | DPNI_QUEUE_OPT_DEST;
	int err;

	err = dpni_get_queue(priv->mc_io, 0, priv->mc_token,
			DPNI_QUEUE_TX, 0, fq->flowid, &q, &qid);
	if (err) {
		dev_err(dev, "dpni_get_queue() failed (%d)\n", err);
		return err;
	}

	//dev_info(dev, "Tx q->fqid %08x q->qdbin %08x\n", qid.fqid, qid.qdbin); /*m*/
	fq->tx_qdbin = qid.qdbin;

	err = dpni_get_queue(priv->mc_io, 0, priv->mc_token,
			DPNI_QUEUE_TX_CONFIRM, 0, fq->flowid, &q, &qid);
	if (err) {
		dev_err(dev, "dpni_get_queue() failed (%d)\n", err);
		return err;
	}

	//dev_info(dev, "Txc q->fqid %08x\n", qid.fqid); /*m*/
	fq->fqid = qid.fqid;

	q.destination.id = fq->channel->dpcon_id;
	q.destination.type = DPNI_DEST_DPCON;
	q.destination.priority = 0;
	q.user_context = (u64)fq;
	err = dpni_set_queue(priv->mc_io, 0, priv->mc_token,
			DPNI_QUEUE_TX_CONFIRM, 0, fq->flowid, q_opt, &q);
	if (err) {
		dev_err(dev, "dpni_get_queue() failed (%d)\n", err);
		return err;
	}

	return 0;
}

#ifdef CONFIG_FSL_DPAA2_ETH_USE_ERR_QUEUE
static int setup_rx_err_flow(struct dpaa2_eth_priv *priv,
		struct dpaa2_eth_fq *fq)
{
	struct device *dev = priv->net_dev->dev.parent;
	struct dpni_queue q = { { 0 } };
	struct dpni_queue_id qid;
	u8 q_opt = DPNI_QUEUE_OPT_USER_CTX | DPNI_QUEUE_OPT_DEST;
	int err;

	err = dpni_get_queue(priv->mc_io, 0, priv->mc_token,
			DPNI_QUEUE_RX_ERR, 0, 0, &q, &qid);
	if (err) {
		dev_err(dev, "dpni_get_queue() failed (%d)\n", err);
		return err;
	}

	dev_info(dev, "RxERR q->fqid %08x\n", qid.fqid);
	fq->fqid = qid.fqid;

	q.destination.id = fq->channel->dpcon_id;
	q.destination.type = DPNI_DEST_DPCON;
	q.destination.priority = 1;
	q.user_context = (u64)fq;
	err = dpni_set_queue(priv->mc_io, 0, priv->mc_token,
			DPNI_QUEUE_RX_ERR, 0, 0, q_opt, &q);
	if (err) {
		dev_err(dev, "dpni_set_queue() failed (%d)\n", err);
		return err;
	}

	return 0;
}
#endif

/* default hash key fields */
static struct dpaa2_eth_hash_fields default_hash_fields[] = {
	{
		/* L2 header */
		.rxnfc_field = RXH_L2DA,
		.cls_prot = NET_PROT_ETH,
		.cls_field = NH_FLD_ETH_DA,
		.size = 6,
	}, {
		.cls_prot = NET_PROT_ETH,
		.cls_field = NH_FLD_ETH_SA,
		.size = 6,
	}, {
		/* This is the last ethertype field parsed:
		 * depending on frame format, it can be the MAC ethertype
		 * or the VLAN etype.
		 */
		.cls_prot = NET_PROT_ETH,
		.cls_field = NH_FLD_ETH_TYPE,
		.size = 2,
	}, {
		/* VLAN header */
		.rxnfc_field = RXH_VLAN,
		.cls_prot = NET_PROT_VLAN,
		.cls_field = NH_FLD_VLAN_TCI,
		.size = 2,
	}, {
		/* IP header */
		.rxnfc_field = RXH_IP_SRC,
			.cls_prot = NET_PROT_IP,
			.cls_field = NH_FLD_IP_SRC,
			.size = 4,
	}, {
		.rxnfc_field = RXH_IP_DST,
			.cls_prot = NET_PROT_IP,
			.cls_field = NH_FLD_IP_DST,
			.size = 4,
	}, {
		.rxnfc_field = RXH_L3_PROTO,
			.cls_prot = NET_PROT_IP,
			.cls_field = NH_FLD_IP_PROTO,
			.size = 1,
	}, {
		/* Using UDP ports, this is functionally equivalent to raw
		 * byte pairs from L4 header.
		 */
		.rxnfc_field = RXH_L4_B_0_1,
			.cls_prot = NET_PROT_UDP,
			.cls_field = NH_FLD_UDP_PORT_SRC,
			.size = 2,
	}, {
		.rxnfc_field = RXH_L4_B_2_3,
			.cls_prot = NET_PROT_UDP,
			.cls_field = NH_FLD_UDP_PORT_DST,
			.size = 2,
	},
};

/* Set RX hash options */
int set_hash(struct dpaa2_eth_priv *priv)
{
	struct device *dev = priv->net_dev->dev.parent;
	struct dpkg_profile_cfg cls_cfg;
	struct dpni_rx_tc_dist_cfg dist_cfg;
	u8 *dma_mem;
	int i;
	int err = 0;

	memset(&cls_cfg, 0, sizeof(cls_cfg));

	for (i = 0; i < priv->num_hash_fields; i++) {
		struct dpkg_extract *key =
			&cls_cfg.extracts[cls_cfg.num_extracts];

		key->type = DPKG_EXTRACT_FROM_HDR;
		key->extract.from_hdr.prot = priv->hash_fields[i].cls_prot;
		key->extract.from_hdr.type = DPKG_FULL_FIELD;
		key->extract.from_hdr.field = priv->hash_fields[i].cls_field;
		cls_cfg.num_extracts++;

		priv->rx_flow_hash |= priv->hash_fields[i].rxnfc_field;
	}

	dma_mem = kzalloc(DPAA2_CLASSIFIER_DMA_SIZE, GFP_DMA | GFP_KERNEL);
	if (!dma_mem)
		return -ENOMEM;

	err = dpni_prepare_key_cfg(&cls_cfg, dma_mem);
	if (err) {
		dev_err(dev, "dpni_prepare_key_cfg() failed (%d)", err);
		kfree(dma_mem);
		return err;
	}

	memset(&dist_cfg, 0, sizeof(dist_cfg));

	/* Prepare for setting the rx dist */
	dist_cfg.key_cfg_iova = dma_map_single(dev, dma_mem,
			DPAA2_CLASSIFIER_DMA_SIZE,
			DMA_TO_DEVICE);
	if (dma_mapping_error(dev, dist_cfg.key_cfg_iova)) {
		dev_err(dev, "DMA mapping failed\n");
		kfree(dma_mem);
		return -ENOMEM;
	}

	dist_cfg.dist_size = dpaa2_eth_queue_count(priv);
	if (dpaa2_eth_fs_enabled(priv)) {
		dist_cfg.dist_mode = DPNI_DIST_MODE_FS;
		dist_cfg.fs_cfg.miss_action = DPNI_FS_MISS_HASH;
	} else {
		dist_cfg.dist_mode = DPNI_DIST_MODE_HASH;
	}

	err = dpni_set_rx_tc_dist(priv->mc_io, 0, priv->mc_token, 0, &dist_cfg);
	dma_unmap_single(dev, dist_cfg.key_cfg_iova,
			DPAA2_CLASSIFIER_DMA_SIZE, DMA_TO_DEVICE);
	kfree(dma_mem);
	if (err) {
		dev_err(dev, "dpni_set_rx_tc_dist() failed (%d)\n", err);
		return err;
	}

	return 0;
}

/* Bind the DPNI to its needed objects and resources: buffer pool, DPIOs,
 * frame queues and channels
 */
static int bind_dpni(struct dpaa2_eth_priv *priv)
{
	struct net_device *net_dev = priv->net_dev;
	struct device *dev = net_dev->dev.parent;
	struct dpni_pools_cfg pools_params;
	struct dpni_error_cfg err_cfg;
	int err = 0;
	int i;

	pools_params.num_dpbp = 1;
	pools_params.pools[0].dpbp_id = priv->dpbp_dev->obj_desc.id;
	pools_params.pools[0].backup_pool = 0;
	pools_params.pools[0].buffer_size = DPAA2_ETH_RX_BUF_SIZE;
	err = dpni_set_pools(priv->mc_io, 0, priv->mc_token, &pools_params);
	if (err) {
		dev_err(dev, "dpni_set_pools() failed\n");
		return err;
	}

	/* Verify classification options and disable hashing and/or
	 * flow steering support in case of invalid configuration values
	 */
	check_cls_support(priv);

	/* have the interface implicitly distribute traffic based on
	 * a static hash key
	 */
	if (dpaa2_eth_hash_enabled(priv)) {
		priv->hash_fields = default_hash_fields;
		priv->num_hash_fields = ARRAY_SIZE(default_hash_fields);
		err = set_hash(priv);
		if (err) {
			dev_err(dev, "Hashing configuration failed\n");
			return err;
		}
	}

	/* Configure handling of error frames */
	err_cfg.errors = DPAA2_FAS_RX_ERR_MASK;
	err_cfg.set_frame_annotation = 1;
#ifdef CONFIG_FSL_DPAA2_ETH_USE_ERR_QUEUE
	err_cfg.error_action = DPNI_ERROR_ACTION_SEND_TO_ERROR_QUEUE;
#else
	err_cfg.error_action = DPNI_ERROR_ACTION_DISCARD;
#endif
	err = dpni_set_errors_behavior(priv->mc_io, 0, priv->mc_token,
			&err_cfg);
	if (err) {
		dev_err(dev, "dpni_set_errors_behavior() failed (%d)\n", err);
		return err;
	}

	/* Configure Rx and Tx conf queues to generate CDANs */
	for (i = 0; i < priv->num_fqs; i++) {
		switch (priv->fq[i].type) {
			case DPAA2_RX_FQ:
				err = setup_rx_flow(priv, &priv->fq[i]);
				break;
			case DPAA2_TX_CONF_FQ:
				err = setup_tx_flow(priv, &priv->fq[i]);
				break;
#ifdef CONFIG_FSL_DPAA2_ETH_USE_ERR_QUEUE
			case DPAA2_RX_ERR_FQ:
				err = setup_rx_err_flow(priv, &priv->fq[i]);
				break;
#endif
			default:
				dev_err(dev, "Invalid FQ type %d\n", priv->fq[i].type);
				return -EINVAL;
		}
		if (err)
			return err;
	}

	err = dpni_get_qdid(priv->mc_io, 0, priv->mc_token, DPNI_QUEUE_TX,
			&priv->tx_qdid);
	if (err) {
		dev_err(dev, "dpni_get_qdid() failed\n");
		return err;
	}

	return 0;
}

/* Allocate rings for storing incoming frame descriptors */
static int alloc_rings(struct dpaa2_eth_priv *priv)
{
	struct net_device *net_dev = priv->net_dev;
	struct device *dev = net_dev->dev.parent;
	int i;

	for (i = 0; i < priv->num_channels; i++) {
		priv->channel[i]->store =
			dpaa2_io_store_create(DPAA2_ETH_STORE_SIZE, dev);
		if (!priv->channel[i]->store) {
			netdev_err(net_dev, "dpaa2_io_store_create() failed\n");
			goto err_ring;
		}
	}

	return 0;

err_ring:
	for (i = 0; i < priv->num_channels; i++) {
		if (!priv->channel[i]->store)
			break;
		dpaa2_io_store_destroy(priv->channel[i]->store);
	}

	return -ENOMEM;
}

static void free_rings(struct dpaa2_eth_priv *priv)
{
	int i;

	for (i = 0; i < priv->num_channels; i++)
		dpaa2_io_store_destroy(priv->channel[i]->store);
}

static int netdev_init(struct net_device *net_dev)
{
	int err;
	struct device *dev = net_dev->dev.parent;
	struct dpaa2_eth_priv *priv = netdev_priv(net_dev);
	u8 mac_addr[ETH_ALEN], dpni_mac_addr[ETH_ALEN];
	u8 bcast_addr[ETH_ALEN];
	u16 rx_headroom, rx_req_headroom;

	net_dev->netdev_ops = &dpaa2_eth_ops;

	/* Get firmware address, if any */
	err = dpni_get_port_mac_addr(priv->mc_io, 0, priv->mc_token, mac_addr);
	if (err) {
		dev_err(dev, "dpni_get_port_mac_addr() failed (%d)\n", err);
		return err;
	}

	/* Get DPNI atttributes address, if any */
	err = dpni_get_primary_mac_addr(priv->mc_io, 0, priv->mc_token,
			dpni_mac_addr);
	if (err) {
		dev_err(dev, "dpni_get_primary_mac_addr() failed (%d)\n", err);
		return err;
	}

	/* First check if firmware has any address configured by bootloader */
	if (!is_zero_ether_addr(mac_addr)) {
		/* If the DPMAC addr != the DPNI addr, update it */
		if (!ether_addr_equal(mac_addr, dpni_mac_addr)) {
			err = dpni_set_primary_mac_addr(priv->mc_io, 0,
					priv->mc_token, mac_addr);
			if (err) {
				dev_err(dev,
						"dpni_set_primary_mac_addr() failed (%d)\n",
						err);
				return err;
			}
		}
		memcpy(net_dev->dev_addr, mac_addr, net_dev->addr_len);
	} else if (is_zero_ether_addr(dpni_mac_addr)) {
		/* Fills in net_dev->dev_addr, as required by
		 * register_netdevice()
		 */
		eth_hw_addr_random(net_dev);
		/* Make the user aware, without cluttering the boot log */
		pr_info_once(KBUILD_MODNAME
				" device(s) have zero hwaddr, replaced with random");
		err = dpni_set_primary_mac_addr(priv->mc_io, 0,
				priv->mc_token, net_dev->dev_addr);
		if (err) {
			dev_err(dev,
					"dpni_set_primary_mac_addr() failed  (%d)\n", err);
			return err;
		}
		/* Override NET_ADDR_RANDOM set by eth_hw_addr_random(); for all
		 * practical purposes, this will be our "permanent" mac address,
		 * at least until the next reboot. This move will also permit
		 * register_netdevice() to properly fill up net_dev->perm_addr.
		 */
		net_dev->addr_assign_type = NET_ADDR_PERM;
		/* If DPMAC address is non-zero, use that one */
	} else {
		/* NET_ADDR_PERM is default, all we have to do is
		 * fill in the device addr.
		 */
		memcpy(net_dev->dev_addr, dpni_mac_addr, net_dev->addr_len);
	}

	/* Explicitly add the broadcast address to the MAC filtering table;
	 * the MC won't do that for us.
	 */
	eth_broadcast_addr(bcast_addr);
	err = dpni_add_mac_addr(priv->mc_io, 0, priv->mc_token, bcast_addr);
	if (err) {
		dev_warn(dev, "dpni_add_mac_addr() failed (%d)\n", err);
		/* Won't return an error; at least, we'd have egress traffic */
	}

	/* Reserve enough space to align buffer as per hardware requirement;
	 * NOTE: priv->tx_data_offset MUST be initialized at this point.
	 */
	net_dev->needed_headroom = DPAA2_ETH_NEEDED_HEADROOM(priv);

	/* Required headroom for Rx skbs, to avoid reallocation on
	 * forwarding path.
	 */
	rx_req_headroom = LL_RESERVED_SPACE(net_dev) - ETH_HLEN;
	rx_headroom = ALIGN(DPAA2_ETH_HWA_SIZE + DPAA2_ETH_SWA_SIZE,
			DPAA2_ETH_RX_BUF_ALIGN);
	if (rx_req_headroom > rx_headroom)
		priv->rx_extra_head = ALIGN(rx_req_headroom - rx_headroom, 4);

	/* Our .ndo_init will be called herein */
	err = register_netdev(net_dev);
	if (err < 0) {
		dev_err(dev, "register_netdev() failed (%d)\n", err);
		return err;
	}

	return 0;
}

static int poll_link_state(void *arg)
{
	struct dpaa2_eth_priv *priv = (struct dpaa2_eth_priv *)arg;
	int err;

	while (!kthread_should_stop()) {
		err = link_state_update(priv);
		if (unlikely(err))
			return err;

		msleep(DPAA2_ETH_LINK_STATE_REFRESH);
	}

	return 0;
}

static irqreturn_t dpni_irq0_handler(int irq_num, void *arg)
{
	return IRQ_WAKE_THREAD;
}

static irqreturn_t dpni_irq0_handler_thread(int irq_num, void *arg)
{
	u8 irq_index = DPNI_IRQ_INDEX;
	u32 status = 0, clear = 0;
	struct device *dev = (struct device *)arg;
	struct fsl_mc_device *dpni_dev = to_fsl_mc_device(dev);
	struct net_device *net_dev = dev_get_drvdata(dev);
	int err;

	err = dpni_get_irq_status(dpni_dev->mc_io, 0, dpni_dev->mc_handle,
			irq_index, &status);
	if (unlikely(err)) {
		netdev_err(net_dev, "Can't get irq status (err %d)", err);
		clear = 0xffffffff;
		goto out;
	}

	if (status & DPNI_IRQ_EVENT_LINK_CHANGED) {
		clear |= DPNI_IRQ_EVENT_LINK_CHANGED;
		link_state_update(netdev_priv(net_dev));
	}

out:
	dpni_clear_irq_status(dpni_dev->mc_io, 0, dpni_dev->mc_handle,
			irq_index, clear);
	return IRQ_HANDLED;
}

static int setup_irqs(struct fsl_mc_device *ls_dev)
{
	int err = 0;
	struct fsl_mc_device_irq *irq;
	u8 irq_index = DPNI_IRQ_INDEX;
	u32 mask = DPNI_IRQ_EVENT_LINK_CHANGED;

	err = fsl_mc_allocate_irqs(ls_dev);
	if (err) {
		dev_err(&ls_dev->dev, "MC irqs allocation failed\n");
		return err;
	}

	irq = ls_dev->irqs[0];
	err = devm_request_threaded_irq(&ls_dev->dev, irq->irq_number,
			dpni_irq0_handler,
			dpni_irq0_handler_thread,
			IRQF_NO_SUSPEND | IRQF_ONESHOT,
			dev_name(&ls_dev->dev), &ls_dev->dev);
	if (err < 0) {
		dev_err(&ls_dev->dev, "devm_request_threaded_irq(): %d", err);
		goto free_mc_irq;
	}

	err = dpni_set_irq_mask(ls_dev->mc_io, 0, ls_dev->mc_handle,
			irq_index, mask);
	if (err < 0) {
		dev_err(&ls_dev->dev, "dpni_set_irq_mask(): %d", err);
		goto free_irq;
	}

	err = dpni_set_irq_enable(ls_dev->mc_io, 0, ls_dev->mc_handle,
			irq_index, 1);
	if (err < 0) {
		dev_err(&ls_dev->dev, "dpni_set_irq_enable(): %d", err);
		goto free_irq;
	}

	return 0;

free_irq:
	devm_free_irq(&ls_dev->dev, irq->irq_number, &ls_dev->dev);
free_mc_irq:
	fsl_mc_free_irqs(ls_dev);

	return err;
}

static void add_ch_napi(struct dpaa2_eth_priv *priv)
{
	int i;
	struct dpaa2_eth_channel *ch;

	for (i = 0; i < priv->num_channels; i++) {
		ch = priv->channel[i];
		/* NAPI weight *MUST* be a multiple of DPAA2_ETH_STORE_SIZE */
		netif_napi_add(priv->net_dev, &ch->napi, dpaa2_eth_poll,
				NAPI_POLL_WEIGHT);
	}
#if FSLU_NVME_INIC
	if((if_count >= 0) && (if_count < NUM_NET_DEV))
	{
		netif_napi_add(priv->net_dev,&priv->napi, nic_tx_napi,NAPI_POLL_WEIGHT);
	}
#endif /* FSLU_NVME_INIC */
}

static void del_ch_napi(struct dpaa2_eth_priv *priv)
{
	int i;
	struct dpaa2_eth_channel *ch;

	for (i = 0; i < priv->num_channels; i++) {
		ch = priv->channel[i];
		netif_napi_del(&ch->napi);
	}
}

/* SysFS support */
static ssize_t dpaa2_eth_show_tx_shaping(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct dpaa2_eth_priv *priv = netdev_priv(to_net_dev(dev));
	/* No MC API for getting the shaping config. We're stateful. */
	struct dpni_tx_shaping_cfg *scfg = &priv->shaping_cfg;

	return sprintf(buf, "%u %hu\n", scfg->rate_limit, scfg->max_burst_size);
}

static ssize_t dpaa2_eth_write_tx_shaping(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	int err, items;
	struct dpaa2_eth_priv *priv = netdev_priv(to_net_dev(dev));
	struct dpni_tx_shaping_cfg scfg;

	items = sscanf(buf, "%u %hu", &scfg.rate_limit, &scfg.max_burst_size);
	if (items != 2) {
		pr_err("Expected format: \"rate_limit(Mbps) max_burst_size(bytes)\"\n");
		return -EINVAL;
	}
	/* Size restriction as per MC API documentation */
	if (scfg.max_burst_size > 64000) {
		pr_err("max_burst_size must be <= 64000, thanks.\n");
		return -EINVAL;
	}

	err = dpni_set_tx_shaping(priv->mc_io, 0, priv->mc_token, &scfg);
	if (err) {
		dev_err(dev, "dpni_set_tx_shaping() failed\n");
		return -EPERM;
	}
	/* If successful, save the current configuration for future inquiries */
	priv->shaping_cfg = scfg;

	return count;
}

static ssize_t dpaa2_eth_show_txconf_cpumask(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct dpaa2_eth_priv *priv = netdev_priv(to_net_dev(dev));

	return cpumap_print_to_pagebuf(1, buf, &priv->txconf_cpumask);
}

static ssize_t dpaa2_eth_write_txconf_cpumask(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct dpaa2_eth_priv *priv = netdev_priv(to_net_dev(dev));
	struct dpaa2_eth_fq *fq;
	bool running = netif_running(priv->net_dev);
	int i, err;

	err = cpulist_parse(buf, &priv->txconf_cpumask);
	if (err)
		return err;

	/* Only accept CPUs that have an affine DPIO */
	if (!cpumask_subset(&priv->txconf_cpumask, &priv->dpio_cpumask)) {
		netdev_info(priv->net_dev,
				"cpumask must be a subset of 0x%lx\n",
				*cpumask_bits(&priv->dpio_cpumask));
		cpumask_and(&priv->txconf_cpumask, &priv->dpio_cpumask,
				&priv->txconf_cpumask);
	}

	/* Rewiring the TxConf FQs requires interface shutdown.
	 */
	if (running) {
		err = dpaa2_eth_stop(priv->net_dev);
		if (err)
			return -ENODEV;
	}

	/* Set the new TxConf FQ affinities */
	set_fq_affinity(priv);

	/* dpaa2_eth_open() below will *stop* the Tx queues until an explicit
	 * link up notification is received. Give the polling thread enough time
	 * to detect the link state change, or else we'll end up with the
	 * transmission side forever shut down.
	 */
	if (priv->do_link_poll)
		msleep(2 * DPAA2_ETH_LINK_STATE_REFRESH);

	for (i = 0; i < priv->num_fqs; i++) {
		fq = &priv->fq[i];
		if (fq->type != DPAA2_TX_CONF_FQ)
			continue;
		setup_tx_flow(priv, fq);
	}

	if (running) {
		err = dpaa2_eth_open(priv->net_dev);
		if (err)
			return -ENODEV;
	}

	return count;
}

static struct device_attribute dpaa2_eth_attrs[] = {
	__ATTR(txconf_cpumask,
			S_IRUSR | S_IWUSR,
			dpaa2_eth_show_txconf_cpumask,
			dpaa2_eth_write_txconf_cpumask),

	__ATTR(tx_shaping,
			S_IRUSR | S_IWUSR,
			dpaa2_eth_show_tx_shaping,
			dpaa2_eth_write_tx_shaping),
};

void dpaa2_eth_sysfs_init(struct device *dev)
{
	int i, err;

	for (i = 0; i < ARRAY_SIZE(dpaa2_eth_attrs); i++) {
		err = device_create_file(dev, &dpaa2_eth_attrs[i]);
		if (err) {
			dev_err(dev, "ERROR creating sysfs file\n");
			goto undo;
		}
	}
	return;

undo:
	while (i > 0)
		device_remove_file(dev, &dpaa2_eth_attrs[--i]);
}

void dpaa2_eth_sysfs_remove(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dpaa2_eth_attrs); i++)
		device_remove_file(dev, &dpaa2_eth_attrs[i]);
}

static int dpaa2_eth_probe(struct fsl_mc_device *dpni_dev)
{
#if FSLU_NVME_INIC
	struct buf_pool_desc *bman_pool = NULL;
#endif /* FSLU_NVME_INIC */

	struct device			*dev;
	struct net_device		*net_dev = NULL;
	struct dpaa2_eth_priv		*priv = NULL;
	int				err = 0;

	dev = &dpni_dev->dev;

#if FSLU_NVME_INIC
	if(if_count == 0 ) {

		/*INBOUND AND OUTBOUND PCIE MAPPING*/
		global->pci_config = (uint64_t)ioremap_nocache(PEX3,0xfffff);
		if (!global->pci_config) {
			printk("PEX3 iormap failed\n");
			return -ENOMEM;
		}
		
		iowrite32(0x1, (global->pci_config + 0x8bc));/*enable read write*/
		iowrite32(0x0, (global->pci_config + 0x507f8));/*PF's related*/

		/*outbound mapings can be moved to u-boot level*/
#if 1 
		/*outbound of whole x86 RAM 4GB*/
		iowrite32(0x00000002, (global->pci_config + 0x900));/*set outbound region2 as current region*/
		iowrite32(0x00000000, (global->pci_config + 0x90c));/*set lower  address*/
		iowrite32(0x00000030, (global->pci_config + 0x910));/*set  upper address*/
		iowrite32(0xffffffff, (global->pci_config + 0x914));/*limit*/
		iowrite32(0x00000000,(global->pci_config + 0x918));/*target lower address*/
		iowrite32(0x00000000,(global->pci_config + 0x91c));/*target upper address*/
		iowrite32(0x00000000,(global->pci_config + 0x904));/*ctrl_reg mem region*/
		iowrite32(0x80000000,(global->pci_config + 0x908));/*enable address match mode*/

		/*outbound of whole x86 RAM  second 4GB*/
		iowrite32(0x00000000, (global->pci_config + 0x900));
		iowrite32(0x00000000, (global->pci_config + 0x90c));
		iowrite32(0x00000031, (global->pci_config + 0x910));
		iowrite32(0xffffffff, (global->pci_config + 0x914)); 
		iowrite32(0x00000000,(global->pci_config + 0x918));
		iowrite32(0x00000001,(global->pci_config + 0x91c));
		iowrite32(0x00000000,(global->pci_config + 0x904));
		iowrite32(0x80000000,(global->pci_config + 0x908));
#endif
		iowrite32(0x80000000, (global->pci_config + 0x900));
		iowrite32(0x08000000, (global->pci_config + 0x918));
		iowrite32(0x00000000, (global->pci_config + 0x91c));
		iowrite32(0x00100000, (global->pci_config + 0x904));
		iowrite32(0xc0080000, (global->pci_config + 0x908));

		global->dma_virt = (uint64_t)dma_alloc_coherent(dev,SHARED_CONF_SIZE,&global->dma_handle,GFP_KERNEL);
		if(!global->dma_virt) {
			printk("PCIE:DMA alloc failed \n");
			err= -ENOMEM;
			goto err_dma_alloc;
		}
		
		memset(global->dma_virt, 0, SHARED_CONF_SIZE);
		global->dma_bkp = global->dma_handle;
		global->dma_handle = PTR_ALIGN(global->dma_handle,0x4000000);
		global->dma_virt =  (void *) ((unsigned long) (global->dma_virt) + (global->dma_handle - global->dma_bkp));
		
		iowrite32(0x80000002, (global->pci_config + 0x900));
		iowrite32((global->dma_handle & 0xffffffff), (global->pci_config + 0x918));
		iowrite32((global->dma_handle >> 32), (global->pci_config + 0x91c));
		iowrite32(0x00100000, (global->pci_config + 0x904));
		iowrite32(0xc0080200,(global->pci_config + 0x908));

		global->dreg_config = (uint64_t)ioremap_nocache(0x08000000,0x400000);
		if (!global->dreg_config) {
			printk("PCIE: DMA ioremap failed\n");
			err =-ENOMEM;
			goto err_ccsr_ioremap;
		}

		global->pci_outbound=(uint64_t)ioremap_nocache(PEX_OFFSET,0x7ffffffff);
		if (!global->pci_outbound) {
			printk("PEX3 iormap failed\n");
			err= -ENOMEM;
			goto err_outbound_ioremap;
		}
		global->dma_netreg = global->dma_virt;

#if FSLU_NVME_INIC_QDMA
		/* qdma engine channel relted registrations*/
		dma_cap_mask_t mask;
		dma_cap_zero(mask);
		dma_cap_set(DMA_MEMCPY, mask);
		global->dma_chan = dma_request_channel(mask, NULL, NULL);
		if(!global->dma_chan)
		{
			//TODO
			printk("qdma-client unbale to get channel use cpu memcpy \n");
			return -ENOMEM;
		}
		global->dma_device = global->dma_chan->device;

		if(global->dma_device->copy_align)
		{
			printk("qdma-client alignment required align before starting dma transaction\n");
			return -ENOMEM;
		}

#endif/*FSLU_NVME_INIC_QDMA*/

	}
#endif

	/* Net device */
	net_dev = alloc_etherdev_mq(sizeof(*priv), DPAA2_ETH_MAX_TX_QUEUES);
	if (!net_dev) {
		dev_err(dev, "alloc_etherdev_mq() failed\n");
		return -ENOMEM;
	}

	SET_NETDEV_DEV(net_dev, dev);
	dev_set_drvdata(dev, net_dev);

	priv = netdev_priv(net_dev);
	priv->net_dev = net_dev;

	/* Obtain a MC portal */
	err = fsl_mc_portal_allocate(dpni_dev, FSL_MC_IO_ATOMIC_CONTEXT_PORTAL,
			&priv->mc_io);
	if (err) {
		dev_err(dev, "MC portal allocation failed\n");
		goto err_portal_alloc;
	}

	/* MC objects initialization and configuration */
	err = setup_dpni(dpni_dev);
	if (err)
		goto err_dpni_setup;

	err = setup_dpio(priv);
	if (err)
		goto err_dpio_setup;

	setup_fqs(priv);
#if FSLU_NVME_INIC
	priv->interface_id = if_count;
	if((if_count >= 0) && (if_count < NUM_NET_DEV))
	{
		/*dummy qdma*/
		priv->poll_host = kthread_create(inic_poll_host_work,(void *)priv,"interface ni%d thread",dpni_dev->obj_desc.id);
		kthread_bind(priv->poll_host,(dpni_dev->obj_desc.id+3));

#if FSLU_NVME_INIC_FPGA_IRQ
		struct of_phandle_args irq_data;
		static struct device_node *nic_of_genirq_node;
		int ret1;

		if (!nic_of_genirq_node)
			nic_of_genirq_node = of_find_matching_node(NULL, nic_of_genirq_match);

		if (WARN_ON(!nic_of_genirq_node)){
			printk("node WARN\n");
			return -1;
		}
		memset(&irq_data,0,sizeof(irq_data));
		irq_data.np = nic_of_genirq_node;
		irq_data.args_count = 3;
		irq_data.args[0] = 0;
		irq_data.args[1] = ((irq_start+if_count) - 32);
		irq_data.args[2] = IRQ_TYPE_EDGE_RISING;

		priv->fpga_irq = irq_create_of_mapping(&irq_data);
		if (WARN_ON(!priv->fpga_irq)){
			printk("mapping WARN\n");
			priv->fpga_irq = (irq_start+if_count);
		}
		snprintf(priv->tx_name, sizeof(priv->tx_name) - 1,"INIC TX IRQ%d", if_count);
		ret1 = request_irq(priv->fpga_irq,nic_irq_handler,0,priv->tx_name,priv);
		if (ret1) {
			err=ret1;
			printk("PCIE:Unable to register irq handler for interafce %d \n",if_count);
			goto err_register_irq;
		}
#endif
		/*VVDN init TX/RX BD and BMAN BD */
		priv->netregs =global->dma_netreg + (INTERFACE_REG * if_count);
		priv->g_outaddr=global->pci_outbound;
		priv->tx_base = priv->netregs + PCINET_TXBD_BASE ;
		priv->bman_buf =priv->netregs;
		bman_pool=priv->bman_buf;
		priv->g_ccsr=global->pci_config;
		priv->fpga_reg = global->dreg_config;

#if FSLU_NVME_INIC_QDMA
		int i;
		void *buf_addr;
		priv->tx_buf_count=0;
		priv->rx_buf_count=0;
		priv->tx_buf_ready=0;
		priv->tx_buf_free=0;
		priv->tx_buf_dma=0;
		/*qdma store in priv structure can be used individual port requirment*/
		priv->dma_chan=global->dma_chan;
		priv->dma_device=global->dma_device;

		for(i=0;i<DPAA2_ETH_NUM_TX_BUFS;i++)
		{
			priv->tx_buf[priv->tx_buf_count]=kzalloc(sizeof(struct tx_buf_desc),GFP_KERNEL);

			buf_addr= napi_alloc_frag(DPAA2_ETH_BUF_RAW_SIZE(priv));
			if (unlikely(!buf_addr)) {
				printk("PCIE: %s buffer allocation failed\n",__func__);
			}

			buf_addr = PTR_ALIGN(buf_addr, DPAA2_ETH_TX_BUF_ALIGN);
			priv->tx_buf[priv->tx_buf_count]->addr = virt_to_phys(buf_addr);
			/* store physical address of tx buffers since we are handling physical only in tx_napi and tx_conform*/
			priv->tx_buf[priv->tx_buf_count++]->flag = BD_MEM_READY;

		}
#endif/*FSLU_NVME_INIC_QDMA*/
	}
#endif/*FSLU_NVME_INIC*/

	err = setup_dpbp(priv);
	if (err)
		goto err_dpbp_setup;

	err = bind_dpni(priv);
	if (err)
		goto err_bind;

	/* Add a NAPI context for each channel */
	add_ch_napi(priv);

	/* Percpu statistics */
	priv->percpu_stats = alloc_percpu(*priv->percpu_stats);
	if (!priv->percpu_stats) {
		dev_err(dev, "alloc_percpu(percpu_stats) failed\n");
		err = -ENOMEM;
		goto err_alloc_percpu_stats;
	}
	priv->percpu_extras = alloc_percpu(*priv->percpu_extras);
	if (!priv->percpu_extras) {
		dev_err(dev, "alloc_percpu(percpu_extras) failed\n");
		err = -ENOMEM;
		goto err_alloc_percpu_extras;
	}

	snprintf(net_dev->name, IFNAMSIZ, "ni%d", dpni_dev->obj_desc.id);
	if (!dev_valid_name(net_dev->name)) {
		dev_warn(&net_dev->dev,
				"netdevice name \"%s\" cannot be used, reverting to default..\n",
				net_dev->name);
		dev_alloc_name(net_dev, "eth%d");
		dev_warn(&net_dev->dev, "using name \"%s\"\n", net_dev->name);
	}

	err = netdev_init(net_dev);
	if (err)
		goto err_netdev_init;
#if FSLU_NVME_INIC
	/*add the hw mac address in LS2 bar2*/
	if((if_count >= 0) && (if_count < NUM_NET_DEV))
	{
		bman_pool->obj_id= dpni_dev->obj_desc.id;
		bman_pool->mac_id[0] = priv->net_dev->dev_addr[0];
		bman_pool->mac_id[1] = priv->net_dev->dev_addr[1];
		bman_pool->mac_id[2] = priv->net_dev->dev_addr[2];
		bman_pool->mac_id[3] = priv->net_dev->dev_addr[3];
		bman_pool->mac_id[4] = priv->net_dev->dev_addr[4];
		bman_pool->mac_id[5] = priv->net_dev->dev_addr[5];
		bman_pool->mac_id[6] = MAC_SET;
	}
#endif /* FSLU_NVME_INIC */
	/* Configure checksum offload based on current interface flags */
	err = set_rx_csum(priv, !!(net_dev->features & NETIF_F_RXCSUM));
	if (err)
		goto err_csum;

	err = set_tx_csum(priv, !!(net_dev->features &
				(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM)));
	if (err)
		goto err_csum;

	err = alloc_rings(priv);
	if (err)
		goto err_alloc_rings;

	net_dev->ethtool_ops = &dpaa2_ethtool_ops;

	err = setup_irqs(dpni_dev);
	if (err) {
		netdev_warn(net_dev, "Failed to set link interrupt, fall back to polling\n");
		priv->poll_thread = kthread_run(poll_link_state, priv,
				"%s_poll_link", net_dev->name);
		if (IS_ERR(priv->poll_thread)) {
			netdev_err(net_dev, "Error starting polling thread\n");
			goto err_poll_thread;
		}
		priv->do_link_poll = true;
	}

#if FSLU_NVME_INIC
	if((if_count >= 0) && (if_count < NUM_NET_DEV))
	{

		bman_pool->tx_data_offset =priv->tx_data_offset;
		bman_pool->tx_irq = BD_IRQ_CLEAR;//moved to host memory need to remove
		bman_pool->iface_status = IFACE_IDLE;
		bman_pool->bpid = priv->dpbp_attrs.bpid;
		priv->msix_addr = ((uint64_t)global->pci_config+0x948);//requires host handshake but generate msix only after handshake
		priv->msix_value =if_count + 0x1000000;//requires host handshake
		/*initialiaze locks */
		spin_lock_init(&priv->tx_desc_lock);
		spin_lock_init(&priv->tx_share_lock);
		spin_lock_init(&priv->rx_desc_lock);
		spin_lock_init(&priv->tx_conf_lock);
#if FSLU_NVME_INIC_QDMA
		spin_lock_init(&priv->rx_dma_lock);
#endif
		/*initialize WORK queue */
		INIT_WORK(&priv->net_start_work, nic_open);
		INIT_WORK(&priv->net_stop_work, nic_stop);
		/*store priv members in global memory can be accesed from common irq or other place*/
		global->priv[if_count]=(uint64_t)priv;
		/*dummy qdma*/
		wake_up_process(priv->poll_host);/*starting kthread */
	}
	if_count++;

#endif /* FSLU_NVME_INIC */

	dpaa2_eth_sysfs_init(&net_dev->dev);
	dpaa2_dbg_add(priv);

	dev_info(dev, "Probed interface %s\n", net_dev->name);
	return 0;

err_poll_thread:
	free_rings(priv);
err_alloc_rings:
err_csum:
	unregister_netdev(net_dev);
err_netdev_init:
	free_percpu(priv->percpu_extras);
err_alloc_percpu_extras:
	free_percpu(priv->percpu_stats);
err_alloc_percpu_stats:
	del_ch_napi(priv);
err_bind:
	free_dpbp(priv);
err_dpbp_setup:
	free_dpio(priv);
err_dpio_setup:
	kfree(priv->cls_rule);
	dpni_close(priv->mc_io, 0, priv->mc_token);
err_dpni_setup:
	fsl_mc_portal_free(priv->mc_io);
err_portal_alloc:
	dev_set_drvdata(dev, NULL);
	free_netdev(net_dev);
#if FSLU_NVME_INIC
#if FSLU_NVME_INIC_FPGA_IRQ
err_register_irq:
	iounmap(global->pci_outbound);
#endif
err_outbound_ioremap:
	iounmap(global->dreg_config);
err_ccsr_ioremap:
	global->dma_virt =(void *) ((unsigned long) (global->dma_virt) - (global->dma_handle - global->dma_bkp));
	dma_free_coherent(dev,SHARED_CONF_SIZE,global->dma_virt,global->dma_handle);
err_dma_alloc:
	iounmap(global->pci_config);
#endif

	return err;
}

static int dpaa2_eth_remove(struct fsl_mc_device *ls_dev)
{
	struct device		*dev;
	struct net_device	*net_dev;
	struct dpaa2_eth_priv *priv;

	dev = &ls_dev->dev;
	net_dev = dev_get_drvdata(dev);
	priv = netdev_priv(net_dev);

	dpaa2_dbg_remove(priv);
	dpaa2_eth_sysfs_remove(&net_dev->dev);

	unregister_netdev(net_dev);
	dev_info(net_dev->dev.parent, "Removed interface %s\n", net_dev->name);

	free_dpio(priv);
	free_rings(priv);
	del_ch_napi(priv);
	free_dpbp(priv);
	free_dpni(priv);

	fsl_mc_portal_free(priv->mc_io);

	free_percpu(priv->percpu_stats);
	free_percpu(priv->percpu_extras);

	if (priv->do_link_poll)
		kthread_stop(priv->poll_thread);
	else
		fsl_mc_free_irqs(ls_dev);

	kfree(priv->cls_rule);

	dev_set_drvdata(dev, NULL);
	free_netdev(net_dev);

	return 0;
}

static const struct fsl_mc_device_match_id dpaa2_eth_match_id_table[] = {
	{
		.vendor = FSL_MC_VENDOR_FREESCALE,
		.obj_type = "dpni",
		.ver_major = DPNI_VER_MAJOR,
		.ver_minor = DPNI_VER_MINOR
	},
	{ .vendor = 0x0 }
};

static struct fsl_mc_driver dpaa2_eth_driver = {
	.driver = {
		.name		= KBUILD_MODNAME,
		.owner		= THIS_MODULE,
	},
	.probe		= dpaa2_eth_probe,
	.remove		= dpaa2_eth_remove,
	.match_id_table = dpaa2_eth_match_id_table
};

static int __init dpaa2_eth_driver_init(void)
{
	int err;

	dpaa2_eth_dbg_init();

	err = fsl_mc_driver_register(&dpaa2_eth_driver);
	if (err) {
		dpaa2_eth_dbg_exit();
		return err;
	}

	return 0;
}

static void __exit dpaa2_eth_driver_exit(void)
{
	fsl_mc_driver_unregister(&dpaa2_eth_driver);
	dpaa2_eth_dbg_exit();
}

module_init(dpaa2_eth_driver_init);
module_exit(dpaa2_eth_driver_exit);
