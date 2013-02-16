/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <part.h>
#include <linux/compiler.h>
#include <cros/boot_kernel.h>
#include <cros/common.h>
#include <cros/crossystem_data.h>
#ifdef CONFIG_X86
#include <i8042.h>
#include <asm/zimage.h>
#endif

#include <vboot_api.h>

enum { CROS_32BIT_ENTRY_ADDR = 0x100000 };

/*
 * We uses a static variable to communicate with ft_board_setup().
 * For more information, please see commit log.
 */
static crossystem_data_t *g_crossystem_data = NULL;

/* defined in common/cmd_bootm.c */
int do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

/* Maximum kernel command-line size */
#define CROS_CONFIG_SIZE	4096

/* Size of the x86 zeropage table */
#define CROS_PARAMS_SIZE	4096

/* Extra buffer to string replacement */
#define EXTRA_BUFFER		4096

#if !defined(CONFIG_SANDBOX)
/**
 * This loads kernel command line from the buffer that holds the loaded kernel
 * image. This function calculates the address of the command line from the
 * bootloader address.
 *
 * @param kernel_buffer		Address of kernel buffer in memory
 * @param bootloader_offset	Offset of bootloader in kernel_buffer
 * @return kernel config address
 */
static char *get_kernel_config(void *kernel_buffer, size_t bootloader_offset)
{
	/* Use the bootloader address to find the kernel config location. */
	return kernel_buffer + bootloader_offset -
		(CROS_PARAMS_SIZE + CROS_CONFIG_SIZE);
}

static uint32_t get_dev_num(const block_dev_desc_t *dev)
{
	return dev->dev;
}

/* assert(0 <= val && val < 99); sprintf(dst, "%u", val); */
static char *itoa(char *dst, int val)
{
	if (val > 9)
		*dst++ = '0' + val / 10;
	*dst++ = '0' + val % 10;
	return dst;
}

/* copied from x86 bootstub code; sprintf(dst, "%02x", val) */
static void one_byte(char *dst, uint8_t val)
{
	dst[0] = "0123456789abcdef"[(val >> 4) & 0x0F];
	dst[1] = "0123456789abcdef"[val & 0x0F];
}

/* copied from x86 bootstub code; display a GUID in canonical form */
static char *emit_guid(char *dst, uint8_t *guid)
{
	one_byte(dst, guid[3]); dst += 2;
	one_byte(dst, guid[2]); dst += 2;
	one_byte(dst, guid[1]); dst += 2;
	one_byte(dst, guid[0]); dst += 2;
	*dst++ = '-';
	one_byte(dst, guid[5]); dst += 2;
	one_byte(dst, guid[4]); dst += 2;
	*dst++ = '-';
	one_byte(dst, guid[7]); dst += 2;
	one_byte(dst, guid[6]); dst += 2;
	*dst++ = '-';
	one_byte(dst, guid[8]); dst += 2;
	one_byte(dst, guid[9]); dst += 2;
	*dst++ = '-';
	one_byte(dst, guid[10]); dst += 2;
	one_byte(dst, guid[11]); dst += 2;
	one_byte(dst, guid[12]); dst += 2;
	one_byte(dst, guid[13]); dst += 2;
	one_byte(dst, guid[14]); dst += 2;
	one_byte(dst, guid[15]); dst += 2;
	return dst;
}

/**
 * This replaces:
 *   %D -> device number
 *   %P -> partition number
 *   %U -> GUID
 * in kernel command line.
 *
 * For example:
 *   ("root=/dev/sd%D%P", 2, 3)      -> "root=/dev/sdc3"
 *   ("root=/dev/mmcblk%Dp%P", 0, 5) -> "root=/dev/mmcblk0p5".
 *
 * @param src		Input string
 * @param devnum	Device number of the storage device we will mount
 * @param partnum	Partition number of the root file system we will mount
 * @param guid		GUID of the kernel partition
 * @param dst		Output string
 * @param dst_size	Size of output string
 * @return zero if it succeeds, non-zero if it fails
 */
static int update_cmdline(char *src, int devnum, int partnum, uint8_t *guid,
		char *dst, int dst_size)
{
	char *dst_end = dst + dst_size;
	int c;

	/* sanity check on inputs */
	if (devnum < 0 || devnum > 25 || partnum < 1 || partnum > 99 ||
			dst_size < 0 || dst_size > 10000) {
		VBDEBUG("insane input: %d, %d, %d\n", devnum, partnum,
				dst_size);
		return 1;
	}

	/*
	 * Condition "dst + X <= dst_end" checks if there is at least X bytes
	 * left in dst. We use X > 1 so that there is always 1 byte for '\0'
	 * after the loop.
	 *
	 * We constantly estimate how many bytes we are going to write to dst
	 * for copying characters from src or for the string replacements, and
	 * check if there is sufficient space.
	 */

#define CHECK_SPACE(bytes) \
	if (!(dst + (bytes) <= dst_end)) { \
		VBDEBUG("fail: need at least %d bytes\n", (bytes)); \
		return 1; \
	}

	while ((c = *src++)) {
		if (c != '%') {
			CHECK_SPACE(2);
			*dst++ = c;
			continue;
		}

		switch ((c = *src++)) {
		case '\0':
			VBDEBUG("mal-formed input: end in '%%'\n");
			return 1;
		case 'D':
			/*
			 * TODO: Do we have any better way to know whether %D
			 * is replaced by a letter or digits? So far, this is
			 * done by a rule of thumb that if %D is followed by a
			 * 'p' character, then it is replaced by digits.
			 */
			if (*src == 'p') {
				CHECK_SPACE(3);
				dst = itoa(dst, devnum);
			} else {
				CHECK_SPACE(2);
				*dst++ = 'a' + devnum;
			}
			break;
		case 'P':
			CHECK_SPACE(3);
			dst = itoa(dst, partnum);
			break;
		case 'U':
			/* GUID replacement needs 36 bytes */
			CHECK_SPACE(36 + 1);
			dst = emit_guid(dst, guid);
			break;
		default:
			CHECK_SPACE(3);
			*dst++ = '%';
			*dst++ = c;
			break;
		}
	}

#undef CHECK_SPACE

	*dst = '\0';
	return 0;
}
#endif

int boot_kernel(VbSelectAndLoadKernelParams *kparams, crossystem_data_t *cdata)
{
#if defined(CONFIG_SANDBOX)
	return 0;
#else
	/* sizeof(CHROMEOS_BOOTARGS) reserves extra 1 byte */
	char cmdline_buf[sizeof(CHROMEOS_BOOTARGS) + CROS_CONFIG_SIZE];
	/* Reserve EXTRA_BUFFER bytes for update_cmdline's string replacement */
	char cmdline_out[sizeof(CHROMEOS_BOOTARGS) + CROS_CONFIG_SIZE +
		EXTRA_BUFFER];
	char *cmdline;
#ifdef CONFIG_X86
	struct boot_params *params;
#else
	/* Chrome OS kernel has to be loaded at fixed location */
	char address[20];
	char *argv[] = { "bootm", address };

	sprintf(address, "%p", kparams->kernel_buffer);
#endif

	strcpy(cmdline_buf, CHROMEOS_BOOTARGS);

	/*
	 * bootloader_address is the offset in kernel image plus kernel body
	 * load address; so subtrate this address from bootloader_address and
	 * you have the offset.
	 *
	 * Note that kernel body load address is kept in kernel preamble but
	 * actually serves no real purpose; for one, kernel buffer is not
	 * always allocated at that address (nor even recommended to be).
	 *
	 * Because this address does not effect kernel buffer location (or in
	 * fact anything else), the current consensus is not to adjust this
	 * address on a per-board basis.
	 *
	 * If for any unforeseeable reason this address is going to be not
	 * CROS_32BIT_ENTRY_ADDR=0x100000, please also update codes here.
	 */
	cmdline = get_kernel_config(kparams->kernel_buffer,
			kparams->bootloader_address - CROS_32BIT_ENTRY_ADDR);
	/*
	 * strncat could write CROS_CONFIG_SIZE + 1 bytes to cmdline_buf. This
	 * is okay because the extra 1 byte has been reserved in sizeof().
	 */
	strncat(cmdline_buf, cmdline, CROS_CONFIG_SIZE);

	VBDEBUG("cmdline before update: ");
	VBDEBUG_PUTS(cmdline_buf);
	VBDEBUG_PUTS("\n");

	if (update_cmdline(cmdline_buf,
			get_dev_num(kparams->disk_handle),
			kparams->partition_number + 1,
			kparams->partition_guid,
			cmdline_out, sizeof(cmdline_out))) {
		VBDEBUG("failed replace %%[DUP] in command line\n");
		return 1;
	}

	setenv("bootargs", cmdline_out);
	VBDEBUG("cmdline after update:  ");
	VBDEBUG_PUTS(getenv("bootargs"));
	VBDEBUG_PUTS("\n");

	g_crossystem_data = cdata;

#ifdef CONFIG_X86
	/* Disable keyboard and flush buffer so that further key strokes
	 * won't interfere kernel driver init. */
	if (i8042_disable())
		VBDEBUG("i8042_disable() failed. fine, continue.\n");
	i8042_flush();

	crossystem_data_update_acpi(cdata);

	params = (struct boot_params *)(uintptr_t)
		(kparams->bootloader_address - CROS_PARAMS_SIZE);
	if (!setup_zimage(params, cmdline, 0, 0, 0))
		boot_zimage(params, kparams->kernel_buffer);
#else
	do_bootm(NULL, 0, ARRAY_SIZE(argv), argv);
#endif

	VBDEBUG("failed to boot; is kernel broken?\n");
	return 1;
#endif
}

#ifdef CONFIG_OF_BOARD_SETUP
/* Optional function */
__weak int ft_system_setup(void *blob, bd_t *bd)
{
	return 0;
}

/*
 * This function does the last chance FDT update before booting to kernel.
 * Currently we modify the FDT by embedding crossystem data. So before
 * calling bootm(), g_crossystem_data should be set.
 */
int ft_board_setup(void *fdt, bd_t *bd)
{
	int err;

	err = ft_system_setup(fdt, bd);
	if (err) {
		VBDEBUG("warning: fdt_system_setup() fails\n");
		return err;
	}
	if (!g_crossystem_data) {
		VBDEBUG("warning: g_crossystem_data is NULL\n");
		return 0;
	}

	err = crossystem_data_embed_into_fdt(g_crossystem_data, fdt);
	if (err) {
		VBDEBUG("crossystem_data_embed_into_fdt() failed\n");
		return err;
	}
	VBDEBUG("Completed setting up fdt information for kerrnel\n");

	return 0;
}
#endif
