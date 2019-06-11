LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CLANG := true

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_SRC_FILES := \
    Client.cpp \
    DisplayDevice.cpp \
    DispSync.cpp \
    EventControlThread.cpp \
    EventThread.cpp \
    FrameTracker.cpp \
    Layer.cpp \
    LayerDim.cpp \
    MessageQueue.cpp \
    MonitoredProducer.cpp \
    SurfaceFlinger.cpp \
    SurfaceFlingerConsumer.cpp \
    Transform.cpp \
    DisplayHardware/FramebufferSurface.cpp \
    DisplayHardware/HWComposer.cpp \
    DisplayHardware/PowerHAL.cpp \
    DisplayHardware/VirtualDisplaySurface.cpp \
    Effects/Daltonizer.cpp \
    EventLog/EventLogTags.logtags \
    EventLog/EventLog.cpp \
    RenderEngine/Description.cpp \
    RenderEngine/Mesh.cpp \
    RenderEngine/Program.cpp \
    RenderEngine/ProgramCache.cpp \
    RenderEngine/GLExtensions.cpp \
    RenderEngine/RenderEngine.cpp \
    RenderEngine/Texture.cpp \
    RenderEngine/GLES10RenderEngine.cpp \
    RenderEngine/GLES11RenderEngine.cpp \
    RenderEngine/GLES20RenderEngine.cpp


LOCAL_CFLAGS := -DLOG_TAG=\"SurfaceFlinger\"
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

ifeq ($(TARGET_BOARD_PLATFORM),omap4)
    LOCAL_CFLAGS += -DHAS_CONTEXT_PRIORITY
endif
ifeq ($(TARGET_BOARD_PLATFORM),s5pc110)
    LOCAL_CFLAGS += -DHAS_CONTEXT_PRIORITY
endif

ifeq ($(TARGET_DISABLE_TRIPLE_BUFFERING),true)
    LOCAL_CFLAGS += -DTARGET_DISABLE_TRIPLE_BUFFERING
endif

ifeq ($(TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS),true)
    LOCAL_CFLAGS += -DFORCE_HWC_COPY_FOR_VIRTUAL_DISPLAYS
endif

ifeq ($(BOARD_USES_VIRTUAL_DISPLAY_DECON_EXT_WB), true)
	LOCAL_CFLAGS += -DUSES_VIRTUAL_DISPLAY_DECON_EXT_WB
endif

ifeq ($(BOARD_USES_EGL_SURFACE_FOR_COMPOSITION_MIXED), true)
    LOCAL_CFLAGS += -DUSES_EGL_SURFACE_FOR_COMPOSITION_MIXED
endif

ifeq ($(BOARD_USES_VDS_OTHERFORMAT), true)
    LOCAL_CFLAGS += -DUSES_VDS_OTHERFORMAT
endif

ifneq ($(NUM_FRAMEBUFFER_SURFACE_BUFFERS),)
    LOCAL_CFLAGS += -DNUM_FRAMEBUFFER_SURFACE_BUFFERS=$(NUM_FRAMEBUFFER_SURFACE_BUFFERS)
endif

ifeq ($(TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK),true)
    LOCAL_CFLAGS += -DRUNNING_WITHOUT_SYNC_FRAMEWORK
endif

# See build/target/board/generic/BoardConfig.mk for a description of this setting.
ifneq ($(VSYNC_EVENT_PHASE_OFFSET_NS),)
    LOCAL_CFLAGS += -DVSYNC_EVENT_PHASE_OFFSET_NS=$(VSYNC_EVENT_PHASE_OFFSET_NS)
else
    LOCAL_CFLAGS += -DVSYNC_EVENT_PHASE_OFFSET_NS=0
endif

# See build/target/board/generic/BoardConfig.mk for a description of this setting.
ifneq ($(SF_VSYNC_EVENT_PHASE_OFFSET_NS),)
    LOCAL_CFLAGS += -DSF_VSYNC_EVENT_PHASE_OFFSET_NS=$(SF_VSYNC_EVENT_PHASE_OFFSET_NS)
else
    LOCAL_CFLAGS += -DSF_VSYNC_EVENT_PHASE_OFFSET_NS=0
endif

ifneq ($(PRESENT_TIME_OFFSET_FROM_VSYNC_NS),)
    LOCAL_CFLAGS += -DPRESENT_TIME_OFFSET_FROM_VSYNC_NS=$(PRESENT_TIME_OFFSET_FROM_VSYNC_NS)
else
    LOCAL_CFLAGS += -DPRESENT_TIME_OFFSET_FROM_VSYNC_NS=0
endif

ifneq ($(MAX_VIRTUAL_DISPLAY_DIMENSION),)
    LOCAL_CFLAGS += -DMAX_VIRTUAL_DISPLAY_DIMENSION=$(MAX_VIRTUAL_DISPLAY_DIMENSION)
else
    LOCAL_CFLAGS += -DMAX_VIRTUAL_DISPLAY_DIMENSION=0
endif

LOCAL_CFLAGS += -fvisibility=hidden -Werror=format
LOCAL_CFLAGS += -std=c++11

ifeq ($(BOARD_USE_EXTERNAL_BGRA), true)
    LOCAL_CFLAGS += -DUSE_EXTERNAL_BGRA
endif

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libdl \
    libhardware \
    libutils \
    libEGL \
    libGLESv1_CM \
    libGLESv2 \
    libbinder \
    libui \
    libgui \
    libpowermanager

ifeq ($(BOARD_USES_HWC_SERVICES), true)
	LOCAL_CFLAGS += -DUSES_HWC_SERVICES
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

ifeq ($(BOARD_USES_VIRTUAL_DISPLAY),true)
	LOCAL_CFLAGS += -DUSES_VIRTUAL_DISPLAY
endif
ifeq ($(BOARD_SUPPORT_DQ_Q_SEQUENCE), true)
	LOCAL_CFLAGS += -DSUPPORT_DQ_Q_SEQUENCE
endif
ifeq ($(BOARD_SUPPORT_Q_DQ_SEQUENCE), true)
	LOCAL_CFLAGS += -DSUPPORT_Q_DQ_SEQUENCE
endif
ifeq ($(BOARD_USES_VDS_BGRA8888), true)
	LOCAL_CFLAGS += -DUSES_VDS_BGRA8888
endif
ifeq ($(BOARD_USES_DISABLE_COMPOSITIONTYPE_GLES), true)
	LOCAL_CFLAGS += -DUSES_DISABLE_COMPOSITIONTYPE_GLES
endif
ifneq ($(BOARD_HDMI_INCAPABLE), true)
	LOCAL_CFLAGS += -DHDMI_ENABLED
endif
endif

ifeq ($(BOARD_USES_FIMC), true)
	LOCAL_CFLAGS += -DUSES_FIMC
endif
LOCAL_CFLAGS += -DEXYNOS

ifeq ($(BOARD_USE_RENDER_ENGINE_CONFIG), true)
	LOCAL_CFLAGS += -DUSE_RENDER_ENGINE_CONFIG
endif

ifeq ($(BOARD_USES_EXYNOS5_DSS_FEATURE), true)
        LOCAL_CFLAGS += -DUSES_EXYNOS5_DSS_FEATURE
endif

ifeq ($(BOARD_USES_EXYNOS5_AFBC_FEATURE), true)
	LOCAL_CFLAGS += -DEXYNOS_AFBC
endif

LOCAL_MODULE := libsurfaceflinger

LOCAL_CFLAGS += -Wall -Werror -Wunused -Wunreachable-code

include $(BUILD_SHARED_LIBRARY)

###############################################################
# build surfaceflinger's executable
include $(CLEAR_VARS)

LOCAL_CLANG := true

LOCAL_LDFLAGS := -Wl,--version-script,art/sigchainlib/version-script.txt -Wl,--export-dynamic
LOCAL_CFLAGS := -DLOG_TAG=\"SurfaceFlinger\"
LOCAL_CPPFLAGS := -std=c++11

LOCAL_SRC_FILES := \
    main_surfaceflinger.cpp

LOCAL_SHARED_LIBRARIES := \
    libsurfaceflinger \
    libcutils \
    liblog \
    libbinder \
    libutils \
    libdl

LOCAL_WHOLE_STATIC_LIBRARIES := libsigchain

ifeq ($(BOARD_USES_EXYNOS5_DSS_FEATURE), true)
        LOCAL_CFLAGS += -DUSES_EXYNOS5_DSS_FEATURE
endif

LOCAL_MODULE := surfaceflinger

ifdef TARGET_32_BIT_SURFACEFLINGER
LOCAL_32_BIT_ONLY := true
endif

LOCAL_CFLAGS += -Wall -Werror -Wunused -Wunreachable-code

include $(BUILD_EXECUTABLE)

###############################################################
# uses jni which may not be available in PDK
ifneq ($(wildcard libnativehelper/include),)
include $(CLEAR_VARS)

LOCAL_CLANG := true

LOCAL_CFLAGS := -DLOG_TAG=\"SurfaceFlinger\"
LOCAL_CPPFLAGS := -std=c++11

LOCAL_SRC_FILES := \
    DdmConnection.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libdl

LOCAL_MODULE := libsurfaceflinger_ddmconnection

LOCAL_CFLAGS += -Wall -Werror -Wunused -Wunreachable-code

include $(BUILD_SHARED_LIBRARY)
endif # libnativehelper
