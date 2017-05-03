/* Freescale QUICC Engine HDLC Device Driver
 *
 * Copyright 2014 Freescale Semiconductor Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/hdlc.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/stddef.h>
#include <soc/fsl/qe/qe_tdm.h>
#include <uapi/linux/if_arp.h>

#include "fsl_ucc_hdlc.h"

#define DRV_DESC "Freescale QE UCC HDLC Driver"
#define DRV_NAME "ucc_hdlc"

#define TDM_PPPOHT_SLIC_MAXIN
/* #define DEBUG */
/* #define QE_HDLC_TEST */
#define BROKEN_FRAME_INFO

static struct ucc_tdm_info utdm_primary_info = {
	.uf_info = {
		.tsa = 0,
		.cdp = 0,
		.cds = 1,
		.ctsp = 1,
		.ctss = 1,
		.revd = 0,
		.urfs = 256,
		.utfs = 256,
		.urfet = 128,
		.urfset = 192,
		.utfet = 128,
		.utftt = 0x40,
		.ufpt = 256,
		.mode = UCC_FAST_PROTOCOL_MODE_HDLC,
		.ttx_trx = UCC_FAST_GUMR_TRANSPARENT_TTX_TRX_NORMAL,
		.tenc = UCC_FAST_TX_ENCODING_NRZ,
		.renc = UCC_FAST_RX_ENCODING_NRZ,
		.tcrc = UCC_FAST_16_BIT_CRC,
		.synl = UCC_FAST_SYNC_LEN_NOT_USED,
	},

	.si_info = {
#ifdef CONFIG_FSL_PQ_MDS_T1
		.simr_rfsd = 1,		/* TDM card need 1 bit delay */
		.simr_tfsd = 0,
#else
#ifdef TDM_PPPOHT_SLIC_MAXIN
		.simr_rfsd = 1,
		.simr_tfsd = 2,
#else
		.simr_rfsd = 0,
		.simr_tfsd = 0,
#endif
#endif
		.simr_crt = 0,
		.simr_sl = 0,
		.simr_ce = 1,
		.simr_fe = 1,
		.simr_gm = 0,
	},
};

static struct ucc_tdm_info utdm_info[MAX_HDLC_NUM];

#ifdef DEBUG
static void mem_disp(u8 *addr, int size)
{
	void *i;
	int size16_aling = (size >> 4) << 4;
	int size4_aling = (size >> 2) << 2;
	int not_align = 0;

	if (size % 16)
		not_align = 1;

	for (i = addr;  i < addr + size16_aling; i += 16) {
		u32 *i32 = i;

		pr_info("0x%08p: %08x %08x %08x %08x\r\n",
			i32, be32_to_cpu(i32[0]), be32_to_cpu(i32[1]),
			be32_to_cpu(i32[2]), be32_to_cpu(i32[3]));
	}

	if (not_align == 1)
		pr_info("0x%08p: ", i);
	for (; i < addr + size4_aling; i += 4)
		pr_info("%08x ", be32_to_cpu(*((u32 *)(i))));
	for (; i < addr + size; i++)
		pr_info("%02x", *((u8 *)(i)));
	if (not_align == 1)
		pr_info("\r\n");
}

static void dump_ucc(struct ucc_hdlc_private *priv)
{
	struct ucc_hdlc_param *ucc_pram;

	ucc_pram = priv->ucc_pram;

	dev_info(priv->dev, "DumpiniCC %d Registers\n",
		 priv->ut_info->uf_info.ucc_num);
	ucc_fast_dump_regs(priv->uccf);
	dev_info(priv->dev, "Dumping UCC %d Parameter RAM\n",
		 priv->ut_info->uf_info.ucc_num);
	dev_info(priv->dev, "rbase = 0x%x\n", ioread32be(&ucc_pram->rbase));
	dev_info(priv->dev, "rbptr = 0x%x\n", ioread32be(&ucc_pram->rbptr));
	dev_info(priv->dev, "mrblr = 0x%x\n", ioread16be(&ucc_pram->mrblr));
	dev_info(priv->dev, "rbdlen = 0x%x\n", ioread16be(&ucc_pram->rbdlen));
	dev_info(priv->dev, "rbdstat = 0x%x\n", ioread16be(&ucc_pram->rbdstat));
	dev_info(priv->dev, "rstate = 0x%x\n", ioread32be(&ucc_pram->rstate));
	dev_info(priv->dev, "rdptr = 0x%x\n", ioread32be(&ucc_pram->rdptr));
	dev_info(priv->dev, "riptr = 0x%x\n", ioread16be(&ucc_pram->riptr));
	dev_info(priv->dev, "tbase = 0x%x\n", ioread32be(&ucc_pram->tbase));
	dev_info(priv->dev, "tbptr = 0x%x\n", ioread32be(&ucc_pram->tbptr));
	dev_info(priv->dev, "tbdlen = 0x%x\n", ioread16be(&ucc_pram->tbdlen));
	dev_info(priv->dev, "tbdstat = 0x%x\n", ioread16be(&ucc_pram->tbdstat));
	dev_info(priv->dev, "tstate = 0x%x\n", ioread32be(&ucc_pram->tstate));
	dev_info(priv->dev, "tdptr = 0x%x\n", ioread32be(&ucc_pram->tdptr));
	dev_info(priv->dev, "tiptr = 0x%x\n", ioread16be(&ucc_pram->tiptr));
	dev_info(priv->dev, "rcrc = 0x%x\n", ioread32be(&ucc_pram->rcrc));
	dev_info(priv->dev, "tcrc = 0x%x\n", ioread32be(&ucc_pram->tcrc));
	dev_info(priv->dev, "c_mask = 0x%x\n", ioread32be(&ucc_pram->c_mask));
	dev_info(priv->dev, "c_pers = 0x%x\n", ioread32be(&ucc_pram->c_pres));
	dev_info(priv->dev, "disfc = 0x%x\n", ioread16be(&ucc_pram->disfc));
	dev_info(priv->dev, "crcec = 0x%x\n", ioread16be(&ucc_pram->crcec));
}

static void dump_bds(struct ucc_hdlc_private *priv)
{
	int length;

	if (priv->tx_bd_base) {
		length = sizeof(struct qe_bd) * TX_BD_RING_LEN;
		dev_info(priv->dev, " Dump tx BDs\n");
		mem_disp((u8 *)priv->tx_bd_base, length);
	}

	if (priv->rx_bd_base) {
		length = sizeof(struct qe_bd) * RX_BD_RING_LEN;
		dev_info(priv->dev, " Dump rx BDs\n");
		mem_disp((u8 *)priv->rx_bd_base, length);
	}
}

static void dump_priv(struct ucc_hdlc_private *priv)
{
	dev_info(priv->dev, "ut_info = 0x%x\n", (u32)priv->ut_info);
	dev_info(priv->dev, "uccf = 0x%x\n", (u32)priv->uccf);
	dev_info(priv->dev, "uf_regs = 0x%x\n", (u32)priv->uf_regs);
	dev_info(priv->dev, "si_regs = 0x%x\n", (u32)priv->utdm->si_regs);
	dev_info(priv->dev, "ucc_pram = 0x%x\n", (u32)priv->ucc_pram);
	dev_info(priv->dev, "tdm_port = 0x%x\n", (u32)priv->utdm->tdm_port);
	dev_info(priv->dev, "siram_entry_id = 0x%x\n",
		 priv->utdm->siram_entry_id);
	dev_info(priv->dev, "siram = 0x%x\n", (u32)priv->utdm->siram);
	dev_info(priv->dev, "tdm_mode = 0x%x\n", (u32)priv->utdm->tdm_mode);
	dev_info(priv->dev, "tdm_framer_type; = 0x%x\n",
		 (u32)priv->utdm->tdm_framer_type);
	dev_info(priv->dev, "rx_buffer; = 0x%x\n", (u32)priv->rx_buffer);
	dev_info(priv->dev, "tx_buffer; = 0x%x\n", (u32)priv->tx_buffer);
	dev_info(priv->dev, "dma_rx_addr; = 0x%x\n", (u32)priv->dma_rx_addr);
	dev_info(priv->dev, "tx_bd; = 0x%x\n", (u32)priv->tx_bd_base);
	dev_info(priv->dev, "rx_bd; = 0x%x\n", (u32)priv->rx_bd_base);
	dev_info(priv->dev, "curtx_bd = 0x%x\n", (u32)priv->curtx_bd);
	dev_info(priv->dev, "currx_bd = 0x%x\n", (u32)priv->currx_bd);
	dev_info(priv->dev, "ucc_pram_offset = 0x%x\n", priv->ucc_pram_offset);
}

#endif /* DEBUG */

static int uhdlc_init(struct ucc_hdlc_private *priv)
{
	struct ucc_tdm_info *ut_info;
	struct ucc_fast_info *uf_info;
	u32 cecr_subblock;
	u32 bd_status;
	int ret, i;
	void *bd_buffer;
	dma_addr_t bd_dma_addr;
	u32 riptr;
	u32 tiptr;
	u32 gumr;

	ut_info = priv->ut_info;
	uf_info = &ut_info->uf_info;

	if (priv->tsa) {
		uf_info->tsa = 1;
		uf_info->ctsp = 1;
	}
	uf_info->uccm_mask = (u32)((UCC_HDLC_UCCE_RXB | UCC_HDLC_UCCE_RXF |
				UCC_HDLC_UCCE_TXB) << 16);

	if (ucc_fast_init(uf_info, &priv->uccf)) {
		dev_err(priv->dev, "Failed to init uccf.");
		return -ENOMEM;
	}

	priv->uf_regs = priv->uccf->uf_regs;
	ucc_fast_disable(priv->uccf, COMM_DIR_RX | COMM_DIR_TX);

	/* Loopback mode */
	if (priv->loopback) {
		pr_info("TDM Mode: Loopback Mode\n");
		gumr = ioread32be(&priv->uf_regs->gumr);
		gumr |= (0x40000000 | UCC_FAST_GUMR_CDS | UCC_FAST_GUMR_TCI);
		gumr &= ~(UCC_FAST_GUMR_CTSP | UCC_FAST_GUMR_RSYN);
		iowrite32be(gumr, &priv->uf_regs->gumr);
	}

	/* Initialize SI */
	if (priv->tsa)
		init_si(priv->utdm, priv->ut_info);

	/* Write to QE CECR, UCCx channel to Stop Transmission */
	cecr_subblock = ucc_fast_get_qe_cr_subblock(uf_info->ucc_num);
	ret = qe_issue_cmd(QE_STOP_TX, cecr_subblock,
			   (u8)QE_CR_PROTOCOL_UNSPECIFIED, 0);

	/* Set UPSMR normal mode (need fixed)*/
	iowrite32be(0, &priv->uf_regs->upsmr);

	priv->rx_ring_size = RX_BD_RING_LEN;
	priv->tx_ring_size = TX_BD_RING_LEN;
	/* Alloc Rx BD */
	priv->rx_bd_base = dma_alloc_coherent(priv->dev,
			RX_BD_RING_LEN * sizeof(struct qe_bd *),
			&priv->dma_rx_bd, GFP_KERNEL);

	if (IS_ERR_VALUE((unsigned long)priv->rx_bd_base)) {
		dev_err(priv->dev, "Cannot allocate MURAM memory for RxBDs\n");
		ret = -ENOMEM;
		goto rxbd_alloc_error;
	}

	/* Alloc Tx BD */
	priv->tx_bd_base = dma_alloc_coherent(priv->dev,
			TX_BD_RING_LEN * sizeof(struct qe_bd *),
			&priv->dma_tx_bd, GFP_KERNEL);

	if (IS_ERR_VALUE((unsigned long)priv->tx_bd_base)) {
		dev_err(priv->dev, "Cannot allocate MURAM memory for TxBDs\n");
		ret = -ENOMEM;
		goto txbd_alloc_error;
	}

	/* Alloc parameter ram for ucc hdlc */
	priv->ucc_pram_offset = qe_muram_alloc(sizeof(priv->ucc_pram),
				ALIGNMENT_OF_UCC_HDLC_PRAM);

	if (IS_ERR_VALUE(priv->ucc_pram_offset)) {
		dev_err(priv->dev, "Can not allocate MURAM for hdlc prameter.\n");
		ret = -ENOMEM;
		goto pram_alloc_error;
	}

	priv->rx_skbuff = kmalloc_array(priv->rx_ring_size,
			sizeof(*priv->rx_skbuff), GFP_KERNEL);
	if (!priv->rx_skbuff)
		goto rx_skb_alloc_error;
	for (i = 0; i < priv->rx_ring_size; i++)
		priv->rx_skbuff[i] = NULL;

	priv->tx_skbuff = kmalloc_array(priv->tx_ring_size,
			sizeof(*priv->tx_skbuff), GFP_KERNEL);
	if (!priv->tx_skbuff)
		goto tx_skb_alloc_error;
	for (i = 0; i < priv->tx_ring_size; i++)
		priv->tx_skbuff[i] = NULL;

	priv->skb_curtx = 0;
	priv->skb_dirtytx = 0;
	priv->curtx_bd = priv->tx_bd_base;
	priv->dirty_tx = priv->tx_bd_base;
	priv->currx_bd = priv->rx_bd_base;
	priv->currx_bdnum = 0;

	/* init parameter base */
	cecr_subblock = ucc_fast_get_qe_cr_subblock(uf_info->ucc_num);
	ret = qe_issue_cmd(QE_ASSIGN_PAGE_TO_DEVICE, cecr_subblock,
			   QE_CR_PROTOCOL_UNSPECIFIED, priv->ucc_pram_offset);

	priv->ucc_pram = (struct ucc_hdlc_param __iomem *)
					qe_muram_addr(priv->ucc_pram_offset);

	/* Zero out parameter ram */
	memset_io(priv->ucc_pram, 0, sizeof(struct ucc_hdlc_param));

	/* Alloc riptr, tiptr */
	riptr = qe_muram_alloc(32, 32);
	if (IS_ERR_VALUE(riptr)) {
		dev_err(priv->dev, "Cannot allocate MURAM mem for Receive internal temp data pointer\n");
		ret = -ENOMEM;
		goto riptr_alloc_error;
	}

	tiptr = qe_muram_alloc(32, 32);
	if (IS_ERR_VALUE(tiptr)) {
		dev_err(priv->dev, "Cannot allocate MURAM mem for Transmit internal temp data pointer\n");
		ret = -ENOMEM;
		goto tiptr_alloc_error;
	}

	/* Set RIPTR, TIPTR */
	iowrite16be((u16)riptr, &priv->ucc_pram->riptr);
	iowrite16be((u16)tiptr, &priv->ucc_pram->tiptr);

	/* Set MRBLR */
	iowrite16be((u16)MAX_RX_BUF_LENGTH, &priv->ucc_pram->mrblr);

		/* Set RBASE, TBASE */
	iowrite32be((u32)priv->dma_rx_bd, &priv->ucc_pram->rbase);
	iowrite32be((u32)priv->dma_tx_bd, &priv->ucc_pram->tbase);

	/* Set RSTATE, TSTATE */
	iowrite32be(0x30000000, &priv->ucc_pram->rstate);
	iowrite32be(0x30000000, &priv->ucc_pram->tstate);

	/* Set C_MASK, C_PRES for 16bit CRC */
	iowrite32be(0x0000F0B8, &priv->ucc_pram->c_mask);
	iowrite32be(0x0000FFFF, &priv->ucc_pram->c_pres);

	iowrite16be(MAX_RX_BUF_LENGTH + 8, &priv->ucc_pram->mflr);
	iowrite16be(1, &priv->ucc_pram->rfthr);
	iowrite16be(1, &priv->ucc_pram->rfcnt);
	iowrite16be(DEFAULT_ADDR_MASK, &priv->ucc_pram->hmask);
	iowrite16be(DEFAULT_HDLC_ADDR, &priv->ucc_pram->haddr1);
	iowrite16be(DEFAULT_HDLC_ADDR, &priv->ucc_pram->haddr2);
	iowrite16be(DEFAULT_HDLC_ADDR, &priv->ucc_pram->haddr3);
	iowrite16be(DEFAULT_HDLC_ADDR, &priv->ucc_pram->haddr4);

	/* Get BD buffer */
	bd_buffer = dma_alloc_coherent(priv->dev,
				       (RX_BD_RING_LEN + TX_BD_RING_LEN) *
				       MAX_RX_BUF_LENGTH,
				       &bd_dma_addr, GFP_KERNEL);

	if (!bd_buffer) {
		dev_err(priv->dev, "Could not allocate buffer descriptors\n");
		return -ENOMEM;
	}

	memset(bd_buffer, 0, (RX_BD_RING_LEN + TX_BD_RING_LEN)
			* MAX_RX_BUF_LENGTH);

	priv->rx_buffer = bd_buffer;
	priv->tx_buffer = bd_buffer + RX_BD_RING_LEN * MAX_RX_BUF_LENGTH;

	priv->dma_rx_addr = bd_dma_addr;
	priv->dma_tx_addr = bd_dma_addr + RX_BD_RING_LEN * MAX_RX_BUF_LENGTH;

	for (i = 0; i < RX_BD_RING_LEN; i++) {
		if (i < (RX_BD_RING_LEN - 1))
			bd_status = R_E | R_I;
		else
			bd_status = R_E | R_I | R_W;

		iowrite32be(bd_status, (u32 *)(priv->rx_bd_base + i));
		iowrite32be(priv->dma_rx_addr + i * MAX_RX_BUF_LENGTH,
			    &priv->rx_bd_base[i].buf);
	}

	for (i = 0; i < TX_BD_RING_LEN; i++) {
		if (i < (TX_BD_RING_LEN - 1))
			bd_status =  T_I | T_TC;
		else
			bd_status =  T_I | T_TC | T_W;

		iowrite32be(bd_status, (u32 *)(priv->tx_bd_base + i));
		iowrite32be(priv->dma_tx_addr + i * MAX_RX_BUF_LENGTH,
			    &priv->tx_bd_base[i].buf);
	}

	return 0;

tiptr_alloc_error:
	qe_muram_free(riptr);
riptr_alloc_error:
	kfree(priv->tx_skbuff);
tx_skb_alloc_error:
	kfree(priv->rx_skbuff);
rx_skb_alloc_error:
	qe_muram_free(priv->ucc_pram_offset);
pram_alloc_error:
	dma_free_coherent(priv->dev,
			  TX_BD_RING_LEN * sizeof(struct qe_bd),
			  priv->tx_bd_base, priv->dma_tx_bd);
txbd_alloc_error:
	dma_free_coherent(priv->dev,
			  RX_BD_RING_LEN * sizeof(struct qe_bd),
			  priv->rx_bd_base, priv->dma_rx_bd);
rxbd_alloc_error:
	ucc_fast_free(priv->uccf);

	return ret;
}

static netdev_tx_t ucc_hdlc_tx(struct sk_buff *skb, struct net_device *dev)
{
	hdlc_device *hdlc = dev_to_hdlc(dev);
	struct ucc_hdlc_private *priv = (struct ucc_hdlc_private *)hdlc->priv;
	struct qe_bd __iomem *bd;
	u32 bd_status;
	unsigned long flags;
#ifdef QE_HDLC_TEST
	u8 *send_buf;
	int i;
#endif
	u16 *proto_head, tmp_head;

	switch (dev->type) {
	case ARPHRD_RAWHDLC:
		if (skb_headroom(skb) < HDLC_HEAD_LEN) {
			dev->stats.tx_dropped++;
			dev_kfree_skb(skb);
			netdev_err(dev, "No enough space for hdlc head\n");
			return -ENOMEM;
		}

		skb_push(skb, HDLC_HEAD_LEN);

		proto_head = (u16 *)skb->data;
		tmp_head = *proto_head;
		tmp_head = (tmp_head & HDLC_HEAD_MASK) |
			    htons(DEFAULT_HDLC_HEAD);
		*proto_head = tmp_head;

		dev->stats.tx_bytes += skb->len;
		break;

	case ARPHRD_PPP:
		proto_head = (u16 *)skb->data;
		if (*proto_head != ntohs(DEFAULT_PPP_HEAD)) {
			dev->stats.tx_dropped++;
			dev_kfree_skb(skb);
			netdev_err(dev, "Wrong ppp header\n");
			return -ENOMEM;
		}

		dev->stats.tx_bytes += skb->len;
		break;

	default:
		dev->stats.tx_dropped++;
		dev_kfree_skb(skb);
		netdev_err(dev, "Protocol not supported!\n");
		return -ENOMEM;

	} /*switch right bracket*/

#ifdef QE_HDLC_TEST
	pr_info("Tx data skb->len:%d ", skb->len);
	send_buf = (u8 *)skb->data;
	pr_info("\nTransmitted data:\n");
	for (i = 0; (i < 16); i++) {
		if (i == skb->len)
			pr_info("++++");
		else
		pr_info("%02x\n", send_buf[i]);
	}
#endif
	spin_lock_irqsave(&priv->lock, flags);

	/* Start from the next BD that should be filled */
	bd = priv->curtx_bd;
	bd_status = ioread32be((u32 __iomem *)bd);
	/* Save the skb pointer so we can free it later */
	priv->tx_skbuff[priv->skb_curtx] = skb;

	/* Update the current skb pointer (wrapping if this was the last) */
	priv->skb_curtx =
	    (priv->skb_curtx + 1) & TX_RING_MOD_MASK(TX_BD_RING_LEN);

	/* copy skb data to tx buffer for sdma processing */
	memcpy(priv->tx_buffer + (be32_to_cpu(bd->buf) - priv->dma_tx_addr),
	       skb->data, skb->len);

	/* set bd status and length */
	bd_status = (bd_status & T_W) | T_R | T_I | T_L | T_TC | skb->len;

	iowrite32be(bd_status, (u32 __iomem *)bd);

	/* Move to next BD in the ring */
	if (!(bd_status & T_W))
		bd += 1;
	else
		bd = priv->tx_bd_base;

	if (bd == priv->dirty_tx) {
		if (!netif_queue_stopped(dev))
			netif_stop_queue(dev);
	}

	priv->curtx_bd = bd;

	spin_unlock_irqrestore(&priv->lock, flags);

	return NETDEV_TX_OK;
}

static int hdlc_tx_done(struct ucc_hdlc_private *priv)
{
	/* Start from the next BD that should be filled */
	struct net_device *dev = priv->ndev;
	struct qe_bd *bd;		/* BD pointer */
	u32 bd_status;

	bd = priv->dirty_tx;
	bd_status = ioread32be((u32 __iomem *)bd);

	/* Normal processing. */
	while ((bd_status & T_R) == 0) {
		struct sk_buff *skb;

		/* BD contains already transmitted buffer.   */
		/* Handle the transmitted buffer and release */
		/* the BD to be used with the current frame  */

		skb = priv->tx_skbuff[priv->skb_dirtytx];
		if (!skb)
			break;
#ifdef QE_HDLC_TEST
		pr_info("TxBD: %x\n", bd_status);
#endif
		dev->stats.tx_packets++;
		memset(priv->tx_buffer +
		       (be32_to_cpu(bd->buf) - priv->dma_tx_addr),
		       0, skb->len);
		dev_kfree_skb_irq(skb);

		priv->tx_skbuff[priv->skb_dirtytx] = NULL;
		priv->skb_dirtytx =
		    (priv->skb_dirtytx +
		     1) & TX_RING_MOD_MASK(TX_BD_RING_LEN);

		/* We freed a buffer, so now we can restart transmission */
		if (netif_queue_stopped(dev))
			netif_wake_queue(dev);

		/* Advance the confirmation BD pointer */
		if (!(bd_status & T_W))
			bd += 1;
		else
			bd = priv->tx_bd_base;
		bd_status = ioread32be((u32 __iomem *)bd);
	}
	priv->dirty_tx = bd;

	return 0;
}

static int hdlc_rx_done(struct ucc_hdlc_private *priv, int rx_work_limit)
{
	struct net_device *dev = priv->ndev;
	struct sk_buff *skb;
	hdlc_device *hdlc = dev_to_hdlc(dev);
	struct qe_bd *bd;
	u32 bd_status;
	u16 length, howmany = 0;
	u8 *bdbuffer;
#ifdef QE_HDLC_TEST
	int i;
	static int entry;
#endif

	bd = priv->currx_bd;
	bd_status = ioread32be((u32 __iomem *)bd);

	/* while there are received buffers and BD is full (~R_E) */
	while (!((bd_status & (R_E)) || (--rx_work_limit < 0))) {
		if (bd_status & R_CR) {
#ifdef BROKEN_FRAME_INFO
			pr_info("Broken Frame with RxBD: %x\n", bd_status);
#endif
			dev->stats.rx_dropped++;
			goto recycle;
		}
		bdbuffer = priv->rx_buffer +
			(priv->currx_bdnum * MAX_RX_BUF_LENGTH);
		length = (u16)(bd_status & BD_LENGTH_MASK);

#ifdef QE_HDLC_TEST
		pr_info("Received data length:%d", length);
		pr_info("while entry times:%d", entry++);

		pr_info("\nReceived data:\n");
		for (i = 0; (i < 16); i++) {
			if (i == length)
				pr_info("++++");
			else
			pr_info("%02x\n", bdbuffer[i]);
		}
#endif

		switch (dev->type) {
		case ARPHRD_RAWHDLC:
			bdbuffer += HDLC_HEAD_LEN;
			length -= (HDLC_HEAD_LEN + HDLC_CRC_SIZE);

			skb = dev_alloc_skb(length);
			if (!skb) {
				dev->stats.rx_dropped++;
				return -ENOMEM;
			}

			skb_put(skb, length);
			skb->len = length;
			skb->dev = dev;
			memcpy(skb->data, bdbuffer, length);
			break;

		case ARPHRD_PPP:
			length -= HDLC_CRC_SIZE;

			skb = dev_alloc_skb(length);
			if (!skb) {
				dev->stats.rx_dropped++;
				return -ENOMEM;
			}

			skb_put(skb, length);
			skb->len = length;
			skb->dev = dev;
			memcpy(skb->data, bdbuffer, length);
			break;
		}

		dev->stats.rx_packets++;
		dev->stats.rx_bytes += skb->len;
		howmany++;
		if (hdlc->proto)
			skb->protocol = hdlc_type_trans(skb, dev);
#ifdef QE_HDLC_TEST
		pr_info("skb->protocol:%x\n", skb->protocol);
#endif
		netif_receive_skb(skb);

recycle:
		iowrite32be((bd_status & ~BD_LENGTH_MASK) | R_E | R_I,
			    (u32 *)bd);

		/* update to point at the next bd */
		if (bd_status & R_W) {
			priv->currx_bdnum = 0;
			bd = priv->rx_bd_base;
		} else {
			if (priv->currx_bdnum < (RX_BD_RING_LEN - 1))
				priv->currx_bdnum += 1;
			else
				priv->currx_bdnum = RX_BD_RING_LEN - 1;

			bd += 1;
		}

		bd_status = ioread32be((u32 __iomem *)bd);
	}

	priv->currx_bd = bd;
	return howmany;
}

static int ucc_hdlc_poll(struct napi_struct *napi, int budget)
{
	struct ucc_hdlc_private *priv = container_of(napi,
						     struct ucc_hdlc_private,
						     napi);
	int howmany;

	/* Tx event processing */
	spin_lock(&priv->lock);
		hdlc_tx_done(priv);
	spin_unlock(&priv->lock);

	howmany = 0;
	howmany += hdlc_rx_done(priv, budget - howmany);

	if (howmany < budget) {
		napi_complete(napi);
		qe_setbits32(priv->uccf->p_uccm,
			     (UCCE_HDLC_RX_EVENTS | UCCE_HDLC_TX_EVENTS) << 16);
	}

	return howmany;
}

static irqreturn_t ucc_hdlc_irq_handler(int irq, void *dev_id)
{
	struct ucc_hdlc_private *priv = (struct ucc_hdlc_private *)dev_id;
	struct net_device *dev = priv->ndev;
	struct ucc_fast_private *uccf;
	struct ucc_tdm_info *ut_info;
	u32 ucce;
	u32 uccm;

	ut_info = priv->ut_info;
	uccf = priv->uccf;

	ucce = ioread32be(uccf->p_ucce);
	uccm = ioread32be(uccf->p_uccm);
	ucce &= uccm;
	iowrite32be(ucce, uccf->p_ucce);
#ifdef QE_HDLC_TEST
	pr_info("irq ucce:%x\n", ucce);
#endif

	if ((ucce >> 16) & (UCCE_HDLC_RX_EVENTS | UCCE_HDLC_TX_EVENTS)) {
		if (napi_schedule_prep(&priv->napi)) {
			uccm &= ~((UCCE_HDLC_RX_EVENTS | UCCE_HDLC_TX_EVENTS)
				  << 16);
			iowrite32be(uccm, uccf->p_uccm);
			__napi_schedule(&priv->napi);
		}
	}

	/* Errors and other events */
	if (ucce >> 16 & UCC_HDLC_UCCE_BSY)
		dev->stats.rx_errors++;
	if (ucce >> 16 & UCC_HDLC_UCCE_TXE)
		dev->stats.tx_errors++;

	return IRQ_HANDLED;
}

static int uhdlc_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	const size_t size = sizeof(te1_settings);
	te1_settings line;
	struct ucc_hdlc_private *priv = netdev_priv(dev);

	if (cmd != SIOCWANDEV)
		return hdlc_ioctl(dev, ifr, cmd);

	switch (ifr->ifr_settings.type) {
	case IF_GET_IFACE:
		ifr->ifr_settings.type = IF_IFACE_E1;
		if (ifr->ifr_settings.size < size) {
			ifr->ifr_settings.size = size; /* data size wanted */
			return -ENOBUFS;
		}
		line.clock_type = priv->clocking;
		line.clock_rate = 0;
		line.loopback = 0;

		if (copy_to_user(ifr->ifr_settings.ifs_ifsu.sync, &line, size))
			return -EFAULT;
		return 0;

	default:
		return hdlc_ioctl(dev, ifr, cmd);
	}
}

static int uhdlc_open(struct net_device *dev)
{
	u32 cecr_subblock;
	hdlc_device *hdlc = dev_to_hdlc(dev);
	struct ucc_hdlc_private *priv = hdlc->priv;
	struct ucc_tdm *utdm = priv->utdm;

	if (priv->hdlc_busy != 1) {
		if (request_irq(priv->ut_info->uf_info.irq,
				ucc_hdlc_irq_handler, 0,
				"hdlc", (void *)priv)) {
			dev_err(priv->dev, "request_irq for ucc hdlc failed\n");
			return -ENODEV;
		}
		cecr_subblock = ucc_fast_get_qe_cr_subblock(
					priv->ut_info->uf_info.ucc_num);

		qe_issue_cmd(QE_INIT_TX_RX, cecr_subblock,
			     (u8)QE_CR_PROTOCOL_UNSPECIFIED, 0);

		ucc_fast_enable(priv->uccf, COMM_DIR_RX | COMM_DIR_TX);

		/* Enable the TDM port */
		if (priv->tsa)
			utdm->si_regs->siglmr1_h |= (0x1 << utdm->tdm_port);

		priv->hdlc_busy = 1;
		netif_device_attach(priv->ndev);
		napi_enable(&priv->napi);
		netif_start_queue(dev);
		hdlc_open(dev);
	} else {
		dev_err(priv->dev, "HDLC IS RUNNING!\n");
	}

#ifdef DEBUG
	dump_priv(priv);
	dump_ucc(priv);
	dump_bds(priv);
#endif
	return 0;
}

static void uhdlc_memclean(struct ucc_hdlc_private *priv)
{
	qe_muram_free(priv->ucc_pram->riptr);
	qe_muram_free(priv->ucc_pram->tiptr);

	if (priv->rx_bd_base) {
		dma_free_coherent(priv->dev,
				  RX_BD_RING_LEN * sizeof(struct qe_bd),
				  priv->rx_bd_base, priv->dma_rx_bd);

		priv->rx_bd_base = NULL;
		priv->dma_rx_bd = 0;
	}

	if (priv->tx_bd_base) {
		dma_free_coherent(priv->dev,
				  TX_BD_RING_LEN * sizeof(struct qe_bd),
				  priv->tx_bd_base, priv->dma_tx_bd);

		priv->tx_bd_base = NULL;
		priv->dma_tx_bd = 0;
	}

	if (priv->ucc_pram) {
		qe_muram_free(priv->ucc_pram_offset);
		priv->ucc_pram = NULL;
		priv->ucc_pram_offset = 0;
	 }

	kfree(priv->rx_skbuff);
	priv->rx_skbuff = NULL;

	kfree(priv->tx_skbuff);
	priv->tx_skbuff = NULL;

	if (priv->uf_regs) {
		iounmap(priv->uf_regs);
		priv->uf_regs = NULL;
	}

	if (priv->uccf) {
		ucc_fast_free(priv->uccf);
		priv->uccf = NULL;
	}

	if (priv->rx_buffer) {
		dma_free_coherent(priv->dev,
				  RX_BD_RING_LEN * MAX_RX_BUF_LENGTH,
				  priv->rx_buffer, priv->dma_rx_addr);
		priv->rx_buffer = NULL;
		priv->dma_rx_addr = 0;
	}

	if (priv->tx_buffer) {
		dma_free_coherent(priv->dev,
				  TX_BD_RING_LEN * MAX_RX_BUF_LENGTH,
				  priv->tx_buffer, priv->dma_tx_addr);
		priv->tx_buffer = NULL;
		priv->dma_tx_addr = 0;
	}
}

static int uhdlc_close(struct net_device *dev)
{
	struct ucc_hdlc_private *priv = dev_to_hdlc(dev)->priv;
	struct ucc_tdm *utdm = priv->utdm;
	u32 cecr_subblock;

	napi_disable(&priv->napi);
	cecr_subblock = ucc_fast_get_qe_cr_subblock(
				priv->ut_info->uf_info.ucc_num);

	qe_issue_cmd(QE_GRACEFUL_STOP_TX, cecr_subblock,
		     (u8)QE_CR_PROTOCOL_UNSPECIFIED, 0);
	qe_issue_cmd(QE_CLOSE_RX_BD, cecr_subblock,
		     (u8)QE_CR_PROTOCOL_UNSPECIFIED, 0);

	if (priv->tsa)
		utdm->si_regs->siglmr1_h &= ~(0x1 << utdm->tdm_port);

	ucc_fast_disable(priv->uccf, COMM_DIR_RX | COMM_DIR_TX);

	free_irq(priv->ut_info->uf_info.irq, priv);
	netif_stop_queue(dev);
	priv->hdlc_busy = 0;

	return 0;
}

static int ucc_hdlc_attach(struct net_device *dev, unsigned short encoding,
			   unsigned short parity)
{
	struct ucc_hdlc_private *priv = dev_to_hdlc(dev)->priv;

	if (encoding != ENCODING_NRZ &&
	    encoding != ENCODING_NRZI)
		return -EINVAL;

	if (parity != PARITY_NONE &&
	    parity != PARITY_CRC32_PR1_CCITT &&
	    parity != PARITY_CRC16_PR1_CCITT)
		return -EINVAL;

	priv->encoding = encoding;
	priv->parity = parity;

	return 0;
}

#ifdef CONFIG_PM
static void store_clk_config(struct ucc_hdlc_private *priv)
{
	struct qe_mux *qe_mux_reg = &qe_immr->qmx;

	/* store si clk */
	priv->cmxsi1cr_h = ioread32be(&qe_mux_reg->cmxsi1cr_h);
	priv->cmxsi1cr_l = ioread32be(&qe_mux_reg->cmxsi1cr_l);

	/* store si sync */
	priv->cmxsi1syr = ioread32be(&qe_mux_reg->cmxsi1syr);

	/* store ucc clk */
	memcpy_fromio(priv->cmxucr, qe_mux_reg->cmxucr, 4 * sizeof(u32));
}

static void resume_clk_config(struct ucc_hdlc_private *priv)
{
	struct qe_mux *qe_mux_reg = &qe_immr->qmx;

	memcpy_toio(qe_mux_reg->cmxucr, priv->cmxucr, 4 * sizeof(u32));

	iowrite32be(priv->cmxsi1cr_h, &qe_mux_reg->cmxsi1cr_h);
	iowrite32be(priv->cmxsi1cr_l, &qe_mux_reg->cmxsi1cr_l);

	iowrite32be(priv->cmxsi1syr, &qe_mux_reg->cmxsi1syr);
}

static int uhdlc_suspend(struct device *dev)
{
	struct ucc_hdlc_private *priv = dev_get_drvdata(dev);
	struct ucc_tdm_info *ut_info;
	struct ucc_fast __iomem *uf_regs;

	if (!priv)
		return -EINVAL;

	if (!netif_running(priv->ndev))
		return 0;

	netif_device_detach(priv->ndev);
	napi_disable(&priv->napi);

	ut_info = priv->ut_info;
	uf_regs = priv->uf_regs;

	/* backup gumr guemr*/
	priv->gumr = ioread32be(&uf_regs->gumr);
	priv->guemr = ioread8(&uf_regs->guemr);

	priv->ucc_pram_bak = kmalloc(sizeof(*priv->ucc_pram_bak),
					GFP_KERNEL);
	if (!priv->ucc_pram_bak)
		return -ENOMEM;

	/* backup HDLC parameter */
	memcpy_fromio(priv->ucc_pram_bak, priv->ucc_pram,
		      sizeof(struct ucc_hdlc_param));

	/* store the clk configuration */
	store_clk_config(priv);

	/* save power */
	ucc_fast_disable(priv->uccf, COMM_DIR_RX | COMM_DIR_TX);

	dev_dbg(dev, "ucc hdlc suspend\n");
	return 0;
}

static int uhdlc_resume(struct device *dev)
{
	struct ucc_hdlc_private *priv = dev_get_drvdata(dev);
	struct ucc_tdm *utdm = priv->utdm;
	struct ucc_tdm_info *ut_info;
	struct ucc_fast __iomem *uf_regs;
	struct ucc_fast_private *uccf;
	struct ucc_fast_info *uf_info;
	int ret, i;
	u32 cecr_subblock, bd_status;

	if (!priv)
		return -EINVAL;

	if (!netif_running(priv->ndev))
		return 0;

	ut_info = priv->ut_info;
	uf_info = &ut_info->uf_info;
	uf_regs = priv->uf_regs;
	uccf = priv->uccf;

	/* restore gumr guemr */
	iowrite8(priv->guemr, &uf_regs->guemr);
	iowrite32be(priv->gumr, &uf_regs->gumr);

	/* Set Virtual Fifo registers */
	iowrite16be(uf_info->urfs, &uf_regs->urfs);
	iowrite16be(uf_info->urfet, &uf_regs->urfet);
	iowrite16be(uf_info->urfset, &uf_regs->urfset);
	iowrite16be(uf_info->utfs, &uf_regs->utfs);
	iowrite16be(uf_info->utfet, &uf_regs->utfet);
	iowrite16be(uf_info->utftt, &uf_regs->utftt);
	/* utfb, urfb are offsets from MURAM base */
	iowrite32be(uccf->ucc_fast_tx_virtual_fifo_base_offset, &uf_regs->utfb);
	iowrite32be(uccf->ucc_fast_rx_virtual_fifo_base_offset, &uf_regs->urfb);

	/* Rx Tx and sync clock routing */
	resume_clk_config(priv);

	iowrite32be(uf_info->uccm_mask, &uf_regs->uccm);
	iowrite32be(0xffffffff, &uf_regs->ucce);

	ucc_fast_disable(priv->uccf, COMM_DIR_RX | COMM_DIR_TX);

	/* rebuild SIRAM */
	if (priv->tsa)
		init_si(priv->utdm, priv->ut_info);

	/* Write to QE CECR, UCCx channel to Stop Transmission */
	cecr_subblock = ucc_fast_get_qe_cr_subblock(uf_info->ucc_num);
	ret = qe_issue_cmd(QE_STOP_TX, cecr_subblock,
			   (u8)QE_CR_PROTOCOL_UNSPECIFIED, 0);

	/* Set UPSMR normal mode */
	iowrite32be(0, &uf_regs->upsmr);

	/* init parameter base */
	cecr_subblock = ucc_fast_get_qe_cr_subblock(uf_info->ucc_num);
	ret = qe_issue_cmd(QE_ASSIGN_PAGE_TO_DEVICE, cecr_subblock,
			   QE_CR_PROTOCOL_UNSPECIFIED, priv->ucc_pram_offset);

	priv->ucc_pram = (struct ucc_hdlc_param __iomem *)
				qe_muram_addr(priv->ucc_pram_offset);

	/* restore ucc parameter */
	memcpy_toio(priv->ucc_pram, priv->ucc_pram_bak,
		    sizeof(struct ucc_hdlc_param));
	kfree(priv->ucc_pram_bak);

	/* rebuild BD entry */
	for (i = 0; i < RX_BD_RING_LEN; i++) {
		if (i < (RX_BD_RING_LEN - 1))
			bd_status = R_E | R_I;
		else
			bd_status = R_E | R_I | R_W;

		iowrite32be(bd_status, (u32 *)(priv->rx_bd_base + i));
		iowrite32be(priv->dma_rx_addr + i * MAX_RX_BUF_LENGTH,
			    &priv->rx_bd_base[i].buf);
	}

	for (i = 0; i < TX_BD_RING_LEN; i++) {
		if (i < (TX_BD_RING_LEN - 1))
			bd_status =  T_I | T_TC;
		else
			bd_status =  T_I | T_TC | T_W;

		iowrite32be(bd_status, (u32 *)(priv->tx_bd_base + i));
		iowrite32be(priv->dma_tx_addr + i * MAX_RX_BUF_LENGTH,
			    &priv->tx_bd_base[i].buf);
	}

	/* if hdlc is busy enable TX and RX */
	if (priv->hdlc_busy == 1) {
		cecr_subblock = ucc_fast_get_qe_cr_subblock(
					priv->ut_info->uf_info.ucc_num);

		qe_issue_cmd(QE_INIT_TX_RX, cecr_subblock,
			     (u8)QE_CR_PROTOCOL_UNSPECIFIED, 0);

		ucc_fast_enable(priv->uccf, COMM_DIR_RX | COMM_DIR_TX);

		/* Enable the TDM port */
		if (priv->tsa)
			utdm->si_regs->siglmr1_h |= (0x1 << utdm->tdm_port);
	}

	napi_enable(&priv->napi);
	netif_device_attach(priv->ndev);

	return 0;
}

static const struct dev_pm_ops uhdlc_pm_ops = {
	.suspend = uhdlc_suspend,
	.resume = uhdlc_resume,
	.freeze = uhdlc_suspend,
	.thaw = uhdlc_resume,
};

#define HDLC_PM_OPS (&uhdlc_pm_ops)

#else

#define HDLC_PM_OPS NULL

#endif
static const struct net_device_ops uhdlc_ops = {
	.ndo_open       = uhdlc_open,
	.ndo_stop       = uhdlc_close,
	.ndo_change_mtu = hdlc_change_mtu,
	.ndo_start_xmit = hdlc_start_xmit,
	.ndo_do_ioctl   = uhdlc_ioctl,
};

static int ucc_hdlc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ucc_hdlc_private *uhdlc_priv = NULL;
	struct ucc_tdm_info *ut_info;
	struct ucc_tdm *utdm;
	struct resource res;
	struct net_device *dev;
	hdlc_device *hdlc;
	int ucc_num;
	const char *sprop;
	int ret;
	u32 val;

	ret = of_property_read_u32_index(np, "cell-index", 0, &val);
	if (ret) {
		dev_err(&pdev->dev, "Invalid ucc property\n");
		return -ENODEV;
	}

	ucc_num = val - 1;
	if ((ucc_num > 3) || (ucc_num < 0)) {
		dev_err(&pdev->dev, ": Invalid UCC num\n");
		return -EINVAL;
	}

	memcpy(&utdm_info[ucc_num], &utdm_primary_info,
	       sizeof(utdm_primary_info));

	ut_info = &utdm_info[ucc_num];
	ut_info->uf_info.ucc_num = ucc_num;

	sprop = of_get_property(np, "rx-clock-name", NULL);
	if (sprop) {
		ut_info->uf_info.rx_clock = qe_clock_source(sprop);
		if ((ut_info->uf_info.rx_clock < QE_CLK_NONE) ||
		    (ut_info->uf_info.rx_clock > QE_CLK24)) {
			dev_err(&pdev->dev, "Invalid rx-clock-name property\n");
			return -EINVAL;
		}
	} else {
		dev_err(&pdev->dev, "Invalid rx-clock-name property\n");
		return -EINVAL;
	}

	sprop = of_get_property(np, "tx-clock-name", NULL);
	if (sprop) {
		ut_info->uf_info.tx_clock = qe_clock_source(sprop);
		if ((ut_info->uf_info.tx_clock < QE_CLK_NONE) ||
		    (ut_info->uf_info.tx_clock > QE_CLK24)) {
			dev_err(&pdev->dev, "Invalid tx-clock-name property\n");
			return -EINVAL;
		}
	} else {
		dev_err(&pdev->dev, "Invalid tx-clock-name property\n");
		return -EINVAL;
	}

	/* use the same clock when work in loopback */
	if (ut_info->uf_info.rx_clock == ut_info->uf_info.tx_clock)
		qe_setbrg(ut_info->uf_info.rx_clock, 20000000, 1);

	ret = of_address_to_resource(np, 0, &res);
	if (ret)
		return -EINVAL;

	ut_info->uf_info.regs = res.start;
	ut_info->uf_info.irq = irq_of_parse_and_map(np, 0);

	uhdlc_priv = kzalloc(sizeof(*uhdlc_priv), GFP_KERNEL);
	if (!uhdlc_priv) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "No mem to alloc hdlc private data\n");
		goto err_alloc_priv;
	}

	dev_set_drvdata(&pdev->dev, uhdlc_priv);
	uhdlc_priv->dev = &pdev->dev;
	uhdlc_priv->ut_info = ut_info;

	if (of_get_property(np, "fsl,tdm-interface", NULL))
		uhdlc_priv->tsa = 1;

	if (of_get_property(np, "fsl,inter-loopback", NULL))
		uhdlc_priv->loopback = 1;

	if (uhdlc_priv->tsa == 1) {
		utdm = kzalloc(sizeof(*utdm), GFP_KERNEL);
		if (!utdm) {
			ret = -ENOMEM;
			dev_err(&pdev->dev, "No mem to alloc ucc tdm data\n");
			goto err_alloc_utdm;
		}
		uhdlc_priv->utdm = utdm;
		ret = of_parse_tdm(np, utdm, ut_info);
		if (ret)
			goto err_miss_tsa_property;
	}

	ret = uhdlc_init(uhdlc_priv);
	if (ret) {
		dev_err(&pdev->dev, "Failed to init uhdlc\n");
		goto err_hdlc_init;
	}

	dev = alloc_hdlcdev(uhdlc_priv);
	if (!dev) {
		ret = -ENOMEM;
		pr_err("ucc_hdlc: unable to allocate memory\n");
		goto err_hdlc_init;
	}

	uhdlc_priv->ndev = dev;
	hdlc = dev_to_hdlc(dev);
	dev->tx_queue_len = 16;
	dev->netdev_ops = &uhdlc_ops;
	hdlc->attach = ucc_hdlc_attach;
	hdlc->xmit = ucc_hdlc_tx;
	netif_napi_add(dev, &uhdlc_priv->napi, ucc_hdlc_poll, 32);
	if (register_hdlc_device(dev)) {
		ret = -ENOBUFS;
		pr_err("ucc_hdlc: unable to register hdlc device\n");
		free_netdev(dev);
		goto err_hdlc_init;
	}

#ifdef DEBUG
	dump_priv(uhdlc_priv);
	dump_ucc(uhdlc_priv);
	dump_bds(uhdlc_priv);
	if (uhdlc_priv->tsa)
		mem_disp((u8 *)uhdlc_priv->utdm->si_regs, 0x20);
#endif

	return 0;

err_hdlc_init:
err_miss_tsa_property:
	kfree(uhdlc_priv);
	if (uhdlc_priv->tsa)
		kfree(utdm);
err_alloc_utdm:
	kfree(uhdlc_priv);
err_alloc_priv:
	return ret;
}

static int ucc_hdlc_remove(struct platform_device *pdev)
{
	struct ucc_hdlc_private *priv = dev_get_drvdata(&pdev->dev);

	uhdlc_memclean(priv);

	if (priv->utdm->si_regs) {
		iounmap(priv->utdm->si_regs);
		priv->utdm->si_regs = NULL;
	}

	if (priv->utdm->siram) {
		iounmap(priv->utdm->siram);
		priv->utdm->siram = NULL;
	}
	kfree(priv);

	dev_info(&pdev->dev, "UCC based hdlc module removed\n");

	return 0;
}

static const struct of_device_id fsl_ucc_hdlc_of_match[] = {
	{
	.compatible = "fsl,ucc_hdlc",
	},
	{},
};

MODULE_DEVICE_TABLE(of, fsl_ucc_hdlc_of_match);

static struct platform_driver ucc_hdlc_driver = {
	.probe	= ucc_hdlc_probe,
	.remove	= ucc_hdlc_remove,
	.driver	= {
		.owner		= THIS_MODULE,
		.name		= DRV_NAME,
		.pm		= HDLC_PM_OPS,
		.of_match_table	= fsl_ucc_hdlc_of_match,
	},
};

static int __init ucc_hdlc_init(void)
{
	return platform_driver_register(&ucc_hdlc_driver);
}

static void __exit ucc_hdlc_exit(void)
{
	platform_driver_unregister(&ucc_hdlc_driver);
}

module_init(ucc_hdlc_init);
module_exit(ucc_hdlc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Freescale Semiconductor Inc.");
MODULE_DESCRIPTION("Driver For Freescale QE UCC HDLC controller");
MODULE_VERSION("1.0");
