/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

__BEGIN_DECLS

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ID_A  (0)
#define ID_M  (1)
#define ID_O  (2)
#define ID_GY (3)
#define ID_GR (4)
#define ID_LA (5)
#define ID_RX (6)
#define ID_PRESS (7)
#define ID_TEMP  (8)
#define ID_ROT  (9)
#define ID_PROX (10)
#define ID_LIGHT (11)

/* ANDROID VERSION */
#define ANDROID_JELLY_BEAN		1					// Android 4.1

#if ANDROID_JELLY_BEAN == 1
	#define STLOGI(...)		ALOGI(__VA_ARGS__)
	#define STLOGE(...)		ALOGE(__VA_ARGS__)
	#define STLOGD(...)		ALOGD(__VA_ARGS__)
	#define STLOGD_IF(...)		ALOGD_IF(__VA_ARGS__)
	#define STLOGE_IF(...)		ALOGE_IF(__VA_ARGS__)
#else
	#define STLOGI(...)		LOGI(__VA_ARGS__)
	#define STLOGE(...)		LOGE(__VA_ARGS__)
	#define STLOGD(...)		LOGD(__VA_ARGS__)
	#define STLOGD_IF(...)		LOGD_IF(__VA_ARGS__)
	#define STLOGE_IF(...)		LOGE_IF(__VA_ARGS__)
#endif

// Event Type
#define EVENT_TYPE_GYRO_X           REL_RY
#define EVENT_TYPE_GYRO_Y           REL_RX
#define EVENT_TYPE_GYRO_Z           REL_RZ

// conversion of acceleration data to SI units (m/s^2)
#define ACCEL_MAX_RANGE		    (2*GRAVITY_EARTH)
#define ACCEL_POWER		    0.230f
#define ACCEL_MAX_ODR		    50.0f
#define ACCEL_FASTEST_DELTATIME	    (1.0f/ACCEL_MAX_ODR)*1000.0f*1000.0f
#define CONVERT_A                   (GRAVITY_EARTH/1000.0f)

// conversion of magnetic data to uT units (10mG = 1uT)
#define MAG_MAX_RANGE		    800.0f
#define MAG_POWER		    0.230f
#define MAG_MAX_ODR		    50.0f
#define MAG_FASTEST_DELTATIME	    (1.0f/MAG_MAX_ODR)*1000.0f*1000.0f
#define CONVERT_M                   (1.0f/10.0f) // from mG to uT

// conversion of gyro data to SI units
#define RAD2DEGR(rad)		    (rad  * (180.0f / (float)M_PI))
#define DEGR2RAD(degr)		    (degr * ((float)M_PI / 180.0f))
#define GYR_MAX_RANGE		    DEGR2RAD(2000.0f)
#define GYR_POWER		    6.1f
#define GYR_MAX_ODR		    50.0f
#define GYR_FASTEST_DELTATIME	    (1.0f/GYR_MAX_ODR)*1000.0f*1000.0f
#define CONVERT_GYRO                1.0f/1000.0f // from mdps to dps

// conversion of Pressure data to hPa, 1hPa = 1 mBar
#define PRESS_MAX_RANGE		    1260.0f
#define PRESS_POWER		    0.15f
#define PRESS_MAX_ODR		    25.0f
#define PRESS_FASTEST_DELTATIME	    (1.0f/PRESS_MAX_ODR)*1000.0f*1000.0f
#define CONVERT_PRESS                1.0f

// conversion of Pressure data to Celsius
#define TEMP_MAX_RANGE		    80.0f
#define TEMP_POWER		    0.15f
#define TEMP_MAX_ODR		    25.0f
#define TEMP_FASTEST_DELTATIME	    (1.0f/TEMP_MAX_ODR)*1000.0f*1000.0f
#define CONVERT_TEMP                1.0f

#define LOGGING_RAM_MAX (3*1024-512)
#define DATA_SIZE_RVECT                   9
#define ROT_MAX_RANGE		    1
#define ROT_CONVERT_ROT                 (1.0f/10000.0f)
#define ROT_VECTOR_POWER                       (4.0f)
#define ROT_VECTOR_MINDELAY                    (20 * 1000)
#define ROT_VECTOR_FIFOMAX                     (LOGGING_RAM_MAX/DATA_SIZE_RVECT)
#define ROTATION_VECTOR_MAXDELAY                    (400 * 1000)
/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
