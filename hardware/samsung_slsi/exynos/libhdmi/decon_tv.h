#ifndef SAMSUNG_DEOCN_TV_H
#define SAMSUNG_DECON_TV_H

struct exynos_hdmi_data {
    enum {
        EXYNOS_HDMI_STATE_PRESET = 0,
        EXYNOS_HDMI_STATE_ENUM_PRESET,
        EXYNOS_HDMI_STATE_CEC_ADDR,
        EXYNOS_HDMI_STATE_HDCP,
        EXYNOS_HDMI_STATE_AUDIO,
    } state;
    struct  v4l2_dv_timings timings;
    struct  v4l2_enum_dv_timings etimings;
    __u32   cec_addr;
    __u32   audio_info;
    int     hdcp;
};

#define EXYNOS_GET_HDMI_CONFIG          _IOW('F', 220, \
        struct exynos_hdmi_data)
#define EXYNOS_SET_HDMI_CONFIG          _IOW('F', 221, \
        struct exynos_hdmi_data)

#endif /* SAMSUNG_DECON_TV_H */
