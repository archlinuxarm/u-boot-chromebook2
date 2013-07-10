/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/*
 * Implementation of APIs provided by firmware and exported to vboot_reference.
 * They includes debug output, memory allocation, timer and delay, etc.
 */

#include <common.h>
#include <asm/global_data.h>
#include <malloc.h>
#include <cros/common.h>
#include <cros/cros_init.h>
#include <cros/pcbeep.h>
#include <cros/power_management.h>
#include <sound.h>

/* Import the definition of vboot_wrapper interfaces. */
#include <vboot_api.h>

DECLARE_GLOBAL_DATA_PTR;

#define TICKS_PER_MSEC		(CONFIG_SYS_HZ / 1000)
#define MAX_MSEC_PER_LOOP	((uint32_t)((UINT32_MAX / TICKS_PER_MSEC) / 2))

static void system_abort(void)
{
	/* Wait for 3 seconds to let users see error messages and reboot. */
	VbExSleepMs(3000);
	cold_reboot();
}

void *cros_memalign_cache(size_t n)
{
	static unsigned int dcache_line_size;

	if (!dcache_line_size) {
		/* Select cache line size based on available information */
#ifdef CONFIG_ARM
		dcache_line_size = dcache_get_line_size();
#elif defined CACHE_LINE_SIZE
		dcache_line_size = CACHE_LINE_SIZE;
#else
		dcache_line_size = __BIGGEST_ALIGNMENT__;
#endif
	}

	return memalign(dcache_line_size, n);
}

void VbExError(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	system_abort();
}

void VbExDebug(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
}

void *VbExMalloc(size_t size)
{
	void *ptr = cros_memalign_cache(size);
	if (!ptr) {
		VbExError("Internal malloc error.");
	}
	return ptr;
}

void VbExFree(void *ptr)
{
	free(ptr);
}

void VbExSleepMs(uint32_t msec)
{
	uint32_t delay, start;

	/*
	 * Can't use entire UINT32_MAX range in the max delay, because it
	 * pushes get_timer() too close to wraparound. So use /2.
	 */
	while(msec > MAX_MSEC_PER_LOOP) {
		VbExSleepMs(MAX_MSEC_PER_LOOP);
		msec -= MAX_MSEC_PER_LOOP;
	}

	delay = msec * TICKS_PER_MSEC;
	start = get_timer(0);

	while (get_timer(start) < delay)
		udelay(100);
}

VbError_t VbExBeep(uint32_t msec, uint32_t frequency)
{
#if defined CONFIG_SOUND
	if (sound_init(gd->fdt_blob)) {
		VBDEBUG("Failed to initialize sound.\n");
		return VBERROR_NO_SOUND;
	}

	VBDEBUG("About to beep for %d ms at %d Hz.\n", msec, frequency);
	if (msec) {
		if (frequency) {
			if (sound_play(msec, frequency)) {
				VBDEBUG("Failed to play beep.\n");
				return VBERROR_NO_SOUND;
			}
		} else {
			VbExSleepMs(msec);
		}
	}

	return VBERROR_SUCCESS;
#else
	VbExSleepMs(msec);
	VBDEBUG("Beep!\n");
	return VBERROR_NO_SOUND;
#endif
}

int Memcmp(const void *src1, const void *src2, size_t n)
{
	return memcmp(src1, src2, n);
}

void *Memcpy(void *dest, const void *src, uint64_t n)
{
	return memcpy(dest, src, (size_t) n);
}

void *Memset(void *d, const uint8_t c, uint64_t n)
{
	return memset(d, c, n);
}

uint64_t VbExGetTimer(void)
{
	return timer_get_us();
}
