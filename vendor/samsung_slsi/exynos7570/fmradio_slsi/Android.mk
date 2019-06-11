LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libcutils libc \
	libui \
	libexpat \

LOCAL_SRC_FILES := \
        fmradio_test/FmRadioController_slsi.cpp \
        fmradio_test/FmRadioController_test.cpp

#LOCAL_LDLIBS := -lpthread

LOCAL_MODULE_TAGS := eng debug
LOCAL_MODULE := fmradio_app

include $(BUILD_EXECUTABLE)
