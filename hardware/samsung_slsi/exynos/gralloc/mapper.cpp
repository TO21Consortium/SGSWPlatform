/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <ion/ion.h>
#include <linux/ion.h>
#include "gralloc_priv.h"
#include "exynos_format.h"

#define INT_TO_PTR(var) ((void *)(unsigned long)var)
#define PRIV_SIZE 64

#if MALI_AFBC_GRALLOC == 1
//#include "gralloc_buffer_priv.h"
#endif

#include "format_chooser.h"


/*****************************************************************************/
int getIonFd(gralloc_module_t const *module)
{
    private_module_t* m = const_cast<private_module_t*>(reinterpret_cast<const private_module_t*>(module));
    if (m->ionfd == -1)
        m->ionfd = ion_open();
    return m->ionfd;
}

#ifdef USES_EXYNOS_CRC_BUFFER_ALLOC
int gralloc_get_tile_num(unsigned int value)
{
    int tile_num;
    tile_num = ((value + CRC_TILE_SIZE - 1) & ~(CRC_TILE_SIZE - 1)) / CRC_TILE_SIZE;
    return tile_num;
}

bool gralloc_crc_allocation_check(int format, int width, int height, int flags)
{
    bool supported = false;
    switch (format) {
    case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_RAW_SENSOR:
    case HAL_PIXEL_FORMAT_RAW_OPAQUE:
    case HAL_PIXEL_FORMAT_BLOB:
        if (!(flags & GRALLOC_USAGE_PROTECTED) &&
			(width >= CRC_LIMIT_WIDTH) && (height >= CRC_LIMIT_HEIGHT))
            supported = true;
        break;
    default:
        break;
    }

    return supported;
}
#endif /* USES_EXYNOS_CRC_BUFFER_ALLOC */

static int gralloc_map(gralloc_module_t const* module, buffer_handle_t handle)
{
    size_t chroma_vstride = 0;
    size_t chroma_size = 0;
    size_t ext_size = 256;
    void *privAddress;

    private_handle_t *hnd = (private_handle_t*)handle;

    if (hnd->flags & GRALLOC_USAGE_PROTECTED || hnd->flags & GRALLOC_USAGE_NOZEROED) {
        hnd->base = hnd->base1 = hnd->base2 = 0;
    }

    switch (hnd->format) {
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
        chroma_vstride = ALIGN(hnd->height / 2, 32);
        chroma_size = chroma_vstride * hnd->stride + ext_size;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        chroma_size = hnd->stride * ALIGN(hnd->vstride / 2, 8) + ext_size;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
        chroma_size = (hnd->vstride / 2) * ALIGN(hnd->stride / 2, 16) + ext_size;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
        chroma_size = (hnd->stride * ALIGN(hnd->vstride / 2, 8) + 64) + ((ALIGN(hnd->width / 4, 16) * (hnd->height / 2)) + 64) + ext_size;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
        chroma_size = hnd->stride * ALIGN(hnd->vstride / 2, 8) + ext_size;
        privAddress = mmap(0, PRIV_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, hnd->fd2, 0);
        if (privAddress == MAP_FAILED) {
            ALOGE("%s: could not mmap %s", __func__, strerror(errno));
        } else {
            hnd->base2 = (uint64_t)privAddress;
            ion_sync_fd(getIonFd(module), hnd->fd2);
        }
        break;
    default:
#ifdef USES_EXYNOS_CRC_BUFFER_ALLOC
        if (gralloc_crc_allocation_check(hnd->format, hnd->width, hnd->height, hnd->flags)) {
            int num_tiles_x, num_tiles_y;
            num_tiles_x = gralloc_get_tile_num(hnd->width);
            num_tiles_y = gralloc_get_tile_num(hnd->height);
            chroma_size = num_tiles_x * num_tiles_y * sizeof(long long unsigned int)
                + sizeof(struct gralloc_crc_header);
        }
#endif /* USES_EXYNOS_CRC_BUFFER_ALLOC */
        break;
    }

    if ((hnd->flags & GRALLOC_USAGE_PROTECTED) &&
            !(hnd->flags & GRALLOC_USAGE_PRIVATE_NONSECURE)) {
        return 0;
    }

    if (!(hnd->flags & GRALLOC_USAGE_PROTECTED) && !(hnd->flags & GRALLOC_USAGE_NOZEROED)) {
        void* mappedAddress = mmap(0, hnd->size, PROT_READ|PROT_WRITE, MAP_SHARED,
                                   hnd->fd, 0);
        if (mappedAddress == MAP_FAILED) {
            ALOGE("%s: could not mmap %s", __func__, strerror(errno));
            return -errno;
        }
        ALOGV("%s: base %p %d %d %d %d\n", __func__, mappedAddress, hnd->size,
              hnd->width, hnd->height, hnd->stride);
        hnd->base = (uint64_t)mappedAddress;
        ion_sync_fd(getIonFd(module), hnd->fd);

        if (hnd->fd1 >= 0) {
            void *mappedAddress1 = (void*)mmap(0, chroma_size, PROT_READ|PROT_WRITE,
                                                MAP_SHARED, hnd->fd1, 0);
            hnd->base1 = (uint64_t)mappedAddress1;
            ion_sync_fd(getIonFd(module), hnd->fd1);
        }
        if (hnd->fd2 >= 0) {
            if (hnd->format != HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV) {
                void *mappedAddress2 = (void*)mmap(0, chroma_size, PROT_READ|PROT_WRITE, MAP_SHARED, hnd->fd2, 0);
                hnd->base2 = (uint64_t)mappedAddress2;
                ion_sync_fd(getIonFd(module), hnd->fd2);
            }
        }
    }

    return 0;
}

static int gralloc_unmap(gralloc_module_t const* module, buffer_handle_t handle)
{
    private_handle_t* hnd = (private_handle_t*)handle;
    size_t chroma_vstride = 0;
    size_t chroma_size = 0;
    size_t ext_size = 256;

    switch (hnd->format) {
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
        chroma_vstride = ALIGN(hnd->height / 2, 32);
        chroma_size = chroma_vstride * hnd->stride + ext_size;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        chroma_size = hnd->stride * ALIGN(hnd->vstride / 2, 8) + ext_size;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
        chroma_size = (hnd->vstride / 2) * ALIGN(hnd->stride / 2, 16) + ext_size;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
        chroma_size = (hnd->stride * ALIGN(hnd->vstride / 2, 8) + 64) + ((ALIGN(hnd->width / 4, 16) * (hnd->height / 2)) + 64) + ext_size;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
        chroma_size = hnd->stride * ALIGN(hnd->vstride / 2, 8) + ext_size;
        if (munmap(INT_TO_PTR(hnd->base2), PRIV_SIZE) < 0) {
            ALOGE("%s :could not unmap %s %llx %d", __func__, strerror(errno), hnd->base2, chroma_size);
        }
        hnd->base2 = 0;
        break;
    default:
#ifdef USES_EXYNOS_CRC_BUFFER_ALLOC
        if (gralloc_crc_allocation_check(hnd->format, hnd->width, hnd->height, hnd->flags)) {
            int num_tiles_x, num_tiles_y;
            num_tiles_x = gralloc_get_tile_num(hnd->width);
            num_tiles_y = gralloc_get_tile_num(hnd->height);
            chroma_size = num_tiles_x * num_tiles_y * sizeof(long long unsigned int)
                + sizeof(struct gralloc_crc_header);
        }
#endif /* USES_EXYNOS_CRC_BUFFER_ALLOC */
        break;
    }

    if (!hnd->base)
        return 0;

    if (munmap(INT_TO_PTR(hnd->base), hnd->size) < 0) {
        ALOGE("%s :could not unmap %s %llx %d", __func__, strerror(errno),
              hnd->base, hnd->size);
    }
    ALOGV("%s: base %llx %d %d %d %d\n", __func__, hnd->base, hnd->size,
          hnd->width, hnd->height, hnd->stride);
    hnd->base = 0;
    if (hnd->fd1 >= 0) {
        if (!hnd->base1)
            return 0;
        if (munmap(INT_TO_PTR(hnd->base1), chroma_size) < 0) {
            ALOGE("%s :could not unmap %s %llx %d", __func__, strerror(errno),
                  hnd->base1, chroma_size);
        }
        hnd->base1 = 0;
    }
    if (hnd->fd2 >= 0) {
        if (!hnd->base2)
            return 0;
        if (munmap(INT_TO_PTR(hnd->base2), chroma_size) < 0) {
            ALOGE("%s :could not unmap %s %llx %d", __func__, strerror(errno),
                  hnd->base2, chroma_size);
        }
        hnd->base2 = 0;
    }
    return 0;
}

/*****************************************************************************/

int grallocMap(gralloc_module_t const* module, private_handle_t *hnd)
{
    return gralloc_map(module, hnd);
}

int grallocUnmap(gralloc_module_t const* module, private_handle_t *hnd)
{
    return gralloc_unmap(module, hnd);
}

static pthread_mutex_t sMapLock = PTHREAD_MUTEX_INITIALIZER;

/*****************************************************************************/

int gralloc_register_buffer(gralloc_module_t const* module,
                            buffer_handle_t handle)
{
    int err;
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    err = gralloc_map(module, handle);

    private_handle_t* hnd = (private_handle_t*)handle;
    ALOGV("%s: base %llx %d %d %d %d\n", __func__, hnd->base, hnd->size,
          hnd->width, hnd->height, hnd->stride);

    int ret;
    ret = ion_import(getIonFd(module), hnd->fd, &hnd->handle);
    if (ret)
        ALOGE("error importing handle %d %x\n", hnd->fd, hnd->format);
    if (hnd->fd1 >= 0) {
        ret = ion_import(getIonFd(module), hnd->fd1, &hnd->handle1);
        if (ret)
            ALOGE("error importing handle1 %d %x\n", hnd->fd1, hnd->format);
    }
    if (hnd->fd2 >= 0) {
        ret = ion_import(getIonFd(module), hnd->fd2, &hnd->handle2);
        if (ret)
            ALOGE("error importing handle2 %d %x\n", hnd->fd2, hnd->format);
    }

    return err;
}

int gralloc_unregister_buffer(gralloc_module_t const* module,
                              buffer_handle_t handle)
{
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;
    ALOGV("%s: base %llx %d %d %d %d\n", __func__, hnd->base, hnd->size,
          hnd->width, hnd->height, hnd->stride);

    gralloc_unmap(module, handle);

    if (hnd->handle)
        ion_free(getIonFd(module), hnd->handle);
    if (hnd->handle1)
        ion_free(getIonFd(module), hnd->handle1);
    if (hnd->handle2)
        ion_free(getIonFd(module), hnd->handle2);

    return 0;
}

int gralloc_lock(gralloc_module_t const* module,
                 buffer_handle_t handle, int usage,
                 int l, int t, int w, int h,
                 void** vaddr)
{
    // this is called when a buffer is being locked for software
    // access. in thin implementation we have nothing to do since
    // not synchronization with the h/w is needed.
    // typically this is used to wait for the h/w to finish with
    // this buffer if relevant. the data cache may need to be
    // flushed or invalidated depending on the usage bits and the
    // hardware.

    int ext_size = 256;

    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;

    if (hnd->frameworkFormat == HAL_PIXEL_FORMAT_YCbCr_420_888) {
        ALOGE("gralloc_lock can't be used with YCbCr_420_888 format");
        return -EINVAL;
    }

#ifdef GRALLOC_RANGE_FLUSH
    if(usage & GRALLOC_USAGE_SW_WRITE_MASK)
    {
        hnd->lock_usage = GRALLOC_USAGE_SW_WRITE_RARELY;
        hnd->lock_offset = t * hnd->stride;
        hnd->lock_len = h * hnd->stride;
    }
    else
    {
        hnd->lock_usage = 0;
        hnd->lock_offset = 0;
        hnd->lock_len = 0;
    }
#endif

    if (!hnd->base)
        gralloc_map(module, hnd);
    *vaddr = INT_TO_PTR(hnd->base);

    if (hnd->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN)
        vaddr[1] = vaddr[0] + (hnd->stride * hnd->vstride) + ext_size;
    else if (hnd->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B)
        vaddr[1] = vaddr[0] + (hnd->stride * hnd->vstride) + ext_size + (ALIGN(hnd->width / 4, 16) * hnd->vstride) + 64;

#ifdef USES_EXYNOS_CRC_BUFFER_ALLOC
    if (!gralloc_crc_allocation_check(hnd->format, hnd->width, hnd->height, hnd->flags))
#endif /* USES_EXYNOS_CRC_BUFFER_ALLOC */
    {
        if (hnd->fd1 >= 0)
            vaddr[1] = INT_TO_PTR(hnd->base1);
        if (hnd->fd2 >= 0)
            vaddr[2] = INT_TO_PTR(hnd->base2);
    }

    return 0;
}

int gralloc_unlock(gralloc_module_t const* module,
                   buffer_handle_t handle)
{
    // we're done with a software buffer. nothing to do in this
    // implementation. typically this is used to flush the data cache.
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;

    if (!((hnd->flags & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN))
        return 0;

#ifdef GRALLOC_RANGE_FLUSH
    if(hnd->lock_usage & GRALLOC_USAGE_SW_WRITE_MASK)
    {
        if(((hnd->format == HAL_PIXEL_FORMAT_RGBA_8888)
            || (hnd->format == HAL_PIXEL_FORMAT_RGBX_8888)) && (hnd->lock_offset != 0))
            ion_sync_fd_partial(getIonFd(module), hnd->fd, hnd->lock_offset * 4, hnd->lock_len * 4);
        else
            ion_sync_fd(getIonFd(module), hnd->fd);

#ifdef USES_EXYNOS_CRC_BUFFER_ALLOC
        if (!gralloc_crc_allocation_check(hnd->format, hnd->width, hnd->height, hnd->flags))
#endif /* USES_EXYNOS_CRC_BUFFER_ALLOC */
        {
            if (hnd->fd1 >= 0)
                ion_sync_fd(getIonFd(module), hnd->fd1);
            if (hnd->fd2 >= 0)
                ion_sync_fd(getIonFd(module), hnd->fd2);
        }

        hnd->lock_usage = 0;
    }
#else
    ion_sync_fd(getIonFd(module), hnd->fd);
#ifdef USES_EXYNOS_CRC_BUFFER_ALLOC
    if (!gralloc_crc_allocation_check(hnd->format, hnd->width, hnd->height, hnd->flags))
#endif /* USES_EXYNOS_CRC_BUFFER_ALLOC */
    {
        if (hnd->fd1 >= 0)
            ion_sync_fd(getIonFd(module), hnd->fd1);
        if (hnd->fd2 >= 0)
            ion_sync_fd(getIonFd(module), hnd->fd2);
    }
#endif

    return 0;
}

int gralloc_lock_ycbcr(gralloc_module_t const* module,
                        buffer_handle_t handle, int usage,
                        int l, int t, int w, int h,
                        android_ycbcr *ycbcr)
{
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    if (!ycbcr) {
        ALOGE("gralloc_lock_ycbcr got NULL ycbcr struct");
        return -EINVAL;
    }

    private_handle_t* hnd = (private_handle_t*)handle;

    // Calculate offsets to underlying YUV data
    size_t yStride;
    size_t cStride;
    size_t yOffset;
    size_t uOffset;
    size_t vOffset;
    size_t cStep;
    switch (hnd->format) {
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        yStride = cStride = hnd->width;
        yOffset = 0;
        vOffset = yStride * hnd->height;
        uOffset = vOffset + 1;
        cStep = 2;
        ycbcr->y = (void *)(((unsigned long)hnd->base) + yOffset);
        ycbcr->cb = (void *)(((unsigned long)hnd->base) + uOffset);
        ycbcr->cr = (void *)(((unsigned long)hnd->base) + vOffset);
        break;
    case HAL_PIXEL_FORMAT_YV12:
        yStride = ALIGN(hnd->width, 16);
        cStride = ALIGN(yStride/2, 16);
        ycbcr->y  = (void*)((unsigned long)hnd->base);
        ycbcr->cr = (void*)(((unsigned long)hnd->base) + yStride * hnd->height);
        ycbcr->cb = (void*)(((unsigned long)hnd->base) + yStride * hnd->height +
                    cStride * hnd->height/2);
        cStep = 1;
        break;
    default:
        ALOGE("gralloc_lock_ycbcr unexpected internal format %x",
                hnd->format);
        return -EINVAL;
    }

    ycbcr->ystride = yStride;
    ycbcr->cstride = cStride;
    ycbcr->chroma_step = cStep;

    // Zero out reserved fields
    memset(ycbcr->reserved, 0, sizeof(ycbcr->reserved));

    ALOGD("gralloc_lock_ycbcr success. format : %x, usage: %x, ycbcr.y: %p, .cb: %p, .cr: %p, "
            ".ystride: %d , .cstride: %d, .chroma_step: %d", hnd->format, usage,
            ycbcr->y, ycbcr->cb, ycbcr->cr, ycbcr->ystride, ycbcr->cstride,
            ycbcr->chroma_step);

    return 0;
}
