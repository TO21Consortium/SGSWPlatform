# Copy GPS libraries and binaries to the target directory
GPS_ROOT := vendor/samsung_slsi/gps_libs/release

PRODUCT_COPY_FILES += \
    $(GPS_ROOT)/HARRIER_ASIC_PATCH_RAM_IMAGE.bin:system/etc/HARRIER_ASIC_PATCH_RAM_IMAGE.bin \
    $(GPS_ROOT)/HARRIER_ASIC_PATCH_LPRAM_IMAGE.bin:system/etc/HARRIER_ASIC_PATCH_LPRAM_IMAGE.bin \
    $(GPS_ROOT)/lib_gnss.so:system/lib/hw/gps.default.so \
    $(GPS_ROOT)/gps.exynos.xml:system/etc/gps.xml
