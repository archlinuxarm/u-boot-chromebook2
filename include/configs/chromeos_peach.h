/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Configuration settings for the SAMSUNG Peach board.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_CHROMEOS_PEACH_H
#define __CONFIG_CHROMEOS_PEACH_H

#define CONFIG_CROS_EC

/*
 * For now we have to hard-code some additional settings for Exynos5420.
 * Eventually these should move to the FDT.
 */
#include <configs/exynos5420.h>

#include <configs/exynos5-dt.h>

#include <configs/chromeos.h>

#define CONFIG_CHROMEOS_PEACH

#undef CONFIG_DEFAULT_DEVICE_TREE
#define CONFIG_DEFAULT_DEVICE_TREE	exynos5420-peach-pit
#define CONFIG_STD_DEVICES_SETTINGS    EXYNOS_DEVICE_SETTINGS

#define CONFIG_SYS_PROMPT	"Peach # "
#define CONFIG_IDENT_STRING	" for Peach"

/*
 * Extra bootargs used for direct booting, but not for vboot.
 * - console of the board
 * - debug and earlyprintk: easier to debug; they could be removed later
 */
#define CONFIG_DIRECT_BOOTARGS \
	"console=ttySAC3," STRINGIFY(CONFIG_BAUDRATE) " debug earlyprintk"

#define CONFIG_EXTRA_BOOTARGS ""

#define ANX1120_I2C_BUS		7
#define CONFIG_RTC_MAX77802
#define CONFIG_CMD_DATE

#define CONFIG_LCD
#define RPLL_MDIV		0x5E
#define RPLL_PDIV		0x2
#define RPLL_SDIV		0x4
#define CONFIG_EXYNOS_FB
#define CONFIG_EXYNOS_DP
#define LCD_BPP			LCD_COLOR16

/* Actual number of RAM banks is determined at run time */
#define CONFIG_RUN_TIME_BANK_NUMBER
#define CONFIG_CHROMEOS_GPIO_FLAG
#define CONFIG_CHROMEOS_CROS_EC_FLAG

#define CONFIG_EXYNOS_FAST_SPI_BOOT

/* Replace default CONFIG_EXTRA_ENV_SETTINGS */
#ifdef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_EXTRA_ENV_SETTINGS
#endif
#define CONFIG_EXTRA_ENV_SETTINGS \
	EXYNOS_DEVICE_SETTINGS \
	CONFIG_CHROMEOS_EXTRA_ENV_SETTINGS

/* Replace default CONFIG_BOOTCOMMAND */
#ifdef CONFIG_BOOTCOMMAND
#undef CONFIG_BOOTCOMMAND
#endif
#define CONFIG_BOOTCOMMAND CONFIG_NON_VERIFIED_BOOTCOMMAND

#define CONFIG_CROS_EC_SPI		/* Support CROS_EC over SPI */

#endif	/* __CONFIG_CHROMEOS_PEACH_H */
