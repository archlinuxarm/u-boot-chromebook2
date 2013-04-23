/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <cros_ec.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <errno.h>
#include <i2c.h>
#include <lcd.h>
#include <netdev.h>
#include <spi.h>
#include <asm/arch/cpu.h>
#include <asm/arch/dwmmc.h>
#include <asm/arch/board.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/power.h>
#include <asm/arch/sromc.h>
#include <power/pmic.h>
#include <power/max77686_pmic.h>
#include <power/tps65090_pmic.h>
#include <tmu.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_USB_EHCI_EXYNOS
int board_usb_vbus_init(void)
{
	/* Enable VBUS power switch */
	gpio_direction_output(EXYNOS5_GPIO_X26, 1);

	/* VBUS turn ON time */
	mdelay(3);

	return 0;
}
#endif

#ifdef CONFIG_SOUND_MAX98095
int board_enable_audio_codec(void)
{
	/* Enable MAX98095 Codec */
	gpio_direction_output(EXYNOS5_GPIO_X17, 1);
	gpio_set_pull(EXYNOS5_GPIO_X17, S5P_GPIO_PULL_NONE);

	return 0;
}
#endif

#ifdef CONFIG_POWER
int power_init_board(void)
{
	int ret;

	set_ps_hold_ctrl();

	/*
	 * For now, this is not device-tree-controlled.
	 * one and only one is supposed to succeed
	 */
#ifdef CONFIG_EXYNOS5250
	ret = board_init_max77686();
#else
	ret = !((board_init_s2mps11() ? 1 : 0) ^
		(board_init_max77802() ? 1 : 0));
#endif
	if (ret)
		return ret;

	return board_init_tps65090();
}
#endif

int exynos_init(void)
{
#ifdef CONFIG_USB_EHCI_EXYNOS
	board_usb_vbus_init();
#endif
	return 0;
}

static int decode_sromc(const void *blob, struct fdt_sromc *config)
{
	int err;
	int node;

	node = fdtdec_next_compatible(blob, 0, COMPAT_SAMSUNG_EXYNOS5_SROMC);
	if (node < 0) {
		debug("Could not find SROMC node\n");
		return node;
	}

	config->bank = fdtdec_get_int(blob, node, "bank", 0);
	config->width = fdtdec_get_int(blob, node, "width", 2);

	err = fdtdec_get_int_array(blob, node, "srom-timing", config->timing,
			FDT_SROM_TIMING_COUNT);
	if (err < 0) {
		debug("Could not decode SROMC configuration Error: %s\n",
		      fdt_strerror(err));
		return -FDT_ERR_NOTFOUND;
	}
	return 0;
}

int board_eth_init(bd_t *bis)
{
#ifdef CONFIG_SMC911X
	u32 smc_bw_conf, smc_bc_conf;
	struct fdt_sromc config;
	fdt_addr_t base_addr;
	int node;

	node = decode_sromc(gd->fdt_blob, &config);
	if (node < 0) {
		debug("%s: Could not find sromc configuration\n", __func__);
		return 0;
	}
	node = fdtdec_next_compatible(gd->fdt_blob, node, COMPAT_SMSC_LAN9215);
	if (node < 0) {
		debug("%s: Could not find lan9215 configuration\n", __func__);
		return 0;
	}

	/* We now have a node, so any problems from now on are errors */
	base_addr = fdtdec_get_addr(gd->fdt_blob, node, "reg");
	if (base_addr == FDT_ADDR_T_NONE) {
		debug("%s: Could not find lan9215 address\n", __func__);
		return -1;
	}

	/* Ethernet needs data bus width of 16 bits */
	if (config.width != 2) {
		debug("%s: Unsupported bus width %d\n", __func__,
		      config.width);
		return -1;
	}
	smc_bw_conf = SROMC_DATA16_WIDTH(config.bank)
			| SROMC_BYTE_ENABLE(config.bank);

	smc_bc_conf = SROMC_BC_TACS(config.timing[FDT_SROM_TACS])   |
			SROMC_BC_TCOS(config.timing[FDT_SROM_TCOS]) |
			SROMC_BC_TACC(config.timing[FDT_SROM_TACC]) |
			SROMC_BC_TCOH(config.timing[FDT_SROM_TCOH]) |
			SROMC_BC_TAH(config.timing[FDT_SROM_TAH])   |
			SROMC_BC_TACP(config.timing[FDT_SROM_TACP]) |
			SROMC_BC_PMC(config.timing[FDT_SROM_PMC]);

	/* Select and configure the SROMC bank */
	exynos_pinmux_config(PERIPH_ID_SROMC, config.bank);
	s5p_config_sromc(config.bank, smc_bw_conf, smc_bc_conf);
	return smc911x_initialize(0, base_addr);
#endif
	return 0;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	const char *board_name;

	board_name = fdt_getprop(gd->fdt_blob, 0, "model", NULL);
	if (board_name == NULL) {
		printf("\nUnknown Board\n");
	} else {
		printf("\nBoard: %s, rev %d\n", board_name,
		       board_get_revision());
	}

	return 0;
}
#endif

#ifdef CONFIG_GENERIC_MMC
int board_mmc_init(bd_t *bis)
{
	int ret;
	/* dwmmc initializattion for available channels */
	ret = exynos_dwmmc_init(gd->fdt_blob);
	if (ret)
		debug("dwmmc init failed\n");

	return ret;
}
#endif

#if defined(CONFIG_LCD) && defined(CONFIG_EXYNOS5250)
static int board_dp_bridge_setup(void)
{
	const int MAX_TRIES = 10;
	int num_tries;

	debug("%s\n", __func__);

	/* Mux HPHPD to the special hotplug detect mode */
	exynos_pinmux_config(PERIPH_ID_DPHPD, 0);

	/* Setup the GPIOs */

	/* PD is ACTIVE_LOW, and initially de-asserted */
	gpio_set_pull(EXYNOS5_GPIO_Y25, S5P_GPIO_PULL_NONE);
	gpio_direction_output(EXYNOS5_GPIO_Y25, 1);

	/* Reset is ACTIVE_LOW */
	gpio_set_pull(EXYNOS5_GPIO_X15, S5P_GPIO_PULL_NONE);
	gpio_direction_output(EXYNOS5_GPIO_X15, 0);

	udelay(10);
	gpio_set_value(EXYNOS5_GPIO_X15, 1);

	gpio_direction_input(EXYNOS5_GPIO_X07);

	/*
	 * We need to wait for 90ms after bringing up the bridge since there
	 * is a phantom "high" on the HPD chip during its bootup.  The phantom
	 * high comes within 7ms of de-asserting PD and persists for at least
	 * 15ms.  The real high comes roughly 50ms after PD is de-asserted. The
	 * phantom high makes it hard for us to know when the NXP chip is up.
	 */
	mdelay(90);

	for (num_tries = 0; num_tries < MAX_TRIES; num_tries++) {
		/* Check HPD.  If it's high, we're all good. */
		if (gpio_get_value(EXYNOS5_GPIO_X07))
				return 0;

		debug("%s: eDP bridge failed to come up; try %d of %d\n",
				__func__, num_tries, MAX_TRIES);
	}

	/* Immediately go into bridge reset if the hp line is not high */
	return -ENODEV;
}

void exynos_set_dp_phy(unsigned int onoff)
{
	debug("%s(%u)\n", __func__, onoff);

	set_dp_phy_ctrl(onoff);
}

/* BEGIN exynos_fb lcd_panel_on stages */

void exynos_cfg_lcd_gpio(void)
{
	debug("%s\n", __func__);
}

void exynos_backlight_on(unsigned int onoff)
{
	debug("%s(%u)\n", __func__, onoff);

	if (onoff) {
#ifdef CONFIG_POWER_TPS65090
		int ret;

		ret = tps65090_fet_enable(1); /* Enable FET1, backlight */
		if (ret)
			return;

		/* T5 in the LCD timing spec (defined as > 10ms) */
		mdelay(10);

		/* board_dp_backlight_pwm */
		gpio_direction_output(EXYNOS5_GPIO_B20, 1);

		/* T6 in the LCD timing spec (defined as > 10ms) */
		mdelay(10);

		/* board_dp_backlight_en */
		gpio_direction_output(EXYNOS5_GPIO_X30, 1);
#endif
	}
}

void exynos_reset_lcd(void)
{
	debug("%s\n", __func__);
}

void exynos_lcd_power_on(void)
{
	debug("%s\n", __func__);

	/* board_dp_lcd_vdd */
	tps65090_fet_enable(6); /* Enable FET6, lcd panel */

	board_dp_bridge_setup();
}

void exynos_cfg_ldo(void)
{
	debug("%s\n", __func__);

}
void exynos_enable_ldo(unsigned int onoff)
{
	debug("%s(%u)\n", __func__, onoff);
}

void exynos_backlight_reset(void)
{
	debug("%s\n", __func__);
}

/* END exynos_fb lcd_panel_on stages */

void init_panel_info(vidinfo_t *vid)
{
	debug("%s\n", __func__);
}
#endif
