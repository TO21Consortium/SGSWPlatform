#
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
#

# video codecs
PRODUCT_PACKAGES := \
	libOMX.Exynos.MPEG4.Decoder \
	libOMX.Exynos.AVC.Decoder \
	libOMX.Exynos.WMV.Decoder \
	libOMX.Exynos.VP8.Decoder \
	libOMX.Exynos.HEVC.Decoder \
	libOMX.Exynos.MPEG4.Encoder \
	libOMX.Exynos.AVC.Encoder \
	libOMX.Exynos.VP8.Encoder \
	libOMX.Exynos.HEVC.Encoder

# stagefright and device specific modules
PRODUCT_PACKAGES += \
	libstagefrighthw \
	libExynosOMX_Core \
	libExynosOMX_Resourcemanager
