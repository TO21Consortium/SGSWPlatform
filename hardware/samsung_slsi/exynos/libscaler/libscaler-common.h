/*
 * Copyright (C) 2014 The Android Open Source Project
 * Copyright@ Samsung Electronics Co. LTD
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

/*!
 * \file      libscaler-common.h
 * \brief     source file for Scaler HAL
 * \author    Cho KyongHo <pullip.cho@samsung.com>
 * \date      2014/05/08
 *
 * <b>Revision History: </b>
 * - 2014.05.08 : Cho KyongHo (pullip.cho@samsung.com) \n
 *   Create
 */
#ifndef _LIBSCALER_COMMON_H_
#define _LIBSCALER_COMMON_H_

#define LOG_TAG "libexynosscaler"
#include <cutils/log.h>
#include <cerrno>
#include <cstring>

//#define LOG_NDEBUG 0

#ifdef __GNUC__
#  define __UNUSED__ __attribute__((__unused__))
#else
#  define __UNUSED__
#endif

#define SC_LOGERR(fmt, args...) ((void)ALOG(LOG_ERROR, LOG_TAG, "%s: " fmt " [%s]", __func__, ##args, strerror(errno)))
#define SC_LOGE(fmt, args...)     ((void)ALOG(LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ##args))
#define SC_LOGI(fmt, args...)     ((void)ALOG(LOG_INFO, LOG_TAG, "%s: " fmt, __func__, ##args))
#define SC_LOGI_IF(cond, fmt, args...)  do { \
    if (cond)                                \
        SC_LOGI(fmt, ##args);                \
    } while (0)
#define SC_LOGE_IF(cond, fmt, args...)  do { \
    if (cond)                                \
        SC_LOGE(fmt, ##args);                   \
    } while (0)
#define SC_LOG_ASSERT(cont, fmt, args...) ((void)ALOG_ASSERT(cond, "%s: " fmt, __func__, ##args))

#ifdef SC_DEBUG
#define SC_LOGD(args...) ((void)ALOG(LOG_INFO, LOG_TAG, ##args))
#define SC_LOGD_IF(cond, fmt, args...)  do { \
    if (cond)                                \
        SC_LOGD(fmt, ##args);                \
    } while (0)
#else
#define SC_LOGD(args...) do { } while (0)
#define SC_LOGD_IF(cond, fmt, args...)  do { } while (0)
#endif

#define ARRSIZE(arr) (sizeof(arr)/sizeof(arr[0]))



namespace LibScaler {
template <typename T>
static inline T min (T a, T b) {
    return (a > b) ? b : a;
}

template <typename T>
static inline void swap(T &a, T &b) {
    T t = a;
    a = b;
    b = a;
}

static inline bool UnderOne16thScaling(unsigned int srcw, unsigned int srch,
        unsigned int dstw, unsigned int dsth, unsigned int rot) {
    if ((rot == 90) || (rot == 270))
        swap(srcw, srch);

    return ((srcw > (dstw * 16)) || (srch > (dsth * 16)));
}

};
// marker for output parameters
#define __out

#endif //_LIBSCALER_COMMON_H_
