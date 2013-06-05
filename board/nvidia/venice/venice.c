/*
 * Copyright (c) 2010-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <asm-generic/gpio.h>
#include <asm/arch/gpio.h>
#include <asm/arch/gp_padctrl.h>
#include <asm/arch/pinmux.h>
#include "pinmux-config-venice.h"
#include <i2c.h>

/* TODO(twarren@nvidia.com): Move to device tree */
#define PMU_I2C_ADDRESS		0x58	/* TPS65913 PMU */

/*
 * NOTE: On Venice, the TPS65090 PMIC is behind the EC, and
 * isn't directly controlled from U-Boot. See comments below
 * marked with *EC* for those PMIC inits.
 */

/*
 * Routine: pinmux_init
 * Description: Do individual peripheral pinmux configs
 */
void pinmux_init(void)
{
	pinmux_config_table(tegra114_pinmux_set_nontristate,
		ARRAY_SIZE(tegra114_pinmux_set_nontristate));

	pinmux_config_table(tegra114_pinmux_common,
		ARRAY_SIZE(tegra114_pinmux_common));

	pinmux_config_table(unused_pins_lowpower,
		ARRAY_SIZE(unused_pins_lowpower));

	/* Initialize any non-default pad configs (APB_MISC_GP regs) */
	padgrp_config_table(venice_padctrl, ARRAY_SIZE(venice_padctrl));
}

/* TODO(twarren@nvidia.com): Move to pmic infrastructure (pmic_common_init) */

/* Writes val to reg @ chip address pmu */
void i2c_write_pmic(uchar pmu, uchar reg, uchar val)
{
	uchar data_buffer[1];
	int ret;

	data_buffer[0] = val;

	ret = i2c_write(pmu, reg, 1, data_buffer, 1);
	if (ret)
		printf("%s: PMU i2c_write %02X<-%02X returned %d\n",
			__func__, reg, data_buffer[0], ret);
}

#if defined(CONFIG_TEGRA_MMC)
/*
 * Do I2C/PMU writes to bring up SD card bus power
 *
 */
void board_sdmmc_voltage_init(void)
{
	int ret = i2c_set_bus_num(0);	/* PMU is on bus 0 */
	if (ret)
		printf("%s: i2c_set_bus_num returned %d\n", __func__, ret);

	/* TPS65913: LDO9_VOLTAGE = 3.3V */
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x61, 0x31);

	/* TPS65913: LDO9_CTRL = Active */
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x60, 0x01);

	/* *EC*: TPS65090: FET6_CTRL = enable FET6 output auto discharge */
}

void board_vreg_init(void)
{
	int ret = i2c_set_bus_num(0);	/* PMU is on bus 0 */
	if (ret)
		printf("%s: i2c_set_bus_num returned %d\n", __func__, ret);

	/*
	 * Enable USB voltage: AVDD_USB
	 *   LDOUSB_VOLTAGE = 3.3v
	 *   LDOUSB_CTRL = Active
	 */
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x65, 0x31);
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x64, 0x01);

	/*
	 * Enable HVDD_USB3 voltage: HVDD_USB3_AP
	 *   LDOLN_VOLTAGE = 3.3v
	 *   LDOLN_CTRL = Active
	 */
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x63, 0x31);
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x62, 0x01);

	/*
	 * Enable additional VDD_1V1_CORE
	 *
	 *   SMPS7_CTRL: enable active: auto
	 *
	 *   VDD_CORE is provided by SMPS4_SW, 5 and 7 where
	 *   4 and 5 are enabled after power on.
	 */
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x30, 0x05);

	/*
	 * Set and enable 1V2_AVDD_USB
	 *   LDO1_VOLTAGE = 1.2v
	 *   LDO1_CTRL = Active
	 */
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x51, 0x07);
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x50, 0x01);

	/*
	 * Set and enable 1V2_GEN_VDD for VDDIO_HSIC, AVDD_DSI_CSI,
	 * WiFi, audio, and AVDD_HDMI_PLL
	 *   LDO3_VOLTAGE = 1.2v
	 *   LDO3_CTRL = Active
	 */
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x55, 0x07);
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x54, 0x01);

	/*
	 * Set and enable 1V2_USB3_PLL
	 *   LDO4_VOLTAGE = 1.2v
	 *   LDO4_CTRL = Active
	 */
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x57, 0x07);
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x56, 0x01);

	/*
	 * Set and enable 1V2_EDP
	 *   LDO5_VOLTAGE = 1.2v (TPS65913)
	 *   LDO5_CTRL = Active
	 */

	i2c_write_pmic(PMU_I2C_ADDRESS, 0x59, 0x07); /* LD05_VOLTAGE */
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x58, 0x01); /* LD05_CTRL */

	/*
	 * *EC*: Enable VDD_LCD_BL
	 *   VOUT1 (FET1) (TPS65090): auto discharge and enable
	 * *EC*: Enable 3V3_PANEL
	 *   VOUT4 (FET4) (TPS65090): auto discharge and enable
	 */

	/*
	 * Set and enable 2V8_SENSOR (temp sensor)
	 *   LDO6_VOLTAGE = 2.85v
	 *   LDO6_CTRL = Active
	 */
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x5B, 0x28);
	i2c_write_pmic(PMU_I2C_ADDRESS, 0x5A, 0x01);

	/*
	 * *EC*: Enable 3V3_VDD_WF
	 *   VOUT7 (FET7) (TPS65090): auto discharge and enable
	 */

	/* Enable LCD backlight */
	gpio_direction_output(DSI_PANEL_BL_EN_GPIO, 1);
}

/*
 * Routine: pin_mux_mmc
 * Description: setup the MMC muxes, power rails, etc.
 */
void pin_mux_mmc(void)
{
	/*
	 * NOTE: We don't do mmc-specific pin muxes here.
	 * They were done globally in pinmux_init().
	 */

	/* Bring up the SDIO3 power rail */
	board_sdmmc_voltage_init();
}
#endif /* MMC */
