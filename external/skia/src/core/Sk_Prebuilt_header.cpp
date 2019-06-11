#include "Sk_Prebuilt_header.h"
#include <cutils/log.h>
#include "SkFimgApi.h"

int check_S32A_D565_Opaque_neon(uint16_t* SK_RESTRICT dst,
		const SkPMColor* SK_RESTRICT src, int count, U8CPU alpha)
{
    if (!gLibHandle) {
        gLibHandle = dlopen("libskia_opt.so", RTLD_NOW);
        if (!gLibHandle) {
            ALOGE("libskia_opt: Can't be loaded: gLibHandle(NULL)");
            return -1;
        }
    }
    if (!S32A_D565_Opaque_neon_OPT && gLibHandle) {
        S32A_D565_Opaque_neon_OPT =
            (S32A_D565_Opaque_neon_temp)dlsym(gLibHandle, "S32A_D565_Opaque_neon_OPT");
        if (!S32A_D565_Opaque_neon_OPT) {
            ALOGE("S32A_D565_Opaque_neon_OPT cannot be found gLibHandle(%p),\
                S32A_D565_Opaque_neon N(%p)",
                gLibHandle, S32A_D565_Opaque_neon_OPT);
            return -1;
        }
    }
    S32A_D565_Opaque_neon_OPT(dst, src, count, alpha);
    return 0;
}


int check_blitH_NEON(uint16_t *dst, int count, uint32_t src_expand, unsigned int scale)
{
    if (!gLibHandle) {
        gLibHandle = dlopen("libskia_opt.so", RTLD_NOW);
        if (!gLibHandle) {
            ALOGE("libskia_opt: Can't be loaded: gLibHandle(NULL)");
            return -1;
        }
    }

    if (!blitH_NEON && gLibHandle) {
        blitH_NEON = (blitH_temp)dlsym(gLibHandle, "blitH_NEON");
        if (!blitH_NEON) {
            ALOGE("blitH_OPT cannot be found gLibHandle(%p), blitH_NEON(%p)", gLibHandle, blitH_NEON);
            return -1;
        }
    }

    blitH_NEON(dst, count, src_expand, scale);
    return 0;
}
int check_S32A_Opaque_BlitRow32_neon_src_alpha(SkPMColor* SK_RESTRICT dst,
        const SkPMColor* SK_RESTRICT src, int count, U8CPU alpha) {
    if (!gLibHandle) {
        gLibHandle = dlopen("libskia_opt.so", RTLD_NOW);
        if (!gLibHandle) {
            ALOGE("libskia_opt: Can't be loaded: gLibHandle(NULL)");
            return -1;
        }
    }

    if (!S32A_Opaque_BlitRow32_OPT && gLibHandle) {
        S32A_Opaque_BlitRow32_OPT = (S32A_Opaque_BlitRow32_neon_src_alpha_temp)
                                  dlsym(gLibHandle, "S32A_Opaque_BlitRow32_neon_src_alpha_OPT");
        if (!S32A_Opaque_BlitRow32_OPT) {
            ALOGE("S32A_Opaque_BlitRow32_OPT cannot be found gLibHandle(%p), S32A_Opaque_BlitRow32_OPT(%p)", gLibHandle, S32A_Opaque_BlitRow32_OPT);
            return -1;
        }
    }

    S32A_Opaque_BlitRow32_OPT(dst, src, count, alpha);
    return 0;
}
int check_S32_Blend_BlitRow32_neon(SkPMColor* SK_RESTRICT dst,
        const SkPMColor* SK_RESTRICT src, int count, U8CPU alpha) {
    if (!gLibHandle) {
        gLibHandle = dlopen("libskia_opt.so", RTLD_NOW);
        if (!gLibHandle) {
            ALOGE("libskia_opt: Can't be loaded: gLibHandle(NULL)");
            return -1;
        }
    }

    if (!S32_Blend_BlitRow32_OPT && gLibHandle) {
        S32_Blend_BlitRow32_OPT = (S32_Blend_BlitRow32_neon_temp)
            dlsym(gLibHandle, "S32_Blend_BlitRow32_neon_OPT");
        if (!S32_Blend_BlitRow32_OPT) {
            ALOGE("S32_Blend_BlitRow32_OPT cannot be found gLibHandle(%p), S32_Blend_BlitRow32_OPT(%p)", gLibHandle, S32_Blend_BlitRow32_OPT);
            return -1;
        }
    }

    S32_Blend_BlitRow32_OPT(dst, src, count, alpha);
    return 0;
}
