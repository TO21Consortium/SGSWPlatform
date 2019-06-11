LOCAL_PATH := $(call my-dir)

ifndef TRUSTONIC_ANDROID_LEGACY_SUPPORT

# Proxy server lib

include $(CLEAR_VARS)

LOCAL_MODULE := libMcProxy
LOCAL_MODULE_TAGS := eng

LOCAL_CFLAGS := -fvisibility=hidden
LOCAL_CFLAGS += -DTBASE_API_LEVEL=5
LOCAL_CFLAGS += -Wall -Wextra
LOCAL_CFLAGS += -std=c++11
LOCAL_CFLAGS += -DLOG_ANDROID
LOCAL_CFLAGS += -DGOOGLE_PROTOBUF_NO_RTTI

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/GP \
	external/protobuf/src

ifeq ($(APP_PROJECT_PATH),)
LOCAL_SHARED_LIBRARIES := \
	liblog \
	libprotobuf-cpp-lite
else
LOCAL_C_INCLUDES += \
	${COMP_PATH_AndroidProtoBuf}/Bin/host/include

LOCAL_STATIC_LIBRARIES := \
	libprotobuf-cpp-lite
endif

LOCAL_SRC_FILES := \
	src/driver_client.cpp \
	src/proxy_server.cpp \
	src/mc.pb.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_EXPORT_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)

endif # !TRUSTONIC_ANDROID_LEGACY_SUPPORT

# Client lib

include $(CLEAR_VARS)

LOCAL_MODULE := libMcClient
LOCAL_MODULE_TAGS := eng

LOCAL_CFLAGS := -fvisibility=hidden
LOCAL_CFLAGS += -DTBASE_API_LEVEL=5
LOCAL_CFLAGS += -Wall -Wextra
LOCAL_CFLAGS += -std=c++11
LOCAL_CFLAGS += -DLOG_ANDROID
ifndef TRUSTONIC_ANDROID_LEGACY_SUPPORT
LOCAL_CFLAGS += -DGOOGLE_PROTOBUF_NO_RTTI
else # !TRUSTONIC_ANDROID_LEGACY_SUPPORT
LOCAL_CFLAGS += -DWITHOUT_PROXY
endif # TRUSTONIC_ANDROID_LEGACY_SUPPORT

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/GP

ifeq ($(APP_PROJECT_PATH),)
LOCAL_SHARED_LIBRARIES := \
	liblog

ifdef TRUSTONIC_ANDROID_LEGACY_SUPPORT
include external/stlport/libstlport.mk

LOCAL_C_INCLUDES += \
	external/stlport/stlport

LOCAL_SHARED_LIBRARIES += \
	libstlport
else # TRUSTONIC_ANDROID_LEGACY_SUPPORT
LOCAL_C_INCLUDES += \
	external/protobuf/src

LOCAL_SHARED_LIBRARIES += \
	libprotobuf-cpp-lite
endif # !TRUSTONIC_ANDROID_LEGACY_SUPPORT
else # !NDK
LOCAL_LDLIBS := -llog

LOCAL_CFLAGS += -static-libstdc++

LOCAL_C_INCLUDES += \
	${COMP_PATH_AndroidProtoBuf}/Bin/host/include

LOCAL_STATIC_LIBRARIES := \
	libprotobuf-cpp-lite
endif # NDK

LOCAL_SRC_FILES := \
	src/common_client.cpp \
	src/driver_client.cpp \
	src/mc_client_api.cpp \
	src/tee_client_api.cpp

ifndef TRUSTONIC_ANDROID_LEGACY_SUPPORT
LOCAL_SRC_FILES += \
	src/proxy_client.cpp \
	src/mc.pb.cpp
endif # !TRUSTONIC_ANDROID_LEGACY_SUPPORT

LOCAL_EXPORT_CFLAGS := -DLOG_ANDROID
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_EXPORT_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)

# Static version of the client lib for recovery

include $(CLEAR_VARS)

LOCAL_MODULE := libMcClient_static
LOCAL_MODULE_TAGS := eng

LOCAL_CFLAGS := -fvisibility=hidden
LOCAL_CFLAGS += -DTBASE_API_LEVEL=5
LOCAL_CFLAGS += -Wall -Wextra
LOCAL_CFLAGS += -std=c++11
LOCAL_CFLAGS += -DLOG_ANDROID
LOCAL_CFLAGS += -DWITHOUT_PROXY

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/GP

ifdef TRUSTONIC_ANDROID_LEGACY_SUPPORT
include external/stlport/libstlport.mk

LOCAL_C_INCLUDES += \
	external/stlport/stlport

LOCAL_SHARED_LIBRARIES += \
	libstlport
endif # TRUSTONIC_ANDROID_LEGACY_SUPPORT

LOCAL_SRC_FILES := \
	src/common_client.cpp \
	src/driver_client.cpp \
	src/mc_client_api.cpp \
	src/tee_client_api.cpp

LOCAL_EXPORT_CFLAGS := -DLOG_ANDROID
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/include/GP
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_EXPORT_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)

# =============================================================================

# adding the root folder to the search path appears to make absolute paths
# work for import-module - lets see how long this works and what surprises
# future developers get from this.
$(call import-add-path,/)
$(call import-module,$(COMP_PATH_AndroidProtoBuf))
