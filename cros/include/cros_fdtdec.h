/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CROS_FDTDEC_H_
#define CROS_FDTDEC_H_

#include <fdtdec.h>
#include <cros/fmap.h>

/* Holds information about the Chrome EC */
struct fdt_chrome_ec {
	struct fmap_entry flash;	/* Address and size of EC flash */
	/*
	 * Byte value of erased flash, or -1 if not known. It is normally
	 * 0xff but some flash devices use 0 (e.g. STM32Lxxx)
	 */
	int flash_erase_value;
};

/* Firmware type as given by the fdt */
enum cros_firmware_type {
	CROS_FIRMWARE_RO,
	CROS_FIRMWARE_RW_A,
	CROS_FIRMWARE_RW_B,
};

/* Decode Chrome OS specific configuration from fdt */

int cros_fdtdec_flashmap(const void *fdt, struct twostop_fmap *config);

/**
 * Return offset of /chromeos-config node
 *
 * @param blob	FDT blob
 * @return the offset or -FDT_ERR_NOTFOUND if not found
 */
int cros_fdtdec_config_node(const void *blob);

/**
 * This checks whether a property exists.
 *
 * @param fdt	FDT blob to use
 * @param name	The path and name to the property in question
 * @return non-zero if the property exists, zero if it does not exist.
 */
int cros_fdtdec_config_has_prop(const void *fdt, const char *name);

/**
 * Look up a property in chromeos-config which contains a memory region
 * address and size. Then return a pointer to this address. if the address
 * is zero, it is allocated with malloc() instead.
 *
 * The property must hold one address with a length. This is only tested on
 * 32-bit machines.
 *
 * @param blob		FDT blob
 * @param node		node to examine
 * @param prop_name	name of property to find
 * @param size		returns size of region
 * @return pointer to region, or NULL if property not found/malloc failed
 */
void *cros_fdtdec_alloc_region(const void *blob,
		const char *prop_name, size_t *size);

/**
 * Returns information from the FDT about memory for a given root
 *
 * @param blob          FDT blob to use
 * @param name          Root name of alias to search for
 * @param config        structure to use to return information
 */
int cros_fdtdec_memory(const void *blob, const char *name,
		struct fdt_memory *config);

/**
 * Returns information from the FDT about the Chrome EC
 *
 * @param blob		FDT blob to use
 * @param config	Structure to use to return information
 */
int cros_fdtdec_chrome_ec(const void *blob, struct fdt_chrome_ec *config);
#endif /* CROS_FDTDEC_H_ */
