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

	p->bus = bus;
	p->hw.i2c.addr = S2MPS11_I2C_ADDR;
	p->name = name;
	p->interface = PMIC_I2C;
	p->number_of_regs = S2MPS11_NUM_OF_REGS;
	p->hw.i2c.tx_num = 1;

	puts("Board PMIC init\n");

	return 0;
}
