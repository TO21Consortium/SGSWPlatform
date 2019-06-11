/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file       Exynos_OMX_Baseport.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             HyeYeon Chung (hyeon.chung@samsung.com)
 * @version    2.0.0
 * @history
 *    2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Macros.h"
#include "Exynos_OSAL_Event.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Mutex.h"

#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Basecomponent.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_BASE_PORT"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

#ifdef PERFORMANCE_DEBUG
#include "Exynos_OSAL_ETC.h"
#endif


OMX_ERRORTYPE Exynos_OMX_InputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent, OMX_BUFFERHEADERTYPE* bufferHeader)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    OMX_U32                   i = 0;
    OMX_BOOL                  bBufferFind = OMX_FALSE;

    Exynos_OSAL_MutexLock(pExynosPort->hPortMutex);
    for (i = 0; i < pExynosPort->portDefinition.nBufferCountActual; i++) {
        if (bufferHeader == pExynosPort->extendBufferHeader[i].OMXBufferHeader) {
            if (pExynosPort->extendBufferHeader[i].bBufferInOMX == OMX_TRUE) {
                pExynosPort->extendBufferHeader[i].bBufferInOMX = OMX_FALSE;
                bBufferFind = OMX_TRUE;
                break;
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "Trying to return the input buffer without ownership!!");
            }
        }
    }

#ifdef PERFORMANCE_DEBUG
    Exynos_OSAL_CountDecrease(pExynosPort->hBufferCount, bufferHeader, INPUT_PORT_INDEX);
#endif

    Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);

    if ((bBufferFind == OMX_TRUE) &&
        (bufferHeader != NULL) &&
        (bufferHeader->pBuffer != NULL)) {
        pExynosComponent->pCallbacks->EmptyBufferDone(pOMXComponent, pExynosComponent->callbackData, bufferHeader);
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] bufferHeader: %p", pOMXComponent, __FUNCTION__, bufferHeader);
    }

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_OutputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent, OMX_BUFFERHEADERTYPE* bufferHeader)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                   i = 0;
    OMX_BOOL                  bBufferFind = OMX_FALSE;
    Exynos_OSAL_MutexLock(pExynosPort->hPortMutex);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        if (bufferHeader == pExynosPort->extendBufferHeader[i].OMXBufferHeader) {
            if (pExynosPort->extendBufferHeader[i].bBufferInOMX == OMX_TRUE) {
                bBufferFind = OMX_TRUE;
                pExynosPort->extendBufferHeader[i].bBufferInOMX = OMX_FALSE;
                break;
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "Trying to return the output buffer without ownership!!");
            }
        }
    }

#ifdef PERFORMANCE_DEBUG
    {
        EXYNOS_OMX_BASEPORT      *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
        EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
        BUFFER_TIME inputBufferInfo, outBufferInfo;

        Exynos_OSAL_Log(EXYNOS_LOG_INFO, "################################################################################");
        Exynos_OSAL_GetCountInfoUseTimestamp(pExynosInputPort->hBufferCount, bufferHeader->nTimeStamp, &inputBufferInfo);
        Exynos_OSAL_GetCountInfoUseTimestamp(pExynosOutputPort->hBufferCount, bufferHeader->nTimeStamp, &outBufferInfo);
        Exynos_OSAL_PrintCountInfo(inputBufferInfo, outBufferInfo);
        Exynos_OSAL_Log(EXYNOS_LOG_INFO, "################################################################################");
    }

    Exynos_OSAL_CountDecrease(pExynosPort->hBufferCount, bufferHeader, OUTPUT_PORT_INDEX);
#endif

    Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);

    if ((bBufferFind == OMX_TRUE) &&
        (bufferHeader != NULL) &&
        (bufferHeader->pBuffer != NULL)) {
        pExynosComponent->pCallbacks->FillBufferDone(pOMXComponent, pExynosComponent->callbackData, bufferHeader);
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] bufferHeader: %p", pOMXComponent, __FUNCTION__, bufferHeader);
    }

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferFlushProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex, OMX_BOOL bEvent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_S32                   portIndex = 0;
    EXYNOS_OMX_DATABUFFER    *flushPortBuffer[2] = {NULL, NULL};
    OMX_U32                   i = 0, cnt = 0;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        pExynosComponent->exynos_BufferFlush(pOMXComponent, portIndex, bEvent);
    }

EXIT:
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR,"%s : %d", __FUNCTION__, __LINE__);

        if ((pOMXComponent != NULL) &&
            (pExynosComponent != NULL)) {
            pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_EnablePort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_U32                i = 0, cnt = 0;

    FunctionIn();

    pExynosPort = &pExynosComponent->pExynosPort[portIndex];

    if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: Port($d) is waiting for loadedResource", __func__, portIndex);
        Exynos_OSAL_SemaphoreWait(pExynosPort->loadedResource);

        if (pExynosPort->exceptionFlag == INVALID_STATE) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: Port(%d) is in the INVALID_STATE", __func__, portIndex);
            pExynosPort->exceptionFlag = NEED_PORT_DISABLE;
            goto EXIT;
        }
        pExynosPort->portDefinition.bPopulated = OMX_TRUE;
    }
    pExynosPort->exceptionFlag = GENERAL_STATE;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;

    if ((pExynosComponent->bMultiThreadProcess == OMX_TRUE) &&
        (pExynosPort->portWayType == WAY2_PORT)) {
        for (i = 0; i < ALL_WAY_NUM; i++) {
            if (pExynosPort->semWaitPortEnable[i] != NULL)
                Exynos_OSAL_SemaphorePost(pExynosPort->semWaitPortEnable[i]);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_PortEnableProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        ret = Exynos_OMX_EnablePort(pOMXComponent, portIndex);
        if (ret == OMX_ErrorNone) {
            pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandPortEnable, portIndex, NULL);
        }
    }

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pExynosComponent != NULL)) {
            pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
        }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_DisablePort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_U32                i = 0, elemNum = 0;
    EXYNOS_OMX_MESSAGE       *message;

    FunctionIn();

    pExynosPort = &pExynosComponent->pExynosPort[portIndex];

    if (!CHECK_PORT_ENABLED(pExynosPort)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pExynosComponent->currentState != OMX_StateLoaded) {
        if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
            while (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) > 0) {
                message = (EXYNOS_OMX_MESSAGE*)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
                Exynos_OSAL_Free(message);
            }
        }
        pExynosPort->portDefinition.bPopulated = OMX_FALSE;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: Port(%d) is waiting for unloadedResource", __func__, portIndex);
        Exynos_OSAL_SemaphoreWait(pExynosPort->unloadedResource);
    }
    pExynosPort->portDefinition.bEnabled = OMX_FALSE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_PortDisableProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;
    EXYNOS_OMX_DATABUFFER    *flushPortBuffer[2] = {NULL, NULL};

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    if ((pExynosComponent->currentState == OMX_StateExecuting) ||
        (pExynosComponent->currentState == OMX_StatePause)) {
        /* port flush*/
        for(i = 0; i < cnt; i++) {
            if (nPortIndex == ALL_PORT_INDEX)
                portIndex = i;
            else
                portIndex = nPortIndex;

            Exynos_OMX_BufferFlushProcess(pOMXComponent, portIndex, OMX_FALSE);
        }
    }

    for(i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        ret = Exynos_OMX_DisablePort(pOMXComponent, portIndex);
        pExynosComponent->pExynosPort[portIndex].bIsPortDisabled = OMX_FALSE;
        if (ret == OMX_ErrorNone) {
            pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandPortDisable, portIndex, NULL);
        }
    }

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pExynosComponent != NULL)) {
        pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                        pExynosComponent->callbackData,
                        OMX_EventError,
                        ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_EmptyThisBuffer(
    OMX_IN OMX_HANDLETYPE        hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE                ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE           *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent   = NULL;
    EXYNOS_OMX_BASEPORT         *pExynosPort        = NULL;
    EXYNOS_OMX_MESSAGE          *message            = NULL;

    OMX_U32 i = 0;
    OMX_BOOL bFindBuffer = OMX_FALSE;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pBuffer->nInputPortIndex != INPUT_PORT_INDEX) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    ret = Exynos_OMX_Check_SizeVersion(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if ((pExynosComponent->currentState != OMX_StateIdle) &&
        (pExynosComponent->currentState != OMX_StateExecuting) &&
        (pExynosComponent->currentState != OMX_StatePause)) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    if ((!CHECK_PORT_ENABLED(pExynosPort)) ||
        (CHECK_PORT_BEING_FLUSHED(pExynosPort) &&
        (!CHECK_PORT_TUNNELED(pExynosPort) || !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort))) ||
        ((pExynosComponent->transientState == EXYNOS_OMX_TransStateExecutingToIdle) &&
        (CHECK_PORT_TUNNELED(pExynosPort) && !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)))) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    Exynos_OSAL_MutexLock(pExynosPort->hPortMutex);
    for (i = 0; i < pExynosPort->portDefinition.nBufferCountActual; i++) {
        if (pBuffer == pExynosPort->extendBufferHeader[i].OMXBufferHeader) {
            if (pExynosPort->extendBufferHeader[i].bBufferInOMX == OMX_FALSE) {
                pExynosPort->extendBufferHeader[i].bBufferInOMX = OMX_TRUE;
                bFindBuffer = OMX_TRUE;
                break;
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "input buffer(%p) was already entered!", pBuffer);
            }
        }
    }

    if (bFindBuffer == OMX_FALSE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "EmptyThisBuffer is failed : %p", pBuffer);
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);
        goto EXIT;
    }

#ifdef PERFORMANCE_DEBUG
    Exynos_OSAL_CountIncrease(pExynosPort->hBufferCount, pBuffer, INPUT_PORT_INDEX);
#endif

    message = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_MESSAGE));
    if (message == NULL) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);
        goto EXIT;
    }
    message->messageType = EXYNOS_OMX_CommandEmptyBuffer;
    message->messageParam = (OMX_U32) i;
    message->pCmdData = (OMX_PTR)pBuffer;

    ret = Exynos_OSAL_Queue(&pExynosPort->bufferQ, (void *)message);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);
        goto EXIT;
    }
    ret = Exynos_OSAL_SemaphorePost(pExynosPort->bufferSemID);

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] bufferHeader:%p, nAllocLen:%d, nFilledLen:%d, nOffset:%d", pOMXComponent, __FUNCTION__, pBuffer, pBuffer->nAllocLen, pBuffer->nFilledLen, pBuffer->nOffset);

    Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_FillThisBuffer(
    OMX_IN OMX_HANDLETYPE        hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE                ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE           *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent   = NULL;
    EXYNOS_OMX_BASEPORT         *pExynosPort        = NULL;
    EXYNOS_OMX_MESSAGE          *message            = NULL;

    OMX_U32 i = 0;
    OMX_BOOL bFindBuffer = OMX_FALSE;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pBuffer->nOutputPortIndex != OUTPUT_PORT_INDEX) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    ret = Exynos_OMX_Check_SizeVersion(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if ((pExynosComponent->currentState != OMX_StateIdle) &&
        (pExynosComponent->currentState != OMX_StateExecuting) &&
        (pExynosComponent->currentState != OMX_StatePause)) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    if ((!CHECK_PORT_ENABLED(pExynosPort)) ||
        (CHECK_PORT_BEING_FLUSHED(pExynosPort) &&
        (!CHECK_PORT_TUNNELED(pExynosPort) || !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort))) ||
        ((pExynosComponent->transientState == EXYNOS_OMX_TransStateExecutingToIdle) &&
        (CHECK_PORT_TUNNELED(pExynosPort) && !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)))) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    Exynos_OSAL_MutexLock(pExynosPort->hPortMutex);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        if (pBuffer == pExynosPort->extendBufferHeader[i].OMXBufferHeader) {
            if (pExynosPort->extendBufferHeader[i].bBufferInOMX == OMX_FALSE) {
                pExynosPort->extendBufferHeader[i].bBufferInOMX = OMX_TRUE;
                bFindBuffer = OMX_TRUE;
                break;
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "output buffer(%p) was already entered!", pBuffer);
            }
        }
    }

    if (bFindBuffer == OMX_FALSE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "FillThisBuffer is failed : %p", pBuffer);
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);
        goto EXIT;
    }

#ifdef PERFORMANCE_DEBUG
    Exynos_OSAL_CountIncrease(pExynosPort->hBufferCount, pBuffer, OUTPUT_PORT_INDEX);
#endif

    message = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_MESSAGE));
    if (message == NULL) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);
        goto EXIT;
    }
    message->messageType = EXYNOS_OMX_CommandFillBuffer;
    message->messageParam = (OMX_U32) i;
    message->pCmdData = (OMX_PTR)pBuffer;

    ret = Exynos_OSAL_Queue(&pExynosPort->bufferQ, (void *)message);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);
        goto EXIT;
    }

    ret = Exynos_OSAL_SemaphorePost(pExynosPort->bufferSemID);

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] bufferHeader:%p", pOMXComponent, __FUNCTION__, pBuffer);

    Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_FillThisBufferAgain(
    OMX_IN OMX_HANDLETYPE        hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE                ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE           *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent   = NULL;
    EXYNOS_OMX_BASEPORT         *pExynosPort        = NULL;
    EXYNOS_OMX_MESSAGE          *message            = NULL;

    OMX_U32 i = 0;
    OMX_BOOL bFindBuffer = OMX_FALSE;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pBuffer->nOutputPortIndex != OUTPUT_PORT_INDEX) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    ret = Exynos_OMX_Check_SizeVersion(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if ((pExynosComponent->currentState != OMX_StateIdle) &&
        (pExynosComponent->currentState != OMX_StateExecuting) &&
        (pExynosComponent->currentState != OMX_StatePause)) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    if ((!CHECK_PORT_ENABLED(pExynosPort)) ||
        (CHECK_PORT_BEING_FLUSHED(pExynosPort) &&
        (!CHECK_PORT_TUNNELED(pExynosPort) || !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort))) ||
        ((pExynosComponent->transientState == EXYNOS_OMX_TransStateExecutingToIdle) &&
        (CHECK_PORT_TUNNELED(pExynosPort) && !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)))) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    Exynos_OSAL_MutexLock(pExynosPort->hPortMutex);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        if (pBuffer == pExynosPort->extendBufferHeader[i].OMXBufferHeader) {
            if (pExynosPort->extendBufferHeader[i].bBufferInOMX == OMX_TRUE) {
                bFindBuffer = OMX_TRUE;
                break;
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "output buffer(%p) is firstly entered!", pBuffer);
            }
        }
    }

    if (bFindBuffer == OMX_FALSE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "FillThisBufferAgain is failed : %p", pBuffer);
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);
        goto EXIT;
    }

    message = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_MESSAGE));
    if (message == NULL) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);
        goto EXIT;
    }
    message->messageType = EXYNOS_OMX_CommandFillBuffer;
    message->messageParam = (OMX_U32) i;
    message->pCmdData = (OMX_PTR)pBuffer;

    ret = Exynos_OSAL_Queue(&pExynosPort->bufferQ, (void *)message);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);
        goto EXIT;
    }

    ret = Exynos_OSAL_SemaphorePost(pExynosPort->bufferSemID);
    Exynos_OSAL_MutexUnlock(pExynosPort->hPortMutex);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Port_Constructor(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE             ret               = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent     = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent  = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort       = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = NULL;
    int i = 0, j = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    INIT_SET_SIZE_VERSION(&pExynosComponent->portParam, OMX_PORT_PARAM_TYPE);
    pExynosComponent->portParam.nPorts = ALL_PORT_NUM;
    pExynosComponent->portParam.nStartPortNumber = INPUT_PORT_INDEX;

    pExynosPort = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_BASEPORT) * ALL_PORT_NUM);
    if (pExynosPort == NULL) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosPort, 0, sizeof(EXYNOS_OMX_BASEPORT) * ALL_PORT_NUM);
    pExynosComponent->pExynosPort = pExynosPort;

    /* Input Port */
    pExynosInputPort = &pExynosPort[INPUT_PORT_INDEX];

    Exynos_OSAL_QueueCreate(&pExynosInputPort->bufferQ, MAX_QUEUE_ELEMENTS);

    pExynosInputPort->extendBufferHeader = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_BUFFERHEADERTYPE) * MAX_BUFFER_NUM);
    if (pExynosInputPort->extendBufferHeader == NULL) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosInputPort->extendBufferHeader, 0, sizeof(EXYNOS_OMX_BUFFERHEADERTYPE) * MAX_BUFFER_NUM);

    pExynosInputPort->bufferStateAllocate = Exynos_OSAL_Malloc(sizeof(OMX_U32) * MAX_BUFFER_NUM);
    if (pExynosInputPort->bufferStateAllocate == NULL) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosInputPort->bufferStateAllocate, 0, sizeof(OMX_U32) * MAX_BUFFER_NUM);

    pExynosInputPort->bufferSemID = NULL;
    pExynosInputPort->assignedBufferNum = 0;
    pExynosInputPort->portState = OMX_StateMax;
    pExynosInputPort->bIsPortFlushed = OMX_FALSE;
    pExynosInputPort->bIsPortDisabled = OMX_FALSE;
    pExynosInputPort->tunneledComponent = NULL;
    pExynosInputPort->tunneledPort = 0;
    pExynosInputPort->tunnelBufferNum = 0;
    pExynosInputPort->bufferSupplier = OMX_BufferSupplyUnspecified;
    pExynosInputPort->tunnelFlags = 0;
    pExynosInputPort->supportFormat = NULL;
    pExynosInputPort->bNeedContigMem = OMX_FALSE;
    pExynosInputPort->bDynamicDPBMode = OMX_FALSE;
    pExynosInputPort->latestTimeStamp = DEFAULT_TIMESTAMP_VAL;

    ret = Exynos_OSAL_SemaphoreCreate(&pExynosInputPort->loadedResource);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = Exynos_OSAL_SemaphoreCreate(&pExynosInputPort->unloadedResource);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    for (i = 0; i < ALL_WAY_NUM; i++) {
        ret = Exynos_OSAL_SemaphoreCreate(&(pExynosInputPort->semWaitPortEnable[i]));
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
            goto EXIT;
        }
    }

    INIT_SET_SIZE_VERSION(&pExynosInputPort->portDefinition, OMX_PARAM_PORTDEFINITIONTYPE);
    pExynosInputPort->portDefinition.nPortIndex = INPUT_PORT_INDEX;
    pExynosInputPort->portDefinition.eDir = OMX_DirInput;
    pExynosInputPort->portDefinition.nBufferCountActual = 0;
    pExynosInputPort->portDefinition.nBufferCountMin = 0;
    pExynosInputPort->portDefinition.nBufferSize = 0;
    pExynosInputPort->portDefinition.bEnabled = OMX_FALSE;
    pExynosInputPort->portDefinition.bPopulated = OMX_FALSE;
    pExynosInputPort->portDefinition.eDomain = OMX_PortDomainMax;
    pExynosInputPort->portDefinition.bBuffersContiguous = OMX_FALSE;
    pExynosInputPort->portDefinition.nBufferAlignment = 0;
    pExynosInputPort->markType.hMarkTargetComponent = NULL;
    pExynosInputPort->markType.pMarkData = NULL;
    pExynosInputPort->exceptionFlag = GENERAL_STATE;

#ifdef PERFORMANCE_DEBUG
    Exynos_OSAL_CountCreate(&pExynosInputPort->hBufferCount);
#endif

    /* Output Port */
    pExynosOutputPort = &pExynosPort[OUTPUT_PORT_INDEX];

    Exynos_OSAL_QueueCreate(&pExynosOutputPort->bufferQ, MAX_QUEUE_ELEMENTS); /* For in case of "Output Buffer Share", MAX ELEMENTS(DPB + EDPB) */

    pExynosOutputPort->extendBufferHeader = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_BUFFERHEADERTYPE) * MAX_BUFFER_NUM);
    if (pExynosOutputPort->extendBufferHeader == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosOutputPort->extendBufferHeader, 0, sizeof(EXYNOS_OMX_BUFFERHEADERTYPE) * MAX_BUFFER_NUM);

    pExynosOutputPort->bufferStateAllocate = Exynos_OSAL_Malloc(sizeof(OMX_U32) * MAX_BUFFER_NUM);
    if (pExynosOutputPort->bufferStateAllocate == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosOutputPort->bufferStateAllocate, 0, sizeof(OMX_U32) * MAX_BUFFER_NUM);

    pExynosOutputPort->bufferSemID = NULL;
    pExynosOutputPort->assignedBufferNum = 0;
    pExynosOutputPort->portState = OMX_StateMax;
    pExynosOutputPort->bIsPortFlushed = OMX_FALSE;
    pExynosOutputPort->bIsPortDisabled = OMX_FALSE;
    pExynosOutputPort->tunneledComponent = NULL;
    pExynosOutputPort->tunneledPort = 0;
    pExynosOutputPort->tunnelBufferNum = 0;
    pExynosOutputPort->bufferSupplier = OMX_BufferSupplyUnspecified;
    pExynosOutputPort->tunnelFlags = 0;
    pExynosOutputPort->supportFormat = NULL;
    pExynosOutputPort->bNeedContigMem = OMX_FALSE;
    pExynosOutputPort->bDynamicDPBMode = OMX_FALSE;
    pExynosOutputPort->latestTimeStamp = DEFAULT_TIMESTAMP_VAL;

    ret = Exynos_OSAL_SemaphoreCreate(&pExynosOutputPort->loadedResource);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = Exynos_OSAL_SemaphoreCreate(&pExynosOutputPort->unloadedResource);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    for (i = 0; i < ALL_WAY_NUM; i++) {
        ret = Exynos_OSAL_SemaphoreCreate(&(pExynosOutputPort->semWaitPortEnable[i]));
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
            goto EXIT;
        }
    }

    INIT_SET_SIZE_VERSION(&pExynosOutputPort->portDefinition, OMX_PARAM_PORTDEFINITIONTYPE);
    pExynosOutputPort->portDefinition.nPortIndex = OUTPUT_PORT_INDEX;
    pExynosOutputPort->portDefinition.eDir = OMX_DirOutput;
    pExynosOutputPort->portDefinition.nBufferCountActual = 0;
    pExynosOutputPort->portDefinition.nBufferCountMin = 0;
    pExynosOutputPort->portDefinition.nBufferSize = 0;
    pExynosOutputPort->portDefinition.bEnabled = OMX_FALSE;
    pExynosOutputPort->portDefinition.bPopulated = OMX_FALSE;
    pExynosOutputPort->portDefinition.eDomain = OMX_PortDomainMax;
    pExynosOutputPort->portDefinition.bBuffersContiguous = OMX_FALSE;
    pExynosOutputPort->portDefinition.nBufferAlignment = 0;
    pExynosOutputPort->markType.hMarkTargetComponent = NULL;
    pExynosOutputPort->markType.pMarkData = NULL;
    pExynosOutputPort->exceptionFlag = GENERAL_STATE;

#ifdef PERFORMANCE_DEBUG
    Exynos_OSAL_CountCreate(&pExynosOutputPort->hBufferCount);
#endif

    pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
    pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
    pExynosComponent->checkTimeStamp.startTimeStamp = 0;
    pExynosComponent->checkTimeStamp.nStartFlags = 0x0;

    pOMXComponent->EmptyThisBuffer = &Exynos_OMX_EmptyThisBuffer;
    pOMXComponent->FillThisBuffer  = &Exynos_OMX_FillThisBuffer;

    ret = OMX_ErrorNone;

EXIT:
    if ((ret != OMX_ErrorNone) &&
        (pExynosComponent != NULL) &&
        (pExynosComponent->pExynosPort != NULL)) {
        for (i = 0; i < ALL_PORT_NUM; i++) {
            pExynosPort = &pExynosComponent->pExynosPort[i];

#ifdef PERFORMANCE_DEBUG
            Exynos_OSAL_CountTerminate(&pExynosPort->hBufferCount);
#endif
            for (j = 0; j < ALL_WAY_NUM; j++) {
                Exynos_OSAL_SemaphoreTerminate(pExynosPort->semWaitPortEnable[j]);
                pExynosPort->semWaitPortEnable[j] = NULL;
            }

            Exynos_OSAL_SemaphoreTerminate(pExynosPort->loadedResource);
            pExynosPort->loadedResource = NULL;
            Exynos_OSAL_SemaphoreTerminate(pExynosPort->unloadedResource);
            pExynosPort->unloadedResource = NULL;
            Exynos_OSAL_Free(pExynosPort->bufferStateAllocate);
            pExynosPort->bufferStateAllocate = NULL;
            Exynos_OSAL_Free(pExynosPort->extendBufferHeader);
            pExynosPort->extendBufferHeader = NULL;

            Exynos_OSAL_QueueTerminate(&pExynosPort->bufferQ);
        }

        Exynos_OSAL_Free(pExynosComponent->pExynosPort);
        pExynosComponent->pExynosPort = NULL;
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Port_Destructor(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE             ret               = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent     = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent  = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort       = NULL;

    OMX_S32 countValue = 0;
    int i = 0, j = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pExynosComponent->transientState == EXYNOS_OMX_TransStateLoadedToIdle) {
        pExynosComponent->abendState = OMX_TRUE;
        for (i = 0; i < ALL_PORT_NUM; i++) {
            pExynosPort = &pExynosComponent->pExynosPort[i];
            Exynos_OSAL_SemaphorePost(pExynosPort->loadedResource);
        }
        Exynos_OSAL_SignalWait(pExynosComponent->abendStateEvent, DEF_MAX_WAIT_TIME);
        Exynos_OSAL_SignalReset(pExynosComponent->abendStateEvent);
    }

    for (i = 0; i < ALL_PORT_NUM; i++) {
        pExynosPort = &pExynosComponent->pExynosPort[i];

#ifdef PERFORMANCE_DEBUG
        Exynos_OSAL_CountTerminate(&pExynosPort->hBufferCount);
#endif

        for (j = 0; j < ALL_WAY_NUM; j++) {
            Exynos_OSAL_SemaphoreTerminate(pExynosPort->semWaitPortEnable[j]);
            pExynosPort->semWaitPortEnable[j] = NULL;
        }

        Exynos_OSAL_SemaphoreTerminate(pExynosPort->loadedResource);
        pExynosPort->loadedResource = NULL;
        Exynos_OSAL_SemaphoreTerminate(pExynosPort->unloadedResource);
        pExynosPort->unloadedResource = NULL;
        Exynos_OSAL_Free(pExynosPort->bufferStateAllocate);
        pExynosPort->bufferStateAllocate = NULL;
        Exynos_OSAL_Free(pExynosPort->extendBufferHeader);
        pExynosPort->extendBufferHeader = NULL;

        Exynos_OSAL_QueueTerminate(&pExynosPort->bufferQ);
    }
    Exynos_OSAL_Free(pExynosComponent->pExynosPort);
    pExynosComponent->pExynosPort = NULL;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_ResetDataBuffer(EXYNOS_OMX_DATABUFFER *pDataBuffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (pDataBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pDataBuffer->dataValid     = OMX_FALSE;
    pDataBuffer->dataLen       = 0;
    pDataBuffer->remainDataLen = 0;
    pDataBuffer->usedDataLen   = 0;
    pDataBuffer->bufferHeader  = NULL;
    pDataBuffer->nFlags        = 0;
    pDataBuffer->timeStamp     = 0;
    pDataBuffer->pPrivate      = NULL;

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_ResetCodecData(EXYNOS_OMX_DATA *pData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (pData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pData->dataLen       = 0;
    pData->usedDataLen   = 0;
    pData->remainDataLen = 0;
    pData->nFlags        = 0;
    pData->timeStamp     = 0;
    pData->pPrivate      = NULL;
    pData->bufferHeader  = NULL;

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_Shared_BufferToData(EXYNOS_OMX_DATABUFFER *pUseBuffer, EXYNOS_OMX_DATA *pData, EXYNOS_OMX_PLANE nPlane)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (nPlane == ONE_PLANE) {
        /* Case of Shared Buffer, Only support singlePlaneBuffer */
        pData->multiPlaneBuffer.dataBuffer[0] = pUseBuffer->bufferHeader->pBuffer;
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not support plane");
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
    }

    pData->allocSize     = pUseBuffer->allocSize;
    pData->dataLen       = pUseBuffer->dataLen;
    pData->usedDataLen   = pUseBuffer->usedDataLen;
    pData->remainDataLen = pUseBuffer->remainDataLen;
    pData->timeStamp     = pUseBuffer->timeStamp;
    pData->nFlags        = pUseBuffer->nFlags;
    pData->pPrivate      = pUseBuffer->pPrivate;
    pData->bufferHeader  = pUseBuffer->bufferHeader;

EXIT:
    return ret;
}

int Exynos_GetPlaneFromPort(EXYNOS_OMX_BASEPORT *pPort)
{
    int ret = 0;

    if (pPort == NULL)
        goto EXIT;

    ret = pPort->processData.multiPlaneBuffer.validPlaneNum;

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_SetPlaneToPort(EXYNOS_OMX_BASEPORT *pPort, int nPlaneNum)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (pPort == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pPort->processData.multiPlaneBuffer.validPlaneNum = nPlaneNum;

EXIT:
    return ret;
}

