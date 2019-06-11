/*
 * Copyright 2008, The Android Open Source Project
 * Copyright 2013, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      Exif.cpp
 * \brief     source file for Android Camera Ext HAL
 * \author    teahyung kim (tkon.kim@samsung.com)
 * \date      2013/04/30
 *
 */

#define LOG_TAG "Exif"

#include <cutils/compiler.h>
#include <utils/Log.h>
#include <fcntl.h>

#include <camera/Camera.h>

#include "Exif.h"
#include "model/include/SecCameraHardware-model.h"

namespace android {
#define CLEAR(x)    memset(&(x), 0, sizeof(x))

const char Exif::DEFAULT_MAKER[] = "samsung";
const char Exif::DEFAULT_MODEL[] = "SAMSUNG";
const char Exif::DEFAULT_SOFTWARE[] = "SAMSUNG";
const char Exif::DEFAULT_EXIF_VERSION[] = "0220";
const char Exif::DEFAULT_USERCOMMENTS[] = "User comments";

const int Exif::DEFAULT_YCBCR_POSITIONING = 1;

const int Exif::DEFAULT_EXPOSURE_PROGRAM = 2;

const int Exif::DEFAULT_FLASH = 0;
const int Exif::DEFAULT_COLOR_SPACE = 1;
const int Exif::DEFAULT_EXPOSURE_MODE = EXIF_EXPOSURE_AUTO;
const int Exif::DEFAULT_APEX_DEN = 100;
const int Exif::DEFAULT_SENSING_METHOD = 2;

const int Exif::DEFAULT_COMPRESSION = 6;
const int Exif::DEFAULT_RESOLUTION_NUM = 72;
const int Exif::DEFAULT_RESOLUTION_DEN = 1;
const int Exif::DEFAULT_RESOLUTION_UNIT = 2;

const int Exif::DEFAULT_CONTINUOUS_SHOT_INFO = 0;
Exif::Exif(int cameraId, int CameraType)
{
    mCameraId = cameraId;
    mCameraType = CameraType;

    mNum0thIfdTiff = 10;
    mNum0thIfdExif = 26;
    mNum0thIfdGps = 10;
    mNum1thIfdTiff = 9;
}

Exif::~Exif()
{
}

uint32_t Exif::make(void *exifOutBuf,
            exif_attribute_t *exifInfo,
            unsigned int exifOutBufSize,
            unsigned char *thumbBuf,
            unsigned int thumbSize)
{
    ALOGV("makeExif E");

    unsigned char *pCur, *pApp1Start, *pIfdStart, *pGpsIfdPtr, *pNextIfdOffset;
    unsigned int tmp, LongerTagOffest = 0;
    pApp1Start = pCur = (unsigned char *)exifOutBuf;

    /* Exif Identifier Code & TIFF Header */
    pCur += 4;
    /* Skip 4 Byte for APP1 marker and length */
    unsigned char ExifIdentifierCode[6] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};
    memcpy(pCur, ExifIdentifierCode, 6);
    pCur += 6;

    /* Byte Order - little endian, Offset of IFD - 0x00000008.H */
    unsigned char TiffHeader[8] = {0x49, 0x49, 0x2A, 0x00, 0x08, 0x00, 0x00, 0x00};
    memcpy(pCur, TiffHeader, 8);
    pIfdStart = pCur;
    pCur += 8;

    const char asciiPrefix[] = {0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0};
    unsigned char tmpBuf[256] = {0, };
    size_t len;

    /* 0th IFD TIFF Tags */
    if (exifInfo->enableGps)
        tmp = mNum0thIfdTiff;
    else
        tmp = mNum0thIfdTiff - 1;

    memcpy(pCur, &tmp, NUM_SIZE);
    pCur += NUM_SIZE;

    LongerTagOffest += 8 + NUM_SIZE + tmp * IFD_SIZE + OFFSET_SIZE;

    writeExifIfd(&pCur, EXIF_TAG_IMAGE_WIDTH, EXIF_TYPE_LONG,
                 1, exifInfo->width);
    writeExifIfd(&pCur, EXIF_TAG_IMAGE_HEIGHT, EXIF_TYPE_LONG,
                 1, exifInfo->height);
    writeExifIfd(&pCur, EXIF_TAG_MAKE, EXIF_TYPE_ASCII,
                 strlen((char *)exifInfo->maker) + 1, exifInfo->maker, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_MODEL, EXIF_TYPE_ASCII,
                 strlen((char *)exifInfo->model) + 1, exifInfo->model, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_ORIENTATION, EXIF_TYPE_SHORT,
                 1, exifInfo->orientation);
    writeExifIfd(&pCur, EXIF_TAG_SOFTWARE, EXIF_TYPE_ASCII,
                 strlen((char *)exifInfo->software) + 1, exifInfo->software, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_DATE_TIME, EXIF_TYPE_ASCII,
                 20, exifInfo->date_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_YCBCR_POSITIONING, EXIF_TYPE_SHORT,
                 1, exifInfo->ycbcr_positioning);
    writeExifIfd(&pCur, EXIF_TAG_EXIF_IFD_POINTER, EXIF_TYPE_LONG,
                 1, LongerTagOffest);
    if (exifInfo->enableGps) {
        pGpsIfdPtr = pCur;
        /* Skip a ifd size for gps IFD pointer */
        pCur += IFD_SIZE;
    }

    /* Skip a offset size for next IFD offset */
    pNextIfdOffset = pCur;
    pCur += OFFSET_SIZE;

    /* 0th IFD Exif Private Tags */
    pCur = pIfdStart + LongerTagOffest;

    tmp = mNum0thIfdExif;
    if (mCameraId == CAMERA_FACING_BACK) {
        if (mCameraType != CAMERA_TYPE_ISP) {
            tmp -= 3;
        }
    } else {
        tmp -= 8;
    }
    memcpy(pCur, &tmp , NUM_SIZE);
    pCur += NUM_SIZE;

    LongerTagOffest += NUM_SIZE + (tmp * IFD_SIZE) + OFFSET_SIZE;

    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_TIME, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->exposure_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_FNUMBER, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->fnumber, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_PROGRAM, EXIF_TYPE_SHORT,
                 1, exifInfo->exposure_program);
    writeExifIfd(&pCur, EXIF_TAG_ISO_SPEED_RATING, EXIF_TYPE_SHORT,
                 1, exifInfo->iso_speed_rating);
    writeExifIfd(&pCur, EXIF_TAG_EXIF_VERSION, EXIF_TYPE_UNDEFINED,
                 4, exifInfo->exif_version);
    writeExifIfd(&pCur, EXIF_TAG_DATE_TIME_ORG, EXIF_TYPE_ASCII,
                 20, exifInfo->date_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_DATE_TIME_DIGITIZE, EXIF_TYPE_ASCII,
                 20, exifInfo->date_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_IMAGE_UNIQUE_ID, EXIF_TYPE_ASCII,
                 12, exifInfo->unique_id, &LongerTagOffest, pIfdStart);

    if (mCameraId == CAMERA_FACING_BACK) {
        if (CAMERA_TYPE_ISP == mCameraType) {
            writeExifIfd(&pCur, EXIF_TAG_SHUTTER_SPEED, EXIF_TYPE_SRATIONAL,
                         1, (rational_t *)&exifInfo->shutter_speed, &LongerTagOffest, pIfdStart);
            writeExifIfd(&pCur, EXIF_TAG_APERTURE, EXIF_TYPE_RATIONAL,
                         1, &exifInfo->aperture, &LongerTagOffest, pIfdStart);
            writeExifIfd(&pCur, EXIF_TAG_BRIGHTNESS, EXIF_TYPE_SRATIONAL,
                         1, (rational_t *)&exifInfo->brightness, &LongerTagOffest, pIfdStart);
        }
        writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_BIAS, EXIF_TYPE_SRATIONAL,
                     1, (rational_t *)&exifInfo->exposure_bias, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_LIGHT_SOURCE, EXIF_TYPE_SHORT,
                     1, exifInfo->light_source);
        writeExifIfd(&pCur, EXIF_TAG_FLASH, EXIF_TYPE_SHORT,
                     1, exifInfo->flash);
    }

    writeExifIfd(&pCur, EXIF_TAG_MAX_APERTURE, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->max_aperture, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_METERING_MODE, EXIF_TYPE_SHORT,
                 1, exifInfo->metering_mode);
    writeExifIfd(&pCur, EXIF_TAG_FOCAL_LENGTH, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->focal_length, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_FOCAL_35mm_LENGTH, EXIF_TYPE_SHORT,
                 1, exifInfo->focal_35mm_length);
    CLEAR(tmpBuf);
    len = strlen((char *)exifInfo->user_comment) + 1;
    memcpy(tmpBuf, asciiPrefix, sizeof(asciiPrefix));
    memcpy(tmpBuf + sizeof(asciiPrefix), exifInfo->user_comment, len);
    writeExifIfd(&pCur, EXIF_TAG_USER_COMMENT, EXIF_TYPE_UNDEFINED,
                 len + sizeof(asciiPrefix), tmpBuf, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_COLOR_SPACE, EXIF_TYPE_SHORT,
                 1, exifInfo->color_space);
    writeExifIfd(&pCur, EXIF_TAG_PIXEL_X_DIMENSION, EXIF_TYPE_LONG,
                 1, exifInfo->width);
    writeExifIfd(&pCur, EXIF_TAG_PIXEL_Y_DIMENSION, EXIF_TYPE_LONG,
                 1, exifInfo->height);
    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_MODE, EXIF_TYPE_LONG,
                 1, exifInfo->exposure_mode);
    writeExifIfd(&pCur, EXIF_TAG_WHITE_BALANCE, EXIF_TYPE_LONG,
                 1, exifInfo->white_balance);

    if (mCameraId == CAMERA_FACING_BACK) {
        writeExifIfd(&pCur, EXIF_TAG_SCENCE_CAPTURE_TYPE, EXIF_TYPE_LONG,
                 1, exifInfo->scene_capture_type);
        writeExifIfd(&pCur, EXIF_TAG_SENSING_METHOD, EXIF_TYPE_SHORT,
                 1, exifInfo->sensing_method);
    }

    tmp = 0;
    /* next IFD offset */
    memcpy(pCur, &tmp, OFFSET_SIZE);
    pCur += OFFSET_SIZE;

    /* 0th IFD GPS Info Tags */
    if (exifInfo->enableGps) {
        /* GPS IFD pointer skipped on 0th IFD */
        writeExifIfd(&pGpsIfdPtr, EXIF_TAG_GPS_IFD_POINTER, EXIF_TYPE_LONG,
                     1, LongerTagOffest);

        pCur = pIfdStart + LongerTagOffest;

        tmp = mNum0thIfdGps;
        memcpy(pCur, &tmp, NUM_SIZE);
        pCur += NUM_SIZE;

        LongerTagOffest += NUM_SIZE + mNum0thIfdGps * IFD_SIZE + OFFSET_SIZE;

        writeExifIfd(&pCur, EXIF_TAG_GPS_VERSION_ID, EXIF_TYPE_BYTE,
                     4, exifInfo->gps_version_id);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LATITUDE_REF, EXIF_TYPE_ASCII,
                     2, exifInfo->gps_latitude_ref);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LATITUDE, EXIF_TYPE_RATIONAL,
                     3, exifInfo->gps_latitude, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LONGITUDE_REF, EXIF_TYPE_ASCII,
                     2, exifInfo->gps_longitude_ref);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LONGITUDE, EXIF_TYPE_RATIONAL,
                     3, exifInfo->gps_longitude, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_GPS_ALTITUDE_REF, EXIF_TYPE_BYTE,
                     1, exifInfo->gps_altitude_ref);
        writeExifIfd(&pCur, EXIF_TAG_GPS_ALTITUDE, EXIF_TYPE_RATIONAL,
                     1, &exifInfo->gps_altitude, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_GPS_TIMESTAMP, EXIF_TYPE_RATIONAL,
                     3, exifInfo->gps_timestamp, &LongerTagOffest, pIfdStart);
        CLEAR(tmpBuf);
        len = strlen((char *)exifInfo->gps_processing_method);
        memcpy(tmpBuf, asciiPrefix, sizeof(asciiPrefix));
        memcpy(tmpBuf + sizeof(asciiPrefix), exifInfo->gps_processing_method, len);
        writeExifIfd(&pCur, EXIF_TAG_GPS_PROCESSING_METHOD, EXIF_TYPE_UNDEFINED,
                     len + sizeof(asciiPrefix), tmpBuf, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_GPS_DATESTAMP, EXIF_TYPE_ASCII,
                     11, exifInfo->gps_datestamp, &LongerTagOffest, pIfdStart);
        tmp = 0;
        /* next IFD offset */
        memcpy(pCur, &tmp, OFFSET_SIZE);
        pCur += OFFSET_SIZE;
    }

    /* 1th IFD TIFF Tags */
    if ((thumbBuf != NULL) && (thumbSize > 0)) {
        if (CC_UNLIKELY(!exifOutBufSize)) {
            ALOGE("makeExif: error, exifOutBufSize is zero");
            return 0;
        }

        tmp = LongerTagOffest;
        /* NEXT IFD offset skipped on 0th IFD */
        memcpy(pNextIfdOffset, &tmp, OFFSET_SIZE);

        pCur = pIfdStart + LongerTagOffest;

        tmp = mNum1thIfdTiff;
        memcpy(pCur, &tmp, NUM_SIZE);
        pCur += NUM_SIZE;

        LongerTagOffest += NUM_SIZE + mNum1thIfdTiff * IFD_SIZE + OFFSET_SIZE;

        writeExifIfd(&pCur, EXIF_TAG_IMAGE_WIDTH, EXIF_TYPE_LONG,
                     1, exifInfo->widthThumb);
        writeExifIfd(&pCur, EXIF_TAG_IMAGE_HEIGHT, EXIF_TYPE_LONG,
                     1, exifInfo->heightThumb);
        writeExifIfd(&pCur, EXIF_TAG_COMPRESSION_SCHEME, EXIF_TYPE_SHORT,
                     1, exifInfo->compression_scheme);
        writeExifIfd(&pCur, EXIF_TAG_ORIENTATION, EXIF_TYPE_SHORT,
                     1, exifInfo->orientation);
        writeExifIfd(&pCur, EXIF_TAG_X_RESOLUTION, EXIF_TYPE_RATIONAL,
                     1, &exifInfo->x_resolution, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_Y_RESOLUTION, EXIF_TYPE_RATIONAL,
                     1, &exifInfo->y_resolution, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_RESOLUTION_UNIT, EXIF_TYPE_SHORT,
                     1, exifInfo->resolution_unit);
        writeExifIfd(&pCur, EXIF_TAG_JPEG_INTERCHANGE_FORMAT, EXIF_TYPE_LONG,
                     1, LongerTagOffest);
        writeExifIfd(&pCur, EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LEN, EXIF_TYPE_LONG,
                     1, thumbSize);

        tmp = 0;
        /* next IFD offset */
        memcpy(pCur, &tmp, OFFSET_SIZE);
        pCur += OFFSET_SIZE;

#ifdef SUPPORT_64BITS
        if (CC_UNLIKELY((unsigned long)pIfdStart + LongerTagOffest + thumbSize >= ((unsigned long)exifOutBuf + exifOutBufSize))) {
            ALOGE("makeExif: error, thumbnail size(%d bytes) is too big ", thumbSize);
            return 0;
        }
#else
        if (CC_UNLIKELY(pIfdStart + LongerTagOffest + thumbSize >= (void *)((uint32_t)exifOutBuf + exifOutBufSize))) {
            ALOGE("makeExif: error, thumbnail size(%d bytes) is too big ", thumbSize);
            return 0;
        }
#endif
        memcpy(pIfdStart + LongerTagOffest,
               thumbBuf, thumbSize);
        LongerTagOffest += thumbSize;
    } else {
        tmp = 0;
        /* NEXT IFD offset skipped on 0th IFD */
        memcpy(pNextIfdOffset, &tmp, OFFSET_SIZE);
    }

    unsigned char App1Marker[2] = {0xff, 0xe1};
    memcpy(pApp1Start, App1Marker, 2);
    pApp1Start += 2;

    uint32_t size = 10 + LongerTagOffest;
    /* APP1 Maker isn't counted */
    tmp = size - 2;
    unsigned char size_le[2] = {(unsigned char)((tmp >> 8) & 0xFF), (unsigned char)(tmp & 0xFF)};
    memcpy(pApp1Start, size_le, 2);

    ALOGV("makeExif X: size %d byte", size);
    return size;
}

inline void Exif::writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 uint32_t value)
{
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, &value, 4);
    *pCur += 4;
}

inline void Exif::writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 unsigned char *pValue)
{
    char buf[4] = {0,};
    memcpy(buf, pValue, count);
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, buf, 4);
    *pCur += 4;
}


inline void Exif::writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 unsigned char *pValue,
                                 unsigned int *offset,
                                 unsigned char *start)
{
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, offset, 4);
    *pCur += 4;
    memcpy(start + *offset, pValue, count);
    *offset += count;
}

inline void Exif::writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 rational_t *pValue,
                                 unsigned int *offset,
                                 unsigned char *start)
{
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, offset, 4);
    *pCur += 4;
    memcpy(start + *offset, pValue, 8 * count);
    *offset += 8 * count;
}

};

