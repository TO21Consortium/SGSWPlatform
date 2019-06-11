#ifndef SK_PREBUILT_OPT_HEADER
#define SK_PREBUILT_OPT_HEADER

#if defined(USES_SKIA_PREBUILT_LIBRARY)
#include "SkColorPriv.h"

#include <dlfcn.h>
static void* gLibHandle = NULL;
typedef int (*blitH_temp)(uint16_t *dst, int count, uint32_t src_expand, unsigned int scale);
typedef int (*S32A_Opaque_BlitRow32_neon_src_alpha_temp)(SkPMColor* SK_RESTRICT dst,
        const SkPMColor* SK_RESTRICT src, int count, U8CPU alpha);
typedef int (*S32_Blend_BlitRow32_neon_temp)(SkPMColor* SK_RESTRICT dst,
        const SkPMColor* SK_RESTRICT src, int count, U8CPU alpha);
typedef int (*S32A_D565_Opaque_neon_temp)(uint16_t* SK_RESTRICT dst,
		const SkPMColor* SK_RESTRICT src, int count, U8CPU alpha);


static blitH_temp blitH_NEON = NULL;
static S32A_Opaque_BlitRow32_neon_src_alpha_temp S32A_Opaque_BlitRow32_OPT = NULL;
static S32_Blend_BlitRow32_neon_temp S32_Blend_BlitRow32_OPT = NULL;
static S32A_D565_Opaque_neon_temp S32A_D565_Opaque_neon_OPT = NULL;


int check_blitH_NEON(uint16_t *dst, int count, uint32_t src_expand, unsigned int scale);
int check_S32A_Opaque_BlitRow32_neon_src_alpha(SkPMColor* SK_RESTRICT dst,
        const SkPMColor* SK_RESTRICT src, int count, U8CPU alpha);
int check_S32_Blend_BlitRow32_neon(SkPMColor* SK_RESTRICT dst,
        const SkPMColor* SK_RESTRICT src, int count, U8CPU alpha);
int check_S32A_D565_Opaque_neon(uint16_t* SK_RESTRICT dst,
        const SkPMColor* SK_RESTRICT src, int count, U8CPU alpha);
#endif

#endif
