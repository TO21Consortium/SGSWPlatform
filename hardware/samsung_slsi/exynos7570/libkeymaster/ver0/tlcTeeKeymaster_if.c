/*
 * Copyright (c) 2013-2015 TRUSTONIC LIMITED
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

#include <stdlib.h>
#include <string.h>

#include "MobiCoreDriverApi.h"
#include "tlTeeKeymaster_Api.h"
#include "tlcTeeKeymaster_if.h"
#include "buildTag.h"

#define LOG_TAG "TlcTeeKeyMaster"
#include "log.h"

/* Global definitions */
static const __attribute__((used)) char* buildtag = MOBICORE_COMPONENT_BUILD_TAG;
static const uint32_t gDeviceId = MC_DEVICE_ID_DEFAULT;
static const mcUuid_t gUuid = TEE_KEYMASTER_TL_UUID;

/**
 * TEE_Open
 *
 * Open session to the TEE Keymaster trusted application
 *
 * @param  pSessionHandle  [out] Return pointer to the session handle
 */
static tciMessage_ptr TEE_Open(
    mcSessionHandle_t *pSessionHandle
){
    tciMessage_ptr pTci = NULL;
    mcResult_t     mcRet;

    do
    {
        /* Validate session handle */
        if (pSessionHandle == NULL)
        {
            LOG_E("TEE_Open(): Invalid session handle\n");
            break;
        }

        /* Initialize session handle data */
        memset(pSessionHandle, 0, sizeof(mcSessionHandle_t));

        /* Open MobiCore device */
        mcRet = mcOpenDevice(gDeviceId);
        if ((MC_DRV_OK != mcRet) &&
            (MC_DRV_ERR_DEVICE_ALREADY_OPEN != mcRet))
        {
            LOG_E("TEE_Open(): mcOpenDevice returned: %d\n", mcRet);
            break;
        }

        /* Allocating WSM for TCI */
        mcRet = mcMallocWsm(gDeviceId, 0, sizeof(tciMessage_t), (uint8_t **) &pTci, 0);
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("TEE_Open(): mcMallocWsm returned: %d\n", mcRet);
            break;
        }

        /* Open session the TEE Keymaster trusted application */
        pSessionHandle->deviceId = gDeviceId;
        mcRet = mcOpenSession(pSessionHandle,
                              &gUuid,
                              (uint8_t *) pTci,
                              (uint32_t) sizeof(tciMessage_t));
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("TEE_Open(): mcOpenSession returned: %d\n", mcRet);
            break;
        }

    } while (false);

    LOG_I("TEE_Open(): returning pointer to TCI buffer: 0x%p\n", pTci);

    return pTci;
}


/**
 * TEE_Close
 *
 * Close session to the TEE Keymaster trusted application
 *
 * @param  sessionHandle  [in] Session handle
 */
static void TEE_Close(
    mcSessionHandle_t *pSessionHandle
){
    mcResult_t    mcRet;

    do
    {
        /* Validate session handle */
        if (pSessionHandle == NULL)
        {
            LOG_E("TEE_Close(): Invalid session handle\n");
            break;
        }

        /* Close session */
        mcRet = mcCloseSession(pSessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("TEE_Close(): mcCloseSession returned: %d\n", mcRet);
            break;
        }

        /* Close MobiCore device */
        mcRet = mcCloseDevice(gDeviceId);
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("TEE_Close(): mcCloseDevice returned: %d\n", mcRet);
        }

    } while (false);
}


/**
 * TEE_RSAGenerateKeyPair
 *
 * Generates RSA key pair and returns key pair data as wrapped object
 *
 * @param  keyType        [in]  Key pair type. RSA or RSACRT
 * @param  keyData        [in]  Pointer to the key data buffer
 * @param  keyDataLength  [in]  Key data buffer length
 * @param  keySize        [in]  Key size
 * @param  exponent       [in]  Exponent number
 * @param  soLen          [out] Key data secure object length
 */
teeResult_t TEE_RSAGenerateKeyPair(
    teeRsaKeyPairType_t keyType,
    uint8_t*            keyData,
    uint32_t            keyDataLength,
    uint32_t            keySize,
    uint32_t            exponent,
    uint32_t*           soLen
){
    teeResult_t         ret = TEE_ERR_NONE;
    tciMessage_ptr      pTci = NULL;
    mcSessionHandle_t   sessionHandle;
    mcBulkMap_t         mapInfo = {0};
    mcResult_t          mcRet;

    /* Validate pointers */
    if ((keyData == NULL) || (soLen == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Validate key pair type */
        if ((keyType != TEE_KEYPAIR_RSA) &&
            (keyType != TEE_KEYPAIR_RSACRT))
        {
            ret = TEE_ERR_INVALID_INPUT;
            break;
        }

        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, keyData, keyDataLength, &mapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_RSA_GEN_KEY_PAIR;
        pTci->rsa_gen_key.type         = keyType;
        pTci->rsa_gen_key.key_size     = keySize;
        pTci->rsa_gen_key.key_data     = (uint32_t)mapInfo.sVirtualAddr;
        pTci->rsa_gen_key.key_data_len = keyDataLength;
        pTci->rsa_gen_key.exponent     = exponent;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("mcNotify failed with %x\n", mcRet);
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        mcRet = mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT);
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("TEE_RSAGenerateKeyPair(): mcWaitNotification failed with %x\n", mcRet);
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_RSAGenerateKeyPair(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Update secure object length */
        *soLen =  pTci->rsa_gen_key.so_len;

    } while (false);

    /* Unmap memory */
    if (mapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, keyData, &mapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}


/**
 * TEE_RSASign
 *
 * Signs given plain data and returns signature data
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  plainData        [in]  Pointer to plain data to be signed
 * @param  plainDataLength  [in]  Plain data length
 * @param  signatureData    [out] Pointer to signature data
 * @param  signatureDataLength  [out] Signature data length
 * @param  algorithm        [in]  RSA signature algorithm
 */
teeResult_t TEE_RSASign(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  plainData,
    const uint32_t  plainDataLength,
    uint8_t*        signatureData,
    uint32_t*       signatureDataLength,
    teeRsaSigAlg_t  algorithm
){
    teeResult_t        ret = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo       = {0};
    mcBulkMap_t        plainMapInfo     = {0};
    mcBulkMap_t        signatureMapInfo = {0};
    mcResult_t         mcRet;

    /* Validate pointers */
    if ((keyData == NULL)       ||
        (plainData == NULL)     ||
        (signatureData == NULL) ||
        (signatureDataLength == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)plainData, plainDataLength, &plainMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)signatureData, *signatureDataLength, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId    = CMD_ID_TEE_RSA_SIGN;
        pTci->rsa_sign.key_data           = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->rsa_sign.plain_data         = (uint32_t)plainMapInfo.sVirtualAddr;
        pTci->rsa_sign.signature_data     = (uint32_t)signatureMapInfo.sVirtualAddr;
        pTci->rsa_sign.key_data_len       = keyDataLength;
        pTci->rsa_sign.plain_data_len     = plainDataLength;
        pTci->rsa_sign.signature_data_len = *signatureDataLength;
        pTci->rsa_sign.algorithm          = algorithm;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_RSASign(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Retrieve signature data length */
        *signatureDataLength = pTci->rsa_sign.signature_data_len;

    } while (false);

    /* Unmap memory */
    if (keyMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (plainMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)plainData, &plainMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (signatureMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)signatureData, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}


/**
 * TEE_RSAVerify
 *
 * Verifies given data with RSA public key and return status
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  plainData        [in]  Pointer to plain data to be signed
 * @param  plainDataLength  [in]  Plain data length
 * @param  signatureData    [in]  Pointer to signed data
 * @param  signatureData    [in]  Plain  data length
 * @param  algorithm        [in]  RSA signature algorithm
 * @param  validity         [out] Signature validity
 */
teeResult_t TEE_RSAVerify(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  plainData,
    const uint32_t  plainDataLength,
    const uint8_t*  signatureData,
    const uint32_t  signatureDataLength,
    teeRsaSigAlg_t  algorithm,
    bool            *validity
){
    teeResult_t        ret = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo       = {0};
    mcBulkMap_t        plainMapInfo     = {0};
    mcBulkMap_t        signatureMapInfo = {0};
    mcResult_t         mcRet;

    /* Validate pointers */
    if ((keyData == NULL)       ||
        (plainData == NULL)     ||
        (signatureData == NULL) ||
        (validity == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)plainData, plainDataLength, &plainMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)signatureData, signatureDataLength, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId      = CMD_ID_TEE_RSA_VERIFY;
        pTci->rsa_verify.key_data           = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->rsa_verify.plain_data         = (uint32_t)plainMapInfo.sVirtualAddr;
        pTci->rsa_verify.signature_data     = (uint32_t)signatureMapInfo.sVirtualAddr;
        pTci->rsa_verify.key_data_len       = keyDataLength;
        pTci->rsa_verify.plain_data_len     = plainDataLength;
        pTci->rsa_verify.signature_data_len = signatureDataLength;
        pTci->rsa_verify.algorithm          = algorithm;
        pTci->rsa_verify.validity           = false;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_RSAVerify(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        *validity =  pTci->rsa_verify.validity;

    } while (false);


    /* Unmap memory */
    if (keyMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (plainMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)plainData, &plainMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (signatureMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)signatureData, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}


/**
 * TEE_KeyImport
 *
 * Imports key data and returns key data as secure object
 *
 * Key data needs to be in the following format
 *
 * RSA key data:
 * |--key metadata--|--public modulus--|--public exponent--|--private exponent--|
 *
 * RSA CRT key data:
 * |--key metadata--|--public modulus--|--public exponent--|--P--|--Q--|--DP--|--DQ--|--Qinv--|
 *
 * Where:
 * P:     secret prime factor
 * Q:     secret prime factor
 * DP:    d mod (p-1)
 * DQ:    d mod (q-1)
 * Qinv:  q^-1 mod p
 *
 * DSA key data:
 * |-- Key metadata --|--p--|--q--|--g--|--y--|--private key data (x)--|
 *
 * Where:
 * p:     prime (modulus)
 * q:     sub prime
 * g:     generator
 * y:     public y
 * x:     private x
 *
 * ECDSA key data:
 * |-- Key metadata --|--x--|--y--|--private key data --|
 *
 * Where:
 * x:     affine point x
 * y:     affine point y
 *
 * @param  keyData          [in]  Pointer to key data
 * @param  keyDataLength    [in]  Key data length
 * @param  soData           [out] Pointer to wrapped key data
 * @param  soDataLength     [out] Wrapped key data length
 */
teeResult_t TEE_KeyImport(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    uint8_t*        soData,
    uint32_t*       soDataLength
){
    teeResult_t         ret  = TEE_ERR_NONE;
    tciMessage_ptr      pTci = NULL;
    mcSessionHandle_t   sessionHandle;
    mcBulkMap_t         keyMapInfo = {0};
    mcBulkMap_t         soMapInfo  = {0};
    mcResult_t          mcRet;

    /* Validate pointers */
    if ((keyData == NULL)   ||
        (soData == NULL)    ||
        (soDataLength == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)soData, *soDataLength, &soMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_KEY_IMPORT;
        pTci->key_import.key_data      = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->key_import.so_data       = (uint32_t)soMapInfo.sVirtualAddr;
        pTci->key_import.key_data_len  = keyDataLength;
        pTci->key_import.so_data_len   = *soDataLength;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_KeyImport(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Update secure object length */
        *soDataLength =  pTci->key_import.so_data_len;

    } while (false);

    /* Unmap memory */
    if (keyMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (soMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)soData, &soMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}


/** * TEE_GetPubKey
 *
 * Retrieves public key data from wrapped key data
 *
 *
 * RSA public key data:
 * |--public key metadata--|--public modulus--|--public exponent--|
 *
 * DSA public key data:
 * |-- public key metadata --|--p--|--q--|--g--|--y--|
 *
 * ECDSA public key data:
 * |-- public key metadata --|--x--|--y--|
 *
 * @param  keyData          [in]  Pointer to key data
 * @param  keyDataLength    [in]  Key data length
 * @param  pubKeyData       [out] Pointer to public key data
 * @param  pubKeyDataLength [out] Public key data length
 */
teeResult_t TEE_GetPubKey(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    uint8_t*        pubKeyData,
    uint32_t*       pubKeyDataLength
){
    teeResult_t         ret  = TEE_ERR_NONE;
    tciMessage_ptr      pTci = NULL;
    mcSessionHandle_t   sessionHandle;
    mcBulkMap_t         keyMapInfo    = {0};
    mcBulkMap_t         pubKeyMapInfo = {0};
    mcResult_t          mcRet;

    /* Validate pointers */
    if ((keyData == NULL)       ||
        (pubKeyData == NULL)    ||
        (pubKeyDataLength == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)pubKeyData, *pubKeyDataLength, &pubKeyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId     = CMD_ID_TEE_GET_PUB_KEY;
        pTci->get_pub_key.key_data         = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->get_pub_key.pub_key_data     = (uint32_t)pubKeyMapInfo.sVirtualAddr;
        pTci->get_pub_key.key_data_len     = keyDataLength;
        pTci->get_pub_key.pub_key_data_len = *pubKeyDataLength;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_GetPubKey(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Update  modulus and exponent lengths */
        *pubKeyDataLength  =   pTci->get_pub_key.pub_key_data_len;

    } while (false);


    /* Unmap memory */
    if (keyMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (pubKeyMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)pubKeyData, &pubKeyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}


/**
 * TEE_DSAGenerateKeyPair
 *
 * Generates DSA key pair and returns key pair data as wrapped object
 *
 * @param  keyData        [in]  Pointer to the key data buffer
 * @param  keyDataLength  [in]  Key data buffer length
 * @param  params         [in]  DSA parameters
 * @param  soLen          [out] Key data secure object length
 */
teeResult_t TEE_DSAGenerateKeyPair(
    uint8_t*            keyData,
    uint32_t            keyDataLength,
    teeDsaParams_t      *params,
    uint32_t*           soLen
){
    teeResult_t         ret = TEE_ERR_NONE;
    tciMessage_ptr      pTci = NULL;
    mcSessionHandle_t   sessionHandle;
    mcBulkMap_t         keyDataInfo = {0};
    mcBulkMap_t         pInfo = {0};
    mcBulkMap_t         qInfo = {0};
    mcBulkMap_t         gInfo = {0};
    mcResult_t          mcRet;

    /* Validate pointers */
    if ((keyData == NULL)   ||
        (params == NULL)    ||
        (params->p == NULL) ||
        (params->q == NULL) ||
        (params->g == NULL) ||
        (params->pLen == 0) ||
        (params->qLen == 0) ||
        (params->gLen == 0))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map key data buffer to the secure world */
        mcRet = mcMap(&sessionHandle, keyData, keyDataLength, &keyDataInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Map p buffer to the secure world */
        mcRet = mcMap(&sessionHandle, params->p, params->pLen, &pInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Map q buffer to the secure world */
        mcRet = mcMap(&sessionHandle, params->q, params->qLen, &qInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Map g buffer to the secure world */
        mcRet = mcMap(&sessionHandle, params->g, params->gLen, &gInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_DSA_GEN_KEY_PAIR;
        pTci->dsa_gen_key.key_data     = (uint32_t)keyDataInfo.sVirtualAddr;
        pTci->dsa_gen_key.key_data_len = keyDataLength;
        pTci->dsa_gen_key.p            = (uint32_t)pInfo.sVirtualAddr;
        pTci->dsa_gen_key.q            = (uint32_t)qInfo.sVirtualAddr;
        pTci->dsa_gen_key.g            = (uint32_t)gInfo.sVirtualAddr;
        pTci->dsa_gen_key.p_len        = params->pLen;
        pTci->dsa_gen_key.q_len        = params->qLen;
        pTci->dsa_gen_key.g_len        = params->gLen;
        pTci->dsa_gen_key.x_len        = params->xLen;
        pTci->dsa_gen_key.y_len        = params->yLen;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_DSAGenerateKeyPair(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Update secure object length */
        *soLen = pTci->dsa_gen_key.so_len;

    } while (false);

    /* Unmap key data buffer memory */
    if (keyDataInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, keyData, &keyDataInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }
    /* Unmap p buffer */
    if (pInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, params->p, &pInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Unmap q buffer */
    if (qInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, params->q, &qInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Unmap g buffer */
    if (gInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, params->g, &gInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}


/**
 * TEE_DSASign
 *
 * Signs given plain data and returns signature data
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  digest           [in]  Digest data to be signed
 * @param  digestLength     [in]  Digest data length
 * @param  signatureData    [out] Pointer to signature data
 * @param  signatureDataLength  [out] Signature data length
 */
teeResult_t TEE_DSASign(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  digest,
    const uint32_t  digestLength,
    uint8_t*        signatureData,
    uint32_t*       signatureDataLength
){
    teeResult_t        ret = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo    = {0};
    mcBulkMap_t        digestMapInfo = {0};
    mcBulkMap_t        signatureMapInfo = {0};
    mcResult_t         mcRet;

    /* Validate pointers */
    if ((keyData == NULL)   ||
        (digest == NULL) ||
        (signatureData == NULL) ||
        (signatureDataLength == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)digest, digestLength, &digestMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)signatureData, *signatureDataLength, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId    = CMD_ID_TEE_DSA_SIGN;
        pTci->dsa_sign.key_data           = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->dsa_sign.digest_data        = (uint32_t)digestMapInfo.sVirtualAddr;
        pTci->dsa_sign.signature_data     = (uint32_t)signatureMapInfo.sVirtualAddr;
        pTci->dsa_sign.key_data_len       = keyDataLength;
        pTci->dsa_sign.digest_data_len    = digestLength;
        pTci->dsa_sign.signature_data_len = *signatureDataLength;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_DSASign(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Retrieve signature data length */
        *signatureDataLength = pTci->dsa_sign.signature_data_len;

    } while (false);

    /* Unmap memory */
    if (keyMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (digestMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)digest, &digestMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (signatureMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)signatureData, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}


/**
 * TEE_DSAVerify
 *
 * Verifies given data with DSA public key and return status
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  digest           [in]  Pointer to digest data to be verified
 * @param  digestLen        [in]  Digest data length
 * @param  signatureData    [in]  Pointer to signed data
 * @param  signatureData    [in]  Plain  data length
 * @param  validity         [out] Signature validity
 */
teeResult_t TEE_DSAVerify(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  digest,
    const uint32_t  digestLen,
    const uint8_t*  signatureData,
    const uint32_t  signatureDataLength,
    bool*           validity
){
    teeResult_t        ret  = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo       = {0};
    mcBulkMap_t        digestMapInfo    = {0};
    mcBulkMap_t        signatureMapInfo = {0};
    mcResult_t         mcRet;

    /* Validate pointers */
    if ((keyData == NULL)       ||
        (digest == NULL)        ||
        (signatureData == NULL) ||
        (validity == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)digest, digestLen, &digestMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)signatureData, signatureDataLength, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId      = CMD_ID_TEE_DSA_VERIFY;
        pTci->dsa_verify.key_data           = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->dsa_verify.digest_data        = (uint32_t)digestMapInfo.sVirtualAddr;
        pTci->dsa_verify.signature_data     = (uint32_t)signatureMapInfo.sVirtualAddr;
        pTci->dsa_verify.key_data_len       = keyDataLength;
        pTci->dsa_verify.digest_data_len    = digestLen;
        pTci->dsa_verify.signature_data_len = signatureDataLength;
        pTci->dsa_verify.validity           = false;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_DSAVerify(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        *validity =  pTci->dsa_verify.validity;

    } while (false);

    /* Unmap memory */
    if (keyMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (digestMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)digest, &digestMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (signatureMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)signatureData, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}


/**
 * TEE_ECDSAGenerateKeyPair
 *
 * Generates ECDSA key pair and returns key pair data as wrapped object
 *
 * @param  keyData        [in]  Pointer to the key data buffer
 * @param  keyDataLength  [in]  Key data buffer length
 * @param  curveType      [in]  Curve type
 * @param  soLen          [out] Key data secure object length
 */
teeResult_t TEE_ECDSAGenerateKeyPair(
    uint8_t*            keyData,
    uint32_t            keyDataLength,
    teeEccCurveType_t   curveType,
    uint32_t*           soLen
){
    teeResult_t         ret     = TEE_ERR_NONE;
    tciMessage_ptr      pTci    = NULL;
    mcSessionHandle_t   sessionHandle;
    mcBulkMap_t         keyDataInfo = {0};
    mcResult_t          mcRet;

    /* Validate pointers */
    if ((keyData == NULL) || (soLen == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map key data buffer to the secure world */
        mcRet = mcMap(&sessionHandle, keyData, keyDataLength, &keyDataInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }
        /* Update TCI buffer */
        pTci->command.header.commandId   = CMD_ID_TEE_ECDSA_GEN_KEY_PAIR;
        pTci->ecdsa_gen_key.key_data     = (uint32_t)keyDataInfo.sVirtualAddr;
        pTci->ecdsa_gen_key.key_data_len = keyDataLength;
        pTci->ecdsa_gen_key.curve        = curveType;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_ECDSAGenerateKeyPair(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Update secure object length */
        *soLen =  pTci->ecdsa_gen_key.so_len;

    } while (false);

    /* Unmap key data buffer */
    if (keyDataInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, keyData, &keyDataInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;

}


/**
 * TEE_ECDSASign
 *
 * Signs given plain data and returns signature data
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  digest           [in]  Digest data to be signed
 * @param  digestLength     [in]  Digest data length
 * @param  signatureData    [out] Pointer to signature data
 * @param  signatureDataLength  [out] Signature data length
 */
teeResult_t TEE_ECDSASign(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  digest,
    const uint32_t  digestLength,
    uint8_t*        signatureData,
    uint32_t*       signatureDataLength
){
    teeResult_t        ret  = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo       = {0};
    mcBulkMap_t        digestMapInfo    = {0};
    mcBulkMap_t        signatureMapInfo = {0};
    mcResult_t         mcRet;

    /* Validate pointers */
    if ((keyData == NULL)       ||
        (digest == NULL)        ||
        (signatureData == NULL) ||
        (signatureDataLength == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)digest, digestLength, &digestMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)signatureData, *signatureDataLength, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId      = CMD_ID_TEE_ECDSA_SIGN;
        pTci->ecdsa_sign.key_data           = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->ecdsa_sign.digest_data        = (uint32_t)digestMapInfo.sVirtualAddr;
        pTci->ecdsa_sign.signature_data     = (uint32_t)signatureMapInfo.sVirtualAddr;
        pTci->ecdsa_sign.key_data_len       = keyDataLength;
        pTci->ecdsa_sign.digest_data_len    = digestLength;
        pTci->ecdsa_sign.signature_data_len = *signatureDataLength;


        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_ECDSASign(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Retrieve signature data length */
        *signatureDataLength = pTci->ecdsa_sign.signature_data_len;

    } while (false);


    /* Unmap memory */
    if (keyMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (digestMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)digest, &digestMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (signatureMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)signatureData, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}


/**
 * TEE_ECDSAVerify
 *
 * Verifies given data with ECDSA public key and return status
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  digest           [in]  Pointer to digest data to be verified
 * @param  digestLen        [in]  Digest data length
 * @param  signatureData    [in]  Pointer to signed data
 * @param  signatureData    [in]  Plain  data length
 * @param  validity         [out] Signature validity
 */
teeResult_t TEE_ECDSAVerify(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  digest,
    const uint32_t  digestLen,
    const uint8_t*  signatureData,
    const uint32_t  signatureDataLength,
    bool*           validity
){
    teeResult_t        ret  = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo       = {0};
    mcBulkMap_t        digestMapInfo    = {0};
    mcBulkMap_t        signatureMapInfo = {0};
    mcResult_t         mcRet;

    /* Validate pointers */
    if ((keyData == NULL)       ||
        (digest == NULL)        ||
        (signatureData == NULL) ||
        (validity == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)digest, digestLen, &digestMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)signatureData, signatureDataLength, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId        = CMD_ID_TEE_ECDSA_VERIFY;
        pTci->ecdsa_verify.key_data           = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->ecdsa_verify.digest_data        = (uint32_t)digestMapInfo.sVirtualAddr;
        pTci->ecdsa_verify.signature_data     = (uint32_t)signatureMapInfo.sVirtualAddr;
        pTci->ecdsa_verify.key_data_len       = keyDataLength;
        pTci->ecdsa_verify.digest_data_len    = digestLen;
        pTci->ecdsa_verify.signature_data_len = signatureDataLength;
        pTci->ecdsa_verify.validity = false;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_ECDSAVerify(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        *validity =  pTci->ecdsa_verify.validity;

    } while (false);

    /* Unmap memory */
    if (keyMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (digestMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)digest, &digestMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (signatureMapInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)signatureData, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}


/**
 * TEE_GetKeyInfo
 *
 * Retrieves key information (type, length,..) from key data blob and populates key metadata
 * accordingly
 *
 * @param  keyBlob          [in]  Pointer to key blob data
 * @param  keyBlobLength    [in]  Key data buffer length
 * @param  metadata         [out] Pointer to digest data to be verified
 */
teeResult_t TEE_GetKeyInfo(
    const uint8_t*  keyBlob,
    const uint32_t  keyBlobLength,
    teeKeyMeta_t*   metadata)
{
    teeResult_t        ret  = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyBlobInfo = {0};
    mcBulkMap_t        keyDataInfo = {0};
    mcResult_t         mcRet;

    /* Validate pointers */
    if ((keyBlob == NULL) || (metadata == NULL))
    {
        return TEE_ERR_INVALID_INPUT;
    }

    do
    {
        /* Open session to the trusted application */
        pTci = TEE_Open(&sessionHandle);
        if (pTci == NULL)
        {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyBlob, keyBlobLength, &keyBlobInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)metadata, sizeof(teeKeyMeta_t), &keyDataInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId        = CMD_ID_TEE_GET_KEY_INFO;
        pTci->get_key_info.key_blob           = (uint32_t)keyBlobInfo.sVirtualAddr;
        pTci->get_key_info.key_metadata       = (uint32_t)keyDataInfo.sVirtualAddr;
        pTci->get_key_info.key_blob_len       = keyBlobLength;

        /* Notify the trusted application */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trusted application */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_GetKeyInfo(): TEE Keymaster trusted application returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

    } while (false);

    if (keyBlobInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)keyBlob, &keyBlobInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    if (keyDataInfo.sVirtualAddr != 0)
    {
        mcRet = mcUnmap(&sessionHandle, (void*)metadata, &keyDataInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_UNMAP;
        }
    }

    /* Close session to the trusted application */
    TEE_Close(&sessionHandle);

    return ret;
}
