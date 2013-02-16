/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <vboot_api.h>
#include <cbfs.h>
#include <asm/byteorder.h>
#ifdef CONFIG_LZMA
#include <lzma/LzmaTypes.h>
#include <lzma/LzmaDec.h>
#include <lzma/LzmaTools.h>
#endif /* CONFIG_LZMA */
#include <cros/common.h>
#include <cros/cros_fdtdec.h>
#include <tlcl.h>

#if 1 /* Not ported to upstream yet */

int VbExLegacy(void)
{
	printf("Legacy mode not implemented.\n");
	return 1;
}

#else

int board_final_cleanup(void);

/* VbExLegacy calls a payload (e.g. SeaBIOS) from an alternate CBFS
 * that lives in the RW section if CTRL-L is pressed at the dev screen.
 * FIXME: Right now no verification is done what so ever!
 */
int VbExLegacy(void)
{
	CbfsFile payload;
	int legacy_node, flash_node;
	u32 flash_base, legacy_offset, legacy_length;
	u32 reg[2];

	flash_node = fdt_path_offset(gd->fdt_blob, "/flash");
	if (flash_node < 0) {
		printf("Could not find /flash in FDT\n");
		return 1;
	}

	legacy_node = fdt_path_offset(gd->fdt_blob, "/flash/rw-legacy");
	if (!legacy_node) {
		printf("Could not find /flash/rw-legacy in FDT\n");
		return 1;
	}

	if (fdtdec_get_int_array(gd->fdt_blob, flash_node, "reg", reg, 2)) {
		printf("Error decoding reg property of /flash\n");
		return 1;
	}
	flash_base = reg[0];

	if (fdtdec_get_int_array(gd->fdt_blob, legacy_node, "reg", reg, 2)) {
		printf("Error decoding reg property of /flash/rw-legacy\n");
		return 1;
	}
	legacy_offset = reg[0];
	legacy_length = reg[1];

	/* Point to alternate CBFS */
	file_cbfs_init(flash_base + legacy_offset + legacy_length - 1);

	/* For debugging, show the contents of our CBFS */
	do_cbfs_ls(NULL, 0, 0, NULL);

	/* Look for a payload named "payload" */
	payload = file_cbfs_find("payload");
	if (!payload) {
		printf("No file \"payload\" found in CBFS.\n");
		return 1;
	}

	/* This is a minimalistic SELF parser.  */
	CbfsPayloadSegment *seg = payload->data;
	while (1) {
		void (*payload_entry)(void);
		void *src = payload->data + be32_to_cpu(seg->offset);
		void *dst = (void *)(unsigned long)be64_to_cpu(seg->load_addr);
		u32 src_len = be32_to_cpu(seg->len);
		u32 dst_len = be32_to_cpu(seg->mem_len);

		switch (seg->type) {
		case PAYLOAD_SEGMENT_CODE:
		case PAYLOAD_SEGMENT_DATA:
			printf("CODE/DATA: dst=%p dst_len=%d src=%p "
				"src_len=%d\n", dst, dst_len, src, src_len);
			if (be32_to_cpu(seg->compression) ==
						CBFS_COMPRESS_NONE) {
				memcpy(dst, src, src_len);
			} else
#ifdef CONFIG_LZMA
			if (be32_to_cpu(seg->compression) ==
						CBFS_COMPRESS_LZMA) {
				int ret;
				SizeT lzma_len = dst_len;
				ret = lzmaBuffToBuffDecompress(
					(unsigned char *)dst, &lzma_len,
					(unsigned char *)src, src_len);
				if (ret != SZ_OK) {
					printf("LZMA: Decompression failed. "
						"ret=%d.\n", ret);
					return 1;
				}
			} else
#endif
			{
				printf("Compression type %x not supported\n",
					be32_to_cpu(seg->compression));
				return 1;
			}
			break;
		case PAYLOAD_SEGMENT_BSS:
			printf("BSS: dst=%p len=%d\n", dst, dst_len);
			memset(dst, 0, dst_len);
			break;
		case PAYLOAD_SEGMENT_PARAMS:
			printf("PARAMS: skipped\n");
			break;
		case PAYLOAD_SEGMENT_ENTRY:
			board_final_cleanup();
			TlclSaveState();
			payload_entry = dst;
			payload_entry();
			return 0;
		default:
			printf("segment type %x not implemented. Exiting\n",
				seg->type);
			return 1;
		}
		seg++;
	}

	/* Make GCC happy. This point is never reached. */
	return 0;
}
#endif
