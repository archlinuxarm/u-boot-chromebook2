/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __configs_chromeos_h__
#define __configs_chromeos_h__

/*
 * This config file defines platform-independent settings that a verified boot
 * firmware must have.
 */

/* Stringify a token */
#ifndef STRINGIFY
#define _STRINGIFY(x)	#x
#define STRINGIFY(x)	_STRINGIFY(x)
#endif

/* Enable verified boot */
#define CONFIG_CHROMEOS
#define CONFIG_CHROMEOS_TEST

/* Enable test codes */
#ifdef VBOOT_DEBUG
#define CONFIG_CHROMEOS_TEST
#endif /* VBOOT_DEBUG */

/* Enable graphics display */
#define CONFIG_LCD_BMP_RLE8
#define CONFIG_LZMA
#define CONFIG_SPLASH_SCREEN
#define CONFIG_CHROMEOS_DISPLAY

/* Enable USB, used for recovery mode */
#define CONFIG_CHROMEOS_USB

/* Support constant vboot flag from fdt */
#define CONFIG_CHROMEOS_CONST_FLAG

/*
 * Use the fdt to decide whether to load the environment early in start-up
 * (even before we decide if we're entering developer mode).
 */
#define CONFIG_OF_LOAD_ENVIRONMENT

/*
 * Enable this feature to embed crossystem data into device tree before booting
 * the kernel. We add quite a few things to the FDT, including a 16KB binary
 * blob.
 */
#define CONFIG_OF_BOARD_SETUP
#define CONFIG_SYS_FDT_PAD	0x8000

/* Make sure that the safe version of printf() is compiled in. */
#define CONFIG_SYS_VSNPRINTF

/*
 * This is the default kernel command line to a Chrome OS kernel. An ending
 * space character helps us concatenate more arguments.
 */
#ifndef CONFIG_BOOTARGS
#define CONFIG_BOOTARGS
#endif
#define CHROMEOS_BOOTARGS "cros_secure " CONFIG_BOOTARGS " "

/*******************************************************************************
 * Non-verified boot script                                                    *
 ******************************************************************************/

/*
 * Defines the regen_all variable, which is used by other commands
 * defined in this file.  Usage is to override one or more of the environment
 * variables and then run regen_all to regenerate the environment.
 *
 * Args from other scipts in this file:
 *   bootdev_bootargs: Filled in by other commands below based on the boot
 *       device.
 *
 * Args:
 *   common_bootargs: A copy of the default bootargs so we can run regen_all
 *       more than once.
 *   dev_extras: Placeholder space for developers to put their own boot args.
 *   extra_bootargs: Filled in by update_firmware_vars.py script in some cases.
 */
#define CONFIG_REGEN_ALL_SETTINGS \
	"common_bootargs=cros_legacy " CONFIG_DIRECT_BOOTARGS "\0" \
	\
	"dev_extras=\0" \
	"extra_bootargs=" \
		CONFIG_EXTRA_BOOTARGS "\0" \
	"bootdev_bootargs=\0" \
	\
	"regen_all=" \
		"setenv bootargs " \
			"${common_bootargs} " \
			"${dev_extras} " \
			"${extra_bootargs} " \
			"${bootdev_bootargs}\0"

/*
 * Defines ext2_boot and run_disk_boot_script.
 *
 * The run_disk_boot_script runs a u-boot script on the boot disk.  At the
 * moment this is used to allow the boot disk to choose a partion to boot from,
 * but could theoretically be used for more complicated things.
 *
 * The ext2_boot script boots from an ext2 device.
 *
 * Args from other scipts in this file:
 *   devtype: The device type we're booting from, like "usb" or "mmc"
 *   devnum: The device number (depends on devtype).  If we're booting from
 *       extranal MMC (for instance), this would be 1
 *   devname: The linux device name that will be assigned, like "sda" or
 *       mmcblk0p
 *
 * Args expected to be set by the u-boot script in /u-boot/boot.scr.uimg:
 *   rootpart: The root filesystem partion; we default to 3 in case there are
 *       problems reading the boot script.
 *   cros_bootfile: The name of the kernel in the root partition; we default to
 *       "/boot/vmlinux.uimg"
 *
 * Other args:
 *   script_part: The FAT partion we'll look for a boot script in.
 *   script_img: The name of the u-boot script.
 *
 * When we boot from an ext2 device, we will look at partion 12 (0x0c) to find
 * a u-boot script (as /u-boot/boot.scr.uimg).  That script is expected to
 * override "rootpart" and "cros_bootfile" as needed to select which partition
 * to boot from.
 *
 * USB download support:
 *
 * Once we have loaded the kernel from the selected device successfully,
 * we check whether a kernel has in fact been provided through the USB
 * download feature. In that case the kernaddr environment variable will
 * be set. It might seem strange that we load the original kernel and
 * then ignore it, but we try to load the kernel from a number of different
 * places. If the USB disk fails (because there is no disk inserted or
 * it is invalid) we don't want to pull in the kernaddr kernel and boot it
 * with USB as the root disk. So allow the normal boot failover to occur,
 * and only insert the kernaddr kernel when we actually have decided
 * what to boot from.
 */
#define CONFIG_EXT2_BOOT_HELPER_SETTINGS \
	"rootpart=3\0" \
	"cros_bootfile=/boot/vmlinux.uimg\0" \
	\
	"script_part=c\0" \
	"script_img=/u-boot/boot.scr.uimg\0" \
	\
	"run_disk_boot_script=" \
		"if fatload ${devtype} ${devnum}:${script_part} " \
				"${loadaddr} ${script_img}; then " \
			"source ${loadaddr}; " \
		"fi\0" \
	\
	"regen_ext2_bootargs=" \
		"setenv bootdev_bootargs " \
		"root=/dev/${devname}${rootpart} rootwait ro; " \
		"run regen_all\0" \
	\
	"ext2_boot=" \
		"run regen_ext2_bootargs; " \
		"if ext2load ${devtype} ${devnum}:${rootpart} " \
			"${loadaddr} ${cros_bootfile}; then " \
			"if test ${kernaddr} != \"\"; then "\
				"echo \"Using bundled kernel\"; "\
				"bootm ${kernaddr};" \
			"fi; "\
			"bootm ${loadaddr};" \
		"fi\0"

/*
 * Network-boot related settings.
 *
 * At the moment, we support:
 *   - initramfs factory install (tftp kernel with factory installer initramfs)
 *   - full network root booting (tftp kernel and initial ramdisk)
 *   - nfs booting (tftp kernel and point root to NFS)
 *
 * Network booting is enabled if you have an ethernet adapter plugged in at boot
 * and also have set tftpserverip/nfsserverip to something other than 0.0.0.0.
 * For full network booting or initramfs factory install you just need
 * tftpserverip. To choose full network booting over initramfs factory intsall,
 * you have to set has_initrd=1. For full NFS root you neet to set both
 * tftpserverip and nfsserverip.
 */
#define CONFIG_NETBOOT_SETTINGS \
	"tftpserverip=0.0.0.0\0" \
	"nfsserverip=0.0.0.0\0" \
	"has_initrd=0\0" \
	\
	"rootaddr=" STRINGIFY(CONFIG_INITRD_ADDRESS) "\0" \
	"initrd_high=0xffffffff\0" \
	\
	"regen_nfsroot_bootargs=" \
		"setenv bootdev_bootargs " \
			"dev=/dev/nfs4 rw nfsroot=${nfsserverip}:${rootpath} " \
			"ip=dhcp noinitrd; " \
		"run regen_all\0" \
	"regen_initrdroot_bootargs=" \
		"setenv bootdev_bootargs " \
			"rw root=/dev/ram0 ramdisk_size=512000 cros_netboot; " \
		"run regen_all\0" \
	"regen_initramfs_install_bootargs=" \
		"setenv bootdev_bootargs " \
			"lsm.module_locking=0 cros_netboot_ramfs " \
			"cros_factory_install cros_secure; " \
		"run regen_all\0" \
	\
	"tftp_setup=" \
		"setenv tftpkernelpath " \
			"/tftpboot/vmlinux.uimg; " \
		"setenv tftprootpath " \
			"/tftpboot/initrd.uimg; " \
		"setenv rootpath " \
			"/export/nfsroot; " \
		"setenv autoload n\0" \
	"initrdroot_boot=" \
		"run tftp_setup; " \
		"run regen_initrdroot_bootargs; " \
		"bootp; " \
		"if tftpboot ${rootaddr} ${tftpserverip}:${tftprootpath} && " \
		"   tftpboot ${loadaddr} ${tftpserverip}:${tftpkernelpath}; " \
		"then " \
			"bootm ${loadaddr} ${rootaddr}; " \
		"else " \
			"echo 'ERROR: Could not load root/kernel from TFTP'; " \
			"exit; " \
		"fi\0" \
	"initramfs_boot=" \
		"run tftp_setup; "\
		"run regen_initramfs_install_bootargs; "\
		"bootp; " \
		"if tftpboot ${loadaddr} ${tftpserverip}:${tftpkernelpath}; " \
		"then " \
			"bootm ${loadaddr}; "\
		"else " \
			"echo 'ERROR: Could not load kernel from TFTP'; " \
			"exit; " \
		"fi\0" \
	"tftp_ext2_boot=" \
		"run tftp_setup; " \
		"run regen_ext2_bootargs; " \
		"bootp; " \
		"if tftpboot ${loadaddr} ${tftpserverip}:${tftpkernelpath}; " \
		"then " \
			"bootm ${loadaddr}; " \
		"else " \
			"echo 'ERROR: Could not load kernel from TFTP'; " \
			"exit; " \
		"fi\0" \
	"nfsroot_boot=" \
		"run tftp_setup; " \
		"run regen_nfsroot_bootargs; " \
		"bootp; " \
		"if tftpboot ${loadaddr} ${tftpserverip}:${tftpkernelpath}; " \
		"then " \
			"bootm ${loadaddr}; " \
		"else " \
			"echo 'ERROR: Could not load kernel from TFTP'; " \
			"exit; " \
		"fi\0" \
	\
	"net_boot=" \
		"if test ${ethact} != \"\"; then " \
			"if test ${tftpserverip} != \"0.0.0.0\"; then " \
				"if test ${has_initrd} != \"0\"; then " \
					"run initrdroot_boot; " \
				"else " \
					"run initramfs_boot; " \
				"fi; " \
				"if test ${nfsserverip} != \"0.0.0.0\"; then " \
					"run nfsroot_boot; " \
				"fi; " \
			"fi; " \
		"fi\0" \

/*
 * Our full set of extra enviornment variables.
 *
 * A few notes:
 * - Right now, we can only boot from one USB device.  Need to fix this once
 *   usb works better.
 * - We define "non_verified_boot", which is the normal boot command unless
 *   it is overridden in the FDT.
 * - When we're running securely, the FDT will specify to call vboot_twostop
 *   directly.
 */

#define CONFIG_CHROMEOS_EXTRA_ENV_SETTINGS \
	CONFIG_STD_DEVICES_SETTINGS \
	CONFIG_REGEN_ALL_SETTINGS \
	CONFIG_EXT2_BOOT_HELPER_SETTINGS \
	CONFIG_NETBOOT_SETTINGS \
	\
	"usb_boot=setenv devtype usb; " \
		"setenv devnum 0; " \
		"setenv devname sda; " \
		"run run_disk_boot_script;" \
		"run ext2_boot\0" \
	\
	"mmc_setup=" \
		"mmc dev ${devnum}; " \
		"mmc rescan ${devnum}; " \
		"setenv devtype mmc; " \
		"setenv devname mmcblk${devnum}p\0" \
	"mmc_boot=" \
		"run mmc_setup; " \
		"run run_disk_boot_script;" \
		"run ext2_boot\0" \
	"mmc0_boot=setenv devnum 0; " \
		"run mmc_boot\0" \
	"mmc1_boot=setenv devnum 1; " \
		"run mmc_boot\0" \
	"mmc0_tftpboot=setenv devnum 0; " \
		"run mmc_setup; " \
		"run tftp_ext2_boot\0" \
	\
	"non_verified_boot=" \
		"usb start; " \
		"run net_boot; " \
		"run usb_boot; " \
		\
		"run mmc1_boot; " \
		"run mmc0_boot\0"

#define CONFIG_NON_VERIFIED_BOOTCOMMAND "run non_verified_boot"

#endif /* __configs_chromeos_h__ */
