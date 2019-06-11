ifeq ($(TARGET_BOARD_PLATFORM),exynos3)

include $(call all-subdir-makefiles)

endif

ifeq ($(TARGET_BOARD_PLATFORM),exynos4)

include $(call all-subdir-makefiles)

endif

ifeq ($(TARGET_BOARD_PLATFORM),exynos5)

include $(call all-subdir-makefiles)

endif
