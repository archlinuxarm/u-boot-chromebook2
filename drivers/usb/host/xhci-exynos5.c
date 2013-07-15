/*
 * SAMSUNG EXYNOS5 USB HOST XHCI Controller
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 *	Vivek Gautam <gautam.vivek@samsung.com>
 *	Vikas Sajjan <vikas.sajjan@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

/*
 * This file is a conglomeration for DWC3-init sequence and further
 * exynos5 specific PHY-init sequence.
 */

#include <common.h>
#include <fdtdec.h>
#include <libfdt.h>
#include <malloc.h>
#include <usb.h>
#include <watchdog.h>
#include <asm/arch/cpu.h>
#include <asm/arch/power.h>
#include <asm/arch/xhci-exynos.h>
#include <asm/gpio.h>
#include <asm-generic/errno.h>
#include <linux/compat.h>
#include <linux/usb/dwc3.h>

#include "xhci.h"

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

/**
 * Contains pointers to register base addresses
 * for the usb controller.
 */
struct exynos_xhci {
	struct exynos_usb3_phy *usb3_phy;
	struct xhci_hccr *hcd;
	struct dwc3 *dwc3_reg;
};

static struct exynos_xhci reg_bases[CONFIG_USB_MAX_CONTROLLER_COUNT];

#ifdef CONFIG_OF_CONTROL
static int exynos_usb3_parse_dt(const void *blob,
				struct exynos_xhci *base,
				int index)
{
	fdt_addr_t addr;
	int depth, count;
	unsigned int node = 0;
	int nodes[CONFIG_USB_MAX_CONTROLLER_COUNT];
	struct fdt_gpio_state vbus_gpio;

	count = fdtdec_find_aliases_for_id(blob, "xhci",
			COMPAT_SAMSUNG_EXYNOS5_XHCI, nodes,
			CONFIG_USB_MAX_CONTROLLER_COUNT);
	if (count < 0) {
		printf("XHCI: Can't get device node for xhci\n");
		return -ENODEV;
	}

	node = nodes[index];
	if (node <= 0) {
		printf("XHCI: Can't get device node for xhci\n");
		return -ENODEV;
	}

	/* Turn the USB VBUS GPIO on, if it exists */
	if (!fdtdec_decode_gpio(blob, node, "samsung,vbus-gpio", &vbus_gpio)) {
		fdtdec_setup_gpio(&vbus_gpio);
		gpio_direction_output(vbus_gpio.gpio, 1);
	}

	/*
	 * Get the base address for XHCI controller from the device node
	 */
	addr = fdtdec_get_addr(blob, node, "reg");
	if (addr == FDT_ADDR_T_NONE) {
		printf("Can't get the XHCI register base address\n");
		return -ENXIO;
	}
	base->hcd = (struct xhci_hccr *)addr;

	depth = 0;
	node = fdtdec_next_compatible_subnode(blob, node,
				COMPAT_SAMSUNG_EXYNOS5_USB3_PHY, &depth);
	if (node <= 0) {
		printf("XHCI: Can't get device node for usb3-phy controller\n");
		return -ENODEV;
	}

	/*
	 * Get the base address for usbphy from the device node
	 */
	base->usb3_phy = (struct exynos_usb3_phy *)fdtdec_get_addr(blob, node,
								"reg");
	if (base->usb3_phy == NULL) {
		printf("Can't get the usbphy register address\n");
		return -ENXIO;
	}

	return 0;
}
#endif

#ifdef CONFIG_EXYNOS5420
static void crport_handshake(struct exynos_usb3_phy *phy, u32 val, u32 cmd)
{
	u32 usec = 100;
	u32 result;

	writel(val | cmd, &phy->phy_reg0);

	do {
		result = readl(&phy->phy_reg1);
		if (result & PHYREG1_CR_ACK)
			break;

		udelay(1);
	} while (usec-- > 0);

	if (!usec)
		printf("CRPORT handshake timeout1 (0x%08x)\n", val);

	usec = 100;

	writel(val, &phy->phy_reg0);

	do {
		result = readl(&phy->phy_reg1);
		if (!(result & PHYREG1_CR_ACK))
			break;

		udelay(1);
	} while (usec-- > 0);

	if (!usec)
		printf("CRPORT handshake timeout2 (0x%08x)\n", val);
}

static void crport_ctrl_write(struct exynos_usb3_phy *phy, u32 addr, u32 data)
{
	/* Write Address */
	crport_handshake(phy, PHYREG0_CR_DATA_IN(addr),
				PHYREG0_CR_CAP_ADDR);

	/* Write Data */
	crport_handshake(phy, PHYREG0_CR_DATA_IN(data),
				PHYREG0_CR_CAP_DATA);
	crport_handshake(phy, PHYREG0_CR_DATA_IN(data),
				PHYREG0_CR_WRITE);
}

static void exynos5_usb3_phy_tune(struct exynos_usb3_phy *phy)
{
	u32 reg;

	/*
	 * Change los_bias to (0x5) for 28nm PHY from a
	 * default value (0x0);
	 * los_level is set as default (0x9) as also reflected in
	 * los_level[30:26] bits of PHYPARAM0 register
	 */
	reg = LOSLEVEL_OVRD_IN_LOS_BIAS_5420 |
			LOSLEVEL_OVRD_IN_EN |
			LOSLEVEL_OVRD_IN_LOS_LEVEL_DEFAULT;
	crport_ctrl_write(phy, PHYSS_LOSLEVEL_OVRD_IN, reg);

	/*
	 * Set tx_vboost_lvl to (0x5) for 28nm PHY Tuning,
	 * to raise Tx signal level from its default value of (0x4)
	 */
	reg = TX_VBOOSTLEVEL_OVRD_IN_VBOOST_5420;
	crport_ctrl_write(phy, PHYSS_TX_VBOOSTLEVEL_OVRD_IN, reg);
}

void xhci_hcd_tune(int index)
{
	struct exynos_xhci *ctx = &reg_bases[index];
	struct exynos_usb3_phy *phy = ctx->usb3_phy;

	/* Tune USB3.0 PHY here by controlling CR_PORT register here */
	exynos5_usb3_phy_tune(phy);
}
#endif

static void exynos5_usb3_phy_init(struct exynos_usb3_phy *phy)
{
	u32 reg;

	/* Reset USB 3.0 PHY */
	writel(0x0, &phy->phy_reg0);

	clrbits_le32(&phy->phy_param0,
			/* Select PHY CLK source */
			PHYPARAM0_REF_USE_PAD |
			/* Set Loss-of-Signal Detector sensitivity */
			PHYPARAM0_REF_LOSLEVEL_MASK);
	setbits_le32(&phy->phy_param0, PHYPARAM0_REF_LOSLEVEL);

	writel(0x0, &phy->phy_resume);

	/*
	 * Setting the Frame length Adj value[6:1] to default 0x20
	 * See xHCI 1.0 spec, 5.2.4
	 */
	setbits_le32(&phy->link_system,
			LINKSYSTEM_XHCI_VERSION_CONTROL |
			LINKSYSTEM_FLADJ(0x20));

	/* Set Tx De-Emphasis level */
	clrbits_le32(&phy->phy_param1, PHYPARAM1_PCS_TXDEEMPH_MASK);
	setbits_le32(&phy->phy_param1, PHYPARAM1_PCS_TXDEEMPH);

	setbits_le32(&phy->phy_batchg, PHYBATCHG_UTMI_CLKSEL);

	/* PHYTEST POWERDOWN Control */
	clrbits_le32(&phy->phy_test,
			PHYTEST_POWERDOWN_SSP |
			PHYTEST_POWERDOWN_HSP);

	/* UTMI Power Control */
	writel(PHYUTMI_OTGDISABLE, &phy->phy_utmi);

		/* Use core clock from main PLL */
	reg = PHYCLKRST_REFCLKSEL_EXT_REFCLK |
		/* Default 24Mhz crystal clock */
		PHYCLKRST_FSEL(FSEL_CLKSEL_24M) |
		PHYCLKRST_MPLL_MULTIPLIER_24MHZ_REF |
		PHYCLKRST_SSC_REFCLKSEL(0x88) |
		/* Force PortReset of PHY */
		PHYCLKRST_PORTRESET |
		/* Digital power supply in normal operating mode */
		PHYCLKRST_RETENABLEN |
		/* Enable ref clock for SS function */
		PHYCLKRST_REF_SSP_EN |
		/* Enable spread spectrum */
		PHYCLKRST_SSC_EN |
		/* Power down HS Bias and PLL blocks in suspend mode */
		PHYCLKRST_COMMONONN;

	writel(reg, &phy->phy_clk_rst);

	/* giving time to Phy clock to settle before resetting */
	udelay(10);

	reg &= ~PHYCLKRST_PORTRESET;
	writel(reg, &phy->phy_clk_rst);
}

static void exynos5_usb3_phy_exit(struct exynos_usb3_phy *phy)
{
	setbits_le32(&phy->phy_utmi,
			PHYUTMI_OTGDISABLE |
			PHYUTMI_FORCESUSPEND |
			PHYUTMI_FORCESLEEP);

	clrbits_le32(&phy->phy_clk_rst,
			PHYCLKRST_REF_SSP_EN |
			PHYCLKRST_SSC_EN |
			PHYCLKRST_COMMONONN);

	/* PHYTEST POWERDOWN Control to remove leakage current */
	setbits_le32(&phy->phy_test,
			PHYTEST_POWERDOWN_SSP |
			PHYTEST_POWERDOWN_HSP);
}

void dwc3_set_mode(struct dwc3 *dwc3_reg, u32 mode)
{
	clrsetbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG),
			DWC3_GCTL_PRTCAPDIR(mode));
}

static void dwc3_core_soft_reset(struct dwc3 *dwc3_reg)
{
	/* Before Resetting PHY, put Core in Reset */
	setbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_CORESOFTRESET);

	/* Assert USB3 PHY reset */
	setbits_le32(&dwc3_reg->g_usb3pipectl[0],
			DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Assert USB2 PHY reset */
	setbits_le32(&dwc3_reg->g_usb2phycfg,
			DWC3_GUSB2PHYCFG_PHYSOFTRST);

	mdelay(100);

	/* Clear USB3 PHY reset */
	clrbits_le32(&dwc3_reg->g_usb3pipectl[0],
			DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Clear USB2 PHY reset */
	clrbits_le32(&dwc3_reg->g_usb2phycfg,
			DWC3_GUSB2PHYCFG_PHYSOFTRST);

	/* After PHYs are stable we can take Core out of reset state */
	clrbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_CORESOFTRESET);
}

static int dwc3_core_init(struct dwc3 *dwc3_reg)
{
	u32 reg;
	u32 revision;
	unsigned int dwc3_hwparams1;

	revision = readl(&dwc3_reg->g_snpsid);
	/* This should read as U3 followed by revision number */
	if ((revision & DWC3_GSNPSID_MASK) != 0x55330000) {
		printf("this is not a DesignWare USB3 DRD Core\n");
		return -1;
	}

	dwc3_core_soft_reset(dwc3_reg);

	dwc3_hwparams1 = readl(&dwc3_reg->g_hwparams1);

	reg = readl(&dwc3_reg->g_ctl);
	reg &= ~DWC3_GCTL_SCALEDOWN_MASK;
	reg &= ~DWC3_GCTL_DISSCRAMBLE;
	switch (DWC3_GHWPARAMS1_EN_PWROPT(dwc3_hwparams1)) {
	case DWC3_GHWPARAMS1_EN_PWROPT_CLK:
		reg &= ~DWC3_GCTL_DSBLCLKGTNG;
		break;
	default:
		debug("No power optimization available\n");
	}

	/*
	 * WORKAROUND: DWC3 revisions <1.90a have a bug
	 * where the device can fail to connect at SuperSpeed
	 * and falls back to high-speed mode which causes
	 * the device to enter a Connect/Disconnect loop
	 */
	if ((revision & DWC3_REVISION_MASK) < 0x190a)
		reg |= DWC3_GCTL_U2RSTECN;

	writel(reg, &dwc3_reg->g_ctl);

	return 0;
}

static int exynos_xhci_core_init(struct exynos_xhci *base)
{
	int ret;

	exynos5_usb3_phy_init(base->usb3_phy);

	ret = dwc3_core_init(base->dwc3_reg);
	if (ret) {
		debug("failed to initialize core\n");
		return -1;
	}

	/* We are hard-coding DWC3 core to Host Mode */
	dwc3_set_mode(base->dwc3_reg, DWC3_GCTL_PRTCAP_HOST);

	return 0;
}

static void exynos_xhci_core_exit(struct exynos_xhci *base)
{
	exynos5_usb3_phy_exit(base->usb3_phy);
}

int xhci_hcd_init(int index, struct xhci_hccr **hccr, struct xhci_hcor **hcor)
{
	struct exynos_xhci *ctx = &reg_bases[index];

#ifdef CONFIG_OF_CONTROL
	exynos_usb3_parse_dt(gd->fdt_blob, ctx, index);
#else
	/*
	 * Right now we only have H/W with 2 controllers, so limiting the
	 * index to two here: either 0 or 1.
	 */
	if (index == 0) {
		ctx->usb3_phy = (struct exynos_usb3_phy *)
					samsung_get_base_usb3_phy();
		ctx->hcd = (struct xhci_hccr *)
					samsung_get_base_usb_xhci();
	} else if (index == 1) {
		ctx->usb3_phy = (struct exynos_usb3_phy *)
					samsung_get_base_usb3_phy_1();
		ctx->hcd = (struct xhci_hccr *)
					samsung_get_base_usb_xhci_1();
	}
#endif

	if (!ctx->hcd || !ctx->usb3_phy) {
		printf("XHCI: Unable to find Host controller\n");
		return -ENODEV;
	}

	ctx->dwc3_reg = (struct dwc3 *)((char *)(ctx->hcd) + DWC3_REG_OFFSET);

	/* Power-on usb_drd phy */
	set_usbdrd_phy_ctrl(POWER_USB_DRD_PHY_CTRL_EN, index);

	if (exynos_xhci_core_init(ctx)) {
		printf("XHCI: Can't init Host controller\n");
		return -EINVAL;
	}

	*hccr = (ctx->hcd);
	*hcor = (struct xhci_hcor *)((uint32_t) *hccr
				+ HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	debug("Exynos5-xhci: init hccr %x and hcor %x hc_length %d\n",
		(uint32_t)*hccr, (uint32_t)*hcor,
		(uint32_t)HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	return 0;
}

void xhci_hcd_stop(int index)
{
	struct exynos_xhci *ctx = &reg_bases[index];

	exynos_xhci_core_exit(ctx);

	/* Power-off usb_drd phy */
	set_usbdrd_phy_ctrl(POWER_USB_DRD_PHY_CTRL_DISABLE, index);
}
