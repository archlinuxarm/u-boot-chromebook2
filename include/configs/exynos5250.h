/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Common settings for Exynos5250 boards.
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

#ifndef __CONFIG_EXYNOS5250_H
#define __CONFIG_EXYNOS5250_H

#define CONFIG_EXYNOS5250

#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define CONFIG_SYS_TEXT_BASE		0x43E00000
#define CONFIG_SPL_TEXT_BASE		0x02023400
#define CONFIG_IRAM_TOP			0x02050000

#define CONFIG_MAX_I2C_NUM	8

/* This defines maximum number of channels available for dwmmc */
#define	DWMMC_MAX_CH_NUM		4
#define CONFIG_DEVICE_TREE_LIST "exynos5250-smdk5250" \
	" exynos5250-snow exynos5250-spring"

/* DRAM Memory Banks */
#define CONFIG_NR_DRAM_BANKS	4
#define SDRAM_BANK_SIZE		(512UL << 20)	/* 512 MB */

#endif
