/*
 *
 * Copyright 2013 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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
 * @file    csc_BGRA8888_to_RGBA8888.s
 * @brief   color format converter
 * @author  Hyungdeok Lee (hd0408.lee@samsung.com)
 * @version 1.0
 * @history
 *   2013.02.28 : Create
 */

/*
 * Source BGRA8888 copy to Dest RGBA8888.
 * Use neon interleaved load instruction, easly swap R ch to B ch.
 *
 * @param dest
 *   dst address[out]
 *
 * @param src
 *   src address[in]
 *
 * @param width
 *   line width [in]
 *
 * @param bpp
 *   bpp only concerned about 4
 */

    .arch armv7-a
    .text
    .global csc_BGRA8888_RGBA8888_NEON
    .type   csc_BGRA8888_RGBA8888_NEON, %function
csc_BGRA8888_RGBA8888_NEON:
    .fnstart

    @r0     dest
    @r1     src
    @r2     width
    @r3     bpp
    @r4
    @r5
    @r6
    @r7
    @r8     temp1
    @r9     temp2
    @r10    dest_addr
    @r11    src_addr
    @r12    temp_width
    @r14    i

    stmfd       sp!, {r4-r12,r14}       @ backup registers

    mov         r10, r0
    mov         r11, r1
	mov			r9, r2, lsr #5			@ r9 = r2 >> 5 (32)
	and			r14, r9, #3				@ r14 = r9 & 3
	mov			r12, r2, lsr #7			@ r12 = r2 >> 7 (128)

    cmp         r12, #0
    beq         LESS_THAN_128

@ Process d0 to d3 at once. 4 times same operation. := 8 byte * 4 * 4 = 128 byte loop.
LOOP_128:
	@pld		[r11]	@ cache line fill. use this for r11 region set by cachable.
	vld4.8		{d0, d1, d2, d3},	[r11]!
	vswp		d0, d2
	vst4.8		{d0, d1, d2, d3},	[r10]!

	vld4.8		{d0, d1, d2, d3},	[r11]!
	vswp		d0, d2
	vst4.8		{d0, d1, d2, d3},	[r10]!

	vld4.8		{d0, d1, d2, d3},	[r11]!
	vswp		d0, d2
	vst4.8		{d0, d1, d2, d3},	[r10]!

	vld4.8		{d0, d1, d2, d3},	[r11]!
	vswp		d0, d2
	vst4.8		{d0, d1, d2, d3},	[r10]!

    subs        r12, #1
    bne         LOOP_128

LESS_THAN_128:
	cmp			r14, #0
	beq			END

LOOP_32:
	vld4.8		{d0, d1, d2, d3},	[r11]!
	vswp		d0, d2
	vst4.8		{d0, d1, d2, d3},	[r10]!
    subs        r14, #1
    bne         LOOP_32

END:
    ldmfd       sp!, {r4-r12,r15}       @ restore registers
    .fnend
