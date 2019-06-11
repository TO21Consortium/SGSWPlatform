# =============================================================================
#
# MobiCore Android build components
#
# =============================================================================

LOCAL_PATH := $(call my-dir)

# Registry Shared Library
# =============================================================================
include $(CLEAR_VARS)

LOCAL_MODULE := libMcRegistry
LOCAL_MODULE_TAGS := eng

LOCAL_CFLAGS += -DLOG_TAG=\"McRegistry\"
LOCAL_CFLAGS += -Wall -Wextra
LOCAL_CFLAGS += -DLOG_ANDROID

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SHARED_LIBRARIES := libMcClient
ifeq ($(APP_PROJECT_PATH),)
LOCAL_SHARED_LIBRARIES += liblog
else
# Local build
LOCAL_LDLIBS := -llog
endif

LOCAL_SRC_FILES := \
	src/Connection.cpp \
	src/Registry.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_EXPORT_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)

# Daemon Application
# =============================================================================
include $(CLEAR_VARS)

LOCAL_MODULE := mcDriverDaemon
LOCAL_MODULE_TAGS := eng
LOCAL_CFLAGS += -DLOG_TAG=\"McDaemon\"
LOCAL_CFLAGS += -DTBASE_API_LEVEL=5
LOCAL_CFLAGS += -Wall -Wextra
LOCAL_CFLAGS += -std=c++11
LOCAL_CFLAGS += -DLOG_ANDROID
ifdef TRUSTONIC_ANDROID_LEGACY_SUPPORT
LOCAL_CFLAGS += -DWITHOUT_PROXY
endif # TRUSTONIC_ANDROID_LEGACY_SUPPORT

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_STATIC_LIBRARIES := libMcClient_static
ifeq ($(APP_PROJECT_PATH),)
LOCAL_SHARED_LIBRARIES += \
	liblog

ifdef TRUSTONIC_ANDROID_LEGACY_SUPPORT
include external/stlport/libstlport.mk

LOCAL_C_INCLUDES += \
	external/stlport/stlport

LOCAL_SHARED_LIBRARIES += \
	libstlport
else # TRUSTONIC_ANDROID_LEGACY_SUPPORT
LOCAL_STATIC_LIBRARIES += \
	libMcProxy

LOCAL_SHARED_LIBRARIES += \
	libprotobuf-cpp-lite \
	libcutils

endif # !TRUSTONIC_ANDROID_LEGACY_SUPPORT
else # !NDK
# Local build
LOCAL_LDLIBS := -llog
ifndef TRUSTONIC_ANDROID_LEGACY_SUPPORT
LOCAL_CFLAGS += -static-libstdc++

LOCAL_STATIC_LIBRARIES += \
	libMcProxy \
	libprotobuf-cpp-lite
endif # !TRUSTONIC_ANDROID_LEGACY_SUPPORT
endif # NDK

LOCAL_SRC_FILES := \
	src/Connection.cpp \
	src/CThread.cpp \
	src/MobiCoreDriverDaemon.cpp \
	src/SecureWorld.cpp \
	src/FSD2.cpp \
	src/Server.cpp \
	src/PrivateRegistry.cpp

include $(BUILD_EXECUTABLE)

ifndef TRUSTONIC_ANDROID_LEGACY_SUPPORT

# Static version of the daemon for recovery

include $(CLEAR_VARS)

LOCAL_MODULE := mcDriverDaemon_static
LOCAL_MODULE_TAGS := eng
LOCAL_CFLAGS += -DLOG_TAG=\"McDaemon\"
LOCAL_CFLAGS += -DTBASE_API_LEVEL=5
LOCAL_CFLAGS += -Wall -Wextra
LOCAL_CFLAGS += -std=c++11
LOCAL_CFLAGS += -DLOG_ANDROID
LOCAL_CFLAGS += -DWITHOUT_FSD
LOCAL_CFLAGS += -DWITHOUT_PROXY

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_STATIC_LIBRARIES := libMcClient_static
ifeq ($(APP_PROJECT_PATH),)
LOCAL_STATIC_LIBRARIES += \
	liblog libc libc++_static libcutils

else # !NDK
# Local build
LOCAL_LDLIBS := -llog
endif # NDK

LOCAL_SRC_FILES := \
	src/Connection.cpp \
	src/CThread.cpp \
	src/MobiCoreDriverDaemon.cpp \
	src/SecureWorld.cpp \
	src/Server.cpp \
	src/PrivateRegistry.cpp

LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_EXECUTABLE)

endif # !TRUSTONIC_ANDROID_LEGACY_SUPPORT

# adding the root folder to the search path appears to make absolute paths
# work for import-module - lets see how long this works and what surprises
# future developers get from this.
$(call import-add-path,/)
$(call import-module,$(COMP_PATH_MobiCoreClientLib_module))
$(call import-module,$(COMP_PATH_AndroidProtoBuf))
