/*
 * Chromium OS cros_ec driver - SPI interface
 *
 * Copyright (c) 2012 The Chromium OS Authors.
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <cros_ec.h>
#include <spi.h>

/*
 * The EC driver handles talking to the embedded controller.
 *
 * Data exchanged between the master and the EC is encapsulated in frames, as
 * follows:
 *
 * Master to EC: <version><data type><data size><data...><checksum>
 * EC to Master: <start><result code><response size><response...><checksum>
 *
 * All fields but <data> and <response> are one byte is size.
 */

/*
 * Technically the frame includes the MSG_HEADER_BYTES header plus the byte of
 * checksum. However, the SPI driver does not return the framing byte, hence
 * the framing overhead above the driver is [MSG_HEADER_BYTES + <cs size> -
 * <framing byte size>], i.e. MSG_HEADER_BYTES
 */
#define EC_FRAME_OVERHEAD MSG_HEADER_BYTES

/**
 * Send a command to a SPI CROS_EC device and return the reply.
 *
 * The device's internal input/output buffers are used.
 *
 * @param dev		CROS_EC device
 * @param cmd		Command to send (EC_CMD_...)
 * @param cmd_version	Version of command to send (EC_VER_...)
 * @param dout		Output data (may be NULL If dout_len=0)
 * @param dout_len      Size of output data in bytes
 * @param dinp		Returns pointer to response data. This will be
 *			untouched unless we return a value > 0.
 * @param din_len	Maximum size of response in bytes
 * @return number of bytes in response, or -1 on error
 */
int cros_ec_spi_command(struct cros_ec_dev *dev, uint8_t cmd, int cmd_version,
		     const uint8_t *dout, int dout_len,
		     uint8_t **dinp, int din_len)
{
	/* SPI driver will return the entire frame but the framing byte. */
	int in_bytes = din_len + EC_FRAME_OVERHEAD;
	uint8_t *out;
	uint8_t *p;
	int csum, len;
	int rv;

	if (!dev->cmd_version_is_supported) {
		debug("%s: Can't talk to EC not supporting command versions\n",
		      __func__);
		return -1;
	}

	/*
	 * Sanity-check input size to make sure it plus transaction overhead
	 * fits in the internal device buffer.
	 */
	if (in_bytes > MSG_BYTES) {
		debug("%s: Cannot receive %d bytes\n", __func__, din_len);
		return -1;
	}

	/* We represent message length as a byte */
	if (dout_len > MSG_BYTES) {
		debug("%s: Cannot send %d bytes\n", __func__, dout_len);
		return -1;
	}

	/*
	 * Clear input buffer so we don't get false hits for MSG_HEADER
	 */
	memset(dev->din, '\0', in_bytes);

	if (spi_claim_bus(dev->u.spi)) {
		debug("%s: Cannot claim SPI bus\n", __func__);
		return -1;
	}

	out = dev->dout;

	*out++ = EC_CMD_VERSION0 + cmd_version;
	*out++ = cmd;
	*out++ = (uint8_t)dout_len;
	memcpy(out, dout, dout_len);
	out[dout_len] = cros_ec_calc_checksum(dev->dout, dout_len + 3);

	/*
	 * Send output data and receive input data starting such that the
	 * message body will be dword aligned.
	 *
	 * The framing byte will not be stored, need to align at one less than
	 * the header size.
	 */
	p = dev->din + sizeof(int64_t) - MSG_HEADER_BYTES + 1;

	/* transmit length includes checksum */
	len = dout_len + EC_FRAME_OVERHEAD + 1;
	cros_ec_dump_data("out", cmd, dev->dout, len);

	rv = spi_xfer(dev->u.spi, max(len, in_bytes) * 8, dev->dout, p,
		      SPI_XFER_BEGIN | SPI_XFER_END);

	spi_release_bus(dev->u.spi);

	if (rv) {
		debug("%s: Cannot complete SPI transfer\n", __func__);
		return -1;
	}

	len = min(p[1] + EC_FRAME_OVERHEAD, din_len + EC_FRAME_OVERHEAD);

	cros_ec_dump_data("in", -1, p, len);

	/* Response code is first byte of the message after start symbol */
	if (p[0] != EC_RES_SUCCESS) {
		printf("%s: Returned status %d\n", __func__, p[0]);
		return -(int)(p[0]);
	}

	/* Verify checksum, exclude checksum itslef from calculations. */
	csum = cros_ec_calc_checksum(p, len - 1);
	if (csum != p[len - 1]) {
		debug("%s: Invalid checksum rx %#02x, calced %#02x\n", __func__,
		      p[len - 1], csum);
		return -1;
	}

	/* Anything else is the response data */
	*dinp = p + 2;

	/*
	 * Driver dropped the framing byte, drop the rest of the header and
	 * the cs here.
	 */
	return len - EC_FRAME_OVERHEAD;
}

int cros_ec_spi_decode_fdt(struct cros_ec_dev *dev, const void *blob)
{
	/* Decode interface-specific FDT params */
	dev->max_frequency = fdtdec_get_int(blob, dev->node,
					    "spi-max-frequency", 500000);

	return 0;
}

/**
 * Initialize SPI protocol.
 *
 * @param dev		CROS_EC device
 * @param blob		Device tree blob
 * @return 0 if ok, -1 on error
 */
int cros_ec_spi_init(struct cros_ec_dev *dev, const void *blob)
{
	dev->u.spi = spi_setup_slave_fdt(blob, dev->node, dev->parent_node);
	if (!dev->u.spi) {
		debug("%s: Could not setup SPI slave\n", __func__);
		return -1;
	}

	return 0;
}
