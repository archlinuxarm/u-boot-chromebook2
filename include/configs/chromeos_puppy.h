/*
 * Copyright (c) 2010-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CHROMEOS_PUPPY_CONFIG_H
#define __CHROMEOS_PUPPY_CONFIG_H

#define CONFIG_EXTRA_BOOTARGS \
	"console=ttyS0,115200n8 " \
	"lp0_vec=0x00002000@0xbdffd000 " \
	"video=tegrafb " \
	"noinitrd usb_port_owner_info=0 " \
	"usbcore.old_scheme_first=1 " \
	"core_edp_mv=1120 core_edp_ma=4000 " \
	"no_console_suspend=1 " \
	"tegraid=35.1.1.0.0 " \
	"mem=1980M@2048M vpr=32M@4063M tsec=32M@4031M " \
	"otf_key=0 commchip_id=0 " \
	"console=tty1 " \
	"tegra_fbmem=18554880@0xad013000 " \
	"pmuboard=0x8028:0x81c8:0x31:0x80:0x3c " \
	"displayboard=0x065b:0x03e8:0x02:0x43:0x03 " \
	"display_panel=0 " \
	"audio_codec=rt5640 " \
	"board_info=64b:3e8:2:45:3 " \
	"debug_uartport=lsport,3"

/* Add chromeos specific only for non spl build */
#ifndef CONFIG_SPL_BUILD

#define CONFIG_DIRECT_BOOTARGS ""
#define CONFIG_STD_DEVICES_SETTINGS ""
#include <configs/chromeos.h>

#define CONFIG_PHYSMEM
#define CONFIG_CROS_EC
#define CONFIG_TPM
#define CONFIG_INFINEON_TPM_I2C

#endif

#include <configs/dalmore.h>

/* High-level configuration options */
#ifdef V_PROMPT
#undef V_PROMPT
#endif
#define V_PROMPT		"Tegra114 (Puppy) # "

#ifdef CONFIG_TEGRA_BOARD_STRING
#undef CONFIG_TEGRA_BOARD_STRING
#endif
#define CONFIG_TEGRA_BOARD_STRING	"NVIDIA Puppy"

#endif /* __CHROMEOS_PUPPY_CONFIG_H */
