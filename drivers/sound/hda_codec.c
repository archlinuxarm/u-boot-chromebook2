/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of per-board codec beeping */

#include <common.h>
#include <fdtdec.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <pci.h>
#include <hda_codec.h>

#define HDA_ICII_COMMAND_REG		0x5C
#define HDA_ICII_RESPONSE_REG		0x60
#define HDA_ICII_REG			0x64
#define HDA_ICII_BUSY			(1 << 0)
#define HDA_ICII_VALID			(1 << 1)

/* Common node IDs. */
#define HDA_ROOT_NODE			0x00

/* HDA verbs. */
#define HDA_VERB(nid, verb, param) ((nid) << 20 | (verb) << 8 | (param))

#define HDA_VERB_GET_PARAMS		0xF00
#define HDA_VERB_SET_BEEP		0x70A

/* GET_PARAMS parameter IDs. */
#define GET_PARAMS_NODE_COUNT		0x04
#define GET_PARAMS_AUDIO_GROUP_CAPS	0x08
#define GET_PARAMS_AUDIO_WIDGET_CAPS	0x09

/* Get Sub-node count fields. */
#define AUDIO_NODE_NUM_SUB_NODES(x)	(x & 0x000000ff)
#define AUDIO_NODE_FIRST_SUB_NODE(x)	((x >> 16) & 0x000000ff)

/* Get Audio Function Group Capabilities fields. */
#define AUDIO_GROUP_CAPS_BEEP_GEN	0x10000

/* Get Audio Widget Capabilities fields. */
#define AUDIO_WIDGET_TYPE_BEEP		0x7
#define AUDIO_WIDGET_TYPE(x)		(((x) >> 20) & 0x0f)

#define BEEP_FREQ_BASE			12000

/**
 *  Wait 50usec for the codec to indicate it is ready
 *  no response would imply that the codec is non-operative
 */
static int wait_for_ready(uint32_t base)
{
	/*
	 * Use a 50 usec timeout - the Linux kernel uses the
	 * same duration
	 */

	int timeout = 50;

	while (timeout--) {
		uint32_t reg32 = readl(base +  HDA_ICII_REG);

		asm("" ::: "memory");
		if (!(reg32 & HDA_ICII_BUSY))
			return 0;
		udelay(1);
	}

	printf("Audio: wait_for_ready timeout\n");
	return -1;
}

/**
 *  Wait 50usec for the codec to indicate that it accepted
 *  the previous command.  No response would imply that the code
 *  is non-operative
 */
static int wait_for_response(uint32_t base, uint32_t *response)
{
	uint32_t reg32;

	/* Send the verb to the codec */
	reg32 = readl(base + HDA_ICII_REG);
	reg32 |= HDA_ICII_BUSY | HDA_ICII_VALID;
	writel(reg32, base + HDA_ICII_REG);

	/* Use a 50 usec timeout - the Linux kernel uses the
	 * same duration
	 */

	int timeout = 50;
	while (timeout--) {
		reg32 = readl(base + HDA_ICII_REG);
		asm("" ::: "memory");
		if ((reg32 & (HDA_ICII_VALID | HDA_ICII_BUSY)) ==
				HDA_ICII_VALID) {
			if (response != NULL)
				*response = readl(base + HDA_ICII_RESPONSE_REG);
			return 0;
		}
		udelay(1);
	}

	printf("Audio: wait_for_valid timeout\n");
	return -1;
}

/* Wait for the codec to be ready, write the verb, then wait for the
 * codec to be have a valid response.
 */
static int exec_one_verb(uint32_t base, uint32_t val, uint32_t *response)
{
	if (wait_for_ready(base) == -1)
		return -1;

	writel(val, base + HDA_ICII_COMMAND_REG);

	if (wait_for_response(base, response) == -1)
		return -1;

	return 0;
}

/* Write one verb and ignore the response.
 */
static int write_one_verb(uint32_t base, uint32_t val)
{
	return exec_one_verb(base, val, NULL);
}

/* Supported sound devices.
 */
static struct pci_device_id supported[] = {
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_COUGARPOINT_HDA},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_PANTHERPOINT_HDA},
	{}
};

/* Find the base address to talk tot he HDA codec.
 */
static u32 get_hda_base(void)
{
	pci_dev_t devbusfn;
	u32 pci_mem_base;

	devbusfn = pci_find_devices(supported, 0);
	if (devbusfn < 0) {
		printf("Audio: Controller not found !\n");
		return 0;
	}

	pci_read_config_dword(devbusfn, PCI_BASE_ADDRESS_0, &pci_mem_base);
	pci_mem_base = pci_mem_to_phys(devbusfn, pci_mem_base);
	return pci_mem_base;
}

/* Gets the count of sub-nodes and node id they start at for the given
 * super-node.
 */
static uint32_t get_subnode_info(uint32_t base,
				 uint32_t nid,
				 uint32_t *num_sub_nodes,
				 uint32_t *start_sub_node_nid)
{
	int rc;
	uint32_t response;

	rc = exec_one_verb(base,
			   HDA_VERB(nid,
				    HDA_VERB_GET_PARAMS,
				    GET_PARAMS_NODE_COUNT),
			   &response);
	if (rc < 0) {
		printf("Audio: Error reading sub-node info %d.\n", nid);
		return rc;
	}

	*num_sub_nodes = AUDIO_NODE_NUM_SUB_NODES(response);
	*start_sub_node_nid = AUDIO_NODE_FIRST_SUB_NODE(response);
	return 0;
}

/* Searches the audio group for a node that supports beeping.
 */
static uint32_t find_beep_node_in_group(uint32_t base, uint32_t group_nid)
{
	int rc;
	uint32_t node_count = 0;
	uint32_t current_nid = 0;
	uint32_t end_nid;
	uint32_t response;

	rc = get_subnode_info(base, group_nid, &node_count, &current_nid);
	if (rc < 0)
		return 0;

	end_nid = current_nid + node_count;
	while (current_nid < end_nid) {
		rc = exec_one_verb(base,
				   HDA_VERB(current_nid,
					    HDA_VERB_GET_PARAMS,
					    GET_PARAMS_AUDIO_WIDGET_CAPS),
				   &response);
		if (rc < 0) {
			printf("Audio: Error reading widget caps.\n");
			return 0;
		}

		if(AUDIO_WIDGET_TYPE(response) == AUDIO_WIDGET_TYPE_BEEP)
			return current_nid;

		current_nid++;
	}

	return 0; /* no beep node found. */
}

/* Checks if the given audio group contains a beep generator.
 */
static int audio_group_has_beep_node(uint32_t base, uint32_t nid)
{
	int rc;
	uint32_t response;

	rc = exec_one_verb(base,
			   HDA_VERB(nid,
				    HDA_VERB_GET_PARAMS,
				    GET_PARAMS_AUDIO_GROUP_CAPS),
			   &response);
	if (rc < 0) {
		printf("Audio: Error reading audio group caps %d.\n", nid);
		return 0;
	}

	return !!(response & AUDIO_GROUP_CAPS_BEEP_GEN);
}

/* Finds the nid of the beep node if it exists, if not return 0. Starts at the
 * root node, for each sub-node checks if the group contains a beep node.  If
 * the group contains a beep node, polls each node in the group until it is
 * found.
 */
static uint32_t get_hda_beep_nid(uint32_t base)
{
	int rc;
	uint32_t node_count = 0;
	uint32_t current_nid = 0;
	uint32_t end_nid;

	/* If the field exists, use the beep nid set in the fdt */
	rc = fdtdec_get_config_int(gd->fdt_blob, "hda-codec-beep-nid", -1);
	if (rc != -1)
		return rc;

	rc = get_subnode_info(base, HDA_ROOT_NODE, &node_count, &current_nid);
	if (rc < 0)
		return rc;

	end_nid = current_nid + node_count;
	while (current_nid < end_nid) {
		if (audio_group_has_beep_node(base, current_nid))
			return find_beep_node_in_group(base, current_nid);
		current_nid++;
	}

	return 0; /* no beep node found. */
}

/* Sets the beep generator with the given divisor.  pass 0 to disable beep.
 */
static void set_beep_divisor(uint8_t divider)
{
	uint32_t base;
	uint32_t beep_nid;

	base = get_hda_base();
	beep_nid = get_hda_beep_nid(base);
	if (beep_nid <= 0) {
		printf("Audio: Failed to find a beep-capable node.\n");
		return;
	}
	write_one_verb(base,
		       HDA_VERB(beep_nid, HDA_VERB_SET_BEEP, divider));
}

void enable_beep_hda(uint32_t frequency)
{
	uint8_t divider_val;

	if (0 == frequency)
		divider_val = 0;	/* off */
	else if (frequency > BEEP_FREQ_BASE)
		divider_val = 1;
	else if (frequency < BEEP_FREQ_BASE / 0xFF)
		divider_val = 0xff;
	else
		divider_val = (uint8_t)(0xFF & (BEEP_FREQ_BASE / frequency));

	set_beep_divisor(divider_val);
}

void disable_beep_hda(void)
{
	set_beep_divisor(0);
}

int sound_play(uint32_t msec, uint32_t frequency)
{
	enable_beep_hda(frequency);
	udelay(msec * 1000);
	disable_beep_hda();

	return 0;
}

int sound_init(const void *blob)
{
	return 0;
}
