/*
 *
 * Copyright 2014 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file    swconverter_dummy.c
 *
 * @brief   dummu file of machine dependent implemention of CSC functions
 *
 * @author  Cho KyongHo (pullip.cho@samsung.com)
 *
 * @version 1.0
 *
 * @history
 *   2014.08.27 : Create
 */

void csc_interleave_memcpy_neon(
    unsigned char *dest,
    unsigned char *src1,
    unsigned char *src2,
    unsigned int src_size)
{
}


void csc_tiled_to_linear_y_neon(
    unsigned char *y_dst,
    unsigned char *y_src,
    unsigned int width,
    unsigned int height)
{
}

void csc_tiled_to_linear_uv_neon(
    unsigned char *uv_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height)
{
}

void csc_tiled_to_linear_uv_deinterleave_neon(
    unsigned char *u_dst,
    unsigned char *v_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height)
{
}

void csc_ARGB8888_to_YUV420SP_NEON(
    unsigned char *y_dst,
    unsigned char *uv_dst,
    unsigned char *rgb_src,
    unsigned int width,
    unsigned int height)
{
}

void csc_RGBA8888_to_YUV420SP_NEON(
    unsigned char *y_dst,
    unsigned char *uv_dst,
    unsigned char *rgb_src,
    unsigned int width,
    unsigned int height)
{
}
