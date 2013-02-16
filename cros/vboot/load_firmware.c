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
#include <cros/common.h>
#include <cros/hasher_state.h>

#include <vboot_api.h>
#include <vboot_struct.h>

/* This can only be called after key block has been verified */
static uintptr_t firmware_body_size(const uintptr_t vblock_address)
{
	const VbKeyBlockHeader         const *keyblock;
	const VbFirmwarePreambleHeader const *preamble;

	keyblock = (VbKeyBlockHeader *)vblock_address;
	preamble = (VbFirmwarePreambleHeader *)
		(vblock_address + (uintptr_t)keyblock->key_block_size);

	return preamble->body_signature.data_size;
}

VbError_t VbExHashFirmwareBody(VbCommonParams* cparams, uint32_t firmware_index)
{
	hasher_state_t *s = cparams->caller_context;
	const int i = (firmware_index == VB_SELECT_FIRMWARE_A ? 0 : 1);
	firmware_storage_t *file = s->file;

	if (firmware_index != VB_SELECT_FIRMWARE_A &&
			firmware_index != VB_SELECT_FIRMWARE_B) {
		VBDEBUG("incorrect firmware index: %d\n", firmware_index);
		return 1;
	}

	/*
	 * The key block has been verified. It is safe now to infer the actual
	 * firmware body size from the key block.
	 */
	s->fw[i].size = firmware_body_size((uintptr_t)s->fw[i].vblock);

	if (file->read(file, s->fw[i].offset,
		       s->fw[i].size, BT_EXTRA(s->fw[i].cache))) {
		VBDEBUG("fail to read firmware: %d\n", firmware_index);
		return 1;
	}

	VbUpdateFirmwareBodyHash(cparams, s->fw[i].cache, s->fw[i].size);
	return 0;
}
