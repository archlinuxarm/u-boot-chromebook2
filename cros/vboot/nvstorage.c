/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <fdtdec.h>
#include <cros/common.h>
#include <cros/cros_fdtdec.h>
#include <cros/crossystem_data.h>
#include <cros/nvstorage.h>

#include <vboot_api.h>

DECLARE_GLOBAL_DATA_PTR;

static uint8_t nv_type = NONVOLATILE_STORAGE_NONE;
static nvstorage_read_funcptr nv_read;
static nvstorage_write_funcptr nv_write;

int nvstorage_init(void)
{
	const void *blob = gd->fdt_blob;
	int croscfg_node, length;
	const char *media;
	uint8_t type;

	croscfg_node = cros_fdtdec_config_node(blob);
	if (croscfg_node < 0)
		return 1;

	media = fdt_getprop(blob, croscfg_node, "nvstorage-media", &length);
	if (!media) {
		VBDEBUG("Cannot find nvstorage-media\n");
		return 1;
	}

	if (!strcmp(media, "nvram"))
		type = NONVOLATILE_STORAGE_NVRAM;
	else if (!strcmp(media, "mkbp"))
		type = NONVOLATILE_STORAGE_CROS_EC;
	else if (!strcmp(media, "disk"))
		type = NONVOLATILE_STORAGE_DISK;
	else
		type = NONVOLATILE_STORAGE_NONE;

	if (type == NONVOLATILE_STORAGE_NONE) {
		VBDEBUG("Unknown/unsupport storage media: %s\n", media);
		return 1;
	}

	return nvstorage_set_type(type);
}

uint8_t nvstorage_get_type(void)
{
	return nv_type;
}

int nvstorage_set_type(uint8_t type)
{
#ifdef CONFIG_SYS_COREBOOT
	if (type == NONVOLATILE_STORAGE_NVRAM) {
		nv_type = type;
		nv_read = nvstorage_read_nvram;
		nv_write = nvstorage_write_nvram;
		return 0;
	}
#endif
#ifdef CONFIG_CROS_EC
	if (type == NONVOLATILE_STORAGE_CROS_EC) {
		nv_type = type;
		nv_read = nvstorage_read_cros_ec;
		nv_write = nvstorage_write_cros_ec;
		return 0;
	}
#endif
	if (type == NONVOLATILE_STORAGE_DISK) {
		nv_type = type;
		nv_read = nvstorage_read_disk;
		nv_write = nvstorage_write_disk;
		return 0;
	}

	VBDEBUG("Unknown/unsupport storage type: %d\n", type);
	return 1;
}

VbError_t VbExNvStorageRead(uint8_t* buf)
{
	return nv_read(buf);
}

VbError_t VbExNvStorageWrite(const uint8_t* buf)
{
	return nv_write(buf);
}
