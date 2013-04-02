/*
 *  Copyright (C) 2012 Samsung Electronics
 *  Rajeshwari Shinde <rajeshwari.s@samsung.com>
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
#include <power/max77686_pmic.h>
#include <errno.h>

DECLARE_GLOBAL_DATA_PTR;

int pmic_init(unsigned char bus)
{
	static const char name[] = "MAX77686_PMIC";
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_OF_CONTROL
	const void *blob = gd->fdt_blob;
	int node, parent;
	int busnum;

	node = fdtdec_next_compatible(blob, 0, COMPAT_MAXIM_MAX77686_PMIC);
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

	busnum = i2c_get_bus_num_fdt(parent);
	if (busnum < 0) {
		debug("%s: Cannot find I2C bus\n", __func__);
		return -1;
	}
	p->bus = busnum;
	p->hw.i2c.addr = fdtdec_get_int(blob, node, "reg", 9);
#else
	p->bus = bus;
	p->hw.i2c.addr = MAX77686_I2C_ADDR;
#endif

	p->name = name;
	p->interface = PMIC_I2C;
	p->number_of_regs = PMIC_NUM_OF_REGS;
	p->hw.i2c.tx_num = 1;

	puts("Board PMIC init\n");

	return 0;
}

int pmic_set_voltage(u32 new_voltage)
{
	struct pmic *p;
	u32 read_data, volt_level, ret;

	p = pmic_get("MAX77686_PMIC");
	if (!p)
		return -ENODEV;

	/* Read BUCK2 DVS1 value */
	ret = pmic_reg_read(p, MAX77686_REG_PMIC_BUCK2DVS1, &read_data);
	if (ret != 0) {
		debug("CPUFREQ: max77686 BUCK2 DVS1 register read failed.\n");
		return -1;
	}

	/* Calculate voltage level */
	volt_level = new_voltage - MAX77686_BUCK2_VOL_MIN * 1000;

	if (volt_level < 0) {
		debug("CPUFREQ: Not a valid voltage level to set\n");
		return -1;
	}

	volt_level /= MAX77686_BUCK2_VOL_DIV;

	/* Update voltage level in BUCK2 DVS1 register value */
	clrsetbits_8(&read_data,
		     MAX77686_BUCK2_VOL_BITMASK << MAX77686_BUCK2_VOL_BITPOS,
		     volt_level << MAX77686_BUCK2_VOL_BITPOS);

	/* Write new value in BUCK2 DVS1 */
	ret = pmic_reg_write(p, MAX77686_REG_PMIC_BUCK2DVS1, read_data);
	if (ret != 0) {
		debug("CPUFREQ: max77686 BUCK2 DVS1 register write failed.\n");
		return -1;
	}

	/* Set ENABLE BUCK2 register bits */
	read_data = 0;
	ret = pmic_reg_read(p, MAX77686_BUCK2_VOL_ENADDR, &read_data);
	if (ret != 0) {
		debug("CPUFREQ: max77686 BUCK2 enable address read failed.\n");
		return -1;
	}

	clrsetbits_8(&read_data,
		     (MAX77686_BUCK2_VOL_ENBITMASK
		     << MAX77686_BUCK2_VOL_ENBITPOS),
		     (MAX77686_BUCK2_VOL_ENBITON
		     << MAX77686_BUCK2_VOL_ENBITPOS));

	ret = pmic_reg_write(p, MAX77686_BUCK2_VOL_ENADDR, read_data);
	if (ret != 0) {
		debug("CPUFREQ: max77686 BUCK2 enable address write failed.\n");
		return -1;
	}

	return 0;
}
