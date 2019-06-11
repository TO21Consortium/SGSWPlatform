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

#include <errno.h>
#include <string.h>
#include <stdint.h>

#include <hardware/hardware.h>
#include <hardware/gatekeeper.h>
#include <hardware/hw_auth_token.h>

#include <UniquePtr.h>

#include "tlcTeeGatekeeper_if.h"
#include "password_handle.h"

#include <utils/Log.h>
#include <cutils/log.h>

typedef UniquePtr<gatekeeper_device_t> Unique_gatekeeper_device_t;

#define MAX_PASSWORD_LEN 100

#undef GATEKEEPER_DEBUG

#ifdef GATEKEEPER_DEBUG
uint8_t temp[1000];
#endif

#ifdef GATEKEEPER_DEBUG
void dump(const uint8_t *ptr, uint32_t size)
{
	unsigned int i;

	memset(temp, 0x0, sizeof(temp));

	ALOGE("[exy_gk] DUMP=================");
	for (i = 0; i < size; i++) {
		if(i % 16 == 0){
			ALOGE("[exy_gk] %s", temp);
			memset(temp, 0x0, sizeof(temp));
		}
		sprintf((char*)(temp + ((i % 16) * 3)), "%02x ", *(ptr + i));
	}

	if (i % 16)
		ALOGE("[exy_gk] %s", temp);
}
#endif

int enroll(const struct gatekeeper_device *dev, uint32_t uid,
		const uint8_t *current_password_handle,
		uint32_t current_password_handle_length,
		const uint8_t *current_password, uint32_t current_password_length,
		const uint8_t *desired_password, uint32_t desired_password_length,
		uint8_t **enrolled_password_handle,
		uint32_t *enrolled_password_handle_length)
{
	teeResult_t teeRet = TEE_ERR_NONE;
	int32_t retry_timeout = 0;
	password_handle_t *handle;

	if (dev == NULL ||
			desired_password == NULL ||
			enrolled_password_handle == NULL ||
			enrolled_password_handle_length == NULL) {
		ALOGE("[exy_gk][%s][%d] Wrong parameter.", __func__, __LINE__);
		return -1;
	}

	if (current_password_handle == NULL || current_password_handle_length == 0 ||
			current_password == NULL || current_password_length == 0) {
		current_password_handle = NULL;
		current_password_handle_length = 0;
		current_password = NULL;
		current_password_length = 0;
	}

	if (current_password_handle_length > sizeof(struct password_handle_t) ||
			current_password_length > MAX_PASSWORD_LEN ||
			desired_password_length > MAX_PASSWORD_LEN) {
		ALOGE("[exy_gk][%s][%d] Wrong parameter.", __func__, __LINE__);
		return -1;
	}

#ifdef GATEKEEPER_DEBUG
	ALOGE("[exy_gk][%s][%d]", __func__, __LINE__);

	memset(temp, 0x0, sizeof(temp));
	if (current_password_length > sizeof(temp)) {
		 ALOGE("[exy_gk][%s][%d] current_password_length is too long.",
				 __func__, __LINE__);
		 return -1;
	}
	memcpy(temp, current_password, current_password_length);
	ALOGE("[exy_gk] current_password [%s]",temp);

	memset(temp, 0x0, sizeof(temp));
	if (desired_password_length > sizeof(temp)) {
		 ALOGE("[exy_gk][%s][%d] desired_password_length is too long.",
				 __func__, __LINE__);
		 return -1;
	}
	memcpy(temp, desired_password, desired_password_length);
	ALOGE("[exy_gk] desired_password [%s]",temp);
#endif
	if (enrolled_password_handle == NULL) {
		ALOGE("[exy_gk] enrolled_password_handle ptr is NULL. [%s][%d]", __func__, __LINE__);
		return -1;
	}
	if (enrolled_password_handle_length == NULL) {
		ALOGE("[exy_gk] enrolled_password_handle_length ptr is NULL. [%s][%d]", __func__, __LINE__);
		return -1;
	}

	// the handle will be removed in the reture process of gatekeeprd.
	handle = new password_handle_t;
	*enrolled_password_handle = (uint8_t*)handle;
	*enrolled_password_handle_length = sizeof(password_handle_t);

	if (current_password_handle_length != 0) {
#ifdef GATEKEEPER_DEBUG
		ALOGE("[exy_gk] Dump current_handle");
		dump(current_password_handle, current_password_handle_length);
#endif
	}
	teeRet = TEE_Enroll(uid, current_password_handle,
				current_password_handle_length,
				current_password,
				current_password_length,
				desired_password, desired_password_length,
				(uint8_t*)handle,
				enrolled_password_handle_length,
				&retry_timeout, dev);

	if (teeRet != TEE_ERR_NONE) {
		return (retry_timeout ? retry_timeout : -1);
	}

#ifdef GATEKEEPER_DEBUG
	ALOGE("[exy_gk] Dump new_handle");
	dump(*enrolled_password_handle, *enrolled_password_handle_length);
	ALOGE("[exy_gk] enrolled user_id [%llx]",handle->user_id);
#endif

	return 0;
}

int verify(const struct gatekeeper_device *dev, uint32_t uid,
		uint64_t challenge, const uint8_t *enrolled_password_handle,
		uint32_t enrolled_password_handle_length,
		const uint8_t *provided_password,
		uint32_t provided_password_length, uint8_t **auth_token,
		uint32_t *auth_token_length, bool *request_reenroll)
{
	teeResult_t teeRet = TEE_ERR_NONE;
	int32_t retry_timeout = 0;
	hw_auth_token_t *token;

	if (dev == NULL ||
			enrolled_password_handle == NULL ||
			provided_password == NULL ||
			auth_token == NULL ||
			auth_token_length == NULL ||
			request_reenroll == NULL) {
		ALOGE("[exy_gk][%s][%d] Wrong parameter.", __func__, __LINE__);
		return -1;
	}

	if (enrolled_password_handle_length > sizeof(struct password_handle_t) ||
			provided_password_length > MAX_PASSWORD_LEN) {
		ALOGE("[exy_gk][%s][%d] Wrong parameter.", __func__, __LINE__);
		return -1;
	}
#ifdef GATEKEEPER_DEBUG
	ALOGE("[exy_gk][%s][%d]", __func__, __LINE__);

	memset(temp, 0x0, sizeof(temp));
	if (provided_password_length > sizeof(temp)) {
		 ALOGE("[exy_gk][%s][%d] provided_password_length is too long.",
				 __func__, __LINE__);
		 return -1;
	}
	memcpy(temp, provided_password, provided_password_length);
	ALOGE("[exy_gk] provided_password [%s]",temp);
#endif
	if (auth_token == NULL) {
		ALOGE("[exy_gk] auth_token ptr is NULL. [%s][%d]", __func__, __LINE__);
		return -1;
	}

	if (auth_token_length == NULL) {
		ALOGE("[exy_gk] auth_token_length ptr is NULL. [%s][%d]", __func__, __LINE__);
		return -1;
	}

	// the handle will be removed in the return process of keymaster.
	token = new hw_auth_token_t;

	*auth_token = (uint8_t*)token;
	*auth_token_length = sizeof(hw_auth_token_t);

	teeRet = TEE_Verify(uid, challenge,
				enrolled_password_handle,
				enrolled_password_handle_length,
				provided_password, provided_password_length,
				auth_token, auth_token_length, request_reenroll,
				&retry_timeout,	dev);

	if (teeRet != TEE_ERR_NONE) {
		return (retry_timeout ? retry_timeout : -1);
	}

	return 0;
}

/* Close an opened Exynos GK instance */
static int exynos_gk_close(hw_device_t *dev)
{
#ifdef GATEKEEPER_DEBUG
	ALOGE("[exy_gk][%s][%d]", __func__, __LINE__);
#endif
	free(dev);
	return 0;
}

/*
 * Generic device handling
 */
static int exynos_gk_open(const hw_module_t *module, const char *name,
				hw_device_t **device)
{
	if (module == NULL || name == NULL) {
		ALOGE("[exy_gk][%s][%d] Wrong parameter.", __func__, __LINE__);
		return -1;
	}
#ifdef GATEKEEPER_DEBUG
	ALOGE("[exy_gk][%s][%d]", __func__, __LINE__);
#endif
	if (strncmp(name, HARDWARE_GATEKEEPER, strlen(HARDWARE_GATEKEEPER)) != 0) {
		ALOGE("[exy_gk]requested name = [%s], my name = [%s]\n",
				name, HARDWARE_GATEKEEPER);
		return -EINVAL;
	}

	Unique_gatekeeper_device_t dev(new gatekeeper_device_t);
	if (dev.get() == NULL)
		return -ENOMEM;

	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 1;
	dev->common.module = (struct hw_module_t*) module;
	dev->common.close = exynos_gk_close;

	dev->enroll = enroll;
	dev->verify = verify;
	dev->delete_user = NULL;
	dev->delete_all_users = NULL;

	*device = reinterpret_cast<hw_device_t*>(dev.release());

	return 0;
}

static struct hw_module_methods_t gatekeeper_module_methods = {
    open: exynos_gk_open,
};


struct gatekeeper_module HAL_MODULE_INFO_SYM
__attribute__ ((visibility ("default"))) = {
	.common =
	{
		.tag = HARDWARE_MODULE_TAG,
		.module_api_version = GATEKEEPER_MODULE_API_VERSION_0_1,
		.hal_api_version = HARDWARE_HAL_API_VERSION,
		.id = GATEKEEPER_HARDWARE_MODULE_ID,
		.name = HARDWARE_GATEKEEPER,
		.author = "The Android Open Source Project",
		.methods = &gatekeeper_module_methods,
		.dso = 0,
		.reserved = {},
	},
};
