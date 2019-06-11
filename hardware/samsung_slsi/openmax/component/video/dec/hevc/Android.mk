LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Exynos_OMX_HEVCdec.c \
	library_register.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libOMX.Exynos.HEVC.Decoder
LOCAL_MODULE_RELATIVE_PATH := omx

LOCAL_CFLAGS :=
LOCAL_CFLAGS += -DUSE_HEVC_SUPPORT

ifeq ($(BOARD_USE_ANB), true)
LOCAL_CFLAGS += -DUSE_ANB
endif

ifeq ($(BOARD_USE_DMA_BUF), true)
LOCAL_CFLAGS += -DUSE_DMA_BUF
endif

ifeq ($(BOARD_USE_S3D_SUPPORT), true)
ifeq ($(BOARD_USES_HWC_SERVICES), true)
LOCAL_CFLAGS += -DUSE_S3D_SUPPORT
else
ifeq ($(TARGET_BOARD_PLATFORM), exynos5)
LOCAL_CFLAGS += -DUSE_S3D_SUPPORT
endif
endif
endif

ifeq ($(BOARD_USE_CSC_HW), true)
LOCAL_CFLAGS += -DUSE_CSC_HW
endif

ifeq ($(BOARD_USE_CUSTOM_COMPONENT_SUPPORT), true)
LOCAL_CFLAGS += -DUSE_CUSTOM_COMPONENT_SUPPORT
endif

ifeq ($(BOARD_USE_TIMESTAMP_REORDER_SUPPORT), true)
LOCAL_CFLAGS += -DUSE_TIMESTAMP_REORDER_SUPPORT
endif

ifeq ($(BOARD_USE_SINGLE_PLANE_IN_DRM), true)
LOCAL_CFLAGS += -DUSE_SINGLE_PLANE_IN_DRM
endif

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libExynosOMX_Vdec libExynosOMX_OSAL libExynosOMX_Basecomponent \
	libExynosVideoApi
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libui \
	libExynosOMX_Resourcemanager libcsc libexynosv4l2 libion libhardware

LOCAL_C_INCLUDES := \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_TOP)/core \
	$(EXYNOS_OMX_COMPONENT)/common \
	$(EXYNOS_OMX_COMPONENT)/video/dec \
	$(EXYNOS_VIDEO_CODEC)/include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include

ifeq ($(BOARD_USE_KHRONOS_OMX_HEADER), true)
LOCAL_CFLAGS += -DUSE_KHRONOS_OMX_HEADER
LOCAL_C_INCLUDES += $(EXYNOS_OMX_INC)/khronos
else
ifeq ($(BOARD_USE_ANDROID), true)
LOCAL_C_INCLUDES += $(ANDROID_MEDIA_INC)/openmax
endif
endif

include $(BUILD_SHARED_LIBRARY)
