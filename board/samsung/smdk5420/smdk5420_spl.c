/*
 * Copyright (c) 2012 The Chromium OS Authors.
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
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>
#include <asm/arch/power.h>
#include <asm/arch/spl.h>
#include <asm/arch/clk.h>

#define SIGNATURE	0xdeadbeef

/* Parameters of early board initialization in SPL */
static struct spl_machine_param machine_param
		__attribute__((section(".machine_param"))) = {
	.signature	= SIGNATURE,
	.version	= 1,
	.params		= SPL_PARAM_STRING,
	.size		= sizeof(machine_param),

	.mem_iv_size	= 0x1f,
	.mem_type	= DDR_MODE_DDR3,

	/*
	 * Set uboot_size to 512KB bytes.
	 *
	 * This is an overly conservative value chosen to accommodate all
	 * possible U-Boot image.  You are advised to set this value to a
	 * smaller realistic size via scripts that modifies the .machine_param
	 * section of output U-Boot image.
	 */
	.uboot_size	= 512 << 10,

	.boot_source	= BOOT_MODE_OM,
	.frequency_mhz	= 800,
	.arm_freq_mhz	= 800,
	.serial_base	= 0x12c30000,
	.i2c_base	= 0x12c60000,
	.board_rev_gpios = { 0xffffffff, 0xffffffff },
	.mem_manuf	= MEM_MANUF_SAMSUNG,
	.bad_wake_gpio	= 0xffffffff,
	.write_protect_gpio = 0xffffffff,
};

struct spl_machine_param *spl_get_machine_params(void)
{
	if (machine_param.signature != SIGNATURE) {
		/* Will hang if SIGNATURE dont match */
		while (1)
			;
	}

	return &machine_param;
}

/*
 * We place this in the upper word of INFORM3 just to make it clearer that
 * we're using the register, since there's no real central repository of users.
 *
 * It is not strictly required.
 */
#define INFORM3_BOARD_REV_HINT	0xc0de0000

void board_get_full_revision(int *board_rev_out, int *subrev_out)
{
	struct exynos5_power *power =
	  (struct exynos5_power *)samsung_get_base_power();
	int board_rev = -1;
	int subrev = 0;
	int count;
	struct spl_machine_param *params = spl_get_machine_params();
	unsigned gpio_list[4];
	uint32_t reset_status;

	/*
	 * Use the cached value if we're waking up from sleep.  That's important
	 * when resuming from sleep where we can't reconfigure GPIOs.
	 */
	reset_status = get_reset_status();
	if (reset_status == S5P_CHECK_SLEEP ||
	    reset_status == S5P_CHECK_DIDLE ||
	    reset_status == S5P_CHECK_LPA) {
		if (board_rev_out) {
			*board_rev_out = (power->inform3 >> 8) & 0xff;
			if (*board_rev_out == 0xff)
				*board_rev_out = -1;
		}
		if (subrev_out)
			*subrev_out = power->inform3 & 0xff;
		return;
	}

	/*
	 * Translate the machine params into something gpio_read_strappings()
	 * can handle.  Assume that any unused gpios are set to 0xffff and that
	 * gpios are filled in starting in position 0.
	 */
	gpio_list[0] = params->board_rev_gpios[0] & 0xffff;
	gpio_list[1] = params->board_rev_gpios[0] >> 16;
	gpio_list[2] = params->board_rev_gpios[1] & 0xffff;
	gpio_list[3] = params->board_rev_gpios[1] >> 16;
	for (count = ARRAY_SIZE(gpio_list); count > 0; count--) {
		if (gpio_list[count-1] != 0xffff)
			break;
	}

	if (count) {
		board_rev = gpio_read_strappings(gpio_list, count);

		if (params->map_offset) {
			const u8 *map = ((u8 *)params) + params->map_offset;

			subrev = map[(board_rev * 2) + 1];
			board_rev = map[board_rev * 2];
		}
	}

	/* Cache the value for later suspend/resume */
	power->inform3 = INFORM3_BOARD_REV_HINT |
		(((u8)board_rev) << 8) | subrev;

	if (board_rev_out)
		*board_rev_out = board_rev;
	if (subrev_out)
		*subrev_out = subrev;
}

phys_size_t board_get_memory_size(void)
{
	int subrev;
	board_get_full_revision(NULL, &subrev);

	/*
	 * Two memory sizes are supported, 2GB and 3.5GB, as denoted by bit 1
	 * in subrevision field.
	 */
	if (subrev & (1 << 1))
		return 0xe0000000;

	return 0x80000000;
}
