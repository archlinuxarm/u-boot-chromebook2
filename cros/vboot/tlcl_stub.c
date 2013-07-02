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
#include <config.h>
#include <tpm.h>

/* Import the header files from vboot_reference. */
#include <tss_constants.h>
#include <vboot_api.h>

VbError_t VbExTpmInit(void)
{
	if (tis_init())
		return TPM_E_IOERROR;
	/* tpm_lite lib doesn't call VbExTpmOpen after VbExTpmInit. */
	return VbExTpmOpen();
}

VbError_t VbExTpmClose(void)
{
	if (tis_close())
		return TPM_E_IOERROR;
	return TPM_SUCCESS;
}

VbError_t VbExTpmOpen(void)
{
	if (tis_open())
		return TPM_E_IOERROR;
	return TPM_SUCCESS;
}

VbError_t VbExTpmSendReceive(const uint8_t* request, uint32_t request_length,
		uint8_t* response, uint32_t* response_length)
{
	size_t resp_len = *response_length;
	int ret;

	ret = tis_sendrecv(request, request_length, response, &resp_len);
	*response_length = resp_len;
	if (ret)
		return TPM_E_IOERROR;
	return TPM_SUCCESS;
}
