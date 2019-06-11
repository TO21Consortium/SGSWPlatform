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
 * @file        Exynos_OSAL_Log.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <utils/Log.h>
#include <cutils/properties.h>

#include "Exynos_OSAL_Log.h"
#include "Exynos_OSAL_ETC.h"
/* =======TAG=========
EXYNOS_RM
EXYNOS_LOG
EXYNOS_COMP_REGS
EXYNOS_OMX_CORE
EXYNOS_LOG_THREAD
EXYNOS_LOG_SEMA
Exynos_OSAL_SkypeHD
Exynos_OSAL_Android
Exynos_OSAL_EVENT
EXYNOS_BASE_COMP
EXYNOS_BASE_PORT
EXYNOS_VIDEO_DEC
EXYNOS_VIDEO_DECCONTROL
EXYNOS_H264_DEC
EXYNOS_HEVC_DEC
EXYNOS_MPEG2_DEC
EXYNOS_MPEG4_DEC
EXYNOS_WMV_DEC
EXYNOS_VP8_DEC
EXYNOS_VP9_DEC
EXYNOS_VIDEO_ENC
EXYNOS_VIDEO_ENCCONTROL
EXYNOS_HEVC_ENC
EXYNOS_H264_ENC
EXYNOS_MPEG4_ENC
EXYNOS_VP8_ENC
EXYNOS_VP9_ENC
======================*/
static char debugProp[PROPERTY_VALUE_MAX];
typedef enum _DEBUG_LEVEL
{
    LOG_DEFAULT  = EXYNOS_LOG_INFO,
    LOG_LEVEL1   = EXYNOS_LOG_ESSENTIAL,
    LOG_LEVEL2   = EXYNOS_LOG_TRACE,
    LOG_LEVELTAG = 3
} EXYNOS_DEBUG_LEVEL;

static unsigned int log_prop = LOG_DEFAULT;

void Exynos_OSAL_Get_Log_Property()
{
#ifdef EXYNOS_LOG
    if (property_get("debug.omx.level", debugProp, NULL) > 0) {
        if(!(Exynos_OSAL_Strncmp(debugProp, "0", 1))) {
            log_prop = LOG_DEFAULT;
        } else if (!(Exynos_OSAL_Strncmp(debugProp, "1", 1))) {
                log_prop = LOG_LEVEL1;
        } else if (!(Exynos_OSAL_Strncmp(debugProp, "2", 1))) {
                log_prop = LOG_LEVEL2;
        } else {
                log_prop = LOG_LEVELTAG;
        }
    }
#endif
}

void _Exynos_OSAL_Log(EXYNOS_LOG_LEVEL logLevel, const char *tag, const char *msg, ...)
{
    va_list argptr;
#ifdef EXYNOS_LOG
    if (log_prop == LOG_LEVELTAG) {
        if(!Exynos_OSAL_Strstr(debugProp, tag))
            return;
    } else if (logLevel < log_prop)
        return;
#endif
    va_start(argptr, msg);

    switch (logLevel) {
    case EXYNOS_LOG_TRACE:
        __android_log_vprint(ANDROID_LOG_VERBOSE, tag, msg, argptr);
        break;
    case EXYNOS_LOG_ESSENTIAL:
        __android_log_vprint(ANDROID_LOG_DEBUG, tag, msg, argptr);
        break;
    case EXYNOS_LOG_INFO:
        __android_log_vprint(ANDROID_LOG_INFO, tag, msg, argptr);
        break;
    case EXYNOS_LOG_WARNING:
        __android_log_vprint(ANDROID_LOG_WARN, tag, msg, argptr);
        break;
    case EXYNOS_LOG_ERROR:
        __android_log_vprint(ANDROID_LOG_ERROR, tag, msg, argptr);
        break;
    default:
        __android_log_vprint(ANDROID_LOG_VERBOSE, tag, msg, argptr);
    }

    va_end(argptr);
}
