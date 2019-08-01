// SPDX-License-Identifier: GPL-2.0
/*
 * Mediatek PHY driver
 *
 * Copyright (C) 2019 <>
 */

#include <common.h>
#include <clk.h>
#include <div64.h>
#include <dm.h>
#include <fdtdec.h>
#include <generic-phy.h>
#include <reset.h>
#include <syscon.h>
#include <usb.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <power/regulator.h>

struct mtk_tphy {
    fdt_addr_t  regs;
    struct clk  clk;
};


static const struct phy_ops mtk_phy_ops = {
};

static int mtk_phy_probe(struct udevice *dev)
{
	return 0;
}

static int mtk_phy_remove(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id mtk_phy_of_match[] = {
    { .compatible = "mediatek,generic-tphy-v1", },
    { },
};

U_BOOT_DRIVER(mtk_tphy) = {
	.name       = "mtk-tphy",
	.id     = UCLASS_PHY,
	.of_match   = mtk_phy_of_match,
	.ops        = &mtk_phy_ops,
	.probe      = mtk_phy_probe,
	.remove     = mtk_phy_remove,
	.priv_auto_alloc_size = sizeof(struct mtk_tphy),
};
