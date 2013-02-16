/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <asm/arch-coreboot/sysinfo.h>
#include <asm/io.h>
#include <cros/nvstorage.h>

/* Import the header files from vboot_reference. */
#include <vboot_api.h>

/*
 * The below definitiond and two functions have been borroewd from coreboot's
 * libpayload
 */
#define RTC_PORT_STANDARD      0x70
#define RTC_PORT_EXTENDED      0x72

/**
 * Read a byte from the specified NVRAM address.
 *
 * @param addr The NVRAM address to read a byte from.
 * @return The byte at the given NVRAM address.
 */
u8 nvram_read(u8 addr)
{
	u16 rtc_port = addr < 128 ? RTC_PORT_STANDARD : RTC_PORT_EXTENDED;

	outb(addr, rtc_port);
	return inb(rtc_port + 1);
}

/**
 * Write a byte to the specified NVRAM address.
 *
 * @param val The byte to write to NVRAM.
 * @param addr The NVRAM address to write to.
 */
void nvram_write(u8 val, u8 addr)
{
	u16 rtc_port = addr < 128 ? RTC_PORT_STANDARD : RTC_PORT_EXTENDED;

	outb(addr, rtc_port);
	outb(val, rtc_port + 1);
}

VbError_t nvstorage_read_nvram(uint8_t *buf)
{
	int i;

	if (lib_sysinfo.vbnv_start == (uint32_t)(-1)) {
		printf("%s:%d - vbnv address undefined\n",
		       __FUNCTION__, __LINE__);
		return VBERROR_INVALID_SCREEN_INDEX;
	}

	for (i = 0; i < lib_sysinfo.vbnv_size; i++)
		buf[i] = nvram_read(lib_sysinfo.vbnv_start + i);

	return VBERROR_SUCCESS;
}

VbError_t nvstorage_write_nvram(const uint8_t *buf)
{
	int i;

	if (lib_sysinfo.vbnv_start == (uint32_t)(-1)) {
		printf("%s:%d - vbnv address undefined\n",
		       __FUNCTION__, __LINE__);
		return VBERROR_INVALID_SCREEN_INDEX;
	}

	for (i = 0; i < lib_sysinfo.vbnv_size; i++)
		nvram_write(buf[i], lib_sysinfo.vbnv_start + i);

	return VBERROR_SUCCESS;
}
