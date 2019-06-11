/*
 * Copyright (C) 2012 The Android Open Source Project
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
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/Trace.h>

#include "ExynosHWC.h"
#include "ExynosOverlayDisplay.h"
#include "ExynosPrimaryDisplay.h"





int exynos5_prepare(hwc_composer_device_1_t *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    ATRACE_CALL();
    ALOGD("#### exynos5_prepare");
    if (!numDisplays || !displays)
        return 0;

    exynos5_hwc_composer_device_1_t *pdev =
            (exynos5_hwc_composer_device_1_t *)dev;
    hwc_display_contents_1_t *fimd_contents = displays[HWC_DISPLAY_PRIMARY];

    pdev->updateCallCnt++;
    pdev->update_event_cnt++;
    pdev->LastUpdateTimeStamp = systemTime(SYSTEM_TIME_MONOTONIC);

    if (fimd_contents) {
        int err = pdev->primaryDisplay->prepare(fimd_contents);
        if (err)
            return err;
    }

    return 0;
}

int exynos5_set(struct hwc_composer_device_1 *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    ATRACE_CALL();
    ALOGD("#### exynos5_set");
    if (!numDisplays || !displays)
        return 0;

    exynos5_hwc_composer_device_1_t *pdev =
            (exynos5_hwc_composer_device_1_t *)dev;
    hwc_display_contents_1_t *fimd_contents = displays[HWC_DISPLAY_PRIMARY];
    int fimd_err = 0;

    if (fimd_contents) {
        fimd_err = pdev->primaryDisplay->set(fimd_contents);
    }


    return fimd_err;

}

void exynos5_registerProcs(struct hwc_composer_device_1* dev,
        hwc_procs_t const* procs)
{
    struct exynos5_hwc_composer_device_1_t* pdev =
            (struct exynos5_hwc_composer_device_1_t*)dev;
    pdev->procs = procs;
}

int exynos5_query(struct hwc_composer_device_1* dev, int what, int *value)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;

    switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // we support the background layer
        value[0] = 1;
        break;
    case HWC_VSYNC_PERIOD:
        // vsync period in nanosecond
        value[0] = pdev->primaryDisplay->mVsyncPeriod;
        break;
    default:
        // unsupported query
        return -EINVAL;
    }
    return 0;
}

int exynos5_eventControl(struct hwc_composer_device_1 *dev, int __UNUSED__ dpy,
        int event, int enabled)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;

    switch (event) {
    case HWC_EVENT_VSYNC:
        __u32 val = !!enabled;
        pdev->VsyncInterruptStatus = val;
        int err = ioctl(pdev->primaryDisplay->mDisplayFd, S3CFB_SET_VSYNC_INT, &val);
        if (err < 0) {
            ALOGE("vsync ioctl failed");
            return -errno;
        }
        return 0;
    }

    return -EINVAL;
}


void handle_vsync_event(struct exynos5_hwc_composer_device_1_t *pdev)
{
    if (!pdev->procs)
        return;

    int err = lseek(pdev->vsync_fd, 0, SEEK_SET);
    if (err < 0) {
        ALOGE("error seeking to vsync timestamp: %s", strerror(errno));
        return;
    }

    char buf[4096];
    err = read(pdev->vsync_fd, buf, sizeof(buf));
    if (err < 0) {
        ALOGE("error reading vsync timestamp: %s", strerror(errno));
        return;
    }
    buf[sizeof(buf) - 1] = '\0';

    errno = 0;
    uint64_t timestamp = strtoull(buf, NULL, 0);
    if (!errno)
        pdev->procs->vsync(pdev->procs, 0, timestamp);
}

void *hwc_vsync_thread(void *data)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)data;
    char uevent_desc[4096];
    memset(uevent_desc, 0, sizeof(uevent_desc));

    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    uevent_init();

    char temp[4096];
    int err = read(pdev->vsync_fd, temp, sizeof(temp));
    if (err < 0) {
        ALOGE("error reading vsync timestamp: %s", strerror(errno));
        return NULL;
    }

    struct pollfd fds[2];
    fds[0].fd = pdev->vsync_fd;
    fds[0].events = POLLPRI;
    fds[1].fd = uevent_get_fd();
    fds[1].events = POLLIN;

    while (true) {

        int err = poll(fds, 2, -1);

        if (err > 0) {
            if (fds[0].revents & POLLPRI) {
                handle_vsync_event(pdev);
            }
            else if (fds[1].revents & POLLIN) {
                int len = uevent_next_event(uevent_desc,
                        sizeof(uevent_desc) - 2);

            }
        }
        else if (err == -1) {
            if (errno == EINTR)
                break;
            ALOGE("error in vsync thread: %s", strerror(errno));
        }
    }

    return NULL;
}

int exynos5_blank(struct hwc_composer_device_1 *dev, int disp, int blank)
{
    ATRACE_CALL();
    ALOGD("#### exynos5_blank");
    int fence = 0;
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;

    ALOGI("%s:: disp(%d), blank(%d)", __func__, disp, blank);
    switch (disp) {
    case HWC_DISPLAY_PRIMARY: {
        int fb_blank = blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK;
        if (fb_blank == FB_BLANK_POWERDOWN) {
            int fence = pdev->primaryDisplay->clearDisplay();
            if (fence < 0) {
                HLOGE("error clearing primary display");
            } else {
                close(fence);
            }
        }
        pdev->primaryDisplay->mBlanked = !!blank;

        int err = ioctl(pdev->primaryDisplay->mDisplayFd, FBIOBLANK, fb_blank);
        if (err < 0) {
            if (errno == EBUSY)
                ALOGI("%sblank ioctl failed (display already %sblanked)",
                        blank ? "" : "un", blank ? "" : "un");
            else
                ALOGE("%sblank ioctl failed: %s", blank ? "" : "un",
                        strerror(errno));
            return -errno;
        }
        break;
    }
    default:
        return -EINVAL;

    }

    return 0;
}

void exynos5_dump(hwc_composer_device_1* dev, char *buff, int buff_len)
{
    return;
}

int exynos5_getDisplayConfigs(struct hwc_composer_device_1 *dev,
        int disp, uint32_t *configs, size_t *numConfigs)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
               (struct exynos5_hwc_composer_device_1_t *)dev;

    if (*numConfigs == 0)
        return 0;

    if (disp == HWC_DISPLAY_PRIMARY) {
        configs[0] = 0;
        *numConfigs = 1;
        return 0;
    }

    return -EINVAL;
}


int exynos5_getDisplayAttributes(struct hwc_composer_device_1 *dev,
        int disp, uint32_t __UNUSED__ config, const uint32_t *attributes, int32_t *values)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
                   (struct exynos5_hwc_composer_device_1_t *)dev;

    for (int i = 0; attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE; i++) {
        if (disp == HWC_DISPLAY_PRIMARY)
            values[i] = pdev->primaryDisplay->getDisplayAttributes(attributes[i]);
        else {
            ALOGE("unknown display type %u", disp);
            return -EINVAL;
        }
    }

    return 0;
}

int exynos5_close(hw_device_t* device);

int exynos5_open(const struct hw_module_t *module, const char *name,
        struct hw_device_t **device)
{
    int ret = 0;
    int refreshRate;
    int sw_fd;

    ALOGD("#### exynos5_open START");
    if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
        return -EINVAL;
    }

    struct exynos5_hwc_composer_device_1_t *dev;
    dev = (struct exynos5_hwc_composer_device_1_t *)malloc(sizeof(*dev));
    memset(dev, 0, sizeof(*dev));

    dev->primaryDisplay = new ExynosPrimaryDisplay(NUM_GSC_UNITS, dev);

    dev->primaryDisplay->mDisplayFd = open("/dev/graphics/fb0", O_RDWR);
    if (dev->primaryDisplay->mDisplayFd < 0) {
        ALOGE("failed to open framebuffer");
        ret = dev->primaryDisplay->mDisplayFd;
        goto err_open_fb;
    }

    struct fb_var_screeninfo info;
    if (ioctl(dev->primaryDisplay->mDisplayFd, FBIOGET_VSCREENINFO, &info) == -1) {
        ALOGE("FBIOGET_VSCREENINFO ioctl failed: %s", strerror(errno));
        ret = -errno;
        goto err_ioctl;
    }

    if (info.reserved[0] == 0 && info.reserved[1] == 0) {
        /* save physical lcd width, height to reserved[] */
        info.reserved[0] = info.xres;
        info.reserved[1] = info.yres;

        if (ioctl(dev->primaryDisplay->mDisplayFd, FBIOPUT_VSCREENINFO, &info) == -1) {
            ALOGE("FBIOPUT_VSCREENINFO ioctl failed: %s", strerror(errno));
            ret = -errno;
            goto err_ioctl;
        }
    }


    /* restore physical lcd width, height from reserved[] */
    int lcd_xres, lcd_yres;
    lcd_xres = info.reserved[0];
    lcd_yres = info.reserved[1];

    refreshRate = 1000000000000LLU /
        (
         uint64_t( info.upper_margin + info.lower_margin + lcd_yres )
         * ( info.left_margin  + info.right_margin + lcd_xres )
         * info.pixclock
        );

    if (refreshRate == 0) {
        ALOGW("invalid refresh rate, assuming 60 Hz");
        refreshRate = 60;
    }

    dev->primaryDisplay->mXres = lcd_xres;
    dev->primaryDisplay->mYres = lcd_yres;
    dev->primaryDisplay->mXdpi = 1000 * (lcd_xres * 25.4f) / info.width;
    dev->primaryDisplay->mYdpi = 1000 * (lcd_yres * 25.4f) / info.height;
    dev->primaryDisplay->mVsyncPeriod  = 1000000000 / refreshRate;

    ALOGD("using\n"
          "xres         = %d px\n"
          "yres         = %d px\n"
          "width        = %d mm (%f dpi)\n"
          "height       = %d mm (%f dpi)\n"
          "refresh rate = %d Hz\n",
          dev->primaryDisplay->mXres, dev->primaryDisplay->mYres, info.width, dev->primaryDisplay->mXdpi / 1000.0,
          info.height, dev->primaryDisplay->mYdpi / 1000.0, refreshRate);

    char devname[MAX_DEV_NAME + 1];
    devname[MAX_DEV_NAME] = '\0';

    strncpy(devname, VSYNC_DEV_PREFIX, MAX_DEV_NAME);
    strlcat(devname, VSYNC_DEV_NAME, MAX_DEV_NAME);

    dev->vsync_fd = open(devname, O_RDONLY);
    if (dev->vsync_fd < 0) {
        ALOGI("Failed to open vsync attribute at %s", devname);
        devname[strlen(VSYNC_DEV_PREFIX)] = '\0';
        strlcat(devname, VSYNC_DEV_MIDDLE, MAX_DEV_NAME);
        strlcat(devname, VSYNC_DEV_NAME, MAX_DEV_NAME);
        ALOGI("Retrying with %s", devname);
        dev->vsync_fd = open(devname, O_RDONLY);
    }


    if (dev->vsync_fd < 0) {
        ALOGE("failed to open vsync attribute");
        ret = dev->vsync_fd;
        goto err_hdmi_open;
    } else {
        struct stat st;
        if (fstat(dev->vsync_fd, &st) < 0) {
            ALOGE("Failed to stat vsync node at %s", devname);
            goto err_vsync_stat;
        }

        if (!S_ISREG(st.st_mode)) {
            ALOGE("vsync node at %s should be a regualar file", devname);
            goto err_vsync_stat;
        }
    }

    dev->base.common.tag = HARDWARE_DEVICE_TAG;
    dev->base.common.version = HWC_DEVICE_API_VERSION_1_3;
    dev->base.common.module = const_cast<hw_module_t *>(module);
    dev->base.common.close = exynos5_close;

    dev->base.prepare = exynos5_prepare;
    dev->base.set = exynos5_set;
    dev->base.eventControl = exynos5_eventControl;
    dev->base.blank = exynos5_blank;
    dev->base.query = exynos5_query;
    dev->base.registerProcs = exynos5_registerProcs;
    dev->base.dump = exynos5_dump;
    dev->base.getDisplayConfigs = exynos5_getDisplayConfigs;
    dev->base.getDisplayAttributes = exynos5_getDisplayAttributes;

    *device = &dev->base.common;

#ifdef IP_SERVICE
    android::ExynosIPService *mIPService;
    mIPService = android::ExynosIPService::getExynosIPService();
    ret = mIPService->createServiceLocked();
    if (ret < 0)
        goto err_vsync;
#endif


    ret = pthread_create(&dev->vsync_thread, NULL, hwc_vsync_thread, dev);
    if (ret) {
        ALOGE("failed to start vsync thread: %s", strerror(ret));
        ret = -ret;
        goto err_vsync;
    }

    ALOGD("#### exynos5_open END with success");
    return 0;

err_vsync:
err_vsync_stat:
    close(dev->vsync_fd);
err_hdmi_open:
err_ioctl:
    close(dev->primaryDisplay->mDisplayFd);
err_open_fb:
err_get_module:
    free(dev);
    ALOGD("#### exynos5_open END with error");
    return ret;
}

int exynos5_close(hw_device_t *device)
{
    struct exynos5_hwc_composer_device_1_t *dev =
            (struct exynos5_hwc_composer_device_1_t *)device;
    pthread_kill(dev->vsync_thread, SIGTERM);
    pthread_join(dev->vsync_thread, NULL);
    close(dev->vsync_fd);

    return 0;
}

static struct hw_module_methods_t exynos5_hwc_module_methods = {
    open: exynos5_open,
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        module_api_version: HWC_MODULE_API_VERSION_0_1,
        hal_api_version: HARDWARE_HAL_API_VERSION,
        id: HWC_HARDWARE_MODULE_ID,
        name: "Samsung exynos5 hwcomposer module",
        author: "Samsung LSI",
        methods: &exynos5_hwc_module_methods,
        dso: 0,
        reserved: {0},
    }
};
