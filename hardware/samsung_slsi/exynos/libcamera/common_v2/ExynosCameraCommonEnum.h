/*
**
** Copyright 2013, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef EXYNOS_CAMERA_COMMON_CONFIG_H
#define EXYNOS_CAMERA_COMMON_CONFIG_H

typedef enum
{
    SENSOR_NAME_NOTHING             = 0,
    SENSOR_NAME_S5K3H2              = 1, // 1 ~ 100, SLSI sensors
    SENSOR_NAME_S5K6A3              = 2,
    SENSOR_NAME_S5K3H5              = 3,
    SENSOR_NAME_S5K3H7              = 4,
    SENSOR_NAME_S5K3H7_SUNNY        = 5,
    SENSOR_NAME_S5K3H7_SUNNY_2M     = 6,
    SENSOR_NAME_S5K6B2              = 7,
    SENSOR_NAME_S5K3L2              = 8,
    SENSOR_NAME_S5K4E5              = 9,
    SENSOR_NAME_S5K2P2              = 10,
    SENSOR_NAME_S5K8B1              = 11,
    SENSOR_NAME_S5K1P2              = 12,
    SENSOR_NAME_S5K4H5              = 13,
    SENSOR_NAME_S5K3M2              = 14,
    SENSOR_NAME_S5K2P2_12M          = 15,
    SENSOR_NAME_S5K6D1              = 16,
    SENSOR_NAME_S5K5E3              = 17,
    SENSOR_NAME_S5K2T2              = 18,
    SENSOR_NAME_S5K2P3              = 19,
    SENSOR_NAME_S5K2P8              = 20,
    SENSOR_NAME_S5K4E6              = 21,
    SENSOR_NAME_S5K5E2              = 22,
    SENSOR_NAME_S5K3P3              = 23,
    SENSOR_NAME_S5K4H5YC            = 24,
    SENSOR_NAME_S5K3L8_MASTER       = 25,
    SENSOR_NAME_S5K3L8_SLAVE        = 26,
    SENSOR_NAME_S5K4H8              = 27,
    SENSOR_NAME_S5K2L1              = 29,
    SENSOR_NAME_S5K5E8              = 30,

    SENSOR_NAME_IMX135              = 101, // 101 ~ 200 Sony sensors
    SENSOR_NAME_IMX134              = 102,
    SENSOR_NAME_IMX175              = 103,
    SENSOR_NAME_IMX240              = 104,
    SENSOR_NAME_IMX228              = 106,
    SENSOR_NAME_IMX219              = 107,
    SENSOR_NAME_IMX260              = 109,

    SENSOR_NAME_SR261               = 201, // 201 ~ 300 Other vendor sensors
    SENSOR_NAME_OV5693              = 202,
    SENSOR_NAME_SR544               = 203,
    SENSOR_NAME_OV5670              = 204,
    SENSOR_NAME_OV8865              = 205,
    SENSOR_NAME_SR259               = 206,

    SENSOR_NAME_CUSTOM              = 301,
    SENSOR_NAME_SR200               = 302, // SoC Module
    SENSOR_NAME_SR352               = 303,
    SENSOR_NAME_SR130PC20           = 304,
    SENSOR_NAME_END,

}IS_SensorNameEnum;

#endif /* EXYNOS_CAMERA_COMMON_CONFIG_H */
