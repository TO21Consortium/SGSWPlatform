/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CIS_5E8_H
#define FIMC_IS_CIS_5E8_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (24)

#define SENSOR_5E8_MAX_WIDTH		(2592)
#define SENSOR_5E8_MAX_HEIGHT		(1944)

/* TODO: Check below values are valid */
#define SENSOR_5E8_FINE_INTEGRATION_TIME_MIN                0x64
#define SENSOR_5E8_FINE_INTEGRATION_TIME_MAX                0x64
#define SENSOR_5E8_COARSE_INTEGRATION_TIME_MIN              0x3
#define SENSOR_5E8_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x4

#define USE_GROUP_PARAM_HOLD	(1)

#endif

