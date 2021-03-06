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

#ifndef __MAX77686_H_
#define __MAX77686_H_

enum {
	MAX77686_REG_PMIC_ID		= 0x0,
	MAX77686_REG_PMIC_INTSRC,
	MAX77686_REG_PMIC_INT1,
	MAX77686_REG_PMIC_INT2,
	MAX77686_REG_PMIC_INT1MSK,
	MAX77686_REG_PMIC_INT2MSK,

	MAX77686_REG_PMIC_STATUS1,
	MAX77686_REG_PMIC_STATUS2,

	MAX77686_REG_PMIC_PWRON,
	MAX77686_REG_PMIC_ONOFFDELAY,
	MAX77686_REG_PMIC_MRSTB,

	MAX77686_REG_PMIC_BUCK1CRTL	= 0x10,
	MAX77686_REG_PMIC_BUCK1OUT,
	MAX77686_REG_PMIC_BUCK2CTRL1,
	MAX77686_REG_PMIC_BUCK234FREQ,
	MAX77686_REG_PMIC_BUCK2DVS1,
	MAX77686_REG_PMIC_BUCK2DVS2,
	MAX77686_REG_PMIC_BUCK2DVS3,
	MAX77686_REG_PMIC_BUCK2DVS4,
	MAX77686_REG_PMIC_BUCK2DVS5,
	MAX77686_REG_PMIC_BUCK2DVS6,
	MAX77686_REG_PMIC_BUCK2DVS7,
	MAX77686_REG_PMIC_BUCK2DVS8,
	MAX77686_REG_PMIC_BUCK3CTRL,
	MAX77686_REG_PMIC_BUCK3DVS1,
	MAX77686_REG_PMIC_BUCK3DVS2,
	MAX77686_REG_PMIC_BUCK3DVS3,
	MAX77686_REG_PMIC_BUCK3DVS4,
	MAX77686_REG_PMIC_BUCK3DVS5,
	MAX77686_REG_PMIC_BUCK3DVS6,
	MAX77686_REG_PMIC_BUCK3DVS7,
	MAX77686_REG_PMIC_BUCK3DVS8,
	MAX77686_REG_PMIC_BUCK4CTRL1,
	MAX77686_REG_PMIC_BUCK4DVS1	= 0x28,
	MAX77686_REG_PMIC_BUCK4DVS2,
	MAX77686_REG_PMIC_BUCK4DVS3,
	MAX77686_REG_PMIC_BUCK4DVS4,
	MAX77686_REG_PMIC_BUCK4DVS5,
	MAX77686_REG_PMIC_BUCK4DVS6,
	MAX77686_REG_PMIC_BUCK4DVS7,
	MAX77686_REG_PMIC_BUCK4DVS8,
	MAX77686_REG_PMIC_BUCK5CTRL,
	MAX77686_REG_PMIC_BUCK5OUT,
	MAX77686_REG_PMIC_BUCK6CRTL,
	MAX77686_REG_PMIC_BUCK6OUT,
	MAX77686_REG_PMIC_BUCK7CRTL,
	MAX77686_REG_PMIC_BUCK7OUT,
	MAX77686_REG_PMIC_BUCK8CRTL,
	MAX77686_REG_PMIC_BUCK8OUT,
	MAX77686_REG_PMIC_BUCK9CRTL,
	MAX77686_REG_PMIC_BUCK9OUT,

	MAX77686_REG_PMIC_LDO1CTRL1	= 0x40,
	MAX77686_REG_PMIC_LDO2CTRL1,
	MAX77686_REG_PMIC_LDO3CTRL1,
	MAX77686_REG_PMIC_LDO4CTRL1,
	MAX77686_REG_PMIC_LDO5CTRL1,
	MAX77686_REG_PMIC_LDO6CTRL1,
	MAX77686_REG_PMIC_LDO7CTRL1,
	MAX77686_REG_PMIC_LDO8CTRL1,
	MAX77686_REG_PMIC_LDO9CTRL1,
	MAX77686_REG_PMIC_LDO10CTRL1,
	MAX77686_REG_PMIC_LDO11CTRL1,
	MAX77686_REG_PMIC_LDO12CTRL1,
	MAX77686_REG_PMIC_LDO13CTRL1,
	MAX77686_REG_PMIC_LDO14CTRL1,
	MAX77686_REG_PMIC_LDO15CTRL1,
	MAX77686_REG_PMIC_LDO16CTRL1,
	MAX77686_REG_PMIC_LDO17CTRL1,
	MAX77686_REG_PMIC_LDO18CTRL1,
	MAX77686_REG_PMIC_LDO19CTRL1,
	MAX77686_REG_PMIC_LDO20CTRL1,
	MAX77686_REG_PMIC_LDO21CTRL1,
	MAX77686_REG_PMIC_LDO22CTRL1,
	MAX77686_REG_PMIC_LDO23CTRL1,
	MAX77686_REG_PMIC_LDO24CTRL1,
	MAX77686_REG_PMIC_LDO25CTRL1,
	MAX77686_REG_PMIC_LDO26CTRL1,
	MAX77686_REG_PMIC_LDO1CTRL2,
	MAX77686_REG_PMIC_LDO2CTRL2,
	MAX77686_REG_PMIC_LDO3CTRL2,
	MAX77686_REG_PMIC_LDO4CTRL2,
	MAX77686_REG_PMIC_LDO5CTRL2,
	MAX77686_REG_PMIC_LDO6CTRL2,
	MAX77686_REG_PMIC_LDO7CTRL2,
	MAX77686_REG_PMIC_LDO8CTRL2,
	MAX77686_REG_PMIC_LDO9CTRL2,
	MAX77686_REG_PMIC_LDO10CTRL2,
	MAX77686_REG_PMIC_LDO11CTRL2,
	MAX77686_REG_PMIC_LDO12CTRL2,
	MAX77686_REG_PMIC_LDO13CTRL2,
	MAX77686_REG_PMIC_LDO14CTRL2,
	MAX77686_REG_PMIC_LDO15CTRL2,
	MAX77686_REG_PMIC_LDO16CTRL2,
	MAX77686_REG_PMIC_LDO17CTRL2,
	MAX77686_REG_PMIC_LDO18CTRL2,
	MAX77686_REG_PMIC_LDO19CTRL2,
	MAX77686_REG_PMIC_LDO20CTRL2,
	MAX77686_REG_PMIC_LDO21CTRL2,
	MAX77686_REG_PMIC_LDO22CTRL2,
	MAX77686_REG_PMIC_LDO23CTRL2,
	MAX77686_REG_PMIC_LDO24CTRL2,
	MAX77686_REG_PMIC_LDO25CTRL2,
	MAX77686_REG_PMIC_LDO26CTRL2,

	MAX77686_REG_PMIC_BBAT		= 0x7e,
	MAX77686_REG_PMIC_32KHZ,

	MAX77686_NUM_OF_REGS
};

/* I2C device address for pmic max77686 */
#define MAX77686_I2C_ADDR (0x12 >> 1)

/* Buck1 1 volt value */
#define MAX77686_BUCK1OUT_1V	0x5
#define MAX77686_BUCK1CTRL_EN	(3 << 0)
/* Buck2 1.3 volt value */
#define MAX77686_BUCK2DVS1_1_3V	0x38
#define MAX77686_BUCK2CTRL_ON	(1 << 4)
/* Buck3 1.0125 volt value */
#define MAX77686_BUCK3DVS1_1_0125V	0x21
#define MAX77686_BUCK3CTRL_ON	(1 << 4)
/* Buck4 1.2 volt value */
#define MAX77686_BUCK4DVS1_1_2V	0x30
#define MAX77686_BUCK4CTRL_ON	(1 << 4)
/* LDO2 1.5 volt value */
#define MAX77686_LD02CTRL1_1_5V	0x1c
/* LDO3 1.8 volt value */
#define MAX77686_LD03CTRL1_1_8V	0x14
/* LDO5 1.8 volt value */
#define MAX77686_LD05CTRL1_1_8V	0x14
/* LDO10 1.8 volt value */
#define MAX77686_LD10CTRL1_1_8V	0x14

/* BUCK2 voltage parameter values */
#define MAX77686_BUCK2_VOL_BITPOS	0x0
#define MAX77686_BUCK2_VOL_BITMASK	0xff
#define MAX77686_BUCK2_VOL_ENBITPOS	0x4
#define MAX77686_BUCK2_VOL_ENBITMASK	0x3
#define MAX77686_BUCK2_VOL_ENADDR	0x12
#define MAX77686_BUCK2_VOL_ENBITON	0x1
#define MAX77686_BUCK2_VOL_ENBITOFF	0x0
#define MAX77686_BUCK2_VOL_MIN		925
#define MAX77686_BUCK2_VOL_DIV		12500

/*
 * MAX77686_REG_PMIC_32KHZ set to 32KH CP
 * output is activated
 */
#define MAX77686_32KHCP_EN	(1 << 1)
#define MAX77686_32KHCP_LOW_JITTER (1 << 3)
/*
 * MAX77686_REG_PMIC_BBAT set to
 * Back up batery charger on and
 * limit voltage setting to 3.5v
 */
#define MAX77686_BBCHOSTEN	(1 << 0)
#define MAX77686_BBCVS_3_5V	(3 << 3)

#define MAX77686_EN_LDO		(0x3 << 6)
#endif /* __MAX77686_PMIC_H_ */
