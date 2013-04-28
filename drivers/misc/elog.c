/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 The ChromiumOS Authors.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 */

#include <common.h>
#include <cros/cros_fdtdec.h>
#include <cros/firmware_storage.h>
#include <cros/fmap.h>
#include <elog.h>
#include <fdtdec.h>
#include <malloc.h>
#include <rtc.h>
#include <spi.h>
#include <spi_flash.h>

#include "elog_internal.h"

DECLARE_GLOBAL_DATA_PTR;

enum {
	SF_DEFAULT_SPEED = 1000000,
};

/*
 * Static variables for ELOG state
 */
static struct elog_area *elog_area;
static u16 total_size;
static u16 log_size;
static u32 flash_base;
static u32 shrink_size;
static u32 full_threshold;

static enum elog_area_state area_state;
static enum elog_header_state header_state;
static enum elog_event_buffer_state event_buffer_state;

static u16 next_event_offset;
static u16 event_count;

static int elog_initialized;
static struct spi_flash *elog_spi;

/*
 * Convert a memory mapped flash address into a flash offset
 */
static inline u32 elog_flash_address_to_offset(u8 *address)
{
	if (!elog_spi)
		return 0;
	return (u32)address - ((u32)~0UL - elog_spi->size + 1);
}

/*
 * Convert a flash offset into a memory mapped flash address
 */
static inline u8 *elog_flash_offset_to_address(u32 offset)
{
	if (!elog_spi)
		return NULL;
	return (u8 *)((u32)~0UL - elog_spi->size + 1 + offset);
}

/*
 * Pointer to an event log header in the event data area
 */
static inline struct event_header*
elog_get_event_base(u32 offset)
{
	return (struct event_header *)&elog_area->data[offset];
}

/*
 * Update the checksum at the last byte
 */
static void elog_update_checksum(struct event_header *event, u8 checksum)
{
	u8 *event_data = (u8 *)event;
	event_data[event->length - 1] = checksum;
}

/*
 * Simple byte checksum for events
 */
static u8 elog_checksum_event(struct event_header *event)
{
	u8 index, checksum = 0;
	u8 *data = (u8 *)event;

	for (index = 0; index < event->length; index++)
		checksum += data[index];
	return checksum;
}

/*
 * Check if a raw buffer is filled with ELOG_TYPE_EOL byte
 */
static int elog_is_buffer_clear(void *base, u32 size)
{
	u8 *current = base;
	u8 *end = current + size;

	debug("elog_is_buffer_clear(base=0x%p size=%u)\n", base, size);

	for (; current != end; current++) {
		if (*current != ELOG_TYPE_EOL)
			return 0;
	}
	return 1;
}

/*
 * Check that the ELOG area has been initialized and is valid.
 */
static int elog_is_area_valid(void)
{
	debug("elog_is_area_valid()\n");

	if (area_state != ELOG_AREA_HAS_CONTENT)
		return 0;
	if (header_state != ELOG_HEADER_VALID)
		return 0;
	if (event_buffer_state != ELOG_EVENT_BUFFER_OK)
		return 0;
	return 1;
}

/*
 * Verify the contents of an ELOG Header structure
 * Returns 1 if the header is valid, 0 otherwise
 */
static int elog_is_header_valid(struct elog_header *header)
{
	debug("elog_is_header_valid()\n");

	if (header->magic != ELOG_SIGNATURE) {
		printf("ELOG: header magic 0x%X != 0x%X\n",
		       header->magic, ELOG_SIGNATURE);
		return 0;
	}
	if (header->version != ELOG_VERSION) {
		printf("ELOG: header version %u != %u\n",
		       header->version, ELOG_VERSION);
		return 0;
	}
	if (header->header_size != sizeof(*header)) {
		printf("ELOG: header size mismatch %u != %u\n",
		       header->header_size, sizeof(*header));
		return 0;
	}
	return 1;
}

/*
 * Validate the event header and data.
 */
static int elog_is_event_valid(u32 offset)
{
	struct event_header *event;

	event = elog_get_event_base(offset);
	if (!event)
		return 0;

	/* Validate event length */
	if ((offsetof(struct event_header, type) +
	     sizeof(event->type) - 1 + offset) >= log_size)
		return 0;

	/* End of event marker has been found */
	if (event->type == ELOG_TYPE_EOL)
		return 0;

	/* Check if event fits in area */
	if ((offsetof(struct event_header, length) +
	     sizeof(event->length) - 1 + offset) >= log_size)
		return 0;

	/*
	 * If the current event length + the current offset exceeds
	 * the area size then the event area is corrupt.
	 */
	if ((event->length + offset) >= log_size)
		return 0;

	/* Event length must be at least header size + checksum */
	if (event->length < (sizeof(*event) + 1))
		return 0;

	/* If event checksum is invalid the area is corrupt */
	if (elog_checksum_event(event) != 0)
		return 0;

	/* Event is valid */
	return 1;
}

/*
 * Scan the event area and validate each entry and update the ELOG state.
 */
static void elog_update_event_buffer_state(void)
{
	u32 count = 0;
	u32 offset = 0;
	struct event_header *event;

	debug("elog_update_event_buffer_state()\n");

	/* Go through each event and validate it */
	while (1) {
		event = elog_get_event_base(offset);

		/* Do not de-reference anything past the area length */
		if ((offsetof(struct event_header, type) +
		     sizeof(event->type) - 1 + offset) >= log_size) {
			event_buffer_state = ELOG_EVENT_BUFFER_CORRUPTED;
			break;
		}

		/* The end of the event marker has been found */
		if (event->type == ELOG_TYPE_EOL)
			break;

		/* Validate the event */
		if (!elog_is_event_valid(offset)) {
			event_buffer_state = ELOG_EVENT_BUFFER_CORRUPTED;
			break;
		}

		/* Move to the next event */
		count++;
		offset += event->length;
	}

	/* Ensure the remaining buffer is empty */
	if (!elog_is_buffer_clear(&elog_area->data[offset], log_size - offset))
		event_buffer_state = ELOG_EVENT_BUFFER_CORRUPTED;

	/* Update ELOG state */
	event_count = count;
	next_event_offset = offset;
}

static void elog_scan_flash(void)
{
	debug("elog_scan_flash()\n");

	area_state = ELOG_AREA_UNDEFINED;
	header_state = ELOG_HEADER_INVALID;
	event_buffer_state = ELOG_EVENT_BUFFER_OK;

	/* Fill memory buffer by reading from SPI */
	spi_flash_read(elog_spi, flash_base, total_size, elog_area);

	next_event_offset = 0;
	event_count = 0;

	/* Check if the area is empty or not */
	if (elog_is_buffer_clear(elog_area, total_size)) {
		area_state = ELOG_AREA_EMPTY;
		return;
	}

	area_state = ELOG_AREA_HAS_CONTENT;

	/* Validate the header */
	if (!elog_is_header_valid(&elog_area->header)) {
		header_state = ELOG_HEADER_INVALID;
		return;
	}

	header_state = ELOG_HEADER_VALID;
	elog_update_event_buffer_state();
}

static void elog_prepare_empty(void)
{
	struct elog_header *header;

	debug("elog_prepare_empty()\n");

	/* Write out the header */
	header = &elog_area->header;
	header->magic = ELOG_SIGNATURE;
	header->version = ELOG_VERSION;
	header->header_size = sizeof(struct elog_header);
	header->reserved[0] = ELOG_TYPE_EOL;
	header->reserved[1] = ELOG_TYPE_EOL;
	spi_flash_write(elog_spi, flash_base, header->header_size, header);

	elog_scan_flash();
}

/*
 * Shrink the log, deleting old entries and moving the
 * remining ones to the front of the log.
 */
static int elog_shrink(void)
{
	struct event_header *event;
	u16 discard_count = 0;
	u16 offset = 0;
	u16 new_size = 0;

	debug("elog_shrink()\n");

	if (next_event_offset < shrink_size)
		return 0;

	while (1) {
		/* Next event has exceeded constraints */
		if (offset > shrink_size)
			break;

		event = elog_get_event_base(offset);

		/* Reached the end of the area */
		if (!event || event->type == ELOG_TYPE_EOL)
			break;

		offset += event->length;
		discard_count++;
	}

	new_size = next_event_offset - offset;
	memmove(&elog_area->data[0], &elog_area->data[offset], new_size);
	memset(&elog_area->data[new_size], ELOG_TYPE_EOL, log_size - new_size);

	spi_flash_erase(elog_spi, flash_base, total_size);
	spi_flash_write(elog_spi, flash_base, total_size, elog_area);
	elog_scan_flash();

	/* Add clear event */
	elog_add_event_word(ELOG_TYPE_LOG_CLEAR, offset);

	return 0;
}

/*
 * Clear the entire event log
 */
int elog_clear(void)
{
	debug("elog_clear()\n");

	/* Make sure ELOG structures are initialized */
	if (elog_init() < 0)
		return -1;

	/* Erase flash area */
	spi_flash_erase(elog_spi, flash_base, total_size);
	elog_prepare_empty();

	if (!elog_is_area_valid())
		return -1;

	/* Log the clear event */
	elog_add_event_word(ELOG_TYPE_LOG_CLEAR, total_size);

	return 0;
}

static struct spi_flash *elog_find_flash(void)
{
	static struct twostop_fmap fmap;
	firmware_storage_t file;
	const void *blob = gd->fdt_blob;
	int node;

	debug("elog_find_flash()\n");

	node = cros_fdtdec_config_node(blob);
	if (node < 0) {
		debug("Couldn't find config node.\n");
		return NULL;
	}
	node = fdtdec_lookup_phandle(blob, node, "firmware-storage");
	if (node < 0) {
		debug("Couldn't find elog info in firmware-storage.\n");
		return NULL;
	}
	shrink_size = fdtdec_get_int(blob, node, "elog-shrink-size", 0x400);
	full_threshold = fdtdec_get_int(blob, node, "elog-full-threshold",
					0xc00);

	if (firmware_storage_open_spi(&file)) {
		debug("Failed to open firmware storage.\n");
		return NULL;
	}

	/* Find the ELOG base and size in FMAP */
	if (!fmap.readonly.fmap.length &&
	    cros_fdtdec_flashmap(blob, &fmap)) {
		debug("failed to decode fmap\n");
		return NULL;
	}
	total_size = fmap.elog.length;
	flash_base = fmap.elog.offset;
	if (!total_size) {
		printf("ELOG: Unable to find RW_ELOG in FMAP\n");
		return NULL;
	}
	log_size = total_size - sizeof(struct elog_header);
	return file.context;
}

/*
 * Event log main entry point
 */
int elog_init(void)
{
	if (elog_initialized)
		return 0;

	debug("elog_init()\n");

	/* Prepare SPI */
	elog_spi = elog_find_flash();
	if (elog_spi == 0) {
		printf("ELOG: Unable to find SPI flash\n");
		return -1;
	}

	/* Set up the backing store */
	elog_area = malloc(total_size);
	if (!elog_area) {
		printf("ELOG: Unable to allocate backing store\n");
		return -1;
	}

	/* Load the log from flash */
	elog_scan_flash();

	/* Prepare the flash if necessary */
	if (header_state == ELOG_HEADER_INVALID ||
	    event_buffer_state == ELOG_EVENT_BUFFER_CORRUPTED) {
		/* If the header is invalid or the events are corrupted,
		 * no events can be salvaged so erase the entire area. */
		printf("ELOG: flash area invalid\n");
		elog_spi->erase(elog_spi, flash_base, total_size);
		elog_prepare_empty();
	}

	if (area_state == ELOG_AREA_EMPTY)
		elog_prepare_empty();

	if (!elog_is_area_valid()) {
		printf("ELOG: Unable to prepare flash\n");
		return -1;
	}

	elog_initialized = 1;

	printf("ELOG: FLASH @0x%p [SPI 0x%08x]\n",
	       elog_area, flash_base);

	printf("ELOG: area is %d bytes, full threshold %d, shrink size %d\n",
	       total_size, full_threshold, shrink_size);

	/* Log a clear event if necessary */
	if (event_count == 0)
		elog_add_event_word(ELOG_TYPE_LOG_CLEAR, total_size);

	/* Shrink the log if we are getting too full */
	if (next_event_offset >= full_threshold)
		elog_shrink();

	return 0;
}

/*
 * Populate timestamp in event header with current time
 */
static void elog_fill_timestamp(struct event_header *event)
{
	struct rtc_time time;

	rtc_get(&time);
	event->second = bin2bcd(time.tm_sec);
	event->minute = bin2bcd(time.tm_min);
	event->hour   = bin2bcd(time.tm_hour);
	event->day    = bin2bcd(time.tm_mday);
	event->month  = bin2bcd(time.tm_mon);
	event->year   = bin2bcd(time.tm_year % 100);

	/* Basic sanity check of expected ranges */
	if (event->month > 0x12 || event->day > 0x31 || event->hour > 0x23 ||
	    event->minute > 0x59 || event->second > 0x59) {
		event->year   = 0;
		event->month  = 0;
		event->day    = 0;
		event->hour   = 0;
		event->minute = 0;
		event->second = 0;
	}
}

/*
 * Add an event to the log
 */
void elog_add_event_raw(u8 event_type, void *data, u8 data_size)
{
	struct event_header *event;
	u8 event_size;
	int offset;

	debug("elog_add_event_raw(type=%X)\n", event_type);

	/* Make sure ELOG structures are initialized */
	if (elog_init() < 0)
		return;

	/* Header + Data + Checksum */
	event_size = sizeof(*event) + data_size + 1;
	if (event_size > MAX_EVENT_SIZE) {
		printf("ELOG: Event(%X) data size too big (%d)\n",
		       event_type, event_size);
		return;
	}

	/* Make sure event data can fit */
	if ((next_event_offset + event_size) >= log_size) {
		printf("ELOG: Event(%X) does not fit\n",
		       event_type);
		return;
	}

	/* Fill out event data */
	event = elog_get_event_base(next_event_offset);
	event->type = event_type;
	event->length = event_size;
	elog_fill_timestamp(event);

	if (data_size)
		memcpy(&event[1], data, data_size);

	/* Zero the checksum byte and then compute checksum */
	elog_update_checksum(event, 0);
	elog_update_checksum(event, -(elog_checksum_event(event)));

	/* Update the ELOG state */
	event_count++;

	offset = offsetof(struct elog_area, data) + next_event_offset;
	elog_spi->write(elog_spi, flash_base + offset, event_size, event);

	next_event_offset += event_size;

	printf("ELOG: Event(%X) added with size %d\n", event_type, event_size);

	/* Shrink the log if we are getting too full */
	if (next_event_offset >= full_threshold)
		elog_shrink();
}

void elog_add_event(u8 event_type)
{
	elog_add_event_raw(event_type, NULL, 0);
}

void elog_add_event_byte(u8 event_type, u8 data)
{
	elog_add_event_raw(event_type, &data, sizeof(data));
}

void elog_add_event_word(u8 event_type, u16 data)
{
	elog_add_event_raw(event_type, &data, sizeof(data));
}

void elog_add_event_dword(u8 event_type, u32 data)
{
	elog_add_event_raw(event_type, &data, sizeof(data));
}

void elog_add_event_wake(u8 source, u32 instance)
{
	struct elog_event_data_wake wake = {
		.source = source,
		.instance = instance
	};
	elog_add_event_raw(ELOG_TYPE_WAKE_SOURCE, &wake, sizeof(wake));
}
