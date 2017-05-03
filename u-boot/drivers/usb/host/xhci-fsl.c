/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * FSL USB HOST xHCI Controller
 *
 * Author: Ramneek Mehresh<ramneek.mehresh@freescale.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <usb.h>
#include <asm-generic/errno.h>
#include <linux/compat.h>
#include <linux/usb/xhci-fsl.h>
#include <linux/usb/dwc3.h>
#include "xhci.h"

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

static struct fsl_xhci fsl_xhci;
unsigned long ctr_addr[] = FSL_USB_XHCI_ADDR;

__weak int __board_usb_init(int index, enum usb_init_type init)
{
	return 0;
}

static void fsl_xhci_set_beat_burst_length(struct dwc3 *dwc3_reg)
{
	int val = readl(&dwc3_reg->g_sbuscfg0);
	val &= ~USB3_ENABLE_BEAT_BURST_MASK;
	writel(val | USB3_ENABLE_BEAT_BURST, &dwc3_reg->g_sbuscfg0);
	val = readl(&dwc3_reg->g_sbuscfg1);
	writel(val | USB3_SET_BEAT_BURST_LIMIT, &dwc3_reg->g_sbuscfg1);
}

static int fsl_xhci_core_init(struct fsl_xhci *fsl_xhci)
{
	int ret = 0;

	ret = dwc3_core_init(fsl_xhci->dwc3_reg);
	if (ret) {
		debug("%s:failed to initialize core\n", __func__);
		return ret;
	}

	/* We are hard-coding DWC3 core to Host Mode */
	dwc3_set_mode(fsl_xhci->dwc3_reg, DWC3_GCTL_PRTCAP_HOST);

	/* Set GFLADJ_30MHZ as 20h as per XHCI spec default value */
	dwc3_set_fladj(fsl_xhci->dwc3_reg, GFLADJ_30MHZ_DEFAULT);

	/*
	 * A-010151: USB controller to configure USB in P2 mode
	 * whenever the Receive Detect feature is required
	 */
	dwc3_set_rxdetect_power_mode(fsl_xhci->dwc3_reg,
				     DWC3_GUSB3PIPECTL_DISRXDETP3);

	/* Change beat burst and outstanding pipelined transfers requests */
	fsl_xhci_set_beat_burst_length(fsl_xhci->dwc3_reg);

	return ret;
}

static int fsl_xhci_core_exit(struct fsl_xhci *fsl_xhci)
{
	/*
	 * Currently fsl socs do not support PHY shutdown from
	 * sw. But this support may be added in future socs.
	 */
	return 0;
}

int xhci_hcd_init(int index, struct xhci_hccr **hccr, struct xhci_hcor **hcor)
{
	struct fsl_xhci *ctx = &fsl_xhci;
	int ret = 0;

	ctx->hcd = (struct xhci_hccr *)ctr_addr[index];
	ctx->dwc3_reg = (struct dwc3 *)((char *)(ctx->hcd) + DWC3_REG_OFFSET);

	ret = board_usb_init(index, USB_INIT_HOST);
	if (ret != 0) {
		puts("Failed to initialize board for USB\n");
		return ret;
	}

	ret = fsl_xhci_core_init(ctx);
	if (ret < 0) {
		puts("Failed to initialize xhci\n");
		return ret;
	}

	*hccr = (struct xhci_hccr *)ctx->hcd;
	*hcor = (struct xhci_hcor *)((uintptr_t) *hccr
				+ HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	debug("fsl-xhci: init hccr %lx and hcor %lx hc_length %lx\n",
	      (uintptr_t)*hccr, (uintptr_t)*hcor,
	      (uintptr_t)HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	return ret;
}

void xhci_hcd_stop(int index)
{
	struct fsl_xhci *ctx = &fsl_xhci;

	fsl_xhci_core_exit(ctx);
}
