/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Configuration settings for the SAMSUNG EXYNOS5420 board.
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

#ifndef __CONFIG_SMDK5420_H
#define __CONFIG_SMDK5420_H

#include <configs/exynos5-dt.h>

/*
 * For now we have to hard-code some additional settings for Exynos5420.
 * Eventually these should move to the FDT.
 */
#include <configs/exynos5420.h>


#undef CONFIG_DEFAULT_DEVICE_TREE
#define CONFIG_DEFAULT_DEVICE_TREE	exynos5420-smdk5420

/* MACH_TYPE_SMDK5420 macro will be removed once added to mach-types */
#define MACH_TYPE_SMDK5420	8002
#define CONFIG_MACH_TYPE	MACH_TYPE_SMDK5420

/* select serial console configuration */
#define CONFIG_SERIAL2		/* use SERIAL 2 */

#define CONFIG_SYS_PROMPT	"SMDK5420 # "
#define CONFIG_IDENT_STRING	" for SMDK5420"

#endif	/* __CONFIG_SMDK5420_H */
