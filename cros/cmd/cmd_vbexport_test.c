/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/*
 * Debug commands for testing Verify Boot Export APIs that are implemented by
 * firmware and exported to vboot_reference. Some of the tests are automatic
 * but some of them are manual.
 */

#include <common.h>
#include <command.h>
#include <gbb_header.h>
#include <cros/cros_init.h>
#include <cros/firmware_storage.h>
#include <cros/gbb.h>
#include <cros/vboot_flag.h>

#include <vboot_api.h>
#include <bmpblk_header.h>

DECLARE_GLOBAL_DATA_PTR;

#define TEST_LBA_START		0
#define DEFAULT_TEST_LBA_COUNT	2

#define KEY_CTRL_C	0x03

static int do_vbexport_test_debug(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	const char c = 'K';
	const char s[] = "Hello! It's \"Chrome OS\".";
	const int16_t hd = -22222;
	const uint16_t hu = 44444;
	const int32_t ld = -1111111111L;
	const uint32_t lu = 2222222222UL;
	const int64_t lld = -8888888888888888888LL;
	const uint64_t llu = 11111111111111111111ULL;
	VbExDebug("The \"Expect\" and \"Actual\" should be the same...\n");
	VbExDebug("Expect: K 75 Hello! It's \"Chrome OS\".\n");
	VbExDebug("Actual: %c %d %s\n", c, c, s);
	VbExDebug("Expect: -22222 0xa932\n");
	VbExDebug("Actual: %hd 0x%hx\n", hd, hd);
	VbExDebug("Expect: 44444 0xad9c\n");
	VbExDebug("Actual: %hu 0x%hx\n", hu, hu);
	VbExDebug("Expect: -1111111111 0xbdc5ca39\n");
	VbExDebug("Actual: %ld 0x%lx\n", ld, ld);
	VbExDebug("Expect: 2222222222 0x84746b8e\n");
	VbExDebug("Actual: %lu 0x%lx\n", lu, lu);
	VbExDebug("Expect: -8888888888888888888 0x84a452a6a1dc71c8\n");
	VbExDebug("Actual: %lld 0x%llx\n", lld, lld);
	VbExDebug("Expect: 11111111111111111111 0x9a3298afb5ac71c7\n");
	VbExDebug("Actual: %llu 0x%llx\n", llu, llu);
	return 0;
}

static int do_vbexport_test_malloc_size(uint32_t size)
{
	char *mem = VbExMalloc(size);
	int i, line_size;

#if CONFIG_ARM
	line_size = dcache_get_line_size();
#elif defined CACHE_LINE_SIZE
	line_size = CACHE_LINE_SIZE;
#else
	line_size = __BIGGEST_ALIGNMENT__;
#endif

	VbExDebug("Trying to malloc a memory block for %lu bytes...", size);
	if ((uintptr_t)mem % line_size != 0) {
		VbExDebug("\nMemory not algined with a cache line!\n");
		VbExFree(mem);
		return 1;
	}
	for (i = 0; i < size; i++) {
		mem[i] = i % 0x100;
	}
	for (i = 0; i < size; i++) {
		if (mem[i] != i % 0x100) {
			VbExDebug("\nMemory verification failed!\n");
			VbExFree(mem);
			return 1;
		}
	}
	VbExFree(mem);
	VbExDebug(" - SUCCESS\n");
	return 0;
}

static int do_vbexport_test_malloc(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;
	VbExDebug("Performing the malloc/free tests...\n");
	ret |= do_vbexport_test_malloc_size(1);
	ret |= do_vbexport_test_malloc_size(2);
	ret |= do_vbexport_test_malloc_size(4);
	ret |= do_vbexport_test_malloc_size(8);
	ret |= do_vbexport_test_malloc_size(32);
	ret |= do_vbexport_test_malloc_size(1024);
	ret |= do_vbexport_test_malloc_size(4 * 1024);
	ret |= do_vbexport_test_malloc_size(32 * 1024);
	ret |= do_vbexport_test_malloc_size(1 * 1024 * 1024);
	ret |= do_vbexport_test_malloc_size(12345);
	ret |= do_vbexport_test_malloc_size(13579);
	return ret;
}

typedef void (*sleep_handler_t)(uint32_t);

static void sleep_ms_handler(uint32_t msec)
{
	VbExSleepMs(msec);
}

static void beep_handler(uint32_t msec)
{
	VbExBeep(msec, 4000);
}

static int do_vbexport_test_sleep_time(sleep_handler_t handler, uint32_t msec)
{
	uint32_t start, end, delta, expected;
	VbExDebug("System is going to sleep for %lu ms...\n", msec);
	start = VbExGetTimer();
	(*handler)(msec);
	end = VbExGetTimer();
	delta = end - start;
	VbExDebug("From tick %lu to %lu (delta: %lu)", start, end, delta);

	expected = msec * CONFIG_SYS_HZ;
	/* The sleeping time should be accurate to within 10%. */
	if (delta > expected + expected / 10) {
		VbExDebug("\nSleep too long: expected %lu but actaully %lu!\n",
				expected, delta);
		return 1;
	} else if (delta < expected - expected / 10) {
		VbExDebug("\nSleep too short: expected %lu but actaully %lu!\n",
				expected, delta);
		return 1;
	}
	VbExDebug(" - SUCCESS\n");
	return 0;
}

static int do_vbexport_test_sleep(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;
	VbExDebug("Performing the sleep tests...\n");
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 10);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 50);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 100);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 500);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 1000);
	return ret;
}

static int do_vbexport_test_longsleep(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;
	VbExDebug("Performing the long sleep tests...\n");
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 5000);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 10000);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 50000);
	return ret;
}

static int do_vbexport_test_beep(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;
	VbExDebug("Performing the beep tests...\n");
	ret |= do_vbexport_test_sleep_time(&beep_handler, 500);
	return ret;
}

static int do_vbexport_test_diskinfo_flags(uint32_t flags)
{
	int ret = 0;
	VbDiskInfo *info;
	uint32_t count, i;

	ret = VbExDiskGetInfo(&info, &count, flags);
	if (ret)
		return ret;

	if (count == 0) {
		VbExDebug("No disk found!\n");
	} else {
		VbExDebug("handle    byte/lba  lba_count  f  name\n");
		for (i = 0; i < count; i++) {
			VbExDebug("%08lx  %-9llu %-10llu %-2lu %s",
					info[i].handle,
					info[i].bytes_per_lba,
					info[i].lba_count,
					info[i].flags,
					info[i].name);

			if ((flags & info[i].flags) != flags) {
				VbExDebug("    INCORRECT: flag mismatched\n");
				ret = 1;
			} else
				VbExDebug("\n");
		}
	}

	return VbExDiskFreeInfo(info, NULL) || ret;
}

static int do_vbexport_test_diskinfo(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;

	VbExDebug("Detecting all fixed disks...\n");
	ret |= do_vbexport_test_diskinfo_flags(VB_DISK_FLAG_FIXED);

	VbExDebug("\nDetecting all removable disks...\n");
	ret |= do_vbexport_test_diskinfo_flags(VB_DISK_FLAG_REMOVABLE);

	return ret;
}

static int do_vbexport_test_diskrw(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;
	VbDiskInfo *disk_info;
	VbExDiskHandle_t handle;
	uint32_t disk_count, test_lba_count, buf_byte_count, i;
	uint8_t *original_buf, *target_buf, *verify_buf;
	uint64_t t0, t1;

	switch (argc) {
	case 1:  /* if no argument given, use the default lba count */
		test_lba_count = DEFAULT_TEST_LBA_COUNT;
		break;
	case 2:  /* use argument */
		test_lba_count = simple_strtoul(argv[1], NULL, 10);
		if (!test_lba_count) {
			VbExDebug("The first argument is not a number!\n");
			return cmd_usage(cmdtp);
		}
		break;
	default:
		return cmd_usage(cmdtp);
	}

	/* We perform read/write operations on the first internal disk. */
	if (VbExDiskGetInfo(&disk_info, &disk_count, VB_DISK_FLAG_FIXED) ||
			disk_count == 0) {
		VbExDebug("No internal disk found!\n");
		return 1;
	}
	handle = disk_info[0].handle;
	buf_byte_count = disk_info[0].bytes_per_lba * test_lba_count;
	VbExDiskFreeInfo(disk_info, handle);

	/* Allocate the buffer and fill the target test pattern. */
	original_buf = VbExMalloc(buf_byte_count);
	target_buf = VbExMalloc(buf_byte_count);
	verify_buf = VbExMalloc(buf_byte_count);

	/* Fill the target test pattern. */
	for (i = 0; i < buf_byte_count; i++)
		target_buf[i] = i & 0xff;

	t0 = VbExGetTimer();
	if (VbExDiskRead(handle, TEST_LBA_START, test_lba_count,
			original_buf)) {
		VbExDebug("Failed to read disk.\n");
		goto out;
	}
	t1 = VbExGetTimer();
	VbExDebug("test_diskrw: disk_read, lba_count: %u, time: %llu\n",
			test_lba_count, t1 - t0);

	t0 = VbExGetTimer();
	ret = VbExDiskWrite(handle, TEST_LBA_START, test_lba_count, target_buf);
	t1 = VbExGetTimer();
	VbExDebug("test_diskrw: disk_write, lba_count: %u, time: %llu\n",
			test_lba_count, t1 - t0);

	if (ret) {
		VbExDebug("Failed to write disk.\n");
		ret = 1;
	} else {
		/* Read back and verify the data. */
		VbExDiskRead(handle, TEST_LBA_START, test_lba_count,
				verify_buf);
		if (memcmp(target_buf, verify_buf, buf_byte_count) != 0) {
			VbExDebug("Verify failed. The target data wrote "
					"wrong.\n");
			ret = 1;
		}
	}

	/* Write the original data back. */
	if (VbExDiskWrite(handle, TEST_LBA_START, test_lba_count,
			original_buf)) {
		VbExDebug("Failed to write the original data back. The disk "
				"may now be corrupt.\n");
	}

out:
	VbExDiskFreeInfo(disk_info, NULL);

	VbExFree(original_buf);
	VbExFree(target_buf);
	VbExFree(verify_buf);

	if (ret == 0)
		VbExDebug("Read and write disk test SUCCESS.\n");

	return ret;
}

static int do_vbexport_test_nvclear(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	uint8_t zero_buf[VBNV_BLOCK_SIZE] = {0};

	if (VbExNvStorageWrite(zero_buf)) {
		VbExDebug("Failed to write nvstorage.\n");
		return 1;
	}

	return 0;
}

static int do_vbexport_test_nvrw(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;
	uint8_t original_buf[VBNV_BLOCK_SIZE];
	uint8_t target_buf[VBNV_BLOCK_SIZE];
	uint8_t verify_buf[VBNV_BLOCK_SIZE];
	int i;

	for (i = 0; i < VBNV_BLOCK_SIZE; i++) {
		target_buf[i] = (0x27 + i) % 0x100;
	}

	if (VbExNvStorageRead(original_buf)) {
		VbExDebug("Failed to read nvstorage.\n");
		return 1;
	}

	if (VbExNvStorageWrite(target_buf)) {
		VbExDebug("Failed to write nvstorage.\n");
		ret = 1;
	} else {
		/* Read back and verify the data. */
		VbExNvStorageRead(verify_buf);
		if (memcmp(target_buf, verify_buf, VBNV_BLOCK_SIZE) != 0) {
			VbExDebug("Verify failed. The target data wrote "
				  "wrong.\n");
			ret = 1;
		}
	}

	/* Write the original data back. */
	VbExNvStorageWrite(original_buf);

	if (ret == 0)
		VbExDebug("Read and write nvstorage test SUCCESS.\n");

	return ret;
}

static int do_vbexport_test_key(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;
	uint32_t c = 0;

	VbExDebug("Press any key for test. Press Ctrl-C to exit...\n");
	while (c != KEY_CTRL_C) {
		c = VbExKeyboardRead();
		if (c)
			VbExDebug("Key pressed: 0x%02x\n", c);
	}

	return ret;
}

static int show_screen_and_delay(uint32_t screen_type)
{
	if (VbExDisplayScreen(screen_type)) {
		VbExDebug("Failed to show a screen.\n");
		return 1;
	}

	VbExSleepMs(1000);
	return 0;
}

static uint8_t *read_gbb_from_firmware(void)
{
	void *fdt_ptr = (void *)gd->fdt_blob;
	firmware_storage_t file;
	struct twostop_fmap fmap;
	void *gbb;
	size_t gbb_size;

	gbb = cros_fdtdec_alloc_region(gd->fdt_blob,
			"google-binary-block-offset", &gbb_size);
	if (!gbb) {
		VbExDebug("Failed to find gbb region!\n");
		return NULL;
	}

	if (cros_fdtdec_flashmap(fdt_ptr, &fmap)) {
		VbExDebug("Failed to load fmap config from fdt!\n");
		return NULL;
	}

	/* Open firmware storage device. */
	if (firmware_storage_open_spi(&file)) {
		VbExDebug("Failed to open firmware device!\n");
		return NULL;
	}

	if (gbb_init(gbb, &file, fmap.readonly.gbb.offset, gbb_size)) {
		VbExDebug("Failed to read GBB!\n");
		return NULL;
	}

	if (gbb_read_bmp_block(gbb, &file, fmap.readonly.gbb.offset,
			       gbb_size)) {
		VbExDebug("Failed to load BMP Block in GBB!\n");
		return NULL;
	}

	if (file.close(&file)) {
		VbExDebug("Failed to close firmware device!\n");
	}

	return gbb;
}

/**
 * Show an image on the screen at the given location
 */
static int show_image(ImageInfo *image, int x, int y)
{
	uint32_t inoutsize;
	void *rawimg;
	int err;

	inoutsize = image->original_size;

	if (COMPRESS_NONE == image->compression) {
		rawimg = NULL;
	} else {
		rawimg = VbExMalloc(inoutsize);
		if (VbExDecompress(image + 1, image->compressed_size,
					image->compression,
					rawimg, &inoutsize)) {
			return -1;
		}
	}

	err = VbExDisplayImage(x, y,  rawimg ? rawimg : image + 1, inoutsize);
	if (rawimg)
		VbExFree(rawimg);

	return err ? -1 : 0;
}

/**
 * need comment here
 */
static int show_images_and_delay(BmpBlockHeader *bmph, int local, int index)
{
	int i;
	ScreenLayout *screen;
	ImageInfo *image;

	screen = (ScreenLayout *)(bmph + 1);
	screen += local * bmph->number_of_screenlayouts;
	screen += index;

	for (i = 0;
	     i < MAX_IMAGE_IN_LAYOUT && screen->images[i].image_info_offset;
	     i++) {
		image = (ImageInfo *)((uint8_t *)bmph +
				screen->images[i].image_info_offset);
		if (show_image(image, screen->images[i].x,
				screen->images[i].y))
			goto bad;
	}

	VbExSleepMs(1000);
	return 0;

bad:
	VbExDebug("Failed to display image, screen=%lu, image=%d!\n", index, i);
	return 1;
}

static int do_vbexport_test_display(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;
	uint32_t width, height;
	GoogleBinaryBlockHeader *gbbh;
	BmpBlockHeader *bmph;
	int screen = -1;
	int local = 0;

	if (argc > 1) {
		screen = simple_strtoul(argv[1], NULL, 10);
		if (argc > 2)
			local = simple_strtoul(argv[2], NULL, 10);
	}

	if (VbExDisplayInit(&width, &height)) {
		VbExDebug("Failed to init display.\n");
		return 1;
	}

	VbExDebug("The screen dimensions is %ldx%ld.\n", width, height);

	VbExDebug("Showing screens for localisation %d...\n", local);
	mdelay(500);
	if (screen == -1) {
		ret |= show_screen_and_delay(VB_SCREEN_BLANK);
		ret |= show_screen_and_delay(VB_SCREEN_DEVELOPER_WARNING);
		ret |= show_screen_and_delay(VB_SCREEN_DEVELOPER_EGG);
		ret |= show_screen_and_delay(VB_SCREEN_RECOVERY_REMOVE);
		ret |= show_screen_and_delay(VB_SCREEN_RECOVERY_INSERT);
		ret |= show_screen_and_delay(VB_SCREEN_RECOVERY_NO_GOOD);
		ret |= show_screen_and_delay(VB_SCREEN_WAIT);
	}

	gbbh = (GoogleBinaryBlockHeader *)read_gbb_from_firmware();
	if (gbbh) {
		bmph = (BmpBlockHeader *)((uint8_t *)gbbh + gbbh->bmpfv_offset);

		VbExDebug("Showing images...\n");
		if (screen != -1) {
			ret |= show_images_and_delay(bmph, local, screen);
		} else {
			for (screen = 0; screen < MAX_VALID_SCREEN_INDEX;
					screen++)
				ret |= show_images_and_delay(bmph, local,
							     screen);
		}
	} else {
		ret = 1;
	}

	VbExDebug("Showing debug info...\n");
	ret |= VbExDisplayDebugInfo("Hello!\n"
			"This is a debug message.\n"
			"Bye Bye!\n");

	return ret;
}

static int do_vbexport_test_isshutdown(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	VbExDebug("Shutdown requested? %s\n",
			VbExIsShutdownRequested() ? "Yes" : "No");
	return 0;
}

static int do_vbexport_test_image(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	uint32_t width, height;
	GoogleBinaryBlockHeader *gbbh;
	BmpBlockHeader *bmph;
	ScreenLayout *screens;
	int snum, locale = 0;

	if (argc > 1)
		locale = simple_strtoul(argv[1], NULL, 10);

	if (VbExDisplayInit(&width, &height)) {
		VbExDebug("Failed to init display.\n");
		return 1;
	}

	gbbh = (GoogleBinaryBlockHeader *)read_gbb_from_firmware();
	if (!gbbh) {
		VbExDebug("Cannot read GBB\n");
		return 1;
	}

	bmph = (BmpBlockHeader *)((uint8_t *)gbbh + gbbh->bmpfv_offset);
	screens = (ScreenLayout *)(bmph + 1);

	VbExDebug("Total screen layouts: %d\n", bmph->number_of_screenlayouts);
	VbExDebug("Total localizations: %d\n", bmph->number_of_localizations);
	VbExDebug("Total images: %d\n", bmph->number_of_imageinfos);
	VbExDebug("bmpfv_offset=%#x, size=%#x\n", gbbh->bmpfv_offset,
		  gbbh->bmpfv_size);

	if (locale >= bmph->number_of_localizations) {
		VbExDebug("Selected localization is out of range\n");
		return 1;
	}

	/* Select the correct block */
	screens += locale * bmph->number_of_screenlayouts;

	for (snum = 0; snum < bmph->number_of_screenlayouts; snum++) {
		ScreenLayout *screen = &screens[snum];
		ImageInfo *image;
		int inum;

		VbExDebug("Screen: %d\n", snum);
		for (inum = 0; inum < MAX_IMAGE_IN_LAYOUT; inum++) {
			if (!screen->images[inum].image_info_offset)
				break;
			image = (ImageInfo *)((uint8_t *)bmph +
				screen->images[inum].image_info_offset);
			VbExDebug("   Image: %d at %d, %d: offset=%#x, "
				"size=%#x, end=%#x\n", inum,
				  screen->images[inum].x,
				  screen->images[inum].y,
				  gbbh->bmpfv_offset +
					screen->images[inum].image_info_offset,
				  image->compressed_size,
				  gbbh->bmpfv_offset +
				  screen->images[inum].image_info_offset +
					image->compressed_size);
			show_image(image, screen->images[inum].x,
				   screen->images[inum].y);
		}
		mdelay(500);
	}

	return 0;
}

static int do_vbexport_test_flags(cmd_tbl_t *cmdtp, int flag,
				  int argc, char * const argv[])
{
	struct vboot_flag_details details;
	int ret;
	int i;

	for (i = 0; i < VBOOT_FLAG_MAX_FLAGS; i++) {
		ret = vboot_flag_fetch(i, &details);
		printf("%s: ", vboot_flag_node_name(i));
		if (ret)
			printf("Error %d\n", ret);
		else {
			printf("value=%d, port=%d, active_high=%d, prev_value=%d\n",
			       details.value, details.port, details.active_high,
			       details.prev_value);
		}
	}

	return 0;
}

static int do_vbexport_test_all(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;
	ret |= do_vbexport_test_debug(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_malloc(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_sleep(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_beep(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_diskinfo(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_diskrw(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_nvrw(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_key(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_display(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_isshutdown(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_flags(cmdtp, flag, argc, argv);
	if (!ret)
		VbExDebug("All tests passed!\n");
	return ret;
}

static int do_vbexport_init(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	return cros_init();
}

U_BOOT_SUBCMD_START(cmd_vbexport_test_sub)
	U_BOOT_CMD_MKENT(init, 0, 1, do_vbexport_init, "", "")
	U_BOOT_CMD_MKENT(all, 0, 1, do_vbexport_test_all, "", "")
	U_BOOT_CMD_MKENT(debug, 0, 1, do_vbexport_test_debug, "", "")
	U_BOOT_CMD_MKENT(malloc, 0, 1, do_vbexport_test_malloc, "", "")
	U_BOOT_CMD_MKENT(sleep, 0, 1, do_vbexport_test_sleep, "", "")
	U_BOOT_CMD_MKENT(longsleep, 0, 1, do_vbexport_test_longsleep, "", "")
	U_BOOT_CMD_MKENT(beep, 0, 1, do_vbexport_test_beep, "", "")
	U_BOOT_CMD_MKENT(diskinfo, 0, 1, do_vbexport_test_diskinfo, "", "")
	U_BOOT_CMD_MKENT(diskrw, 0, 1, do_vbexport_test_diskrw, "", "")
	U_BOOT_CMD_MKENT(nvrw, 0, 1, do_vbexport_test_nvrw, "", "")
	U_BOOT_CMD_MKENT(nvclear, 0, 1, do_vbexport_test_nvclear, "", "")
	U_BOOT_CMD_MKENT(key, 0, 1, do_vbexport_test_key, "", "")
	U_BOOT_CMD_MKENT(display, 0, 1, do_vbexport_test_display, "", "")
	U_BOOT_CMD_MKENT(isshutdown, 0, 1, do_vbexport_test_isshutdown, "", "")
	U_BOOT_CMD_MKENT(image, 0, 1, do_vbexport_test_image, "", "")
	U_BOOT_CMD_MKENT(flags, 0, 1, do_vbexport_test_flags, "", "")
U_BOOT_SUBCMD_END

static int do_vbexport_test(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_vbexport_test_sub[0],
			ARRAY_SIZE(cmd_vbexport_test_sub));
	if (c)
		return c->cmd(c, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(vbexport_test, CONFIG_SYS_MAXARGS, 1, do_vbexport_test,
	"Perform tests for vboot_wrapper",
	"init - initialize cros library\n"
	"vbexport_test all - perform all tests\n"
	"vbexport_test debug - test the debug function\n"
	"vbexport_test malloc - test the malloc and free functions\n"
	"vbexport_test sleep - test the sleep and timer functions\n"
	"vbexport_test longsleep - test the sleep functions for long delays\n"
	"vbexport_test beep - test the beep functions\n"
	"vbexport_test diskinfo - test the diskgetinfo and free functions\n"
	"vbexport_test diskrw [lba_count] - test the disk read and write\n"
	"vbexport_test nvclear - clear the nvstorage content\n"
	"vbexport_test nvrw - test the nvstorage read and write functions\n"
	"vbexport_test key - test the keyboard read function\n"
	"vbexport_test display [screen [locale]] - test display functions\n"
	"vbexport_test isshutdown - check if shutdown requested\n"
	"vbexport_test image [locale] - show bmp images\n"
	"vbexport_test flags - show vboot flags"
);

