/*
 * Copyright (C) 2013 Samsung Electronics
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
#include <errno.h>
#include <fdtdec.h>
#include <i2c.h>
#include <lcd.h>
#include <spi.h>
#include <tmu.h>
#include <asm/io.h>
#include <asm/arch/board.h>
#include <asm/arch/cpu.h>
#include <asm/arch/dwmmc.h>
#include <asm/arch/fb.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/power.h>
#include <asm/arch/system.h>
#include <power/pmic.h>
#include <power/max77686_pmic.h>
#include <power/max77802_pmic.h>
#include <power/s2mps11_pmic.h>
#include <power/tps65090_pmic.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * If required - get the run time determined number of DRAM banks available on
 * the device.
 */
static int get_num_dram_banks(void)
{
#ifdef CONFIG_RUN_TIME_BANK_NUMBER
	return board_get_num_dram_banks();
#else
	return CONFIG_NR_DRAM_BANKS;
#endif
}

#if defined CONFIG_EXYNOS_TMU
/*
 * Boot Time Thermal Analysis for SoC temperature threshold breach
 */
static void boot_temp_check(void)
{
	int temp;

	switch (tmu_monitor(&temp)) {
	case TMU_STATUS_NORMAL:
		break;
	/* Status TRIPPED ans WARNING means corresponding threshold breach */
	case TMU_STATUS_TRIPPED:
		puts("EXYNOS_TMU: TRIPPING! Device power going down ...\n");
		power_shutdown();
		hang();
		break;
	case TMU_STATUS_WARNING:
		puts("EXYNOS_TMU: WARNING! Temperature very high\n");
		break;
	/*
	 * TMU_STATUS_INIT means something is wrong with temperature sensing
	 * and TMU status was changed back from NORMAL to INIT.
	 */
	case TMU_STATUS_INIT:
		debug("EXYNOS_TMU: TMU is in init state\n");
		break;
	default:
		debug("EXYNOS_TMU: Unknown TMU state\n");
	}
}
#endif

int board_init(void)
{
	gd->bd->bi_boot_params = (PHYS_SDRAM_1 + 0x100UL);
#if defined CONFIG_EXYNOS_TMU
	if (tmu_init(gd->fdt_blob) != TMU_STATUS_NORMAL) {
		debug("%s: Failed to init TMU\n", __func__);
		return -1;
	}
	boot_temp_check();
#endif

#ifdef CONFIG_EXYNOS_SPI
	spi_init();
#endif
#ifdef CONFIG_SOUND_MAX98095
	if (board_enable_audio_codec()) {
		debug("%s: Failed to init audio\n", __func__);
		return -1;
	}
#endif

	return exynos_init();
}

int dram_init(void)
{
	int i;
	u32 addr;
	int real_nr_dram_banks = get_num_dram_banks();

	for (i = 0; i < real_nr_dram_banks; i++) {
		addr = CONFIG_SYS_SDRAM_BASE + (i * SDRAM_BANK_SIZE);
		gd->ram_size += get_ram_size((long *)addr, SDRAM_BANK_SIZE);
	}
	return 0;
}

void dram_init_banksize(void)
{
	int i;
	u32 addr, size;
	int real_nr_dram_banks = get_num_dram_banks();

	for (i = 0; i < real_nr_dram_banks; i++) {
		addr = CONFIG_SYS_SDRAM_BASE + (i * SDRAM_BANK_SIZE);
		size = get_ram_size((long *)addr, SDRAM_BANK_SIZE);

		gd->bd->bi_dram[i].start = addr;
		gd->bd->bi_dram[i].size = size;
	}
#ifdef CONFIG_RUN_TIME_BANK_NUMBER
	if (i < CONFIG_NR_DRAM_BANKS)
		memset(gd->bd->bi_dram + i, 0,
		       sizeof(gd->bd->bi_dram[0]
			      ) * (CONFIG_NR_DRAM_BANKS - 1));
#endif
}

static int board_uart_init(void)
{
	int err, uart_id, ret = 0;

	for (uart_id = PERIPH_ID_UART0; uart_id <= PERIPH_ID_UART3; uart_id++) {
		err = exynos_pinmux_config(uart_id, PINMUX_FLAG_NONE);
		if (err) {
			debug("UART%d not configured\n",
					 (uart_id - PERIPH_ID_UART0));
			ret |= err;
		}
	}
	return ret;
}

#ifdef CONFIG_BOARD_EARLY_INIT_F
int board_early_init_f(void)
{
	int err;
	err = board_uart_init();
	if (err) {
		debug("UART init failed\n");
		return err;
	}
#ifdef CONFIG_SYS_I2C_INIT_BOARD
	board_i2c_init(gd->fdt_blob);
#endif
#ifdef CONFIG_EXYNOS_FB
	exynos_lcd_early_init(gd->fdt_blob);
#endif
	return 0;
}
#endif

#if defined(CONFIG_POWER)
#ifdef CONFIG_POWER_MAX77686
int board_init_max77686(void)
{
	const struct pmic_init_ops pmic_ops[] = {
		{PMIC_REG_UPDATE, MAX77686_REG_PMIC_32KHZ,
		 MAX77686_32KHCP_EN|MAX77686_32KHCP_LOW_JITTER},
		{PMIC_REG_WRITE, MAX77686_REG_PMIC_BUCK1OUT,
		 MAX77686_BUCK1OUT_1V},
		{PMIC_REG_UPDATE, MAX77686_REG_PMIC_BUCK1CRTL,
		 MAX77686_BUCK1CTRL_EN},
		{PMIC_REG_WRITE, MAX77686_REG_PMIC_BUCK2DVS1,
		 MAX77686_BUCK2DVS1_1_3V},
		{PMIC_REG_UPDATE, MAX77686_REG_PMIC_BUCK2CTRL1,
		 MAX77686_BUCK2CTRL_ON},
		{PMIC_REG_WRITE, MAX77686_REG_PMIC_BUCK3DVS1,
		 MAX77686_BUCK3DVS1_1_0125V},
		{PMIC_REG_UPDATE, MAX77686_REG_PMIC_BUCK3CTRL,
		 MAX77686_BUCK3CTRL_ON},
		{PMIC_REG_WRITE, MAX77686_REG_PMIC_BUCK4DVS1,
		 MAX77686_BUCK4DVS1_1_2V},
		{PMIC_REG_UPDATE, MAX77686_REG_PMIC_BUCK4CTRL1,
		 MAX77686_BUCK3CTRL_ON},
		{PMIC_REG_UPDATE, MAX77686_REG_PMIC_LDO2CTRL1,
		 MAX77686_LD02CTRL1_1_5V | MAX77686_EN_LDO},
		{PMIC_REG_UPDATE, MAX77686_REG_PMIC_LDO3CTRL1,
		 MAX77686_LD03CTRL1_1_8V | MAX77686_EN_LDO},
		{PMIC_REG_UPDATE, MAX77686_REG_PMIC_LDO5CTRL1,
		 MAX77686_LD05CTRL1_1_8V | MAX77686_EN_LDO},
		{PMIC_REG_UPDATE, MAX77686_REG_PMIC_LDO10CTRL1,
		 MAX77686_LD10CTRL1_1_8V | MAX77686_EN_LDO},
		{PMIC_REG_BAIL}
	};

	return pmic_common_init(COMPAT_MAXIM_MAX77686_PMIC, pmic_ops);
}
#endif

#ifdef CONFIG_POWER_MAX77802
int board_init_max77802(void)
{
	const struct pmic_init_ops pmic_ops[] = {
		{PMIC_REG_UPDATE, MAX77802_REG_PMIC_32KHZ, MAX77802_32KHCP_EN},
		{PMIC_REG_WRITE, MAX77802_REG_PMIC_BUCK1DVS1,
		 MAX77802_BUCK1DVS1_1V},
		{PMIC_REG_UPDATE, MAX77802_REG_PMIC_BUCK1CRTL,
		 MAX77802_BUCK_TYPE1_ON | MAX77802_BUCK_TYPE1_IGNORE_PWRREQ},
		{PMIC_REG_WRITE, MAX77802_REG_PMIC_BUCK2DVS1,
		 MAX77802_BUCK2DVS1_1_2625V},
		{PMIC_REG_UPDATE, MAX77802_REG_PMIC_BUCK2CTRL1,
		 MAX77802_BUCK_TYPE2_ON | MAX77802_BUCK_TYPE2_IGNORE_PWRREQ},
		{PMIC_REG_WRITE, MAX77802_REG_PMIC_BUCK3DVS1,
		 MAX77802_BUCK3DVS1_1V},
		{PMIC_REG_UPDATE, MAX77802_REG_PMIC_BUCK3CTRL1,
		 MAX77802_BUCK_TYPE2_ON | MAX77802_BUCK_TYPE2_IGNORE_PWRREQ},
		{PMIC_REG_WRITE, MAX77802_REG_PMIC_BUCK4DVS1,
		 MAX77802_BUCK4DVS1_1V},
		{PMIC_REG_UPDATE, MAX77802_REG_PMIC_BUCK4CTRL1,
		 MAX77802_BUCK_TYPE2_ON | MAX77802_BUCK_TYPE2_IGNORE_PWRREQ},
		{PMIC_REG_WRITE, MAX77802_REG_PMIC_BUCK6DVS1,
		 MAX77802_BUCK6DVS1_1V},
		{PMIC_REG_UPDATE, MAX77802_REG_PMIC_BUCK6CTRL,
		 MAX77802_BUCK_TYPE1_ON | MAX77802_BUCK_TYPE1_IGNORE_PWRREQ},
		/* Disable Boost(bypass) OUTPUT */
		{PMIC_REG_WRITE, MAX77802_REG_PMIC_BOOSTCTRL,
		 MAX77802_BOOSTCTRL_OFF},

		{PMIC_REG_BAIL}
	};

	return pmic_common_init(COMPAT_MAXIM_MAX77802_PMIC, pmic_ops);
}
#endif

#ifdef CONFIG_POWER_S2MPS11
int board_init_s2mps11(void)
{
	const struct pmic_init_ops pmic_ops[] = {
		{PMIC_REG_WRITE, S2MPS11_BUCK1_CTRL2, S2MPS11_BUCK_CTRL2_1V},
		{PMIC_REG_WRITE, S2MPS11_BUCK2_CTRL2, S2MPS11_BUCK_CTRL2_1_2625V},
		{PMIC_REG_WRITE, S2MPS11_BUCK3_CTRL2, S2MPS11_BUCK_CTRL2_1V},
		{PMIC_REG_WRITE, S2MPS11_BUCK4_CTRL2, S2MPS11_BUCK_CTRL2_1V},
		{PMIC_REG_WRITE, S2MPS11_BUCK6_CTRL2, S2MPS11_BUCK_CTRL2_1V},
		{PMIC_REG_UPDATE, S2MPS11_REG_RTC_CTRL,
		 S2MPS11_RTC_CTRL_32KHZ_CP_EN | S2MPS11_RTC_CTRL_JIT},
		{PMIC_REG_BAIL}
	};

	return pmic_common_init(COMPAT_SAMSUNG_S2MPS11_PMIC, pmic_ops);
}
#endif

/* Set up the TPS65090 if present */
int board_init_tps65090(void)
{
	int ret = 0;

#ifdef CONFIG_POWER_TPS65090
	/*
	 * The TPS65090 may not be in the device tree. If so, it is not
	 * an error.
	 */
	ret = tps65090_init();
	if (ret == -ENODEV)
		return 0;

	/* Enable access to SD card on FET4 even if someone turned it off */
	if (!ret)
		ret = tps65090_fet_enable(4);

	/* Disable backlight and LCD FET, initially */
	if (!ret)
		ret = tps65090_fet_disable(1);
	if (!ret)
		ret = tps65090_fet_disable(6);
#endif

	return ret;
}
#endif /* CONFIG_POWER */

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	stdio_print_current_devices();

	if (cros_ec_get_error()) {
		/* Force console on */
		gd->flags &= ~GD_FLG_SILENT;

		printf("cros-ec communications failure %d\n",
		       cros_ec_get_error());
		puts("\nPlease reset with Power+Refresh\n\n");
		panic("Cannot init cros-ec device");
		return -1;
	}
	return 0;
}
#endif

/**
 * Read and clear the marker value; then return the read value.
 *
 * This marker is set to EXYNOS5_SPL_MARKER when SPL runs. Then in U-Boot
 * we can check (and clear) this marker to see if we were run from SPL.
 * If we were called from another U-Boot, the marker will be clear.
 *
 * @return marker value (EXYNOS5_SPL_MARKER if we were run from SPL, else 0)
 */
static uint32_t exynos5_read_and_clear_spl_marker(void)
{
	uint32_t value, *marker = (uint32_t *)CONFIG_SPL_MARKER;

	value = *marker;
	*marker = 0;

	return value;
}

int board_is_processor_reset(void)
{
	static uint8_t inited, is_reset;
	uint32_t marker_value;

	if (!inited) {
		marker_value = exynos5_read_and_clear_spl_marker();
		is_reset = marker_value == EXYNOS5_SPL_MARKER;
		inited = 1;
	}

	return is_reset;
}

#ifdef CONFIG_OF_CONTROL
#define MAX_REV_GPIO_COUNT	5
void board_get_full_revision(int *board_rev_out, int *subrev_out)
{
	struct fdt_gpio_state gpios[MAX_REV_GPIO_COUNT];
	unsigned gpio_list[MAX_REV_GPIO_COUNT];
	int board_rev = -1;
	int subrev = 0;
	int count = 0;
	int node;

	node = fdtdec_next_compatible(gd->fdt_blob, 0,
				      COMPAT_GOOGLE_BOARD_REV);
	if (node >= 0) {
		count = fdtdec_decode_gpios(gd->fdt_blob, node,
					    "google,board-rev-gpios", gpios,
					    MAX_REV_GPIO_COUNT);
	}
	if (count > 0) {
		int i;
		const u8 *map = NULL;

		for (i = 0; i < count; i++)
			gpio_list[i] = gpios[i].gpio;
		board_rev = gpio_read_strappings(gpio_list, count);

		/* If there's a revision map, apply it */
		map = fdtdec_locate_byte_array(gd->fdt_blob, node,
					       "google,board-rev-map",
					       2 * (board_rev + 1));
		if (map) {
			subrev = map[(board_rev * 2) + 1];
			board_rev = map[board_rev * 2];
		}
	} else {
		debug("%s: No board revision information in fdt\n", __func__);
	}

	if (board_rev_out)
		*board_rev_out = board_rev;
	if (subrev_out)
		*subrev_out = subrev;
}

int board_get_revision(void)
{
	int rev;

	board_get_full_revision(&rev, NULL);
	return rev;
}

/**
 * Fix-up the kernel device tree so the powered-while-resumed is added.
 *
 * TODO: Just add this to the kernel.  It used to be useful when we supported
 * pre-production boards, but we don't anymore.
 *
 * @param blob		Device tree blob
 * @param bd		Pointer to board information
 * @return 0 if ok, -1 on error (e.g. not enough space in fdt)
 */
static int ft_board_setup_tpm_resume(void *blob, bd_t *bd)
{
	static const char kernel_tpm_compat[] = "infineon,slb9635tt";
	static const char prop_name[] = "powered-while-suspended";
	int err, node;

	node = fdt_node_offset_by_compatible(blob, 0, kernel_tpm_compat);
	if (node < 0) {
		debug("%s: fail to find %s: %d\n", __func__,
				kernel_tpm_compat, node);
		return 0;
	}

	err = fdt_setprop(blob, node, prop_name, NULL, 0);
	if (err) {
		debug("%s: fail to setprop: %d\n", __func__, err);
		return -1;
	}

	return 0;
}

int ft_system_setup(void *blob, bd_t *bd)
{
	return ft_board_setup_tpm_resume(blob, bd);
}

__weak int ft_board_setup(void *blob, bd_t *bd)
{
	return ft_system_setup(blob, bd);
}
#endif

int arch_early_init_r(void)
{
#ifdef CONFIG_CROS_EC
	if (cros_ec_board_init()) {
                printf("%s: Failed to init EC\n", __func__);
                return 0;
        }
#endif

	if (power_watchdog_fired())
		puts("** Watchdog reset\n");

	return 0;
}

void board_lcd_panel_on(vidinfo_t *vid)
{
	if (defer_display_init(gd->fdt_blob)) {
		debug("%s: display init deferred\n", __func__);
		return;
	}

	if (!board_is_processor_reset())
		return;

	bootstage_start(BOOTSTAGE_ID_ACCUM_LCD, "LCD init");
	exynos_lcd_panel_on(vid);
	bootstage_accum(BOOTSTAGE_ID_ACCUM_LCD);
}
