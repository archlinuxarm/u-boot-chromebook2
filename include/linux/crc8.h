/*
 * Copyright (c) 2013 Google, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#ifndef __linux_crc8_h
#define __linux_crc8_h

/**
 * crc8() - Calculate and return CRC-8 of the data
 *
 * This uses an x^8 + x^2 + x + 1 polynomial.  A table-based algorithm would
 * be faster, but for only a few bytes it isn't worth the code size
 *
 * @vptr: Buffer to checksum
 * @len: Length of buffer in bytes
 * @return CRC8 checksum
 */
unsigned int crc8(const unsigned char *vptr, int len);

#endif
