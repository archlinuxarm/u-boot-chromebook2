/*
 *  Copyright (C) 2013 Samsung Electronics
 *  Alim Akhtar <alim.akhtar@samsung.com>
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
#include <i2c.h>
#include <power/pmic.h>
#include <power/s2mps11_pmic.h>
#include <errno.h>

DECLARE_GLOBAL_DATA_PTR;

/* Only three clocks are supported, bits d0..d2 */
#define S2MPS11_CLOCK_MASK 7
/* Jitter control bit in the RTC control register */
#define S2MPS11_RTC_CTRL_JIT (1 << 4)

int s2mps11_pmic_init(unsigned char bus)
{
	static const char name[] = S2MPS11_DEVICE_NAME;
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_OF_CONTROL
	const void *blob = gd->fdt_blob;
	int node, parent;

	node = fdtdec_next_compatible(blob, 0, COMPAT_SAMSUNG_S2MPS11_PMIC);
	if (node < 0) {
		debug("PMIC: No node for PMIC Chip in device tree\n");
		debug("node = %d\n", node);
		return -1;
	}

	parent = fdt_parent_offset(blob, node);
	if (parent < 0) {
		debug("%s: Cannot find node parent\n", __func__);
		return -1;
	}

	p->bus = i2c_get_bus_num_fdt(parent);
	if (p->bus < 0) {
		debug("%s: Cannot find I2C bus\n", __func__);
		return -1;
	}
	p->hw.i2c.addr = fdtdec_get_int(blob, node, "reg", 66);
	p->node = node;
#else
	p->bus = bus;
	p->hw.i2c.addr = S2MPS11_I2C_ADDR;
#endif

	p->name = name;
	p->interface = PMIC_I2C;
	p->number_of_regs = PMIC_NUM_OF_REGS;
	p->hw.i2c.tx_num = 1;

	puts("Board PMIC init\n");

	return 0;
}

#ifdef CONFIG_OF_CONTROL
int pmic_enable_clocks(struct pmic *ppmic)
{
	u32 reg;
	int clocks = fdtdec_get_int
		(gd->fdt_blob, ppmic->node, "u-boot-clocks", -1);

	if (clocks == -1)
		/* Not defined, leave at default. */
		return 0;

	/* Set clocks as required. */
	I2C_SET_BUS(ppmic->bus);
	if (pmic_reg_read(ppmic, S2MPS11_REG_RTC_CTRL, &reg)) {
		printf("%s: Failed to read rtc_ctrl!\n", __func__);
		return -1;
	}

	reg = (reg & ~S2MPS11_CLOCK_MASK) | (clocks & S2MPS11_CLOCK_MASK);

	/* Let's enable Jitter control unconditionally. */
	reg |= S2MPS11_RTC_CTRL_JIT;

	if (pmic_reg_write(ppmic, S2MPS11_REG_RTC_CTRL, reg)) {
		printf("%s: Failed to write rtc_ctrl!\n", __func__);
		return -1;
	}

	return 0;
}
#endif
