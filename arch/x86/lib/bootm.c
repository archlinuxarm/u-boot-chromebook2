/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
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

#include <common.h>
#include <command.h>
#include <fdt_support.h>
#include <image.h>
#include <u-boot/zlib.h>
#include <asm/bootparam.h>
#include <asm/byteorder.h>
#include <asm/zimage.h>
#ifdef CONFIG_SYS_COREBOOT
#include <asm/arch/timestamp.h>
#endif

#define COMMAND_LINE_OFFSET 0x9000

/*
 * Implement a weak default function for boards that optionally
 * need to clean up the system before jumping to the kernel.
 */
__weak void board_final_cleanup(void)
{
}

void bootm_announce_and_cleanup(void)
{
	printf("\nStarting kernel ...\n\n");

#ifdef CONFIG_SYS_COREBOOT
	timestamp_add_now(TS_U_BOOT_START_KERNEL);
#endif
	bootstage_mark_name(BOOTSTAGE_ID_BOOTM_HANDOFF, "start_kernel");
#ifdef CONFIG_BOOTSTAGE_REPORT
	bootstage_report();
#endif
	board_final_cleanup();
}

#if defined(CONFIG_OF_LIBFDT) && !defined(CONFIG_OF_NO_KERNEL)
int arch_fixup_memory_node(void *blob)
{
	bd_t	*bd = gd->bd;
	int bank;
	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	for (bank = 0; bank < CONFIG_NR_DRAM_BANKS; bank++) {
		start[bank] = bd->bi_dram[bank].start;
		size[bank] = bd->bi_dram[bank].size;
	}

	return fdt_fixup_memory_banks(blob, start, size, CONFIG_NR_DRAM_BANKS);
}
#endif

/* Subcommand: PREP */
static int boot_prep_linux(bootm_headers_t *images)
{
	char *cmd_line_dest = NULL;
	image_header_t *hdr;
	int is_zimage = 0;
	void *data = NULL;
	size_t len;
	int ret;

#ifdef CONFIG_OF_LIBFDT
	if (images->ft_len) {
		debug("using: FDT\n");
		if (image_setup_linux(images)) {
			puts("FDT creation failed! hanging...");
			hang();
		}
	}
#endif
	if (images->legacy_hdr_valid) {
		hdr = images->legacy_hdr_os;
		if (image_check_type(hdr, IH_TYPE_MULTI)) {
			ulong os_data, os_len;

			/* if multi-part image, we need to get first subimage */
			image_multi_getimg(hdr, 0, &os_data, &os_len);
			data = (void *)os_data;
			len = os_len;
		} else {
			/* otherwise get image data */
			data = (void *)image_get_data(hdr);
			len = image_get_data_size(hdr);
		}
		is_zimage = 1;
#if defined(CONFIG_FIT)
	} else if (images->fit_uname_setup) {
		/* The entry point is already set */
	} else if (images->fit_uname_os && is_zimage) {
		ret = fit_image_get_data(images->fit_hdr_os,
				images->fit_noffset_os,
				(const void **)&data, &len);
		if (ret) {
			puts("Can't get image data/size!\n");
			goto error;
		}
		is_zimage = 1;
#endif
	}

	if (is_zimage) {
		void *load_address;
		char *base_ptr;

		base_ptr = (char *)load_zimage(data, len, &load_address);
		images->os.load = (ulong)load_address;
		cmd_line_dest = base_ptr + COMMAND_LINE_OFFSET;
		images->ep = (ulong)base_ptr;
	}

	if (!images->ep) {
		printf("## Kernel loading failed (no setup) ...\n");
		goto error;
	}

	printf("Setup at %#08lx\n", images->ep);
	ret = setup_zimage((void *)images->ep, cmd_line_dest,
			0, images->rd_start,
			images->rd_end - images->rd_start);

	if (ret) {
		printf("## Setting up boot parameters failed ...\n");
		return 1;
	}

	return 0;

error:
	return 1;
}

/* Subcommand: GO */
static int boot_jump_linux(bootm_headers_t *images)
{
	debug("## Transferring control to Linux (at address %08lx,"
		" kernel %08lx) ...\n", images->ep, images->os.load);

	boot_zimage((struct boot_params *)images->ep, (void *)images->os.load);
	/* does not return */

	return 1;
}

int do_bootm_linux(int flag, int argc, char * const argv[],
		bootm_headers_t *images)
{
	/* No need for those on x86 */
	if (flag & BOOTM_STATE_OS_BD_T || flag & BOOTM_STATE_OS_CMDLINE)
		return -1;

	if (flag & BOOTM_STATE_OS_PREP)
		return boot_prep_linux(images);

	if (flag & BOOTM_STATE_OS_GO) {
		boot_jump_linux(images);
		return 0;
	}

	if (boot_prep_linux(images))
		return 1;
	return boot_jump_linux(images);
}
