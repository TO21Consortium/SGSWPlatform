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

#include <stdlib.h>
#include <string.h>

#include <hardware/hw_auth_token.h>

#include "MobiCoreDriverApi.h"

#include "password_handle.h"
#include "tlTeeGatekeeper_Api.h"
#include "tlcTeeGatekeeper_if.h"
#include "tlcTeeGatekeeper_log.h"
#include "tci.h"

#include <utils/Log.h>
#include <cutils/log.h>

#include <time.h>
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>

#ifdef ACCESS_EFS_POSSIBLE
#define FNAME	"/efs/TEE/gk_context_"
#define FNAME_SUB	"/efs/TEE/gk_context_sub_"
#else
#define FNAME	"/data/app/gk_context_"
#define FNAME_SUB	"/data/app/gk_context_sub_"
#endif
#define SECURE_OBJECT_SIZE 108

#define SEC_TO_MS (1000)
#define NS_TO_MS (1000*1000)

/* Global definitions */
static const uint32_t gDeviceId = MC_DEVICE_ID_DEFAULT;
tciMessage_ptr     pTci = NULL;
mcSessionHandle_t  sessionHandle;

/** File path for Trusted Application */
char secureSecDispTrustedApp[] = "/system/app/mcRegistry/08130000000000000000000000000000.tlbin";

#define TEE_SESSION_CLOSED 0
#define TEE_SESSION_OPENED 1

/* va_gap for mcMap */
/* mcMap in Enroll - current_password_handle(0x0) | secure_object(0x400) | current_password(0x800) | desired_password(0xc00) | enrolled_password_handle(0x1000) */
/* mcMap in Verify - enrolled_password_handle(0x0) | secure_object(0x400) | provided_password(0x800) | auth_token(0xc00) */
#define vagap_secure_object (0x400)
#define vagap_current_password (0x800)
#define vagap_desired_password (0xc00)
#define vagap_enrolled_password_handle (0x1000)

#define vagap_provided_password (0x800)
#define vagap_auth_token (0xc00)

static uint32_t session_status = TEE_SESSION_CLOSED;

uint64_t clock_gettime_millisec(void)
{
	struct timespec time;
	uint64_t sec;
	long ns;

	clock_gettime(CLOCK_MONOTONIC_RAW, &time);

	if (time.tv_sec >= 0)
		sec = time.tv_sec;
	else
		return 0;

	ns = time.tv_nsec;
	return (sec * SEC_TO_MS) + (ns / NS_TO_MS);
}

static size_t getFileContent(const char *pPath, uint8_t **ppContent)
{
	FILE	*pStream;
	long	filesize;
	uint8_t	*content = NULL;
	/*
	* The stat function is not used (not available in WinCE).
	*/

	/* Open the file */
	pStream = fopen(pPath, "rb");
	if (pStream == NULL) {
		fprintf(stderr, "Error: Cannot open file: %s.\n", pPath);
		return 0;
	}
	if (fseek(pStream, 0L, SEEK_END) != 0) {
		fprintf(stderr, "Error: Cannot read file: %s.\n", pPath);
		goto error;
	}

	filesize = ftell(pStream);

	if (filesize < 0) {
		fprintf(stderr, "Error: Cannot get the file size: %s.\n", pPath);
		goto error;
	}

	if (filesize == 0) {
		fprintf(stderr, "Error: Empty file: %s.\n", pPath);
		goto error;
	}
	/* Set the file pointer at the beginning of the file */
	if (fseek(pStream, 0L, SEEK_SET) != 0) {
		fprintf(stderr, "Error: Cannot read file: %s.\n", pPath);
		goto error;
	}
	/* Allocate a buffer for the content */
	content = (uint8_t *)malloc(filesize);
	if (content == NULL) {
		fprintf(stderr, "Error: Cannot read file: Out of memory.\n");
		goto error;
	}

	/* Read data from the file into the buffer */
	if (fread(content, (size_t)filesize, 1, pStream) != 1) {
		fprintf(stderr, "Error: Cannot read file: %s.\n", pPath);
		goto error;
	}

	/*Close the file */
	fclose(pStream);
	*ppContent = content;
	/* Return number of bytes read */
	return (size_t)filesize;

error:
	if (content != NULL) {
		free(content);
	}
	fclose(pStream);
	return 0;
}

/**
* TEE_Open
*
* Open session to the TEE Gatekeeper trusted application
*
* @param  pSessionHandle  [out] Return pointer to the session handle
*/
static tciMessage_ptr TEE_Open(mcSessionHandle_t *SessionHandle)
{
	mcResult_t mcRet;
	uint32_t nTrustedAppLength;
	uint8_t* pTrustedAppData;
	tciMessage_t *tci = NULL;

	LOG_I("Opening <t-base device");
	mcRet = mcOpenDevice(gDeviceId);
	if (mcRet != MC_DRV_OK) {
		LOG_E("Error opening device: %d", mcRet);
		return false;
	}

	LOG_I("Allocating buffer for TCI");
	tci = (tciMessage_t *)malloc(sizeof(tciMessage_t));

	if (tci == NULL) {
		LOG_I("Allocation of TCI failed");
		//LOG_ERRNO("Allocation of TCI failed");
		return false;
	}
	memset(tci, 0x00, sizeof(tciMessage_t));
	nTrustedAppLength = getFileContent(secureSecDispTrustedApp,
						&pTrustedAppData);

	if (nTrustedAppLength == 0) {
		LOG_E("Trusted Application not found");
		free(tci);
		tci = NULL;
		return false;
	}

	LOG_I("Opening the Trusted Application session");
	memset(SessionHandle, 0, sizeof(*SessionHandle)); // Clear the session handle
	SessionHandle->deviceId = gDeviceId; // The device ID (default device is used)
	mcRet = mcOpenTrustlet(SessionHandle,
				MC_SPID_RESERVED_TEST, /* mcSpid_t */
				pTrustedAppData,
				nTrustedAppLength,
				(uint8_t*)tci,
				(uint32_t)sizeof(tciMessage_t));
	free(pTrustedAppData);
	if (MC_DRV_OK != mcRet){
		LOG_E("Open session failed: %d", mcRet);
		free(tci);
		tci= NULL;
		return false;
	}

	LOG_I("mcOpenTrustlet() succeeded");
	return (tciMessage_ptr)tci;
}


/**
 * TEE_Close
 *
 * Close session to the TEE Gatekeeper trusted application
 *
 * @param  sessionHandle  [in] Session handle
 */
static void TEE_Close(mcSessionHandle_t *pSessionHandle)
{
	mcResult_t    mcRet;

	do {
		/* Validate session handle */
		if (pSessionHandle == NULL) {
			LOG_E("TEE_Close(): Invalid session handle\n");
			break;
		}

		/* Close session */
		mcRet = mcCloseSession(pSessionHandle);
		if (MC_DRV_OK != mcRet)	{
			LOG_E("TEE_Close(): mcCloseSession returned: %d\n", mcRet);
			break;
		}

		/* Close MobiCore device */
		mcRet = mcCloseDevice(gDeviceId);
		if (MC_DRV_OK != mcRet)
			LOG_E("TEE_Close(): mcCloseDevice returned: %d\n", mcRet);

	} while (false);
}
#if 0
teeResult_t TEE_SessionTest()
{
	teeResult_t        ret  = TEE_ERR_NONE;
	tciMessage_ptr     pTci = NULL;
	mcSessionHandle_t  sessionHandle;
	mcResult_t         mcRet;

	do {
		/* Open session to the trusted application */
		pTci = TEE_Open(&sessionHandle);
		if (pTci == NULL) {
			ret = TEE_ERR_MEMORY;
			break;
	        }
	} while (false);

	/* Close session to the trusted application */
	TEE_Close(&sessionHandle);

	return ret;
}
#endif

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
		const struct gatekeeper_device * /* not used */)
{
	teeResult_t	ret  = TEE_ERR_NONE;
	mcResult_t	mcRet;
	mcBulkMap_t	mapinfo_va_mapping = {0, 0};
	uint8_t	va_mapping[1024 * 5] = {0,};
	int	fd = 0;
	char	buf[100];

	if (enrolled_password_handle_length == NULL || *enrolled_password_handle_length == 0) {
		LOG_E("enrolled_password_handle_length is NULL, or the size is zero.");
		return TEE_ERR_INVALID_INPUT;
	}

	if (retry_timeout == NULL) {
		LOG_E("retry_timeout is NULL.");
		return TEE_ERR_INVALID_INPUT;
	}

	if ((current_password_handle_length > (0x400)) || (current_password_length > (0x400)) || (desired_password_length > (0x400)))
		return TEE_ERR_INVALID_INPUT;

	do {
		if (session_status == TEE_SESSION_CLOSED) {
			/* Open session to the trusted application */
			pTci = TEE_Open(&sessionHandle);
			if (pTci == NULL) {
				ret = TEE_ERR_MEMORY;
				break;
			}
			session_status = TEE_SESSION_OPENED;
		}

		LOG_I("%s:%d mcMap - current_password_handle\n", __func__, __LINE__);
		mcRet = mcMap(&sessionHandle,
				(void*)va_mapping,
				sizeof(va_mapping),
				&mapinfo_va_mapping);
		if (MC_DRV_OK != mcRet) {
			ret = TEE_ERR_MAP;
			break;
		}

		memcpy(va_mapping, current_password_handle, current_password_handle_length);
		memcpy(va_mapping + vagap_current_password, current_password, current_password_length);
		memcpy(va_mapping + vagap_desired_password, desired_password, desired_password_length);

		/* Update TCI buffer */
		pTci->command.header.commandId = CMD_ID_TEE_ENROLL;
		pTci->gk_enroll.uid = uid;
		pTci->gk_enroll.current_password_handle_va = (uint32_t)mapinfo_va_mapping.sVirtualAddr;
		pTci->gk_enroll.current_password_handle_length = current_password_handle_length;
		pTci->gk_enroll.current_password_va = (uint32_t)mapinfo_va_mapping.sVirtualAddr + vagap_current_password;
		pTci->gk_enroll.current_password_length = current_password_length;
		pTci->gk_enroll.desired_password_va = (uint32_t)mapinfo_va_mapping.sVirtualAddr + vagap_desired_password;
		pTci->gk_enroll.desired_password_length = desired_password_length;
		pTci->gk_enroll.enrolled_password_handle_va = (uint32_t)mapinfo_va_mapping.sVirtualAddr + vagap_enrolled_password_handle;
		pTci->gk_enroll.secure_object_va = (uint32_t)mapinfo_va_mapping.sVirtualAddr + vagap_secure_object;

		/* Notify the trusted application */
		mcRet = mcNotify(&sessionHandle);
		if (MC_DRV_OK != mcRet) {
			ret = TEE_ERR_NOTIFICATION;
			break;
		}

		/* Get Time for sending TA */
		pTci->gk_enroll.nw_timestamp = clock_gettime_millisec();

		/* Wait for response from the trusted application */
		if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT)) {
			ret = TEE_ERR_NOTIFICATION;
			break;
		}

		if (RET_OK != pTci->response.header.returnCode) {
			LOG_E("TEE_GetKeyInfo(): TEE Gatekeeper trusted application returned: 0x%08x\n",
					pTci->response.header.returnCode);
			*retry_timeout = pTci->gk_enroll.retry_timeout;
			ret = TEE_ERR_FAIL;
			break;
		} else {
			/* Copy password handle from WSM */
			memcpy(enrolled_password_handle,
				va_mapping + vagap_enrolled_password_handle,
				pTci->gk_enroll.enrolled_password_handle_length);
			*enrolled_password_handle_length =
				pTci->gk_enroll.enrolled_password_handle_length;

			snprintf(buf, sizeof(buf), FNAME"%x", uid);
			if ((fd = open(buf, O_WRONLY | O_CREAT, 0600)) == -1) {
				LOG_E("Error: Cannot open file : %s\n", buf);
				ret =  TEE_ERR_FAIL;
				break;
			} else {
				write(fd, va_mapping + vagap_secure_object, SECURE_OBJECT_SIZE);
				fsync(fd);
				close(fd);
			}

			snprintf(buf, sizeof(buf), FNAME_SUB"%x", uid);
			if ((fd = open(buf, O_WRONLY | O_CREAT, 0600)) == -1) {
				LOG_E("Error: Cannot open file : %s\n", buf);
				ret =  TEE_ERR_FAIL;
				break;
			} else {
				write(fd, va_mapping + vagap_secure_object, SECURE_OBJECT_SIZE);
				fsync(fd);
				close(fd);
			}
		}
	} while (false);

	/* Removing to mapped buffer */
	memset(va_mapping, 0x00, sizeof(va_mapping));

	if (mapinfo_va_mapping.sVirtualAddr != 0) {
		mcRet = mcUnmap(&sessionHandle,
				(void*)va_mapping,
				&mapinfo_va_mapping);
		if (MC_DRV_OK != mcRet) {
			ret = TEE_ERR_MAP;
		}
	}

	return ret;
}

teeResult_t TEE_Verify(uint32_t uid,
		uint64_t challenge, const uint8_t *enrolled_password_handle,
		uint32_t enrolled_password_handle_length,
		const uint8_t *provided_password,
		uint32_t provided_password_length,
		uint8_t **auth_token, uint32_t *auth_token_length,
		bool *request_reenroll, int32_t *retry_timeout,
		const struct gatekeeper_device * /* not used */)
{
	teeResult_t	ret  = TEE_ERR_NONE;
	mcResult_t	mcRet;
	mcBulkMap_t	mapinfo_va_mapping = {0, 0};
	uint8_t	va_mapping[1024 * 4] = {0,};
	int	fd = 0;
	char	buf[100];
	ssize_t rd_size;

	if (auth_token_length == NULL) {
		LOG_E("auth_token_length is NULL.");
		return TEE_ERR_INVALID_INPUT;
	}
	if (request_reenroll == NULL) {
		LOG_E("request_reenroll is NULL.");
		return TEE_ERR_INVALID_INPUT;
	}

	if((enrolled_password_handle_length > (0x400)) || (provided_password_length > (0x400)))
		return TEE_ERR_INVALID_INPUT;

	snprintf(buf, sizeof(buf), FNAME"%x", uid);
	if ((fd = open(buf, O_RDONLY)) == -1) {
		LOG_E("Error: Cannot open file : %s\n", buf);
	} else {
		rd_size = read(fd, va_mapping + vagap_secure_object, SECURE_OBJECT_SIZE);
		if (rd_size == -1) {
			close(fd);
			return TEE_ERR_FAIL;
		}
		close(fd);
	}

	snprintf(buf, sizeof(buf), FNAME_SUB"%x", uid);
	if ((fd = open(buf, O_RDONLY))== -1) {
		LOG_E("Error: Cannot open file : %s\n", buf);
	} else {
		rd_size = read(fd, va_mapping + vagap_secure_object + SECURE_OBJECT_SIZE, SECURE_OBJECT_SIZE);
		if (rd_size == -1) {
			close(fd);
			return TEE_ERR_FAIL;
		}
		close(fd);
	}

	do {
		if (session_status == TEE_SESSION_CLOSED ) {
			/* Open session to the trusted application */
			pTci = TEE_Open(&sessionHandle);
			if (pTci == NULL) {
				ret = TEE_ERR_MEMORY;
				break;
		        }
			session_status = TEE_SESSION_OPENED;
		}

		LOG_I("%s:%d mcMap - current_password_handle\n", __func__, __LINE__);
		mcRet = mcMap(&sessionHandle,
				(void*)va_mapping,
				sizeof(va_mapping),
				&mapinfo_va_mapping);
		if (MC_DRV_OK != mcRet) {
			ret = TEE_ERR_MAP;
			break;
		}

		memcpy(va_mapping, enrolled_password_handle,
			enrolled_password_handle_length);

		memcpy(va_mapping + vagap_provided_password, provided_password,
			provided_password_length);

		/* Update TCI buffer */
		pTci->command.header.commandId = CMD_ID_TEE_VERIFY;
		pTci->gk_verify.uid = uid;
		pTci->gk_verify.challenge = challenge;
		pTci->gk_verify.enrolled_password_handle_va =
			(uint32_t)mapinfo_va_mapping.sVirtualAddr;
		pTci->gk_verify.enrolled_password_handle_length =
			enrolled_password_handle_length;
		pTci->gk_verify.provided_password_va =
			(uint32_t)mapinfo_va_mapping.sVirtualAddr + vagap_provided_password;
		pTci->gk_verify.provided_password_length =
			provided_password_length;
		pTci->gk_verify.auth_token_va =
			(uint32_t)mapinfo_va_mapping.sVirtualAddr + vagap_auth_token;
		pTci->gk_verify.auth_token_length = *auth_token_length;
		pTci->gk_verify.request_reenroll = *request_reenroll;
		pTci->gk_verify.secure_object_va =
			(uint32_t)mapinfo_va_mapping.sVirtualAddr + vagap_secure_object;

		/* Get Time for sending TA */
		pTci->gk_verify.nw_timestamp = clock_gettime_millisec();

		/* Notify the trusted application */
		mcRet = mcNotify(&sessionHandle);
		if (MC_DRV_OK != mcRet) {
			ret = TEE_ERR_NOTIFICATION;
			break;
		}

		/* Wait for response from the trusted application */
		if (MC_DRV_OK != mcWaitNotification(&sessionHandle,
							MC_INFINITE_TIMEOUT)) {
			ret = TEE_ERR_NOTIFICATION;
			break;
		}

		if (RET_OK != pTci->response.header.returnCode) {
			LOG_E("TEE_GetKeyInfo(): TEE Gatekeeper trusted application returned: 0x%08x\n",
					pTci->response.header.returnCode);
//			LOG_E("TEE_GetKeyInfo(): previous failure count : 0x%x\n", failure_count);
//			LOG_E("TEE_GetKeyInfo(): returned failure count : 0x%x\n", pTci->gk_verify.failure_count);
//			LOG_E("TEE_GetKeyInfo(): Failure count : 0x%x\n", failure_count);
			*retry_timeout = pTci->gk_verify.retry_timeout;
			ret = TEE_ERR_FAIL;
			break;
		}

		/* Copy Auth_Token from WSM */
		memcpy(*auth_token, va_mapping + vagap_auth_token,
			pTci->gk_verify.auth_token_length);

		*auth_token_length = pTci->gk_verify.auth_token_length;
		*request_reenroll = pTci->gk_verify.request_reenroll;
	} while (false);

	snprintf(buf, sizeof(buf), FNAME"%x", uid);
	if ((fd = open(buf, O_WRONLY | O_CREAT, 0600)) == -1) {
		LOG_E("Error: Cannot open file : %s\n", buf);
		ret = TEE_ERR_FAIL;
	} else {
		write(fd, va_mapping + vagap_secure_object, SECURE_OBJECT_SIZE);
		fsync(fd);
		close(fd);
	}

	snprintf(buf, sizeof(buf), FNAME_SUB"%x", uid);
	if ((fd = open(buf, O_WRONLY | O_CREAT, 0600)) == -1) {
		LOG_E("Error: Cannot open file : %s\n", buf);
		ret =  TEE_ERR_FAIL;
	} else {
		write(fd, va_mapping + vagap_secure_object, SECURE_OBJECT_SIZE);
		fsync(fd);
		close(fd);
	}

	/* Removing to mapped buffer */
	memset(va_mapping, 0x00, sizeof(va_mapping));

	if (mapinfo_va_mapping.sVirtualAddr != 0) {
		mcRet = mcUnmap(&sessionHandle,
				(void*)va_mapping,
				&mapinfo_va_mapping);
		if (MC_DRV_OK != mcRet) {
			ret = TEE_ERR_MAP;
		}
	}
	return ret;
}
