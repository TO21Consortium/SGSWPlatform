#include "ExynosHWC.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosOverlayDisplay.h"
#include "ExynosExternalDisplay.h"
#include <errno.h>

#if defined(USE_DV_TIMINGS)
extern struct v4l2_dv_timings dv_timings[];

bool is_same_dv_timings(const struct v4l2_dv_timings *t1,
		const struct v4l2_dv_timings *t2)
{
    if (t1->type == t2->type &&
            t1->bt.width == t2->bt.width &&
            t1->bt.height == t2->bt.height &&
            t1->bt.interlaced == t2->bt.interlaced &&
            t1->bt.polarities == t2->bt.polarities &&
            t1->bt.pixelclock == t2->bt.pixelclock &&
            t1->bt.hfrontporch == t2->bt.hfrontporch &&
            t1->bt.vfrontporch == t2->bt.vfrontporch &&
            t1->bt.vsync == t2->bt.vsync &&
            t1->bt.vbackporch == t2->bt.vbackporch &&
            (!t1->bt.interlaced ||
             (t1->bt.il_vfrontporch == t2->bt.il_vfrontporch &&
              t1->bt.il_vsync == t2->bt.il_vsync &&
              t1->bt.il_vbackporch == t2->bt.il_vbackporch)))
        return true;
    return false;
}
#endif
int ExynosExternalDisplay::getDVTimingsIndex(int preset)
{
    for (int i = 0; i < SUPPORTED_DV_TIMINGS_NUM; i++) {
        if (preset == preset_index_mappings[i].preset)
            return preset_index_mappings[i].dv_timings_index;
    }
    return -1;
}

inline bool hdmi_src_cfg_changed(exynos_mpp_img &c1, exynos_mpp_img &c2)
{
    return c1.format != c2.format ||
            c1.rot != c2.rot ||
            c1.cacheable != c2.cacheable ||
            c1.drmMode != c2.drmMode ||
            c1.fw != c2.fw ||
            c1.fh != c2.fh;
}

ExynosExternalDisplay::ExynosExternalDisplay(struct exynos5_hwc_composer_device_1_t *pdev)
    :   ExynosDisplay(1),
        mMixer(-1),
        mEnabled(false),
        mBlanked(false),
        mIsFbLayer(false),
        mIsVideoLayer(false),
        mFbStarted(false),
        mVideoStarted(false),
        mHasFbComposition(false),
        mHasSkipLayer(false),
        mUiIndex(0),
        mVideoIndex(1),
        mVirtualOverlayFlag(0)
{
    mIsCameraStarted = false;
    mFBT_Transform = 0;
    mUseProtectedLayer = false;
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
    mUseScreenshootLayer = false;
    mLocalExternalDisplayPause = false;
#endif
    mNumMPPs = 1;
    this->mHwc = pdev;
    mOtfMode = OTF_OFF;
    mUseSubtitles = false;

    mMPPs[0] = new ExynosMPPModule(this, HDMI_GSC_IDX);
    memset(mMixerLayers, 0, sizeof(mMixerLayers));
    memset(mLastLayerHandles, 0, sizeof(mLastLayerHandles));
}

ExynosExternalDisplay::~ExynosExternalDisplay()
{
    delete mMPPs[0];
}

int ExynosExternalDisplay::openHdmi()
{
    int ret = 0;
    int sw_fd;

    mMixer = exynos_subdev_open_devname("s5p-mixer0", O_RDWR);
    if (mMixer < 0) {
        ALOGE("failed to open hdmi mixer0 subdev");
        ret = mMixer;
        return ret;
    }

#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
    mUiIndex = 1;
    mVideoIndex = 0;
#else
    mUiIndex = 0;
    mVideoIndex = 1;
#endif

    mMixerLayers[0].id = 0;
    mMixerLayers[0].fd = open("/dev/video16", O_RDWR);
    if (mMixerLayers[0].fd < 0) {
        ALOGE("failed to open hdmi layer0 device");
        ret = mMixerLayers[0].fd;
        close(mMixer);
        return ret;
    }

    mMixerLayers[1].id = 1;
    mMixerLayers[1].fd = open("/dev/video17", O_RDWR);
    if (mMixerLayers[1].fd < 0) {
        ALOGE("failed to open hdmi layer1 device");
        ret = mMixerLayers[1].fd;
        close(mMixerLayers[0].fd);
        return ret;
    }

#if defined(VP_VIDEO)
    mMixerLayers[2].id = VIDEO_LAYER_INDEX;
    mMixerLayers[2].fd = open("/dev/video20", O_RDWR);
    if (mMixerLayers[2].fd < 0) {
        ALOGE("failed to open hdmi video layer device");
        ret = mMixerLayers[2].fd;
        close(mMixerLayers[0].fd);
        close(mMixerLayers[1].fd);
        return ret;
    }
#else
    mMixerLayers[2].id = VIDEO_LAYER_INDEX;
    mMixerLayers[2].fd = -1;
#endif
    return ret;
}

void ExynosExternalDisplay::setHdmiStatus(bool status)
{
    if (status) {
        enable();
    } else {
        disable();
    }
}

bool ExynosExternalDisplay::isPresetSupported(unsigned int preset)
{
#if defined(USE_DV_TIMINGS)
    struct v4l2_enum_dv_timings enum_timings;
    int dv_timings_index = getDVTimingsIndex(preset);
#else
    struct v4l2_dv_enum_preset enum_preset;
#endif
    bool found = false;
    int index = 0;
    int ret;

    if (preset <= V4L2_DV_INVALID || preset > V4L2_DV_1080P30_TB) {
        ALOGE("%s: invalid preset, %d", __func__, preset);
        return -1;
    }
#if defined(USE_DV_TIMINGS)
    if (dv_timings_index < 0) {
        ALOGE("%s: unsupported preset, %d", __func__, preset);
        return -1;
    }
#endif

    while (true) {
#if defined(USE_DV_TIMINGS)
        enum_timings.index = index++;
        ret = ioctl(mMixerLayers[0].fd, VIDIOC_ENUM_DV_TIMINGS, &enum_timings);

        if (ret < 0) {
            if (errno == EINVAL)
                break;
            ALOGE("%s: enum_dv_timings error, %d", __func__, errno);
            return -1;
        }

        ALOGV("%s: %d width=%d height=%d",
                __func__, enum_timings.index,
                enum_timings.timings.bt.width, enum_timings.timings.bt.height);

        if (is_same_dv_timings(&enum_timings.timings, &dv_timings[dv_timings_index])) {
            mXres  = enum_timings.timings.bt.width;
            mYres  = enum_timings.timings.bt.height;
            found = true;
            mHwc->mHdmiCurrentPreset = preset;
        }
#else
        enum_preset.index = index++;
        ret = ioctl(mMixerLayers[0].fd, VIDIOC_ENUM_DV_PRESETS, &enum_preset);

        if (ret < 0) {
            if (errno == EINVAL)
                break;
            ALOGE("%s: enum_dv_presets error, %d", __func__, errno);
            return -1;
        }

        ALOGV("%s: %d preset=%02d width=%d height=%d name=%s",
                __func__, enum_preset.index, enum_preset.preset,
                enum_preset.width, enum_preset.height, enum_preset.name);

        if (preset == enum_preset.preset) {
            mXres  = enum_preset.width;
            mYres  = enum_preset.height;
            found = true;
            mHwc->mHdmiCurrentPreset = preset;
        }
#endif
    }
    return found;
}

int ExynosExternalDisplay::getConfig()
{
#if defined(USE_DV_TIMINGS)
    struct v4l2_dv_timings timings;
    int dv_timings_index = 0;
#endif
    struct v4l2_dv_preset preset;
    int ret;

    if (!mHwc->hdmi_hpd)
        return -1;

#if defined(USE_DV_TIMINGS)
    if (ioctl(mMixerLayers[0].fd, VIDIOC_G_DV_TIMINGS, &timings) < 0) {
        ALOGE("%s: g_dv_timings error, %d", __func__, errno);
        return -1;
    }
    for (int i = 0; i < SUPPORTED_DV_TIMINGS_NUM; i++) {
        dv_timings_index = preset_index_mappings[i].dv_timings_index;
        if (is_same_dv_timings(&timings, &dv_timings[dv_timings_index])) {
            preset.preset = preset_index_mappings[i].preset;
            break;
        }
    }
#else
    if (ioctl(mMixerLayers[0].fd, VIDIOC_G_DV_PRESET, &preset) < 0) {
        ALOGE("%s: g_dv_preset error, %d", __func__, errno);
        return -1;
    }
#endif

    return isPresetSupported(preset.preset) ? 0 : -1;
}

int ExynosExternalDisplay::getDisplayConfigs(uint32_t *configs, size_t *numConfigs)
{
    *numConfigs = 1;
    configs[0] = 0;
    getConfig();
    return 0;
}

int ExynosExternalDisplay::enableLayer(hdmi_layer_t &hl)
{
    if (hl.enabled)
        return 0;

    struct v4l2_requestbuffers reqbuf;
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count  = NUM_HDMI_BUFFERS;
    reqbuf.type   = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbuf.memory = V4L2_MEMORY_DMABUF;
    if (exynos_v4l2_reqbufs(hl.fd, &reqbuf) < 0) {
        ALOGE("%s: layer%d: reqbufs failed %d", __func__, hl.id, errno);
        return -1;
    }

    if (reqbuf.count != NUM_HDMI_BUFFERS) {
        ALOGE("%s: layer%d: didn't get buffer", __func__, hl.id);
        return -1;
    }

    if (hl.id == mUiIndex) {
        if (exynos_v4l2_s_ctrl(hl.fd, V4L2_CID_TV_PIXEL_BLEND_ENABLE, 1) < 0) {
            ALOGE("%s: layer%d: PIXEL_BLEND_ENABLE failed %d", __func__,
                                                                hl.id, errno);
            return -1;
        }
    } else {
        if (exynos_v4l2_s_ctrl(hl.fd, V4L2_CID_TV_PIXEL_BLEND_ENABLE, 0) < 0) {
            ALOGE("%s: layer%d: PIXEL_BLEND_DISABLE failed %d", __func__,
                                                                 hl.id, errno);
            return -1;
        }
    }

    ALOGV("%s: layer%d enabled", __func__, hl.id);
    hl.enabled = true;
    return 0;
}

void ExynosExternalDisplay::disableLayer(hdmi_layer_t &hl)
{
    if (!hl.enabled)
        return;

    if (hl.streaming) {
        if (exynos_v4l2_streamoff(hl.fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) < 0)
            ALOGE("%s: layer%d: streamoff failed %d", __func__, hl.id, errno);
        hl.streaming = false;
    }

    struct v4l2_requestbuffers reqbuf;
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.type   = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbuf.memory = V4L2_MEMORY_DMABUF;
    if (exynos_v4l2_reqbufs(hl.fd, &reqbuf) < 0)
        ALOGE("%s: layer%d: reqbufs failed %d", __func__, hl.id, errno);

    memset(&hl.cfg, 0, sizeof(hl.cfg));
    hl.current_buf = 0;
    hl.queued_buf = 0;
    hl.enabled = false;

    ALOGV("%s: layer%d disabled", __func__, hl.id);
}

int ExynosExternalDisplay::enable()
{
    if (mEnabled)
        return 0;

    if (mBlanked)
        return 0;

    struct v4l2_subdev_format sd_fmt;
    memset(&sd_fmt, 0, sizeof(sd_fmt));
    sd_fmt.pad   = MIXER_G0_SUBDEV_PAD_SINK;
    sd_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    sd_fmt.format.width  = mXres;
    sd_fmt.format.height = mYres;
    sd_fmt.format.code   = V4L2_MBUS_FMT_XRGB8888_4X8_LE;
    if (exynos_subdev_s_fmt(mMixer, &sd_fmt) < 0) {
        ALOGE("%s: s_fmt failed pad=%d", __func__, sd_fmt.pad);
        return -1;
    }

    struct v4l2_subdev_crop sd_crop;
    memset(&sd_crop, 0, sizeof(sd_crop));
    sd_crop.pad   = MIXER_G0_SUBDEV_PAD_SINK;
    sd_crop.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    sd_crop.rect.width  = mXres;
    sd_crop.rect.height = mYres;
    if (exynos_subdev_s_crop(mMixer, &sd_crop) < 0) {
        ALOGE("%s: s_crop failed pad=%d", __func__, sd_crop.pad);
        return -1;
    }

    memset(&sd_fmt, 0, sizeof(sd_fmt));
    sd_fmt.pad   = MIXER_G0_SUBDEV_PAD_SOURCE;
    sd_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    sd_fmt.format.width  = mXres;
    sd_fmt.format.height = mYres;
    sd_fmt.format.code   = V4L2_MBUS_FMT_XRGB8888_4X8_LE;
    if (exynos_subdev_s_fmt(mMixer, &sd_fmt) < 0) {
        ALOGE("%s: s_fmt failed pad=%d", __func__, sd_fmt.pad);
        return -1;
    }

    memset(&sd_crop, 0, sizeof(sd_crop));
    sd_crop.pad   = MIXER_G0_SUBDEV_PAD_SOURCE;
    sd_crop.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    sd_crop.rect.width  = mXres;
    sd_crop.rect.height = mYres;
    if (exynos_subdev_s_crop(mMixer, &sd_crop) < 0) {
        ALOGE("%s: s_crop failed pad=%d", __func__, sd_crop.pad);
        return -1;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("persist.hdmi.hdcp_enabled", value, "1");
    int hdcp_enabled = atoi(value);

    if (exynos_v4l2_s_ctrl(mMixerLayers[mUiIndex].fd, V4L2_CID_TV_HDCP_ENABLE,
                           hdcp_enabled) < 0)
        ALOGE("%s: s_ctrl(CID_TV_HDCP_ENABLE) failed %d", __func__, errno);

    /* "2" is RGB601_16_235 */
    property_get("persist.hdmi.color_range", value, "2");
    int color_range = atoi(value);

    if (exynos_v4l2_s_ctrl(mMixerLayers[mUiIndex].fd, V4L2_CID_TV_SET_COLOR_RANGE,
                           color_range) < 0)
        ALOGE("%s: s_ctrl(CID_TV_COLOR_RANGE) failed %d", __func__, errno);

    enableLayer(mMixerLayers[mUiIndex]);

    mEnabled = true;
    return 0;
}

void ExynosExternalDisplay::disable()
{
    if (!mEnabled)
        return;

    disableLayer(mMixerLayers[0]);
    disableLayer(mMixerLayers[1]);
#if defined(VP_VIDEO)
    disableLayer(mMixerLayers[2]);
#endif

    blank();

    mMPPs[0]->cleanupM2M();
    mEnabled = false;
}

int ExynosExternalDisplay::output(hdmi_layer_t &hl,
                       hwc_layer_1_t &layer,
                       private_handle_t *h,
                       int acquireFenceFd,
                       int *releaseFenceFd)
{
    int ret = 0;

    exynos_mpp_img src_cfg;
    memset(&src_cfg, 0, sizeof(src_cfg));

    if (hl.id == VIDEO_LAYER_INDEX) {
        src_cfg.x = layer.displayFrame.left;
        src_cfg.y = layer.displayFrame.top;
        src_cfg.w = WIDTH(layer.displayFrame);
        src_cfg.h = HEIGHT(layer.displayFrame);
#ifdef USES_DRM_SETTING_BY_DECON
        src_cfg.drmMode = !!(getDrmMode(h->flags) == SECURE_DRM);
#endif
        if (isVPSupported(layer, h->format)) {
            src_cfg.fw = h->stride;
            src_cfg.fh = h->vstride;
        } else {
            src_cfg.fw = ALIGN(mXres, 16);
            src_cfg.fh = ALIGN(mYres, 16);
        }
    }

    exynos_mpp_img cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.x = layer.displayFrame.left;
    cfg.y = layer.displayFrame.top;
    cfg.w = WIDTH(layer.displayFrame);
    cfg.h = HEIGHT(layer.displayFrame);
#ifdef USES_DRM_SETTING_BY_DECON
    cfg.drmMode = !!(getDrmMode(h->flags) == SECURE_DRM);
#endif

    if ((signed int)cfg.x < 0 || (signed int)cfg.y < 0 || (cfg.w > (uint32_t)mXres) || (cfg.h > (uint32_t)mYres)) {
        *releaseFenceFd = -1;
        if (acquireFenceFd >= 0)
            close(acquireFenceFd);
        return ret;
    }

    if ((hl.id == VIDEO_LAYER_INDEX && ExynosMPP::isSrcConfigChanged(hl.cfg, src_cfg)) ||
        (hl.id != VIDEO_LAYER_INDEX && ExynosMPP::isSrcConfigChanged(hl.cfg, cfg)) || mFbStarted || mVideoStarted) {
        struct v4l2_subdev_crop sd_crop;
        memset(&sd_crop, 0, sizeof(sd_crop));
        if (hl.id == 0)
            sd_crop.pad   = MIXER_G0_SUBDEV_PAD_SOURCE;
        else if (hl.id == 1)
            sd_crop.pad   = MIXER_G1_SUBDEV_PAD_SOURCE;

        if ((hl.id == VIDEO_LAYER_INDEX && hdmi_src_cfg_changed(hl.cfg, src_cfg)) ||
            (hl.id != VIDEO_LAYER_INDEX && hdmi_src_cfg_changed(hl.cfg, cfg)) || mFbStarted || mVideoStarted) {
            disableLayer(hl);

            /* Set source image size */
            struct v4l2_format fmt;
            memset(&fmt, 0, sizeof(fmt));
            fmt.type  = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

            if (hl.id == VIDEO_LAYER_INDEX) {
                if (isVPSupported(layer, h->format)) {
                    fmt.fmt.pix_mp.width   = h->stride;
                    fmt.fmt.pix_mp.height  = h->vstride;
                } else {
                    fmt.fmt.pix_mp.width   = ALIGN(mXres, 16);
                    fmt.fmt.pix_mp.height  = ALIGN(mYres, 16);
                }
            } else if (hl.id == mVideoIndex) {
                fmt.fmt.pix_mp.width   = ALIGN(mXres, 16);
                fmt.fmt.pix_mp.height  = ALIGN(mYres, 16);
            } else {
                fmt.fmt.pix_mp.width   = h->stride;
                fmt.fmt.pix_mp.height  = h->vstride;
            }

            if (hl.id == VIDEO_LAYER_INDEX) {
                if (isVPSupported(layer, h->format))
                    fmt.fmt.pix_mp.pixelformat = HAL_PIXEL_FORMAT_2_V4L2_PIX(h->format);
                else
                    fmt.fmt.pix_mp.pixelformat = HAL_PIXEL_FORMAT_2_V4L2_PIX(HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED);
                fmt.fmt.pix_mp.num_planes = 2;
            } else {
                fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_BGR32;
                fmt.fmt.pix_mp.num_planes  = 1;
            }
            fmt.fmt.pix_mp.field       = V4L2_FIELD_ANY;

            ret = exynos_v4l2_s_fmt(hl.fd, &fmt);
            if (ret < 0) {
                ALOGE("%s: layer%d: s_fmt failed %d", __func__, hl.id, errno);
                goto err;
            }

            if (hl.id != VIDEO_LAYER_INDEX) {
                struct v4l2_subdev_format sd_fmt;
                memset(&sd_fmt, 0, sizeof(sd_fmt));
                sd_fmt.pad   = sd_crop.pad;
                sd_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
                sd_fmt.format.width    = mXres;
                sd_fmt.format.height   = mYres;
                sd_fmt.format.code   = V4L2_MBUS_FMT_XRGB8888_4X8_LE;
                if (exynos_subdev_s_fmt(mMixer, &sd_fmt) < 0) {
                    ALOGE("%s: s_fmt failed pad=%d", __func__, sd_fmt.pad);
                    return -1;
                }
            }

            enableLayer(hl);

            mFbStarted = false;
            mVideoStarted = false;
        }

        /* Set source crop size */
        struct v4l2_crop crop;
        memset(&crop, 0, sizeof(crop));
        crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
        crop.c.width = cfg.w;
        crop.c.height = cfg.h;

        if (hl.id == VIDEO_LAYER_INDEX) {
            if (isVPSupported(layer, h->format)) {
                crop.c.left = layer.sourceCropf.left;
                crop.c.top = layer.sourceCropf.top;
                crop.c.width = WIDTH(layer.sourceCropf);
                crop.c.height = HEIGHT(layer.sourceCropf);
            } else {
                crop.c.left = 0;
                crop.c.top = 0;
            }
        } else if (hl.id == mVideoIndex) {
            crop.c.left = 0;
            crop.c.top = 0;
        } else {
            crop.c.left = cfg.x;
            crop.c.top = cfg.y;
        }

        if (exynos_v4l2_s_crop(hl.fd, &crop) < 0) {
            ALOGE("%s: v4l2_s_crop failed ", __func__);
            goto err;
        }

        /* Set destination position & scaling size */
        if (hl.id == VIDEO_LAYER_INDEX) {
            crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
            crop.c.left = cfg.x;
            crop.c.top = cfg.y;
            crop.c.width = cfg.w;
            crop.c.height = cfg.h;

            if (exynos_v4l2_s_crop(hl.fd, &crop) < 0) {
                ALOGE("%s: v4l2_s_crop (mixer output) failed ", __func__);
                goto err;
            }
        } else {
            sd_crop.which = V4L2_SUBDEV_FORMAT_ACTIVE;
            sd_crop.rect.left   = cfg.x;
            sd_crop.rect.top    = cfg.y;
            sd_crop.rect.width  = cfg.w;
            sd_crop.rect.height = cfg.h;
            if (exynos_subdev_s_crop(mMixer, &sd_crop) < 0) {
                ALOGE("%s: s_crop failed pad=%d", __func__, sd_crop.pad);
                goto err;
            }
        }

        ALOGV("HDMI layer%d configuration:", hl.id);
        dumpMPPImage(cfg);
        if (hl.id == VIDEO_LAYER_INDEX)
            hl.cfg = src_cfg;
        else
            hl.cfg = cfg;
    }

    struct v4l2_buffer buffer;
    struct v4l2_plane planes[3];

    if (hl.queued_buf == NUM_HDMI_BUFFERS) {
        memset(&buffer, 0, sizeof(buffer));
        memset(planes, 0, sizeof(planes));
        buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buffer.memory = V4L2_MEMORY_DMABUF;
        if(hl.id == VIDEO_LAYER_INDEX)
            buffer.length = 2;
        else
            buffer.length = 1;
        buffer.m.planes = planes;
        ret = exynos_v4l2_dqbuf(hl.fd, &buffer);
        if (ret < 0) {
            ALOGE("%s: layer%d: dqbuf failed %d", __func__, hl.id, errno);
            goto err;
        }
        hl.queued_buf--;
    }

    memset(&buffer, 0, sizeof(buffer));
    memset(planes, 0, sizeof(planes));
    buffer.index = hl.current_buf;
    buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buffer.memory = V4L2_MEMORY_DMABUF;
    buffer.flags = V4L2_BUF_FLAG_USE_SYNC;
    buffer.reserved = acquireFenceFd;
    buffer.m.planes = planes;


    if (hl.id == VIDEO_LAYER_INDEX) {
        buffer.length = 2;
        buffer.m.planes[0].m.fd = h->fd;
        buffer.m.planes[1].m.fd = h->fd1;
    } else {
        buffer.length = 1;
        buffer.m.planes[0].m.fd = h->fd;
    }

    if (exynos_v4l2_qbuf(hl.fd, &buffer) < 0) {
        ALOGE("%s: layer%d: qbuf failed %d", __func__, hl.id, errno);
        ret = -1;
        goto err;
    }

    if (releaseFenceFd)
        *releaseFenceFd = buffer.reserved;
    else
        close(buffer.reserved);

    hl.queued_buf++;
    hl.current_buf = (hl.current_buf + 1) % NUM_HDMI_BUFFERS;

    if (!hl.streaming) {
#ifdef USES_DRM_SETTING_BY_DECON
        if (exynos_v4l2_s_ctrl(hl.fd, V4L2_CID_CONTENT_PROTECTION, hl.cfg.drmMode) < 0)
            ALOGE("%s: s_ctrl(V4L2_CID_CONTENT_PROTECTION) failed %d", __func__, errno);;
#endif
        if (exynos_v4l2_streamon(hl.fd, (v4l2_buf_type)buffer.type) < 0) {
            ALOGE("%s: layer%d: streamon failed %d", __func__, hl.id, errno);
            ret = -1;
            goto err;
        }
        hl.streaming = true;
    }

err:
    if (acquireFenceFd >= 0)
        close(acquireFenceFd);

    return ret;
}

void ExynosExternalDisplay::skipStaticLayers(hwc_display_contents_1_t *contents, int ovly_idx)
{
    static int init_flag = 0;
    mVirtualOverlayFlag = 0;
    mHasSkipLayer = false;

    if (contents->flags & HWC_GEOMETRY_CHANGED) {
        init_flag = 0;
        return;
    }

    if ((ovly_idx == -1) || (ovly_idx >= ((int)contents->numHwLayers - 2)) ||
        ((contents->numHwLayers - ovly_idx - 1) >= NUM_VIRT_OVER_HDMI)) {
        init_flag = 0;
        return;
    }

    ovly_idx++;
    if (init_flag == 1) {
        for (size_t i = ovly_idx; i < contents->numHwLayers - 1; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (!layer.handle || (mLastLayerHandles[i - ovly_idx] !=  layer.handle)) {
                init_flag = 0;
                return;
            }
        }

        mVirtualOverlayFlag = 1;
        for (size_t i = ovly_idx; i < contents->numHwLayers - 1; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.compositionType == HWC_FRAMEBUFFER) {
                layer.compositionType = HWC_OVERLAY;
                mHasSkipLayer = true;
            }
        }
        return;
    }

    init_flag = 1;
    for (size_t i = ovly_idx; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        mLastLayerHandles[i - ovly_idx] = layer.handle;
    }

    for (size_t i = contents->numHwLayers - ovly_idx; i < NUM_VIRT_OVER_HDMI; i++)
        mLastLayerHandles[i - ovly_idx] = 0;

    return;
}

void ExynosExternalDisplay::setPreset(int preset)
{
    mHwc->mHdmiResolutionChanged = false;
    mHwc->mHdmiResolutionHandled = false;
    mHwc->hdmi_hpd = false;
#if !defined(USE_DV_TIMINGS)
    v4l2_dv_preset v_preset;
    v_preset.preset = preset;
#else
    struct v4l2_dv_timings dv_timing;
    int dv_timings_index = getDVTimingsIndex(preset);
    if (dv_timings_index < 0) {
        ALOGE("invalid preset(%d)", preset);
        return;
    }
    memcpy(&dv_timing, &dv_timings[dv_timings_index], sizeof(v4l2_dv_timings));
#endif

    if (preset <= V4L2_DV_INVALID || preset > V4L2_DV_1080P30_TB) {
        ALOGE("%s: invalid preset, %d", __func__, preset);
        return;
    }

    disable();
#if defined(USE_DV_TIMINGS)
    if (ioctl(mMixerLayers[0].fd, VIDIOC_S_DV_TIMINGS, &dv_timing) != -1) {
        if (mHwc->procs)
            mHwc->procs->hotplug(mHwc->procs, HWC_DISPLAY_EXTERNAL, false);
    }
#else
    if (ioctl(mMixerLayers[0].fd, VIDIOC_S_DV_PRESET, &v_preset) != -1) {
        if (mHwc->procs)
            mHwc->procs->hotplug(mHwc->procs, HWC_DISPLAY_EXTERNAL, false);
    }
#endif
}

int ExynosExternalDisplay::convert3DTo2D(int preset)
{
    switch (preset) {
    case V4L2_DV_720P60_FP:
    case V4L2_DV_720P60_SB_HALF:
    case V4L2_DV_720P60_TB:
        return V4L2_DV_720P60;
    case V4L2_DV_720P50_FP:
    case V4L2_DV_720P50_SB_HALF:
    case V4L2_DV_720P50_TB:
        return V4L2_DV_720P50;
    case V4L2_DV_1080P60_SB_HALF:
    case V4L2_DV_1080P60_TB:
        return V4L2_DV_1080P60;
    case V4L2_DV_1080P30_FP:
    case V4L2_DV_1080P30_SB_HALF:
    case V4L2_DV_1080P30_TB:
        return V4L2_DV_1080P30;
    default:
        return HDMI_PRESET_ERROR;
    }
}

void ExynosExternalDisplay::calculateDstRect(int src_w, int src_h, int dst_w, int dst_h, struct v4l2_rect *dst_rect)
{
    if (dst_w * src_h <= dst_h * src_w) {
        dst_rect->left   = 0;
        dst_rect->top    = (dst_h - ((dst_w * src_h) / src_w)) >> 1;
        dst_rect->width  = dst_w;
        dst_rect->height = ((dst_w * src_h) / src_w);
    } else {
        dst_rect->left   = (dst_w - ((dst_h * src_w) / src_h)) >> 1;
        dst_rect->top    = 0;
        dst_rect->width  = ((dst_h * src_w) / src_h);
        dst_rect->height = dst_h;
    }
}

bool ExynosExternalDisplay::isVPSupported(hwc_layer_1_t &layer, int format)
{
#if defined(VP_VIDEO)
    int min_source_width = 32;
    int min_source_height = 4;
    private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
    if((layer.transform == 0) &&
       (format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED ||
        format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M ||
        format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV ||
        format == HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M)) {
        if (h->stride >= min_source_width &&
            h->height >= min_source_height &&
            (WIDTH(layer.sourceCropf) == WIDTH(layer.displayFrame)) &&
            (HEIGHT(layer.sourceCropf) == HEIGHT(layer.displayFrame))) {
            return true;
        }
    }
#endif
    return false;
}

bool ExynosExternalDisplay::isVideoOverlaySupported(hwc_layer_1_t &layer, int format)
{
#if defined(VP_VIDEO)
    if (isVPSupported(layer, format))
        return true;
#endif
    if (mMPPs[0]->isProcessingSupported(layer, format, false) > 0)
        return true;

    return false;
}

int ExynosExternalDisplay::prepare(hwc_display_contents_1_t* contents)
{
    hwc_layer_1_t *video_layer = NULL;
    uint32_t numVideoLayers = 0;
    uint32_t videoIndex = 0;

    mHwc->force_mirror_mode = false;
    checkGrallocFlags(contents);

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            ALOGV("\tlayer %u: framebuffer target", i);
            continue;
        }

        if (layer.compositionType == HWC_BACKGROUND) {
            ALOGV("\tlayer %u: background layer", i);
            dumpLayer(&layer);
            continue;
        }

#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
        if (mLocalExternalDisplayPause) {
            layer.compositionType = HWC_OVERLAY;
            layer.flags = HWC_SKIP_HDMI_RENDERING;
            continue;
        }
#endif

        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            if ((int)get_yuv_planes(halFormatToV4L2Format(h->format)) > 0) {
                if (mHwc->mS3DMode != S3D_MODE_DISABLED && mHwc->mHdmiResolutionChanged)
                    mHwc->mS3DMode = S3D_MODE_RUNNING;
            }

            if ((mHwc->force_mirror_mode) && getDrmMode(h->flags) == NO_DRM) {
                layer.compositionType = HWC_FRAMEBUFFER;
                continue;
            } else {
#if defined(GSC_VIDEO) || defined(VP_VIDEO)
                if (((getDrmMode(h->flags) != NO_DRM)
                     || ((int)get_yuv_planes(halFormatToV4L2Format(h->format)) > 0)) &&
                        isVideoOverlaySupported(layer, h->format)) {
#else
                if (getDrmMode(h->flags) != NO_DRM) {
#endif
#if !defined(GSC_VIDEO) && !defined(VP_VIDEO)
                    if (((mUseProtectedLayer == true) && (getDrmMode(handle->flags) != NO_DRM)) ||
                        ((mUseProtectedLayer == false) && !video_layer)) {
#endif
                        video_layer = &layer;
                        layer.compositionType = HWC_OVERLAY;
#if defined(GSC_VIDEO) || defined(VP_VIDEO)
                        videoIndex = i;
                        numVideoLayers++;
#endif
                        ALOGV("\tlayer %u: video layer", i);
                        dumpLayer(&layer);
                        continue;
#if !defined(GSC_VIDEO) && !defined(VP_VIDEO)
                    }
#endif
                }
            }
            layer.compositionType = HWC_FRAMEBUFFER;
            dumpLayer(&layer);
        } else {
            layer.compositionType = HWC_FRAMEBUFFER;
        }
    }
#if defined(GSC_VIDEO) || defined(VP_VIDEO)
    if (numVideoLayers == 1) {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (!mUseSubtitles || i == videoIndex) {
                if (mHwc->mS3DMode != S3D_MODE_DISABLED)
                    layer.compositionType = HWC_OVERLAY;
            }

            if (i == videoIndex) {
                struct v4l2_rect dest_rect;
                if (mHwc->mS3DMode != S3D_MODE_DISABLED) {
                    layer.displayFrame.left = 0;
                    layer.displayFrame.top = 0;
                    layer.displayFrame.right = mXres;
                    layer.displayFrame.bottom = mYres;
                }
            }
        }
#if !defined(USE_GRALLOC_FLAG_FOR_HDMI)
        skipStaticLayers(contents, videoIndex);
#endif
    } else if (numVideoLayers > 1) {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            private_handle_t *handle = NULL;
            if (layer.handle) {
                handle = private_handle_t::dynamicCast(layer.handle);
                if (getDrmMode(handle->flags) != NO_DRM)
                    continue;
            }
            if (layer.compositionType == HWC_FRAMEBUFFER_TARGET ||
                layer.compositionType == HWC_BACKGROUND)
                continue;
            layer.compositionType = HWC_FRAMEBUFFER;
        }
    }
#endif
    mHasFbComposition = false;
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.compositionType == HWC_FRAMEBUFFER)
            mHasFbComposition = true;
    }
    return 0;
}

int ExynosExternalDisplay::clearDisplay()
{
    return -1;
}

int ExynosExternalDisplay::set(hwc_display_contents_1_t* contents)
{
    hwc_layer_1_t *fb_layer = NULL;
    hwc_layer_1_t *video_layer = NULL;

    if (!mEnabled || mBlanked) {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.acquireFenceFd >= 0) {
                close(layer.acquireFenceFd);
                layer.acquireFenceFd = -1;
            }
        }
        return 0;
    }

#if !defined(USE_GRALLOC_FLAG_FOR_HDMI)
#if defined(VP_VIDEO)
    mVideoIndex = VIDEO_LAYER_INDEX;
#else
    mVideoIndex = 1;
#endif
    mUiIndex = 0;
#endif

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.flags & HWC_SKIP_LAYER) {
            ALOGV("HDMI skipping layer %d", i);
            continue;
        }

        if (layer.compositionType == HWC_OVERLAY) {
            if (!layer.handle) {
                layer.releaseFenceFd = layer.acquireFenceFd;
                continue;
            }

#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
            if (layer.flags & HWC_SKIP_HDMI_RENDERING) {
                layer.releaseFenceFd = layer.acquireFenceFd;
                continue;
            } else {
#endif
#if defined(GSC_VIDEO) || defined(VP_VIDEO)
                private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
                if ((int)get_yuv_planes(halFormatToV4L2Format(handle->format)) < 0) {
                    layer.releaseFenceFd = layer.acquireFenceFd;
                    continue;
                }
#endif
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
            }
#endif

            ALOGV("HDMI video layer:");
            dumpLayer(&layer);

            int gsc_idx = HDMI_GSC_IDX;
            bool changedPreset = false;
            if (mHwc->mS3DMode != S3D_MODE_DISABLED && mHwc->mHdmiResolutionChanged) {
                if (isPresetSupported(mHwc->mHdmiPreset)) {
                    mHwc->mS3DMode = S3D_MODE_RUNNING;
                    setPreset(mHwc->mHdmiPreset);
                    changedPreset = true;
                } else {
                    mHwc->mS3DMode = S3D_MODE_RUNNING;
                    mHwc->mHdmiResolutionChanged = false;
                    mHwc->mHdmiResolutionHandled = true;
                    int S3DFormat = getS3DFormat(mHwc->mHdmiPreset);
                    if (S3DFormat == S3D_SBS)
                        mMPPs[0]->mS3DMode = S3D_SBS;
                    else if (S3DFormat == S3D_TB)
                        mMPPs[0]->mS3DMode = S3D_TB;
                }
            }
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);

#if defined(GSC_VIDEO) || defined(VP_VIDEO)
            if ((getDrmMode(h->flags) != NO_DRM) ||
                ((int)get_yuv_planes(halFormatToV4L2Format(h->format)) > 0)) {
#else
            if (getDrmMode(h->flags) != NO_DRM) {
#endif
                if (getDrmMode(h->flags) == SECURE_DRM)
                    recalculateDisplayFrame(layer, mXres, mYres);

                video_layer = &layer;

                if (mIsVideoLayer == false)
                    mVideoStarted = true;
                else
                    mVideoStarted = false;
                mIsVideoLayer = true;

#if defined(VP_VIDEO)
                if ((mMPPs[0]->mS3DMode == S3D_MODE_DISABLED) &&
                    (isVPSupported(layer, h->format))) {
                    enableLayer(mMixerLayers[mVideoIndex]);
                    output(mMixerLayers[mVideoIndex], layer, h, layer.acquireFenceFd,
                            &layer.releaseFenceFd);
                } else {
#endif
                ExynosMPPModule &gsc = *mMPPs[0];
#if defined(VP_VIDEO)
                int ret = gsc.processM2M(layer, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED, NULL);
#else
                int ret = gsc.processM2M(layer, HAL_PIXEL_FORMAT_BGRA_8888, NULL);
#endif
                if (ret < 0) {
                    ALOGE("failed to configure gscaler for video layer");
                    continue;
                }

                buffer_handle_t dst_buf = gsc.mDstBuffers[gsc.mCurrentBuf];
                private_handle_t *h = private_handle_t::dynamicCast(dst_buf);

                int acquireFenceFd = gsc.mDstConfig.releaseFenceFd;
                int releaseFenceFd = -1;

                enableLayer(mMixerLayers[mVideoIndex]);

                output(mMixerLayers[mVideoIndex], layer, h, acquireFenceFd,
                                                                 &releaseFenceFd);

                if (gsc.mDstBufFence[gsc.mCurrentBuf] >= 0) {
                    close (gsc.mDstBufFence[gsc.mCurrentBuf]);
                    gsc.mDstBufFence[gsc.mCurrentBuf] = -1;
                }
                gsc.mDstBufFence[gsc.mCurrentBuf] = releaseFenceFd;
                gsc.mCurrentBuf = (gsc.mCurrentBuf + 1) % gsc.mNumAvailableDstBuffers;
#if defined(VP_VIDEO)
                }
#endif
            }

        }

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            if (!layer.handle) {
                layer.releaseFenceFd = layer.acquireFenceFd;
                continue;
            }

            if (!mHasFbComposition && !mHasSkipLayer) {
                layer.releaseFenceFd = layer.acquireFenceFd;
                continue;
            }

            dumpLayer(&layer);

            /* HDMI rotation for camera preview */
            bool camera_preview_started = false;
            bool camera_connected = false;
            char value[PROPERTY_VALUE_MAX];
            property_get("persist.sys.camera.preview", value, "0");
            camera_preview_started = !!(atoi(value));
            property_get("persist.sys.camera.connect", value, "0");
            camera_connected = !!(atoi(value));

            if (camera_preview_started && camera_connected)
                mIsCameraStarted = true;
            else if (!camera_preview_started && !camera_connected)
                mIsCameraStarted = false;

            if (((mFBT_Transform == HAL_TRANSFORM_FLIP_V) ||
                        (mFBT_Transform == HAL_TRANSFORM_FLIP_H) ||
                        (mFBT_Transform == HAL_TRANSFORM_ROT_90) ||
                        (mFBT_Transform == HAL_TRANSFORM_ROT_180) ||
                        (mFBT_Transform == HAL_TRANSFORM_ROT_270))) {
                if (mHasFbComposition && mIsCameraStarted && camera_connected) {
                    struct v4l2_rect dest_rect;
                    bool rot90or270 = !!((mFBT_Transform) & HAL_TRANSFORM_ROT_90);

                    if (rot90or270)
                        calculateDstRect(HEIGHT(layer.sourceCropf), WIDTH(layer.sourceCropf),
                                mXres, mYres, &dest_rect);
                    else
                        calculateDstRect(WIDTH(layer.sourceCropf), HEIGHT(layer.sourceCropf),
                                mXres, mYres, &dest_rect);

                    layer.displayFrame.left = dest_rect.left;
                    layer.displayFrame.top = dest_rect.top;
                    layer.displayFrame.right = dest_rect.width + dest_rect.left;
                    layer.displayFrame.bottom = dest_rect.height + dest_rect.top;
                    layer.transform = mFBT_Transform;

                    ExynosMPPModule &gsc = *mMPPs[0];
                    int ret = gsc.processM2M(layer, HAL_PIXEL_FORMAT_BGRA_8888, NULL);
                    if (ret < 0) {
                        ALOGE("failed to configure gscaler for video layer");
                        continue;
                    }

                    buffer_handle_t dst_buf = gsc.mDstBuffers[gsc.mCurrentBuf];
                    private_handle_t *h = private_handle_t::dynamicCast(dst_buf);

                    int acquireFenceFd = gsc.mDstConfig.releaseFenceFd;
                    int releaseFenceFd = -1;

                    if (mIsVideoLayer == false)
                        mVideoStarted = true;
                    else
                        mVideoStarted = false;
                    mIsVideoLayer = true;

                    enableLayer(mMixerLayers[mVideoIndex]);
                    output(mMixerLayers[mVideoIndex], layer, h, acquireFenceFd,
                                                                     &releaseFenceFd);
                    if (gsc.mDstBufFence[gsc.mCurrentBuf] >= 0) {
                        close (gsc.mDstBufFence[gsc.mCurrentBuf]);
                        gsc.mDstBufFence[gsc.mCurrentBuf] = -1;
                    }
                    gsc.mDstBufFence[gsc.mCurrentBuf] = releaseFenceFd;
                    gsc.mCurrentBuf = (gsc.mCurrentBuf + 1) % NUM_GSC_DST_BUFS;

                } else
                    layer.releaseFenceFd = layer.acquireFenceFd;
                video_layer = &layer;
            } else {
                if (mHasFbComposition && ((mIsCameraStarted && camera_connected) || !mIsCameraStarted)) {
                    if (mIsFbLayer == false)
                        mFbStarted = true;
                    else
                        mFbStarted = false;
                    mIsFbLayer = true;

                    layer.displayFrame.left = 0;
                    layer.displayFrame.right = mXres;
                    layer.displayFrame.top = 0;
                    layer.displayFrame.bottom = mYres;
                    layer.transform = 0;

                    enableLayer(mMixerLayers[mUiIndex]);
                    private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
                    private_module_t *grallocModule = (private_module_t *)((ExynosOverlayDisplay *)mHwc->primaryDisplay)->mGrallocModule;
                    waitForRenderFinish(grallocModule, &layer.handle, 1);
                    output(mMixerLayers[mUiIndex], layer, h, layer.acquireFenceFd,
                            &layer.releaseFenceFd);
                } else
                    layer.releaseFenceFd = layer.acquireFenceFd;
                fb_layer = &layer;
            }
        }
    }

#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
    if (!mLocalExternalDisplayPause) {
#endif
        if (!video_layer) {
            disableLayer(mMixerLayers[mVideoIndex]);
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
            mIsVideoLayer = false;
#endif
            mMPPs[0]->cleanupM2M();
            if (mHwc->mS3DMode == S3D_MODE_RUNNING && contents->numHwLayers > 1) {
                int preset = convert3DTo2D(mHwc->mHdmiCurrentPreset);
                if (isPresetSupported(preset)) {
                    setPreset(preset);
                    mHwc->mS3DMode = S3D_MODE_STOPPING;
                    mHwc->mHdmiPreset = preset;
                    if (mHwc->procs)
                        mHwc->procs->invalidate(mHwc->procs);
                } else {
                    mHwc->mS3DMode = S3D_MODE_DISABLED;
                    mHwc->mHdmiPreset = mHwc->mHdmiCurrentPreset;
                }
            }
        }

        if (!fb_layer) {
            disableLayer(mMixerLayers[mUiIndex]);
            mIsFbLayer = false;
        }
#if !defined(USE_GRALLOC_FLAG_FOR_HDMI)
        if (!video_layer) {
            disableLayer(mMixerLayers[mVideoIndex]);
            mIsVideoLayer = false;
        }
#endif

        /* MIXER_UPDATE */
        if (exynos_v4l2_s_ctrl(mMixerLayers[mUiIndex].fd, V4L2_CID_TV_UPDATE, 1) < 0) {
            ALOGE("%s: s_ctrl(CID_TV_UPDATE) failed %d", __func__, errno);
            return -1;
        }
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
    }
#endif

    return 0;
}

void ExynosExternalDisplay::setHdcpStatus(int status)
{
    if (exynos_v4l2_s_ctrl(mMixerLayers[1].fd, V4L2_CID_TV_HDCP_ENABLE,
                           !!status) < 0)
        ALOGE("%s: s_ctrl(CID_TV_HDCP_ENABLE) failed %d", __func__, errno);
}

void ExynosExternalDisplay::setAudioChannel(uint32_t channels)
{
    if (exynos_v4l2_s_ctrl(mMixerLayers[0].fd,
            V4L2_CID_TV_SET_NUM_CHANNELS, channels) < 0)
        ALOGE("%s: failed to set audio channels", __func__);
}

uint32_t ExynosExternalDisplay::getAudioChannel()
{
    int channels;
    if (exynos_v4l2_g_ctrl(mMixerLayers[0].fd,
            V4L2_CID_TV_MAX_AUDIO_CHANNELS, &channels) < 0)
        ALOGE("%s: failed to get audio channels", __func__);
    return channels;
}

void ExynosExternalDisplay::checkGrallocFlags(hwc_display_contents_1_t *contents)
{
    mUseProtectedLayer = false;
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
    mUseScreenshootLayer = false;

    /* it can get from HWCService */
    mLocalExternalDisplayPause = mHwc->external_display_pause;
    mFBT_Transform = mHwc->ext_fbt_transform;
#endif

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            if (h->flags & GRALLOC_USAGE_PROTECTED)
                mUseProtectedLayer = false;
        }
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
        if (layer.flags & HWC_SCREENSHOT_ANIMATOR_LAYER)
            mUseScreenshootLayer = true;
#endif
    }

#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
    if (mUseScreenshootLayer)
        mLocalExternalDisplayPause = true;
    else
        mLocalExternalDisplayPause = false;
#endif
}

int ExynosExternalDisplay::getCecPaddr()
{
    if (!mHwc->hdmi_hpd)
        return -1;

    int cecPaddr = -1;;

    if (exynos_v4l2_g_ctrl(mMixerLayers[0].fd, V4L2_CID_TV_SOURCE_PHY_ADDR, &cecPaddr) < 0)
        return -1;

    return cecPaddr;
}

int ExynosExternalDisplay::blank()
{
/* USE_HDMI_BLANK */
    /*
     * V4L2_CID_TV_BLANK becomes effective
     * only if it is called before disable() : STREAMOFF
     */
    if (exynos_v4l2_s_ctrl(mMixerLayers[mUiIndex].fd, V4L2_CID_TV_BLANK, 1) < 0) {
        ALOGE("%s: s_ctrl(CID_TV_BLANK) failed %d", __func__, errno);
        return -1;
    }
    return 0;
}

int ExynosExternalDisplay::waitForRenderFinish(private_module_t *grallocModule, buffer_handle_t *handle, int buffers)
{
    return 0;
}
