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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <string.h>
#include<stdlib.h>

#include "Stm_sensors_hub_Sensor.h"

#define FETCH_FULL_EVENT_BEFORE_RETURN 1
#define HUB_BUFFER_LENGTH_BYTE		70

#define PHIS_FW  +1  /* used to adapt hw output to ENU*/

/*****************************************************************************/

//extern "C" void API_Initialization(char LocalEarthMagField);


stm_sensors_hub_Sensor::stm_sensors_hub_Sensor()
    : SensorBase(NULL, STM_SENSORS_HUB_DEV_NAME),
      mEnabled(0),
      mPendingMask(0),
      mInputReader(4),
      mHasPendingEvent(false)
{

	//STLOGI("stm_sensors_hub_Sensor::stm_sensors_hub_Sensor() ");

    memset(mPendingEvents, 0, sizeof(mPendingEvents));

    mPendingEvents[Acceleration].version = sizeof(sensors_event_t);
    mPendingEvents[Acceleration].sensor = ID_A;
    mPendingEvents[Acceleration].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents[Acceleration].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
    mPendingEvents[MagneticField].sensor = ID_M;
    mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
    mPendingEvents[MagneticField].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[AngularRotation].version = sizeof(sensors_event_t);
    mPendingEvents[AngularRotation].sensor = ID_GY;
    mPendingEvents[AngularRotation].type = SENSOR_TYPE_GYROSCOPE;
    mPendingEvents[AngularRotation].gyro.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Pressure].version = sizeof(sensors_event_t);
    mPendingEvents[Pressure].sensor = ID_PRESS;
    mPendingEvents[Pressure].type = SENSOR_TYPE_PRESSURE;

    mPendingEvents[AmbientTemperature].version = sizeof(sensors_event_t);
    mPendingEvents[AmbientTemperature].sensor = ID_TEMP;
    mPendingEvents[AmbientTemperature].type = SENSOR_TYPE_AMBIENT_TEMPERATURE;

    mPendingEvents[Rotation_Vector].version = sizeof(sensors_event_t);
    mPendingEvents[Rotation_Vector].sensor = ID_ROT;
    mPendingEvents[Rotation_Vector].type = SENSOR_TYPE_ROTATION_VECTOR;

	mPendingEvents[proximity_sensor].version = sizeof(sensors_event_t);
    mPendingEvents[proximity_sensor].sensor = ID_PROX;
    mPendingEvents[proximity_sensor].type = SENSOR_TYPE_PROXIMITY;

	mPendingEvents[light_sensor].version = sizeof(sensors_event_t);
    mPendingEvents[light_sensor].sensor = ID_LIGHT;
    mPendingEvents[light_sensor].type = SENSOR_TYPE_LIGHT;
	
    if (data_fd) {
        STLOGI( "%s, %s", input_sysfs_path, input_name);
    }

}

stm_sensors_hub_Sensor::~stm_sensors_hub_Sensor() {
    if (mEnabled) {
    }
}

int stm_sensors_hub_Sensor::setInitialState() {

    return 0;
}

int stm_sensors_hub_Sensor::enable(int32_t handle, int en) {
    int what = -1;
    static int enabled = 0;
    STLOGI( "stm_sensors_hub_Sensor::enable STM Sensor HUB Rev.1.0\n");

    switch (handle) {
        case ID_A: what = Acceleration; break;
        case ID_M: what = MagneticField; break;
	case ID_GY: what = AngularRotation; break;
        case ID_O: what = Orientation;   break;
        case ID_GR: what = Gravity; break;
        case ID_LA: what = Linear_Acceleration;   break;
        case ID_RX: what = Rotation_matrix;   break;
	case ID_PRESS: what = Pressure;	break;
	case ID_TEMP: what = AmbientTemperature; break;
	case ID_ROT: what = Rotation_Vector; break;
	case ID_PROX: what = proximity_sensor;break;
	case ID_LIGHT:what = light_sensor;break;
    }

    char filename[60] = {0};

    int flags = en ? 1 : 0;

    if(flags)
    	mEnabled |= (1<<what);
    else
	mEnabled &= ~(1<<what);

    STLOGI( "stm_sensors_hub_Sensor::enable mEnabled %d", mEnabled);

    if ((mEnabled && (!enabled)) || ((!mEnabled) && enabled)) {
        int fd;
       strcpy(&input_sysfs_path[input_sysfs_path_len], "enable_device");
	STLOGI( "stm_sensors_hub_Sensor::enable open fs %s", input_sysfs_path);
        fd = open(input_sysfs_path, O_RDWR);
        if (fd < 0) {
		STLOGI("open fs %s fail",input_sysfs_path);
		STLOGI("Message : %s\n", strerror(errno)); 
		//return -1;
	}
	char buf[2];
	int err;

	buf[1] = 0;
        if(flags){
		buf[0] = '1';
		enabled = 1;

	} else {
 		buf[0] = '0';
		enabled = 0;
	}
	
	err = write(fd, buf, sizeof(buf));
	if(err > 0)
		STLOGI("write stm_sensors_hub_Sensor::enable success!");
	else
		STLOGI("write stm_sensors_hub_Sensor::enable fail");
	close(fd);

	setInitialState();

    }
    return 0;
}

int stm_sensors_hub_Sensor::setDelay(int32_t handle, int64_t delay_ns)
{
    int fd;
    STLOGE( "stm_sensors_hub_Sensor::setDelay delay_ms = %ld", (long)delay_ns/1000000);
    strcpy(&input_sysfs_path[input_sysfs_path_len], "pollrate_ms");
	STLOGI( "stm_sensors_hub_Sensor::setDelay open fs %s", input_sysfs_path);
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%lld", delay_ns/1000000);
#if 0
        write(fd, buf, strlen(buf)+1);
#endif
	 close(fd);
        return 0;
    }
    return -1;
}

int stm_sensors_hub_Sensor::readEvents(sensors_event_t* data, int count)
{
	//STLOGI("stm_sensors_hub_Sensor::readEvents() ");

    static uint16_t  hub_dataHUB[HUB_BUFFER_LENGTH_BYTE/2];
    static float hub_dataENU[HUB_BUFFER_LENGTH_BYTE/2];
    struct timeval time;

    if (count < 1)
        return -EINVAL;


    ssize_t n = mInputReader.fill(data_fd);

	//STLOGI("stm_sensors_hub_Sensor::fill(%d) ", n);

    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

#if FETCH_FULL_EVENT_BEFORE_RETURN
again:
#endif
    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
	    /* Get Data from HUB */
	    hub_dataHUB[event->code] = event->value;
            //ALOGE("event->code=%d,hub_dataHUB[event->code]=0x%x\n", event->code,hub_dataHUB[event->code] );	
        } else if (type == EV_SYN) {
	    gettimeofday(&time, NULL);

	    if (mEnabled & (1<<Acceleration)) { /* m/s^2 */

		hub_dataENU[ACC_X_F] = (float )((short)hub_dataHUB[ACC_X_F]);	/* hub_dataHUB = [mg] */
		hub_dataENU[ACC_Y_F] = (float )((short)hub_dataHUB[ACC_Y_F]);
		hub_dataENU[ACC_Z_F] = (float )((short)hub_dataHUB[ACC_Z_F]);

		mPendingEvents[Acceleration].acceleration.x = (-1)*((PHIS_FW)*(hub_dataENU[ACC_X_F] * CONVERT_A))/16.3; /* hub_dataENU = [mg] */
		mPendingEvents[Acceleration].acceleration.y = (-1)*((PHIS_FW)*(hub_dataENU[ACC_Y_F] * CONVERT_A))/16.3;
		mPendingEvents[Acceleration].acceleration.z = ((PHIS_FW)*(hub_dataENU[ACC_Z_F] * CONVERT_A))/16.3;
		//ALOGE("mPendingEvents[Acceleration].acceleration.x =%f",mPendingEvents[Acceleration].acceleration.x );
		//ALOGE("mPendingEvents[Acceleration].acceleration.y =%f",mPendingEvents[Acceleration].acceleration.y );
		//ALOGE("mPendingEvents[Acceleration].acceleration.z =%f",mPendingEvents[Acceleration].acceleration.z);
		mPendingMask |= 1<<Acceleration;
	    }

	   if (mEnabled & (1<<MagneticField)) 
		{ /* uT (= 10mG) */
		hub_dataENU[MAG_X_F] = (float )((short)hub_dataHUB[MAG_X_F]);	/* hub_dataHUB = [mGauss] */
		hub_dataENU[MAG_Y_F] = (float )((short)hub_dataHUB[MAG_Y_F]);
		hub_dataENU[MAG_Z_F] = (float )((short)hub_dataHUB[MAG_Z_F]);

		mPendingEvents[MagneticField].magnetic.x = (PHIS_FW)*((hub_dataENU[MAG_X_F]) * CONVERT_M); /* hub_dataENU = [mGauss] */
		mPendingEvents[MagneticField].magnetic.y = (PHIS_FW)*((hub_dataENU[MAG_Y_F]) * CONVERT_M);
		mPendingEvents[MagneticField].magnetic.z = (PHIS_FW)*((hub_dataENU[MAG_Z_F]) * CONVERT_M);
		ALOGE("mPendingEvents[MagneticField].magnetic.x =%f",mPendingEvents[MagneticField].magnetic.x );
		ALOGE("mPendingEvents[MagneticField].magnetic.y =%f",mPendingEvents[MagneticField].magnetic.y );
		ALOGE("mPendingEvents[MagneticField].magnetic.z =%f",mPendingEvents[MagneticField].magnetic.z );
		mPendingMask |= 1<<MagneticField;
	    }

	    if (mEnabled & (1<<AngularRotation)) { /* rad/sec */

		hub_dataENU[GYR_X_F] = (float )((short)hub_dataHUB[GYR_X_F]);
		hub_dataENU[GYR_Y_F] = (float )((short)hub_dataHUB[GYR_Y_F]);
		hub_dataENU[GYR_Z_F] = (float )((short)hub_dataHUB[GYR_Z_F]);

		mPendingEvents[AngularRotation].gyro.x = DEGR2RAD((PHIS_FW)*(hub_dataENU[GYR_X_F] * CONVERT_GYRO));
		mPendingEvents[AngularRotation].gyro.y = DEGR2RAD((PHIS_FW)*(hub_dataENU[GYR_Y_F] * CONVERT_GYRO));
		mPendingEvents[AngularRotation].gyro.z = DEGR2RAD((PHIS_FW)*(hub_dataENU[GYR_Z_F] * CONVERT_GYRO));
		ALOGE("mPendingEvents[AngularRotation].gyro.x  =%f",mPendingEvents[AngularRotation].gyro.x );
		ALOGE("mPendingEvents[AngularRotation].gyro.y =%f", mPendingEvents[AngularRotation].gyro.y );
		ALOGE("mPendingEvents[AngularRotation].gyro.z =%f", mPendingEvents[AngularRotation].gyro.z );
		mPendingMask |= 1<<AngularRotation;
	    }

	   if (mEnabled & (1<<Pressure))
		{ /* 1hPa = 1mbar */

		hub_dataENU[PRESS_F] = (*((float *)((void *)&hub_dataHUB[PRESS_FROM_DRIVER])));
		//ALOGE("hub_dataHUB[18] = 0x%x,hub_dataHUB[19] = 0x%x,hub_dataENU[PRESS_F]=%f",hub_dataHUB[PRESS_FROM_DRIVER],hub_dataHUB[PRESS_FROM_DRIVER+1],hub_dataENU[PRESS_F]);
		mPendingEvents[Pressure].pressure = ((PHIS_FW)*(hub_dataENU[PRESS_F] * CONVERT_PRESS))/100;
		ALOGE("mPendingEvents[Pressure].pressure=%f",mPendingEvents[Pressure].pressure);
		mPendingMask |= 1<<Pressure;
	    }

	  	 if (mEnabled & (1<<AmbientTemperature))
		 { /* Celsius */

		hub_dataENU[TEMP_F] = (*((float *)((void *)&hub_dataHUB[TEMP_FROM_DRIVER])));

		mPendingEvents[AmbientTemperature].temperature = (PHIS_FW)*(hub_dataENU[TEMP_F] * CONVERT_TEMP);
		ALOGE("mPendingEvents[AmbientTemperature].temperature =%f",mPendingEvents[AmbientTemperature].temperature);
		mPendingMask |= 1<<AmbientTemperature;
	    }
	   if (mEnabled & (1<<Rotation_Vector)) { /* rad/sec */
		hub_dataENU[ROT_W_F] = (*((float *)((void *)&hub_dataHUB[ROT_W_FROM_DRIVER])));
		hub_dataENU[ROT_X_F] = (*((float *)((void *)&hub_dataHUB[ROT_X_FROM_DRIVER])));
		hub_dataENU[ROT_Y_F] = (*((float *)((void *)&hub_dataHUB[ROT_Y_FROM_DRIVER])));
		hub_dataENU[ROT_Z_F] = (*((float *)((void *)&hub_dataHUB[ROT_Z_FROM_DRIVER])));

		mPendingEvents[Rotation_Vector].data[0] = (PHIS_FW)*(hub_dataENU[ROT_W_F] );
		mPendingEvents[Rotation_Vector].data[1] = (PHIS_FW)*(hub_dataENU[ROT_X_F] );
		mPendingEvents[Rotation_Vector].data[2] = (PHIS_FW)*(hub_dataENU[ROT_Y_F] );
		mPendingEvents[Rotation_Vector].data[3] = (PHIS_FW)*(hub_dataENU[ROT_Z_F] );
		mPendingMask |= 1<<Rotation_Vector;
	    }
		if (mEnabled & (1<<proximity_sensor)) 
		{

		mPendingEvents[proximity_sensor].distance = (float)hub_dataHUB[PROXIMITY_F_FROM_DRIVER];
		ALOGE("PROXIMITY_F_FROM_DRIVER = %d,hub_dataHUB[PROXIMITY_F_FROM_DRIVER] =%d",PROXIMITY_F_FROM_DRIVER,hub_dataHUB[PROXIMITY_F_FROM_DRIVER]);
		ALOGE("mPendingEvents[proximity_sensor].distance =%f",mPendingEvents[proximity_sensor].distance);
		mPendingMask |= 1<<proximity_sensor;
	    }

	    if (mEnabled & (1<<light_sensor))
		{ /* 1lux */

		hub_dataENU[LIGHT_F] = (float)(hub_dataHUB[LIGHT_F_FROM_DRIVER+1] | hub_dataHUB[LIGHT_F_FROM_DRIVER]);
		ALOGE("hub_dataHUB[32] = 0x%x,hub_dataHUB[33] = 0x%x,hub_dataENU[LIGHT_F]=%f",hub_dataHUB[LIGHT_F_FROM_DRIVER],hub_dataHUB[LIGHT_F_FROM_DRIVER+1],hub_dataENU[LIGHT_F]);
		mPendingEvents[light_sensor].light = (hub_dataENU[LIGHT_F])/3;
		ALOGE("mPendingEvents[light_sensor].light=%f",mPendingEvents[light_sensor].light);
		mPendingMask |= 1<<light_sensor;
	    }
	    int64_t time = timevalToNano(event->time);
	    for (int j=0 ; count && mPendingMask && j<numSensors ; j++) {

                if (mPendingMask & (1<<j)) {
                    mPendingMask &= ~(1<<j);
                    mPendingEvents[j].timestamp = time;
                    if (mEnabled & (1<<j)) {
                        *data++ = mPendingEvents[j];
                        count--;
                        numEventReceived++;
                    }
                }
            }

        } else {
            STLOGE("GyroSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

#if FETCH_FULL_EVENT_BEFORE_RETURN
    /* if we didn't read a complete event, see if we can fill and
       try again instead of returning with nothing and redoing poll. */
    if (numEventReceived == 0 && mEnabled == 1) {
        n = mInputReader.fill(data_fd);
        if (n)
            goto again;
    }
#endif

    return numEventReceived;
}
