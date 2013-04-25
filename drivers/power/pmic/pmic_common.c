/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <common.h>
#include <fdtdec.h>
#include <errno.h>
#include <power/pmic.h>
#include <power/s2mps11_pmic.h>
#include <power/max77686_pmic.h>

DECLARE_GLOBAL_DATA_PTR;

static unsigned pmic_number_of_regs(enum fdt_compat_id pmic_compat)
{
	switch (pmic_compat) {
	case COMPAT_SAMSUNG_S2MPS11_PMIC:
		return S2MPS11_NUM_OF_REGS;
	default:
		break;
	}
	return 0;
}

int pmic_common_init(enum fdt_compat_id pmic_compat,
		     const struct pmic_init_ops *pmic_ops)
{
	const void *blob = gd->fdt_blob;
	struct pmic *p;
	int node, parent, ret;
	unsigned number_of_regs = pmic_number_of_regs(pmic_compat);
	const char *pmic_name, *comma;

	if (!number_of_regs) {
		printf("%s: %s - not a supported PMIC\n",
		       __func__, fdtdec_get_compatible(pmic_compat));
		return -1;
	}

	node = fdtdec_next_compatible(blob, 0, pmic_compat);
	if (node < 0) {
		debug("PMIC: Error %s. No node for %s in device tree\n",
		      fdt_strerror(node), fdtdec_get_compatible(pmic_compat));
		return node;
	}

	pmic_name = fdtdec_get_compatible(pmic_compat);
	comma = strchr(pmic_name, ',');
	if (comma)
		pmic_name = comma + 1;

	p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
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
	p->hw.i2c.addr = fdtdec_get_int(blob, node, "reg", 9);

	p->name = pmic_name;
	p->interface = PMIC_I2C;
	p->hw.i2c.tx_num = 1;
	p->number_of_regs = number_of_regs;
	p->compat_id = pmic_compat;

	ret = 0;
	while ((pmic_ops->reg_op != PMIC_REG_BAIL) && !ret) {
		if (pmic_ops->reg_op == PMIC_REG_WRITE)
			ret = pmic_reg_write(p,
					     pmic_ops->reg_addr,
					     pmic_ops->reg_value);
		else
			ret = pmic_reg_update(p,
					     pmic_ops->reg_addr,
					     pmic_ops->reg_value);
		pmic_ops++;
	}

	if (ret)
		printf("%s: Failed accessing reg 0x%x of %s\n",
		       __func__, pmic_ops[-1].reg_addr, p->name);
	else
		printf("PMIC %s initialized\n", p->name);
	return ret;
}
