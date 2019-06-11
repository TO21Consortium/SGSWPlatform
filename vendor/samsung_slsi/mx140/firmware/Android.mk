ifeq ($(CONFIG_SAMSUNG_SCSC_WIFIBT),true)

LOCAL_PATH := $(my-dir)

######### fw #############

include $(CLEAR_VARS)

LOCAL_MODULE := mx140_bin

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional

#IMAGE_FILES := $(wildcard $(LOCAL_PATH)/*/*)

IMAGE_FILES := $(shell find $(LOCAL_PATH)/mx140 -type f)
IMAGE_FILES_OUT := $(patsubst $(LOCAL_PATH)/mx140/%,$(TARGET_OUT)/etc/wifi/%,$(IMAGE_FILES))

$(TARGET_OUT)/etc/wifi/%: $(LOCAL_PATH)/mx140/%
	mkdir -p $(@D)
	cp $< $@

#include $(CLEAR_VARS)

ALL_DEFAULT_INSTALLED_MODULES += $(IMAGE_FILES_OUT)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########## localmib #############
# MIB to suppress debug only in user build
#ifeq ($(TARGET_BUILD_VARIANT),user)

#include $(CLEAR_VARS)
#LOCAL_MODULE := localmib_hcf
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := EXECUTABLES

#IMAGE_FILES := $(LOCAL_PATH)/localmib.hcf
#IMAGE_FILES_OUT := $(TARGET_OUT)/etc/wifi/mx140/conf/wlan/localmib.hcf

#$(TARGET_OUT)/etc/wifi/mx140/conf/wlan/localmib.hcf: $(LOCAL_PATH)/localmib.hcf
#	@echo "localmib: var: $(TARGET_BUILD_VARIANT), type: $(TARGET_BUILD_TYPE), $(@D), $<, $@"
#	mkdir -p $(@D)
#	cp $< $@

#ALL_DEFAULT_INSTALLED_MODULES += $(IMAGE_FILES_OUT)
#LOCAL_SRC_FILES := $(LOCAL_MODULE)
#include $(BUILD_PREBUILT)

#endif

include $(CLEAR_VARS)
LOCAL_MODULE := localmib_dbg_ena_hcf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES

IMAGE_FILES := $(LOCAL_PATH)/localmib_dbg_ena.hcf
IMAGE_FILES_OUT := $(TARGET_OUT)/etc/wifi/mx140/conf/wlan/localmib_dbg_ena.hcf

$(TARGET_OUT)/etc/wifi/mx140/conf/wlan/localmib_dbg_ena.hcf: $(LOCAL_PATH)/localmib_dbg_ena.hcf
	@echo "localmib_dbg_ena: var: $(TARGET_BUILD_VARIANT), type: $(TARGET_BUILD_TYPE), $(@D), $<, $@"
	mkdir -p $(@D)
	cp $< $@

ALL_DEFAULT_INSTALLED_MODULES += $(IMAGE_FILES_OUT)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)


# The below check prevents any new rules to be applied if nothing has been specified as products
# The default will take precedence.
ifneq ($(SCSC_WIFIBT_PRODUCT),)
include $(CLEAR_VARS)
LOCAL_MODULE := wifi_bt_product
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES

########## MIB overwrite ###########

$(TARGET_OUT)/etc/wifi/mx140/conf/wlan/wlan.hcf: $(LOCAL_PATH)/mx140/mx140/conf/$(SCSC_WIFIBT_PRODUCT)/wlan/wlan.hcf
	@echo "conf: product: $(SCSC_WIFIBT_PRODUCT), $(@D), $<, $@"
	mkdir -p $(@D)
	cp $< $@
	@echo "Overwriting the $(SCSC_WIFIBT_PRODUCT) conf files ... "

$(TARGET_OUT)/etc/wifi/mx140/conf/wlan/wlan_t.hcf: $(LOCAL_PATH)/mx140/mx140/conf/$(SCSC_WIFIBT_PRODUCT)/wlan/wlan_t.hcf
	@echo "conf: product: $(SCSC_WIFIBT_PRODUCT), $(@D), $<, $@"
	mkdir -p $(@D)
	cp $< $@
	@echo "Overwriting the $(SCSC_WIFIBT_PRODUCT) conf files ... "

$(TARGET_OUT)/etc/wifi/mx140_t/conf/wlan/wlan.hcf: $(LOCAL_PATH)/mx140/mx140_t/conf/$(SCSC_WIFIBT_PRODUCT)/wlan/wlan.hcf
	@echo "conf: product: $(SCSC_WIFIBT_PRODUCT), $(@D), $<, $@"
	mkdir -p $(@D)
	cp $< $@
	@echo "Overwriting the $(SCSC_WIFIBT_PRODUCT) conf files ... "

$(TARGET_OUT)/etc/wifi/mx140_t/conf/wlan/wlan_t.hcf: $(LOCAL_PATH)/mx140/mx140_t/conf/$(SCSC_WIFIBT_PRODUCT)/wlan/wlan_t.hcf
	@echo "conf: product: $(SCSC_WIFIBT_PRODUCT), $(@D), $<, $@"
	mkdir -p $(@D)
	cp $< $@
	@echo "Overwriting the $(SCSC_WIFIBT_PRODUCT) conf files ... "
endif

######### tools ###########

include $(CLEAR_VARS)
LOCAL_MODULE := enable_test_mode.sh
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := vectordriverbroker
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := vectordriverbroker.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := mx_logger.sh
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := mx_logger_dump.sh
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := bt_dump.sh
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

endif
