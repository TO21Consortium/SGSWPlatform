/*
 * Copyright 2015, Samsung Electronics Co. LTD
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

/*!
 * \file      ExynosCameraConfig.h
 * \brief     hearder file for ExynosCameraConfig
 * \author    Sangwoo Park(sw5771.park@samsung.com)
 * \date      2015/7/2
 *
 */

#ifndef EXYNOS_CAMERA_CONFIG_WRAPPER_H__
#define EXYNOS_CAMERA_CONFIG_WRAPPER_H__

#ifdef USE_CAMERA2_API_SUPPORT
#include "ExynosCamera3Config.h"
#else
#error This ExynosCameraConfig.h is 3.2 wrapper
#endif

#endif /* EXYNOS_CAMERA_CONFIG_WRAPPER_H__ */
