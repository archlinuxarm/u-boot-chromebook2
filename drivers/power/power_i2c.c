/*
 * Copyright (C) 2011 Samsung Electronics
 * Lukasz Majewski <l.majewski@samsung.com>
 *
 * (C) Copyright 2010
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de
 *
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
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
#include <linux/types.h>
#include <power/pmic.h>
#include <i2c.h>
#include <compiler.h>

static int pmic_select(struct pmic *p)
{
	int ret, old_bus;

	old_bus = i2c_get_bus_num();
	if (old_bus != p->bus) {
		debug("%s: Select bus %d\n", __func__, p->bus);
		ret = i2c_set_bus_num(p->bus);
		if (ret) {
			debug("%s: Cannot select pmic %s, err %d\n",
			      __func__, p->name, ret);
			return -1;
		}
	}

	return old_bus;
}

static int pmic_deselect(int old_bus)
{
	int ret;

	if (old_bus != i2c_get_bus_num()) {
		ret = i2c_set_bus_num(old_bus);
		debug("%s: Select bus %d\n", __func__, old_bus);
		if (ret) {
			debug("%s: Cannot restore i2c bus, err %d\n",
			      __func__, ret);
			return -1;
		}
	}

	return 0;
}

int pmic_reg_write(struct pmic *p, u32 reg, u32 val)
{
	unsigned char buf[4] = { 0 };
	int ret, old_bus;

	if (check_reg(p, reg))
		return -1;

	switch (pmic_i2c_tx_num) {
	case 3:
		if (p->sensor_byte_order == PMIC_SENSOR_BYTE_ORDER_BIG) {
			buf[2] = (cpu_to_le32(val) >> 16) & 0xff;
			buf[1] = (cpu_to_le32(val) >> 8) & 0xff;
			buf[0] = cpu_to_le32(val) & 0xff;
		} else {
			buf[0] = (cpu_to_le32(val) >> 16) & 0xff;
			buf[1] = (cpu_to_le32(val) >> 8) & 0xff;
			buf[2] = cpu_to_le32(val) & 0xff;
		}
		break;
	case 2:
		if (p->sensor_byte_order == PMIC_SENSOR_BYTE_ORDER_BIG) {
			buf[1] = (cpu_to_le32(val) >> 8) & 0xff;
			buf[0] = cpu_to_le32(val) & 0xff;
		} else {
			buf[0] = (cpu_to_le32(val) >> 8) & 0xff;
			buf[1] = cpu_to_le32(val) & 0xff;
		}
		break;
	case 1:
		buf[0] = cpu_to_le32(val) & 0xff;
		break;
	default:
		printf("%s: invalid tx_num: %d", __func__, pmic_i2c_tx_num);
		return -1;
	}

	old_bus = pmic_select(p);
	if (old_bus < 0)
		return -1;

	ret = i2c_write(pmic_i2c_addr, reg, 1, buf, pmic_i2c_tx_num);
	pmic_deselect(old_bus);
	return ret;
}

int pmic_reg_read(struct pmic *p, u32 reg, u32 *val)
{
	unsigned char buf[4] = { 0 };
	u32 ret_val = 0;
	int ret, old_bus;

	if (check_reg(p, reg))
		return -1;

	old_bus = pmic_select(p);
	if (old_bus < 0)
		return -1;

	ret = i2c_read(pmic_i2c_addr, reg, 1, buf, pmic_i2c_tx_num);
	pmic_deselect(old_bus);
	if (ret)
		return ret;

	switch (pmic_i2c_tx_num) {
	case 3:
		if (p->sensor_byte_order == PMIC_SENSOR_BYTE_ORDER_BIG)
			ret_val = le32_to_cpu(buf[2] << 16
					      | buf[1] << 8 | buf[0]);
		else
			ret_val = le32_to_cpu(buf[0] << 16 |
					      buf[1] << 8 | buf[2]);
		break;
	case 2:
		if (p->sensor_byte_order == PMIC_SENSOR_BYTE_ORDER_BIG)
			ret_val = le32_to_cpu(buf[1] << 8 | buf[0]);
		else
			ret_val = le32_to_cpu(buf[0] << 8 | buf[1]);
		break;
	case 1:
		ret_val = le32_to_cpu(buf[0]);
		break;
	default:
		printf("%s: invalid tx_num: %d", __func__, pmic_i2c_tx_num);
		return -1;
	}
	memcpy(val, &ret_val, sizeof(ret_val));

	return 0;
}

int pmic_reg_clear_bits_masked(struct pmic *p, int reg, uint bic, uint or)
{
	u32 val;
	int ret = 0;

	ret = pmic_reg_read(p, reg, &val);
	if (ret) {
		debug("%s: PMIC %d register read failed\n", __func__, reg);
		return -1;
	}
	val &= ~bic;
	val |= or;
	ret = pmic_reg_write(p, reg, val);
	if (ret) {
		debug("%s: PMIC %d register write failed\n", __func__, reg);
		return -1;
	}
	return 0;
}

int pmic_reg_update(struct pmic *p, int reg, uint regval)
{
	return pmic_reg_clear_bits_masked(p, reg, 0, regval);
}

int pmic_probe(struct pmic *p)
{
	int ret, old_bus;

	old_bus = pmic_select(p);
	if (old_bus < 0)
		return -1;
	debug("Bus: %d PMIC:%s probed!\n", p->bus, p->name);
	ret = i2c_probe(pmic_i2c_addr);
	pmic_deselect(old_bus);
	if (ret) {
		printf("Can't find PMIC:%s\n", p->name);
		return -1;
	}

	return 0;
}
