#
# Copyright (C) 2014 Samsung Electronics S.LSI
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
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Support 64/32 or 32/64
ifdef TARGET_2ND_ARCH
  include $(LOCAL_PATH)/64/Android.mk
else
# Support 32 Only
ifneq (,$(filter $(TARGET_CPU_VARIANT),cortex-a7 cortex-a15 cortex-a53 cortex-a57))
  #include 32 idiv for exynos3, exynos5 series
  include $(LOCAL_PATH)/32/Android.mk
else
  #include 32 default for exynos4 series
  $(error exynos4 is not supported)
endif
endif
