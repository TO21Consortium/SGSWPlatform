LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	Exynos_OMX_VdecControl.c \
	Exynos_OMX_Vdec.c

LOCAL_MODULE := libExynosOMX_Vdec
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_TOP)/core \
	$(EXYNOS_OMX_COMPONENT)/common \
	$(EXYNOS_OMX_COMPONENT)/video/dec \
	$(EXYNOS_VIDEO_CODEC)/include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include

LOCAL_STATIC_LIBRARIES := libExynosVideoApi

ifeq ($(BOARD_USE_KHRONOS_OMX_HEADER), true)
LOCAL_CFLAGS += -DUSE_KHRONOS_OMX_HEADER
LOCAL_C_INCLUDES += $(EXYNOS_OMX_INC)/khronos
else
ifeq ($(BOARD_USE_ANDROID), true)
LOCAL_C_INCLUDES += $(ANDROID_MEDIA_INC)/openmax
endif
endif

ifeq ($(BOARD_USE_ANB), true)
LOCAL_STATIC_LIBRARIES += libExynosOMX_OSAL
LOCAL_CFLAGS += -DUSE_ANB
endif

ifeq ($(BOARD_USE_DMA_BUF), true)
LOCAL_CFLAGS += -DUSE_DMA_BUF
endif

ifeq ($(BOARD_USE_CSC_HW), true)
LOCAL_CFLAGS += -DUSE_CSC_HW
endif

ifeq ($(BOARD_USE_QOS_CTRL), true)
LOCAL_CFLAGS += -DUSE_QOS_CTRL
endif

ifeq ($(BOARD_USE_STOREMETADATA), true)
LOCAL_CFLAGS += -DUSE_STOREMETADATA
endif

ifeq ($(EXYNOS_OMX_SUPPORT_TUNNELING), true)
LOCAL_CFLAGS += -DTUNNELING_SUPPORT
endif

include $(BUILD_STATIC_LIBRARY)
