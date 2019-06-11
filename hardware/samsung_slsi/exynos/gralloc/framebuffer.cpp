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

#include <sys/mman.h>

#include <dlfcn.h>

#include <cutils/ashmem.h>
#include <cutils/log.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#include <utils/Vector.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#if HAVE_ANDROID_OS
#include <linux/fb.h>
#endif

#include "gralloc_priv.h"
#include "gr.h"

#include "format_chooser.h"

/*****************************************************************************/

// numbers of buffers for page flipping
#define NUM_BUFFERS 2
#define HWC_EXIST 0

struct hwc_callback_entry
{
    void (*callback)(void *, private_handle_t *);
    void *data;
};

#if HWC_EXIST
typedef android::Vector<struct hwc_callback_entry> hwc_callback_queue_t;
#endif

struct fb_context_t {
    framebuffer_device_t  device;
};

/*****************************************************************************/

static int fb_setSwapInterval(struct framebuffer_device_t* dev,
                              int interval)
{
    fb_context_t* ctx = (fb_context_t*)dev;
    if (interval < dev->minSwapInterval || interval > dev->maxSwapInterval)
        return -EINVAL;
    // FIXME: implement fb_setSwapInterval
    return 0;
}

static int fb_post(struct framebuffer_device_t* dev, buffer_handle_t buffer)
{
    if (private_handle_t::validate(buffer) < 0)
        return -EINVAL;

    private_handle_t const* hnd = reinterpret_cast<private_handle_t const*>(buffer);
    private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
#if HWC_EXIST
    hwc_callback_queue_t *queue = reinterpret_cast<hwc_callback_queue_t *>(m->queue);
    pthread_mutex_lock(&m->queue_lock);
    if(queue->isEmpty())
        pthread_mutex_unlock(&m->queue_lock);
    else {
        private_handle_t *hnd = private_handle_t::dynamicCast(buffer);
        struct hwc_callback_entry entry = queue->top();
        queue->pop();
        pthread_mutex_unlock(&m->queue_lock);
        entry.callback(entry.data, hnd);
    }
#else
    // If we can't do the page_flip, just copy the buffer to the front
    // FIXME: use copybit HAL instead of memcpy
    void* fb_vaddr;
    void* buffer_vaddr;

    m->base.lock(&m->base, m->framebuffer,
            GRALLOC_USAGE_SW_WRITE_RARELY,
            0, 0, m->info.xres, m->info.yres,
            &fb_vaddr);

    m->base.lock(&m->base, buffer,
            GRALLOC_USAGE_SW_READ_RARELY,
            0, 0, m->info.xres, m->info.yres,
            &buffer_vaddr);

    memcpy(fb_vaddr, buffer_vaddr, m->finfo.line_length * m->info.yres);

    m->base.unlock(&m->base, buffer);
    m->base.unlock(&m->base, m->framebuffer);
#endif
    return 0;
}

/*****************************************************************************/

static int fb_close(struct hw_device_t *dev)
{
    fb_context_t* ctx = (fb_context_t*)dev;
    if (ctx) {
        free(ctx);
    }
    return 0;
}

int init_fb(struct private_module_t* module)
{
    char const * const device_template[] = {
        "/dev/graphics/fb%u",
        "/dev/fb%u",
        NULL
    };

    int fd = -1;
    int i = 0;

    fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        ALOGE("/dev/graphics/fb0 Open fail");
        return -errno;
    }

    struct fb_fix_screeninfo finfo;
    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        ALOGE("Fail to get FB Screen Info");
        close(fd);
        return -errno;
    }

    struct fb_var_screeninfo info;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1) {
        ALOGE("First, Fail to get FB VScreen Info");
        close(fd);
        return -errno;
    }

    int refreshRate = 1000000000000000LLU /
        (
         uint64_t( info.upper_margin + info.lower_margin + info.yres )
         * ( info.left_margin  + info.right_margin + info.xres )
         * info.pixclock
        );

    if (refreshRate == 0)
        refreshRate = 60*1000;  /* 60 Hz */

    float xdpi = (info.xres * 25.4f) / info.width;
    float ydpi = (info.yres * 25.4f) / info.height;
    float fps  = refreshRate / 1000.0f;

    ALOGI("using (id=%s)\n"
          "xres         = %d px\n"
          "yres         = %d px\n"
          "width        = %d mm (%f dpi)\n"
          "height       = %d mm (%f dpi)\n"
          "refresh rate = %.2f Hz\n",
          finfo.id, info.xres, info.yres, info.width,  xdpi, info.height, ydpi,
          fps);

    module->xres = info.xres;
    module->yres = info.yres;
    module->line_length = info.xres;
    module->xdpi = xdpi;
    module->ydpi = ydpi;
    module->fps = fps;
    module->info = info;
    module->finfo = finfo;

#if !HWC_EXIST
    size_t fbSize = roundUpToPageSize(finfo.line_length * info.yres_virtual);
    module->framebuffer = new private_handle_t(dup(fd), fbSize, 0);

    void* vaddr = mmap(0, fbSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (vaddr == MAP_FAILED) {
        ALOGE("Error mapping the framebuffer (%s)", strerror(errno));
        close(fd);
        return -errno;
    }
    module->framebuffer->base = (size_t)vaddr;
    memset(vaddr, 0, fbSize);
#endif

    close(fd);

    return 0;
}

int fb_device_open(hw_module_t const* module, const char* name,
                   hw_device_t** device)
{
    int status = -EINVAL;
#ifdef GRALLOC_16_BITS
    int bits_per_pixel = 16;
    int format = HAL_PIXEL_FORMAT_RGB_565;
#else
    int bits_per_pixel = 32;
    int format = HAL_PIXEL_FORMAT_RGBA_8888;
#endif

    alloc_device_t* gralloc_device;
    status = gralloc_open(module, &gralloc_device);
    if (status < 0) {
        ALOGE("Fail to Open gralloc device");
        return status;
    }

    framebuffer_device_t *dev = (framebuffer_device_t *)malloc(sizeof(framebuffer_device_t));
    if (dev == NULL) {
        ALOGE("Failed to allocate memory for dev");
        gralloc_close(gralloc_device);
        return status;
    }

    private_module_t* m = (private_module_t*)module;
    status = init_fb(m);
    if (status < 0) {
        ALOGE("Fail to init framebuffer");
        free(dev);
        gralloc_close(gralloc_device);
        return status;
    }

    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = fb_close;
    dev->setSwapInterval = 0;
    dev->post = fb_post;
    dev->setUpdateRect = 0;
    dev->compositionComplete = 0;
#if HWC_EXIST
    m->queue = new hwc_callback_queue_t;
#endif
    pthread_mutex_init(&m->queue_lock, NULL);

    int stride = m->line_length / (bits_per_pixel >> 3);
    const_cast<uint32_t&>(dev->flags) = 0;
    const_cast<uint32_t&>(dev->width) = m->xres;
    const_cast<uint32_t&>(dev->height) = m->yres;
    const_cast<int&>(dev->stride) = stride;
    const_cast<int&>(dev->format) = format;
    const_cast<float&>(dev->xdpi) = m->xdpi;
    const_cast<float&>(dev->ydpi) = m->ydpi;
    const_cast<float&>(dev->fps) = m->fps;
    const_cast<int&>(dev->minSwapInterval) = 1;
    const_cast<int&>(dev->maxSwapInterval) = 1;
    *device = &dev->common;
    status = 0;

    return status;
}
