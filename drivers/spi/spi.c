/*
 * Copyright (c) 2011 The Chromium OS Authors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <fdtdec.h>

void *spi_do_alloc_slave(int offset, int size, unsigned int bus,
			 unsigned int cs)
{
	struct spi_slave *slave;
	void *ptr;

	ptr = malloc(size);
	if (ptr) {
		memset(ptr, '\0', size);
		slave = (struct spi_slave *)(ptr + offset);
		slave->bus = bus;
		slave->cs = cs;
	}

	return ptr;
}

#ifdef CONFIG_OF_SPI
/**
 * Set up a new SPI slave for an fdt node
 *
 * @param blob         Device tree blob
 * @param slave_node   pointer to this SPI slave node in the device tree
 * @param spi_node     cached pointer to the SPI interface this node belongs to
 * @return pointer to a new slave if ok, NULL on error
 */
struct spi_slave *spi_setup_slave_fdt(const void *blob,
				      int slave_node,
				      int spi_node)
{
	unsigned int bus_index;
	uint32_t max_freq;
	unsigned cs;
	unsigned mode = 0;
	int half_duplex;
	struct spi_slave *spi_slave;
	unsigned frame_header = 0;

	bus_index = spi_get_bus_by_node(blob, spi_node);
	if (bus_index < 0) {
		debug("%s: Failed to find bus node %d\n", __func__, spi_node);
		return NULL;
	}

	/* Decode slave-specific params, providing sendible defaults */
	max_freq = fdtdec_get_int(blob, slave_node,
				  "spi-max-frequency", 0);
	if (fdtdec_get_bool(blob, slave_node, "spi-cpol"))
		mode |= SPI_CPOL;
	if (fdtdec_get_bool(blob, slave_node, "spi-cpha"))
		mode |= SPI_CPHA;
	if (fdtdec_get_bool(blob, slave_node, "spi-cs-high"))
		mode |= SPI_CS_HIGH;
	cs = fdtdec_get_int(blob, slave_node, "reg", 0);
	half_duplex = fdtdec_get_bool(blob, slave_node, "spi-half-duplex");
	if (half_duplex) {
		frame_header = fdtdec_get_int(blob, slave_node,
					      "spi-frame-header", 0x100);
		if (frame_header >= 0x100) {
			debug("%s: frame header not defined or invalid!\n",
			      __func__);
			return NULL;
		}
	}

	spi_slave = spi_setup_slave(bus_index, cs, max_freq, mode);
	if (spi_slave && half_duplex) {
		spi_slave->half_duplex = 1;
		spi_slave->frame_header = frame_header;
		spi_slave->max_timeout_ms = fdtdec_get_int(blob,
							   slave_node,
							   "spi-max-timeout-ms",
							   1000);
	}

	return spi_slave;
}
#endif /* CONFIG_OF_SPI */
