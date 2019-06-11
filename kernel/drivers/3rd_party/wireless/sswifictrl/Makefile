# bcmdhd
#-DSET_RANDOM_MAC_SOFTAP
#
#  -DGET_CUSTOM_MAC_ENABLE
EXTRA_CFLAGS = -Idrivers/3rd_party/wireless/bcmdhd -Idrivers/3rd_party/wireless/bcmdhd/include   \
		-Idrivers/3rd_party/wireless/bcmdhd/common/include \
		-Iarch/arm/plat-samsung/include \
		-Iarch/arm

obj-$(CONFIG_SAMSUNG_WIFI_CONTROL) += dhd_exynos7420_platdev.o 
