/*
 * Gigadevice SPI flash driver
 * Copyright 2013, Samsung Electronics Co., Ltd.
 * Author: Banajit Goswami <banajit.g@samsung.com>
 *
 */

#include <common.h>
#include <malloc.h>
#include <spi_flash.h>

#include "spi_flash_internal.h"

struct gigadevice_spi_flash_params {
	uint16_t	id;
	uint16_t	nr_blocks;
	const char	*name;
};

static const struct gigadevice_spi_flash_params gigadevice_spi_flash_table[] = {
	{
		.id			= 0x6016,
		.nr_blocks		= 64,
		.name			= "GD25LQ",
	},
	{
		.id			= 0x4017,
		.nr_blocks		= 128,
		.name			= "GD25Q64B",
	},

};

/**
 * Get write protection status for a SPI flash device.
 *
 * @param flash	    pointer to the structure defining the device

 * @param result    pointer to the location where write protection status needs
 *                  to be saved. If any part of the flash is protected, the
 *                  saved value is True.
 *
 * @return zero if succeeds, non-zero if fails to read the status.
 */
static int gigadevice_read_sw_wp_status(struct spi_flash *flash, u8 *result)
{
	unsigned int status_reg;
	int r;

	r = spi_flash_cmd_read_status(flash, &status_reg);
	if (r)					/* couldn't tell, assume no */
		return r;

	/* Return true if ANY area is protected (BP[2:0] != 000b) */
	if (status_reg & 0x1c)
		*result = 1;
	else
		*result = 0;

	return 0;
}

struct spi_flash *spi_flash_probe_gigadevice(struct spi_slave *spi, u8 *idcode)
{
	const struct gigadevice_spi_flash_params *params;
	struct spi_flash *flash;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(gigadevice_spi_flash_table); i++) {
		params = &gigadevice_spi_flash_table[i];
		if (params->id == ((idcode[1] << 8) | idcode[2]))
			break;
	}

	if (i == ARRAY_SIZE(gigadevice_spi_flash_table)) {
		debug("SF: Unsupported Gigadevice ID %02x%02x\n",
				idcode[1], idcode[2]);
		return NULL;
	}

	flash = spi_flash_alloc_base(spi, params->name);
	if (!flash) {
		debug("SF: Failed to allocate memory\n");
		return NULL;
	}
	/* Assuming power-of-two page size initially. */
	flash->page_size = 256;
	/* sector_size = page_size * pages_per_sector */
	flash->sector_size = flash->page_size * 16;
	/* size = sector_size * sector_per_block * number of blocks */
	flash->size = flash->sector_size * 16 * params->nr_blocks;
	flash->read_sw_wp_status = gigadevice_read_sw_wp_status;

	return flash;
}
