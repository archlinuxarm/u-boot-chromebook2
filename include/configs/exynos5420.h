/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Common settings for Exynos5420 boards.
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

#ifndef __CONFIG_EXYNOS5420_H
#define __CONFIG_EXYNOS5420_H

#define CONFIG_EXYNOS5420

#define CONFIG_SYS_SDRAM_BASE		0x20000000
#define CONFIG_SYS_TEXT_BASE		0x23E00000
#ifdef CONFIG_VAR_SIZE_SPL
#define CONFIG_SPL_TEXT_BASE		0x02024410
#else
#define CONFIG_SPL_TEXT_BASE		0x02024400
#endif
#define CONFIG_IRAM_TOP			0x02074000

#define CONFIG_MAX_I2C_NUM      11

/* This defines maximum number of channels available for dwmmc */
#define	DWMMC_MAX_CH_NUM		3

#endif
