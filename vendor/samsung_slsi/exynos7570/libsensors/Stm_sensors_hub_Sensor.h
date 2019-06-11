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

#ifndef ANDROID_STM_SENSORS_HUB_SENSOR_H
#define ANDROID_STM_SENSORS_HUB_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

#define ACC_X_F		0
#define ACC_Y_F		1
#define ACC_Z_F		2
#define MAG_X_F		3
#define MAG_Y_F		4
#define MAG_Z_F		5
#define GYR_X_F		6
#define GYR_Y_F		7
#define GYR_Z_F		8
#define PRESS_F		9
#define	TEMP_F		10
#define ROT_W_F		11
#define ROT_X_F		12
#define ROT_Y_F		13
#define ROT_Z_F		14
#define PROX_F		15
#define LIGHT_F		16


#define BYTEs_PER_ELEMENT	2	// Driver reports uint_16

#define ACC_X_FROM_DRIVER	(BYTEs_PER_ELEMENT * ACC_X_F)
#define ACC_Y_FROM_DRIVER	(BYTEs_PER_ELEMENT * ACC_Y_F)
#define ACC_Z_FROM_DRIVER	(BYTEs_PER_ELEMENT * ACC_Z_F)
#define MAG_X_FROM_DRIVER	(BYTEs_PER_ELEMENT * MAG_X_F)
#define MAG_Y_FROM_DRIVER	(BYTEs_PER_ELEMENT * MAG_Y_F)
#define MAG_Z_FROM_DRIVER	(BYTEs_PER_ELEMENT * MAG_Z_F)
#define GYR_X_FROM_DRIVER	(BYTEs_PER_ELEMENT * GYR_X_F)
#define GYR_Y_FROM_DRIVER	(BYTEs_PER_ELEMENT * GYR_Y_F)
#define GYR_Z_FROM_DRIVER	(BYTEs_PER_ELEMENT * GYR_Z_F)
#define PRESS_FROM_DRIVER	(BYTEs_PER_ELEMENT * PRESS_F)
#define TEMP_FROM_DRIVER	(BYTEs_PER_ELEMENT * TEMP_F)
#define ROT_W_FROM_DRIVER	(BYTEs_PER_ELEMENT * ROT_W_F)
#define ROT_X_FROM_DRIVER	(BYTEs_PER_ELEMENT * ROT_X_F)
#define ROT_Y_FROM_DRIVER	(BYTEs_PER_ELEMENT * ROT_Y_F)
#define ROT_Z_FROM_DRIVER	(BYTEs_PER_ELEMENT * ROT_Z_F)
#define PROXIMITY_F_FROM_DRIVER (BYTEs_PER_ELEMENT * PROX_F)
#define LIGHT_F_FROM_DRIVER (BYTEs_PER_ELEMENT * LIGHT_F)

#define STM_SENSORS_HUB_DEV_NAME	"stm_sensor_hub"

/*****************************************************************************/

struct input_event;

class stm_sensors_hub_Sensor : public SensorBase {

public:
            stm_sensors_hub_Sensor();
    virtual ~stm_sensors_hub_Sensor();
    virtual int readEvents(sensors_event_t* data, int count);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);

    enum {
	Acceleration		= 0,
	MagneticField		= 1,
	AngularRotation		= 2,
	Orientation      	= 3,
        Gravity		 	= 4,
        Linear_Acceleration     = 5,
	Rotation_matrix		= 6,
	Pressure		= 7,
	AmbientTemperature	= 8,
	Rotation_Vector        =9,
	proximity_sensor = 10,
	light_sensor = 11,
        numSensors
    };

private:
    int mEnabled;
    uint32_t mPendingMask;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvents[numSensors];
    bool mHasPendingEvent;
    int setInitialState();
};

/*****************************************************************************/

#endif  // ANDROID_STM_SENSORS_HUB_SENSOR_H
