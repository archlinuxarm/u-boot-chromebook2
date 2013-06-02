/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
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

/* #define TEST_BATTERY */

#ifdef TEST_BATTERY
#include <battery.h>
#endif

#include <common.h>
#include <command.h>
#include <cros_ec.h>
#include <ec_commands.h>
#include <fdtdec.h>
#include <i2c.h>
#include <malloc.h>
#include <asm/gpio.h>
#include <power/tps65090_pmic.h>
#include <cros/common.h>
#include <cros/firmware_storage.h>

DECLARE_GLOBAL_DATA_PTR;

static int do_cros_test_i2c(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
#ifdef CONFIG_POWER_TPS65090
	int fet_id = 2;
	int ret = 0;
	int pass;

# ifdef TEST_BATTERY
	struct battery_info base_info, info;

	if (smartbat_init()) {
		printf("Cannot init smart battery\n");
		return 1;
	}
# endif
	if (tps65090_init()) {
		printf("Cannot init tps65090\n");
		return 1;
	}
# ifdef TEST_BATTERY
	if (smartbat_get_info(&base_info)) {
		printf("Cannot get battery info\n");
		return 1;
	}
# endif
	/* Generate continuous i2c traffic and make sure all is well */
	for (pass = 0; pass < 100000; pass++) {
		/* Do some things on the bus, first with TPSCHROME */
		ret = tps65090_fet_enable(fet_id);
		if (ret)
			break;
		if (!tps65090_fet_is_enabled(fet_id)) {
			ret = -1;
			break;
		}

		ret = tps65090_fet_disable(fet_id);
		if (ret)
			break;
		if (tps65090_fet_is_enabled(fet_id)) {
			ret = -1;
			break;
		}
# ifdef TEST_BATTERY
		/* Now have a look at the battery */
		if (smartbat_get_info(&info)) {
			printf("Pass %d: Battery read error\n", pass);
			continue;
		}

		if (0 == memcmp(&base_info, &info, sizeof(info))) {
			puts("Battery info comparison failed\n");
			ret = -1;
			break;
		}
# endif
		if (ctrlc()) {
			puts("Test terminated by ctrl-c\n");
			break;
		}

		/* mdelay(1); */
	}

	if (ret)
		printf("Error detected on pass %d\n", pass);
#endif /* CONFIG_POWER_TPS65090 */
	return 0;
}

#if defined(CONFIG_DRIVER_S3C24X0_I2C) && defined(CONFIG_EXYNOS5250) \
		&& defined(CONFIG_CMD_GPIO)
#include <asm/arch/pinmux.h>

static int setup_i2c(int *nodep)
{
	int node;
	int err;

	node = fdt_path_offset(gd->fdt_blob, "i2c4");
	if (node < 0) {
		printf("Error: Cannot find fdt node\n");
		return -1;
	}

	if (board_i2c_claim_bus(node)) {
		printf("Error: Cannot claim bus\n");
		return -1;
	}
	err = gpio_request(EXYNOS5_GPIO_A20, "i2creset");
	err |= gpio_request(EXYNOS5_GPIO_A21, "i2creset");
	err |= gpio_direction_output(EXYNOS5_GPIO_A20, 0);
	err |= gpio_direction_output(EXYNOS5_GPIO_A21, 0);
	if (err) {
		printf("Error: Could not set up GPIOs\n");
		return -1;
	}
	*nodep = node;

	return 0;
}

static int restore_i2c(int node)
{
	int err;

	err = gpio_direction_output(EXYNOS5_GPIO_A20, 1);
	err |= gpio_direction_output(EXYNOS5_GPIO_A21, 1);
	if (err) {
		printf("Error: Could not restore GPIOs\n");
		return -1;
	}

	if (i2c_reset_port_fdt(gd->fdt_blob, node)) {
		printf("Error: Could not reset I2C after operation\n");
		return -1;
	}

	if (exynos_pinmux_config(PERIPH_ID_I2C4, 0)) {
		printf("Error: Could not restore I2C\n");
		return -1;
	}
	board_i2c_release_bus(node);

	return 0;
}

static int do_cros_test_i2creset(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int node;

	printf("Holding I2C bus 4 low for 15 seconds\n");
	if (setup_i2c(&node))
		return 1;

	mdelay(15 * 1000);
	if (restore_i2c(node))
		return 1;

	printf("   - done, I2C restored\n");

	return 0;
}

/**
 * Return a 1-bit pseudo-random number related to the supplied seed
 *
 * @param seed		Seed value (any integer)
 * @return pseudo-random number (0 or 1)
 */
static int get_fiddle_value(int seed)
{
	int value = 0;
	int i;

	for (i = 0; i < 32; i++)
		value ^= (seed >> i) & 1;

	return value;
}

static int do_cros_test_i2cfiddle(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int node, i;

	printf("Generate random transitions on I2C bus 4\n");
	if (setup_i2c(&node))
		return 1;

	for (i = 0; i < 100; i++) {
		gpio_set_value(EXYNOS5_GPIO_A20, get_fiddle_value(i));
		udelay(1);
		gpio_set_value(EXYNOS5_GPIO_A21, get_fiddle_value(100 - i));
		udelay(1);
	}

	if (restore_i2c(node))
		return 1;
	printf("   - done, I2C restored\n");

	return 0;
}
#endif

#ifdef CONFIG_CROS_EC
struct ssync_info {
	struct cros_ec_dev *dev;	/* cros_ec device */
	firmware_storage_t file;	/* Firmware storage access */
	struct twostop_fmap fmap;	/* Chrome OS Flash map */
	uint8_t *old_data;		/* Buffer to hold old EC image */
	uint8_t *new_data;		/* Buffer to hold new EC image */
	int force;			/* Force flashing even if the same */
	int verify;			/* Verify EC image after writing */
	struct fdt_chrome_ec ec;	/* EC configuration */
};

static int ensure_region_writable(struct cros_ec_dev *dev,
				  enum ec_flash_region region)
{
	enum ec_current_image image;
	enum ec_flash_region want_region;
	enum ec_reboot_cmd cmd;
	int ret;

	ret = cros_ec_read_current_image(dev, &image);
	if (ret) {
		debug("%s: Could not read image\n", __func__);
		return ret;
	}

	if (image == EC_IMAGE_UNKNOWN) {
		debug("%s: Unknown current image\n", __func__);
		return -1;
	}

	/*
	 * Work out which flash region we want to be in. If we are writing
	 * to RO, then the EC needs to be running from RW, and vice versa.
	 */
	want_region = region == EC_FLASH_REGION_RO ?
		EC_FLASH_REGION_RW : EC_FLASH_REGION_RO;

	/* If we are not already in the right image, change it */
	if (!!(want_region == EC_FLASH_REGION_RO) !=
			!!(image == EC_IMAGE_RO)) {
		if (want_region == EC_FLASH_REGION_RO)
			cmd = EC_REBOOT_JUMP_RO;
		else
			cmd = EC_REBOOT_JUMP_RW;
		ret = cros_ec_reboot(dev, cmd, 0);
		if (ret) {
			debug("%s: Could not boot into image\n", __func__);
			return ret;
		}
	}

	/* We are now in the right image */
	return 0;
}

/*
 * Remove FMAP signature pollution from EC image
 *
 * crosbug.com/p/13143
 *
 * cros_bundle_firmware has a workaround which pollutes the fmap
 * signature so that the RO EC binary doesn't include a valid fmap.
 * This would confuse flashrom when it writes the firmware to SPI
 * flash.
 *
 * As a result, we must fix this up here. This can be removed when
 * flashrom is no longer involved with EC update (i.e. a pure
 * software sync is done).
 *
 * This only needs to be done for the RO image, but we do it for both since
 * the __fMAP__ signature doesn't exist in RW, so this code will have no
 * effect.
 */
static void unpollute_image(uint8_t *data, int size)
{
	uint32_t *ptr, *end;

	end = (uint32_t *)(data + size);
	for (ptr = (uint32_t *)data; ptr < end - 1; ptr++) {
		/* Look for __fMAP__ and change to __FMAP__ */
		if (ptr[0] == 0x4d665f5f && ptr[1] == 0x5f5f5041) {
			*ptr = 0x4d465f5f;
			break;
		}
	}
}

static int do_ssync(struct ssync_info *ssync, uint32_t offset, uint32_t size,
		    struct fmap_entry *entry)
{
	uint32_t reset, base;
	int ret;

	if (ssync->file.read(&ssync->file, entry->offset,
			entry->length, ssync->new_data)) {
		debug("%s: Cannot read firmware storage\n", __func__);
		return -1;
	}

	unpollute_image(ssync->new_data, entry->length);

	/*
	* Do a sanity check on the image. This uses information about EC
	* internals, which is nasty, but OTOH it is very useful to get a
	* check that we are not writing garbage to the EC. This can easily
	* happen if the firmware in the SPI flash is not the same as that
	* running (e.g. USB download is used).
	*
	* The second word of the image is the reset vector, and it should
	* point at least 100 bytes into the image, since the vector table
	* and version info precede the init code pointed to by the reset
	* vector. Otherwise, on link, where base=0, you could end up
	* thinking /dev/zero is a valid EC-RO image.
	*
	* (Currently on snow, reset vector points to 0x15c; on link, it
	* points to 0x278).
	*/
	reset = *(uint32_t *)(ssync->new_data + 4);
	base = ssync->ec.flash.offset + offset;
	if (reset < base + 100 || reset >= base + size) {
		printf("Invalid EC image (reset vector %#08x)\n",
			reset);
		return -1;
	}

	if (!ssync->force) {
		puts("read, ");
		ret = cros_ec_flash_read(ssync->dev, ssync->old_data,
				      offset, size);
		if (ret) {
			debug("%s: Cannot read flash\n", __func__);
			return ret;
		}

		if (0 == memcmp(ssync->old_data, ssync->new_data, size)) {
			printf("same, skipping update, ");
			return 0;
		}
	}

	puts("erase, ");
	ret = cros_ec_flash_erase(ssync->dev, offset, size);
	if (ret) {
		debug("%s: Cannot erase flash\n", __func__);
		return ret;
	}

	puts("write, ");
	ret = cros_ec_flash_write(ssync->dev, ssync->new_data, offset, size);
	if (ret) {
		debug("%s: Cannot write flash\n", __func__);
		return ret;
	}

	if (ssync->verify) {
		puts("verify, ");
		ret = cros_ec_flash_read(ssync->dev, ssync->old_data, offset,
				      size);
		if (ret) {
			debug("%s: Cannot write flash\n", __func__);
			return ret;
		}
		if (memcmp(ssync->old_data, ssync->new_data, size)) {
			debug("%s: Cannot verify flash\n", __func__);
			return ret;
		}
	}

	return 0;
}

static int process_region(struct ssync_info *ssync,
			  enum ec_flash_region region)
{
	struct fmap_entry *entry;
	uint32_t offset, size;
	int ret;

	printf("Flashing %s EC image: ",
		region == EC_FLASH_REGION_RW ? "RW" : "RO");
	ret = ensure_region_writable(ssync->dev, region);
	if (ret)
		return ret;
	ret = cros_ec_flash_offset(ssync->dev, region, &offset, &size);
	if (ret) {
		debug("%s: Cannot read flash offset for region %d\n",
			__func__, region);
		return ret;
	}

	ssync->new_data = malloc(size);
	ssync->old_data = malloc(size);
	if (!ssync->new_data || !ssync->old_data) {
		debug("%s: Cannot malloc %d bytes\n", __func__,
			size * 2);
		return -1;
	}

	/*
	 * Clear to the erase value to make it flash friendly. If we don't
	 * know the erase value, assume 0xff. Getting this wrong won't break
	 * things, but might take longer.
	 */
	memset(ssync->new_data, ssync->ec.flash_erase_value, size);

	entry = region == EC_FLASH_REGION_RW ?
		&ssync->fmap.readonly.ec_rw.image :
		&ssync->fmap.readonly.ec_ro.image;
	ret = do_ssync(ssync, offset, size, entry);
	if (ret)
		return -1;

	free(ssync->new_data);
	free(ssync->old_data);
	puts("done\n");

	return 0;
}

int cros_test_swsync(struct cros_ec_dev *dev, int region_mask, int force,
		     int verify)
{
	struct ssync_info ssync;
	ulong start, duration;
	int ret = 0, reboot_ret;

	ssync.dev = dev;
	ssync.force = force;
	ssync.verify = verify;

	assert(dev);
	if (cros_fdtdec_flashmap(gd->fdt_blob, &ssync.fmap)) {
		printf("Cannot read Chrome OS flashmap information\n");
		return 1;
	}

	if (cros_fdtdec_chrome_ec(gd->fdt_blob, &ssync.ec)) {
		printf("Cannot read Chrome OS EC information\n");
		return 1;
	}

	if (firmware_storage_open_spi(&ssync.file)) {
		debug("%s: Cannot open firmware storage\n", __func__);
		return 1;
	}

	start = get_timer(0);

	/* Do RW first, since it might not be bootable, but RO must be */
	if (region_mask & (1 << EC_FLASH_REGION_RW))
		ret = process_region(&ssync, EC_FLASH_REGION_RW);
	if (!ret && (region_mask & (1 << EC_FLASH_REGION_RO)))
		ret = process_region(&ssync, EC_FLASH_REGION_RO);

	ssync.file.close(&ssync.file);

	if (ret)
		printf("\nSoftware sync failed with error %d\n", ret);

	/* Ensure we exit in the RO image */
	reboot_ret = cros_ec_reboot(dev, EC_REBOOT_JUMP_RO, 0);
	if (reboot_ret)
		debug("%s: Could not boot into RO image\n", __func__);

	if (ret)
		return ret;

	duration = get_timer(start);
	printf("Full software sync completed in %lu.%lus\n", duration / 1000,
	       duration % 1000);

	return 0;
}

static int do_cros_test_swsync(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int region_mask = -1;
	struct cros_ec_dev *dev;
	int region, force, verify;
	int ret;

	force = verify = 0;
	dev = board_get_cros_ec_dev();
	if (!dev) {
		printf("No cros_ec device available\n");
		return 1;
	}

	argc--;
	argv++;
	if (argc > 0 && *argv[0] == '-') {
		const char *arg;

		for (arg = argv[0] + 1; *arg; arg++) {
			switch (*arg) {
			case 'f':
				force = 1;
				break;
			case 'v':
				verify = 1;
				break;
			default:
				return CMD_RET_USAGE;
			}
		}

		argc--;
		argv++;
	}

	if (argc > 0) {
		region = cros_ec_decode_region(argc, argv);
		if (region == -1) {
			printf("Invalid region\n");
			return 1;
		}
		region_mask = 1 << region;
	}

	ret = cros_test_swsync(dev, region_mask, force, verify);
	if (ret)
		return 1;

	return 0;
}
#endif /* CONFIG_CROS_EC */

static int do_cros_test_corruptec(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	char *endp;
	int offset;
	int byte;

	if (argc < 3)
		return CMD_RET_USAGE;
	offset = simple_strtoul(argv[1], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return CMD_RET_USAGE;
	byte = simple_strtoul(argv[2], &endp, 16);
	if (*argv[2] == 0 || *endp != 0)
		return CMD_RET_USAGE;

	printf("Setting byte at offset %#x to %#02x\n", offset, byte);
	cros_ec_set_corrupt_image(offset, byte);

	return 0;
}

static int do_cros_test_all(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int ret = 0;

	ret |= do_cros_test_i2c(cmdtp, flag, argc, argv);
	if (!ret)
		printf("All tests passed\n");
	return ret;
}

U_BOOT_SUBCMD_START(cmd_cros_test_sub)
	U_BOOT_CMD_MKENT(i2c, 0, 1, do_cros_test_i2c, "", "")
#if defined(CONFIG_DRIVER_S3C24X0_I2C) && defined(CONFIG_EXYNOS5250) \
		&& defined(CONFIG_CMD_GPIO)
	U_BOOT_CMD_MKENT(i2creset, 0, 1, do_cros_test_i2creset, "", "")
	U_BOOT_CMD_MKENT(i2cfiddle, 0, 1, do_cros_test_i2cfiddle, "", "")
#endif
#ifdef CONFIG_CROS_EC
	U_BOOT_CMD_MKENT(swsync, 0, 1, do_cros_test_swsync, "", "")
#endif
	U_BOOT_CMD_MKENT(corruptec, 0, 1, do_cros_test_corruptec, "", "")
	U_BOOT_CMD_MKENT(all, 0, 1, do_cros_test_all, "", "")
U_BOOT_SUBCMD_END

static int do_cros_test(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_cros_test_sub[0],
			ARRAY_SIZE(cmd_cros_test_sub));
	if (c)
		return c->cmd(c, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(cros_test, CONFIG_SYS_MAXARGS, 1, do_cros_test,
	"Perform tests for Chrome OS",
	"all        Run all tests\n"
	"i2c        Test i2c link with EC, and arbitration\n"
	"i2creset   Try to reset i2c bus by holding clk, data low for 15s\n"
	"i2cfiddle  Try to break TPSCHROME or the battery on i2c\n"
	"swsync [-f] [ro|rw]   Flash the EC (read-only/read-write/both)\n"
	"                         -f   Force update even if the same\n"
	"corruptec <offset> <byte>  Corrupt a single byte of the EC image"
		"during verified boot"
);
