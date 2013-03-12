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
#include <spi.h>
#include <tmu.h>
#include <asm/io.h>
#include <asm/arch/board.h>
#include <asm/arch/cpu.h>
#include <asm/arch/dwmmc.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/power.h>
#include <power/pmic.h>
#include <power/max77686_pmic.h>

DECLARE_GLOBAL_DATA_PTR;

struct local_info {
	struct cros_ec_dev *cros_ec_dev;	/* Pointer to cros_ec device */
	int cros_ec_err;			/* Error for cros_ec, 0 if ok */
};

static struct local_info local;

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
		set_ps_hold_ctrl();
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

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		addr = CONFIG_SYS_SDRAM_BASE + (i * SDRAM_BANK_SIZE);
		gd->ram_size += get_ram_size((long *)addr, SDRAM_BANK_SIZE);
	}
	return 0;
}

void dram_init_banksize(void)
{
	int i;
	u32 addr, size;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		addr = CONFIG_SYS_SDRAM_BASE + (i * SDRAM_BANK_SIZE);
		size = get_ram_size((long *)addr, SDRAM_BANK_SIZE);

		gd->bd->bi_dram[i].start = addr;
		gd->bd->bi_dram[i].size = size;
	}
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
	return err;
}
#endif

struct cros_ec_dev *board_get_cros_ec_dev(void)
{
	return local.cros_ec_dev;
}

static int board_init_cros_ec_devices(const void *blob)
{
	local.cros_ec_err = cros_ec_init(blob, &local.cros_ec_dev);
	if (local.cros_ec_err)
		return -1;  /* Will report in board_late_init() */

	return 0;
}

#if defined(CONFIG_POWER)
static int pmic_reg_update(struct pmic *p, int reg, uint regval)
{
	u32 val;
	int ret = 0;

	ret = pmic_reg_read(p, reg, &val);
	if (ret) {
		debug("%s: PMIC %d register read failed\n", __func__, reg);
		return -1;
	}
	val |= regval;
	ret = pmic_reg_write(p, reg, val);
	if (ret) {
		debug("%s: PMIC %d register write failed\n", __func__, reg);
		return -1;
	}
	return 0;
}

int power_init_board(void)
{
	struct pmic *p;

	set_ps_hold_ctrl();

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	if (pmic_init(I2C_PMIC))
		return -1;

	p = pmic_get("MAX77686_PMIC");
	if (!p)
		return -ENODEV;

	if (pmic_probe(p))
		return -1;

	if (pmic_reg_update(p, MAX77686_REG_PMIC_32KHZ, MAX77686_32KHCP_EN))
		return -1;

	if (pmic_reg_update(p, MAX77686_REG_PMIC_BBAT,
			    MAX77686_BBCHOSTEN | MAX77686_BBCVS_3_5V))
		return -1;

	/* VDD_MIF */
	if (pmic_reg_write(p, MAX77686_REG_PMIC_BUCK1OUT,
			   MAX77686_BUCK1OUT_1V)) {
		debug("%s: PMIC %d register write failed\n", __func__,
		      MAX77686_REG_PMIC_BUCK1OUT);
		return -1;
	}

	if (pmic_reg_update(p, MAX77686_REG_PMIC_BUCK1CRTL,
			    MAX77686_BUCK1CTRL_EN))
		return -1;

	/* VDD_ARM */
	if (pmic_reg_write(p, MAX77686_REG_PMIC_BUCK2DVS1,
			   MAX77686_BUCK2DVS1_1_3V)) {
		debug("%s: PMIC %d register write failed\n", __func__,
		      MAX77686_REG_PMIC_BUCK2DVS1);
		return -1;
	}

	if (pmic_reg_update(p, MAX77686_REG_PMIC_BUCK2CTRL1,
			    MAX77686_BUCK2CTRL_ON))
		return -1;

	/* VDD_INT */
	if (pmic_reg_write(p, MAX77686_REG_PMIC_BUCK3DVS1,
			   MAX77686_BUCK3DVS1_1_0125V)) {
		debug("%s: PMIC %d register write failed\n", __func__,
		      MAX77686_REG_PMIC_BUCK3DVS1);
		return -1;
	}

	if (pmic_reg_update(p, MAX77686_REG_PMIC_BUCK3CTRL,
			    MAX77686_BUCK3CTRL_ON))
		return -1;

	/* VDD_G3D */
	if (pmic_reg_write(p, MAX77686_REG_PMIC_BUCK4DVS1,
			   MAX77686_BUCK4DVS1_1_2V)) {
		debug("%s: PMIC %d register write failed\n", __func__,
		      MAX77686_REG_PMIC_BUCK4DVS1);
		return -1;
	}

	if (pmic_reg_update(p, MAX77686_REG_PMIC_BUCK4CTRL1,
			    MAX77686_BUCK3CTRL_ON))
		return -1;

	/* VDD_LDO2 */
	if (pmic_reg_update(p, MAX77686_REG_PMIC_LDO2CTRL1,
			    MAX77686_LD02CTRL1_1_5V | EN_LDO))
		return -1;

	/* VDD_LDO3 */
	if (pmic_reg_update(p, MAX77686_REG_PMIC_LDO3CTRL1,
			    MAX77686_LD03CTRL1_1_8V | EN_LDO))
		return -1;

	/* VDD_LDO5 */
	if (pmic_reg_update(p, MAX77686_REG_PMIC_LDO5CTRL1,
			    MAX77686_LD05CTRL1_1_8V | EN_LDO))
		return -1;

	/* VDD_LDO10 */
	if (pmic_reg_update(p, MAX77686_REG_PMIC_LDO10CTRL1,
			    MAX77686_LD10CTRL1_1_8V | EN_LDO))
		return -1;

	return 0;
}
#endif

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	stdio_print_current_devices();

	if (local.cros_ec_err) {
		/* Force console on */
		gd->flags &= ~GD_FLG_SILENT;

		printf("cros-ec communications failure %d\n",
		       local.cros_ec_err);
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


int board_get_revision(void)
{
	struct fdt_gpio_state gpios[CONFIG_BOARD_REV_GPIO_COUNT];
	unsigned gpio_list[CONFIG_BOARD_REV_GPIO_COUNT];
	int board_rev = -1;
	int count = 0;
	int node;

	node = fdtdec_next_compatible(gd->fdt_blob, 0,
				      COMPAT_GOOGLE_BOARD_REV);
	if (node >= 0) {
		count = fdtdec_decode_gpios(gd->fdt_blob, node,
				"google,board-rev-gpios", gpios,
				CONFIG_BOARD_REV_GPIO_COUNT);
	}
	if (count > 0) {
		int i;

		for (i = 0; i < count; i++)
			gpio_list[i] = gpios[i].gpio;
		board_rev = gpio_decode_number(gpio_list, count);
	} else {
		debug("%s: No board revision information in fdt\n", __func__);
	}

	return board_rev;
}

/**
 * Fix-up the kernel device tree so the bridge pd_n and rst_n gpios accurately
 * reflect the current board rev.
 *
 * @param blob		Device tree blob
 * @param bd		Pointer to board information
 * @return 0 if ok, -1 on error (e.g. not enough space in fdt)
 */
static int ft_board_setup_gpios(void *blob, bd_t *bd)
{
	int ret, rev, np, len;
	const struct fdt_property *prop;

	/* Do nothing for newer boards */
	rev = board_get_revision();
	if (rev < 4 || rev == 6)
		return 0;

	/*
	 * If this is an older board, replace powerdown-gpio contents with that
	 * of reset-gpio and delete reset-gpio from the dt.
	 * Also do nothing if we have a Parade PS8622 bridge.
	 */
	np = fdtdec_next_compatible(blob, 0, COMPAT_NXP_PTN3460);
	if (np < 0) {
		debug("%s: Could not find COMPAT_NXP_PTN3460\n", __func__);
		return 0;
	}

	prop = fdt_get_property(blob, np, "reset-gpio", &len);
	if (!prop) {
		debug("%s: Could not get property err=%d\n", __func__, len);
		return -1;
	}

	ret = fdt_setprop_inplace(blob, np, "powerdown-gpio", prop->data,
			len);
	if (ret) {
		debug("%s: Could not setprop inplace err=%d\n", __func__, ret);
		return -1;
	}

	ret = fdt_delprop(blob, np, "reset-gpio");
	if (ret) {
		debug("%s: Could not delprop err=%d\n", __func__, ret);
		return -1;
	}

	return 0;
}

/**
 * Fix-up the kernel device tree so the powered-while-resumed is added to MP
 * device tree.
 *
 * @param blob		Device tree blob
 * @param bd		Pointer to board information
 * @return 0 if ok, -1 on error (e.g. not enough space in fdt)
 */
static int ft_board_setup_tpm_resume(void *blob, bd_t *bd)
{
	static const char kernel_tpm_compat[] = "infineon,slb9635tt";
	static const char prop_name[] = "powered-while-suspended";
	int err, node, rev;

	/* Only apply fixup to MP machine */
	rev = board_get_revision();
	if (!(rev == 0 || rev == 3))
		return 0;

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
	if (ft_board_setup_gpios(blob, bd))
		return -1;
	return ft_board_setup_tpm_resume(blob, bd);
}

__weak int ft_board_setup(void *blob, bd_t *bd)
{
	return ft_system_setup(blob, bd);
}

int arch_early_init_r(void)
{
#ifdef CONFIG_CROS_EC
	if (board_init_cros_ec_devices(gd->fdt_blob)) {
                printf("%s: Failed to init EC\n", __func__);
                return 0;
        }
#endif

	return 0;
}
