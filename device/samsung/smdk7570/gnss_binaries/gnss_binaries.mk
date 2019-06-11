# Copy GPS libraries and binaries to the target directory
GPS_ROOT := device/samsung/smdk7570/gnss_binaries/release

PRODUCT_COPY_FILES += \
    $(GPS_ROOT)/K140_ASIC_TRK.bin:system/etc/K140_ASIC_TRK.bin \
    $(GPS_ROOT)/lib_gnss.so:system/lib/hw/gps.default.so \
    $(GPS_ROOT)/smdk_lal.xml:system/etc/smdk_lal.xml \
    $(GPS_ROOT)/kplr_asic_sll_cfg.xml:system/etc/gps.exynos.xml \
    $(GPS_ROOT)/ca.pem:system/etc/ca.pem
