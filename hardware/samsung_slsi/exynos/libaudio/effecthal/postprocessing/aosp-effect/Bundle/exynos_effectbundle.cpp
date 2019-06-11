/*
 * Copyright (C) 2010-2010 NXP Software
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "HW_Bundle"
#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])
//#define LOG_NDEBUG 0

#include <assert.h>
#include <inttypes.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#include <cutils/log.h>
#include "exynos_effectbundle.h"
#include "math.h"


// effect_handle_t interface implementation for bass boost
extern "C" const struct effect_interface_s gLvmHwEffectInterface;

#define LVM_ERROR_CHECK(LvmStatus, callingFunc, calledFunc){\
        if (LvmStatus == LVM_NULLADDRESS){\
            ALOGV("\tLVM_ERROR : Parameter error - "\
                    "null pointer returned by %s in %s\n\n\n\n", callingFunc, calledFunc);\
        }\
        if (LvmStatus == LVM_ALIGNMENTERROR){\
            ALOGV("\tLVM_ERROR : Parameter error - "\
                    "bad alignment returned by %s in %s\n\n\n\n", callingFunc, calledFunc);\
        }\
        if (LvmStatus == LVM_INVALIDNUMSAMPLES){\
            ALOGV("\tLVM_ERROR : Parameter error - "\
                    "bad number of samples returned by %s in %s\n\n\n\n", callingFunc, calledFunc);\
        }\
        if (LvmStatus == LVM_OUTOFRANGE){\
            ALOGV("\tLVM_ERROR : Parameter error - "\
                    "out of range returned by %s in %s\n", callingFunc, calledFunc);\
        }\
    }

// Namespaces
namespace android {
namespace {

// Flag to allow a one time init of global memory, only happens on first call ever
int LvmhwInitFlag = LVM_FALSE;
SessionContext HwGlobalSessionMemory[LVM_MAX_SESSIONS];
int HwSessionIndex[LVM_MAX_SESSIONS];
#define MIXER_CARD 0
#define MIXER_CTL_NAME "NXP BDL data"
#define BUNDLE_PARAM_MAX    35

/* local functions */
#define CHECK_ARG(cond) {                     \
    if (!(cond)) {                            \
        ALOGV("\tLVM_ERROR : Invalid argument: "#cond);      \
        return -EINVAL;                       \
    }                                         \
}

// NXP HW BassBoost UUID
const effect_descriptor_t gHwBassBoostDescriptor = {
        {0x0634f220, 0xddd4, 0x11db, 0xa0fc, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b }},
        {0x0acd3de0, 0x7b93, 0x11e5, 0xbbaf, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_HW_ACC_TUNNEL
        |EFFECT_FLAG_VOLUME_CTRL | EFFECT_FLAG_VOLUME_IND),
        0, //CPU Load information,
        1, //Memory usage for this effect,
        "Offload Dynamic Bass Boost",
        "Offload NXP Software Ltd.",
};

// NXP HW Virtualizer UUID
const effect_descriptor_t gHwVirtualizerDescriptor = {
        {0x37cc2c00, 0xdddd, 0x11db, 0x8577, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        {0xc09d2040, 0x7b93, 0x11e5, 0x8e60, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_HW_ACC_TUNNEL
        |EFFECT_FLAG_VOLUME_CTRL | EFFECT_FLAG_VOLUME_IND),
        0, //CPU Load information,
        1, //Memory usage for this effect,
        "Offload Virtualizer",
        "Offload NXP Software Ltd.",
};

// NXP HW Equalizer UUID
const effect_descriptor_t gHwEqualizerDescriptor = {
        {0x0bed4300, 0xddd6, 0x11db, 0x8f34, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // type
        {0xd35a7d40, 0x7b93, 0x11e5, 0x8aae, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // uuid Eq NXP
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_HW_ACC_TUNNEL
        |EFFECT_FLAG_VOLUME_CTRL | EFFECT_FLAG_VOLUME_IND),
        0, //CPU Load information,
        1, //Memory usage for this effect,
        "Offload Equalizer",
        "Offload NXP Software Ltd.",
};

// NXP HW Volume UUID
const effect_descriptor_t gHwVolumeDescriptor = {
        {0x09e8ede0, 0xddde, 0x11db, 0xb4f6, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b }},
        {0xf3bb1040, 0x7b93, 0x11e5, 0x9930, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b }}, //uuid VOL NXP
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_HW_ACC_TUNNEL
        |EFFECT_FLAG_VOLUME_CTRL | EFFECT_FLAG_VOLUME_IND),
        0, //CPU Load information,
        1, //Memory usage for this effect,
        "Offloadd Volume",
        "Offload NXP Software Ltd.",
};

//--- local function prototypes
void LvmGlobalBundle_init      (void);
int  LvmBundle_init            (EffectContext *pContext);
int  LvmEffect_enable          (EffectContext *pContext);
int  LvmEffect_disable         (EffectContext *pContext);
int  Effect_setConfig          (EffectContext *pContext, effect_config_t *pConfig);
void Effect_getConfig          (EffectContext *pContext, effect_config_t *pConfig);
int  BassBoost_setParameter    (EffectContext *pContext, void *pParam, void *pValue);
int  BassBoost_getParameter    (EffectContext *pContext,
                               void           *pParam,
                               uint32_t       *pValueSize,
                               void           *pValue);
int  Virtualizer_setParameter  (EffectContext *pContext, void *pParam, void *pValue);
int  Virtualizer_getParameter  (EffectContext *pContext,
                               void           *pParam,
                               uint32_t       *pValueSize,
                               void           *pValue);
int  Equalizer_setParameter    (EffectContext *pContext, void *pParam, void *pValue);
int  Equalizer_getParameter    (EffectContext *pContext,
                                void          *pParam,
                                uint32_t      *pValueSize,
                                void          *pValue);
int  Volume_setParameter       (EffectContext *pContext, void *pParam, void *pValue);
int  Volume_getParameter       (EffectContext *pContext,
                                void          *pParam,
                                uint32_t      *pValueSize,
                                void          *pValue);
int Effect_setEnabled(EffectContext *pContext, bool enabled);
LVM_ReturnStatus_en Offload_SetEffect_ControlParameters(EffectContext *pContext);

/* Effect Library Interface Implementation */

extern "C" int EffectHwCreate(const effect_uuid_t *uuid,
                            int32_t             sessionId,
                            int32_t             ioId __unused,
                            effect_handle_t  *pHandle){
    int ret = 0;
    int sessionNo;
    int i;
    EffectContext *pContext = NULL;
    bool newBundle = false;
    SessionContext *pSessionContext;

    ALOGV("\n\tEffectHwCreate start session %d", sessionId);

    if (pHandle == NULL || uuid == NULL){
        ALOGV("\tLVM_ERROR : EffectHwCreate() called with NULL pointer");
        ret = -EINVAL;
        goto exit;
    }

    if(LvmhwInitFlag == LVM_FALSE){
        LvmhwInitFlag = LVM_TRUE;
        ALOGV("\tEffectHwCreate - Initializing all global memory");
        LvmGlobalBundle_init();
    }

    // Find next available sessionNo
    for(i=0; i<LVM_MAX_SESSIONS; i++){
        if((HwSessionIndex[i] == LVM_UNUSED_SESSION)||(HwSessionIndex[i] == sessionId)){
            sessionNo       = i;
            HwSessionIndex[i] = sessionId;
            ALOGV("\tEffectHwCreate: Allocating SessionNo %d for SessionId %d\n", sessionNo,sessionId);
            break;
        }
    }

    if(i==LVM_MAX_SESSIONS){
        ALOGV("\tLVM_ERROR : Cannot find memory to allocate for current session");
        ret = -EINVAL;
        goto exit;
    }

    pContext = new EffectContext;

    // If this is the first create in this session
    if(HwGlobalSessionMemory[sessionNo].bBundledEffectsEnabled == LVM_FALSE){
        ALOGV("\tEffectHwCreate - This is the first effect in current sessionId %d sessionNo %d",
                sessionId, sessionNo);

        HwGlobalSessionMemory[sessionNo].bBundledEffectsEnabled = LVM_TRUE;
        HwGlobalSessionMemory[sessionNo].pBundledContext        = new BundledEffectContext;
        newBundle = true;

        pContext->pBundledContext = HwGlobalSessionMemory[sessionNo].pBundledContext;
        pContext->pBundledContext->SessionNo                = sessionNo;
        pContext->pBundledContext->SessionId                = sessionId;
        pContext->pBundledContext->hInstance                = NULL;
        pContext->pBundledContext->bVolumeEnabled           = LVM_FALSE;
        pContext->pBundledContext->bEqualizerEnabled        = LVM_FALSE;
        pContext->pBundledContext->bBassEnabled             = LVM_FALSE;
        pContext->pBundledContext->bBassTempDisabled        = LVM_FALSE;
        pContext->pBundledContext->bVirtualizerEnabled      = LVM_FALSE;
        pContext->pBundledContext->bVirtualizerTempDisabled = LVM_FALSE;
        pContext->pBundledContext->nOutputDevice            = AUDIO_DEVICE_NONE;
        pContext->pBundledContext->nVirtualizerForcedDevice = AUDIO_DEVICE_NONE;
        pContext->pBundledContext->NumberEffectsEnabled     = 0;
        pContext->pBundledContext->NumberEffectsCalled      = 0;
        pContext->pBundledContext->firstVolume              = LVM_TRUE;
        pContext->pBundledContext->volume                   = 0;

        /* Saved strength is used to return the exact strength that was used in the set to the get
         * because we map the original strength range of 0:1000 to 1:15, and this will avoid
         * quantisation like effect when returning
         */
        pContext->pBundledContext->BassStrengthSaved        = 0;
        pContext->pBundledContext->VirtStrengthSaved        = 0;
        pContext->pBundledContext->CurPreset                = PRESET_CUSTOM;
        pContext->pBundledContext->levelSaved               = 0;
        pContext->pBundledContext->bMuteEnabled             = LVM_FALSE;
        pContext->pBundledContext->bStereoPositionEnabled   = LVM_FALSE;
        pContext->pBundledContext->positionSaved            = 0;
        pContext->pBundledContext->workBuffer               = NULL;
        pContext->pBundledContext->frameCount               = -1;
        pContext->pBundledContext->SamplesToExitCountVirt   = 0;
        pContext->pBundledContext->SamplesToExitCountBb     = 0;
        pContext->pBundledContext->SamplesToExitCountEq     = 0;

        for (int i = 0; i < FIVEBAND_NUMBANDS; i++) {
            pContext->pBundledContext->bandGaindB[i] = EQNB_5BandSoftPresets[i];
        }

        ALOGV("\tEffectHwCreate - Calling LvmBundle_init");
        ret = LvmBundle_init(pContext);

        if (ret < 0){
            ALOGV("\tLVM_ERROR : EffectHwCreate() Bundle init failed");
            goto exit;
        }
    }
    else{
        ALOGV("\tEffectHwCreate - Assigning memory for previously created effect on sessionNo %d",
                sessionNo);
        pContext->pBundledContext =
                HwGlobalSessionMemory[sessionNo].pBundledContext;
    }
    ALOGV("\tEffectHwCreate - pBundledContext is %p", pContext->pBundledContext);

    pSessionContext = &HwGlobalSessionMemory[pContext->pBundledContext->SessionNo];

    // Create each Effect
    if (memcmp(uuid, &gHwBassBoostDescriptor.uuid, sizeof(effect_uuid_t)) == 0){
        // Create Bass Boost
        ALOGD("\tEffectHwCreate - Effect to be created is LVM_BASS_BOOST");
        pSessionContext->bBassInstantiated = LVM_TRUE;
        pContext->pBundledContext->SamplesToExitCountBb = 0;

        pContext->itfe       = &gLvmHwEffectInterface;
        pContext->EffectType = LVM_BASS_BOOST;
    } else if (memcmp(uuid, &gHwVirtualizerDescriptor.uuid, sizeof(effect_uuid_t)) == 0){
        // Create Virtualizer
        ALOGD("\tEffectHwCreate - Effect to be created is LVM_VIRTUALIZER");
        pSessionContext->bVirtualizerInstantiated=LVM_TRUE;
        pContext->pBundledContext->SamplesToExitCountVirt = 0;

        pContext->itfe       = &gLvmHwEffectInterface;
        pContext->EffectType = LVM_VIRTUALIZER;
    } else if (memcmp(uuid, &gHwEqualizerDescriptor.uuid, sizeof(effect_uuid_t)) == 0){
        // Create Equalizer
        ALOGD("\tEffectHwCreate - Effect to be created is LVM_EQUALIZER");
        pSessionContext->bEqualizerInstantiated = LVM_TRUE;
        pContext->pBundledContext->SamplesToExitCountEq = 0;

        pContext->itfe       = &gLvmHwEffectInterface;
        pContext->EffectType = LVM_EQUALIZER;
    } else if (memcmp(uuid, &gHwVolumeDescriptor.uuid, sizeof(effect_uuid_t)) == 0){
        // Create Volume
        ALOGD("\tEffectHwCreate - Effect to be created is LVM_VOLUME");
        pSessionContext->bVolumeInstantiated = LVM_TRUE;

        pContext->itfe       = &gLvmHwEffectInterface;
        pContext->EffectType = LVM_VOLUME;
    }
    else{
        ALOGD("\tLVM_ERROR : EffectHwCreate() invalid UUID");
        ret = -EINVAL;
        goto exit;
    }

exit:
    if (ret != 0) {
        if (pContext != NULL) {
            if (newBundle) {
                HwGlobalSessionMemory[sessionNo].bBundledEffectsEnabled = LVM_FALSE;
                HwSessionIndex[sessionNo] = LVM_UNUSED_SESSION;
                delete pContext->pBundledContext;
            }
            delete pContext;
        }
        *pHandle = (effect_handle_t)NULL;
    } else {
        *pHandle = (effect_handle_t)pContext;
        if (pContext->EffectType == LVM_BASS_BOOST) {
            ALOGV("[DEBUG]\tEffectHwCreate - Address LOG");
            ALOGV("[DEBUG]\tEffectHwCreate - pContext = %p Band = %p", pContext, pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition);
        }
    }
    ALOGV("\tEffectHwCreate end..\n\n");
    return ret;
} /* end EffectHwCreate */

extern "C" int EffectHwRelease(effect_handle_t handle){
    ALOGV("\n\tEffectHwRelease start %p", handle);
    EffectContext * pContext = (EffectContext *)handle;

    ALOGV("\tEffectHwRelease start handle: %p, context %p", handle, pContext->pBundledContext);
    if (pContext == NULL){
        ALOGV("\tLVM_ERROR : EffectHwRelease called with NULL pointer");
        return -EINVAL;
    }

    SessionContext *pSessionContext = &HwGlobalSessionMemory[pContext->pBundledContext->SessionNo];

    // Clear the instantiated flag for the effect
    // protect agains the case where an effect is un-instantiated without being disabled
    if(pContext->EffectType == LVM_BASS_BOOST) {
        ALOGV("\tEffectHwRelease LVM_BASS_BOOST Clearing global intstantiated flag");
        pSessionContext->bBassInstantiated = LVM_FALSE;
        if(pContext->pBundledContext->SamplesToExitCountBb > 0){
            pContext->pBundledContext->NumberEffectsEnabled--;
        }
        pContext->pBundledContext->SamplesToExitCountBb = 0;
    } else if(pContext->EffectType == LVM_VIRTUALIZER) {
        ALOGV("\tEffectHwRelease LVM_VIRTUALIZER Clearing global intstantiated flag");
        pSessionContext->bVirtualizerInstantiated = LVM_FALSE;
        if(pContext->pBundledContext->SamplesToExitCountVirt > 0){
            pContext->pBundledContext->NumberEffectsEnabled--;
        }
        pContext->pBundledContext->SamplesToExitCountVirt = 0;
    } else if(pContext->EffectType == LVM_EQUALIZER) {
        ALOGV("\tEffectHwRelease LVM_EQUALIZER Clearing global intstantiated flag");
        pSessionContext->bEqualizerInstantiated =LVM_FALSE;
        if(pContext->pBundledContext->SamplesToExitCountEq > 0){
            pContext->pBundledContext->NumberEffectsEnabled--;
        }
        pContext->pBundledContext->SamplesToExitCountEq = 0;
    } else if(pContext->EffectType == LVM_VOLUME) {
        ALOGV("\tEffectHwRelease LVM_VOLUME Clearing global intstantiated flag");
        pSessionContext->bVolumeInstantiated = LVM_FALSE;
        if (pContext->pBundledContext->bVolumeEnabled == LVM_TRUE){
            pContext->pBundledContext->NumberEffectsEnabled--;
        }
    } else {
        ALOGV("\tLVM_ERROR : EffectHwRelease : Unsupported effect\n\n\n\n\n\n\n");
    }

    // Disable effect, in this case ignore errors (return codes)
    // if an effect has already been disabled
    Effect_setEnabled(pContext, LVM_FALSE);

    // if all effects are no longer instantiaed free the lvm memory and delete BundledEffectContext
    if ((pSessionContext->bBassInstantiated == LVM_FALSE) &&
            (pSessionContext->bVolumeInstantiated == LVM_FALSE) &&
            (pSessionContext->bEqualizerInstantiated ==LVM_FALSE) &&
            (pSessionContext->bVirtualizerInstantiated==LVM_FALSE))
    {
        // Clear the HwSessionIndex
        for(int i=0; i<LVM_MAX_SESSIONS; i++){
            if(HwSessionIndex[i] == pContext->pBundledContext->SessionId){
                HwSessionIndex[i] = LVM_UNUSED_SESSION;
                ALOGV("\tEffectHwRelease: Clearing HwSessionIndex SessionNo %d for SessionId %d\n",
                        i, pContext->pBundledContext->SessionId);
                break;
            }
        }

        ALOGV("\tEffectHwRelease: All effects are no longer instantiated\n");
        pSessionContext->bBundledEffectsEnabled = LVM_FALSE;
        pSessionContext->pBundledContext = LVM_NULL;

        ALOGV("\tEffectHwRelease: Deleting LVM Bundle context %p\n", pContext->pBundledContext);
        if (pContext->pBundledContext->workBuffer != NULL) {
            free(pContext->pBundledContext->workBuffer);
        }

        if (pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition != NULL) {
            free(pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition);
        }
        /* close mixer control */
        mixer_close(pContext->pBundledContext->mixerHandle);
        delete pContext->pBundledContext;
        pContext->pBundledContext = LVM_NULL;
    }
    // free the effect context for current effect
    delete pContext;

    ALOGV("\tEffectHwRelease end\n");
    return 0;

} /* end EffectHwRelease */

extern "C" int EffectHwGetDescriptor(const effect_uuid_t *uuid,
                                   effect_descriptor_t *pDescriptor) {
    const effect_descriptor_t *desc = NULL;

    ALOGV("\tEffectHwGetDescriptor start\n");
    if (pDescriptor == NULL || uuid == NULL){
        ALOGE("EffectHwGetDescriptor() called with NULL pointer");
        return -EINVAL;
    }

    if (memcmp(uuid, &gHwBassBoostDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        desc = &gHwBassBoostDescriptor;
    } else if (memcmp(uuid, &gHwVirtualizerDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        desc = &gHwVirtualizerDescriptor;
    } else if (memcmp(uuid, &gHwEqualizerDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        desc = &gHwEqualizerDescriptor;
    } else if (memcmp(uuid, &gHwVolumeDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        desc = &gHwVolumeDescriptor;
    }

    if (desc == NULL) {
        return  -EINVAL;
    }

    ALOGV("\tEffectHwGetDescriptor end\n");
    *pDescriptor = *desc;

    return 0;
} /* end EffectHwGetDescriptor */

void LvmGlobalBundle_init(){
    ALOGV("\tLvmGlobalBundle_init start");
    for(int i=0; i<LVM_MAX_SESSIONS; i++){
        HwGlobalSessionMemory[i].bBundledEffectsEnabled   = LVM_FALSE;
        HwGlobalSessionMemory[i].bVolumeInstantiated      = LVM_FALSE;
        HwGlobalSessionMemory[i].bEqualizerInstantiated   = LVM_FALSE;
        HwGlobalSessionMemory[i].bBassInstantiated        = LVM_FALSE;
        HwGlobalSessionMemory[i].bVirtualizerInstantiated = LVM_FALSE;
        HwGlobalSessionMemory[i].pBundledContext          = LVM_NULL;

        HwSessionIndex[i] = LVM_UNUSED_SESSION;
    }
    return;
}

LVM_ReturnStatus_en Offload_SetEffect_ControlParameters(EffectContext *pContext){
    LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;          /* Function call status */
    int32_t param[BUNDLE_PARAM_MAX] = {0};
    int i, ret;

    ALOGV("\tOffload_SetEffect_ControlParameters Enter");
    if (pContext->pBundledContext->OffloadEnabled) {
        /* Update common strcuture parameter to array */
        param[0] = (int32_t)pContext->pBundledContext->ActiveParams.OperatingMode;
        param[1] = (int32_t)pContext->pBundledContext->ActiveParams.SampleRate;
        param[2] = (int32_t)pContext->pBundledContext->ActiveParams.SourceFormat;
        param[3] = (int32_t)pContext->pBundledContext->ActiveParams.SpeakerType;
        param[4] = (int32_t)pContext->pBundledContext->ActiveParams.VirtualizerOperatingMode;
        param[5] = (int32_t)pContext->pBundledContext->ActiveParams.VirtualizerType;
        param[6] = (int32_t)pContext->pBundledContext->ActiveParams.VirtualizerReverbLevel;
        param[7] = (int32_t)pContext->pBundledContext->ActiveParams.CS_EffectLevel;
        param[8] = (int32_t)pContext->pBundledContext->ActiveParams.EQNB_OperatingMode;
        param[9] = (int32_t)pContext->pBundledContext->ActiveParams.EQNB_NBands;
        param[10] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[0].Gain;
        param[11] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[0].Frequency;
        param[12] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[0].QFactor;
        param[13] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[1].Gain;
        param[14] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[1].Frequency;
        param[15] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[1].QFactor;
        param[16] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[2].Gain;
        param[17] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[2].Frequency;
        param[18] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[2].QFactor;
        param[19] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[3].Gain;
        param[20] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[3].Frequency;
        param[21] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[3].QFactor;
        param[22] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[4].Gain;
        param[23] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[4].Frequency;
        param[24] = (int32_t)pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[4].QFactor;
        param[25] = (int32_t)pContext->pBundledContext->ActiveParams.BE_OperatingMode;
        param[26] = (int32_t)pContext->pBundledContext->ActiveParams.BE_EffectLevel;
        param[27] = (int32_t)pContext->pBundledContext->ActiveParams.BE_CentreFreq;
        param[28] = (int32_t)pContext->pBundledContext->ActiveParams.BE_HPF;
        param[29] = (int32_t)pContext->pBundledContext->ActiveParams.VC_EffectLevel;
        param[30] = (int32_t)pContext->pBundledContext->ActiveParams.VC_Balance;
        param[31] = (int32_t)pContext->pBundledContext->ActiveParams.TE_OperatingMode;
        param[32] = (int32_t)pContext->pBundledContext->ActiveParams.TE_EffectLevel;
        param[33] = (int32_t)pContext->pBundledContext->ActiveParams.PSA_Enable;
        param[34] = (int32_t)pContext->pBundledContext->ActiveParams.PSA_PeakDecayRate;

        if (pContext->pBundledContext->mixerCtl) {
            ALOGV("\tOffload_SetEffect_ControlParameters: mixer_ctl_set_array");
            ret = mixer_ctl_set_array(pContext->pBundledContext->mixerCtl, param, ARRAY_SIZE(param));
            if (ret) {
                ALOGE("%s: mixer_ctl_set_array return error(%d)", __func__, LvmStatus);
                LvmStatus = LVM_OUTOFRANGE;
            } else {
                LvmStatus = LVM_SUCCESS;
                /*for (i=0; i < 35; i++) {
                    ALOGD("mixer-array param[%d] = %d", i, param[i]);
                }*/
            }
        }
    }

    ALOGV("\tOffload_SetEffect_ControlParameters Exit");
    return LvmStatus;
}

//----------------------------------------------------------------------------
// LvmBundle_init()
//----------------------------------------------------------------------------
// Purpose: Initialize engine with default configuration, creates instance
// with all effects disabled.
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------

int LvmBundle_init(EffectContext *pContext){
    int status;
    LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;          /* Function call status */
    LVM_EQNB_BandDef_t      *pBandDefs = NULL;        /* Equaliser band definitions */

    ALOGV("\tLvmBundle_init start");

    pContext->config.inputCfg.accessMode                    = EFFECT_BUFFER_ACCESS_READ;
    pContext->config.inputCfg.channels                      = AUDIO_CHANNEL_OUT_STEREO;
    pContext->config.inputCfg.format                        = AUDIO_FORMAT_PCM_16_BIT;
    pContext->config.inputCfg.samplingRate                  = 44100;
    pContext->config.inputCfg.bufferProvider.getBuffer      = NULL;
    pContext->config.inputCfg.bufferProvider.releaseBuffer  = NULL;
    pContext->config.inputCfg.bufferProvider.cookie         = NULL;
    pContext->config.inputCfg.mask                          = EFFECT_CONFIG_ALL;
    pContext->config.outputCfg.accessMode                   = EFFECT_BUFFER_ACCESS_ACCUMULATE;
    pContext->config.outputCfg.channels                     = AUDIO_CHANNEL_OUT_STEREO;
    pContext->config.outputCfg.format                       = AUDIO_FORMAT_PCM_16_BIT;
    pContext->config.outputCfg.samplingRate                 = 44100;
    pContext->config.outputCfg.bufferProvider.getBuffer     = NULL;
    pContext->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    pContext->config.outputCfg.bufferProvider.cookie        = NULL;
    pContext->config.outputCfg.mask                         = EFFECT_CONFIG_ALL;

    CHECK_ARG(pContext != NULL);

    /* open mixer control */
    pContext->pBundledContext->mixerHandle = mixer_open(MIXER_CARD);
    if (!pContext->pBundledContext->mixerHandle) {
        ALOGE("%s: Failed to open mixer", __func__);
        return -EINVAL;
    }

    /* Get required control from mixer */
    pContext->pBundledContext->mixerCtl = mixer_get_ctl_by_name(pContext->pBundledContext->mixerHandle, MIXER_CTL_NAME);
    if (!pContext->pBundledContext->mixerCtl) {
        ALOGE("%s: mixer_get_ctl_by_name failed", __func__);
        mixer_close(pContext->pBundledContext->mixerHandle);
        return -EINVAL;
    }

    /* Set the initial process parameters */
    /* General parameters */
    pContext->pBundledContext->ActiveParams.OperatingMode          = LVM_MODE_ON;
    pContext->pBundledContext->ActiveParams.SampleRate             = LVM_FS_44100;
    pContext->pBundledContext->ActiveParams.SourceFormat           = LVM_STEREO;
    pContext->pBundledContext->ActiveParams.SpeakerType            = LVM_HEADPHONES;

    pContext->pBundledContext->SampleRate = LVM_FS_44100;

    /* Concert Sound parameters */
    pContext->pBundledContext->ActiveParams.VirtualizerOperatingMode   = LVM_MODE_OFF;
    pContext->pBundledContext->ActiveParams.VirtualizerType            = LVM_CONCERTSOUND;
    pContext->pBundledContext->ActiveParams.VirtualizerReverbLevel     = 100;
    pContext->pBundledContext->ActiveParams.CS_EffectLevel             = LVM_CS_EFFECT_NONE;

    /* N-Band Equaliser parameters */
    pContext->pBundledContext->ActiveParams.EQNB_OperatingMode     = LVM_EQNB_OFF;
    pContext->pBundledContext->ActiveParams.EQNB_NBands            = FIVEBAND_NUMBANDS;
    pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition   = (LVM_EQNB_BandDef_t*)malloc(sizeof(LVM_EQNB_BandDef_t) * MAX_NUM_BANDS);
    pBandDefs = pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition;

    for (int i=0; i<FIVEBAND_NUMBANDS; i++)
    {
        pBandDefs[i].Frequency = EQNB_5BandPresetsFrequencies[i];
        pBandDefs[i].QFactor   = EQNB_5BandPresetsQFactors[i];
        pBandDefs[i].Gain      = EQNB_5BandSoftPresets[i];
    }

    /* Volume Control parameters */
    pContext->pBundledContext->ActiveParams.VC_EffectLevel         = 0;
    pContext->pBundledContext->ActiveParams.VC_Balance             = 0;

    /* Treble Enhancement parameters */
    pContext->pBundledContext->ActiveParams.TE_OperatingMode       = LVM_TE_OFF;
    pContext->pBundledContext->ActiveParams.TE_EffectLevel         = 0;

    /* PSA Control parameters */
    pContext->pBundledContext->ActiveParams.PSA_Enable             = LVM_PSA_OFF;
    pContext->pBundledContext->ActiveParams.PSA_PeakDecayRate      = (LVM_PSA_DecaySpeed_en)0;

    /* Bass Enhancement parameters */
    pContext->pBundledContext->ActiveParams.BE_OperatingMode       = LVM_BE_OFF;
    pContext->pBundledContext->ActiveParams.BE_EffectLevel         = 0;
    pContext->pBundledContext->ActiveParams.BE_CentreFreq          = LVM_BE_CENTRE_90Hz;
    pContext->pBundledContext->ActiveParams.BE_HPF                 = LVM_BE_HPF_ON;

    /* PSA Control parameters */
    pContext->pBundledContext->ActiveParams.PSA_Enable             = LVM_PSA_OFF;
    pContext->pBundledContext->ActiveParams.PSA_PeakDecayRate      = LVM_PSA_SPEED_MEDIUM;

    /* TE Control parameters */
    pContext->pBundledContext->ActiveParams.TE_OperatingMode       = LVM_TE_OFF;
    pContext->pBundledContext->ActiveParams.TE_EffectLevel         = 0;

    /* Activate the initial settings */
    LvmStatus = Offload_SetEffect_ControlParameters(pContext);

    LVM_ERROR_CHECK(LvmStatus, "Offload_SetEffect_ControlParameters", "LvmBundle_init")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;

    ALOGV("\tNXPBundle Initial ControlParameters are configured Succesfully\n");
    ALOGV("\tLvmBundle_init End");
    return 0;
}   /* end LvmBundle_init */


//----------------------------------------------------------------------------
// LvmBundle_process()
//----------------------------------------------------------------------------
// Purpose:
// Apply LVM Bundle effects
//
// Inputs:
//  pIn:        pointer to stereo 16 bit input data
//  pOut:       pointer to stereo 16 bit output data
//  frameCount: Frames to process
//  pContext:   effect engine context
//  strength    strength to be applied
//
//  Outputs:
//  pOut:       pointer to updated stereo 16 bit output data
//
//----------------------------------------------------------------------------

int LvmBundle_process(LVM_INT16        *pIn __unused,
                      LVM_INT16        *pOut __unused,
                      int              frameCount __unused,
                      EffectContext    *pContext __unused){
    return 0;
}    /* end LvmBundle_process */


//----------------------------------------------------------------------------
// EqualizerUpdateActiveParams()
//----------------------------------------------------------------------------
// Purpose: Update ActiveParams for Equalizer
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------
void EqualizerUpdateActiveParams(EffectContext *pContext) {
    //LVM_ReturnStatus_en     LvmStatus=LVM_SUCCESS;     /* Function call status */

    /* current control settings are available in EffectContext structure use it and update
         the structure before sending the parameter */
    for (int i = 0; i < FIVEBAND_NUMBANDS; i++) {
           pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[i].Frequency = EQNB_5BandPresetsFrequencies[i];
           pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[i].QFactor   = EQNB_5BandPresetsQFactors[i];
           pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition[i].Gain = pContext->pBundledContext->bandGaindB[i];
       }

    //ALOGV("\tEqualizerUpdateActiveParams just Set -> %d\n",
    //          ActiveParams.pEQNB_BandDefinition[band].Gain);

}

//----------------------------------------------------------------------------
// LvmEffect_limitLevel()
//----------------------------------------------------------------------------
// Purpose: limit the overall level to a value less than 0 dB preserving
//          the overall EQ band gain and BassBoost relative levels.
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------
void LvmEffect_limitLevel(EffectContext *pContext) {
    LVM_ReturnStatus_en     LvmStatus=LVM_SUCCESS;     /* Function call status */

    /* current control settings are available in EffectContext structure use it and update
         the structure before sending the parameter */
    int gainCorrection = 0;
    //Count the energy contribution per band for EQ and BassBoost only if they are active.
    float energyContribution = 0;
    float energyCross = 0;
    float energyBassBoost = 0;
    float crossCorrection = 0;

    //EQ contribution
    if (pContext->pBundledContext->bEqualizerEnabled == LVM_TRUE) {
        for (int i = 0; i < FIVEBAND_NUMBANDS; i++) {
            float bandFactor = pContext->pBundledContext->bandGaindB[i]/15.0;
            float bandCoefficient = LimitLevel_bandEnergyCoefficient[i];
            float bandEnergy = bandFactor * bandCoefficient * bandCoefficient;
            if (bandEnergy > 0)
                energyContribution += bandEnergy;
        }

        //cross EQ coefficients
        float bandFactorSum = 0;
        for (int i = 0; i < FIVEBAND_NUMBANDS-1; i++) {
            float bandFactor1 = pContext->pBundledContext->bandGaindB[i]/15.0;
            float bandFactor2 = pContext->pBundledContext->bandGaindB[i+1]/15.0;

            if (bandFactor1 > 0 && bandFactor2 > 0) {
                float crossEnergy = bandFactor1 * bandFactor2 *
                        LimitLevel_bandEnergyCrossCoefficient[i];
                bandFactorSum += bandFactor1 * bandFactor2;

                if (crossEnergy > 0)
                    energyCross += crossEnergy;
            }
        }
        bandFactorSum -= 1.0;
        if (bandFactorSum > 0)
            crossCorrection = bandFactorSum * 0.7;
    }

    //BassBoost contribution
    if (pContext->pBundledContext->bBassEnabled == LVM_TRUE) {
        float boostFactor = (pContext->pBundledContext->BassStrengthSaved)/1000.0;
        float boostCoefficient = LimitLevel_bassBoostEnergyCoefficient;

        energyContribution += boostFactor * boostCoefficient * boostCoefficient;

        for (int i = 0; i < FIVEBAND_NUMBANDS; i++) {
            float bandFactor = pContext->pBundledContext->bandGaindB[i]/15.0;
            float bandCrossCoefficient = LimitLevel_bassBoostEnergyCrossCoefficient[i];
            float bandEnergy = boostFactor * bandFactor *
                    bandCrossCoefficient;
            if (bandEnergy > 0)
                energyBassBoost += bandEnergy;
        }
    }

    //Virtualizer contribution
    if (pContext->pBundledContext->bVirtualizerEnabled == LVM_TRUE) {
        energyContribution += LimitLevel_virtualizerContribution *
                LimitLevel_virtualizerContribution;
    }

    double totalEnergyEstimation = sqrt(energyContribution + energyCross + energyBassBoost) -
            crossCorrection;
    ALOGV(" TOTAL energy estimation: %0.2f", totalEnergyEstimation);

    //roundoff
    int maxLevelRound = (int)(totalEnergyEstimation + 0.99);
    if (maxLevelRound + pContext->pBundledContext->volume > 0) {
        gainCorrection = maxLevelRound + pContext->pBundledContext->volume;
    }

    pContext->pBundledContext->ActiveParams.VC_EffectLevel  = pContext->pBundledContext->volume - gainCorrection;
    if (pContext->pBundledContext->ActiveParams.VC_EffectLevel < -96) {
        pContext->pBundledContext->ActiveParams.VC_EffectLevel = -96;
    }
    ALOGV("\tVol:%d, GainCorrection: %d, Actual vol: %d", pContext->pBundledContext->volume,
            gainCorrection, pContext->pBundledContext->ActiveParams.VC_EffectLevel);

    /* Activate the initial settings */
    LvmStatus = Offload_SetEffect_ControlParameters(pContext);

    LVM_ERROR_CHECK(LvmStatus, "Offload_SetEffect_ControlParameters", "LvmEffect_limitLevel")
    if(LvmStatus != LVM_SUCCESS) return;

    //ALOGV("\tLvmEffect_limitLevel just set (-96dB -> 0dB) -> %d\n",ActiveParams.VC_EffectLevel );
#if 0 //*******SHOULD BE INFORMED TO FIRMWARE SIDE IMPLEMENTATION
    if (pContext->pBundledContext->firstVolume == LVM_TRUE){
        LvmStatus = LVM_SetVolumeNoSmoothing(pContext->pBundledContext->hInstance, &ActiveParams);
        LVM_ERROR_CHECK(LvmStatus, "LVM_SetVolumeNoSmoothing", "LvmBundle_process")
        ALOGV("\tLVM_VOLUME: Disabling Smoothing for first volume change to remove spikes/clicks");
        pContext->pBundledContext->firstVolume = LVM_FALSE;
    }
#endif
}

//----------------------------------------------------------------------------
// LvmEffect_enable()
//----------------------------------------------------------------------------
// Purpose: Enable the effect in the bundle
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------

int LvmEffect_enable(EffectContext *pContext){
    //ALOGV("\tLvmEffect_enable start");

    LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;                /* Function call status */

    /* current control settings are available in Context structure*/

    if(pContext->EffectType == LVM_BASS_BOOST) {
        ALOGV("\tLvmEffect_enable : Enabling LVM_BASS_BOOST");
        pContext->pBundledContext->ActiveParams.BE_OperatingMode       = LVM_BE_ON;
    }
    if(pContext->EffectType == LVM_VIRTUALIZER) {
        ALOGV("\tLvmEffect_enable : Enabling LVM_VIRTUALIZER");
        pContext->pBundledContext->ActiveParams.VirtualizerOperatingMode   = LVM_MODE_ON;
    }
    if(pContext->EffectType == LVM_EQUALIZER) {
        ALOGV("\tLvmEffect_enable : Enabling LVM_EQUALIZER");
        pContext->pBundledContext->ActiveParams.EQNB_OperatingMode     = LVM_EQNB_ON;
    }
    if(pContext->EffectType == LVM_VOLUME) {
        ALOGV("\tLvmEffect_enable : Enabling LVM_VOLUME");
    }

    LvmEffect_limitLevel(pContext);
    //ALOGV("\tLvmEffect_enable end");
    return 0;
}

//----------------------------------------------------------------------------
// LvmEffect_disable()
//----------------------------------------------------------------------------
// Purpose: Disable the effect in the bundle
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------

int LvmEffect_disable(EffectContext *pContext){
    //ALOGV("\tLvmEffect_disable start");

    LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;                /* Function call status */
    /* Get the current control settings from context structure*/

    if(pContext->EffectType == LVM_BASS_BOOST) {
        ALOGV("\tLvmEffect_disable : Disabling LVM_BASS_BOOST");
        pContext->pBundledContext->ActiveParams.BE_OperatingMode       = LVM_BE_OFF;
    }
    if(pContext->EffectType == LVM_VIRTUALIZER) {
        ALOGV("\tLvmEffect_disable : Disabling LVM_VIRTUALIZER");
        pContext->pBundledContext->ActiveParams.VirtualizerOperatingMode   = LVM_MODE_OFF;
    }
    if(pContext->EffectType == LVM_EQUALIZER) {
        ALOGV("\tLvmEffect_disable : Disabling LVM_EQUALIZER");
        pContext->pBundledContext->ActiveParams.EQNB_OperatingMode     = LVM_EQNB_OFF;
    }
    if(pContext->EffectType == LVM_VOLUME) {
        ALOGV("\tLvmEffect_disable : Disabling LVM_VOLUME");
    }

    LvmEffect_limitLevel(pContext);
    //ALOGV("\tLvmEffect_disable end");
    return 0;
}

//----------------------------------------------------------------------------
// Effect_setConfig()
//----------------------------------------------------------------------------
// Purpose: Set input and output audio configuration.
//
// Inputs:
//  pContext:   effect engine context
//  pConfig:    pointer to effect_config_t structure holding input and output
//      configuration parameters
//
// Outputs:
//
//----------------------------------------------------------------------------

int Effect_setConfig(EffectContext *pContext, effect_config_t *pConfig){
    LVM_Fs_en   SampleRate;
    //ALOGV("\tEffect_setConfig start");

    CHECK_ARG(pContext != NULL);
    CHECK_ARG(pConfig != NULL);

    CHECK_ARG(pConfig->inputCfg.samplingRate == pConfig->outputCfg.samplingRate);
    CHECK_ARG(pConfig->inputCfg.channels == pConfig->outputCfg.channels);
    CHECK_ARG(pConfig->inputCfg.format == pConfig->outputCfg.format);
    CHECK_ARG(pConfig->inputCfg.channels == AUDIO_CHANNEL_OUT_STEREO);
    CHECK_ARG(pConfig->outputCfg.accessMode == EFFECT_BUFFER_ACCESS_WRITE
              || pConfig->outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE);
    CHECK_ARG(pConfig->inputCfg.format == AUDIO_FORMAT_PCM_16_BIT);

    pContext->config = *pConfig;

    switch (pConfig->inputCfg.samplingRate) {
    case 8000:
        SampleRate = LVM_FS_8000;
        pContext->pBundledContext->SamplesPerSecond = 8000*2; // 2 secs Stereo
        break;
    case 16000:
        SampleRate = LVM_FS_16000;
        pContext->pBundledContext->SamplesPerSecond = 16000*2; // 2 secs Stereo
        break;
    case 22050:
        SampleRate = LVM_FS_22050;
        pContext->pBundledContext->SamplesPerSecond = 22050*2; // 2 secs Stereo
        break;
    case 32000:
        SampleRate = LVM_FS_32000;
        pContext->pBundledContext->SamplesPerSecond = 32000*2; // 2 secs Stereo
        break;
    case 44100:
        SampleRate = LVM_FS_44100;
        pContext->pBundledContext->SamplesPerSecond = 44100*2; // 2 secs Stereo
        break;
    case 48000:
        SampleRate = LVM_FS_48000;
        pContext->pBundledContext->SamplesPerSecond = 48000*2; // 2 secs Stereo
        break;
    default:
        ALOGV("\tEffect_setConfig invalid sampling rate %d", pConfig->inputCfg.samplingRate);
        return -EINVAL;
    }

    if(pContext->pBundledContext->SampleRate != SampleRate){

        LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;

        ALOGV("\tEffect_setConfig change sampling rate to %d", SampleRate);

        /* Use current control settings from context structure*/
        pContext->pBundledContext->ActiveParams.SampleRate = SampleRate;

        /* Activate the initial settings */
        LvmStatus = Offload_SetEffect_ControlParameters(pContext);

        LVM_ERROR_CHECK(LvmStatus, "Offload_SetEffect_ControlParameters", "Effect_setConfig")
        if(LvmStatus != LVM_SUCCESS) return -EINVAL;

        ALOGV("\tEffect_setConfig Succesfully called Offload_SetEffect_ControlParameters\n");
        pContext->pBundledContext->SampleRate = SampleRate;

    }else{
        //ALOGV("\tEffect_setConfig keep sampling rate at %d", SampleRate);
    }

    //ALOGV("\tEffect_setConfig End....");
    return 0;
}   /* end Effect_setConfig */

//----------------------------------------------------------------------------
// Effect_getConfig()
//----------------------------------------------------------------------------
// Purpose: Get input and output audio configuration.
//
// Inputs:
//  pContext:   effect engine context
//  pConfig:    pointer to effect_config_t structure holding input and output
//      configuration parameters
//
// Outputs:
//
//----------------------------------------------------------------------------

void Effect_getConfig(EffectContext *pContext, effect_config_t *pConfig)
{
    *pConfig = pContext->config;
}   /* end Effect_getConfig */

//----------------------------------------------------------------------------
// BassGetStrength()
//----------------------------------------------------------------------------
// Purpose:
// get the effect strength currently being used, what is actually returned is the strengh that was
// previously used in the set, this is because the app uses a strength in the range 0-1000 while
// the bassboost uses 1-15, so to avoid a quantisation the original set value is used. However the
// actual used value is checked to make sure it corresponds to the one being returned
//
// Inputs:
//  pContext:   effect engine context
//
//----------------------------------------------------------------------------

uint32_t BassGetStrength(EffectContext *pContext){
    //ALOGV("\tBassGetStrength() (0-1000) -> %d\n", pContext->pBundledContext->BassStrengthSaved);

    LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;                /* Function call status */
    /* Use the current control settings from Context structure*/
    /* Check that the strength returned matches the strength that was set earlier */
    if(pContext->pBundledContext->ActiveParams.BE_EffectLevel !=
       (LVM_INT16)((15*pContext->pBundledContext->BassStrengthSaved)/1000)){
        ALOGV("\tLVM_ERROR : BassGetStrength module strength does not match savedStrength %d %d\n",
                pContext->pBundledContext->ActiveParams.BE_EffectLevel, pContext->pBundledContext->BassStrengthSaved);
        return -EINVAL;
    }

    //ALOGV("\tBassGetStrength() (0-15)   -> %d\n", ActiveParams.BE_EffectLevel );
    //ALOGV("\tBassGetStrength() (saved)  -> %d\n", pContext->pBundledContext->BassStrengthSaved );
    return pContext->pBundledContext->BassStrengthSaved;
}    /* end BassGetStrength */

//----------------------------------------------------------------------------
// BassSetStrength()
//----------------------------------------------------------------------------
// Purpose:
// Apply the strength to the BassBosst. Must first be converted from the range 0-1000 to 1-15
//
// Inputs:
//  pContext:   effect engine context
//  strength    strength to be applied
//
//----------------------------------------------------------------------------

void BassSetStrength(EffectContext *pContext, uint32_t strength){
    //ALOGV("\tBassSetStrength(%d)", strength);

    pContext->pBundledContext->BassStrengthSaved = (int)strength;

    LVM_ReturnStatus_en     LvmStatus=LVM_SUCCESS;     /* Function call status */

    /* Use the current control settings from Context Structure */
    /* Bass Enhancement parameters */
    pContext->pBundledContext->ActiveParams.BE_EffectLevel    = (LVM_INT16)((15*strength)/1000);
    pContext->pBundledContext->ActiveParams.BE_CentreFreq     = LVM_BE_CENTRE_90Hz;

    //ALOGV("\tBassSetStrength() (0-15)   -> %d\n", ActiveParams.BE_EffectLevel );

    LvmEffect_limitLevel(pContext);
}    /* end BassSetStrength */

//----------------------------------------------------------------------------
// VirtualizerGetStrength()
//----------------------------------------------------------------------------
// Purpose:
// get the effect strength currently being used, what is actually returned is the strengh that was
// previously used in the set, this is because the app uses a strength in the range 0-1000 while
// the Virtualizer uses 1-100, so to avoid a quantisation the original set value is used.However the
// actual used value is checked to make sure it corresponds to the one being returned
//
// Inputs:
//  pContext:   effect engine context
//
//----------------------------------------------------------------------------

uint32_t VirtualizerGetStrength(EffectContext *pContext){
    //ALOGV("\tVirtualizerGetStrength (0-1000) -> %d\n",pContext->pBundledContext->VirtStrengthSaved);

    //ALOGV("\tVirtualizerGetStrength() (0-100)   -> %d\n", ActiveParams.VirtualizerReverbLevel*10);
    return pContext->pBundledContext->VirtStrengthSaved;
}    /* end getStrength */

//----------------------------------------------------------------------------
// VirtualizerSetStrength()
//----------------------------------------------------------------------------
// Purpose:
// Apply the strength to the Virtualizer. Must first be converted from the range 0-1000 to 1-15
//
// Inputs:
//  pContext:   effect engine context
//  strength    strength to be applied
//
//----------------------------------------------------------------------------

void VirtualizerSetStrength(EffectContext *pContext, uint32_t strength){
    //ALOGV("\tVirtualizerSetStrength(%d)", strength);
    LVM_ReturnStatus_en     LvmStatus=LVM_SUCCESS;     /* Function call status */

    pContext->pBundledContext->VirtStrengthSaved = (int)strength;

    /* Get the current control settings from Context structure */
    /* Virtualizer parameters */
    pContext->pBundledContext->ActiveParams.CS_EffectLevel             = (int)((strength*32767)/1000);

    ALOGV("\tVirtualizerSetStrength() (0-1000)   -> %d\n", strength );
    ALOGV("\tVirtualizerSetStrength() (0- 100)   -> %d\n", pContext->pBundledContext->ActiveParams.CS_EffectLevel );

    LvmEffect_limitLevel(pContext);
}    /* end setStrength */

//----------------------------------------------------------------------------
// VirtualizerIsDeviceSupported()
//----------------------------------------------------------------------------
// Purpose:
// Check if an audio device type is supported by this implementation
//
// Inputs:
//  deviceType   the type of device that affects the processing (e.g. for binaural vs transaural)
// Output:
//  -EINVAL      if the configuration is not supported or it is unknown
//  0            if the configuration is supported
//----------------------------------------------------------------------------
int VirtualizerIsDeviceSupported(audio_devices_t deviceType) {
    switch (deviceType) {
    case AUDIO_DEVICE_OUT_WIRED_HEADSET:
    case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
    case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES:
        return 0;
    default :
        return -EINVAL;
    }
}

//----------------------------------------------------------------------------
// VirtualizerIsConfigurationSupported()
//----------------------------------------------------------------------------
// Purpose:
// Check if a channel mask + audio device type is supported by this implementation
//
// Inputs:
//  channelMask  the channel mask of the input to virtualize
//  deviceType   the type of device that affects the processing (e.g. for binaural vs transaural)
// Output:
//  -EINVAL      if the configuration is not supported or it is unknown
//  0            if the configuration is supported
//----------------------------------------------------------------------------
int VirtualizerIsConfigurationSupported(audio_channel_mask_t channelMask,
        audio_devices_t deviceType) {
    uint32_t channelCount = audio_channel_count_from_out_mask(channelMask);
    if ((channelCount == 0) || (channelCount > 2)) {
        return -EINVAL;
    }

    return VirtualizerIsDeviceSupported(deviceType);
}

//----------------------------------------------------------------------------
// VirtualizerForceVirtualizationMode()
//----------------------------------------------------------------------------
// Purpose:
// Force the virtualization mode to that of the given audio device
//
// Inputs:
//  pContext     effect engine context
//  forcedDevice the type of device whose virtualization mode we'll always use
// Output:
//  -EINVAL      if the device is not supported or is unknown
//  0            if the device is supported and the virtualization mode forced
//
//----------------------------------------------------------------------------
int VirtualizerForceVirtualizationMode(EffectContext *pContext, audio_devices_t forcedDevice) {
    ALOGV("VirtualizerForceVirtualizationMode: forcedDev=0x%x enabled=%d tmpDisabled=%d",
            forcedDevice, pContext->pBundledContext->bVirtualizerEnabled,
            pContext->pBundledContext->bVirtualizerTempDisabled);
    int status = 0;
    bool useVirtualizer = false;

    if (VirtualizerIsDeviceSupported(forcedDevice) != 0) {
        if (forcedDevice != AUDIO_DEVICE_NONE) {
            //forced device is not supported, make it behave as a reset of forced mode
            forcedDevice = AUDIO_DEVICE_NONE;
            // but return an error
            status = -EINVAL;
        }
    }

    if (forcedDevice == AUDIO_DEVICE_NONE) {
        // disabling forced virtualization mode:
        // verify whether the virtualization should be enabled or disabled
        if (VirtualizerIsDeviceSupported(pContext->pBundledContext->nOutputDevice) == 0) {
            useVirtualizer = (pContext->pBundledContext->bVirtualizerEnabled == LVM_TRUE);
        }
        pContext->pBundledContext->nVirtualizerForcedDevice = AUDIO_DEVICE_NONE;
    } else {
        // forcing virtualization mode: here we already know the device is supported
        pContext->pBundledContext->nVirtualizerForcedDevice = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
        // only enable for a supported mode, when the effect is enabled
        useVirtualizer = (pContext->pBundledContext->bVirtualizerEnabled == LVM_TRUE);
    }

    if (useVirtualizer) {
        if (pContext->pBundledContext->bVirtualizerTempDisabled == LVM_TRUE) {
            ALOGV("\tVirtualizerForceVirtualizationMode re-enable LVM_VIRTUALIZER");
            android::LvmEffect_enable(pContext);
            pContext->pBundledContext->bVirtualizerTempDisabled = LVM_FALSE;
        } else {
            ALOGV("\tVirtualizerForceVirtualizationMode leaving LVM_VIRTUALIZER enabled");
        }
    } else {
        if (pContext->pBundledContext->bVirtualizerTempDisabled == LVM_FALSE) {
            ALOGV("\tVirtualizerForceVirtualizationMode disable LVM_VIRTUALIZER");
            android::LvmEffect_disable(pContext);
            pContext->pBundledContext->bVirtualizerTempDisabled = LVM_TRUE;
        } else {
            ALOGV("\tVirtualizerForceVirtualizationMode leaving LVM_VIRTUALIZER disabled");
        }
    }

    ALOGV("\tafter VirtualizerForceVirtualizationMode: enabled=%d tmpDisabled=%d",
            pContext->pBundledContext->bVirtualizerEnabled,
            pContext->pBundledContext->bVirtualizerTempDisabled);

    return status;
}
//----------------------------------------------------------------------------
// VirtualizerGetSpeakerAngles()
//----------------------------------------------------------------------------
// Purpose:
// Get the virtual speaker angles for a channel mask + audio device type
// configuration which is guaranteed to be supported by this implementation
//
// Inputs:
//  channelMask:   the channel mask of the input to virtualize
//  deviceType     the type of device that affects the processing (e.g. for binaural vs transaural)
// Input/Output:
//  pSpeakerAngles the array of integer where each speaker angle is written as a triplet in the
//                 following format:
//                    int32_t a bit mask with a single value selected for each speaker, following
//                            the convention of the audio_channel_mask_t type
//                    int32_t a value in degrees expressing the speaker azimuth, where 0 is in front
//                            of the user, 180 behind, -90 to the left, 90 to the right of the user
//                    int32_t a value in degrees expressing the speaker elevation, where 0 is the
//                            horizontal plane, +90 is directly above the user, -90 below
//
//----------------------------------------------------------------------------
void VirtualizerGetSpeakerAngles(audio_channel_mask_t channelMask __unused,
        audio_devices_t deviceType __unused, int32_t *pSpeakerAngles) {
    // the channel count is guaranteed to be 1 or 2
    // the device is guaranteed to be of type headphone
    // this virtualizer is always 2in with speakers at -90 and 90deg of azimuth, 0deg of elevation
    *pSpeakerAngles++ = (int32_t) AUDIO_CHANNEL_OUT_FRONT_LEFT;
    *pSpeakerAngles++ = -90; // azimuth
    *pSpeakerAngles++ = 0;   // elevation
    *pSpeakerAngles++ = (int32_t) AUDIO_CHANNEL_OUT_FRONT_RIGHT;
    *pSpeakerAngles++ = 90;  // azimuth
    *pSpeakerAngles   = 0;   // elevation
}

//----------------------------------------------------------------------------
// VirtualizerGetVirtualizationMode()
//----------------------------------------------------------------------------
// Purpose:
// Retrieve the current device whose processing mode is used by this effect
//
// Output:
//   AUDIO_DEVICE_NONE if the effect is not virtualizing
//   or the device type if the effect is virtualizing
//----------------------------------------------------------------------------
audio_devices_t VirtualizerGetVirtualizationMode(EffectContext *pContext) {
    audio_devices_t virtDevice = AUDIO_DEVICE_NONE;
    if ((pContext->pBundledContext->bVirtualizerEnabled == LVM_TRUE)
            && (pContext->pBundledContext->bVirtualizerTempDisabled == LVM_FALSE)) {
        if (pContext->pBundledContext->nVirtualizerForcedDevice != AUDIO_DEVICE_NONE) {
            // virtualization mode is forced, return that device
            virtDevice = pContext->pBundledContext->nVirtualizerForcedDevice;
        } else {
            // no forced mode, return the current device
            virtDevice = pContext->pBundledContext->nOutputDevice;
        }
    }
    ALOGV("VirtualizerGetVirtualizationMode() returning 0x%x", virtDevice);
    return virtDevice;
}

//----------------------------------------------------------------------------
// EqualizerGetBandLevel()
//----------------------------------------------------------------------------
// Purpose: Retrieve the gain currently being used for the band passed in
//
// Inputs:
//  band:       band number
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------
int32_t EqualizerGetBandLevel(EffectContext *pContext, int32_t band){
    //ALOGV("\tEqualizerGetBandLevel -> %d\n", pContext->pBundledContext->bandGaindB[band] );
    return pContext->pBundledContext->bandGaindB[band] * 100;
}

//----------------------------------------------------------------------------
// EqualizerSetBandLevel()
//----------------------------------------------------------------------------
// Purpose:
//  Sets gain value for the given band.
//
// Inputs:
//  band:       band number
//  Gain:       Gain to be applied in millibels
//  pContext:   effect engine context
//
// Outputs:
//
//---------------------------------------------------------------------------
void EqualizerSetBandLevel(EffectContext *pContext, int band, short Gain){
    int gainRounded;
    if(Gain > 0){
        gainRounded = (int)((Gain+50)/100);
    }else{
        gainRounded = (int)((Gain-50)/100);
    }
    //ALOGV("\tEqualizerSetBandLevel(%d)->(%d)", Gain, gainRounded);
    pContext->pBundledContext->bandGaindB[band] = gainRounded;
    pContext->pBundledContext->CurPreset = PRESET_CUSTOM;

    EqualizerUpdateActiveParams(pContext);
    LvmEffect_limitLevel(pContext);
}

//----------------------------------------------------------------------------
// EqualizerGetCentreFrequency()
//----------------------------------------------------------------------------
// Purpose: Retrieve the frequency being used for the band passed in
//
// Inputs:
//  band:       band number
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------
int32_t EqualizerGetCentreFrequency(EffectContext *pContext, int32_t band){
    int32_t Frequency =0;

    LVM_EQNB_BandDef_t      *BandDef;
    /* Use the current control settings from context structure */

    BandDef   = pContext->pBundledContext->ActiveParams.pEQNB_BandDefinition;
    Frequency = (int32_t)BandDef[band].Frequency*1000;     // Convert to millibels

    //ALOGV("\tEqualizerGetCentreFrequency -> %d\n", Frequency );
    return Frequency;
}

//----------------------------------------------------------------------------
// EqualizerGetBandFreqRange(
//----------------------------------------------------------------------------
// Purpose:
//
// Gets lower and upper boundaries of a band.
// For the high shelf, the low bound is the band frequency and the high
// bound is Nyquist.
// For the peaking filters, they are the gain[dB]/2 points.
//
// Inputs:
//  band:       band number
//  pContext:   effect engine context
//
// Outputs:
//  pLow:       lower band range
//  pLow:       upper band range
//----------------------------------------------------------------------------
int32_t EqualizerGetBandFreqRange(EffectContext *pContext __unused, int32_t band, uint32_t *pLow,
                                  uint32_t *pHi){
    *pLow = bandFreqRange[band][0];
    *pHi  = bandFreqRange[band][1];
    return 0;
}

//----------------------------------------------------------------------------
// EqualizerGetBand(
//----------------------------------------------------------------------------
// Purpose:
//
// Returns the band with the maximum influence on a given frequency.
// Result is unaffected by whether EQ is enabled or not, or by whether
// changes have been committed or not.
//
// Inputs:
//  targetFreq   The target frequency, in millihertz.
//  pContext:    effect engine context
//
// Outputs:
//  pLow:       lower band range
//  pLow:       upper band range
//----------------------------------------------------------------------------
int32_t EqualizerGetBand(EffectContext *pContext __unused, uint32_t targetFreq){
    int band = 0;

    if(targetFreq < bandFreqRange[0][0]){
        return -EINVAL;
    }else if(targetFreq == bandFreqRange[0][0]){
        return 0;
    }
    for(int i=0; i<FIVEBAND_NUMBANDS;i++){
        if((targetFreq > bandFreqRange[i][0])&&(targetFreq <= bandFreqRange[i][1])){
            band = i;
        }
    }
    return band;
}

//----------------------------------------------------------------------------
// EqualizerGetPreset(
//----------------------------------------------------------------------------
// Purpose:
//
// Gets the currently set preset ID.
// Will return PRESET_CUSTOM in case the EQ parameters have been modified
// manually since a preset was set.
//
// Inputs:
//  pContext:    effect engine context
//
//----------------------------------------------------------------------------
int32_t EqualizerGetPreset(EffectContext *pContext){
    return pContext->pBundledContext->CurPreset;
}

//----------------------------------------------------------------------------
// EqualizerSetPreset(
//----------------------------------------------------------------------------
// Purpose:
//
// Sets the current preset by ID.
// All the band parameters will be overridden.
//
// Inputs:
//  pContext:    effect engine context
//  preset       The preset ID.
//
//----------------------------------------------------------------------------
void EqualizerSetPreset(EffectContext *pContext, int preset){

    //ALOGV("\tEqualizerSetPreset(%d)", preset);
    pContext->pBundledContext->CurPreset = preset;

    //ActiveParams.pEQNB_BandDefinition = &BandDefs[0];
    for (int i=0; i<FIVEBAND_NUMBANDS; i++)
    {
        pContext->pBundledContext->bandGaindB[i] =
                EQNB_5BandSoftPresets[i + preset * FIVEBAND_NUMBANDS];
    }

    EqualizerUpdateActiveParams(pContext);
    LvmEffect_limitLevel(pContext);

    //ALOGV("\tEqualizerSetPreset Succesfully called LVM_SetControlParameters\n");
    return;
}

int32_t EqualizerGetNumPresets(){
    return sizeof(gEqualizerPresets) / sizeof(PresetConfig);
}

//----------------------------------------------------------------------------
// EqualizerGetPresetName(
//----------------------------------------------------------------------------
// Purpose:
// Gets a human-readable name for a preset ID. Will return "Custom" if
// PRESET_CUSTOM is passed.
//
// Inputs:
// preset       The preset ID. Must be less than number of presets.
//
//-------------------------------------------------------------------------
const char * EqualizerGetPresetName(int32_t preset){
    //ALOGV("\tEqualizerGetPresetName start(%d)", preset);
    if (preset == PRESET_CUSTOM) {
        return "Custom";
    } else {
        return gEqualizerPresets[preset].name;
    }
    //ALOGV("\tEqualizerGetPresetName end(%d)", preset);
    return 0;
}

//----------------------------------------------------------------------------
// VolumeSetVolumeLevel()
//----------------------------------------------------------------------------
// Purpose:
//
// Inputs:
//  pContext:   effect engine context
//  level       level to be applied
//
//----------------------------------------------------------------------------

int VolumeSetVolumeLevel(EffectContext *pContext, int16_t level){

    if (level > 0 || level < -9600) {
        return -EINVAL;
    }

    if (pContext->pBundledContext->bMuteEnabled == LVM_TRUE) {
        pContext->pBundledContext->levelSaved = level / 100;
    } else {
        pContext->pBundledContext->volume = level / 100;
    }

    LvmEffect_limitLevel(pContext);

    return 0;
}    /* end VolumeSetVolumeLevel */

//----------------------------------------------------------------------------
// VolumeGetVolumeLevel()
//----------------------------------------------------------------------------
// Purpose:
//
// Inputs:
//  pContext:   effect engine context
//
//----------------------------------------------------------------------------

int VolumeGetVolumeLevel(EffectContext *pContext, int16_t *level){

    if (pContext->pBundledContext->bMuteEnabled == LVM_TRUE) {
        *level = pContext->pBundledContext->levelSaved * 100;
    } else {
        *level = pContext->pBundledContext->volume * 100;
    }
    return 0;
}    /* end VolumeGetVolumeLevel */

//----------------------------------------------------------------------------
// VolumeSetMute()
//----------------------------------------------------------------------------
// Purpose:
//
// Inputs:
//  pContext:   effect engine context
//  mute:       enable/disable flag
//
//----------------------------------------------------------------------------

int32_t VolumeSetMute(EffectContext *pContext, uint32_t mute){
    //ALOGV("\tVolumeSetMute start(%d)", mute);

    pContext->pBundledContext->bMuteEnabled = mute;

    /* Set appropriate volume level */
    if(pContext->pBundledContext->bMuteEnabled == LVM_TRUE){
        pContext->pBundledContext->levelSaved = pContext->pBundledContext->volume;
        pContext->pBundledContext->volume = -96;
    }else{
        pContext->pBundledContext->volume = pContext->pBundledContext->levelSaved;
    }

    LvmEffect_limitLevel(pContext);

    return 0;
}    /* end setMute */

//----------------------------------------------------------------------------
// VolumeGetMute()
//----------------------------------------------------------------------------
// Purpose:
//
// Inputs:
//  pContext:   effect engine context
//
// Ourputs:
//  mute:       enable/disable flag
//----------------------------------------------------------------------------

int32_t VolumeGetMute(EffectContext *pContext, uint32_t *mute){
    //ALOGV("\tVolumeGetMute start");
    if((pContext->pBundledContext->bMuteEnabled == LVM_FALSE)||
       (pContext->pBundledContext->bMuteEnabled == LVM_TRUE)){
        *mute = pContext->pBundledContext->bMuteEnabled;
        return 0;
    }else{
        ALOGV("\tLVM_ERROR : VolumeGetMute read an invalid value from context %d",
              pContext->pBundledContext->bMuteEnabled);
        return -EINVAL;
    }
    //ALOGV("\tVolumeGetMute end");
}    /* end getMute */

int16_t VolumeConvertStereoPosition(int16_t position){
    int16_t convertedPosition = 0;

    convertedPosition = (int16_t)(((float)position/1000)*96);
    return convertedPosition;

}

//----------------------------------------------------------------------------
// VolumeSetStereoPosition()
//----------------------------------------------------------------------------
// Purpose:
//
// Inputs:
//  pContext:       effect engine context
//  position:       stereo position
//
// Outputs:
//----------------------------------------------------------------------------

int VolumeSetStereoPosition(EffectContext *pContext, int16_t position){

    LVM_ReturnStatus_en     LvmStatus=LVM_SUCCESS;     /* Function call status */
    LVM_INT16               Balance = 0;



    pContext->pBundledContext->positionSaved = position;
    Balance = VolumeConvertStereoPosition(pContext->pBundledContext->positionSaved);

    //ALOGV("\tVolumeSetStereoPosition start pContext->pBundledContext->positionSaved = %d",
    //pContext->pBundledContext->positionSaved);

    if(pContext->pBundledContext->bStereoPositionEnabled == LVM_TRUE){

        //ALOGV("\tVolumeSetStereoPosition Position to be set is %d %d\n", position, Balance);
        pContext->pBundledContext->positionSaved = position;
        /* Use the current control settings from Context structure */
        //ALOGV("\tVolumeSetStereoPosition curent VC_Balance: %d\n", pContext->pBundledContext->ActiveParams.VC_Balance);

        /* Volume parameters */
        pContext->pBundledContext->ActiveParams.VC_Balance  = Balance;
        //ALOGV("\tVolumeSetStereoPosition() (-96dB -> +96dB)   -> %d\n", pContext->pBundledContext->ActiveParams.VC_Balance );

        /* Activate the initial settings */
        LvmStatus = Offload_SetEffect_ControlParameters(pContext);

        LVM_ERROR_CHECK(LvmStatus, "Offload_SetEffect_ControlParameters", "VolumeSetStereoPosition")
        if(LvmStatus != LVM_SUCCESS) return -EINVAL;

        //ALOGV("\tVolumeSetStereoPosition Succesfully called Offload_SetEffect_ControlParameters\n");

    }
    else{
        //ALOGV("\tVolumeSetStereoPosition Position attempting to set, but not enabled %d %d\n",
        //position, Balance);
    }
    //ALOGV("\tVolumeSetStereoPosition end pContext->pBundledContext->positionSaved = %d\n",
    //pContext->pBundledContext->positionSaved);
    return 0;
}    /* end VolumeSetStereoPosition */


//----------------------------------------------------------------------------
// VolumeGetStereoPosition()
//----------------------------------------------------------------------------
// Purpose:
//
// Inputs:
//  pContext:       effect engine context
//
// Outputs:
//  position:       stereo position
//----------------------------------------------------------------------------

int32_t VolumeGetStereoPosition(EffectContext *pContext, int16_t *position){
    //ALOGV("\tVolumeGetStereoPosition start");

    LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;                /* Function call status */
    LVM_INT16               balance;

    //ALOGV("\tVolumeGetStereoPosition start pContext->pBundledContext->positionSaved = %d",
    //pContext->pBundledContext->positionSaved);

    //ALOGV("\tVolumeGetStereoPosition -> %d\n", pContext->pBundledContext->ActiveParams.VC_Balance);
    //ALOGV("\tVolumeGetStereoPosition Succesfully returned from LVM_GetControlParameters\n");

    balance = VolumeConvertStereoPosition(pContext->pBundledContext->positionSaved);

    if(pContext->pBundledContext->bStereoPositionEnabled == LVM_TRUE){
        if(balance != pContext->pBundledContext->ActiveParams.VC_Balance){
            return -EINVAL;
        }
    }
    *position = (LVM_INT16)pContext->pBundledContext->positionSaved;     // Convert dB to millibels
    //ALOGV("\tVolumeGetStereoPosition end returning pContext->pBundledContext->positionSaved =%d\n",
    //pContext->pBundledContext->positionSaved);
    return 0;
}    /* end VolumeGetStereoPosition */

//----------------------------------------------------------------------------
// VolumeEnableStereoPosition()
//----------------------------------------------------------------------------
// Purpose:
//
// Inputs:
//  pContext:   effect engine context
//  mute:       enable/disable flag
//
//----------------------------------------------------------------------------

int32_t VolumeEnableStereoPosition(EffectContext *pContext, uint32_t enabled){
    //ALOGV("\tVolumeEnableStereoPosition start()");

    pContext->pBundledContext->bStereoPositionEnabled = enabled;

    LVM_ReturnStatus_en     LvmStatus=LVM_SUCCESS;     /* Function call status */

    /* Use the current control settings from context structure */
    //ALOGV("\tVolumeEnableStereoPosition to %d, position was %d\n",
    //     enabled, pContext->pBundledContext->ActiveParams.VC_Balance );

    /* Set appropriate stereo position */
    if(pContext->pBundledContext->bStereoPositionEnabled == LVM_FALSE){
        pContext->pBundledContext->ActiveParams.VC_Balance = 0;
    }else{
        pContext->pBundledContext->ActiveParams.VC_Balance  =
                            VolumeConvertStereoPosition(pContext->pBundledContext->positionSaved);
    }

    /* Activate the initial settings */
    LvmStatus = Offload_SetEffect_ControlParameters(pContext);

    LVM_ERROR_CHECK(LvmStatus, "Offload_SetEffect_ControlParameters", "VolumeEnableStereoPosition")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;

    //ALOGV("\tVolumeEnableStereoPosition Succesfully called Offload_SetEffect_ControlParameters\n");
    //ALOGV("\tVolumeEnableStereoPosition end()\n");
    return 0;
}    /* end VolumeEnableStereoPosition */

//----------------------------------------------------------------------------
// BassBoost_getParameter()
//----------------------------------------------------------------------------
// Purpose:
// Get a BassBoost parameter
//
// Inputs:
//  pBassBoost       - handle to instance data
//  pParam           - pointer to parameter
//  pValue           - pointer to variable to hold retrieved value
//  pValueSize       - pointer to value size: maximum size as input
//
// Outputs:
//  *pValue updated with parameter value
//  *pValueSize updated with actual value size
//
//
// Side Effects:
//
//----------------------------------------------------------------------------

int BassBoost_getParameter(EffectContext     *pContext,
                           void              *pParam,
                           uint32_t          *pValueSize,
                           void              *pValue){
    int status = 0;
    int32_t *pParamTemp = (int32_t *)pParam;
    int32_t param = *pParamTemp++;
    int32_t param2;
    char *name;

    //ALOGV("\tBassBoost_getParameter start");

    switch (param){
        case BASSBOOST_PARAM_STRENGTH_SUPPORTED:
            if (*pValueSize != sizeof(uint32_t)){
                ALOGV("\tLVM_ERROR : BassBoost_getParameter() invalid pValueSize1 %d", *pValueSize);
                return -EINVAL;
            }
            *pValueSize = sizeof(uint32_t);
            break;
        case BASSBOOST_PARAM_STRENGTH:
            if (*pValueSize != sizeof(int16_t)){
                ALOGV("\tLVM_ERROR : BassBoost_getParameter() invalid pValueSize2 %d", *pValueSize);
                return -EINVAL;
            }
            *pValueSize = sizeof(int16_t);
            break;

        default:
            ALOGV("\tLVM_ERROR : BassBoost_getParameter() invalid param %d", param);
            return -EINVAL;
    }

    switch (param){
        case BASSBOOST_PARAM_STRENGTH_SUPPORTED:
            *(uint32_t *)pValue = 1;

            //ALOGV("\tBassBoost_getParameter() BASSBOOST_PARAM_STRENGTH_SUPPORTED Value is %d",
            //        *(uint32_t *)pValue);
            break;

        case BASSBOOST_PARAM_STRENGTH:
            *(int16_t *)pValue = BassGetStrength(pContext);

            //ALOGV("\tBassBoost_getParameter() BASSBOOST_PARAM_STRENGTH Value is %d",
            //        *(int16_t *)pValue);
            break;

        default:
            ALOGV("\tLVM_ERROR : BassBoost_getParameter() invalid param %d", param);
            status = -EINVAL;
            break;
    }

    //ALOGV("\tBassBoost_getParameter end");
    return status;
} /* end BassBoost_getParameter */

//----------------------------------------------------------------------------
// BassBoost_setParameter()
//----------------------------------------------------------------------------
// Purpose:
// Set a BassBoost parameter
//
// Inputs:
//  pBassBoost       - handle to instance data
//  pParam           - pointer to parameter
//  pValue           - pointer to value
//
// Outputs:
//
//----------------------------------------------------------------------------

int BassBoost_setParameter (EffectContext *pContext, void *pParam, void *pValue){
    int status = 0;
    int16_t strength;
    int32_t *pParamTemp = (int32_t *)pParam;

    //ALOGV("\tBassBoost_setParameter start");

    switch (*pParamTemp){
        case BASSBOOST_PARAM_STRENGTH:
            strength = *(int16_t *)pValue;
            //ALOGV("\tBassBoost_setParameter() BASSBOOST_PARAM_STRENGTH value is %d", strength);
            //ALOGV("\tBassBoost_setParameter() Calling pBassBoost->BassSetStrength");
            BassSetStrength(pContext, (int32_t)strength);
            //ALOGV("\tBassBoost_setParameter() Called pBassBoost->BassSetStrength");
           break;
        default:
            ALOGV("\tLVM_ERROR : BassBoost_setParameter() invalid param %d", *pParamTemp);
            break;
    }

    //ALOGV("\tBassBoost_setParameter end");
    return status;
} /* end BassBoost_setParameter */

//----------------------------------------------------------------------------
// Virtualizer_getParameter()
//----------------------------------------------------------------------------
// Purpose:
// Get a Virtualizer parameter
//
// Inputs:
//  pVirtualizer     - handle to instance data
//  pParam           - pointer to parameter
//  pValue           - pointer to variable to hold retrieved value
//  pValueSize       - pointer to value size: maximum size as input
//
// Outputs:
//  *pValue updated with parameter value
//  *pValueSize updated with actual value size
//
//
// Side Effects:
//
//----------------------------------------------------------------------------

int Virtualizer_getParameter(EffectContext        *pContext,
                             void                 *pParam,
                             uint32_t             *pValueSize,
                             void                 *pValue){
    int status = 0;
    int32_t *pParamTemp = (int32_t *)pParam;
    int32_t param = *pParamTemp++;
    char *name;

    //ALOGV("\tVirtualizer_getParameter start");

    switch (param){
        case VIRTUALIZER_PARAM_STRENGTH_SUPPORTED:
            if (*pValueSize != sizeof(uint32_t)){
                ALOGV("\tLVM_ERROR : Virtualizer_getParameter() invalid pValueSize %d",*pValueSize);
                return -EINVAL;
            }
            *pValueSize = sizeof(uint32_t);
            break;
        case VIRTUALIZER_PARAM_STRENGTH:
            if (*pValueSize != sizeof(int16_t)){
                ALOGV("\tLVM_ERROR : Virtualizer_getParameter() invalid pValueSize2 %d",*pValueSize);
                return -EINVAL;
            }
            *pValueSize = sizeof(int16_t);
            break;
        case VIRTUALIZER_PARAM_VIRTUAL_SPEAKER_ANGLES:
            // return value size can only be interpreted as relative to input value,
            // deferring validity check to below
            break;
        case VIRTUALIZER_PARAM_VIRTUALIZATION_MODE:
            if (*pValueSize != sizeof(uint32_t)){
                ALOGV("\tLVM_ERROR : Virtualizer_getParameter() invalid pValueSize %d",*pValueSize);
                return -EINVAL;
            }
            *pValueSize = sizeof(uint32_t);
            break;
        default:
            ALOGV("\tLVM_ERROR : Virtualizer_getParameter() invalid param %d", param);
            return -EINVAL;
    }

    switch (param){
        case VIRTUALIZER_PARAM_STRENGTH_SUPPORTED:
            *(uint32_t *)pValue = 1;

            //ALOGV("\tVirtualizer_getParameter() VIRTUALIZER_PARAM_STRENGTH_SUPPORTED Value is %d",
            //        *(uint32_t *)pValue);
            break;

        case VIRTUALIZER_PARAM_STRENGTH:
            *(int16_t *)pValue = VirtualizerGetStrength(pContext);

            //ALOGV("\tVirtualizer_getParameter() VIRTUALIZER_PARAM_STRENGTH Value is %d",
            //        *(int16_t *)pValue);
            break;

        case VIRTUALIZER_PARAM_VIRTUAL_SPEAKER_ANGLES: {
            const audio_channel_mask_t channelMask = (audio_channel_mask_t) *pParamTemp++;
            const audio_devices_t deviceType = (audio_devices_t) *pParamTemp;
            uint32_t nbChannels = audio_channel_count_from_out_mask(channelMask);
            if (*pValueSize < 3 * nbChannels * sizeof(int32_t)){
                ALOGV("\tLVM_ERROR : Virtualizer_getParameter() invalid pValueSize %d",*pValueSize);
                return -EINVAL;
            }
            // verify the configuration is supported
            status = VirtualizerIsConfigurationSupported(channelMask, deviceType);
            if (status == 0) {
                ALOGV("VIRTUALIZER_PARAM_VIRTUAL_SPEAKER_ANGLES supports mask=0x%x device=0x%x",
                        channelMask, deviceType);
                // configuration is supported, get the angles
                VirtualizerGetSpeakerAngles(channelMask, deviceType, (int32_t *)pValue);
            }
            }
            break;

        case VIRTUALIZER_PARAM_VIRTUALIZATION_MODE:
            *(uint32_t *)pValue  = (uint32_t) VirtualizerGetVirtualizationMode(pContext);
            break;

        default:
            ALOGV("\tLVM_ERROR : Virtualizer_getParameter() invalid param %d", param);
            status = -EINVAL;
            break;
    }

    ALOGV("\tVirtualizer_getParameter end returning status=%d", status);
    return status;
} /* end Virtualizer_getParameter */

//----------------------------------------------------------------------------
// Virtualizer_setParameter()
//----------------------------------------------------------------------------
// Purpose:
// Set a Virtualizer parameter
//
// Inputs:
//  pVirtualizer     - handle to instance data
//  pParam           - pointer to parameter
//  pValue           - pointer to value
//
// Outputs:
//
//----------------------------------------------------------------------------

int Virtualizer_setParameter (EffectContext *pContext, void *pParam, void *pValue){
    int status = 0;
    int16_t strength;
    int32_t *pParamTemp = (int32_t *)pParam;
    int32_t param = *pParamTemp++;

    //ALOGV("\tVirtualizer_setParameter start");

    switch (param){
        case VIRTUALIZER_PARAM_STRENGTH:
            strength = *(int16_t *)pValue;
            //ALOGV("\tVirtualizer_setParameter() VIRTUALIZER_PARAM_STRENGTH value is %d", strength);
            //ALOGV("\tVirtualizer_setParameter() Calling pVirtualizer->setStrength");
            VirtualizerSetStrength(pContext, (int32_t)strength);
            ALOGV("\tVirtualizer_setParameter() Called pVirtualizer->setStrength");
           break;

        case VIRTUALIZER_PARAM_FORCE_VIRTUALIZATION_MODE: {
            const audio_devices_t deviceType = *(audio_devices_t *) pValue;
            status = VirtualizerForceVirtualizationMode(pContext, deviceType);
            //ALOGV("VIRTUALIZER_PARAM_FORCE_VIRTUALIZATION_MODE device=0x%x result=%d",
            //        deviceType, status);
            }
            break;

        default:
            ALOGV("\tLVM_ERROR : Virtualizer_setParameter() invalid param %d", param);
            break;
    }

    //ALOGV("\tVirtualizer_setParameter end");
    return status;
} /* end Virtualizer_setParameter */

//----------------------------------------------------------------------------
// Equalizer_getParameter()
//----------------------------------------------------------------------------
// Purpose:
// Get a Equalizer parameter
//
// Inputs:
//  pEqualizer       - handle to instance data
//  pParam           - pointer to parameter
//  pValue           - pointer to variable to hold retrieved value
//  pValueSize       - pointer to value size: maximum size as input
//
// Outputs:
//  *pValue updated with parameter value
//  *pValueSize updated with actual value size
//
//
// Side Effects:
//
//----------------------------------------------------------------------------
int Equalizer_getParameter(EffectContext     *pContext,
                           void              *pParam,
                           uint32_t          *pValueSize,
                           void              *pValue){
    int status = 0;
    int bMute = 0;
    int32_t *pParamTemp = (int32_t *)pParam;
    int32_t param = *pParamTemp++;
    int32_t param2;
    char *name;

    //ALOGV("\tEqualizer_getParameter start");

    switch (param) {
    case EQ_PARAM_NUM_BANDS:
    case EQ_PARAM_CUR_PRESET:
    case EQ_PARAM_GET_NUM_OF_PRESETS:
    case EQ_PARAM_BAND_LEVEL:
    case EQ_PARAM_GET_BAND:
        if (*pValueSize < sizeof(int16_t)) {
            ALOGV("\tLVM_ERROR : Equalizer_getParameter() invalid pValueSize 1  %d", *pValueSize);
            return -EINVAL;
        }
        *pValueSize = sizeof(int16_t);
        break;

    case EQ_PARAM_LEVEL_RANGE:
        if (*pValueSize < 2 * sizeof(int16_t)) {
            ALOGV("\tLVM_ERROR : Equalizer_getParameter() invalid pValueSize 2  %d", *pValueSize);
            return -EINVAL;
        }
        *pValueSize = 2 * sizeof(int16_t);
        break;
    case EQ_PARAM_BAND_FREQ_RANGE:
        if (*pValueSize < 2 * sizeof(int32_t)) {
            ALOGV("\tLVM_ERROR : Equalizer_getParameter() invalid pValueSize 3  %d", *pValueSize);
            return -EINVAL;
        }
        *pValueSize = 2 * sizeof(int32_t);
        break;

    case EQ_PARAM_CENTER_FREQ:
        if (*pValueSize < sizeof(int32_t)) {
            ALOGV("\tLVM_ERROR : Equalizer_getParameter() invalid pValueSize 5  %d", *pValueSize);
            return -EINVAL;
        }
        *pValueSize = sizeof(int32_t);
        break;

    case EQ_PARAM_GET_PRESET_NAME:
        break;

    case EQ_PARAM_PROPERTIES:
        if (*pValueSize < (2 + FIVEBAND_NUMBANDS) * sizeof(uint16_t)) {
            ALOGV("\tLVM_ERROR : Equalizer_getParameter() invalid pValueSize 1  %d", *pValueSize);
            return -EINVAL;
        }
        *pValueSize = (2 + FIVEBAND_NUMBANDS) * sizeof(uint16_t);
        break;

    default:
        ALOGV("\tLVM_ERROR : Equalizer_getParameter unknown param %d", param);
        return -EINVAL;
    }

    switch (param) {
    case EQ_PARAM_NUM_BANDS:
        *(uint16_t *)pValue = (uint16_t)FIVEBAND_NUMBANDS;
        //ALOGV("\tEqualizer_getParameter() EQ_PARAM_NUM_BANDS %d", *(int16_t *)pValue);
        break;

    case EQ_PARAM_LEVEL_RANGE:
        *(int16_t *)pValue = -1500;
        *((int16_t *)pValue + 1) = 1500;
        //ALOGV("\tEqualizer_getParameter() EQ_PARAM_LEVEL_RANGE min %d, max %d",
        //      *(int16_t *)pValue, *((int16_t *)pValue + 1));
        break;

    case EQ_PARAM_BAND_LEVEL:
        param2 = *pParamTemp;
        if (param2 >= FIVEBAND_NUMBANDS) {
            status = -EINVAL;
            break;
        }
        *(int16_t *)pValue = (int16_t)EqualizerGetBandLevel(pContext, param2);
        //ALOGV("\tEqualizer_getParameter() EQ_PARAM_BAND_LEVEL band %d, level %d",
        //      param2, *(int32_t *)pValue);
        break;

    case EQ_PARAM_CENTER_FREQ:
        param2 = *pParamTemp;
        if (param2 >= FIVEBAND_NUMBANDS) {
            status = -EINVAL;
            break;
        }
        *(int32_t *)pValue = EqualizerGetCentreFrequency(pContext, param2);
        //ALOGV("\tEqualizer_getParameter() EQ_PARAM_CENTER_FREQ band %d, frequency %d",
        //      param2, *(int32_t *)pValue);
        break;

    case EQ_PARAM_BAND_FREQ_RANGE:
        param2 = *pParamTemp;
        if (param2 >= FIVEBAND_NUMBANDS) {
            status = -EINVAL;
            break;
        }
        EqualizerGetBandFreqRange(pContext, param2, (uint32_t *)pValue, ((uint32_t *)pValue + 1));
        //ALOGV("\tEqualizer_getParameter() EQ_PARAM_BAND_FREQ_RANGE band %d, min %d, max %d",
        //      param2, *(int32_t *)pValue, *((int32_t *)pValue + 1));
        break;

    case EQ_PARAM_GET_BAND:
        param2 = *pParamTemp;
        *(uint16_t *)pValue = (uint16_t)EqualizerGetBand(pContext, param2);
        //ALOGV("\tEqualizer_getParameter() EQ_PARAM_GET_BAND frequency %d, band %d",
        //      param2, *(uint16_t *)pValue);
        break;

    case EQ_PARAM_CUR_PRESET:
        *(uint16_t *)pValue = (uint16_t)EqualizerGetPreset(pContext);
        //ALOGV("\tEqualizer_getParameter() EQ_PARAM_CUR_PRESET %d", *(int32_t *)pValue);
        break;

    case EQ_PARAM_GET_NUM_OF_PRESETS:
        *(uint16_t *)pValue = (uint16_t)EqualizerGetNumPresets();
        //ALOGV("\tEqualizer_getParameter() EQ_PARAM_GET_NUM_OF_PRESETS %d", *(int16_t *)pValue);
        break;

    case EQ_PARAM_GET_PRESET_NAME:
        param2 = *pParamTemp;
        if (param2 >= EqualizerGetNumPresets()) {
        //if (param2 >= 20) {     // AGO FIX
            status = -EINVAL;
            break;
        }
        name = (char *)pValue;
        strncpy(name, EqualizerGetPresetName(param2), *pValueSize - 1);
        name[*pValueSize - 1] = 0;
        *pValueSize = strlen(name) + 1;
        //ALOGV("\tEqualizer_getParameter() EQ_PARAM_GET_PRESET_NAME preset %d, name %s len %d",
        //      param2, gEqualizerPresets[param2].name, *pValueSize);
        break;

    case EQ_PARAM_PROPERTIES: {
        int16_t *p = (int16_t *)pValue;
        ALOGV("\tEqualizer_getParameter() EQ_PARAM_PROPERTIES");
        p[0] = (int16_t)EqualizerGetPreset(pContext);
        p[1] = (int16_t)FIVEBAND_NUMBANDS;
        for (int i = 0; i < FIVEBAND_NUMBANDS; i++) {
            p[2 + i] = (int16_t)EqualizerGetBandLevel(pContext, i);
        }
    } break;

    default:
        ALOGV("\tLVM_ERROR : Equalizer_getParameter() invalid param %d", param);
        status = -EINVAL;
        break;
    }

    //GV("\tEqualizer_getParameter end\n");
    return status;
} /* end Equalizer_getParameter */

//----------------------------------------------------------------------------
// Equalizer_setParameter()
//----------------------------------------------------------------------------
// Purpose:
// Set a Equalizer parameter
//
// Inputs:
//  pEqualizer    - handle to instance data
//  pParam        - pointer to parameter
//  pValue        - pointer to value
//
// Outputs:
//
//----------------------------------------------------------------------------
int Equalizer_setParameter (EffectContext *pContext, void *pParam, void *pValue){
    int status = 0;
    int32_t preset;
    int32_t band;
    int32_t level;
    int32_t *pParamTemp = (int32_t *)pParam;
    int32_t param = *pParamTemp++;


    //ALOGV("\tEqualizer_setParameter start");
    switch (param) {
    case EQ_PARAM_CUR_PRESET:
        preset = (int32_t)(*(uint16_t *)pValue);

        //ALOGV("\tEqualizer_setParameter() EQ_PARAM_CUR_PRESET %d", preset);
        if ((preset >= EqualizerGetNumPresets())||(preset < 0)) {
            status = -EINVAL;
            break;
        }
        EqualizerSetPreset(pContext, preset);
        break;
    case EQ_PARAM_BAND_LEVEL:
        band =  *pParamTemp;
        level = (int32_t)(*(int16_t *)pValue);
        //ALOGV("\tEqualizer_setParameter() EQ_PARAM_BAND_LEVEL band %d, level %d", band, level);
        if (band >= FIVEBAND_NUMBANDS) {
            status = -EINVAL;
            break;
        }
        EqualizerSetBandLevel(pContext, band, level);
        break;
    case EQ_PARAM_PROPERTIES: {
        //ALOGV("\tEqualizer_setParameter() EQ_PARAM_PROPERTIES");
        int16_t *p = (int16_t *)pValue;
        if ((int)p[0] >= EqualizerGetNumPresets()) {
            status = -EINVAL;
            break;
        }
        if (p[0] >= 0) {
            EqualizerSetPreset(pContext, (int)p[0]);
        } else {
            if ((int)p[1] != FIVEBAND_NUMBANDS) {
                status = -EINVAL;
                break;
            }
            for (int i = 0; i < FIVEBAND_NUMBANDS; i++) {
                EqualizerSetBandLevel(pContext, i, (int)p[2 + i]);
            }
        }
    } break;
    default:
        ALOGV("\tLVM_ERROR : Equalizer_setParameter() invalid param %d", param);
        status = -EINVAL;
        break;
    }

    //ALOGV("\tEqualizer_setParameter end");
    return status;
} /* end Equalizer_setParameter */

//----------------------------------------------------------------------------
// Volume_getParameter()
//----------------------------------------------------------------------------
// Purpose:
// Get a Volume parameter
//
// Inputs:
//  pVolume          - handle to instance data
//  pParam           - pointer to parameter
//  pValue           - pointer to variable to hold retrieved value
//  pValueSize       - pointer to value size: maximum size as input
//
// Outputs:
//  *pValue updated with parameter value
//  *pValueSize updated with actual value size
//
//
// Side Effects:
//
//----------------------------------------------------------------------------

int Volume_getParameter(EffectContext     *pContext,
                        void              *pParam,
                        uint32_t          *pValueSize,
                        void              *pValue){
    int status = 0;
    int bMute = 0;
    int32_t *pParamTemp = (int32_t *)pParam;
    int32_t param = *pParamTemp++;;
    char *name;

    //ALOGV("\tVolume_getParameter start");

    switch (param){
        case VOLUME_PARAM_LEVEL:
        case VOLUME_PARAM_MAXLEVEL:
        case VOLUME_PARAM_STEREOPOSITION:
            if (*pValueSize != sizeof(int16_t)){
                ALOGV("\tLVM_ERROR : Volume_getParameter() invalid pValueSize 1  %d", *pValueSize);
                return -EINVAL;
            }
            *pValueSize = sizeof(int16_t);
            break;

        case VOLUME_PARAM_MUTE:
        case VOLUME_PARAM_ENABLESTEREOPOSITION:
            if (*pValueSize < sizeof(int32_t)){
                ALOGV("\tLVM_ERROR : Volume_getParameter() invalid pValueSize 2  %d", *pValueSize);
                return -EINVAL;
            }
            *pValueSize = sizeof(int32_t);
            break;

        default:
            ALOGV("\tLVM_ERROR : Volume_getParameter unknown param %d", param);
            return -EINVAL;
    }

    switch (param){
        case VOLUME_PARAM_LEVEL:
            status = VolumeGetVolumeLevel(pContext, (int16_t *)(pValue));
            //ALOGV("\tVolume_getParameter() VOLUME_PARAM_LEVEL Value is %d",
            //        *(int16_t *)pValue);
            break;

        case VOLUME_PARAM_MAXLEVEL:
            *(int16_t *)pValue = 0;
            //ALOGV("\tVolume_getParameter() VOLUME_PARAM_MAXLEVEL Value is %d",
            //        *(int16_t *)pValue);
            break;

        case VOLUME_PARAM_STEREOPOSITION:
            VolumeGetStereoPosition(pContext, (int16_t *)pValue);
            //ALOGV("\tVolume_getParameter() VOLUME_PARAM_STEREOPOSITION Value is %d",
            //        *(int16_t *)pValue);
            break;

        case VOLUME_PARAM_MUTE:
            status = VolumeGetMute(pContext, (uint32_t *)pValue);
            ALOGV("\tVolume_getParameter() VOLUME_PARAM_MUTE Value is %d",
                    *(uint32_t *)pValue);
            break;

        case VOLUME_PARAM_ENABLESTEREOPOSITION:
            *(int32_t *)pValue = pContext->pBundledContext->bStereoPositionEnabled;
            //ALOGV("\tVolume_getParameter() VOLUME_PARAM_ENABLESTEREOPOSITION Value is %d",
            //        *(uint32_t *)pValue);
            break;

        default:
            ALOGV("\tLVM_ERROR : Volume_getParameter() invalid param %d", param);
            status = -EINVAL;
            break;
    }

    //ALOGV("\tVolume_getParameter end");
    return status;
} /* end Volume_getParameter */


//----------------------------------------------------------------------------
// Volume_setParameter()
//----------------------------------------------------------------------------
// Purpose:
// Set a Volume parameter
//
// Inputs:
//  pVolume       - handle to instance data
//  pParam        - pointer to parameter
//  pValue        - pointer to value
//
// Outputs:
//
//----------------------------------------------------------------------------

int Volume_setParameter (EffectContext *pContext, void *pParam, void *pValue){
    int      status = 0;
    int16_t  level;
    int16_t  position;
    uint32_t mute;
    uint32_t positionEnabled;
    int32_t *pParamTemp = (int32_t *)pParam;
    int32_t param = *pParamTemp++;

    //ALOGV("\tVolume_setParameter start");

    switch (param){
        case VOLUME_PARAM_LEVEL:
            level = *(int16_t *)pValue;
            //ALOGV("\tVolume_setParameter() VOLUME_PARAM_LEVEL value is %d", level);
            //ALOGV("\tVolume_setParameter() Calling pVolume->setVolumeLevel");
            status = VolumeSetVolumeLevel(pContext, (int16_t)level);
            //ALOGV("\tVolume_setParameter() Called pVolume->setVolumeLevel");
            break;

        case VOLUME_PARAM_MUTE:
            mute = *(uint32_t *)pValue;
            //ALOGV("\tVolume_setParameter() Calling pVolume->setMute, mute is %d", mute);
            //ALOGV("\tVolume_setParameter() Calling pVolume->setMute");
            status = VolumeSetMute(pContext, mute);
            //ALOGV("\tVolume_setParameter() Called pVolume->setMute");
            break;

        case VOLUME_PARAM_ENABLESTEREOPOSITION:
            positionEnabled = *(uint32_t *)pValue;
            status = VolumeEnableStereoPosition(pContext, positionEnabled);
            status = VolumeSetStereoPosition(pContext, pContext->pBundledContext->positionSaved);
            //ALOGV("\tVolume_setParameter() VOLUME_PARAM_ENABLESTEREOPOSITION called");
            break;

        case VOLUME_PARAM_STEREOPOSITION:
            position = *(int16_t *)pValue;
            //ALOGV("\tVolume_setParameter() VOLUME_PARAM_STEREOPOSITION value is %d", position);
            //ALOGV("\tVolume_setParameter() Calling pVolume->VolumeSetStereoPosition");
            status = VolumeSetStereoPosition(pContext, (int16_t)position);
            //ALOGV("\tVolume_setParameter() Called pVolume->VolumeSetStereoPosition");
            break;

        default:
            ALOGV("\tLVM_ERROR : Volume_setParameter() invalid param %d", param);
            break;
    }

    //ALOGV("\tVolume_setParameter end");
    return status;
} /* end Volume_setParameter */

/****************************************************************************************
 * Name : LVC_ToDB_s32Tos16()
 *  Input       : Signed 32-bit integer
 *  Output      : Signed 16-bit integer
 *                  MSB (16) = sign bit
 *                  (15->05) = integer part
 *                  (04->01) = decimal part
 *  Returns     : Db value with respect to full scale
 *  Description :
 *  Remarks     :
 ****************************************************************************************/

LVM_INT16 LVC_ToDB_s32Tos16(LVM_INT32 Lin_fix)
{
    LVM_INT16   db_fix;
    LVM_INT16   Shift;
    LVM_INT16   SmallRemainder;
    LVM_UINT32  Remainder = (LVM_UINT32)Lin_fix;

    /* Count leading bits, 1 cycle in assembly*/
    for (Shift = 0; Shift<32; Shift++)
    {
        if ((Remainder & 0x80000000U)!=0)
        {
            break;
        }
        Remainder = Remainder << 1;
    }

    /*
     * Based on the approximation equation (for Q11.4 format):
     *
     * dB = -96 * Shift + 16 * (8 * Remainder - 2 * Remainder^2)
     */
    db_fix    = (LVM_INT16)(-96 * Shift);               /* Six dB steps in Q11.4 format*/
    SmallRemainder = (LVM_INT16)((Remainder & 0x7fffffff) >> 24);
    db_fix = (LVM_INT16)(db_fix + SmallRemainder );
    SmallRemainder = (LVM_INT16)(SmallRemainder * SmallRemainder);
    db_fix = (LVM_INT16)(db_fix - (LVM_INT16)((LVM_UINT16)SmallRemainder >> 9));

    /* Correct for small offset */
    db_fix = (LVM_INT16)(db_fix - 5);

    return db_fix;
}

//----------------------------------------------------------------------------
// Effect_setEnabled()
//----------------------------------------------------------------------------
// Purpose:
// Enable or disable effect
//
// Inputs:
//  pContext      - pointer to effect context
//  enabled       - true if enabling the effect, false otherwise
//
// Outputs:
//
//----------------------------------------------------------------------------

int Effect_setEnabled(EffectContext *pContext, bool enabled)
{
    ALOGV("\tEffect_setEnabled() type %d, enabled %d", pContext->EffectType, enabled);

    if (enabled) {
        // Bass boost or Virtualizer can be temporarily disabled if playing over device speaker due
        // to their nature.
        bool tempDisabled = false;
        switch (pContext->EffectType) {
            case LVM_BASS_BOOST:
                if (pContext->pBundledContext->bBassEnabled == LVM_TRUE) {
                     ALOGV("\tEffect_setEnabled() LVM_BASS_BOOST is already enabled");
                     return -EINVAL;
                }
                if(pContext->pBundledContext->SamplesToExitCountBb <= 0){
                    pContext->pBundledContext->NumberEffectsEnabled++;
                }
                pContext->pBundledContext->SamplesToExitCountBb =
                     (LVM_INT32)(pContext->pBundledContext->SamplesPerSecond*0.1);
                pContext->pBundledContext->bBassEnabled = LVM_TRUE;
                tempDisabled = pContext->pBundledContext->bBassTempDisabled;
                break;
            case LVM_EQUALIZER:
                if (pContext->pBundledContext->bEqualizerEnabled == LVM_TRUE) {
                    ALOGV("\tEffect_setEnabled() LVM_EQUALIZER is already enabled");
                    return -EINVAL;
                }
                if(pContext->pBundledContext->SamplesToExitCountEq <= 0){
                    pContext->pBundledContext->NumberEffectsEnabled++;
                }
                pContext->pBundledContext->SamplesToExitCountEq =
                     (LVM_INT32)(pContext->pBundledContext->SamplesPerSecond*0.1);
                pContext->pBundledContext->bEqualizerEnabled = LVM_TRUE;
                break;
            case LVM_VIRTUALIZER:
                if (pContext->pBundledContext->bVirtualizerEnabled == LVM_TRUE) {
                    ALOGV("\tEffect_setEnabled() LVM_VIRTUALIZER is already enabled");
                    return -EINVAL;
                }
                if(pContext->pBundledContext->SamplesToExitCountVirt <= 0){
                    pContext->pBundledContext->NumberEffectsEnabled++;
                }
                pContext->pBundledContext->SamplesToExitCountVirt =
                     (LVM_INT32)(pContext->pBundledContext->SamplesPerSecond*0.1);
                pContext->pBundledContext->bVirtualizerEnabled = LVM_TRUE;
                tempDisabled = pContext->pBundledContext->bVirtualizerTempDisabled;
                break;
            case LVM_VOLUME:
                if (pContext->pBundledContext->bVolumeEnabled == LVM_TRUE) {
                    ALOGV("\tEffect_setEnabled() LVM_VOLUME is already enabled");
                    return -EINVAL;
                }
                pContext->pBundledContext->NumberEffectsEnabled++;
                pContext->pBundledContext->bVolumeEnabled = LVM_TRUE;
                break;
            default:
                ALOGV("\tEffect_setEnabled() invalid effect type");
                return -EINVAL;
        }
        if (!tempDisabled) {
            LvmEffect_enable(pContext);
        }
    } else {
        switch (pContext->EffectType) {
            case LVM_BASS_BOOST:
                if (pContext->pBundledContext->bBassEnabled == LVM_FALSE) {
                    ALOGV("\tEffect_setEnabled() LVM_BASS_BOOST is already disabled");
                    return -EINVAL;
                }
                pContext->pBundledContext->bBassEnabled = LVM_FALSE;
                break;
            case LVM_EQUALIZER:
                if (pContext->pBundledContext->bEqualizerEnabled == LVM_FALSE) {
                    ALOGV("\tEffect_setEnabled() LVM_EQUALIZER is already disabled");
                    return -EINVAL;
                }
                pContext->pBundledContext->bEqualizerEnabled = LVM_FALSE;
                break;
            case LVM_VIRTUALIZER:
                if (pContext->pBundledContext->bVirtualizerEnabled == LVM_FALSE) {
                    ALOGV("\tEffect_setEnabled() LVM_VIRTUALIZER is already disabled");
                    return -EINVAL;
                }
                pContext->pBundledContext->bVirtualizerEnabled = LVM_FALSE;
                break;
            case LVM_VOLUME:
                if (pContext->pBundledContext->bVolumeEnabled == LVM_FALSE) {
                    ALOGV("\tEffect_setEnabled() LVM_VOLUME is already disabled");
                    return -EINVAL;
                }
                pContext->pBundledContext->bVolumeEnabled = LVM_FALSE;
                break;
            default:
                ALOGV("\tEffect_setEnabled() invalid effect type");
                return -EINVAL;
        }
        LvmEffect_disable(pContext);
    }

    return 0;
}

//----------------------------------------------------------------------------
// LVC_Convert_VolToDb()
//----------------------------------------------------------------------------
// Purpose:
// Convery volume in Q24 to dB
//
// Inputs:
//  vol:   Q.24 volume dB
//
//-----------------------------------------------------------------------

int16_t LVC_Convert_VolToDb(uint32_t vol){
    int16_t  dB;

    dB = LVC_ToDB_s32Tos16(vol <<7);
    dB = (dB +8)>>4;
    dB = (dB <-96) ? -96 : dB ;

    return dB;
}

} // namespace
} // namespace

extern "C" {
/* Effect Control Interface Implementation: Process */
int Effect_process(effect_handle_t     self,
                              audio_buffer_t         *inBuffer __unused,
                              audio_buffer_t         *outBuffer __unused){
    EffectContext * pContext = (EffectContext *) self;
    int    status = 0;
    return status;
}   /* end Effect_process */

/* Effect Control Interface Implementation: Command */
int Effect_command(effect_handle_t  self,
                              uint32_t            cmdCode,
                              uint32_t            cmdSize,
                              void                *pCmdData,
                              uint32_t            *replySize,
                              void                *pReplyData){
    EffectContext * pContext = (EffectContext *) self;
    int retsize;

    //ALOGV("\t\nEffect_command start");

    if(pContext->EffectType == LVM_BASS_BOOST){
        //ALOGV("\tEffect_command setting command for LVM_BASS_BOOST");
    }
    if(pContext->EffectType == LVM_VIRTUALIZER){
        //ALOGV("\tEffect_command setting command for LVM_VIRTUALIZER");
    }
    if(pContext->EffectType == LVM_EQUALIZER){
        //ALOGV("\tEffect_command setting command for LVM_EQUALIZER");
    }
    if(pContext->EffectType == LVM_VOLUME){
        //ALOGV("\tEffect_command setting command for LVM_VOLUME");
    }

    if (pContext == NULL){
        ALOGV("\tLVM_ERROR : Effect_command ERROR pContext == NULL");
        return -EINVAL;
    }

    ALOGV("\tEffect_command INPUTS are: command %d cmdSize %d",cmdCode, cmdSize);

    // Incase we disable an effect, next time process is
    // called the number of effect called could be greater
    // pContext->pBundledContext->NumberEffectsCalled = 0;

    //ALOGV("\tEffect_command NumberEffectsCalled = %d, NumberEffectsEnabled = %d",
    //        pContext->pBundledContext->NumberEffectsCalled,
    //        pContext->pBundledContext->NumberEffectsEnabled);

    switch (cmdCode){
        case EFFECT_CMD_INIT:
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int)){
                ALOGV("\tLVM_ERROR, EFFECT_CMD_INIT: ERROR for effect type %d",
                        pContext->EffectType);
                return -EINVAL;
            }
            *(int *) pReplyData = 0;
            //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_INIT start");
            if(pContext->EffectType == LVM_BASS_BOOST){
                //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_INIT for LVM_BASS_BOOST");
                android::BassSetStrength(pContext, 0);
            }
            if(pContext->EffectType == LVM_VIRTUALIZER){
                //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_INIT for LVM_VIRTUALIZER");
                android::VirtualizerSetStrength(pContext, 0);
            }
            if(pContext->EffectType == LVM_EQUALIZER){
                //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_INIT for LVM_EQUALIZER");
                android::EqualizerSetPreset(pContext, 0);
            }
            if(pContext->EffectType == LVM_VOLUME){
                //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_INIT for LVM_VOLUME");
                *(int *) pReplyData = android::VolumeSetVolumeLevel(pContext, 0);
            }
            break;

        case EFFECT_CMD_SET_CONFIG:
            //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_SET_CONFIG start");
            if (pCmdData    == NULL || cmdSize     != sizeof(effect_config_t) ||
                    pReplyData  == NULL || replySize == NULL || *replySize  != sizeof(int)) {
                ALOGV("\tLVM_ERROR : Effect_command cmdCode Case: "
                        "EFFECT_CMD_SET_CONFIG: ERROR");
                return -EINVAL;
            }
            *(int *) pReplyData = android::Effect_setConfig(pContext, (effect_config_t *) pCmdData);
            //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_SET_CONFIG end");
            break;

        case EFFECT_CMD_GET_CONFIG:
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(effect_config_t)) {
                ALOGV("\tLVM_ERROR : Effect_command cmdCode Case: "
                        "EFFECT_CMD_GET_CONFIG: ERROR");
                return -EINVAL;
            }

            android::Effect_getConfig(pContext, (effect_config_t *)pReplyData);
            break;

        case EFFECT_CMD_RESET:
            //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_RESET start");
            android::Effect_setConfig(pContext, &pContext->config);
            //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_RESET end");
            break;

        case EFFECT_CMD_GET_PARAM:{
            //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_GET_PARAM start");

            effect_param_t *p = (effect_param_t *)pCmdData;

            if (pCmdData == NULL || cmdSize < sizeof(effect_param_t) ||
                    cmdSize < (sizeof(effect_param_t) + p->psize) ||
                    pReplyData == NULL || replySize == NULL ||
                    *replySize < (sizeof(effect_param_t) + p->psize)) {
                ALOGV("\tLVM_ERROR : EFFECT_CMD_GET_PARAM: ERROR");
                return -EINVAL;
            }

            memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + p->psize);

            p = (effect_param_t *)pReplyData;

            int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);

            if(pContext->EffectType == LVM_BASS_BOOST){
                p->status = android::BassBoost_getParameter(pContext,
                                                            p->data,
                                                            &p->vsize,
                                                            p->data + voffset);
                //ALOGV("\tBassBoost_command EFFECT_CMD_GET_PARAM "
                //        "*pCmdData %d, *replySize %d, *pReplyData %d ",
                //        *(int32_t *)((char *)pCmdData + sizeof(effect_param_t)),
                //        *replySize,
                //        *(int16_t *)((char *)pReplyData + sizeof(effect_param_t) + voffset));
            }

            if(pContext->EffectType == LVM_VIRTUALIZER){
                p->status = android::Virtualizer_getParameter(pContext,
                                                              (void *)p->data,
                                                              &p->vsize,
                                                              p->data + voffset);

                //ALOGV("\tVirtualizer_command EFFECT_CMD_GET_PARAM "
                //        "*pCmdData %d, *replySize %d, *pReplyData %d ",
                //        *(int32_t *)((char *)pCmdData + sizeof(effect_param_t)),
                //        *replySize,
               //         *(int16_t *)((char *)pReplyData + sizeof(effect_param_t) + voffset));
            }
            if(pContext->EffectType == LVM_EQUALIZER){
                //ALOGV("\tEqualizer_command cmdCode Case: "
                //        "EFFECT_CMD_GET_PARAM start");
                p->status = android::Equalizer_getParameter(pContext,
                                                            p->data,
                                                            &p->vsize,
                                                            p->data + voffset);

                //ALOGV("\tEqualizer_command EFFECT_CMD_GET_PARAM *pCmdData %d, *replySize %d, "
                //       "*pReplyData %08x %08x",
                //        *(int32_t *)((char *)pCmdData + sizeof(effect_param_t)), *replySize,
                //        *(int32_t *)((char *)pReplyData + sizeof(effect_param_t) + voffset),
                //        *(int32_t *)((char *)pReplyData + sizeof(effect_param_t) + voffset +
                //        sizeof(int32_t)));
            }
            if(pContext->EffectType == LVM_VOLUME){
                //ALOGV("\tVolume_command cmdCode Case: EFFECT_CMD_GET_PARAM start");
                p->status = android::Volume_getParameter(pContext,
                                                         (void *)p->data,
                                                         &p->vsize,
                                                         p->data + voffset);

                //ALOGV("\tVolume_command EFFECT_CMD_GET_PARAM "
                //        "*pCmdData %d, *replySize %d, *pReplyData %d ",
                //        *(int32_t *)((char *)pCmdData + sizeof(effect_param_t)),
                //        *replySize,
                //        *(int16_t *)((char *)pReplyData + sizeof(effect_param_t) + voffset));
            }
            *replySize = sizeof(effect_param_t) + voffset + p->vsize;

            //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_GET_PARAM end");
        } break;
        case EFFECT_CMD_SET_PARAM:{
            //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_SET_PARAM start");
            if(pContext->EffectType == LVM_BASS_BOOST){
                //ALOGV("\tBassBoost_command EFFECT_CMD_SET_PARAM param %d, *replySize %d, value %d",
                //       *(int32_t *)((char *)pCmdData + sizeof(effect_param_t)),
                //       *replySize,
                //       *(int16_t *)((char *)pCmdData + sizeof(effect_param_t) + sizeof(int32_t)));

                if (pCmdData   == NULL ||
                        cmdSize    != (sizeof(effect_param_t) + sizeof(int32_t) +sizeof(int16_t)) ||
                        pReplyData == NULL || replySize == NULL || *replySize != sizeof(int32_t)) {
                    ALOGV("\tLVM_ERROR : BassBoost_command cmdCode Case: "
                            "EFFECT_CMD_SET_PARAM: ERROR");
                    return -EINVAL;
                }
                effect_param_t *p = (effect_param_t *) pCmdData;

                if (p->psize != sizeof(int32_t)){
                    ALOGV("\tLVM_ERROR : BassBoost_command cmdCode Case: "
                            "EFFECT_CMD_SET_PARAM: ERROR, psize is not sizeof(int32_t)");
                    return -EINVAL;
                }

                //ALOGV("\tnBassBoost_command cmdSize is %d\n"
                //        "\tsizeof(effect_param_t) is  %d\n"
                //        "\tp->psize is %d\n"
                //        "\tp->vsize is %d"
                //        "\n",
                //        cmdSize, sizeof(effect_param_t), p->psize, p->vsize );

                *(int *)pReplyData = android::BassBoost_setParameter(pContext,
                                                                    (void *)p->data,
                                                                    p->data + p->psize);
            }
            if(pContext->EffectType == LVM_VIRTUALIZER){
              // Warning this log will fail to properly read an int32_t value, assumes int16_t
              //ALOGV("\tVirtualizer_command EFFECT_CMD_SET_PARAM param %d, *replySize %d, value %d",
              //        *(int32_t *)((char *)pCmdData + sizeof(effect_param_t)),
              //        *replySize,
              //        *(int16_t *)((char *)pCmdData + sizeof(effect_param_t) + sizeof(int32_t)));

                if (pCmdData   == NULL ||
                        // legal parameters are int16_t or int32_t
                        cmdSize    > (sizeof(effect_param_t) + sizeof(int32_t) +sizeof(int32_t)) ||
                        cmdSize    < (sizeof(effect_param_t) + sizeof(int32_t) +sizeof(int16_t)) ||
                        pReplyData == NULL || replySize == NULL || *replySize != sizeof(int32_t)) {
                    ALOGV("\tLVM_ERROR : Virtualizer_command cmdCode Case: "
                            "EFFECT_CMD_SET_PARAM: ERROR");
                    return -EINVAL;
                }
                effect_param_t *p = (effect_param_t *) pCmdData;

                if (p->psize != sizeof(int32_t)){
                    ALOGV("\tLVM_ERROR : Virtualizer_command cmdCode Case: "
                            "EFFECT_CMD_SET_PARAM: ERROR, psize is not sizeof(int32_t)");
                    return -EINVAL;
                }

                //ALOGV("\tnVirtualizer_command cmdSize is %d\n"
                //        "\tsizeof(effect_param_t) is  %d\n"
                //        "\tp->psize is %d\n"
                //        "\tp->vsize is %d"
                //        "\n",
                //        cmdSize, sizeof(effect_param_t), p->psize, p->vsize );

                *(int *)pReplyData = android::Virtualizer_setParameter(pContext,
                                                                      (void *)p->data,
                                                                       p->data + p->psize);
            }
            if(pContext->EffectType == LVM_EQUALIZER){
               //ALOGV("\tEqualizer_command cmdCode Case: "
               //        "EFFECT_CMD_SET_PARAM start");
               //ALOGV("\tEqualizer_command EFFECT_CMD_SET_PARAM param %d, *replySize %d, value %d ",
               //        *(int32_t *)((char *)pCmdData + sizeof(effect_param_t)),
               //        *replySize,
               //        *(int16_t *)((char *)pCmdData + sizeof(effect_param_t) + sizeof(int32_t)));

                if (pCmdData == NULL || cmdSize < (sizeof(effect_param_t) + sizeof(int32_t)) ||
                        pReplyData == NULL || replySize == NULL || *replySize != sizeof(int32_t)) {
                    ALOGV("\tLVM_ERROR : Equalizer_command cmdCode Case: "
                            "EFFECT_CMD_SET_PARAM: ERROR");
                    return -EINVAL;
                }
                effect_param_t *p = (effect_param_t *) pCmdData;

                *(int *)pReplyData = android::Equalizer_setParameter(pContext,
                                                                    (void *)p->data,
                                                                     p->data + p->psize);
            }
            if(pContext->EffectType == LVM_VOLUME){
                //ALOGV("\tVolume_command cmdCode Case: EFFECT_CMD_SET_PARAM start");
                //ALOGV("\tVolume_command EFFECT_CMD_SET_PARAM param %d, *replySize %d, value %d ",
                //        *(int32_t *)((char *)pCmdData + sizeof(effect_param_t)),
                //        *replySize,
                //        *(int16_t *)((char *)pCmdData + sizeof(effect_param_t) +sizeof(int32_t)));

                if (pCmdData   == NULL ||
                        cmdSize    < (sizeof(effect_param_t) + sizeof(int32_t)) ||
                        pReplyData == NULL || replySize == NULL ||
                        *replySize != sizeof(int32_t)) {
                    ALOGV("\tLVM_ERROR : Volume_command cmdCode Case: "
                            "EFFECT_CMD_SET_PARAM: ERROR");
                    return -EINVAL;
                }
                effect_param_t *p = (effect_param_t *) pCmdData;

                *(int *)pReplyData = android::Volume_setParameter(pContext,
                                                                 (void *)p->data,
                                                                 p->data + p->psize);
            }
            //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_SET_PARAM end");
        } break;

        case EFFECT_CMD_ENABLE:
            ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_ENABLE start");
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int)) {
                ALOGV("\tLVM_ERROR : Effect_command cmdCode Case: EFFECT_CMD_ENABLE: ERROR");
                return -EINVAL;
            }

            *(int *)pReplyData = android::Effect_setEnabled(pContext, LVM_TRUE);
            break;

        case EFFECT_CMD_DISABLE:
            //ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_DISABLE start");
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int)) {
                ALOGV("\tLVM_ERROR : Effect_command cmdCode Case: EFFECT_CMD_DISABLE: ERROR");
                return -EINVAL;
            }
            *(int *)pReplyData = android::Effect_setEnabled(pContext, LVM_FALSE);
            break;

        case EFFECT_CMD_SET_DEVICE:
        {
            ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_SET_DEVICE start");
            if (pCmdData   == NULL){
                ALOGV("\tLVM_ERROR : Effect_command cmdCode Case: EFFECT_CMD_SET_DEVICE: ERROR");
                return -EINVAL;
            }

            uint32_t device = *(uint32_t *)pCmdData;
            pContext->pBundledContext->nOutputDevice = (audio_devices_t) device;

            if (pContext->EffectType == LVM_BASS_BOOST) {
                if((device == AUDIO_DEVICE_OUT_SPEAKER) ||
                        (device == AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT) ||
                        (device == AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER)){
                    ALOGV("\tEFFECT_CMD_SET_DEVICE device is invalid for LVM_BASS_BOOST %d",
                          *(int32_t *)pCmdData);
                    ALOGV("\tEFFECT_CMD_SET_DEVICE temporary disable LVM_BAS_BOOST");

                    // If a device doesnt support bassboost the effect must be temporarily disabled
                    // the effect must still report its original state as this can only be changed
                    // by the ENABLE/DISABLE command

                    if (pContext->pBundledContext->bBassEnabled == LVM_TRUE) {
                        ALOGV("\tEFFECT_CMD_SET_DEVICE disable LVM_BASS_BOOST %d",
                             *(int32_t *)pCmdData);
                        android::LvmEffect_disable(pContext);
                    }
                    pContext->pBundledContext->bBassTempDisabled = LVM_TRUE;
                } else {
                    ALOGV("\tEFFECT_CMD_SET_DEVICE device is valid for LVM_BASS_BOOST %d",
                         *(int32_t *)pCmdData);

                    // If a device supports bassboost and the effect has been temporarily disabled
                    // previously then re-enable it

                    if (pContext->pBundledContext->bBassEnabled == LVM_TRUE) {
                        ALOGV("\tEFFECT_CMD_SET_DEVICE re-enable LVM_BASS_BOOST %d",
                             *(int32_t *)pCmdData);
                        android::LvmEffect_enable(pContext);
                    }
                    pContext->pBundledContext->bBassTempDisabled = LVM_FALSE;
                }
            }
            if (pContext->EffectType == LVM_VIRTUALIZER) {
                if (pContext->pBundledContext->nVirtualizerForcedDevice == AUDIO_DEVICE_NONE) {
                    // default case unless configuration is forced
                    if (android::VirtualizerIsDeviceSupported(device) != 0) {
                        ALOGV("\tEFFECT_CMD_SET_DEVICE device is invalid for LVM_VIRTUALIZER %d",
                                *(int32_t *)pCmdData);
                        ALOGV("\tEFFECT_CMD_SET_DEVICE temporary disable LVM_VIRTUALIZER");

                        //If a device doesnt support virtualizer the effect must be temporarily
                        // disabled the effect must still report its original state as this can
                        // only be changed by the ENABLE/DISABLE command

                        if (pContext->pBundledContext->bVirtualizerEnabled == LVM_TRUE) {
                            ALOGV("\tEFFECT_CMD_SET_DEVICE disable LVM_VIRTUALIZER %d",
                                    *(int32_t *)pCmdData);
                            android::LvmEffect_disable(pContext);
                        }
                        pContext->pBundledContext->bVirtualizerTempDisabled = LVM_TRUE;
                    } else {
                        ALOGV("\tEFFECT_CMD_SET_DEVICE device is valid for LVM_VIRTUALIZER %d",
                                *(int32_t *)pCmdData);

                        // If a device supports virtualizer and the effect has been temporarily
                        // disabled previously then re-enable it

                        if(pContext->pBundledContext->bVirtualizerEnabled == LVM_TRUE){
                            ALOGV("\tEFFECT_CMD_SET_DEVICE re-enable LVM_VIRTUALIZER %d",
                                    *(int32_t *)pCmdData);
                            android::LvmEffect_enable(pContext);
                        }
                        pContext->pBundledContext->bVirtualizerTempDisabled = LVM_FALSE;
                    }
                } // else virtualization mode is forced to a certain device, nothing to do
            }
            ALOGV("\tEffect_command cmdCode Case: EFFECT_CMD_SET_DEVICE end");
            break;
        }
        case EFFECT_CMD_SET_VOLUME:
        {
            uint32_t leftVolume, rightVolume;
            int16_t  leftdB, rightdB;
            int16_t  maxdB, pandB;
            int32_t  vol_ret[2] = {1<<24,1<<24}; // Apply no volume
            int      status = 0;
            LVM_ReturnStatus_en     LvmStatus=LVM_SUCCESS;  /* Function call status */

            // if pReplyData is NULL, VOL_CTRL is delegated to another effect
            if(pReplyData == LVM_NULL){
                break;
            }

            if (pCmdData == NULL || cmdSize != 2 * sizeof(uint32_t) || pReplyData == NULL ||
                    replySize == NULL || *replySize < 2*sizeof(int32_t)) {
                ALOGV("\tLVM_ERROR : Effect_command cmdCode Case: "
                        "EFFECT_CMD_SET_VOLUME: ERROR");
                return -EINVAL;
            }

            leftVolume  = ((*(uint32_t *)pCmdData));
            rightVolume = ((*((uint32_t *)pCmdData + 1)));

            if(leftVolume == 0x1000000){
                leftVolume -= 1;
            }
            if(rightVolume == 0x1000000){
                rightVolume -= 1;
            }

            // Convert volume to dB
            leftdB  = android::LVC_Convert_VolToDb(leftVolume);
            rightdB = android::LVC_Convert_VolToDb(rightVolume);

            pandB = rightdB - leftdB;

            // Calculate max volume in dB
            maxdB = leftdB;
            if(rightdB > maxdB){
                maxdB = rightdB;
            }
            //ALOGV("\tEFFECT_CMD_SET_VOLUME Session: %d, SessionID: %d VOLUME is %d dB (%d), "
            //      "effect is %d",
            //pContext->pBundledContext->SessionNo, pContext->pBundledContext->SessionId,
            //(int32_t)maxdB, maxVol<<7, pContext->EffectType);
            //ALOGV("\tEFFECT_CMD_SET_VOLUME: Left is %d, Right is %d", leftVolume, rightVolume);
            //ALOGV("\tEFFECT_CMD_SET_VOLUME: Left %ddB, Right %ddB, Position %ddB",
            //        leftdB, rightdB, pandB);

            //FIXME CHECK againmemcpy(pReplyData, vol_ret, sizeof(int32_t)*2);
            android::VolumeSetVolumeLevel(pContext, (int16_t)(maxdB*100));

            /* Use the current control settings from Context structure */

            /* Volume parameters */
            pContext->pBundledContext->ActiveParams.VC_Balance  = pandB;
            ALOGV("\t\tVolumeSetStereoPosition() (-96dB -> +96dB)-> %d\n", pContext->pBundledContext->ActiveParams.VC_Balance );

            /* Activate the initial settings */
            LvmStatus = android::Offload_SetEffect_ControlParameters(pContext);

            LVM_ERROR_CHECK(LvmStatus, "Offload_SetEffect_ControlParameters", "Effect_command")
            if(LvmStatus != LVM_SUCCESS) return -EINVAL;
            break;
         }
        case EFFECT_CMD_SET_AUDIO_MODE:
            break;
        case EFFECT_CMD_OFFLOAD:
            if (pCmdData == NULL || cmdSize != sizeof(effect_offload_param_t)
                    || pReplyData == NULL || *replySize != sizeof(uint32_t)) {
                return -EINVAL;
                ALOGE("%s: Command(%u) has Invalid Parameter", __func__, cmdCode);
            } else {
                effect_offload_param_t* offload_param = (effect_offload_param_t*)pCmdData;

                ALOGD("%s: Command(%u)= offload %d, output %d", __func__, cmdCode, offload_param->isOffload, offload_param->ioHandle);

                pContext->pBundledContext->OffloadEnabled = offload_param->isOffload;
                if (pContext->pBundledContext->OutHandle == offload_param->ioHandle) {
                    ALOGV("%s: This context has same output %d", __func__, offload_param->ioHandle);
                } else {
                    pContext->pBundledContext->OutHandle = offload_param->ioHandle;
                }
                *(int *)pReplyData = 0;
            }
            break;
        default:
            return -EINVAL;
    }

    ALOGV("\tEffect_command end...\n\n");
    return 0;
}    /* end Effect_command */

/* Effect Control Interface Implementation: get_descriptor */
int Effect_getDescriptor(effect_handle_t   self,
                                    effect_descriptor_t *pDescriptor)
{
    EffectContext * pContext = (EffectContext *) self;
    const effect_descriptor_t *desc;

    if (pContext == NULL || pDescriptor == NULL) {
        ALOGV("Effect_getDescriptor() invalid param");
        return -EINVAL;
    }

    ALOGD("%s: called", __func__);
    switch(pContext->EffectType) {
        case LVM_BASS_BOOST:
            desc = &android::gHwBassBoostDescriptor;
            ALOGD("%s: called LVM_BASS_BOOST", __func__);
            break;
        case LVM_VIRTUALIZER:
            desc = &android::gHwVirtualizerDescriptor;
            ALOGD("%s: called LVM_VIRTUALIZER", __func__);
            break;
        case LVM_EQUALIZER:
            desc = &android::gHwEqualizerDescriptor;
            ALOGD("%s: called LVM_EQUALIZER", __func__);
            break;
        case LVM_VOLUME:
            desc = &android::gHwVolumeDescriptor;
            ALOGD("%s: called LVM_VOLUME", __func__);
            break;
        default:
            return -EINVAL;
    }

    *pDescriptor = *desc;

    return 0;
}   /* end Effect_getDescriptor */

// effect_handle_t interface implementation for effect
const struct effect_interface_s gLvmHwEffectInterface = {
    Effect_process,
    Effect_command,
    Effect_getDescriptor,
    NULL,
};    /* end gLvmHwEffectInterface */

// This is the only symbol that needs to be exported
__attribute__ ((visibility ("default")))
audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "Effect Bundle hardware Library",
    .implementor = "Samsung SystemLSI",
    .create_effect = android::EffectHwCreate,
    .release_effect = android::EffectHwRelease,
    .get_descriptor = android::EffectHwGetDescriptor,
};

}
