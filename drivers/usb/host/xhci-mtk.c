// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2015 - 2019 MediaTek Inc.
 * Author: Frank Wunderlich <frank-w@public-files.de>
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <mapmem.h>
#include <asm/io.h>

#include <malloc.h>
#include <usb.h>
#include <watchdog.h>
#include <linux/errno.h>
#include <linux/compat.h>
#include <linux/usb/dwc3.h>
#include <power/regulator.h>

#include <usb/xhci.h>

struct mtk_xhci {
	/*struct usb_platdata usb_plat;
	struct xhci_ctrl ctrl;
	struct xhci_hccr *hcd;
	struct dwc3 *dwc3_reg;*/
};

static int mtk_xhci_probe(struct udevice *dev)
{
	printf("%s %d",__FUNCTION__,__LINE__);
/*
	struct rockchip_xhci_platdata *plat = dev_get_platdata(dev);
	struct rockchip_xhci *ctx = dev_get_priv(dev);
	struct xhci_hcor *hcor;
	int ret;

	ctx->hcd = (struct xhci_hccr *)plat->hcd_base;
	ctx->dwc3_reg = (struct dwc3 *)((char *)(ctx->hcd) + DWC3_REG_OFFSET);
	hcor = (struct xhci_hcor *)((uint64_t)ctx->hcd +
			HC_LENGTH(xhci_readl(&ctx->hcd->cr_capbase)));

	if (plat->vbus_supply) {
		ret = regulator_set_enable(plat->vbus_supply, true);
		if (ret) {
			pr_err("XHCI: failed to set VBus supply\n");
			return ret;
		}
	}

	ret = rockchip_xhci_core_init(ctx, dev);
	if (ret) {
		pr_err("XHCI: failed to initialize controller\n");
		return ret;
	}

	return xhci_register(dev, ctx->hcd, hcor);
*/
	return -ENODEV;
}

static const struct udevice_id mtk_xhci_id_table[] = {
	{ .compatible = "mediatek,mt7623-xhci", },
	{ }
};

U_BOOT_DRIVER(mtk_xhci) = {
	.name		= "mtk-xhci",
	.id		= UCLASS_USB,
	.of_match	= mtk_xhci_id_table,
	//.ops		= &xhci_usb_ops,
	.probe		= mtk_xhci_probe,
	.priv_auto_alloc_size = sizeof(struct mtk_xhci),
};
