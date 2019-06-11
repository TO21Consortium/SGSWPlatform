/*
 * Copyright (C) 2015, Samsung Electronics Co., LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed toggle an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __TLTEEGATEKEEPERAPI_H__
#define __TLTEEGATEKEEPERAPI_H__

#include "tci.h"

/* Command ID's */
#define CMD_ID_TEE_ENROLL		1
#define CMD_ID_TEE_VERIFY		2

/* Command message */
typedef struct {
	tciCommandHeader_t  header;
	uint32_t            len;
} command_t;


/* Response structure */
typedef struct {
	tciResponseHeader_t header;
	uint32_t            len;
} response_t;

typedef struct {
	/* in params */
	uint32_t uid;
	uint32_t current_password_handle_va;
	uint32_t current_password_handle_length;
	uint32_t current_password_va;
	uint32_t current_password_length;
	uint32_t desired_password_va;
	uint32_t desired_password_length;
	uint64_t nw_timestamp; /* deprecated, it should be changed to secure timer */
	/* out params */
	uint32_t enrolled_password_handle_va;
	uint32_t enrolled_password_handle_length;
	int32_t retry_timeout;
	/* Wrapping */
	uint32_t secure_object_va;
} gk_enroll_t;

typedef struct {
	/* in params */
	uint32_t uid;
	uint64_t challenge;
	uint32_t enrolled_password_handle_va;
	uint32_t enrolled_password_handle_length;
	uint32_t provided_password_va;
	uint32_t provided_password_length;
	uint64_t nw_timestamp; /* deprecated, it should be changed to secure timer */
	/* out params */
	uint32_t auth_token_va;
	uint32_t auth_token_length;
	int32_t retry_timeout;
	/* in / out params */
	bool request_reenroll;
	/* Wrapping */
	uint32_t secure_object_va;
} gk_verify_t;

/* TCI message data. */
typedef struct {
	union {
		command_t     command;
		response_t    response;
	};
	union {
		gk_verify_t  gk_verify;
		gk_enroll_t  gk_enroll;
	};
} tciMessage_t, *tciMessage_ptr;

/* Overall TCI structure */
typedef struct {
	tciMessage_t message;
} tci_t;

/* Trustlet UUID */
#define TEE_GATEKEEPER_TL_UUID {{0x08, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}

#endif // __TLTEEGATEKEEPERAPI_H__
