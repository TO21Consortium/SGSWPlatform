LOCAL_PATH := $(call my-dir)

ifeq ($(BOARD_USE_SKYPE_HD), true)

#################################
#### libExynosOMX_SkypeHD_Enc ###
#################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosOMX_SkypeHD_Enc

LOCAL_CFLAGS := -DUSE_SKYPE_HD
LOCAL_CFLAGS += -DBUILD_ENC

LOCAL_SRC_FILES := Exynos_OSAL_SkypeHD.c

LOCAL_C_INCLUDES := \
	$(EXYNOS_OMX_TOP)/core \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_INC)/skype \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_COMPONENT)/common \
	$(EXYNOS_OMX_COMPONENT)/video/enc \
	$(EXYNOS_OMX_COMPONENT)/video/enc/h264 \
	$(EXYNOS_VIDEO_CODEC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/samsung_slsi/exynos/include

ifeq ($(BOARD_USE_KHRONOS_OMX_HEADER), true)
LOCAL_CFLAGS += -DUSE_KHRONOS_OMX_HEADER
LOCAL_C_INCLUDES += $(EXYNOS_OMX_INC)/khronos
else
ifeq ($(BOARD_USE_ANDROID), true)
LOCAL_C_INCLUDES += $(ANDROID_MEDIA_INC)/openmax
endif
endif

include $(BUILD_STATIC_LIBRARY)

#################################
#### libExynosOMX_SkypeHD_Dec ###
#################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosOMX_SkypeHD_Dec

LOCAL_CFLAGS := -DUSE_SKYPE_HD
LOCAL_CFLAGS += -DBUILD_DEC
LOCAL_SRC_FILES := Exynos_OSAL_SkypeHD.c

LOCAL_C_INCLUDES := \
	$(EXYNOS_OMX_TOP)/core \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_INC)/skype \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_COMPONENT)/common \
	$(EXYNOS_OMX_COMPONENT)/video/dec \
	$(EXYNOS_OMX_COMPONENT)/video/dec/h264 \
	$(EXYNOS_VIDEO_CODEC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/samsung_slsi/exynos/include

ifeq ($(BOARD_USE_KHRONOS_OMX_HEADER), true)
LOCAL_CFLAGS += -DUSE_KHRONOS_OMX_HEADER
LOCAL_C_INCLUDES += $(EXYNOS_OMX_INC)/khronos
else
ifeq ($(BOARD_USE_ANDROID), true)
LOCAL_C_INCLUDES += $(ANDROID_MEDIA_INC)/openmax
endif
endif

include $(BUILD_STATIC_LIBRARY)
endif  # for Skype HD


##########################
#### libExynosOMX_OSAL ###
##########################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Exynos_OSAL_Event.c \
	Exynos_OSAL_Queue.c \
	Exynos_OSAL_ETC.c \
	Exynos_OSAL_Mutex.c \
	Exynos_OSAL_Thread.c \
	Exynos_OSAL_Memory.c \
	Exynos_OSAL_Semaphore.c \
	Exynos_OSAL_Library.c \
	Exynos_OSAL_Log.c \
	Exynos_OSAL_SharedMemory.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosOMX_OSAL

LOCAL_CFLAGS :=

ifeq ($(BOARD_USE_ANDROID), true)
LOCAL_SRC_FILES += \
	Exynos_OSAL_Android.cpp
endif

ifeq ($(BOARD_USE_ANB), true)
LOCAL_CFLAGS += -DUSE_ANB

ifeq ($(BOARD_USE_ANB_OUTBUF_SHARE), true)
LOCAL_CFLAGS += -DUSE_ANB_OUTBUF_SHARE
endif
endif

ifeq ($(BOARD_USE_DMA_BUF), true)
LOCAL_CFLAGS += -DUSE_DMA_BUF
endif

ifeq ($(BOARD_USE_METADATABUFFERTYPE), true)
LOCAL_CFLAGS += -DUSE_METADATABUFFERTYPE

ifeq ($(BOARD_USE_STOREMETADATA), true)
LOCAL_CFLAGS += -DUSE_STOREMETADATA
endif

ifeq ($(BOARD_USE_ANDROIDOPAQUE), true)
LOCAL_CFLAGS += -DUSE_ANDROIDOPAQUE
endif
endif

ifeq ($(BOARD_USE_IMPROVED_BUFFER), true)
LOCAL_CFLAGS += -DUSE_IMPROVED_BUFFER
endif

ifeq ($(BOARD_USE_CSC_HW), true)
LOCAL_CFLAGS += -DUSE_CSC_HW
endif

ifeq ($(BOARD_USE_NON_CACHED_GRAPHICBUFFER), true)
LOCAL_CFLAGS += -DUSE_NON_CACHED_GRAPHICBUFFER
endif

ifeq ($(TARGET_BOARD_PLATFORM),exynos3)
LOCAL_CFLAGS += -DUSE_MFC5X_ALIGNMENT
endif

ifeq ($(TARGET_BOARD_PLATFORM),exynos4)
LOCAL_CFLAGS += -DUSE_MFC5X_ALIGNMENT
endif

LOCAL_SHARED_LIBRARIES := libhardware
LOCAL_STATIC_LIBRARIES := liblog libcutils libExynosVideoApi

LOCAL_C_INCLUDES := \
	$(EXYNOS_OMX_TOP)/core \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_COMPONENT)/common \
	$(EXYNOS_OMX_COMPONENT)/video/dec \
	$(EXYNOS_OMX_COMPONENT)/video/enc \
	$(EXYNOS_VIDEO_CODEC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/samsung_slsi/exynos/include

ifeq ($(BOARD_USE_ANDROID), true)
LOCAL_C_INCLUDES += \
	$(ANDROID_MEDIA_INC)/hardware \
	$(TOP)/system/core/libion/include
endif

ifeq ($(BOARD_USE_KHRONOS_OMX_HEADER), true)
LOCAL_CFLAGS += -DUSE_KHRONOS_OMX_HEADER
LOCAL_C_INCLUDES += $(EXYNOS_OMX_INC)/khronos
else
ifeq ($(BOARD_USE_ANDROID), true)
LOCAL_C_INCLUDES += $(ANDROID_MEDIA_INC)/openmax
endif
endif

include $(BUILD_STATIC_LIBRARY)
