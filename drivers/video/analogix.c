/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file is a driver for Analogix dP<->LVDS bridges. The original
 * submission is for the anx1120 chip.
 */
#include <common.h>
#include <i2c.h>
#include <fdtdec.h>
#include <errno.h>

/**
 * Write values table into the eDP bridge
 *
 * @return      0 on success, non-0 on failure
 */
static int anx1120_write_regs(void)
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

	for (i = 0; i < ARRAY_SIZE(init_anx1120); i++) {
		u8 value = init_anx1120[i].value;

		i2c_write(init_anx1120[i].addr, init_anx1120[i].reg, 1,
			  &value, 1);
	}

	return 0;
}

int analogix_init(const void *blob)
{
	int bus, old_bus;
	int parent;
	int node;
	int ret;

	node = fdtdec_next_compatible(blob, 0, COMPAT_ANALOGIX_ANX1120);
	if (node < 0)
		return -ENOENT;

	parent = fdt_parent_offset(blob, node);
	if (parent < 0) {
		debug("%s: Could not find parent i2c node\n", __func__);
		return -1;
	}

	/*
	 * We ignore the reg property, since we require that the device
	 * appears at slave addresses 0x29 and 0x47.
	 */
	bus = i2c_get_bus_num_fdt(parent);
	old_bus = i2c_get_bus_num();

	debug("%s: Using i2c bus %d\n", __func__, bus);

	i2c_set_bus_num(bus);
	ret = anx1120_write_regs();

	i2c_set_bus_num(old_bus);

	return ret;
}
