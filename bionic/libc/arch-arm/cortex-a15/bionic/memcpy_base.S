/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 2013 ARM Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define PLD_OFS	8
#define DST_ALIGN_BOUND	(32*1024)

ENTRY_PRIVATE(MEMCPY_BASE)
        .cfi_def_cfa_offset 8
        .cfi_rel_offset r0, 0
        .cfi_rel_offset lr, 4

        // Assumes that n >= 0, and dst, src are valid pointers.
        // For any sizes less than 832 use the neon code that doesn't
        // care about the src alignment. This avoids any checks
        // for src alignment, and offers the best improvement since
        // smaller sized copies are dominated by the overhead of
        // the pre and post main loop.
        // For larger copies, if src and dst cannot both be aligned to
        // word boundaries, use the neon code.
        // For all other copies, align dst to a double word boundary
        // and copy using LDRD/STRD instructions.

        cmp     r2, #16
        blo     .L_copy_less_than_16_unknown_align

        // TODO: The aligned copy code is extremely slow copying some large
        //       buffers so always go through the unaligned path for now.
        cmp     r2, #832
        bge     .L_check_alignment

.L_copy_unknown_alignment:
        // Unknown alignment of src and dst.
        // Assumes that the first few bytes have already been prefetched.

        // Align destination to 128 bits. The mainloop store instructions
        // require this alignment or they will throw an exception.
        rsb         r3, r0, #0
        ands        r3, r3, #0xF
        beq         2f

        // Copy up to 15 bytes (count in r3).
        sub         r2, r2, r3
        movs        ip, r3, lsl #31

        itt         mi
        ldrbmi      lr, [r1], #1
        strbmi      lr, [r0], #1
        itttt       cs
        ldrbcs      ip, [r1], #1
        ldrbcs      lr, [r1], #1
        strbcs      ip, [r0], #1
        strbcs      lr, [r0], #1

        movs        ip, r3, lsl #29
        bge         1f
        // Copies 4 bytes, dst 32 bits aligned before, at least 64 bits after.
        vld4.8      {d0[0], d1[0], d2[0], d3[0]}, [r1]!
        vst4.8      {d0[0], d1[0], d2[0], d3[0]}, [r0, :32]!
1:      bcc         2f
        // Copies 8 bytes, dst 64 bits aligned before, at least 128 bits after.
        vld1.8      {d0}, [r1]!
        vst1.8      {d0}, [r0, :64]!
2:
	subs	r2, r2, #128
	blo	3f

1:	// The main loop copies 128 bytes at a time.
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	subs	r2, r2, #128
	bge	1b

	adds	r2, #128
	beq	.L_copy_done_lt_832
	sub	r2, #128
3:
	adds	r2, r2, #64
	blo	2f

	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	sub	r2, r2, #64

2:      // Fix-up the remaining count and make sure we have >= 32 bytes left.
        adds        r2, r2, #32
        blo         3f

        // 32 bytes. These cache lines were already preloaded.
        vld1.8      {d0 - d3},  [r1]!
        sub         r2, r2, #32
        vst1.8      {d0 - d3},  [r0, :128]!
3:      // Less than 32 left.
        add         r2, r2, #32
        tst         r2, #0x10
        beq         .L_copy_less_than_16_unknown_align
        // Copies 16 bytes, destination 128 bits aligned.
        vld1.8      {d0, d1}, [r1]!
        vst1.8      {d0, d1}, [r0, :128]!

.L_copy_less_than_16_unknown_align:
        // Copy up to 15 bytes (count in r2).
        movs        ip, r2, lsl #29
        bcc         1f
        vld1.8      {d0}, [r1]!
        vst1.8      {d0}, [r0]!
1:      bge         2f
        vld4.8      {d0[0], d1[0], d2[0], d3[0]}, [r1]!
        vst4.8      {d0[0], d1[0], d2[0], d3[0]}, [r0]!

2:      // Copy 0 to 4 bytes.
        lsls        r2, r2, #31
        itt         ne
        ldrbne      lr, [r1], #1
        strbne      lr, [r0], #1
        itttt       cs
        ldrbcs      ip, [r1], #1
        ldrbcs      lr, [r1]
        strbcs      ip, [r0], #1
        strbcs      lr, [r0]

.L_copy_done_lt_832:
        pop         {r0, pc}

.L_check_alignment:
        // If src and dst cannot both be aligned to a word boundary,
        // use the unaligned copy version.
        eor     r3, r0, r1
        ands    r3, r3, #0x3
        bne     .L_copy_unknown_alignment
END(MEMCPY_BASE)

ENTRY_PRIVATE(MEMCPY_BASE_ALIGNED)
        .cfi_def_cfa_offset 8
        .cfi_rel_offset r0, 0
        .cfi_rel_offset lr, 4

        // To try and improve performance, stack layout changed,
        // i.e., not keeping the stack looking like users expect
        // (highest numbered register at highest address).
        strd    r4, r5, [sp, #-8]!
        .cfi_adjust_cfa_offset 8
        .cfi_rel_offset r4, 0
        .cfi_rel_offset r5, 4

	pld	[r1, #(64*1)]
	pld	[r1, #(64*2)]
	pld	[r1, #(64*3)]

	mov	r3, r2		@ backup original size

	cmp	r2, #DST_ALIGN_BOUND
	blt	.L_dst_aligned_done

	ands	r4, r0, 63
	bne	.L_dst_not_64_aligned

.L_dst_aligned_done:
	cmp	r2, 0
	beq	.L_copy_done

	sub	r4, r2, (128*PLD_OFS)
	cmp	r4, 128
	blt	.L_no_pld_copy

#ifndef MEMCPY_AARCH32
	cmp	r3, #(64*1024)
	beq	.L_cross_copy
#endif

.L_normal_copy:
	pld	[r1, #(64*4)]
	pld	[r1, #(64*5)]
	pld	[r1, #(64*6)]
	pld	[r1, #(64*7)]
	pld	[r1, #(64*8)]

	sub     r4, #128
2:
	vld1.64	{d0-d1}, [r1]!	// r1+=16
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	pld     [r1, #(64*PLD_OFS)]
	vld1.64	{d0-d1}, [r1]!	// r1+=16
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	pld     [r1, #(64*PLD_OFS)]
	subs    r4, r4, #128
	bge     2b
	add	r4, 128

.L_no_pld_copy:
	add	r4, #(128*PLD_OFS)
	sub	r4, 128
1:
	vld1.64	{d0-d1}, [r1]!	// r1+=16
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!	// r1+=16
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	subs	r4, r4, 128
	bge	1b

	tst	r4, 127
	beq	.L_copy_done

	mov	r2, r4
	add	r2, #128

#ifndef MEMCPY_AARCH32
	b	.L_rest_copy

.L_cross_copy:
	subs	ip, r1, r0
	bls	1f		@ src < dst ? don't care
	cmp	ip, 128		@ src >= dst+128 --> cross copy ok
	blt	.L_normal_copy

1:	push	{r6-r10}
	mov	r2, r4
	sub     r2, 128

	add	r4, r1, 16
	add	r5, r0, 16
	add	r6, r1, 48
	add	r7, r0, 48
	add	r8, r1, 32
	add	r9, r0, 32
	mov	r10, 64
2:
	vld1.64	{d0-d1}, [r1], r10	// r1+=64
	vst1.64	{d0-d1}, [r0], r10
	vld1.64	{d0-d1}, [r4], r10
	vst1.64	{d0-d1}, [r5], r10
	vld1.64	{d0-d1}, [r6], r10
	vst1.64	{d0-d1}, [r7], r10
	vld1.64	{d0-d1}, [r8], r10
	vst1.64	{d0-d1}, [r9], r10
	pld     [r1, #(64*PLD_OFS)]
	vld1.64	{d0-d1}, [r1], r10	// r1+=64
	vst1.64	{d0-d1}, [r0], r10
	vld1.64	{d0-d1}, [r4], r10
	vst1.64	{d0-d1}, [r5], r10
	vld1.64	{d0-d1}, [r6], r10
	vst1.64	{d0-d1}, [r7], r10
	vld1.64	{d0-d1}, [r8], r10
	vst1.64	{d0-d1}, [r9], r10
	pld     [r1, #(64*PLD_OFS)]

	subs    r2, r2, 128
	bge     2b
	add	r4, r2, 128
	pop	{r6-r10}
	b	.L_no_pld_copy
#endif

.L_rest_copy:
	movs	ip, r2, lsl #25
	bpl	32f
	// copy 64 bytes
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
32:	movs	ip, r2, lsl #27
	bcc	1f
	// copy 32 bytes
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
1:	bge	2f
	// copy 16 bytes
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
2:	movs	ip, r2, lsl #29
	bcc	3f
	// copy 8 bytes
	ldr	r5, [r1], #4
	str	r5, [r0], #4
	ldr	r5, [r1], #4
	str	r5, [r0], #4
3:	bge	4f
	// copy 4 bytes
	ldr	r5, [r1], #4
	str	r5, [r0], #4
4:	lsls	ip, r2, #31
	itt	mi
	ldrbmi	r5, [r1], #1
	strbmi	r5, [r0], #1
	itttt	cs
	ldrbcs	ip, [r1], #1
	ldrbcs	r5, [r1]
	strbcs	ip, [r0], #1
	strbcs	r5, [r0]

.L_copy_done:
        // Restore registers: optimized pop {r0, pc}
	ldrd    r4, r5, [sp], #8
	pop     {r0, pc}

.L_dst_not_64_aligned:
	// align dst to 64
	rsb	r4, r4, 64	@ if dst: 1 --> copy bytes to align: 64-1
	cmp	r2, r4
	itte	lt
	movlt	r4, r2		@ if total < bytes --> bytes = total
	movlt	r2, 0		@ if total < bytes --> total = 0
	subge	r2, r4		@ else total >= bytes --> total -= bytes

	// copy order 1 -> 2 -> 4 -> 8 ... because dst is unaligned
	lsls	ip, r4, #31
	itt	mi
	ldrbmi	r5, [r1], #1
	strbmi	r5, [r0], #1
	itttt	cs
	ldrbcs	ip, [r1], #1
	ldrbcs	r5, [r1], #1
	strbcs	ip, [r0], #1
	strbcs	r5, [r0], #1

	movs	ip, r4, lsl #29
	bge	1f
	// copy 4 bytes
	ldr	r5, [r1], 4
	str	r5, [r0], 4
1:	bcc	2f
	// copy 8 bytes
	ldr	r5, [r1], 4
	str	r5, [r0], 4
	ldr	r5, [r1], 4
	str	r5, [r0], 4
2:	movs	ip, r4, lsl #27
	bge	3f
	// copy 16 bytes
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
3:	bcc	4f
	// copy 32 bytes
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
	vld1.64	{d0-d1}, [r1]!
	vst1.64	{d0-d1}, [r0]!
4:
	b	.L_dst_aligned_done
END(MEMCPY_BASE_ALIGNED)
