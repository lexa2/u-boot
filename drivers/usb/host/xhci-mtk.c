// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019 MediaTek, Inc.
 * Authors: Chunfeng Yun <chunfeng.yun@mediatek.com>
 */

#include <clk.h>
#include <common.h>
#include <dm.h>
#include <dm/devres.h>
#include <generic-phy.h>
#include <malloc.h>
#include <usb.h>
#include <linux/errno.h>
#include <linux/compat.h>
#include <power/regulator.h>
#include <linux/iopoll.h>

#include <usb/xhci.h>

#define MU3C_U3_PORT_MAX 4
#define MU3C_U2_PORT_MAX 5

/**
 * struct mtk_ippc_regs: MTK ssusb ip port control registers
 * @ip_pw_ctr0~3: ip power and clock control registers
 * @ip_pw_sts1~2: ip power and clock status registers
 * @ip_xhci_cap: ip xHCI capability register
 * @u3_ctrl_p[x]: ip usb3 port x control register, only low 4bytes are used
 * @u2_ctrl_p[x]: ip usb2 port x control register, only low 4bytes are used
 * @u2_phy_pll: usb2 phy pll control register
 */
struct mtk_ippc_regs {
	__le32 ip_pw_ctr0;
	__le32 ip_pw_ctr1;
	__le32 ip_pw_ctr2;
	__le32 ip_pw_ctr3;
	__le32 ip_pw_sts1;
	__le32 ip_pw_sts2;
	__le32 reserved0[3];
	__le32 ip_xhci_cap;
	__le32 reserved1[2];
	__le64 u3_ctrl_p[MU3C_U3_PORT_MAX];
	__le64 u2_ctrl_p[MU3C_U2_PORT_MAX];
	__le32 reserved2;
	__le32 u2_phy_pll;
	__le32 reserved3[33]; /* 0x80 ~ 0xff */
};

/* ip_pw_ctrl0 register */
#define CTRL0_IP_SW_RST	BIT(0)

/* ip_pw_ctrl1 register */
#define CTRL1_IP_HOST_PDN	BIT(0)

/* ip_pw_ctrl2 register */
#define CTRL2_IP_DEV_PDN	BIT(0)

/* ip_pw_sts1 register */
#define STS1_IP_SLEEP_STS	BIT(30)
#define STS1_U3_MAC_RST	BIT(16)
#define STS1_XHCI_RST		BIT(11)
#define STS1_SYS125_RST	BIT(10)
#define STS1_REF_RST		BIT(8)
#define STS1_SYSPLL_STABLE	BIT(0)

/* ip_xhci_cap register */
#define CAP_U3_PORT_NUM(p)	((p) & 0xff)
#define CAP_U2_PORT_NUM(p)	(((p) >> 8) & 0xff)

/* u3_ctrl_p register */
#define CTRL_U3_PORT_HOST_SEL	BIT(2)
#define CTRL_U3_PORT_PDN	BIT(1)
#define CTRL_U3_PORT_DIS	BIT(0)

/* u2_ctrl_p register */
#define CTRL_U2_PORT_HOST_SEL	BIT(2)
#define CTRL_U2_PORT_PDN	BIT(1)
#define CTRL_U2_PORT_DIS	BIT(0)

struct mtk_xhci {
	struct xhci_ctrl ctrl;	/* Needs to come first in this struct! */
	struct xhci_hccr *hcd;
	struct mtk_ippc_regs *ippc;
	struct udevice *dev;
	struct udevice *vusb33_supply;
	struct udevice *vbus_supply;
	struct clk *sys_clk;
	struct clk *xhci_clk;
	struct clk *ref_clk;
	struct clk *mcu_clk;
	struct clk *dma_clk;
	int num_u2_ports;
	int num_u3_ports;
	struct phy *phys;
	int num_phys;
};

static int xhci_mtk_host_enable(struct mtk_xhci *mtk)
{
	struct mtk_ippc_regs *ippc = mtk->ippc;
	u32 value, check_val;
	int ret;
	int i;

	/* power on host ip */
	value = readl(&ippc->ip_pw_ctr1);
	value &= ~CTRL1_IP_HOST_PDN;
	writel(value, &ippc->ip_pw_ctr1);

	/* power on and enable all u3 ports */
	for (i = 0; i < mtk->num_u3_ports; i++) {
		value = readl(&ippc->u3_ctrl_p[i]);
		value &= ~(CTRL_U3_PORT_PDN | CTRL_U3_PORT_DIS);
		value |= CTRL_U3_PORT_HOST_SEL;
		writel(value, &ippc->u3_ctrl_p[i]);
	}

	/* power on and enable all u2 ports */
	for (i = 0; i < mtk->num_u2_ports; i++) {
		value = readl(&ippc->u2_ctrl_p[i]);
		value &= ~(CTRL_U2_PORT_PDN | CTRL_U2_PORT_DIS);
		value |= CTRL_U2_PORT_HOST_SEL;
		writel(value, &ippc->u2_ctrl_p[i]);
	}

	/*
	 * wait for clocks to be stable, and clock domains reset to
	 * be inactive after power on and enable ports
	 */
	check_val = STS1_SYSPLL_STABLE | STS1_REF_RST |
			STS1_SYS125_RST | STS1_XHCI_RST;

	if (mtk->num_u3_ports)
		check_val |= STS1_U3_MAC_RST;

	ret = readl_poll_timeout(&ippc->ip_pw_sts1, value,
				 (check_val == (value & check_val)), 20000);
	if (ret) {
		dev_err(mtk->dev, "clocks are not stable (0x%x)\n", value);
		return ret;
	}

	return 0;
}

static int xhci_mtk_host_disable(struct mtk_xhci *mtk)
{
	struct mtk_ippc_regs *ippc = mtk->ippc;
	u32 value;
	int i;

	/* power down all u3 ports */
	for (i = 0; i < mtk->num_u3_ports; i++) {
		value = readl(&ippc->u3_ctrl_p[i]);
		value |= CTRL_U3_PORT_PDN;
		writel(value, &ippc->u3_ctrl_p[i]);
	}

	/* power down all u2 ports */
	for (i = 0; i < mtk->num_u2_ports; i++) {
		value = readl(&ippc->u2_ctrl_p[i]);
		value |= CTRL_U2_PORT_PDN;
		writel(value, &ippc->u2_ctrl_p[i]);
	}

	/* power down host ip */
	value = readl(&ippc->ip_pw_ctr1);
	value |= CTRL1_IP_HOST_PDN;
	writel(value, &ippc->ip_pw_ctr1);

	return 0;
}

static int xhci_mtk_ssusb_init(struct mtk_xhci *mtk)
{
	struct mtk_ippc_regs *ippc = mtk->ippc;
	u32 value;

	/* reset whole ip */
	value = readl(&ippc->ip_pw_ctr0);
	value |= CTRL0_IP_SW_RST;
	writel(value, &ippc->ip_pw_ctr0);
	udelay(1);
	value = readl(&ippc->ip_pw_ctr0);
	value &= ~CTRL0_IP_SW_RST;
	writel(value, &ippc->ip_pw_ctr0);

	value = readl(&ippc->ip_xhci_cap);
	mtk->num_u3_ports = CAP_U3_PORT_NUM(value);
	mtk->num_u2_ports = CAP_U2_PORT_NUM(value);
	dev_info(mtk->dev, "%s u2p:%d, u3p:%d\n", __func__,
		 mtk->num_u2_ports, mtk->num_u3_ports);

	return xhci_mtk_host_enable(mtk);
}

static int xhci_mtk_clks_enable(struct mtk_xhci *mtk)
{
	int ret;

	ret = clk_enable(mtk->sys_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable sys_clk\n");
		goto sys_clk_err;
	}

	ret = clk_enable(mtk->ref_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable ref_clk\n");
		goto ref_clk_err;
	}

	ret = clk_enable(mtk->xhci_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable xhci_clk\n");
		goto xhci_clk_err;
	}

	ret = clk_enable(mtk->mcu_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable mcu_clk\n");
		goto mcu_clk_err;
	}

	ret = clk_enable(mtk->dma_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable dma_clk\n");
		goto dma_clk_err;
	}

	return 0;

dma_clk_err:
	clk_disable(mtk->mcu_clk);
mcu_clk_err:
	clk_disable(mtk->xhci_clk);
xhci_clk_err:
	clk_disable(mtk->sys_clk);
sys_clk_err:
	clk_disable(mtk->ref_clk);
ref_clk_err:
	return ret;
}

static void xhci_mtk_clks_disable(struct mtk_xhci *mtk)
{
	clk_disable(mtk->dma_clk);
	clk_disable(mtk->mcu_clk);
	clk_disable(mtk->xhci_clk);
	clk_disable(mtk->ref_clk);
	clk_disable(mtk->sys_clk);
}

static int xhci_mtk_ofdata_get(struct mtk_xhci *mtk)
{
	struct udevice *dev = mtk->dev;
	fdt_addr_t reg_base;
	int ret = 0;

	reg_base = devfdt_get_addr_name(dev, "mac");
	if (reg_base == FDT_ADDR_T_NONE) {
		pr_err("Failed to get xHCI base address\n");
		return -ENXIO;
	}

	mtk->hcd = (struct xhci_hccr *)reg_base;

	reg_base = devfdt_get_addr_name(dev, "ippc");
	if (reg_base == FDT_ADDR_T_NONE) {
		pr_err("Failed to get IPPC base address\n");
		return -ENXIO;
	}

	mtk->ippc = (struct mtk_ippc_regs *)reg_base;

	dev_info(dev, "hcd: 0x%x, ippc: 0x%x\n",
		 (int)mtk->hcd, (int)mtk->ippc);

	mtk->sys_clk = devm_clk_get(dev, "sys_ck");
	if (IS_ERR(mtk->sys_clk)) {
		dev_err(dev, "Failed to get sys clock\n");
		goto clk_err;
	}

	mtk->ref_clk = devm_clk_get_optional(dev, "ref_ck");
	if (IS_ERR(mtk->ref_clk)) {
		dev_err(dev, "Failed to get ref clock\n");
		goto clk_err;
	}

	mtk->xhci_clk = devm_clk_get_optional(dev, "xhci_ck");
	if (IS_ERR(mtk->xhci_clk)) {
		dev_err(dev, "Failed to get xhci clock\n");
		goto clk_err;
	}

	mtk->mcu_clk = devm_clk_get_optional(dev, "mcu_ck");
	if (IS_ERR(mtk->mcu_clk)) {
		dev_err(dev, "Failed to get mcu clock\n");
		goto clk_err;
	}

	mtk->dma_clk = devm_clk_get_optional(dev, "dma_ck");
	if (IS_ERR(mtk->dma_clk)) {
		dev_err(dev, "Failed to get dma clock\n");
		goto clk_err;
	}

	ret = device_get_supply_regulator(dev, "vusb33-supply",
					  &mtk->vusb33_supply);
	if (ret)
		debug("Can't get vusb33 regulator! %d\n", ret);

	ret = device_get_supply_regulator(dev, "vbus-supply",
					  &mtk->vbus_supply);
	if (ret)
		debug("Can't get vbus regulator! %d\n", ret);

	return 0;

clk_err:
	return ret;
}

static int xhci_mtk_ldos_enable(struct mtk_xhci *mtk)
{
	int ret;

	ret = regulator_set_enable(mtk->vusb33_supply, true);
	if (ret < 0 && ret != -ENOSYS) {
		dev_err(mtk->dev, "failed to enable vusb33\n");
		return ret;
	}

	ret = regulator_set_enable(mtk->vbus_supply, true);
	if (ret < 0 && ret != -ENOSYS) {
		dev_err(mtk->dev, "failed to enable vbus\n");
		regulator_set_enable(mtk->vusb33_supply, false);
		return ret;
	}

	return 0;
}

static void xhci_mtk_ldos_disable(struct mtk_xhci *mtk)
{
	regulator_set_enable(mtk->vbus_supply, false);
	regulator_set_enable(mtk->vusb33_supply, false);
}

int xhci_mtk_phy_setup(struct mtk_xhci *mtk)
{
	struct udevice *dev = mtk->dev;
	struct phy *usb_phys;
	int i, ret, count;

	/* Return if no phy declared */
	if (!dev_read_prop(dev, "phys", NULL))
		return 0;

	count = dev_count_phandle_with_args(dev, "phys", "#phy-cells");
	if (count <= 0)
		return count;

	usb_phys = devm_kcalloc(dev, count, sizeof(*usb_phys),
				GFP_KERNEL);
	if (!usb_phys)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		ret = generic_phy_get_by_index(dev, i, &usb_phys[i]);
		if (ret && ret != -ENOENT) {
			dev_err(dev, "Failed to get USB PHY%d for %s\n",
				i, dev->name);
			return ret;
		}
	}

	for (i = 0; i < count; i++) {
		ret = generic_phy_init(&usb_phys[i]);
		if (ret) {
			dev_err(dev, "Can't init USB PHY%d for %s\n",
				i, dev->name);
			goto phys_init_err;
		}
	}

	for (i = 0; i < count; i++) {
		ret = generic_phy_power_on(&usb_phys[i]);
		if (ret) {
			dev_err(dev, "Can't power USB PHY%d for %s\n",
				i, dev->name);
			goto phys_poweron_err;
		}
	}

	mtk->phys = usb_phys;
	mtk->num_phys =  count;

	return 0;

phys_poweron_err:
	for (i = count - 1; i >= 0; i--)
		generic_phy_power_off(&usb_phys[i]);

	for (i = 0; i < count; i++)
		generic_phy_exit(&usb_phys[i]);

	return ret;

phys_init_err:
	for (; i >= 0; i--)
		generic_phy_exit(&usb_phys[i]);

	return ret;
}

void xhci_mtk_phy_shutdown(struct mtk_xhci *mtk)
{
	struct udevice *dev = mtk->dev;
	struct phy *usb_phys;
	int i, ret;

	usb_phys = mtk->phys;
	for (i = 0; i < mtk->num_phys; i++) {
		if (!generic_phy_valid(&usb_phys[i]))
			continue;

		ret = generic_phy_power_off(&usb_phys[i]);
		ret |= generic_phy_exit(&usb_phys[i]);
		if (ret) {
			dev_err(dev, "Can't shutdown USB PHY%d for %s\n",
				i, dev->name);
		}
	}
}

static int xhci_mtk_probe(struct udevice *dev)
{
	struct mtk_xhci *mtk = dev_get_priv(dev);
	struct xhci_hcor *hcor;
	int ret;

	mtk->dev = dev;
	ret = xhci_mtk_ofdata_get(mtk);
	if (ret)
		return ret;

	ret = xhci_mtk_ldos_enable(mtk);
	if (ret)
		goto ldos_err;

	ret = xhci_mtk_clks_enable(mtk);
	if (ret)
		goto clks_err;

	ret = xhci_mtk_phy_setup(mtk);
	if (ret)
		goto phys_err;

	ret = xhci_mtk_ssusb_init(mtk);
	if (ret)
		goto ssusb_init_err;

	hcor = (struct xhci_hcor *)((uintptr_t)mtk->hcd +
			HC_LENGTH(xhci_readl(&mtk->hcd->cr_capbase)));

	return xhci_register(dev, mtk->hcd, hcor);

ssusb_init_err:
	xhci_mtk_phy_shutdown(mtk);
phys_err:
	xhci_mtk_clks_disable(mtk);
clks_err:
	xhci_mtk_ldos_disable(mtk);
ldos_err:
	return ret;
}

static int xhci_mtk_remove(struct udevice *dev)
{
	struct mtk_xhci *mtk = dev_get_priv(dev);

	xhci_deregister(dev);
	xhci_mtk_host_disable(mtk);
	xhci_mtk_ldos_disable(mtk);
	xhci_mtk_clks_disable(mtk);

	return 0;
}

static const struct udevice_id xhci_mtk_ids[] = {
	{ .compatible = "mediatek,mtk-xhci" },
	{ }
};

U_BOOT_DRIVER(usb_xhci) = {
	.name	= "xhci-mtk",
	.id	= UCLASS_USB,
	.of_match = xhci_mtk_ids,
	.probe = xhci_mtk_probe,
	.remove = xhci_mtk_remove,
	.ops	= &xhci_usb_ops,
	.bind	= dm_scan_fdt_dev,
	.priv_auto_alloc_size = sizeof(struct mtk_xhci),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};
