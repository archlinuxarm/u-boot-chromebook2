/*
 * Copyright 2008, Network Appliance Inc.
 * Author: Jason McMullan <mcmullan <at> netapp.com>
 * Licensed under the GPL-2 or later.
 */

#include <common.h>
#include <malloc.h>
#include <spi_flash.h>

#include "spi_flash_internal.h"

struct winbond_spi_flash_params {
	uint16_t	id;
	uint16_t	nr_blocks;
	const char	*name;
};

static const struct winbond_spi_flash_params winbond_spi_flash_table[] = {
	{
		.id			= 0x3013,
		.nr_blocks		= 8,
		.name			= "W25X40",
	},
	{
		.id			= 0x3015,
		.nr_blocks		= 32,
		.name			= "W25X16",
	},
	{
		.id			= 0x3016,
		.nr_blocks		= 64,
		.name			= "W25X32",
	},
	{
		.id			= 0x3017,
		.nr_blocks		= 128,
		.name			= "W25X64",
	},
	{
		.id			= 0x4014,
		.nr_blocks		= 16,
		.name			= "W25Q80BL",
	},
	{
		.id			= 0x4015,
		.nr_blocks		= 32,
		.name			= "W25Q16",
	},
	{
		.id			= 0x4016,
		.nr_blocks		= 64,
		.name			= "W25Q32",
	},
	{
		.id			= 0x4017,
		.nr_blocks		= 128,
		.name			= "W25Q64",
	},
	{
		.id			= 0x4018,
		.nr_blocks		= 256,
		.name			= "W25Q128",
	},
	{
		.id			= 0x5014,
		.nr_blocks		= 128,
		.name			= "W25Q80",
	},
	{
		.id			= 0x6016,
		.nr_blocks		= 512,
		.name			= "W25Q32DW",
	},
	{
		.id			= 0x6017,
		.nr_blocks		= 128,
		.name			= "W25Q64DW",
	},
	{
		.id			= 0x6016,
		.nr_blocks		= 64,
		.name			= "W25Q32",
	},
};

static int winbond_read_sw_wp_status(struct spi_flash *flash, u8 *result)
{
	int r;
	u8 status_reg = 0;

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

struct spi_flash *spi_flash_probe_winbond(struct spi_slave *spi, u8 *idcode)
{
	const struct winbond_spi_flash_params *params;
	struct spi_flash *flash;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(winbond_spi_flash_table); i++) {
		params = &winbond_spi_flash_table[i];
		if (params->id == ((idcode[1] << 8) | idcode[2]))
			break;
	}

	if (i == ARRAY_SIZE(winbond_spi_flash_table)) {
		debug("SF: Unsupported Winbond ID %02x%02x\n",
				idcode[1], idcode[2]);
		return NULL;
	}

	flash = spi_flash_alloc_base(spi, params->name);
	if (!flash) {
		debug("SF: Failed to allocate memory\n");
		return NULL;
	}

	flash->page_size = 256;
	flash->sector_size = 4096;
	flash->size = 4096 * 16 * params->nr_blocks;
	flash->read_sw_wp_status = winbond_read_sw_wp_status;

	return flash;
}
