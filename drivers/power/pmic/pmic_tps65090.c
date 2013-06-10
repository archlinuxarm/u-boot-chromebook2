/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <errno.h>
#include <fdtdec.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/tps65090_pmic.h>

DECLARE_GLOBAL_DATA_PTR;

#define TPS65090_NAME "TPS65090_PMIC"

/* TPS65090 register addresses */
enum {
	REG_CG_CTRL0 = 4,
	REG_CG_STATUS1 = 0xa,
	REG_FET1_CTRL = 0x0f,
	REG_FET2_CTRL,
	REG_FET3_CTRL,
	REG_FET4_CTRL,
	REG_FET5_CTRL,
	REG_FET6_CTRL,
	REG_FET7_CTRL,
	TPS65090_NUM_REGS,
};

enum {
	CG_CTRL0_ENC_MASK	= 0x01,

	MAX_FET_NUM	= 7,
	MAX_CTRL_READ_TRIES = 5,

	/* TPS65090 FET_CTRL register values */
	FET_CTRL_TOFET		= 1 << 7,  /* Timeout, startup, overload */
	FET_CTRL_PGFET		= 1 << 4,  /* Power good for FET status */
	FET_CTRL_WAIT		= 3 << 2,  /* Overcurrent timeout max */
	FET_CTRL_ADENFET	= 1 << 1,  /* Enable output auto discharge */
	FET_CTRL_ENFET		= 1 << 0,  /* Enable FET */
};

/**
 * Checks for a valid FET number
 *
 * @param fet_id	FET number to check
 * @return 0 if ok, -1 if FET value is out of range
 */
static int tps65090_check_fet(unsigned int fet_id)
{
	if (fet_id == 0 || fet_id > MAX_FET_NUM) {
		debug("parameter fet_id is out of range, %u not in 1 ~ %u\n",
				fet_id, MAX_FET_NUM);
		return -1;
	}

	return 0;
}

/**
 * Set the power state for a FET
 *
 * @param pmic		pmic structure for the tps65090
 * @param fet_id	Fet number to set (1..MAX_FET_NUM)
 * @param set		1 to power on FET, 0 to power off
 * @return FET_ERR_COMMS if we got a comms error, FET_ERR_NOT_READY if the
 * FET failed to change state. If all is ok, returns 0.
 */
static int tps65090_fet_set(struct pmic *pmic, int fet_id, int set)
{
	int retry;
	u32 reg, value;

	value = FET_CTRL_ADENFET | FET_CTRL_WAIT;
	if (set)
		value |= FET_CTRL_ENFET;

	if (pmic_reg_write(pmic, REG_FET1_CTRL + fet_id - 1, value))
		return FET_ERR_COMMS;
	/* Try reading until we get a result */
	for (retry = 0; retry < MAX_CTRL_READ_TRIES; retry++) {
		if (pmic_reg_read(pmic, REG_FET1_CTRL + fet_id - 1, &reg))
			return FET_ERR_COMMS;

		/* Check that the fet went into the expected state */
		if (!!(reg & FET_CTRL_PGFET) == set)
			return 0;

		/* If we got a timeout, there is no point in waiting longer */
		if (reg & FET_CTRL_TOFET)
			break;

		mdelay(1);
	}

	debug("FET %d: Power good should have set to %d but reg=%#02x\n",
	      fet_id, set, reg);
	return FET_ERR_NOT_READY;
}

int tps65090_fet_enable(unsigned int fet_id)
{
	int loops;
	ulong start;
	struct pmic *pmic;
	int ret = 0;

	if (tps65090_check_fet(fet_id))
		return -1;

	pmic = pmic_get(TPS65090_NAME);
	if (pmic == NULL)
		return -1;

	start = get_timer(0);
	for (loops = 0; ; loops++) {
		ret = tps65090_fet_set(pmic, fet_id, 1);
		if (!ret)
			break;

		if (get_timer(start) > 100)
			break;

		/* Turn it off and try again until we time out */
		tps65090_fet_set(pmic, fet_id, 0);
	}

	if (ret) {
		debug("%s: FET%d failed to power on: time=%lums, loops=%d\n",
	      __func__, fet_id, get_timer(start), loops);
	} else if (loops) {
		debug("%s: FET%d powered on after %lums, loops=%d\n",
		      __func__, fet_id, get_timer(start), loops);
	}
	/*
	 * Unfortunately, there are some conditions where the power
	 * good bit will be 0, but the fet still comes up. One such
	 * case occurs with the lcd backlight. We'll just return 0 here
	 * and assume that the fet will eventually come up.
	 */
	if (ret == FET_ERR_NOT_READY)
		ret = 0;

	return ret;
}

int tps65090_fet_disable(unsigned int fet_id)
{
	int ret;
	struct pmic *pmic;

	if (tps65090_check_fet(fet_id))
		return -1;

	pmic = pmic_get(TPS65090_NAME);
	if (pmic == NULL)
		return -1;
	ret = tps65090_fet_set(pmic, fet_id, 0);

	return ret;
}

int tps65090_fet_is_enabled(unsigned int fet_id)
{
	u32 reg;
	int ret;
	struct pmic *pmic;

	if (tps65090_check_fet(fet_id))
		return -1;
	pmic = pmic_get(TPS65090_NAME);
	if (pmic == NULL)
		return -1;
	ret = pmic_reg_read(pmic, REG_FET1_CTRL + fet_id - 1, &reg);
	if (ret) {
		debug("fail to read FET%u_CTRL register over I2C", fet_id);
		return -2;
	}

	return reg & FET_CTRL_ENFET;
}

int tps65090_get_charging(void)
{
	u32 val;
	int ret;
	struct pmic *pmic;

	pmic = pmic_get(TPS65090_NAME);
	if (pmic == NULL)
		return -1;
	ret = pmic_reg_read(pmic, REG_CG_CTRL0, &val);
	if (ret)
		return ret;
	return val & CG_CTRL0_ENC_MASK ? 1 : 0;
}

int tps65090_set_charge_enable(int enable)
{
	u32 val;
	int ret;
	struct pmic *pmic;

	pmic = pmic_get(TPS65090_NAME);
	if (pmic == NULL)
		return -1;

	ret = pmic_reg_read(pmic, REG_CG_CTRL0, &val);
	if (!ret) {
		if (enable)
			val |= CG_CTRL0_ENC_MASK;
		else
			val &= ~CG_CTRL0_ENC_MASK;
		ret = pmic_reg_write(pmic, REG_CG_CTRL0, val);
	}
	if (ret) {
		debug("%s: Failed to read/write register\n", __func__);
		return ret;
	}
	return 0;
}

int tps65090_get_status(void)
{
	u32 val;
	int ret;
	struct pmic *pmic;

	pmic = pmic_get(TPS65090_NAME);

	if (pmic == NULL)
		return -1;
	ret = pmic_reg_read(pmic, REG_CG_STATUS1, &val);
	if (ret)
		return ret;
	return val;
}

int tps65090_init(void)
{
	int bus;
	int addr;
	struct pmic *p;

#ifdef CONFIG_OF_CONTROL
	const void *blob = gd->fdt_blob;
	int node, parent;

	node = fdtdec_next_compatible(blob, 0, COMPAT_TI_TPS65090);
	if (node < 0) {
		debug("PMIC: No node for PMIC Chip in device tree\n");
		debug("node = %d\n", node);
		return -ENODEV;
	}

	parent = fdt_parent_offset(blob, node);
	if (parent < 0) {
		debug("%s: Cannot find node parent\n", __func__);
		return -1;
	}

	bus = i2c_get_bus_num_fdt(parent);
	if (bus < 0) {
		debug("%s: Cannot find I2C bus\n", __func__);
		return -ENODEV;
	}
	addr = fdtdec_get_int(blob, node, "reg", TPS65090_I2C_ADDR);
#else
	bus = CONFIG_POWER_TPS65090_BUS;
	addr = TPS65090_I2C_ADDR;
#endif
	p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

	p->name = TPS65090_NAME;
	p->bus = bus;
	p->interface = PMIC_I2C;
	p->number_of_regs = TPS65090_NUM_REGS;
	p->hw.i2c.addr = addr;
	p->hw.i2c.tx_num = 1;

	puts("TPS65090 PMIC init\n");

	return 0;
}
