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

static struct nvstorage_method *nvstorage_method;

struct nvstorage_method *nvstorage_find_name(const char *name)
{
	struct nvstorage_method *method, *start;
	int upto, count;

	start = ll_entry_start(struct nvstorage_method, nvstorage_method);
	count = ll_entry_count(struct nvstorage_method, nvstorage_method);
	for (upto = 0, method = start; upto < count; method++, upto++) {
		if (!strcmp(method->name, name))
			return method;
	}

	/* Work-around for old 'mkbp' name */
	if (!strcmp(name, "mkbp"))
		return nvstorage_find_name("cros_ec");

	VBDEBUG("Unknown/unsupport storage name: '%s', count=%d\n", name,
		count);
	return NULL;
}

int nvstorage_set_name(const char *name)
{
	struct nvstorage_method *method;

	method = nvstorage_find_name(name);
	if (!method)
		return -1;

	nvstorage_method = method;
	VBDEBUG("Method = %s\n", method->name);

	return 0;
}

struct nvstorage_method *nvstorage_get_method(void)
{
	return nvstorage_method;
}

int nvstorage_init(void)
{
	const void *blob = gd->fdt_blob;
	int croscfg_node, length;
	const char *media;

	croscfg_node = cros_fdtdec_config_node(blob);
	if (croscfg_node < 0)
		return 1;

	media = fdt_getprop(blob, croscfg_node, "nvstorage-media", &length);
	if (!media) {
		VBDEBUG("Cannot find nvstorage-media\n");
		return 1;
	}

	return nvstorage_set_name(media);
}

uint8_t nvstorage_get_type(void)
{
	return nvstorage_method ? nvstorage_method->type :
			NONVOLATILE_STORAGE_NONE;
}

int nvstorage_set_type(uint8_t type)
{
	struct nvstorage_method *method, *start;
	int upto, count;

	start = ll_entry_start(struct nvstorage_method, nvstorage_method);
	count = ll_entry_count(struct nvstorage_method, nvstorage_method);
	for (upto = 0, method = start; upto < count; method++, upto++) {
		if (method->type == type) {
			nvstorage_method = method;
			return 0;
		}
	}

	VBDEBUG("Unknown/unsupport storage type: %d, count=%d\n", type, count);
	return -1;
}

VbError_t VbExNvStorageRead(uint8_t* buf)
{
	if (!nvstorage_method)
		return VBERROR_UNKNOWN;
	return nvstorage_method->read(buf);
}

VbError_t VbExNvStorageWrite(const uint8_t* buf)
{
	if (!nvstorage_method)
		return VBERROR_UNKNOWN;
	return nvstorage_method->write(buf);
}
