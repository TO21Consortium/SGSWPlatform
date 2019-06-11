ifeq ($(CONFIG_SAMSUNG_SCSC_WIFIBT),true)

LOCAL_PATH := $(my-dir)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := iw
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := iperf
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := busybox
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := moredump
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := trigger_moredump
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := disable_auto_coredump
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := moredump.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := omnicli
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := btcli
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := wlan_debug_level.sh
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := disable_wlbt_log
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := enable_wlbt_log
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := scsc_enable_flight_mode.sh
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := if_unifi.so
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#########################
include $(CLEAR_VARS)
LOCAL_MODULE := tgen
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := tool
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

ALL_DEFAULT_INSTALLED_MODULES += $(TARGET_OUT)/bin/ip $(TARGET_OUT)/bin/arp \
				 $(TARGET_OUT)/bin/arping $(TARGET_OUT)/bin/which $(TARGET_OUT)/bin/stty 

$(TARGET_OUT)/bin/ip:
	@echo "Creating Symlinks for busybox ip"
	@ln -sf /system/bin/busybox $(TARGET_OUT)/bin/ip

$(TARGET_OUT)/bin/arp:
	@echo "Creating Symlinks for busybox arp"
	@ln -sf /system/bin/busybox $(TARGET_OUT)/bin/arp

$(TARGET_OUT)/bin/arping:
	@echo "Creating Symlinks for busybox arping"
	@ln -sf /system/bin/busybox $(TARGET_OUT)/bin/arping

$(TARGET_OUT)/bin/which:
	@echo "Creating Symlinks for busybox which"
	@ln -sf /system/bin/busybox $(TARGET_OUT)/bin/which

$(TARGET_OUT)/bin/stty:
	@echo "Creating Symlinks for busybox stty"
	@ln -sf /system/bin/busybox $(TARGET_OUT)/bin/stty

endif
