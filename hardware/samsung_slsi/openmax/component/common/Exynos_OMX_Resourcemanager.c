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
 * @file       Exynos_OMX_Resourcemanager.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    2.0.0
 * @history
 *    2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Def.h"
#include "Exynos_OMX_Resourcemanager.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_Mutex.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_RM"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


#define MAX_RESOURCE_VIDEO_DEC RESOURCE_VIDEO_DEC
#define MAX_RESOURCE_VIDEO_ENC RESOURCE_VIDEO_ENC
#define MAX_RESOURCE_AUDIO_DEC RESOURCE_AUDIO_DEC
#define MAX_RESOURCE_VIDEO_SECURE 2
/* Add new resource block */

typedef enum _EXYNOS_OMX_RESOURCE
{
    VIDEO_DEC,
    VIDEO_ENC,
    AUDIO_DEC,
    VIDEO_SECURE,
    /* Add new resource block */
    RESOURCE_MAX
} EXYNOS_OMX_RESOURCE;

typedef struct _EXYNOS_OMX_RM_COMPONENT_LIST
{
    OMX_COMPONENTTYPE   *pOMXStandComp;
    OMX_U32              groupPriority;
    struct _EXYNOS_OMX_RM_COMPONENT_LIST *pNext;
} EXYNOS_OMX_RM_COMPONENT_LIST;

/* Max allowable scheduler component instance */
static EXYNOS_OMX_RM_COMPONENT_LIST *gpRMList[RESOURCE_MAX];
static EXYNOS_OMX_RM_COMPONENT_LIST *gpRMWaitList[RESOURCE_MAX];
static OMX_HANDLETYPE                ghVideoRMComponentListMutex = NULL;

EXYNOS_OMX_RM_COMPONENT_LIST *getRMList(
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent,
    EXYNOS_OMX_RM_COMPONENT_LIST    *pRMList[],
    int                             *pMaxResource)
{
    EXYNOS_OMX_RM_COMPONENT_LIST *ret = NULL;

    if (pExynosComponent == NULL)
        goto EXIT;

    switch (pExynosComponent->codecType) {
    case HW_VIDEO_DEC_CODEC:
        ret = pRMList[VIDEO_DEC];
        if (pMaxResource != NULL)
            *pMaxResource = MAX_RESOURCE_VIDEO_DEC;
        break;
    case HW_VIDEO_ENC_CODEC:
        ret = pRMList[VIDEO_ENC];
        if (pMaxResource != NULL)
            *pMaxResource = MAX_RESOURCE_VIDEO_ENC;
        break;
    case HW_VIDEO_DEC_SECURE_CODEC:
    case HW_VIDEO_ENC_SECURE_CODEC:
        ret = pRMList[VIDEO_SECURE];
        if (pMaxResource != NULL) {
            *pMaxResource = MAX_RESOURCE_VIDEO_SECURE;
#ifdef USE_SINGLE_DRM
            *pMaxResource = 1;
#endif
        }
        break;
    case HW_AUDIO_DEC_CODEC:
        ret = pRMList[AUDIO_DEC];
        if (pMaxResource != NULL)
            *pMaxResource = MAX_RESOURCE_AUDIO_DEC;
        break;
    /* Add new resource block */
    default:
        ret = NULL;
        if (pMaxResource != NULL)
            *pMaxResource = 0;
        break;
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE setRMList(
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent,
    EXYNOS_OMX_RM_COMPONENT_LIST    *pRMList[],
    EXYNOS_OMX_RM_COMPONENT_LIST    *pRMComponentList)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (pExynosComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (pExynosComponent->codecType) {
    case HW_VIDEO_DEC_CODEC:
        pRMList[VIDEO_DEC] = pRMComponentList;
        break;
    case HW_VIDEO_ENC_CODEC:
        pRMList[VIDEO_ENC] = pRMComponentList;
        break;
    case HW_VIDEO_DEC_SECURE_CODEC:
    case HW_VIDEO_ENC_SECURE_CODEC:
        pRMList[VIDEO_SECURE] = pRMComponentList;
        break;
    case HW_AUDIO_DEC_CODEC:
        pRMList[AUDIO_DEC] = pRMComponentList;
        break;
    /* Add new resource block */
    default:
        ret = OMX_ErrorUndefined;
        break;
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE addElementList(
    EXYNOS_OMX_RM_COMPONENT_LIST    **ppList,
    OMX_COMPONENTTYPE                *pOMXComponent)
{
    OMX_ERRORTYPE                 ret               = OMX_ErrorNone;
    EXYNOS_OMX_RM_COMPONENT_LIST *pTempComp         = NULL;
    EXYNOS_OMX_BASECOMPONENT     *pExynosComponent  = NULL;

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (*ppList != NULL) {
        pTempComp = *ppList;
        while (pTempComp->pNext != NULL) {
            pTempComp = pTempComp->pNext;
        }

        pTempComp->pNext = (EXYNOS_OMX_RM_COMPONENT_LIST *)Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_RM_COMPONENT_LIST));
        if (pTempComp->pNext == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

        ((EXYNOS_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->pNext = NULL;
        ((EXYNOS_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->pOMXStandComp = pOMXComponent;
        ((EXYNOS_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->groupPriority = pExynosComponent->compPriority.nGroupPriority;
        goto EXIT;
    } else {
        *ppList = (EXYNOS_OMX_RM_COMPONENT_LIST *)Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_RM_COMPONENT_LIST));
        if (*ppList == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

        pTempComp = *ppList;
        pTempComp->pNext = NULL;
        pTempComp->pOMXStandComp = pOMXComponent;
        pTempComp->groupPriority = pExynosComponent->compPriority.nGroupPriority;
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE removeElementList(
    EXYNOS_OMX_RM_COMPONENT_LIST   **ppList,
    OMX_COMPONENTTYPE               *pOMXComponent)
{
    OMX_ERRORTYPE                 ret           = OMX_ErrorNone;
    EXYNOS_OMX_RM_COMPONENT_LIST *pCurrComp     = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pPrevComp     = NULL;
    OMX_BOOL                      bDetectComp   = OMX_FALSE;

    if (*ppList == NULL) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    pCurrComp = *ppList;
    while (pCurrComp != NULL) {
        if (pCurrComp->pOMXStandComp == pOMXComponent) {
            if (*ppList == pCurrComp) {
                *ppList = pCurrComp->pNext;
                Exynos_OSAL_Free(pCurrComp);
                pCurrComp = NULL;
            } else {
                if (pPrevComp != NULL)
                    pPrevComp->pNext = pCurrComp->pNext;

                Exynos_OSAL_Free(pCurrComp);
                pCurrComp = NULL;
            }

            bDetectComp = OMX_TRUE;
            break;
        } else {
            pPrevComp = pCurrComp;
            pCurrComp = pCurrComp->pNext;
        }
    }

    if (bDetectComp == OMX_FALSE)
        ret = OMX_ErrorComponentNotFound;
    else
        ret = OMX_ErrorNone;

EXIT:
    return ret;
}

int searchLowPriority(
    EXYNOS_OMX_RM_COMPONENT_LIST  *pRMComponentList,
    OMX_U32                        inComp_priority,
    EXYNOS_OMX_RM_COMPONENT_LIST **outLowComp)
{
    int                           ret               = 0;
    EXYNOS_OMX_RM_COMPONENT_LIST *pTempComp         = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pCandidateComp    = NULL;

    if (pRMComponentList == NULL) {
        ret = -1;
        goto EXIT;
    }

    pTempComp   = pRMComponentList;
    *outLowComp = 0;

    while (pTempComp != NULL) {
        if (pTempComp->groupPriority > inComp_priority) {
            if (pCandidateComp != NULL) {
                if (pCandidateComp->groupPriority < pTempComp->groupPriority)
                    pCandidateComp = pTempComp;
            } else {
                pCandidateComp = pTempComp;
            }
        }

        pTempComp = pTempComp->pNext;
    }

    *outLowComp = pCandidateComp;
    if (pCandidateComp == NULL)
        ret = 0;
    else
        ret = 1;

EXIT:
    return ret;
}

OMX_ERRORTYPE removeComponent(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE             ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent  = NULL;

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateIdle) {
        (*(pExynosComponent->pCallbacks->EventHandler))(pOMXComponent,
                                                        pExynosComponent->callbackData,
                                                        OMX_EventError,
                                                        OMX_ErrorResourcesLost,
                                                        0,
                                                        NULL);
        ret = OMX_SendCommand(pOMXComponent, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if (ret != OMX_ErrorNone) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
    } else if ((pExynosComponent->currentState == OMX_StateExecuting) ||
               (pExynosComponent->currentState == OMX_StatePause)) {
        /* Todo */
    }

    ret = OMX_ErrorNone;

EXIT:
    return ret;
}


OMX_ERRORTYPE Exynos_OMX_ResourceManager_Init()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();

    ret = Exynos_OSAL_MutexCreate(&ghVideoRMComponentListMutex);

    if (ret == OMX_ErrorNone) {
        Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);
        Exynos_OSAL_Memset(gpRMList, 0, (sizeof(EXYNOS_OMX_RM_COMPONENT_LIST*) * RESOURCE_MAX));
        Exynos_OSAL_Memset(gpRMWaitList, 0, (sizeof(EXYNOS_OMX_RM_COMPONENT_LIST*) * RESOURCE_MAX));
        Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ResourceManager_Deinit()
{
    OMX_ERRORTYPE                    ret            = OMX_ErrorNone;
    EXYNOS_OMX_RM_COMPONENT_LIST    *pCurrComponent = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST    *pNextComponent = NULL;
    int i = 0;

    FunctionIn();

    Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);

    for (i = 0; i < RESOURCE_MAX; i++) {
        if (gpRMList[i]) {
            pCurrComponent = gpRMList[i];
            while (pCurrComponent != NULL) {
                pNextComponent = pCurrComponent->pNext;
                Exynos_OSAL_Free(pCurrComponent);
                pCurrComponent = pNextComponent;
            }
            gpRMList[i] = NULL;
        }

        if (gpRMWaitList[i]) {
            pCurrComponent = gpRMWaitList[i];
            while (pCurrComponent != NULL) {
                pNextComponent = pCurrComponent->pNext;
                Exynos_OSAL_Free(pCurrComponent);
                pCurrComponent = pNextComponent;
            }
            gpRMWaitList[i] = NULL;
        }
    }

    Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    Exynos_OSAL_MutexTerminate(ghVideoRMComponentListMutex);
    ghVideoRMComponentListMutex = NULL;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Get_Resource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                 ret                   = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT     *pExynosComponent      = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pRMComponentList      = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pComponentTemp        = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pComponentCandidate   = NULL;
    int numElem       = 0;
    int lowCompDetect = 0;
    int maxResource   = 0;

    FunctionIn();

    Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pRMComponentList = getRMList(pExynosComponent, gpRMList, &maxResource);

#ifndef USE_SECURE_WITH_NONSECURE
    if ((pExynosComponent->codecType == HW_VIDEO_DEC_CODEC) ||
        (pExynosComponent->codecType == HW_VIDEO_ENC_CODEC)) {
        if (gpRMList[VIDEO_SECURE] != NULL) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%s][%s] can't use secure with non-secure",
                                                __FUNCTION__, pExynosComponent->componentName);
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    } else if ((pExynosComponent->codecType == HW_VIDEO_DEC_SECURE_CODEC) ||
               (pExynosComponent->codecType == HW_VIDEO_ENC_SECURE_CODEC)) {
        if ((gpRMList[VIDEO_DEC] != NULL) ||
            (gpRMList[VIDEO_ENC] != NULL)) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%s][%s] can't use secure with non-secure",
                                                __FUNCTION__, pExynosComponent->componentName);
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }
#endif

    pComponentTemp = pRMComponentList;
    if (pComponentTemp != NULL) {
        while (pComponentTemp) {
            numElem++;
            pComponentTemp = pComponentTemp->pNext;
        }
    } else {
        numElem = 0;
    }

    if (numElem >= maxResource) {
        lowCompDetect = searchLowPriority(pRMComponentList,
                                          pExynosComponent->compPriority.nGroupPriority,
                                          &pComponentCandidate);
        if (lowCompDetect <= 0) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        } else {
            ret = removeComponent(pComponentCandidate->pOMXStandComp);
            if (ret != OMX_ErrorNone) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            } else {
                ret = removeElementList(&pRMComponentList, pComponentCandidate->pOMXStandComp);
                if (ret != OMX_ErrorNone)
                    goto EXIT;

                ret = addElementList(&pRMComponentList, pOMXComponent);
                if (ret != OMX_ErrorNone)
                    goto EXIT;
            }
        }
    } else {
        ret = addElementList(&pRMComponentList, pOMXComponent);
        if (ret != OMX_ErrorNone)
            goto EXIT;
    }

    ret = setRMList(pExynosComponent, gpRMList, pRMComponentList);
    if (ret != OMX_ErrorNone)
        goto EXIT;

    ret = OMX_ErrorNone;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%s][%s] has got a resource", __FUNCTION__, pExynosComponent->componentName);

EXIT:
    Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Release_Resource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                 ret                   = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT     *pExynosComponent      = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pRMComponentList      = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pRMComponentWaitList  = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pComponentTemp        = NULL;
    OMX_COMPONENTTYPE            *pOMXWaitComponent     = NULL;
    int numElem = 0;

    FunctionIn();

    Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pRMComponentList = getRMList(pExynosComponent, gpRMList, NULL);
    if (pRMComponentList == NULL) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    ret = removeElementList(&pRMComponentList, pOMXComponent);
    if (ret != OMX_ErrorNone)
        goto EXIT;

    ret = setRMList(pExynosComponent, gpRMList, pRMComponentList);
    if (ret != OMX_ErrorNone)
        goto EXIT;

    pRMComponentWaitList    = getRMList(pExynosComponent, gpRMWaitList, NULL);
    pComponentTemp          = pRMComponentWaitList;

    while (pComponentTemp) {
        numElem++;
        pComponentTemp = pComponentTemp->pNext;
    }

    if (numElem > 0) {
        pOMXWaitComponent = pRMComponentWaitList->pOMXStandComp;
        ret = removeElementList(&pRMComponentWaitList, pOMXWaitComponent);
        if (ret != OMX_ErrorNone)
            goto EXIT;

        ret = setRMList(pExynosComponent, gpRMWaitList, pRMComponentWaitList);
        if (ret != OMX_ErrorNone)
            goto EXIT;

        ret = OMX_SendCommand(pOMXWaitComponent, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if (ret != OMX_ErrorNone)
            goto EXIT;
    }

EXIT:
    Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_In_WaitForResource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                    ret                    = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent       = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST    *pRMComponentWaitList   = NULL;

    FunctionIn();

    Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pExynosComponent        = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pRMComponentWaitList    = getRMList(pExynosComponent, gpRMWaitList, NULL);

    ret = addElementList(&pRMComponentWaitList, pOMXComponent);
    if (ret != OMX_ErrorNone)
        goto EXIT;

    ret = setRMList(pExynosComponent, gpRMWaitList, pRMComponentWaitList);

EXIT:
    Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Out_WaitForResource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                    ret                    = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent       = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST    *pRMComponentWaitList   = NULL;

    FunctionIn();

    Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pExynosComponent        = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pRMComponentWaitList    = getRMList(pExynosComponent, gpRMWaitList, NULL);

    ret = removeElementList(&pRMComponentWaitList, pOMXComponent);
    if (ret != OMX_ErrorNone)
        goto EXIT;

    ret = setRMList(pExynosComponent, gpRMWaitList, pRMComponentWaitList);

EXIT:
    Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

