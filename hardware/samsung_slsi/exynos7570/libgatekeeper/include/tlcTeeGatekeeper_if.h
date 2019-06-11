/*
 * Copyright (c) 2013-2014 TRUSTONIC LIMITED
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the TRUSTONIC LIMITED nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __TLCTEEGATEKEEPERIF_H__
#define __TLCTEEGATEKEEPERIF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* error codes */
typedef enum
{
    TEE_ERR_NONE             = 0,
    TEE_ERR_FAIL             = 1,
    TEE_ERR_INVALID_BUFFER   = 2,
    TEE_ERR_BUFFER_TOO_SMALL = 3,
    TEE_ERR_NOT_IMPLEMENTED  = 4,
    TEE_ERR_SESSION          = 5,
    TEE_ERR_MC_DEVICE        = 6,
    TEE_ERR_NOTIFICATION     = 7,
    TEE_ERR_MEMORY           = 8,
    TEE_ERR_MAP              = 9,
    TEE_ERR_UNMAP            = 10,
    TEE_ERR_INVALID_INPUT    = 11
    /* more can be added as required */
} teeResult_t;

teeResult_t TEE_Enroll(uint32_t uid,
		const uint8_t *current_password_handle,
		uint32_t current_password_handle_length,
		const uint8_t *current_password,
		uint32_t current_password_length,
		const uint8_t *desired_password,
		uint32_t desired_password_length,
		uint8_t *enrolled_password_handle,
		uint32_t *enrolled_password_handle_length,
		int32_t *retry_timeout,
		const struct gatekeeper_device * /* not used */);

teeResult_t TEE_Verify(uint32_t uid,
		uint64_t challenge, const uint8_t *enrolled_password_handle,
		uint32_t enrolled_password_handle_length,
		const uint8_t *provided_password,
		uint32_t provided_password_length,
		uint8_t **auth_token, uint32_t *auth_token_length,
		bool *request_reenroll, int32_t *retry_timeout,
		const struct gatekeeper_device * /* not used */);

#ifdef __cplusplus
}
#endif

#endif // __TLCTEEGATEKEEPERIF_H__
