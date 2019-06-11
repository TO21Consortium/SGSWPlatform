# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

pattern_locales := \
    en-US/en-us \
    Ethi/und-ethi \
    eu/eu \
    hu/hu \
    hy/hy \
    nb/nb \
    nn/nn

# TODO: we have data for sa/sa, but it requires special case handling for case
# folding and normalization, so don't build it until that's fixed.

BUILD_HYB := $(LOCAL_PATH)/build-hyb.mk

#############################################################################
# $(1): The subdirectory where the source files live.
$ $(2): The file name fragment.
#       It is used to find source files, and also generate the resulting binary.
#############################################################################
define build-one-pattern-module
$(eval include $(CLEAR_VARS))\
$(eval LOCAL_MODULE := $(addprefix hyph-, $(2)))\
$(eval LOCAL_SRC_FILES := $(addprefix $(1)/hyph-, $(addprefix $(2), .pat.txt .chr.txt .hyp.txt)))\
$(eval include $(BUILD_HYB))\
$(eval include $(CLEAR_VARS))\
$(eval LOCAL_MODULE := $(addprefix $(addprefix hyph-, $(2)), .lic.txt))\
$(eval LOCAL_SRC_FILES := $(addprefix $(1)/hyph-, $(addprefix $(2), .lic.txt)))\
$(eval LOCAL_MODULE_CLASS := ETC)\
$(eval LOCAL_MODULE_TAGS := optional)\
$(eval LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/hyphen-data)\
$(eval include $(BUILD_PREBUILT))
endef

$(foreach l, $(pattern_locales), $(call build-one-pattern-module, $(dir $(l)), $(notdir $l)))
build-one-pattern-module :=
pattern_locales :=
