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
#include <libfdt.h>
#include <cros/common.h>
#include <cros/cros_fdtdec.h>
#include <cros/fmap.h>
#include <fdtdec.h>
#include <linux/string.h>
#include <malloc.h>

/*
 * Some platforms where DRAM is based at zero do not define DRAM base address
 * explicitly.
 */
#ifdef CONFIG_SYS_SDRAM_BASE
#define DRAM_BASE_ADDRESS CONFIG_SYS_SDRAM_BASE
#else
#define DRAM_BASE_ADDRESS 0
#endif

int cros_fdtdec_config_node(const void *blob)
{
	int node = fdt_path_offset(blob, "/chromeos-config");

	if (node < 0)
		VBDEBUG("failed to find /chromeos-config: %d\n", node);

	return node;
}

/* These are the various flashmap nodes that we are interested in */
enum section_t {
	SECTION_BASE,		/* group section, no name: rw-a and rw-b */
	SECTION_FIRMWARE_ID,
	SECTION_BOOT,
	SECTION_GBB,
	SECTION_VBLOCK,
	SECTION_FMAP,
	SECTION_ECRW,
	SECTION_ECRO,

	SECTION_COUNT,
	SECTION_NONE = -1,
};

/* Names for each section, preceeded by ro-, rw-a- or rw-b- */
static const char *section_name[SECTION_COUNT] = {
	"",
	"firmware-id",
	"boot",
	"gbb",
	"vblock",
	"fmap",
	"ecrw",
	"ecro",
};

/**
 * Look up a section name and return its type
 *
 * @param name		Name of section (after ro- or rw-a/b- part)
 * @return section type section_t, or SECTION_NONE if none
 */
static enum section_t lookup_section(const char *name)
{
	char *at;
	int i, len;

	at = strchr(name, '@');
	len = at ? at - name : strlen(name);
	for (i = 0; i < SECTION_COUNT; i++)
		if (0 == strncmp(name, section_name[i], len))
			return i;

	return SECTION_NONE;
}

/**
 * Read a flash entry from the fdt
 *
 * @param blob		FDT blob
 * @param node		Offset of node to read
 * @param name		Name of node being read
 * @param entry		Place to put offset and size of this node
 * @return 0 if ok, -ve on error
 */
static int read_entry(const void *blob, int node, const char *name,
		      struct fmap_entry *entry)
{
	u32 reg[2];

	if (fdtdec_get_int_array(blob, node, "reg", reg, 2)) {
		VBDEBUG("Node '%s' has bad/missing 'reg' property\n", name);
		return -FDT_ERR_NOTFOUND;
	}
	entry->offset = reg[0];
	entry->length = reg[1];

	return 0;
}

/**
 * Process a flashmap node, storing its information in our config.
 *
 * @param blob		FDT blob
 * @param node		Offset of node to read
 * @param depth		Depth of node: 1 for a normal section, 2 for a
 *			sub-section
 * @param config	Place to put the information we read
 * @param rwp		Indicates the type of data in the last depth 1 node
 *			that we read. For example, if *rwp == NULL then we
 *			read the ro section; if *rwp == &config->readwrite_a
 *			then we read the rw-a section. This is used to work
 *			out which section we are referring to at depth 2.
 * @param ecp		Indicates the EC node that we are currently
 *			processing (this is used at depth 2 to refer back
 *			to the EC node). We get the flash offset of the EC
 *			node so we can work out the absolute position of the
 *			EC in flash. Plus we write the EC hash.
 *
 * Both rwp and ecp start as NULL and are updated when we see an RW and an
 * EC region respectively. This function is called for every node in the
 * device tree and these variables maintain the state that we need to
 * process the nodes correctly.
 *
 * @return 0 if ok, -ve on error
 */
static int process_fmap_node(const void *blob, int node, int depth,
		struct twostop_fmap *config, struct fmap_firmware_entry **rwp,
		struct fmap_ec_image **ecp)
{
	struct fmap_firmware_entry *rw = *rwp;
	enum section_t section;
	struct fmap_entry entry;
	const char *name, *subname, *prop;
	int len;

	/*
	 * At depth 2, we are looking for our ec subnode. There really is no
	 * need for this situation - we no longer have multiple images in
	 * each region so that hashes could be stored up one level.
	 * crbug.com/254311
	 */
	name = fdt_get_name(blob, node, &len);
	if (depth == 2) {
		struct fmap_ec_image *ec = *ecp;
		struct fmap_entry *entry;
		ulong offset = 0;

		if (ec) {
			entry = &ec->image;
			offset = entry->offset;
			ec->hash = fdt_getprop(blob, node, "hash",
					       &ec->hash_size);
		} else if (rw) {
			if (0 == strcmp(name, "boot")) {
				entry = &rw->boot_rwbin;
				offset = rw->boot.offset;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
		if (read_entry(blob, node, name, entry))
			return -FDT_ERR_NOTFOUND;

		/* Add the section offset to get an 'absolute offset' */
		entry->offset += offset;
		return 0;
	}

	*ecp = NULL;
	if (name && !strcmp("rw-vblock-dev", name)) {
		/* handle optional dev key */
		if (read_entry(blob, node, name, &config->readwrite_devkey))
			return -FDT_ERR_NOTFOUND;
		else
			return 0;
	}

	/* We are looking only for ro-, rw-a- and rw-b- */
	if (len < 4 || *name != 'r' || name[2] != '-')
		return 0;
	if (name[1] == 'o') {
		rw = NULL;
		subname = name + 3;
	} else if (name[1] == 'w') {
		if (name[3] == 'a')
			rw = &config->readwrite_a;
		else if (name[3] == 'b')
			rw = &config->readwrite_b;
		else
			return 0;
		subname = name + 4;
		if (*subname == '-')
			subname++;
	} else {
		return 0;
	}

	/* Read in the 'reg' property */
	if (read_entry(blob, node, name, &entry))
		return -FDT_ERR_NOTFOUND;

	/* Figure out what section we are dealing with, either ro or rw */
	section = lookup_section(subname);
	if (rw) {
		switch (section) {
		case SECTION_BASE:
			rw->all = entry;
			rw->block_offset = fdtdec_get_uint64(blob, node,
							"block-offset", ~0ULL);
			if (rw->block_offset == ~0ULL)
				VBDEBUG("Node '%s': bad block-offset\n", name);
			break;
		case SECTION_FIRMWARE_ID:
			rw->firmware_id = entry;
			break;
		case SECTION_VBLOCK:
			rw->vblock = entry;
			break;
		case SECTION_BOOT:
			rw->boot = entry;
			rw->boot_rwbin = entry;
			prop = fdt_getprop(blob, node, "compress", NULL);
			rw->compress = prop && (0 == strcmp(prop, "lzo")) ?
				CROS_COMPRESS_LZO : CROS_COMPRESS_NONE;
			break;
		case SECTION_ECRW:
			rw->ec_rw.image = entry;
			*ecp = &rw->ec_rw;
			break;
		default:
			return 0;
		}
	} else {
		switch (section) {
		case SECTION_GBB:
			config->readonly.gbb = entry;
			break;
		case SECTION_FMAP:
			config->readonly.fmap = entry;
			break;
		case SECTION_FIRMWARE_ID:
			config->readonly.firmware_id = entry;
			break;
		case SECTION_BOOT:
			config->readonly.boot = entry;
			break;
		case SECTION_ECRO:
			config->readonly.ec_ro.image = entry;
			*ecp = &config->readonly.ec_ro;
			break;
		case SECTION_ECRW:
			config->readonly.ec_rw.image = entry;
			*ecp = &config->readonly.ec_rw;
			break;
		default:
			return 0;
		}
	}
	*rwp = rw;

	return 0;
}

int cros_fdtdec_flashmap(const void *blob, struct twostop_fmap *config)
{
	struct fmap_firmware_entry *rw = NULL;
	struct fmap_ec_image *ec = NULL;
	struct fmap_entry entry;
	int offset;
	int depth;

	memset(config, '\0', sizeof(*config));
	offset = fdt_node_offset_by_compatible(blob, -1,
			"chromeos,flashmap");
	if (offset < 0) {
		VBDEBUG("chromeos,flashmap node is missing\n");
		return offset;
	}

	/* Read in the 'reg' property */
	if (read_entry(blob, offset, fdt_get_name(blob, offset, NULL), &entry))
		return -1;
	config->flash_base = entry.offset;

	depth = 0;
	while (offset > 0 && depth >= 0) {
		int node;

		node = fdt_next_node(blob, offset, &depth);
		if (node > 0 && depth > 0) {
			if (process_fmap_node(blob, node, depth, config,
						&rw, &ec)) {
				VBDEBUG("Failed to process Flashmap\n");
				return -1;
			}
		}
		offset = node;
	}

	return 0;
}

int cros_fdtdec_config_has_prop(const void *blob, const char *name)
{
	int nodeoffset = cros_fdtdec_config_node(blob);

	return nodeoffset >= 0 &&
		fdt_get_property(blob, nodeoffset, name, NULL) != NULL;
}

void *cros_fdtdec_alloc_region(const void *blob,
		const char *prop_name, size_t *size)
{
	int node = cros_fdtdec_config_node(blob);
	void *ptr;

	if (node < 0)
		return NULL;

	if (fdtdec_decode_region(blob, node, prop_name, &ptr, size)) {
		VBDEBUG("failed to find %s in /chromeos-config'\n", prop_name);
		return NULL;
	}

	if (!ptr)
		ptr = malloc(*size);
	else
		ptr = (char *)ptr + DRAM_BASE_ADDRESS;

	if (!ptr) {
		VBDEBUG("failed to alloc %d bytes for %s'\n", *size, prop_name);
	}
	return ptr;
}

int cros_fdtdec_memory(const void *blob, const char *name,
		struct fdt_memory *config)
{
	int node, len;
	const fdt_addr_t *cell;

	node = fdt_path_offset(blob, name);
	if (node < 0)
		return node;

	cell = fdt_getprop(blob, node, "reg", &len);
	if (cell && len == sizeof(fdt_addr_t) * 2) {
		config->start = fdt_addr_to_cpu(cell[0]);
		config->end = config->start + fdt_addr_to_cpu(cell[1]);
	} else
		return -FDT_ERR_BADLAYOUT;

	return 0;
}

int cros_fdtdec_chrome_ec(const void *blob, struct fdt_chrome_ec *config)
{
	int flash_node, node;

	node = fdtdec_next_compatible(blob, 0, COMPAT_GOOGLE_CROS_EC);
	if (node < 0) {
		VBDEBUG("Failed to find chrome-ec node'\n");
		return -1;
	}

	flash_node = fdt_subnode_offset(blob, node, "flash");
	if (flash_node < 0) {
		VBDEBUG("Failed to find flash node\n");
		return -1;
	}

	if (read_entry(blob, flash_node, "flash", &config->flash)) {
		VBDEBUG("Failed to find flash node in chrome-ec'\n");
		return -1;
	}

	config->flash_erase_value = fdtdec_get_int(blob, flash_node,
						    "erase-value", -1);

	return 0;
}

int cros_fdtdec_firmware_type(const void *blob,
			      enum cros_firmware_type *typep)
{
	const char *prop;
	int node;

	node = cros_fdtdec_config_node(blob);
	if (node < 0)
		return -1;
	prop = fdt_getprop(blob, node, "firmware-type", NULL);
	if (!prop) {
		VBDEBUG("Failed to find firmware-type in fdt'\n");
		return -1;
	}

	if (!strcmp(prop, "ro"))
		*typep = CROS_FIRMWARE_RO;
	else if (!strcmp(prop, "rw-a"))
		*typep = CROS_FIRMWARE_RW_A;
	else if (!strcmp(prop, "rw-b"))
		*typep = CROS_FIRMWARE_RW_B;
	else {
		VBDEBUG("Invalid firmware-type '%s' in fdt'\n", prop);
		return -1;
	}

	return 0;
}
