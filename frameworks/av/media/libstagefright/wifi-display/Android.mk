LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        MediaSender.cpp                 \
        Parameters.cpp                  \
        rtp/RTPSender.cpp               \
        source/Converter.cpp            \
        source/MediaPuller.cpp          \
        source/PlaybackSession.cpp      \
        source/RepeaterSource.cpp       \
        source/TSPacketizer.cpp         \
        source/WifiDisplaySource.cpp    \
        VideoFormats.cpp                \

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/av/media/libstagefright/mpeg2ts \

LOCAL_SHARED_LIBRARIES:= \
        libbinder                       \
        libcutils                       \
        liblog                          \
        libgui                          \
        libmedia                        \
        libstagefright                  \
        libstagefright_foundation       \
        libui                           \
        libutils                        \

ifeq ($(BOARD_USES_WIFI_DISPLAY),true)
LOCAL_CFLAGS += -DUSES_WIFI_DISPLAY
LOCAL_SHARED_LIBRARIES += libExynosHWCService
LOCAL_C_INCLUDES += \
        $(TOP)/hardware/samsung_slsi/exynos/libhwcService \
        $(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
        $(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
        $(TOP)/hardware/samsung_slsi/exynos/include \
        $(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhwcmodule \
        $(TOP)/hardware/samsung_slsi/exynos/libhwc \
        $(TOP)/hardware/samsung_slsi/exynos/libexynosutils \
        $(TOP)/system/core/libsync/include
endif

ifeq ($(BOARD_USES_VDS_BGRA8888), true)
LOCAL_CFLAGS += -DUSES_VDS_BGRA8888
endif

ifeq ($(BOARD_USES_VDS_YUV420SPM), true)
LOCAL_CFLAGS += -DUSES_VDS_YUV420SPM
LOCAL_C_INCLUDES += \
        $(TOP)/hardware/samsung_slsi/exynos/include
endif

LOCAL_CFLAGS += -Wno-multichar -Werror -Wall
LOCAL_CLANG := true

LOCAL_MODULE:= libstagefright_wfd

LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
