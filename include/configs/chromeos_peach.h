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
#define CONFIG_SPL_ELOG_SUPPORT
#define CONFIG_ELOG_SIZE 0x4000
#define CONFIG_ELOG_OFFSET (0x00400000 - CONFIG_ENV_SIZE - CONFIG_ELOG_SIZE)

#define CONFIG_LCD
#define RPLL_MDIV		0x5E
#define RPLL_PDIV		0x2
#define RPLL_SDIV		0x3
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

#ifndef CONFIG_SPL_BUILD
#define CONFIG_SKIP_LOWLEVEL_INIT
#endif

#ifndef CONFIG_FACTORY_IMAGE
#undef CONFIG_BOOTM_NETBSD
#undef CONFIG_BOOTM_RTEMS
#undef CONFIG_CHROMEOS_TEST
#undef CONFIG_CMDLINE_EDITING
#undef CONFIG_CMD_BDI
#undef CONFIG_CMD_BOOTD
#undef CONFIG_CMD_CACHE
#undef CONFIG_CMD_CRC32
#undef CONFIG_CMD_CROS_EC
#undef CONFIG_CMD_DATE
#undef CONFIG_CMD_DHCP
#undef CONFIG_CMD_DTT
#undef CONFIG_CMD_ECHO
#undef CONFIG_CMD_EDITENV
#undef CONFIG_CMD_ELF
#undef CONFIG_CMD_EXPORTENV
#undef CONFIG_CMD_EXT2
#undef CONFIG_CMD_FAT
#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_GPIO
#undef CONFIG_CMD_HASH
#undef CONFIG_CMD_IMI
#undef CONFIG_CMD_IMPORTENV
#undef CONFIG_CMD_ITEST
#undef CONFIG_CMD_LOADB
#undef CONFIG_CMD_LOADS
#undef CONFIG_CMD_MEMORY
#undef CONFIG_CMD_MISC
#undef CONFIG_CMD_MMC
#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS
#undef CONFIG_CMD_PART
#undef CONFIG_CMD_PING
#undef CONFIG_CMD_PXE
#undef CONFIG_CMD_SAVEENV
#undef CONFIG_CMD_SETGETDCR
#undef CONFIG_CMD_SF
#undef CONFIG_CMD_SF_TEST
#undef CONFIG_CMD_SOUND
#undef CONFIG_CMD_SOURCE
#undef CONFIG_CMD_SPI
#undef CONFIG_CMD_TIME
#undef CONFIG_CMD_TPM
#undef CONFIG_CMD_XIMG
#undef CONFIG_DOS_PARTITION
#undef CONFIG_FS_FAT
#undef CONFIG_GZIP
#undef CONFIG_I2C_EDID
#undef CONFIG_SDHCI
#undef CONFIG_SYS_HUSH_PARSER
#undef CONFIG_SYS_LONGHELP
#undef CONFIG_USB_ETHER_ASIX
#undef CONFIG_USB_ETHER_SMSC95XX
#undef CONFIG_USB_HOST_ETHER
#endif

#endif	/* __CONFIG_CHROMEOS_PEACH_H */
