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

#ifndef __MAX77802_H_
#define __MAX77802_H_

enum {
	MAX77802_REG_PMIC_ID		= 0x0,
	MAX77802_REG_PMIC_INTSRC,
	MAX77802_REG_PMIC_INT1,
	MAX77802_REG_PMIC_INT2,
	MAX77802_REG_PMIC_INT1MSK,
	MAX77802_REG_PMIC_INT2MSK,

	MAX77802_REG_PMIC_STATUS1,
	MAX77802_REG_PMIC_STATUS2,

	MAX77802_REG_PMIC_PWRON,
	MAX77802_REG_PMIC_MRSTB		= 0xA,
	MAX77802_REG_PMIC_EPWRHOLD,
	MAX77802_REG_PMIC_BOOSTCTRL	= 0xE,
	MAX77802_REG_PMIC_BOOSTOUT,

	MAX77802_REG_PMIC_BUCK1CRTL	= 0x10,
	MAX77802_REG_PMIC_BUCK1DVS1,
	MAX77802_REG_PMIC_BUCK1DVS2,
	MAX77802_REG_PMIC_BUCK1DVS3,
	MAX77802_REG_PMIC_BUCK1DVS4,
	MAX77802_REG_PMIC_BUCK1DVS5,
	MAX77802_REG_PMIC_BUCK1DVS6,
	MAX77802_REG_PMIC_BUCK1DVS7,
	MAX77802_REG_PMIC_BUCK1DVS8,

	MAX77802_REG_PMIC_BUCK234FREQ,
	MAX77802_REG_PMIC_BUCK2CTRL1,
	MAX77802_REG_PMIC_BUCK2CTRL2,
	MAX77802_REG_PMIC_BUCK2PHTRAN,

	MAX77802_REG_PMIC_BUCK2DVS1,
	MAX77802_REG_PMIC_BUCK2DVS2,
	MAX77802_REG_PMIC_BUCK2DVS3,
	MAX77802_REG_PMIC_BUCK2DVS4,
	MAX77802_REG_PMIC_BUCK2DVS5,
	MAX77802_REG_PMIC_BUCK2DVS6,
	MAX77802_REG_PMIC_BUCK2DVS7,
	MAX77802_REG_PMIC_BUCK2DVS8,
	MAX77802_REG_PMIC_BUCK3CTRL1	= 0x27,
	MAX77802_REG_PMIC_BUCK3DVS1,
	MAX77802_REG_PMIC_BUCK3DVS2,
	MAX77802_REG_PMIC_BUCK3DVS3,
	MAX77802_REG_PMIC_BUCK3DVS4,
	MAX77802_REG_PMIC_BUCK3DVS5,
	MAX77802_REG_PMIC_BUCK3DVS6,
	MAX77802_REG_PMIC_BUCK3DVS7,
	MAX77802_REG_PMIC_BUCK3DVS8,
	MAX77802_REG_PMIC_BUCK4CTRL1	= 0x37,
	MAX77802_REG_PMIC_BUCK4DVS1,
	MAX77802_REG_PMIC_BUCK4DVS2,
	MAX77802_REG_PMIC_BUCK4DVS3,
	MAX77802_REG_PMIC_BUCK4DVS4,
	MAX77802_REG_PMIC_BUCK4DVS5,
	MAX77802_REG_PMIC_BUCK4DVS6,
	MAX77802_REG_PMIC_BUCK4DVS7,
	MAX77802_REG_PMIC_BUCK4DVS8,
	MAX77802_REG_PMIC_BUCK5CTRL1	= 0x40,
	MAX77802_REG_PMIC_BUCK5CTRL,
	MAX77802_REG_PMIC_BUCK5OUT,

	MAX77802_REG_PMIC_BUCK6CTRL	= 0x44,
	MAX77802_REG_PMIC_BUCK6DVS1,
	MAX77802_REG_PMIC_BUCK6DVS2,
	MAX77802_REG_PMIC_BUCK6DVS3,
	MAX77802_REG_PMIC_BUCK6DVS4,
	MAX77802_REG_PMIC_BUCK6DVS5,
	MAX77802_REG_PMIC_BUCK6DVS6,
	MAX77802_REG_PMIC_BUCK6DVS7,
	MAX77802_REG_PMIC_BUCK6DVS8,

	MAX77802_REG_PMIC_BUCK7CRTL	= 0x4E,
	MAX77802_REG_PMIC_BUCK7OUT,

	MAX77802_REG_PMIC_BUCK8CRTL	= 0x51,
	MAX77802_REG_PMIC_BUCK8OUT,

	MAX77802_REG_PMIC_BUCK9CRTL	= 0x54,
	MAX77802_REG_PMIC_BUCK9OUT,

	MAX77802_REG_PMIC_BUCK10CRTL	= 0x57,
	MAX77802_REG_PMIC_BUCK10OUT,

	MAX77802_REG_PMIC_LDO1CTRL1	= 0x60,
	MAX77802_REG_PMIC_LDO2CTRL1,
	MAX77802_REG_PMIC_LDO3CTRL1,
	MAX77802_REG_PMIC_LDO4CTRL1,
	MAX77802_REG_PMIC_LDO5CTRL1,
	MAX77802_REG_PMIC_LDO6CTRL1,
	MAX77802_REG_PMIC_LDO7CTRL1,
	MAX77802_REG_PMIC_LDO8CTRL1,
	MAX77802_REG_PMIC_LDO9CTRL1,
	MAX77802_REG_PMIC_LDO10CTRL1,
	MAX77802_REG_PMIC_LDO11CTRL1,
	MAX77802_REG_PMIC_LDO12CTRL1,
	MAX77802_REG_PMIC_LDO13CTRL1,
	MAX77802_REG_PMIC_LDO14CTRL1,
	MAX77802_REG_PMIC_LDO15CTRL1,

	MAX77802_REG_PMIC_LDO17CTRL1	= 0x70,
	MAX77802_REG_PMIC_LDO18CTRL1,
	MAX77802_REG_PMIC_LDO19CTRL1,
	MAX77802_REG_PMIC_LDO20CTRL1,
	MAX77802_REG_PMIC_LDO21CTRL1,

	MAX77802_REG_PMIC_LDO23CTRL1	= 0x76,
	MAX77802_REG_PMIC_LDO24CTRL1,
	MAX77802_REG_PMIC_LDO25CTRL1,
	MAX77802_REG_PMIC_LDO26CTRL1,
	MAX77802_REG_PMIC_LDO27CTRL1	= 0x7A,
	MAX77802_REG_PMIC_LDO28CTRL1,
	MAX77802_REG_PMIC_LDO29CTRL1,
	MAX77802_REG_PMIC_LDO30CTRL1,
	MAX77802_REG_PMIC_LDO32CTRL1	= 0x7F,
	MAX77802_REG_PMIC_LDO33CTRL1,
	MAX77802_REG_PMIC_LDO34CTRL1,
	MAX77802_REG_PMIC_LDO35CTRL1,

	MAX77802_REG_PMIC_LDO1CTRL2	= 0x90,
	MAX77802_REG_PMIC_LDO2CTRL2,
	MAX77802_REG_PMIC_LDO3CTRL2,
	MAX77802_REG_PMIC_LDO4CTRL2,
	MAX77802_REG_PMIC_LDO5CTRL2,
	MAX77802_REG_PMIC_LDO6CTRL2	= 0x95,
	MAX77802_REG_PMIC_LDO7CTRL2,
	MAX77802_REG_PMIC_LDO8CTRL2,
	MAX77802_REG_PMIC_LDO9CTRL2,
	MAX77802_REG_PMIC_LDO10CTRL2,
	MAX77802_REG_PMIC_LDO11CTRL2,
	MAX77802_REG_PMIC_LDO12CTRL2,
	MAX77802_REG_PMIC_LDO13CTRL2,
	MAX77802_REG_PMIC_LDO14CTRL2,
	MAX77802_REG_PMIC_LDO15CTRL2,

	MAX77802_REG_PMIC_LDO17CTRL2	= 0xA0,
	MAX77802_REG_PMIC_LDO18CTRL2,
	MAX77802_REG_PMIC_LDO19CTRL2,
	MAX77802_REG_PMIC_LDO20CTRL2,
	MAX77802_REG_PMIC_LDO21CTRL2,
	MAX77802_REG_PMIC_LDO22CTRL2,
	MAX77802_REG_PMIC_LDO23CTRL2,
	MAX77802_REG_PMIC_LDO24CTRL2,
	MAX77802_REG_PMIC_LDO25CTRL2,
	MAX77802_REG_PMIC_LDO26CTRL2,
	MAX77802_REG_PMIC_LDO27CTRL2	= 0xAA,
	MAX77802_REG_PMIC_LDO28CTRL2,
	MAX77802_REG_PMIC_LDO29CTRL2,
	MAX77802_REG_PMIC_LDO30CTRL2,
	MAX77802_REG_PMIC_LDO32CTRL2	= 0xAF,
	MAX77802_REG_PMIC_LDO33CTRL2	= 0xB0,
	MAX77802_REG_PMIC_LDO34CTRL2,
	MAX77802_REG_PMIC_LDO35CTRL2,

	MAX77802_REG_PMIC_BBAT		= 0xB4,
	MAX77802_REG_PMIC_32KHZ,

	MAX77802_REG_PMIC_RTCINT = 0xc0,
	MAX77802_REG_PMIC_RTCINTM,
	MAX77802_REG_PMIC_RTCCNTLM,
	MAX77802_REG_PMIC_RTCCNTL,
	MAX77802_REG_PMIC_RTCUPDATE0,
	/* Both of these values are in the same register. */
	MAX77802_REG_PMIC_RTCSMPL = 0xc6,
	MAX77802_REG_PMIC_RTCWTSR = 0xc6,

	MAX77802_REG_PMIC_RTCSEC,
	MAX77802_REG_PMIC_RTCMIN = 0xc8,
	MAX77802_REG_PMIC_RTCHOUR,
	MAX77802_REG_PMIC_RTCDAY,
	MAX77802_REG_PMIC_RTCMONTH,
	MAX77802_REG_PMIC_RTCYEAR,
	MAX77802_REG_PMIC_RTCDATE,
	MAX77802_REG_PMIC_RTCAE1,
	MAX77802_REG_PMIC_RTCSECA1,

	MAX77802_REG_PMIC_RTCMINA1 = 0xd0,
	MAX77802_REG_PMIC_RTCHOURA1,
	MAX77802_REG_PMIC_RTCDAYA1,
	MAX77802_REG_PMIC_RTCMONTHA1,
	MAX77802_REG_PMIC_RTCYEARA1,
	MAX77802_REG_PMIC_RTCDATEA1,
	MAX77802_REG_PMIC_RTCAE2,
	MAX77802_REG_PMIC_RTCSECA2,
	MAX77802_REG_PMIC_RTCMINA2 = 0xd8,
	MAX77802_REG_PMIC_RTCHOURA2,
	MAX77802_REG_PMIC_RTCDAYA2,
	MAX77802_REG_PMIC_RTCMONTHA2,
	MAX77802_REG_PMIC_RTCYEARA2,
	MAX77802_REG_PMIC_RTCDATEA2,

	MAX77802_NUM_OF_REGS,
};


/* Various fields of various RTC regusters */
enum {
	MAX77802_RTCCNTLM_BCDM = 1 << 0,
	MAX77802_RTCCNTLM_HRMODEM = 1 << 1
};

enum {
	MAX77802_RTCCNTL_BCD = 1 << 0,
	MAX77802_RTCCNTL_HRMODE = 1 << 1
};

enum {
	MAX77802_RTCUPDATE0_UDR = 1 << 0,
	MAX77802_RTCUPDATE0_FREEZE_SEC = 1 << 2,
	MAX77802_RTCUPDATE0_RTCWAKE = 1 << 3,
	MAX77802_RTCUPDATE0_RBUDR = 1 << 4
};

enum {
	MAX77802_RTCHOUR_AMPM = 1 << 6
};

/* I2C device address for pmic max77686 */
#define MAX77802_I2C_ADDR (0x12 >> 1)
#define MAX77802_BUS_NUM	4

enum {
	LDO_OFF = 0,
	LDO_ON,

	DIS_LDO = (0x00 << 6),
	EN_LDO = (0x3 << 6),
};

/* Buck1 1.0 volt value (P1.0V_AP_MIF) */
#define MAX77802_BUCK1DVS1_1V	0x3E
/* Buck2 1.0 volt value (P1.0V_VDD_ARM) */
#define MAX77802_BUCK2DVS1_1V	0x40
/* Buck2 1.2625 volt value (P1.2625V_VDD_ARM) */
#define MAX77802_BUCK2DVS1_1_2625V	0x6A
/* Buck3 1.0 volt value (P1.0V_VDD_INT) */
#define MAX77802_BUCK3DVS1_1V	0x40
/* Buck4 1.0 volt value (P1.0V_VDD_G3D) */
#define MAX77802_BUCK4DVS1_1V	0x40
/* Buck6 1.0 volt value (P1.0V_AP_KFC) */
#define MAX77802_BUCK6DVS1_1V	0x3E

/*
 * Different Bucks use different bits to control power. There are two types,
 * defined below.
 */
/* Type 1, works for BUCKs 1, 5, 6...10 */
#define MAX77802_BUCK_TYPE1_ON	(1 << 0)
#define MAX77802_BUCK_TYPE1_IGNORE_PWRREQ (1 << 1)

/* Type 2, works for BUCKs 2...4 */
#define MAX77802_BUCK_TYPE2_ON	(1 << 4)
#define MAX77802_BUCK_TYPE2_IGNORE_PWRREQ (1 << 5)

#define MAX77802_DEVICE_NAME "max77802-pmic"

/* LDO35 1.2 volt value for bridge ic */
#define MAX77802_LDO35CTRL1_1_2V (1 << 4)
#define MAX77802_LOD35CTRL1_ON	 (1 << 6)

/* Disable Boost Mode*/
#define MAX77802_BOOSTCTRL_OFF	0x09

/*
 * MAX77802_REG_PMIC_32KHZ set to 32KH CP
 * output is activated
 */
#define MAX77802_32KHCP_EN	(1 << 1)

/*
 * MAX77802_REG_PMIC_BBAT set to
 * Back up batery charger on and
 * limit voltage setting to 3.5v
 */
#define MAX77802_BBCHOSTEN	(1 << 0)
#define MAX77802_BBCVS_3_5V	(3 << 3)
#endif /* __MAX77802_PMIC_H_ */
