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

#ifndef EXYNOS_CAMERA_LUT_SR259_H
#define EXYNOS_CAMERA_LUT_SR259_H

/* -------------------------
    SIZE_RATIO_16_9 = 0,
    SIZE_RATIO_4_3,
    SIZE_RATIO_1_1,
    SIZE_RATIO_3_2,
    SIZE_RATIO_5_4,
    SIZE_RATIO_5_3,
    SIZE_RATIO_11_9,
    SIZE_RATIO_END
----------------------------
    RATIO_ID,
    SENSOR_W   = 1,
    SENSOR_H,
    BNS_W,
    BNS_H,
    BCROP_W,
    BCROP_H,
    BDS_W,
    BDS_H,
    TARGET_W,
    TARGET_H,
-----------------------------*/

static int SR259_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080)
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1072, 1072, SIZE_RATIO_1_1},
#endif
    { 1280,  960, SIZE_RATIO_4_3},
    { 1280,  720, SIZE_RATIO_16_9},
    { 1056,  704, SIZE_RATIO_3_2},  /* for 32 align of fimc limitation */
    {  960,  720, SIZE_RATIO_4_3},
    {  736,  736, SIZE_RATIO_1_1},  /* for 32 align of fimc limitation */
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  176,  144, SIZE_RATIO_11_9},
};

static int SR259_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if !(defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080))
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1072, 1072, SIZE_RATIO_1_1},
#endif
    { 1024,  768, SIZE_RATIO_4_3},
    {  880,  720, SIZE_RATIO_4_3},
    {  800,  600, SIZE_RATIO_4_3},
    {  800,  480, SIZE_RATIO_5_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  720, SIZE_RATIO_1_1},
    {  672,  448, SIZE_RATIO_3_2},  /* for 32 align of ficm limitation */
    {  528,  432, SIZE_RATIO_11_9},
    {  480,  320, SIZE_RATIO_3_2},
    {  480,  270, SIZE_RATIO_16_9},
};

static int SR259_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1616, 1212, SIZE_RATIO_4_3},
    { 1600,  900, SIZE_RATIO_16_9},
    { 1280,  960, SIZE_RATIO_4_3},
    { 1280,  720, SIZE_RATIO_16_9},
    { 1072, 1072, SIZE_RATIO_1_1},
    {  960,  720, SIZE_RATIO_4_3},
    {  640,  480, SIZE_RATIO_4_3},
    {  320,  240, SIZE_RATIO_4_3},
};

static int SR259_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1600, 1200, SIZE_RATIO_4_3},
    { 1024,  768, SIZE_RATIO_4_3},
    {  800,  600, SIZE_RATIO_4_3},
    {  800,  480, SIZE_RATIO_5_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  480, SIZE_RATIO_3_2},
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_11_9},
    {  480,  320, SIZE_RATIO_3_2},
    {  320,  180, SIZE_RATIO_16_9},
};

static int SR259_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1616, 1212, SIZE_RATIO_4_3},
    { 1600,  900, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1280,  960, SIZE_RATIO_4_3},
    { 1280,  720, SIZE_RATIO_16_9},
    { 1072, 1072, SIZE_RATIO_1_1},
    { 1056,  704, SIZE_RATIO_3_2},  /* for 32 align of fimc limitation */
    {  960,  720, SIZE_RATIO_4_3},
    {  736,  736, SIZE_RATIO_1_1},  /* for 32 align of fimc limitation */
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
};

static int SR259_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
    {  320,  240, SIZE_RATIO_4_3},
    {    0,    0, SIZE_RATIO_1_1},
};

static int SR259_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1280,  960, SIZE_RATIO_4_3},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  480,  320, SIZE_RATIO_3_2},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  176,  144, SIZE_RATIO_11_9},
};

static int SR259_HIDDEN_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
};

static int SR259_FPS_RANGE_LIST[][2] =
{
    {   5000,   5000},
    {   7000,   7000},
    {  15000,  15000},
    {  24000,  24000},
    {   4000,  30000},
    {   8000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
};

static int SR259_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};
#endif

