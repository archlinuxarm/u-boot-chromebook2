/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of firmware storage access interface for SPI */

#include <common.h>
#include <fdtdec.h>
#include <libfdt.h>
#include <malloc.h>
#include <spi_flash.h>
#include <cros/common.h>
#include <cros/cros_fdtdec.h>
#include <cros/firmware_storage.h>

DECLARE_GLOBAL_DATA_PTR;

enum {
	SF_DEFAULT_SPEED = 1000000,
};

/*
 * Check the right-exclusive range [offset:offset+*count_ptr), and adjust
 * value pointed by <count_ptr> to form a valid range when needed.
 *
 * Return 0 if it is possible to form a valid range. and non-zero if not.
 */
static int border_check(struct spi_flash *flash, uint32_t offset,
		uint32_t count)
{
	uint32_t max_offset = offset + count;

	if (offset >= flash->size) {
		VBDEBUG("at EOF\n");
		return -1;
	}

	/* max_offset will be less than offset iff overflow occurred. */
	if (max_offset < offset || max_offset > flash->size) {
		VBDEBUG("exceed range\n");
		return -1;
	}

	return 0;
}

#ifdef CONFIG_HARDWARE_MAPPED_SPI

/*
 * When using hardware mapped SPI the offset passed to the file_read()
 * function should be added to the base address of the SPI flash in the CPI
 * memory. set_spi_flash_base() retrieves the flash chip base address from the
 * flash map device tree section.
 */
static uint32_t spi_flash_base_addr;

void set_spi_flash_base(void)
{
	int fmap_offset;
	uint32_t *property;
	int length;
	const void *blob = gd->fdt_blob;

	fmap_offset = fdt_node_offset_by_compatible(blob, -1,
			"chromeos,flashmap");
	if (fmap_offset < 0) {
		VBDEBUG("chromeos,flashmap node is missing\n");
		return;
	}
	property = (uint32_t *)fdt_getprop(blob, fmap_offset, "reg", &length);
	if (!property) {
		VBDEBUG("reg property missing in flashmap!'\n");
		return;
	}
	spi_flash_base_addr = fdt32_to_cpu(property[0]);
}

#endif

static int read_spi(firmware_storage_t *file, uint32_t offset, uint32_t count,
		read_buf_type buf)
{
	struct spi_flash *flash = file->context;

	if (border_check(flash, offset, count))
		return -1;

#ifndef CONFIG_HARDWARE_MAPPED_SPI
	if (flash->read(flash, offset, count, buf)) {
		VBDEBUG("SPI read fail\n");
		return -1;
	}
#else
	if (!spi_flash_base_addr)
		set_spi_flash_base();
	*buf =  (void *) (spi_flash_base_addr + offset);
#endif
	return 0;
}

/*
 * FIXME: It is a reasonable assumption that sector size = 4096 bytes.
 * Nevertheless, comparing to coding this magic number here, there should be a
 * better way (maybe rewrite driver interface?) to expose this parameter from
 * eeprom driver.
 */
#define SECTOR_SIZE 0x1000

/*
 * Align the right-exclusive range [*offset_ptr:*offset_ptr+*length_ptr) with
 * SECTOR_SIZE.
 * After alignment adjustment, both offset and length will be multiple of
 * SECTOR_SIZE, and will be larger than or equal to the original range.
 */
static void align_to_sector(uint32_t *offset_ptr, uint32_t *length_ptr)
{
	VBDEBUG("before adjustment\n");
	VBDEBUG("offset: 0x%x\n", *offset_ptr);
	VBDEBUG("length: 0x%x\n", *length_ptr);

	/* Adjust if offset is not multiple of SECTOR_SIZE */
	if (*offset_ptr & (SECTOR_SIZE - 1ul)) {
		*offset_ptr &= ~(SECTOR_SIZE - 1ul);
	}

	/* Adjust if length is not multiple of SECTOR_SIZE */
	if (*length_ptr & (SECTOR_SIZE - 1ul)) {
		*length_ptr &= ~(SECTOR_SIZE - 1ul);
		*length_ptr += SECTOR_SIZE;
	}

	VBDEBUG("after adjustment\n");
	VBDEBUG("offset: 0x%x\n", *offset_ptr);
	VBDEBUG("length: 0x%x\n", *length_ptr);
}

static int write_spi(firmware_storage_t *file, uint32_t offset, uint32_t count,
		void *buf)
{
	struct spi_flash *flash = file->context;
	uint8_t static_buf[SECTOR_SIZE];
	uint8_t *backup_buf;
	uint32_t k, n;
	int status, ret = -1;

	/* We will erase <n> bytes starting from <k> */
	k = offset;
	n = count;
	align_to_sector(&k, &n);

	VBDEBUG("offset:          0x%08x\n", offset);
	VBDEBUG("adjusted offset: 0x%08x\n", k);
	if (k > offset) {
		VBDEBUG("align incorrect: %08x > %08x\n", k, offset);
		return -1;
	}

	if (border_check(flash, k, n))
		return -1;

	backup_buf = n > sizeof(static_buf) ? malloc(n) : static_buf;

	if ((status = flash->read(flash, k, n, backup_buf))) {
		VBDEBUG("cannot backup data: %d\n", status);
		goto EXIT;
	}

	if ((status = flash->erase(flash, k, n))) {
		VBDEBUG("SPI erase fail: %d\n", status);
		goto EXIT;
	}

	/* combine data we want to write and backup data */
	memcpy(backup_buf + (offset - k), buf, count);

	if (flash->write(flash, k, n, backup_buf)) {
		VBDEBUG("SPI write fail\n");
		goto EXIT;
	}

	ret = 0;

EXIT:
	if (backup_buf != static_buf)
		free(backup_buf);

	return ret;
}

static int close_spi(firmware_storage_t *file)
{
	struct spi_flash *flash = file->context;
	spi_flash_free(flash);
	return 0;
}

int firmware_storage_open_spi(firmware_storage_t *file)
{
	const void *blob = gd->fdt_blob;
	int node;
	unsigned int bus, chip_select, max_frequency, mode;
	struct spi_flash *flash;

	node = cros_fdtdec_config_node(blob);
	if (node < 0)
		return -1;
	node = fdtdec_lookup_phandle(blob, node, "firmware-storage");
	if (node < 0) {
		VBDEBUG("fail to look up phandle: %d\n", node);
		return -1;
	}

	bus = fdtdec_get_int(blob, node, "bus", 0);
	chip_select = fdtdec_get_int(blob, node, "reg", 0);
	max_frequency = fdtdec_get_int(blob, node, "spi-max-frequency",
			SF_DEFAULT_SPEED);
	mode = 0;
	if (fdtdec_get_bool(blob, node, "spi-cpol"))
		mode |= SPI_CPOL;
	if (fdtdec_get_bool(blob, node, "spi-cpha"))
		mode |= SPI_CPHA;
	if (fdtdec_get_bool(blob, node, "spi-cs-high"))
		mode |= SPI_CS_HIGH;
	flash = spi_flash_probe(bus, chip_select, max_frequency, mode);
	if (!flash) {
		VBDEBUG("fail to init SPI flash at %u:%u\n", bus, chip_select);
		return -1;
	}

	file->read = read_spi;
	file->write = write_spi;
	file->close = close_spi;
	file->context = (void *)flash;

	return 0;
}
