/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of per-board power management function */

#include <common.h>
#include <i2c.h>
#include <asm/arch/clock.h>

#include <cros/common.h>
#include <cros/power_management.h>

#define PMIC_I2C_BUS		0x00
#define PMIC_I2C_DEVICE_ADDRESS	0x34
#define TPS6586X_SUPPLYENA	0x10
#define TPS6586X_SUPPLYENB	0x11
#define TPS6586X_SUPPLYENC	0x12
#define TPS6586X_SUPPLYEND	0x13
#define TPS6586X_SUPPLYENE	0x14

int is_processor_reset(void)
{
	return clock_was_running() ? 0 : 1;
}

static int pmic_set_bit(int reg, int bit, int value)
{
	uint8_t byte;

	if (i2c_read(PMIC_I2C_DEVICE_ADDRESS, reg, 1, &byte, sizeof(byte))) {
		VBDEBUG("i2c_read fail: reg=%02x\n", reg);
		return 1;
	}

	if (value)
		byte |= 1 << bit;
	else
		byte &= ~(1 << bit);

	if (i2c_write(PMIC_I2C_DEVICE_ADDRESS, reg, 1, &byte, sizeof(byte))) {
		VBDEBUG("i2c_write fail: reg=%02x\n", reg);
		return 1;
	}

	return 0;
}

/* This function never returns */
void cold_reboot(void)
{
	if (i2c_set_bus_num(PMIC_I2C_BUS)) {
		VBDEBUG("i2c_set_bus_num fail\n");
		goto FATAL;
	}

	pmic_set_bit(TPS6586X_SUPPLYENE, 0, 1);

	/* Wait for 10 ms. If not rebootting, go to endless loop */
	udelay(10 * 1000);

FATAL:
	printf("Please press cold reboot button\n");
	while (1);
}

/* This function never returns */
void power_off(void)
{
	if (i2c_set_bus_num(PMIC_I2C_BUS)) {
		VBDEBUG("i2c_set_bus_num fail\n");
		goto FATAL;
	}

	/* Disable vdd_sm2 */
	pmic_set_bit(TPS6586X_SUPPLYENC, 7, 0);
	pmic_set_bit(TPS6586X_SUPPLYEND, 7, 0);

	/* Disable vdd_core */
	pmic_set_bit(TPS6586X_SUPPLYENA, 1, 0);
	pmic_set_bit(TPS6586X_SUPPLYENB, 1, 0);

	/* Disable vdd_cpu */
	pmic_set_bit(TPS6586X_SUPPLYENA, 0, 0);
	pmic_set_bit(TPS6586X_SUPPLYENB, 0, 0);

	/* Wait for 10 ms. If not powering off, go to endless loop */
	udelay(10 * 1000);

FATAL:
	printf("Please unplug the power cable and battery\n");
	while (1);
}
