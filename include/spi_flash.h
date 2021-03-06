/*
 * Interface to SPI flash
 *
 * Copyright (C) 2008 Atmel Corporation
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_

#include <spi.h>
#include <linux/types.h>
#include <linux/compiler.h>

struct spi_flash {
	struct spi_slave *spi;

	const char	*name;

	/* Total flash size */
	u32		size;
	/* Write (page) size */
	u32		page_size;
	/* Erase (sector) size */
	u32		sector_size;

	void *memory_map;	/* Address of read-only SPI flash access */
	int		(*read)(struct spi_flash *flash, u32 offset,
				size_t len, void *buf);
	int		(*write)(struct spi_flash *flash, u32 offset,
				size_t len, const void *buf);
	int		(*erase)(struct spi_flash *flash, u32 offset,
				size_t len);
	int		(*read_sw_wp_status)(struct spi_flash *flash,
				u8 *result);
};

/**
 * spi_flash_do_alloc - Allocate a new spi flash structure
 *
 * The structure is allocated and cleared with default values for
 * read, write and erase, which the caller can modify. The caller must set
 * up size, page_size and sector_size.
 *
 * Use the helper macro spi_flash_alloc() to call this.
 *
 * @offset: Offset of struct spi_slave within slave structure
 * @size: Size of slave structure
 * @spi: SPI slave
 * @name: Name of SPI flash device
 */
void *spi_flash_do_alloc(int offset, int size, struct spi_slave *spi,
			 const char *name);

/**
 * spi_flash_alloc - Allocate a new SPI flash structure
 *
 * @_struct: Name of structure to allocate (e.g. struct ramtron_spi_fram). This
 *	structure must contain a member 'struct spi_flash *flash'.
 * @spi: SPI slave
 * @name: Name of SPI flash device
 */
#define spi_flash_alloc(_struct, spi, name) \
	spi_flash_do_alloc(offsetof(_struct, flash), sizeof(_struct), \
				spi, name)

/**
 * spi_flash_alloc_base - Allocate a new SPI flash structure with no private data
 *
 * @spi: SPI slave
 * @name: Name of SPI flash device
 */
#define spi_flash_alloc_base(spi, name) \
	spi_flash_do_alloc(0, sizeof(struct spi_flash), spi, name)

struct spi_flash *spi_flash_probe(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int spi_mode);

/**
 * Set up a new SPI flash from an fdt node
 *
 * @param blob		Device tree blob
 * @param slave_node	Pointer to this SPI slave node in the device tree
 * @param spi_node	Cached pointer to the SPI interface this node belongs
 *			to
 * @return 0 if ok, -1 on error
 */
struct spi_flash *spi_flash_probe_fdt(const void *blob, int slave_node,
				      int spi_node);

void spi_flash_free(struct spi_flash *flash);

static inline int spi_flash_read(struct spi_flash *flash, u32 offset,
		size_t len, void *buf)
{
	return flash->read(flash, offset, len, buf);
}

static inline int spi_flash_write(struct spi_flash *flash, u32 offset,
		size_t len, const void *buf)
{
	return flash->write(flash, offset, len, buf);
}

static inline int spi_flash_erase(struct spi_flash *flash, u32 offset,
		size_t len)
{
	return flash->erase(flash, offset, len);
}

static inline int spi_flash_read_sw_wp_status(struct spi_flash *flash,
					      u8 *result)
{
	if (flash->read_sw_wp_status)
		return flash->read_sw_wp_status(flash, result);
	return 1;				/* else not implemented */
}

/**
 * spi_flash_cmd_write_status() - Write the SPi flash status
 *
 * @flash: SPI flash to access
 * @status: Status to write
 * @write_16bit: true to write the full 16-bit status, false to write 8 bits
 * @return 0 if ok, -ve on error
 */
int spi_flash_cmd_write_status(struct spi_flash *flash, unsigned int status,
			       bool write_16bit);

/**
 * spi_flash_cmd_write_status() - Read the SPi flash status
 *
 * @flash: SPI flash to access
 * @status: Returns status (16-bits)
 * @return 0 if ok, -ve on error
 */
int spi_flash_cmd_read_status(struct spi_flash *flash, unsigned int *status);

void spi_boot(void) __noreturn;

#endif /* _SPI_FLASH_H_ */
