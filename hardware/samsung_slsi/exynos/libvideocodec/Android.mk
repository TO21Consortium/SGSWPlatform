LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	ExynosVideoInterface.c \
	dec/ExynosVideoDecoder.c \
	enc/ExynosVideoEncoder.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/system/core/libion/include

ifeq ($(BOARD_USE_KHRONOS_OMX_HEADER), true)
LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/openmax/include/khronos
else
LOCAL_C_INCLUDES += $(TOP)/frameworks/native/include/media/openmax
endif

# only 3.4 kernel
ifneq ($(findstring 3.1, $(TARGET_LINUX_KERNEL_VERSION)), 3.1)
LOCAL_CFLAGS += -DUSE_EXYNOS_MEDIA_EXT
endif

# since 3.10 kernel
ifneq ($(filter-out 3.4, $(TARGET_LINUX_KERNEL_VERSION)),)
LOCAL_CFLAGS += -DCID_SUPPORT
LOCAL_CFLAGS += -DUSE_DEFINE_H264_SEI_TYPE
endif

# since 3.18 kernel
ifneq ($(filter 3.18, $(TARGET_LINUX_KERNEL_VERSION)),)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/mfc_headers
LOCAL_CFLAGS += -DUSE_MFC_MEDIA
LOCAL_CFLAGS += -DUSE_ORIGINAL_HEADER
ifeq ($(BOARD_USE_SINGLE_PLANE_IN_DRM), true)
LOCAL_CFLAGS += -DUSE_SINGLE_PALNE_SUPPORT
endif
endif

ifeq ($(BOARD_USE_HEVCDEC_SUPPORT), true)
LOCAL_CFLAGS += -DUSE_HEVCDEC_SUPPORT
endif

ifeq ($(BOARD_USE_HEVCENC_SUPPORT), true)
LOCAL_CFLAGS += -DUSE_HEVCENC_SUPPORT
endif

ifeq ($(BOARD_USE_HEVC_HWIP), true)
LOCAL_CFLAGS += -DUSE_HEVC_HWIP
endif

ifeq ($(BOARD_USE_VP9DEC_SUPPORT), true)
LOCAL_CFLAGS += -DUSE_VP9DEC_SUPPORT
endif

ifeq ($(BOARD_USE_VP9ENC_SUPPORT), true)
LOCAL_CFLAGS += -DUSE_VP9ENC_SUPPORT
endif

ifeq ($(BOARD_USE_FORCEFULLY_DISABLE_DUALDPB), true)
LOCAL_CFLAGS += -DUSE_FORCEFULLY_DISABLE_DUALDPB
endif

ifeq ($(BOARD_USE_DEINTERLACING_SUPPORT), true)
LOCAL_CFLAGS += -DUSE_DEINTERLACING_SUPPORT
endif

LOCAL_MODULE := libExynosVideoApi
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)
