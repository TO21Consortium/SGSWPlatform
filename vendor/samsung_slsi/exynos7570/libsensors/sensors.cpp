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

#define LOG_TAG "Sensors"

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"
#include "Stm_sensors_hub_Sensor.h"

/*****************************************************************************/

#define DELAY_OUT_TIME 0x7FFFFFFF


#define SENSORS_ACCELERATION		(1<<ID_A)
#define SENSORS_MAGNETIC_FIELD		(1<<ID_M)
#define SENSORS_ORIENTATION		(1<<ID_O)
#define SENSORS_GYROSCOPE		(1<<ID_GY)
#define SENSORS_GRAVITY			(1<<ID_GR)
#define SENSORS_LINEAR_ACCELERATION	(1<<ID_LA)
#define SENSORS_ROTATION_MATRIX		(1<<ID_RX)
#define SENSOR_PRESSURE			(1<<ID_PRESS)
#define SENSOR_AMBIENT_TEMPERATURE	(1<<ID_TEMP)
#define SENSOR_ROTATION_VECTOR	(1<<ID_ROT)
#define SENSOR_PROXIMITY 	(1 << ID_PROX)
#define SENSOR_LIGHT		(1 << ID_LIGHT)


#define SENSORS_ACCELERATION_HANDLE     	0
#define SENSORS_MAGNETIC_FIELD_HANDLE   	1
#define SENSORS_ORIENTATION_HANDLE      	2
#define SENSORS_GYROSCOPE_HANDLE        	3
#define SENSORS_GRAVITY_HANDLE          	4
#define SENSORS_LINEAR_ACCELERATION_HANDLE	5
#define SENSORS_ROTATION_VECTOR_HANDLE		6
#define SENSORS_PRESSURE_HANDLE			7
#define SENSORS_AMBIENT_TEMPERATURE_HANDLE	8
#define SENSOR_ROTATION_VECTOR_HANDLE	9
#define SENSOR_PROXIMITY_HANDLE 10
#define SENSOR_LIGHT_HANDLE 11
/*****************************************************************************/

/* The SENSORS Module */
static const struct sensor_t sSensorList[] = {
        { "ICM20608 3-axis Accelerometer Sensor",
          "STMicroelectronics",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, ACCEL_MAX_RANGE, 1.0f, ACCEL_POWER, ACCEL_FASTEST_DELTATIME,
          0, 0, SENSOR_STRING_TYPE_ACCELEROMETER, "", 0, SENSOR_FLAG_CONTINUOUS_MODE, { } },
#if 1
        { "AKM09916 3-axis Magnetic field Sensor",
          "STMicroelectronics",
          1, SENSORS_MAGNETIC_FIELD_HANDLE,
          SENSOR_TYPE_MAGNETIC_FIELD, MAG_MAX_RANGE, 1.0f, MAG_POWER, MAG_FASTEST_DELTATIME,
          0, 0, SENSOR_STRING_TYPE_MAGNETIC_FIELD, "", 0, SENSOR_FLAG_CONTINUOUS_MODE, { } },
#endif
        { "ICM20608 3-axis Gyroscope Sensor",
          "STMicroelectronics",
          1, SENSORS_GYROSCOPE_HANDLE,
          SENSOR_TYPE_GYROSCOPE, GYR_MAX_RANGE, 1.0f, GYR_POWER, GYR_FASTEST_DELTATIME,
          0, 0, SENSOR_STRING_TYPE_GYROSCOPE, "", 0, SENSOR_FLAG_CONTINUOUS_MODE, { } },

        { "BMP280 Pressure Sensor",
          "STMicroelectronics",
          1, SENSORS_PRESSURE_HANDLE,
          SENSOR_TYPE_PRESSURE, PRESS_MAX_RANGE, 1.0f, PRESS_POWER, PRESS_FASTEST_DELTATIME,
          0, 0, SENSOR_STRING_TYPE_PRESSURE, "", 0, SENSOR_FLAG_CONTINUOUS_MODE, { } },
#if 0
        { "LPS25H Temperature Sensor",
          "STMicroelectronics",
          1, SENSORS_AMBIENT_TEMPERATURE_HANDLE,
          SENSOR_TYPE_AMBIENT_TEMPERATURE, TEMP_MAX_RANGE, 0.0f, TEMP_POWER, TEMP_FASTEST_DELTATIME, 0, 0, { } },
#endif
        { "Rotation Vector Sensor",
          "Invensense",
          1, SENSOR_ROTATION_VECTOR_HANDLE,
          SENSOR_TYPE_ROTATION_VECTOR, ROT_MAX_RANGE, ROT_CONVERT_ROT, ROT_VECTOR_POWER, ROT_VECTOR_MINDELAY,
          0, ROT_VECTOR_FIFOMAX, SENSOR_STRING_TYPE_ROTATION_VECTOR, "", ROTATION_VECTOR_MAXDELAY, SENSOR_FLAG_CONTINUOUS_MODE, { } },

	{ "LTR553 Proximity Sensor",
          "STMicroelectronics",
          1, SENSOR_PROXIMITY_HANDLE,
          SENSOR_TYPE_PROXIMITY, PRESS_MAX_RANGE, 1.0f, PRESS_POWER, PRESS_FASTEST_DELTATIME,
          0, 0, SENSOR_STRING_TYPE_PROXIMITY, "", 0, SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP, { } },

	{ "LTR553 LIGHT Sensor",
          "STMicroelectronics",
          1, SENSOR_LIGHT_HANDLE,
          SENSOR_TYPE_LIGHT, PRESS_MAX_RANGE, 1.0f, PRESS_POWER, PRESS_FASTEST_DELTATIME,
          0, 0, SENSOR_STRING_TYPE_LIGHT, "", 0, SENSOR_FLAG_ON_CHANGE_MODE, { } },
};

static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);

void get_ref(sensors_module_t *sm);

static int sensors__get_sensors_list(struct sensors_module_t *module,
                                     struct sensor_t const** list)
{
	STLOGI("sensors__get_sensors_list() ");

        *list = sSensorList;
        return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
        open: open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
        common: {
                tag: HARDWARE_MODULE_TAG,
                version_major: 1,
                version_minor: 0,
                id: SENSORS_HARDWARE_MODULE_ID,
                name: "STM Hub Sensor module",
                author: "STMicroelectronics",
                methods: &sensors_module_methods,
        },
        get_sensors_list: sensors__get_sensors_list,
};

void get_ref(sensors_module_t __attribute__((unused))*sm)
{
	sm = &HAL_MODULE_INFO_SYM;
	return;
}

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

private:
    enum {
	stm_sensors_hub	  = 0,
        numSensorDrivers,
        numFds,
    };

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_A:
            case ID_M:
            case ID_GY:
            case ID_O:
            case ID_GR:
            case ID_LA:
            case ID_RX:
	    case ID_PRESS:
	    case ID_TEMP:
	    case ID_ROT:
		case ID_PROX:
		case ID_LIGHT:
                return stm_sensors_hub;
         }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{

	STLOGI("KJUN() sensors_poll_context_t::sensors_poll_context_t() ");

    mSensors[stm_sensors_hub] = new stm_sensors_hub_Sensor();
    mPollFds[stm_sensors_hub].fd = mSensors[stm_sensors_hub]->getFd();
    mPollFds[stm_sensors_hub].events = POLLIN;
    mPollFds[stm_sensors_hub].revents = 0;

    int wakeFds[2];
    int result = pipe(wakeFds);
    STLOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;

}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    STLOGI("activate handle = %d, enable =%d", handle, enabled);
    int index = handleToDriver(handle);
    if (index < 0) return index;
    int err =  mSensors[index]->enable(handle, enabled);
    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        STLOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {

    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;
	//STLOGI("KJUN() sensors_poll_context_t::pollEvents() ");

    do {
        // see if we have some leftover from the last poll()

        for (int i=0 ; count && i<numSensorDrivers ; i++) {

			//STLOGI("KJUN()	sensors_poll_context_t::pollEvents(%d) ", i);
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN)) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
            if (n<0) {
                STLOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                STLOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                STLOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }
        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
	STLOGI("open_sensors() ");

        int status = -EINVAL;
        sensors_poll_context_t *dev = new sensors_poll_context_t();

        memset(&dev->device, 0, sizeof(sensors_poll_device_t));

        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_0;
        dev->device.common.module   = const_cast<hw_module_t*>(module);
        dev->device.common.close    = poll__close;
        dev->device.activate        = poll__activate;
        dev->device.setDelay        = poll__setDelay;
        dev->device.poll            = poll__poll;

        *device = &dev->device.common;
        status = 0;

        return status;
}

