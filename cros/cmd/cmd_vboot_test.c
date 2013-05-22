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
 * Debug commands for testing basic verified-boot related utilities.
 */

#include <common.h>
#include <command.h>
#include <cros/common.h>
#include <cros/cros_fdtdec.h>
#include <cros/firmware_storage.h>
#include <cros/memory_wipe.h>
#include <cros/power_management.h>
#include <cros/vboot_flag.h>
#include <vboot_api.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * TODO: Pick a better region for test instead of GBB.
 * We now test the region of GBB.
 */
#define TEST_FW_START		0xc1000
#define DEFAULT_TEST_FW_LENGTH	0x1000

static int do_vboot_test_fwrw(cmd_tbl_t *cmdtp,
		int flag, int argc, char * const argv[])
{
	int ret = 0;
	firmware_storage_t file;
	uint32_t test_length, i;
	uint8_t *original_buf, *target_buf, *verify_buf;
	uint64_t t0, t1;

	switch (argc) {
	case 1:  /* if no argument given, use the default length */
		test_length = DEFAULT_TEST_FW_LENGTH;
		break;
	case 2:  /* use argument */
		test_length = simple_strtoul(argv[1], NULL, 16);
		if (!test_length) {
			VbExDebug("The first argument is not a number!\n");
			return cmd_usage(cmdtp);
		}
		break;
	default:
		return cmd_usage(cmdtp);
	}

	/* Allocate the buffer and fill the target test pattern. */
	original_buf = VbExMalloc(test_length);
	verify_buf = VbExMalloc(test_length);
	target_buf = VbExMalloc(test_length);

	/* Fill the target test pattern. */
	for (i = 0; i < test_length; i++)
		target_buf[i] = i & 0xff;

	/* Open firmware storage device. */
	if (firmware_storage_open_spi(&file)) {
		VbExDebug("Failed to open firmware device!\n");
		return 1;
	}

	t0 = VbExGetTimer();
	if (file.read(&file, TEST_FW_START, test_length, original_buf)) {
		VbExDebug("Failed to read firmware!\n");
		goto out;
	}
	t1 = VbExGetTimer();
	VbExDebug("test_fwrw: fw_read, length: %#x, time: %llu\n",
			test_length, t1 - t0);

	t0 = VbExGetTimer();
	ret = file.write(&file, TEST_FW_START, test_length, target_buf);
	t1 = VbExGetTimer();
	VbExDebug("test_fwrw: fw_write, length: %#x, time: %llu\n",
			test_length, t1 - t0);

	if (ret) {
		VbExDebug("Failed to write firmware!\n");
		ret = 1;
	} else {
		/* Read back and verify the data. */
		file.read(&file, TEST_FW_START, test_length, verify_buf);
		if (memcmp(target_buf, verify_buf, test_length) != 0) {
			VbExDebug("Verify failed. The target data wrote "
				  "wrong.\n");
			ret = 1;
		}
	}

	 /* Write the original data back. */
	if (file.write(&file, TEST_FW_START, test_length, original_buf)) {
		VbExDebug("Failed to write the original data back. The "
				"firmware may now be corrupt.\n");

	}

out:
	file.close(&file);

	VbExFree(original_buf);
	VbExFree(verify_buf);
	VbExFree(target_buf);

	if (ret == 0)
		VbExDebug("Read and write firmware test SUCCESS.\n");

	return ret;
}

static int do_vboot_test_memwipe(cmd_tbl_t *cmdtp,
		int flag, int argc, char * const argv[])
{
	memory_wipe_t wipe;
	char s[] = "ABCDEFGHIJ";
	const char r[] = "\0BCDE\0GHIJ";
	const size_t size = strlen(s);
	uintptr_t base = (uintptr_t)s;

	memory_wipe_init(&wipe);
	memory_wipe_add(&wipe, base + 0, base + 1);
	/* Result: -BCDEFGHIJ */
	memory_wipe_sub(&wipe, base + 1, base + 2);
	/* Result: -BCDEFGHIJ */
	memory_wipe_add(&wipe, base + 1, base + 2);
	/* Result: --CDEFGHIJ */
	memory_wipe_sub(&wipe, base + 1, base + 2);
	/* Result: -BCDEFGHIJ */
	memory_wipe_add(&wipe, base + 0, base + 2);
	/* Result: --CDEFGHIJ */
	memory_wipe_sub(&wipe, base + 1, base + 3);
	/* Result: -BCDEFGHIJ */
	memory_wipe_add(&wipe, base + 4, base + 6);
	/* Result: -BCD--GHIJ */
	memory_wipe_add(&wipe, base + 3, base + 4);
	/* Result: -BC---GHIJ */
	memory_wipe_sub(&wipe, base + 3, base + 7);
	/* Result: -BCDEFGHIJ */
	memory_wipe_add(&wipe, base + 2, base + 8);
	/* Result: -B------IJ */
	memory_wipe_sub(&wipe, base + 1, base + 9);
	/* Result: -BCDEFGHIJ */
	memory_wipe_add(&wipe, base + 4, base + 7);
	/* Result: -BCD---HIJ */
	memory_wipe_sub(&wipe, base + 3, base + 5);
	/* Result: -BCDE--HIJ */
	memory_wipe_sub(&wipe, base + 6, base + 8);
	/* Result: -BCDE-GHIJ */
	memory_wipe_execute(&wipe);

	if (memcmp(s, r, size)) {
		int i;

		VbExDebug("Expected: ");
		for (i = 0; i < size; i++)
			VbExDebug("%c", r[i] ? r[i] : '-');
		VbExDebug("\nGot: ");
		for (i = 0; i < size; i++)
			VbExDebug("%c", s[i] ? s[i] : '-');
		VbExDebug("\nFailed to wipe the expected regions!\n");
		return 1;
	}

	VbExDebug("Memory wipe test SUCCESS!\n");
	return 0;
}

static int do_vboot_test_gpio(cmd_tbl_t *cmdtp,
		int flag, int argc, char * const argv[])
{
	int i, ret = 0;

	for (i = 0; i < VBOOT_FLAG_MAX_FLAGS; i++) {
		if (vboot_flag_dump(i, NULL)) {
			VbExDebug("Failed to dump GPIO, %d!\n", i);
			ret = 1;
		}
	}
	return ret;
}

static int do_vboot_reboot(cmd_tbl_t *cmdtp,
		int flag, int argc, char * const argv[])
{
	cold_reboot();
	return 1; /* It should never reach here */
}

static int do_vboot_poweroff(cmd_tbl_t *cmdtp,
		int flag, int argc, char * const argv[])
{
	power_off();
	return 1; /* It should never reach here */
}

static void show_ec_bin(const char *name, const char *region,
			struct fmap_entry *entry)
{
	printf("EC %s binary %s at %#x, length %#x\n", region, name,
	       entry->offset, entry->length);
}

static void show_entry(const char *name, struct fmap_entry *entry)
{
	printf("   %s: offset=%#x, length=%#x\n", name, entry->offset,
	       entry->length);
}

static void show_firmware_entry(const char *name,
				struct fmap_firmware_entry *fw_entry)
{
	printf("%s:\n", name);
	show_entry("boot", &fw_entry->boot);
	show_entry("vblock", &fw_entry->vblock);
	show_entry("firmware_id", &fw_entry->firmware_id);
	printf("block_offset: %llx\n", fw_entry->block_offset);
	show_ec_bin(name, "RW", &fw_entry->ec_rwbin);
	puts("\n");
}

static int do_vboot_fmap(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	enum cros_firmware_type type;
	struct twostop_fmap fmap;

	/* TODO(sjg@chromium.org): Best to automate thist test with sandbox */
	if (cros_fdtdec_flashmap(gd->fdt_blob, &fmap)) {
		VbExDebug("Failed to load fmap config from fdt!\n");
		return 1;
	}

	puts("ro:\n");
	show_entry("fmap", &fmap.readonly.fmap);
	show_entry("gbb", &fmap.readonly.gbb);
	show_entry("firmware_id", &fmap.readonly.firmware_id);
	show_ec_bin("ro", "RO", &fmap.readonly.ec_robin);
	show_ec_bin("ro", "RW", &fmap.readonly.ec_rwbin);
	printf("flash_base: %u\n", fmap.flash_base);

	show_firmware_entry("rw-a", &fmap.readwrite_a);
	show_firmware_entry("rw-b", &fmap.readwrite_b);

	if (cros_fdtdec_firmware_type(gd->fdt_blob, &type)) {
		VbExDebug("Failed to get firmware-type from fdt\n");
		return 1;
	}
	printf("\nFirmware type: %d\n", type);

	return 0;
}

static int do_vboot_chrome_ec(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	struct fdt_chrome_ec ec;

	/* TODO(sjg@chromium.org): Best to automate this test with sandbox */
	if (cros_fdtdec_chrome_ec(gd->fdt_blob, &ec)) {
		VbExDebug("Failed to load EC config from fdt!\n");
		return 1;
	}

	show_entry("flash", &ec.flash);
	puts("Flash erase value: ");
	if (ec.flash_erase_value == -1)
		puts("Unknown\n");
	else
		printf("%#02x\n", ec.flash_erase_value);

	return 0;
}

static int do_vboot_test_all(cmd_tbl_t *cmdtp,
		int flag, int argc, char * const argv[])
{
	int ret = 0;

	ret |= do_vboot_test_fwrw(cmdtp, flag, argc, argv);
	ret |= do_vboot_test_memwipe(cmdtp, flag, argc, argv);
	ret |= do_vboot_test_gpio(cmdtp, flag, argc, argv);
	ret |= do_vboot_fmap(cmdtp, flag, argc, argv);
	ret |= do_vboot_chrome_ec(cmdtp, flag, argc, argv);

	return ret;
}

static cmd_tbl_t cmd_vboot_test_sub[] = {
	U_BOOT_CMD_MKENT(all, 0, 1, do_vboot_test_all, "", ""),
	U_BOOT_CMD_MKENT(fwrw, 0, 1, do_vboot_test_fwrw, "", ""),
	U_BOOT_CMD_MKENT(memwipe, 0, 1, do_vboot_test_memwipe, "", ""),
	U_BOOT_CMD_MKENT(gpio, 0, 1, do_vboot_test_gpio, "", ""),
	U_BOOT_CMD_MKENT(reboot, 0, 1, do_vboot_reboot, "", ""),
	U_BOOT_CMD_MKENT(poweroff, 0, 1, do_vboot_poweroff, "", ""),
	U_BOOT_CMD_MKENT(fmap, 0, 1, do_vboot_fmap, "", ""),
	U_BOOT_CMD_MKENT(chrome_ec, 0, 1, do_vboot_chrome_ec, "", ""),
};

static int do_vboot_test(cmd_tbl_t *cmdtp,
		int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_vboot_test_sub[0],
			ARRAY_SIZE(cmd_vboot_test_sub));
	if (c)
		return c->cmd(c, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(vboot_test, CONFIG_SYS_MAXARGS, 1, do_vboot_test,
	"Perform tests for basic vboot related utilities",
	"all - perform all tests\n"
	"vboot_test fwrw [length] - test the firmware read/write\n"
	"vboot_test memwipe - test the memory wipe functions\n"
	"vboot_test gpio - print the status of gpio\n"
	"vboot_test reboot - test reboot (board will be rebooted!)\n"
	"vboot_test poweroff - test poweroff (board will be shut down!)\n"
	"vboot_test fmap - check that flashmap can be decoded\n"
	"vboot_test chrome_ec - check that EC info can be decoded\n"
);

