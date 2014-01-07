/*
 * Copyright (c) 2012 Samsung Electronics.
 * Abhilash Kesavan <a.kesavan@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <fdtdec.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/sromc.h>

static void exynos5_uart_config(int peripheral)
{
	int i, start, count;

	switch (peripheral) {
	case PERIPH_ID_UART0:
		start = EXYNOS5_GPIO_A00;
		count = 4;
		break;
	case PERIPH_ID_UART1:
		start = EXYNOS5_GPIO_D00;
		count = 4;
		break;
	case PERIPH_ID_UART2:
		start = EXYNOS5_GPIO_A10;
		count = 4;
		break;
	case PERIPH_ID_UART3:
		start = EXYNOS5_GPIO_A14;
		count = 2;
		break;
	default:
		return;
	}
	for (i = start; i < start + count; i++) {
		gpio_set_pull(i, S5P_GPIO_PULL_NONE);
		gpio_cfg_pin(i, S5P_GPIO_FUNC(0x2));
	}
}

static void exynos5420_uart_config(int peripheral)
{
	int i, start, count;

	switch (peripheral) {
	case PERIPH_ID_UART0:
		start = EXYNOS5420_GPIO_A00;
		count = 4;
		break;
	case PERIPH_ID_UART1:
		start = EXYNOS5420_GPIO_A04;
		count = 4;
		break;
	case PERIPH_ID_UART2:
		start = EXYNOS5420_GPIO_A10;
		count = 4;
		break;
	case PERIPH_ID_UART3:
		start = EXYNOS5420_GPIO_A14;
		count = 2;
		break;
	}
	for (i = start; i < start + count; i++) {
		gpio_set_pull(i, S5P_GPIO_PULL_NONE);
		gpio_cfg_pin(i, S5P_GPIO_FUNC(0x2));
	}
}

static int exynos5_mmc_config(int peripheral, int flags)
{
	int i, start = 0, start_ext = 0, gpio_func = 0;

	switch (peripheral) {
	case PERIPH_ID_SDMMC0:
		start = EXYNOS5_GPIO_C00;
		start_ext = EXYNOS5_GPIO_C10;
		gpio_func = S5P_GPIO_FUNC(0x2);
		break;
	case PERIPH_ID_SDMMC1:
		start = EXYNOS5_GPIO_C20;
		start_ext = 0;
		break;
	case PERIPH_ID_SDMMC2:
		start = EXYNOS5_GPIO_C30;
		start_ext = EXYNOS5_GPIO_C43;
		gpio_func = S5P_GPIO_FUNC(0x3);
		break;
	case PERIPH_ID_SDMMC3:
		start = EXYNOS5_GPIO_C40;
		start_ext = 0;
		break;
	default:
		return -1;
	}
	if ((flags & PINMUX_FLAG_8BIT_MODE) && !start_ext) {
		debug("SDMMC device %d does not support 8bit mode",
				peripheral);
		return -1;
	}
	if (flags & PINMUX_FLAG_8BIT_MODE) {
		for (i = start_ext; i <= (start_ext + 3); i++) {
			gpio_cfg_pin(i, gpio_func);
			gpio_set_pull(i, S5P_GPIO_PULL_UP);
			gpio_set_drv(i, S5P_GPIO_DRV_4X);
		}
	}
	for (i = 0; i < 2; i++) {
		gpio_cfg_pin(start + i, S5P_GPIO_FUNC(0x2));
		gpio_set_pull(start + i, S5P_GPIO_PULL_NONE);
		gpio_set_drv(start + i, S5P_GPIO_DRV_4X);
	}
	for (i = 3; i <= 6; i++) {
		gpio_cfg_pin(start + i, S5P_GPIO_FUNC(0x2));
		gpio_set_pull(start + i, S5P_GPIO_PULL_UP);
		gpio_set_drv(start + i, S5P_GPIO_DRV_4X);
	}

	return 0;
}

static int exynos5420_mmc_config(int peripheral, int flags)
{
	int i, start = 0, start_ext = 0, gpio_func = 0;

	switch (peripheral) {
	case PERIPH_ID_SDMMC0:
		start = EXYNOS5420_GPIO_C00;
		start_ext = EXYNOS5420_GPIO_C30;
		gpio_func = S5P_GPIO_FUNC(0x2);
		break;
	case PERIPH_ID_SDMMC1:
		start = EXYNOS5420_GPIO_C10;
		start_ext = EXYNOS5420_GPIO_D14;
		gpio_func = S5P_GPIO_FUNC(0x2);
		break;
	case PERIPH_ID_SDMMC2:
		start = EXYNOS5420_GPIO_C20;
		start_ext = 0;
		gpio_func = S5P_GPIO_FUNC(0x2);
		break;
	}
	if ((flags & PINMUX_FLAG_8BIT_MODE) && !start_ext) {
		debug("SDMMC device %d does not support 8bit mode",
				peripheral);
		return -1;
	}
	if (flags & PINMUX_FLAG_8BIT_MODE) {
		for (i = start_ext; i <= (start_ext + 3); i++) {
			gpio_cfg_pin(i, gpio_func);
			gpio_set_pull(i, S5P_GPIO_PULL_UP);
			gpio_set_drv(i, S5P_GPIO_DRV_4X);
		}
	}
	for (i = 0; i < 3; i++) {
		/*
		 * MMC0 is intended to be used for eMMC. The
		 * card detect pin is used as a VDDEN signal to
		 * power on the eMMC. The 5420 iROM makes
		 * this same assumption.
		 */
		if ((peripheral == PERIPH_ID_SDMMC0) && (i == 2)) {
			gpio_set_drv(start + i, S5P_GPIO_DRV_4X);
			gpio_set_value(start + i, 1);
			gpio_cfg_pin(start + i, S5P_GPIO_OUTPUT);
			gpio_set_pull(start + i, S5P_GPIO_PULL_NONE);
		} else {
			gpio_set_drv(start + i, S5P_GPIO_DRV_4X);
			gpio_cfg_pin(start + i, gpio_func);
			gpio_set_pull(start + i, S5P_GPIO_PULL_NONE);
		}
	}

	for (i = 3; i <= 6; i++) {
		gpio_cfg_pin(start +  i, S5P_GPIO_FUNC(0x2));
		gpio_set_pull(start + i, S5P_GPIO_PULL_UP);
		gpio_set_drv(start + i, S5P_GPIO_DRV_4X);
	}

	return 0;
}

static void exynos5_sromc_config(int flags)
{
	int i;

	/*
	 * SROM:CS1 and EBI
	 *
	 * GPY0[0]	SROM_CSn[0]
	 * GPY0[1]	SROM_CSn[1](2)
	 * GPY0[2]	SROM_CSn[2]
	 * GPY0[3]	SROM_CSn[3]
	 * GPY0[4]	EBI_OEn(2)
	 * GPY0[5]	EBI_EEn(2)
	 *
	 * GPY1[0]	EBI_BEn[0](2)
	 * GPY1[1]	EBI_BEn[1](2)
	 * GPY1[2]	SROM_WAIT(2)
	 * GPY1[3]	EBI_DATA_RDn(2)
	 */
	gpio_cfg_pin(EXYNOS5_GPIO_Y00 + (flags & PINMUX_FLAG_BANK),
		     S5P_GPIO_FUNC(2));
	gpio_cfg_pin(EXYNOS5_GPIO_Y04, S5P_GPIO_FUNC(2));
	gpio_cfg_pin(EXYNOS5_GPIO_Y05, S5P_GPIO_FUNC(2));

	for (i = 0; i < 4; i++)
		gpio_cfg_pin(EXYNOS5_GPIO_Y10 + i, S5P_GPIO_FUNC(2));

	/*
	 * EBI: 8 Addrss Lines
	 *
	 * GPY3[0]	EBI_ADDR[0](2)
	 * GPY3[1]	EBI_ADDR[1](2)
	 * GPY3[2]	EBI_ADDR[2](2)
	 * GPY3[3]	EBI_ADDR[3](2)
	 * GPY3[4]	EBI_ADDR[4](2)
	 * GPY3[5]	EBI_ADDR[5](2)
	 * GPY3[6]	EBI_ADDR[6](2)
	 * GPY3[7]	EBI_ADDR[7](2)
	 *
	 * EBI: 16 Data Lines
	 *
	 * GPY5[0]	EBI_DATA[0](2)
	 * GPY5[1]	EBI_DATA[1](2)
	 * GPY5[2]	EBI_DATA[2](2)
	 * GPY5[3]	EBI_DATA[3](2)
	 * GPY5[4]	EBI_DATA[4](2)
	 * GPY5[5]	EBI_DATA[5](2)
	 * GPY5[6]	EBI_DATA[6](2)
	 * GPY5[7]	EBI_DATA[7](2)
	 *
	 * GPY6[0]	EBI_DATA[8](2)
	 * GPY6[1]	EBI_DATA[9](2)
	 * GPY6[2]	EBI_DATA[10](2)
	 * GPY6[3]	EBI_DATA[11](2)
	 * GPY6[4]	EBI_DATA[12](2)
	 * GPY6[5]	EBI_DATA[13](2)
	 * GPY6[6]	EBI_DATA[14](2)
	 * GPY6[7]	EBI_DATA[15](2)
	 */
	for (i = 0; i < 8; i++) {
		gpio_cfg_pin(EXYNOS5_GPIO_Y30 + i, S5P_GPIO_FUNC(2));
		gpio_set_pull(EXYNOS5_GPIO_Y30 + i, S5P_GPIO_PULL_UP);

		gpio_cfg_pin(EXYNOS5_GPIO_Y50 + i, S5P_GPIO_FUNC(2));
		gpio_set_pull(EXYNOS5_GPIO_Y50 + i, S5P_GPIO_PULL_UP);

		gpio_cfg_pin(EXYNOS5_GPIO_Y60 + i, S5P_GPIO_FUNC(2));
		gpio_set_pull(EXYNOS5_GPIO_Y60 + i, S5P_GPIO_PULL_UP);
	}
}

static void exynos5_i2c_config(int peripheral, int flags)
{

	switch (peripheral) {
	case PERIPH_ID_I2C0:
		gpio_cfg_pin(EXYNOS5_GPIO_B30, S5P_GPIO_FUNC(0x2));
		gpio_cfg_pin(EXYNOS5_GPIO_B31, S5P_GPIO_FUNC(0x2));
		break;
	case PERIPH_ID_I2C1:
		gpio_cfg_pin(EXYNOS5_GPIO_B32, S5P_GPIO_FUNC(0x2));
		gpio_cfg_pin(EXYNOS5_GPIO_B33, S5P_GPIO_FUNC(0x2));
		break;
	case PERIPH_ID_I2C2:
		gpio_cfg_pin(EXYNOS5_GPIO_A06, S5P_GPIO_FUNC(0x3));
		gpio_cfg_pin(EXYNOS5_GPIO_A07, S5P_GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C3:
		gpio_cfg_pin(EXYNOS5_GPIO_A12, S5P_GPIO_FUNC(0x3));
		gpio_cfg_pin(EXYNOS5_GPIO_A13, S5P_GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C4:
		gpio_cfg_pin(EXYNOS5_GPIO_A20, S5P_GPIO_FUNC(0x3));
		gpio_cfg_pin(EXYNOS5_GPIO_A21, S5P_GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C5:
		gpio_cfg_pin(EXYNOS5_GPIO_A22, S5P_GPIO_FUNC(0x3));
		gpio_cfg_pin(EXYNOS5_GPIO_A23, S5P_GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C6:
		gpio_cfg_pin(EXYNOS5_GPIO_B13, S5P_GPIO_FUNC(0x4));
		gpio_cfg_pin(EXYNOS5_GPIO_B14, S5P_GPIO_FUNC(0x4));
		break;
	case PERIPH_ID_I2C7:
		gpio_cfg_pin(EXYNOS5_GPIO_B22, S5P_GPIO_FUNC(0x3));
		gpio_cfg_pin(EXYNOS5_GPIO_B23, S5P_GPIO_FUNC(0x3));
		break;
	default:
		return;
	}
}

static void exynos5420_i2c_config(int peripheral)
{

	switch (peripheral) {
	case PERIPH_ID_I2C0:
		gpio_cfg_pin(EXYNOS5420_GPIO_B30, S5P_GPIO_FUNC(0x2));
		gpio_cfg_pin(EXYNOS5420_GPIO_B31, S5P_GPIO_FUNC(0x2));
		break;
	case PERIPH_ID_I2C1:
		gpio_cfg_pin(EXYNOS5420_GPIO_B32, S5P_GPIO_FUNC(0x2));
		gpio_cfg_pin(EXYNOS5420_GPIO_B33, S5P_GPIO_FUNC(0x2));
		break;
	case PERIPH_ID_I2C2:
		gpio_cfg_pin(EXYNOS5420_GPIO_A06, S5P_GPIO_FUNC(0x3));
		gpio_set_pull(EXYNOS5420_GPIO_A06, S5P_GPIO_PULL_NONE);
		gpio_cfg_pin(EXYNOS5420_GPIO_A07, S5P_GPIO_FUNC(0x3));
		gpio_set_pull(EXYNOS5420_GPIO_A07, S5P_GPIO_PULL_NONE);
		break;
	case PERIPH_ID_I2C3:
		gpio_cfg_pin(EXYNOS5420_GPIO_A12, S5P_GPIO_FUNC(0x3));
		gpio_cfg_pin(EXYNOS5420_GPIO_A13, S5P_GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C4:
		gpio_cfg_pin(EXYNOS5420_GPIO_A20, S5P_GPIO_FUNC(0x3));
		gpio_set_pull(EXYNOS5420_GPIO_A20, S5P_GPIO_PULL_NONE);
		gpio_cfg_pin(EXYNOS5420_GPIO_A21, S5P_GPIO_FUNC(0x3));
		gpio_set_pull(EXYNOS5420_GPIO_A21, S5P_GPIO_PULL_NONE);
		break;
	case PERIPH_ID_I2C5:
		gpio_cfg_pin(EXYNOS5420_GPIO_A22, S5P_GPIO_FUNC(0x3));
		gpio_cfg_pin(EXYNOS5420_GPIO_A23, S5P_GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C6:
		gpio_cfg_pin(EXYNOS5420_GPIO_B13, S5P_GPIO_FUNC(0x4));
		gpio_cfg_pin(EXYNOS5420_GPIO_B14, S5P_GPIO_FUNC(0x4));
		break;
	case PERIPH_ID_I2C7:
		gpio_cfg_pin(EXYNOS5420_GPIO_B22, S5P_GPIO_FUNC(0x3));
		gpio_set_pull(EXYNOS5420_GPIO_B22, S5P_GPIO_PULL_NONE);
		gpio_cfg_pin(EXYNOS5420_GPIO_B23, S5P_GPIO_FUNC(0x3));
		gpio_set_pull(EXYNOS5420_GPIO_B23, S5P_GPIO_PULL_NONE);
		break;
	case PERIPH_ID_I2C8:
		gpio_cfg_pin(EXYNOS5420_GPIO_B34, S5P_GPIO_FUNC(0x2));
		gpio_set_pull(EXYNOS5420_GPIO_B34, S5P_GPIO_PULL_NONE);
		gpio_cfg_pin(EXYNOS5420_GPIO_B35, S5P_GPIO_FUNC(0x2));
		gpio_set_pull(EXYNOS5420_GPIO_B35, S5P_GPIO_PULL_NONE);
		break;
	case PERIPH_ID_I2C9:
		gpio_cfg_pin(EXYNOS5420_GPIO_B36, S5P_GPIO_FUNC(0x2));
		gpio_set_pull(EXYNOS5420_GPIO_B36, S5P_GPIO_PULL_NONE);
		gpio_cfg_pin(EXYNOS5420_GPIO_B37, S5P_GPIO_FUNC(0x2));
		gpio_set_pull(EXYNOS5420_GPIO_B37, S5P_GPIO_PULL_NONE);
		break;
	case PERIPH_ID_I2C10:
		gpio_cfg_pin(EXYNOS5420_GPIO_B40, S5P_GPIO_FUNC(0x2));
		gpio_set_pull(EXYNOS5420_GPIO_B40, S5P_GPIO_PULL_NONE);
		gpio_cfg_pin(EXYNOS5420_GPIO_B41, S5P_GPIO_FUNC(0x2));
		gpio_set_pull(EXYNOS5420_GPIO_B41, S5P_GPIO_PULL_NONE);
		break;
	}
}

static void exynos5_i2s_config(int peripheral)
{
	int i;

	switch (peripheral) {
	case PERIPH_ID_I2S0:
		for (i = 0; i < 5; i++)
			gpio_cfg_pin(EXYNOS5_GPIO_Z0+i, S5P_GPIO_FUNC(0x02));
		break;
	case PERIPH_ID_I2S1:
		for (i = 0; i < 5; i++)
			gpio_cfg_pin(EXYNOS5_GPIO_B00+i, S5P_GPIO_FUNC(0x02));
		break;
	}
}

static void exynos5420_i2s_config(int peripheral)
{
	int i;

	switch (peripheral) {
	case PERIPH_ID_I2S0:
		for (i = 0; i < 5; i++)
			gpio_cfg_pin(EXYNOS5420_GPIO_Z0+i, S5P_GPIO_FUNC(0x02));
			break;
	}
}


void exynos5_spi_config(int peripheral)
{
	int cfg = 0, pin = 0, i;

	switch (peripheral) {
	case PERIPH_ID_SPI0:
		cfg = S5P_GPIO_FUNC(0x2);
		pin = EXYNOS5_GPIO_A20;
		break;
	case PERIPH_ID_SPI1:
		cfg = S5P_GPIO_FUNC(0x2);
		pin = EXYNOS5_GPIO_A24;
		break;
	case PERIPH_ID_SPI2:
		cfg = S5P_GPIO_FUNC(0x5);
		pin = EXYNOS5_GPIO_B11;
		break;
	case PERIPH_ID_SPI3:
		cfg = S5P_GPIO_FUNC(0x2);
		pin = EXYNOS5_GPIO_F10;
		break;
	case PERIPH_ID_SPI4:
		for (i = 0; i < 2; i++) {
			gpio_cfg_pin(EXYNOS5_GPIO_F02 + i, S5P_GPIO_FUNC(0x4));
			gpio_cfg_pin(EXYNOS5_GPIO_E04 + i, S5P_GPIO_FUNC(0x4));
		}
		break;
	default:
		return;
	}
	if (peripheral != PERIPH_ID_SPI4) {
		for (i = pin; i < pin + 4; i++)
			gpio_cfg_pin(i, cfg);
	}
}

void exynos5420_spi_config(int peripheral)
{
	int cfg = 0, pin = 0, i;

	switch (peripheral) {
	case PERIPH_ID_SPI0:
		cfg = S5P_GPIO_FUNC(0x2);
		pin = EXYNOS5420_GPIO_A20;
		break;
	case PERIPH_ID_SPI1:
		cfg = S5P_GPIO_FUNC(0x2);
		pin = EXYNOS5420_GPIO_A24;
		break;
	case PERIPH_ID_SPI2:
		cfg = S5P_GPIO_FUNC(0x5);
		pin = EXYNOS5420_GPIO_B11;
		break;
	case PERIPH_ID_SPI3:
		cfg = S5P_GPIO_FUNC(0x2);
		pin = EXYNOS5420_GPIO_F10;
		break;
	case PERIPH_ID_SPI4:
		for (i = 0; i < 2; i++) {
			gpio_cfg_pin(EXYNOS5420_GPIO_F02 + i,
				     S5P_GPIO_FUNC(0x4));
			gpio_cfg_pin(EXYNOS5420_GPIO_E04 + i,
				     S5P_GPIO_FUNC(0x4));
		}
		break;
	}
	if (peripheral != PERIPH_ID_SPI4) {
		for (i = pin; i < pin + 4; i++)
			gpio_cfg_pin(i, cfg);
	}
}

void exynos5_dp_config(int peripheral)
{
	switch (peripheral) {
	case PERIPH_ID_DPHPD:
		/* Set Hotplug detect for DP */
		gpio_cfg_pin(EXYNOS5_GPIO_X07, S5P_GPIO_FUNC(0x3));

		/*
		 * Hotplug detect should have an external pullup; disable the
		 * internal pulldown so they don't fight.
		 */
		gpio_set_pull(EXYNOS5_GPIO_X07, S5P_GPIO_PULL_NONE);
		break;
	}
}

static int exynos5_pinmux_config(int peripheral, int flags)
{
	switch (peripheral) {
	case PERIPH_ID_UART0:
	case PERIPH_ID_UART1:
	case PERIPH_ID_UART2:
	case PERIPH_ID_UART3:
		exynos5_uart_config(peripheral);
		break;
	case PERIPH_ID_SDMMC0:
	case PERIPH_ID_SDMMC1:
	case PERIPH_ID_SDMMC2:
	case PERIPH_ID_SDMMC3:
		return exynos5_mmc_config(peripheral, flags);
	case PERIPH_ID_SROMC:
		exynos5_sromc_config(flags);
		break;
	case PERIPH_ID_I2C0:
	case PERIPH_ID_I2C1:
	case PERIPH_ID_I2C2:
	case PERIPH_ID_I2C3:
	case PERIPH_ID_I2C4:
	case PERIPH_ID_I2C5:
	case PERIPH_ID_I2C6:
	case PERIPH_ID_I2C7:
		exynos5_i2c_config(peripheral, flags);
		break;
	case PERIPH_ID_I2S0:
	case PERIPH_ID_I2S1:
		exynos5_i2s_config(peripheral);
		break;
	case PERIPH_ID_SPI0:
	case PERIPH_ID_SPI1:
	case PERIPH_ID_SPI2:
	case PERIPH_ID_SPI3:
	case PERIPH_ID_SPI4:
		exynos5_spi_config(peripheral);
		break;
	case PERIPH_ID_DPHPD:
		exynos5_dp_config(peripheral);
		break;
	default:
		debug("%s: invalid peripheral %d", __func__, peripheral);
		return -1;
	}

	return 0;
}

static int exynos5420_pinmux_config(int peripheral, int flags)
{
	switch (peripheral) {
	case PERIPH_ID_UART0:
	case PERIPH_ID_UART1:
	case PERIPH_ID_UART2:
	case PERIPH_ID_UART3:
		exynos5420_uart_config(peripheral);
		break;
	case PERIPH_ID_SDMMC0:
	case PERIPH_ID_SDMMC1:
	case PERIPH_ID_SDMMC2:
	case PERIPH_ID_SDMMC3:
		return exynos5420_mmc_config(peripheral, flags);
	case PERIPH_ID_SPI0:
	case PERIPH_ID_SPI1:
	case PERIPH_ID_SPI2:
	case PERIPH_ID_SPI3:
	case PERIPH_ID_SPI4:
		exynos5420_spi_config(peripheral);
		break;
	case PERIPH_ID_I2C0:
	case PERIPH_ID_I2C1:
	case PERIPH_ID_I2C2:
	case PERIPH_ID_I2C3:
	case PERIPH_ID_I2C4:
	case PERIPH_ID_I2C5:
	case PERIPH_ID_I2C6:
	case PERIPH_ID_I2C7:
	case PERIPH_ID_I2C8:
	case PERIPH_ID_I2C9:
	case PERIPH_ID_I2C10:
		exynos5420_i2c_config(peripheral);
		break;
	case PERIPH_ID_I2S0:
		exynos5420_i2s_config(peripheral);
		break;
	default:
		debug("%s: invalid peripheral %d", __func__, peripheral);
		return -1;
	}

	return 0;
}

static void exynos4_i2c_config(int peripheral, int flags)
{
	struct exynos4_gpio_part1 *gpio1 =
		(struct exynos4_gpio_part1 *) samsung_get_base_gpio_part1();

	switch (peripheral) {
	case PERIPH_ID_I2C0:
		s5p_gpio_cfg_pin(&gpio1->d1, 0, S5P_GPIO_FUNC(0x2));
		s5p_gpio_cfg_pin(&gpio1->d1, 1, S5P_GPIO_FUNC(0x2));
		break;
	case PERIPH_ID_I2C1:
		s5p_gpio_cfg_pin(&gpio1->d1, 2, S5P_GPIO_FUNC(0x2));
		s5p_gpio_cfg_pin(&gpio1->d1, 3, S5P_GPIO_FUNC(0x2));
		break;
	case PERIPH_ID_I2C2:
		s5p_gpio_cfg_pin(&gpio1->a0, 6, S5P_GPIO_FUNC(0x3));
		s5p_gpio_cfg_pin(&gpio1->a0, 7, S5P_GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C3:
		s5p_gpio_cfg_pin(&gpio1->a1, 2, S5P_GPIO_FUNC(0x3));
		s5p_gpio_cfg_pin(&gpio1->a1, 3, S5P_GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C4:
		s5p_gpio_cfg_pin(&gpio1->b, 2, S5P_GPIO_FUNC(0x3));
		s5p_gpio_cfg_pin(&gpio1->b, 3, S5P_GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C5:
		s5p_gpio_cfg_pin(&gpio1->b, 6, S5P_GPIO_FUNC(0x3));
		s5p_gpio_cfg_pin(&gpio1->b, 7, S5P_GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C6:
		s5p_gpio_cfg_pin(&gpio1->c1, 3, S5P_GPIO_FUNC(0x4));
		s5p_gpio_cfg_pin(&gpio1->c1, 4, S5P_GPIO_FUNC(0x4));
		break;
	case PERIPH_ID_I2C7:
		s5p_gpio_cfg_pin(&gpio1->d0, 2, S5P_GPIO_FUNC(0x3));
		s5p_gpio_cfg_pin(&gpio1->d0, 3, S5P_GPIO_FUNC(0x3));
		break;
	}
}

static int exynos4_mmc_config(int peripheral, int flags)
{
	struct exynos4_gpio_part2 *gpio2 =
		(struct exynos4_gpio_part2 *)samsung_get_base_gpio_part2();
	struct s5p_gpio_bank *bank, *bank_ext;
	int i;

	switch (peripheral) {
	case PERIPH_ID_SDMMC0:
		bank = &gpio2->k0;
		bank_ext = &gpio2->k1;
		break;
	case PERIPH_ID_SDMMC2:
		bank = &gpio2->k2;
		bank_ext = &gpio2->k3;
		break;
	default:
		return -1;
	}
	for (i = 0; i < 7; i++) {
		if (i == 2)
			continue;
		s5p_gpio_cfg_pin(bank, i,  S5P_GPIO_FUNC(0x2));
		s5p_gpio_set_pull(bank, i, S5P_GPIO_PULL_NONE);
		s5p_gpio_set_drv(bank, i, S5P_GPIO_DRV_4X);
	}
	if (flags & PINMUX_FLAG_8BIT_MODE) {
		for (i = 3; i < 7; i++) {
			s5p_gpio_cfg_pin(bank_ext, i,  S5P_GPIO_FUNC(0x3));
			s5p_gpio_set_pull(bank_ext, i, S5P_GPIO_PULL_NONE);
			s5p_gpio_set_drv(bank_ext, i, S5P_GPIO_DRV_4X);
		}
	}

	return 0;
}

static void exynos4_uart_config(int peripheral)
{
	struct exynos4_gpio_part1 *gpio1 =
		(struct exynos4_gpio_part1 *)samsung_get_base_gpio_part1();
	struct s5p_gpio_bank *bank;
	int i, start, count;

	switch (peripheral) {
	case PERIPH_ID_UART0:
		bank = &gpio1->a0;
		start = 0;
		count = 4;
		break;
	case PERIPH_ID_UART1:
		bank = &gpio1->a0;
		start = 4;
		count = 4;
		break;
	case PERIPH_ID_UART2:
		bank = &gpio1->a1;
		start = 0;
		count = 4;
		break;
	case PERIPH_ID_UART3:
		bank = &gpio1->a1;
		start = 4;
		count = 2;
		break;
	default:
		return;
	}
	for (i = start; i < start + count; i++) {
		s5p_gpio_set_pull(bank, i, S5P_GPIO_PULL_NONE);
		s5p_gpio_cfg_pin(bank, i, S5P_GPIO_FUNC(0x2));
	}
}
static int exynos4_pinmux_config(int peripheral, int flags)
{
	switch (peripheral) {
	case PERIPH_ID_UART0:
	case PERIPH_ID_UART1:
	case PERIPH_ID_UART2:
	case PERIPH_ID_UART3:
		exynos4_uart_config(peripheral);
		break;
	case PERIPH_ID_I2C0:
	case PERIPH_ID_I2C1:
	case PERIPH_ID_I2C2:
	case PERIPH_ID_I2C3:
	case PERIPH_ID_I2C4:
	case PERIPH_ID_I2C5:
	case PERIPH_ID_I2C6:
	case PERIPH_ID_I2C7:
		exynos4_i2c_config(peripheral, flags);
		break;
	case PERIPH_ID_SDMMC0:
	case PERIPH_ID_SDMMC2:
		return exynos4_mmc_config(peripheral, flags);
	case PERIPH_ID_SDMMC1:
	case PERIPH_ID_SDMMC3:
	case PERIPH_ID_SDMMC4:
		debug("SDMMC device %d not implemented\n", peripheral);
		return -1;
	default:
		debug("%s: invalid peripheral %d", __func__, peripheral);
		return -1;
	}

	return 0;
}

int exynos_pinmux_config(int peripheral, int flags)
{
	if (proid_is_exynos542x())
		return exynos5420_pinmux_config(peripheral, flags);
	else if (proid_is_exynos5250())
		return exynos5_pinmux_config(peripheral, flags);
	else if (cpu_is_exynos4()) {
		return exynos4_pinmux_config(peripheral, flags);
	} else {
		debug("pinmux functionality not supported\n");
		return -1;
	}
}

#ifdef CONFIG_OF_CONTROL
static int exynos5_pinmux_decode_periph_id(const void *blob, int node)
{
	int err;
	u32 cell[3];

	err = fdtdec_get_int_array(blob, node, "interrupts", cell,
					ARRAY_SIZE(cell));
	if (err)
		return PERIPH_ID_NONE;

	/* check for invalid peripheral id */
	if ((PERIPH_ID_SDMMC4 > cell[1]) || (cell[1] < PERIPH_ID_UART0))
		return cell[1];

	debug(" invalid peripheral id\n");
	return PERIPH_ID_NONE;
}

int pinmux_decode_periph_id(const void *blob, int node)
{
	if (cpu_is_exynos5())
		return  exynos5_pinmux_decode_periph_id(blob, node);
	else
		return PERIPH_ID_NONE;
}
#endif
