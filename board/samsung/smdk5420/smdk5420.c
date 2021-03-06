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
#include <fdtdec.h>
#include <asm/io.h>
#include <errno.h>
#include <i2c.h>
#include <lcd.h>
#include <netdev.h>
#include <spi.h>
#include <asm/arch/board.h>
#include <asm/arch/cpu.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/sromc.h>
#include <asm/arch/dp_info.h>
#include <asm/arch/power.h>
#include <asm/arch/setup.h>
#include <asm/arch/system.h>
#include <power/pmic.h>
#include <power/max77802_pmic.h>
#include <power/s2mps11_pmic.h>
#include <power/tps65090_pmic.h>

DECLARE_GLOBAL_DATA_PTR;

const struct exynos_apll_pms exynos_pms = {
	/* APLL @1.8GHz */
	.apll_pdiv = 0x3,
	.apll_mdiv = 0xe1,
	.apll_sdiv = 0x0,
};

#ifdef CONFIG_LCD
#ifdef CONFIG_CHROMEOS_PEACH
static int has_edp_bridge(void)
{
	int node;

	node = fdtdec_next_compatible(gd->fdt_blob, 0, COMPAT_PARADE_PS8625);

	/* No node for bridge in device tree. */
	if (node <= 0)
		return 0;

	/* Default is with bridge ic */
	return 1;
}

static void exynos_lcd_power_on_bridge_rev1(void)
{
	struct pmic *p;
	p = pmic_get_by_id(COMPAT_MAXIM_MAX77802_PMIC);
	if (!p) {
		debug("%s: Failed to get PMIC data\n", __func__);
		return;
	}

	/* LDO35 set 1.2V */
	if (pmic_reg_update(p, MAX77802_REG_PMIC_LDO35CTRL1,
			    MAX77802_LDO35CTRL1_1_2V)) {
		debug("%s: PMIC %d register write failed\n", __func__,
						MAX77802_LDO35CTRL1_1_2V);
		return;
	}

	/* TODO: Take care of the bridge initialization here */
	mdelay(15);	/* TODO: Use state machine to remove delay */

	/* TODO(sjg@chromium.org): Use device tree */
	gpio_direction_output(EXYNOS5420_GPIO_Y77, 1);	/* EDP_RST# */
	gpio_direction_output(EXYNOS5420_GPIO_X35, 1);	/* EDP_SLP# */
	gpio_direction_input(EXYNOS5420_GPIO_X26);	/* EDP_HPD */
	gpio_set_pull(EXYNOS5420_GPIO_X26, S5P_GPIO_PULL_NONE);

	/*
	 * TODO(sjg@chromium.org): printf() for now until this function can
	 * actually return a value.
	 */
	if (parade_init(gd->fdt_blob))
		printf("%s: ps8625_init() failed\n", __func__);

	tps65090_fet_enable(6);
}

static void exynos_lcd_power_on_bridge_rev5(void)
{
	tps65090_fet_enable(6);

	mdelay(5);

	/* TODO(sjg@chromium.org): Use device tree */
	gpio_direction_output(EXYNOS5420_GPIO_X35, 1);	/* EDP_SLP# */
	mdelay(10);
	gpio_direction_output(EXYNOS5420_GPIO_Y77, 1);	/* EDP_RST# */
	gpio_direction_input(EXYNOS5420_GPIO_X26);	/* EDP_HPD */
	gpio_set_pull(EXYNOS5420_GPIO_X26, S5P_GPIO_PULL_NONE);

	/*
	 * TODO(sjg@chromium.org): printf() for now until this function can
	 * actually return a value.
	 */
	if (parade_init(gd->fdt_blob))
		printf("%s: ps8625_init() failed\n", __func__);
}

static void exynos_lcd_power_on_nobridge(void)
{
	tps65090_fet_enable(6);		/* P3.3V_LCD */

	gpio_direction_input(EXYNOS5420_GPIO_X26);	/* EDP_HPD */
	gpio_set_pull(EXYNOS5420_GPIO_X26, S5P_GPIO_PULL_NONE);
}

void exynos_lcd_power_on(void)
{
	if (!has_edp_bridge()) {
		exynos_lcd_power_on_nobridge();
	} else {
		if (board_get_revision() < 5)
			exynos_lcd_power_on_bridge_rev1();
		else
			exynos_lcd_power_on_bridge_rev5();
	}
}

void exynos_backlight_on(unsigned int onoff)
{
	/* For PWM */
	gpio_cfg_pin(EXYNOS5420_GPIO_B20, S5P_GPIO_OUTPUT);
	gpio_set_value(EXYNOS5420_GPIO_B20, onoff);

	tps65090_fet_enable(1);

	if (!has_edp_bridge()) {
		/* LCD1_BL_EN = XEINT_18 */
		gpio_cfg_pin(EXYNOS5420_GPIO_X22, S5P_GPIO_OUTPUT);
		gpio_set_value(EXYNOS5420_GPIO_X22, onoff);
	}
}
#else
void exynos_lcd_power_on(void)
{
	/* LCD_EN */
	gpio_cfg_pin(EXYNOS5420_GPIO_H07, S5P_GPIO_OUTPUT);
	gpio_set_value(EXYNOS5420_GPIO_H07, 1);
}

void exynos_backlight_on(unsigned int onoff)
{
	/* For PWM */
	gpio_cfg_pin(EXYNOS5420_GPIO_B20, S5P_GPIO_OUTPUT);
	gpio_set_value(EXYNOS5420_GPIO_B20, onoff);

	/* BL_EN */
	gpio_cfg_pin(EXYNOS5420_GPIO_X15, S5P_GPIO_OUTPUT);
	gpio_set_value(EXYNOS5420_GPIO_X15, 1);
}
#endif

void exynos_cfg_lcd_gpio(void)
{
	/* Set Hotplug detect for DP */
	gpio_cfg_pin(EXYNOS5420_GPIO_X07, S5P_GPIO_FUNC(0x3));
}

void init_panel_info(vidinfo_t *vid)
{
}
#endif

void  set_max_cpu_freq(void)
{
	struct exynos5420_clock *clk =
		(struct exynos5420_clock *)EXYNOS5420_CLOCK_BASE;
	u32 val;

	/* switch A15 clock source to OSC clock before changing APLL */
	clrbits_le32(&clk->clk_src_cpu, APLL_FOUT);

	/* PLL locktime */
	writel(exynos_pms.apll_pdiv * PLL_LOCK_FACTOR, &clk->apll_lock);

	/* Set APLL */
	writel(APLL_CON1_VAL, &clk->apll_con1);
	val = set_pll(exynos_pms.apll_mdiv, exynos_pms.apll_pdiv,
		      exynos_pms.apll_sdiv);
	writel(val, &clk->apll_con0);
	while ((readl(&clk->apll_con0) & PLL_LOCKED) == 0)
		;
	/* now it is safe to switch to APLL */
	setbits_le32(&clk->clk_src_cpu, APLL_FOUT);
}

#ifdef CONFIG_RUN_TIME_BANK_NUMBER
/*
 * This is supported on peach only and presently hardcoded. A more elaborate
 * way of determining the amount of installed memory could be devised later
 * when/if required.
 */
int board_get_num_dram_banks(void)
{
	int subrev;

	board_get_full_revision(NULL, &subrev);

	/* Bit 1 of the subrev is the SDRAM size.  TODO: cleanup */
	if (subrev & (1 << 1))
		return 7;  /* 7 banks of .5 GB, 3.5GB total. */

	/* Default is set to 2 GB. */
	return 4;
}
#endif
