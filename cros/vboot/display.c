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
#ifdef CONFIG_LCD
#include <lcd.h>
#endif
#ifdef CONFIG_CFB_CONSOLE
#include <video.h>
#endif
#include <cros/cros_fdtdec.h>
#include <cros/common.h>
#include <cros/crossystem_data.h>
#include <lzma/LzmaTypes.h>
#include <lzma/LzmaDec.h>
#include <lzma/LzmaTools.h>

#ifdef CONFIG_EXYNOS_DISPLAYPORT
/* for exynos_lcd_check_next_stage() */
#include <asm/arch/s5p-dp.h>
#endif

#define PRINT_MAX_ROW	20
#define PRINT_MAX_COL	80

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_CHROMEOS_DISPLAY) && defined(CONFIG_SANDBOX)
#error CONFIG_CHROMEOS_DISPLAY is not handled for Sandbox configuration
#endif

struct display_callbacks {
	int (*dc_get_pixel_width) (void);
	int (*dc_get_pixel_height) (void);
	int (*dc_get_screen_columns) (void);
	int (*dc_get_screen_rows) (void);
	void (*dc_position_cursor) (unsigned col, unsigned row);
	void (*dc_puts) (const char *s);
	int (*dc_display_bitmap) (ulong, int, int);
	void (*dc_display_clear) (void);
};

#ifdef CONFIG_CHROMEOS_DISPLAY
static struct display_callbacks display_callbacks_ = {
#ifdef CONFIG_LCD
	.dc_get_pixel_width = lcd_get_pixel_width,
	.dc_get_pixel_height = lcd_get_pixel_height,
	.dc_get_screen_columns = lcd_get_screen_columns,
	.dc_get_screen_rows = lcd_get_screen_rows,
	.dc_position_cursor = lcd_position_cursor,
	.dc_puts = lcd_puts,
	.dc_display_bitmap = lcd_display_bitmap,
	.dc_display_clear = lcd_clear,

#elif defined(CONFIG_VIDEO)
	.dc_get_pixel_width = video_get_pixel_width,
	.dc_get_pixel_height = video_get_pixel_height,
	.dc_get_screen_columns = video_get_screen_columns,
	.dc_get_screen_rows = video_get_screen_rows,
	.dc_position_cursor = video_position_cursor,
	.dc_puts = video_puts,
	.dc_display_bitmap = video_display_bitmap,
	.dc_display_clear = video_clear
#endif
};
#endif

VbError_t VbExDisplayInit(uint32_t *width, uint32_t *height)
{
	/*
	 * crosbug.com/p/13492
	 * This may be an unexpected display init request - probably due to a
	 * software sync. Read the bmpblk.
	 */
	cros_cboot_twostop_read_bmp_block();
#ifdef CONFIG_EXYNOS_DISPLAYPORT
	/* Make sure the LCD is up */
	exynos_lcd_check_next_stage(gd->fdt_blob, 1);
#endif

#ifdef CONFIG_CHROMEOS_DISPLAY
	*width = display_callbacks_.dc_get_pixel_width();
	*height = display_callbacks_.dc_get_pixel_height();
#else
	*width = 640;
	*height = 480;
#endif
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayBacklight(uint8_t enable)
{
	/* TODO(waihong@chromium.org) Implement it later */
	return VBERROR_SUCCESS;
}

#ifdef CONFIG_CHROMEOS_DISPLAY
/**
 * Write out a line of characters to the display
 *
 * @param ch	Character to output
 * @param len	Number of characters to output
 */
static void out_line(int ch, int len)
{
	char str[2];
	int i;

	str[0] = ch;
	str[1] = '\0';
	for (i = 0; i < len; i++)
		display_callbacks_.dc_puts(str);
}

/* Print the message on the center of LCD. */
static void print_on_center(const char *message)
{
	int cols = display_callbacks_.dc_get_screen_columns();
	int rows = display_callbacks_.dc_get_screen_rows();
	int row, len;

	display_callbacks_.dc_position_cursor(0, 0);

	for (row = 0; row < (rows - 4) / 2; row++)
		out_line('.', cols);
	out_line(' ', cols);
	out_line(' ', cols);

	/* Center the message on its line */
	len = strlen(message);
	out_line(' ', (cols - len) / 2);
	display_callbacks_.dc_puts(message);
	out_line(' ', (cols - len + 1) / 2);

	out_line(' ', cols);
	out_line(' ', cols);

	/* Don't write to the last row, since that will cause a scroll */
	for (row += 5; row < rows - 1; row++)
		out_line('.', cols);
}
#endif

VbError_t VbExDisplayScreen(uint32_t screen_type)
{
	const char *msg = NULL;

	/*
	 * Show the debug messages for development. It is a backup method
	 * when GBB does not contain a full set of bitmaps.
	 */
	switch (screen_type) {
	case VB_SCREEN_BLANK:
		/* clear the screen */
#ifdef CONFIG_CHROMEOS_DISPLAY
		display_clear();
#elif defined(CONFIG_SANDBOX)
		msg = "<screen cleared>";
#endif
		break;
	case VB_SCREEN_DEVELOPER_WARNING:
		msg = "developer mode warning";
		break;
	case VB_SCREEN_DEVELOPER_EGG:
		msg = "easter egg";
		break;
	case VB_SCREEN_RECOVERY_REMOVE:
		msg = "remove inserted devices";
		break;
	case VB_SCREEN_RECOVERY_INSERT:
		msg = "insert recovery image";
		break;
	case VB_SCREEN_RECOVERY_NO_GOOD:
		msg = "insert image invalid";
		break;
	case VB_SCREEN_WAIT:
		msg = "wait for ec update";
		break;
	default:
		VBDEBUG("Not a valid screen type: %08x.\n",
			screen_type);
		return VBERROR_INVALID_SCREEN_INDEX;
	}

	if (msg != NULL) {
#ifdef CONFIG_CHROMEOS_DISPLAY
		print_on_center(msg);
#elif defined(CONFIG_SANDBOX)
		VbExDebug("%s", msg);
#endif
	}
	return VBERROR_SUCCESS;
}

VbError_t VbExDecompress(void *inbuf, uint32_t in_size,
                         uint32_t compression_type,
                         void *outbuf, uint32_t *out_size)
{
	switch (compression_type) {
	case COMPRESS_NONE:
		memcpy(outbuf, inbuf, in_size);
                *out_size = in_size;
		return VBERROR_SUCCESS;

#ifdef CONFIG_LZMA
	case COMPRESS_LZMA1: {
		SizeT input_size = in_size;
		SizeT output_size = *out_size;
		int ret;

		ret = lzmaBuffToBuffDecompress(outbuf, &output_size,
					       inbuf, input_size);
		if (ret != SZ_OK) {
			return ret;
		}
                *out_size = output_size;
		return VBERROR_SUCCESS;
	}
#endif
	}

	VBDEBUG("Unsupported compression format: %08x\n", compression_type);
	return VBERROR_INVALID_PARAMETER;
}

#ifdef CONFIG_CHROMEOS_DISPLAY
#include <bmp_layout.h>

static int sanity_check_bitmap(void *buffer, uint32_t buffersize)
{
	bmp_image_t *bmp;
	uint32_t data_offset, width, height, bit_count, bitmap_size;

	if (buffersize < sizeof(bmp_image_t)) {
		VBDEBUG("buffersize = %u < sizeof(bmp_image_t) = %u\n",
				buffersize, sizeof(bmp_image_t));
		return -1;
	}

	bmp = (bmp_image_t *)buffer;
	if (bmp->header.signature[0] != 'B' ||
			bmp->header.signature[1] != 'M') {
		VBDEBUG("invalid bmp image or "
				"gzipped image, which is not supported\n");
		return -1;
	}

	data_offset = le32_to_cpu(bmp->header.data_offset);
	if (data_offset > buffersize) {
		VBDEBUG("data_offset = %u > buffersize = %u\n",
				data_offset, buffersize);
		return -1;
	}

	/*
	 * This is a conservative estimation of bitmap size (it does not
	 * consider padding).
	 */
	width = le32_to_cpu(bmp->header.width);
	height = le32_to_cpu(bmp->header.height);
	bit_count = le16_to_cpu(bmp->header.bit_count);
	bitmap_size = width * height * bit_count / 8;
	if (data_offset + bitmap_size > buffersize) {
		VBDEBUG("data_offset + bitmap_size = %u > buffersize = %u\n",
				data_offset + bitmap_size, buffersize);
		return -1;
	}

	return 0;
}
#endif

VbError_t VbExDisplayImage(uint32_t x, uint32_t y,
                           void *buffer, uint32_t buffersize)
{
#ifdef CONFIG_CHROMEOS_DISPLAY
	int ret;

	if (sanity_check_bitmap(buffer, buffersize))
		return VBERROR_INVALID_BMPFV;

	ret = display_callbacks_.dc_display_bitmap((ulong)buffer, x, y);
	if (ret) {
		VBDEBUG("LCD display error.\n");
		return VBERROR_UNKNOWN;
	}
#endif
	return VBERROR_SUCCESS;
}

#ifdef CONFIG_CHROMEOS_DISPLAY
/**
 * show_cdata_string() - Display a prompt followed a checked string
 *
 * This is used to show string information from crossystem_data. if this is
 * not set up correctly then we need to make sure we don't print garbage.
 *
 * @prompt:	Prompt string to show
 * @str:	String to print. If the length if > 200 then we assume it is
 *		corrupted
 */
static void show_cdata_string(const char *prompt, const unsigned char *ustr)
{
	const char *str = (const char *)ustr;

	display_callbacks_.dc_puts(prompt);
	if (strlen(str) > 200)
		str = "corrupted";
	display_callbacks_.dc_puts(str);
	display_callbacks_.dc_puts("\n");
}
#endif

VbError_t VbExDisplayDebugInfo(const char *info_str)
{
#ifdef CONFIG_CHROMEOS_DISPLAY
	crossystem_data_t *cdata;
	size_t size;

	display_callbacks_.dc_position_cursor(0, 0);
	display_callbacks_.dc_puts(info_str);


	cdata = cros_fdtdec_alloc_region(gd->fdt_blob, "cros-system-data",
					 &size);
	if (!cdata) {
		VBDEBUG("cros-system-data missing "
				"from fdt, or malloc failed\n");
		return VBERROR_UNKNOWN;
	}

	/* Sanity check in case this memory is not yet set up */
	show_cdata_string("read-only firmware id: ",
			  cdata->readonly_firmware_id);
	show_cdata_string("active firmware id: ", cdata->firmware_id);
#endif
	return VBERROR_SUCCESS;
}

/* this function is not technically part of the vboot interface */
int display_clear(void)
{
#ifdef CONFIG_CHROMEOS_DISPLAY
	display_callbacks_.dc_display_clear();
#endif

	return 0;
}
