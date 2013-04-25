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
#include <asm/arch/gpio.h>
#include <asm/arch/mmc.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/sromc.h>
#include <asm/arch/dp_info.h>
#include <asm/arch/power.h>
#include <asm/arch/system.h>
#include <power/pmic.h>
#include <power/s2mps11_pmic.h>
#include <power/tps65090_pmic.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_LCD
/* TODO: Move these LCD initialization settings out of board file */
#ifdef CONFIG_CHROMEOS_PEACH
int anx1120_initialize(unsigned int i2c_bus)
{
	static const struct {
		u8 addr;
		u8 reg;
		u8 value;
	} init_anx1120[] = {
		{0x29, 0x07, 0x00}, {0x29, 0x1f, 0x1d},
		{0x47, 0xb6, 0x18}, {0x29, 0xa8, 0x0a},
		{0x29, 0xd1, 0x0e}, {0x29, 0x19, 0x33},
		{0x29, 0xf8, 0x44}, {0x47, 0xc4, 0x04},
		{0x47, 0xc3, 0x09}, {0x47, 0xc2, 0xc8},
		{0x47, 0xc0, 0x09}, {0x47, 0xc1, 0x71},
		{0x47, 0xb1, 0x00}, {0x47, 0xbf, 0xff},
		{0x47, 0xb5, 0x63}, {0x47, 0xb3, 0x29},
		{0x47, 0xb2, 0xa8}, {0x29, 0x9f, 0x00},
		{0x29, 0xa5, 0x00}, {0x29, 0xde, 0x09},
		{0x29, 0xe7, 0x09}, {0x29, 0xa4, 0x99},
		{0x29, 0xa5, 0x99}, {0x47, 0xbe, 0x01},
		{0x29, 0xf3, 0x00}, {0x29, 0xf4, 0x3c},
		{0x47, 0x20, 0x81}, {0x47, 0x22, 0x01},
		{0x29, 0xa1, 0x00}, {0x29, 0xa2, 0x00},
		{0x29, 0xa3, 0x00}, {0x47, 0x91, 0x00},
		{0x29, 0xd5, 0x00}, {0x29, 0x1f, 0x03},
		{0x29, 0x07, 0xff},
	};
	int i;

	i2c_set_bus_num(i2c_bus);

	if (i2c_probe(0x29) || i2c_probe(0x47)) {
		debug("Can't find ANX R0\n");
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(init_anx1120); i++) {
		u8 value = init_anx1120[i].value;

		i2c_write(init_anx1120[i].addr, init_anx1120[i].reg, 1,
			  &value, 1);
	}

	return 0;
}

void exynos_lcd_power_on(void)
{
	struct pmic *p;

	p = pmic_get("S2MPS11_PMIC");
	if (!p) {
		debug("Failed to get PMIC data\n");
		return;	/* TODO: return error */
	}

	if (pmic_probe(p))
		return;

	i2c_set_bus_num(S2MPS11_BUS_NUM);

	/* PVDD_LDO12 set to 1V */
	if (pmic_reg_write(p, S2MPS11_LDO22_CTRL, S2MPS11_BUCK_CTRL2_1_2V)) {
		debug("%s: PMIC %d register write failed\n", __func__,
							S2MPS11_LDO22_CTRL);
		debug("P1.2V_LDO_OUT22 not initialized\n");
		return;
	}
	mdelay(15);	/* TODO: Use state machine to remove delay */

	/* eDP-LVDS ANX chip RESET_L */
	gpio_cfg_pin(EXYNOS5420_GPIO_X15, S5P_GPIO_OUTPUT);
	gpio_set_value(EXYNOS5420_GPIO_X15, 1);

	/* Initialize the eDP-LVDS ANX chip */
	if (anx1120_initialize(ANX1120_I2C_BUS)) {
		debug("ANX1120 failed to init\n");
		return;
	}

	tps65090_fet_enable(6);
}

void exynos_backlight_on(unsigned int onoff)
{
	/* For PWM */
	gpio_cfg_pin(EXYNOS5420_GPIO_B20, S5P_GPIO_OUTPUT);
	gpio_set_value(EXYNOS5420_GPIO_B20, onoff);

	tps65090_fet_enable(1);

	/* LED backlight reset */
	gpio_cfg_pin(EXYNOS5420_GPIO_X30, S5P_GPIO_OUTPUT);
	gpio_set_value(EXYNOS5420_GPIO_X30, onoff);
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
	/*
	 * The reset value for FIMD SYSMMU register MMU_CTRL:0x14640000 is 3.
	 * This means FIMD SYSMMU is on by default on Exynos5420.
	 * Since in u-boot we don't enable MMU, we are disabling FIMD SYSMMU.
	 */
	writel(0x0, 0x14640000);
}
#endif
