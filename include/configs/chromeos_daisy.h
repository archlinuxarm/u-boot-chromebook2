/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Configuration settings for the SAMSUNG SMDK5250 board.
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

#ifndef __CONFIG_SMDK_H
#define __CONFIG_SMDK_H

#include <configs/exynos5250.h>
#include <configs/exynos5-dt.h>

#undef CONFIG_DEFAULT_DEVICE_TREE
#define CONFIG_DEFAULT_DEVICE_TREE	exynos5250-snow

/* Generally verified boot needs more heap space */
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN	(32 << 20)

#define CONFIG_INITRD_ADDRESS 0x44000000

#include <configs/chromeos.h>

#define CONFIG_CHROMEOS_USB

/* Support vboot flag reading from GPIO hardwrae */
#define CONFIG_CHROMEOS_GPIO_FLAG

/* Support vboot flag reading from EC */
#define CONFIG_CHROMEOS_CROS_EC_FLAG

/* Enable LCD connected to FIMD/DP display pipe */
#define CONFIG_LCD
#define CONFIG_EXYNOS_FB
#define CONFIG_EXYNOS_DP
#define CONFIG_CMD_BMP

/* Adjust the display resolution. */
#undef MAIN_VRESOL_VAL
#undef MAIN_HRESOL_VAL
#define MAIN_VRESOL_VAL 0x300
#define MAIN_HRESOL_VAL 0x556
#undef LCD_XRES
#undef LCD_YRES
#define LCD_XRES 1366
#define LCD_YRES 768
#define LCD_BPP LCD_COLOR16
#define CONFIG_SYS_WHITE_ON_BLACK

#define CONFIG_SERIAL3

#define CONFIG_SYS_PROMPT	"SMDK5250 # "

#endif	/* __CONFIG_SMDK_H */
