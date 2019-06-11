#/system/bin/sh
# Switch between WLAN test and production mode

# Exit on any error
set -e

if [ "$#" -eq 1 ] ; then
	if [ "$1" -eq 0 ] ; then
		echo "Stopping WLAN, enabling production mode"
		# Stop any existing WLAN mode (belt and braces)
		svc wifi disable
		ifconfig wlan0 down
		ifconfig p2p0 down
		echo -n "mx140" > /sys/module/scsc_mx/parameters/firmware_variant
		echo 0 > /sys/module/scsc_wlan/parameters/EnableTestMode
		echo 1 > /sys/module/scsc_mx/parameters/enable_auto_sense
		echo N > /sys/module/scsc_bt/parameters/disable_service
		echo Y > /proc/driver/mx140_clk0/restart
		# WLAN should subsqeuently be turned on manually via framework
	elif [ "$1" -eq 1 ] ; then
		echo "Start WLAN in test mode"
		# Stop any existing WLAN mode (belt and braces)
		svc wifi disable
		ifconfig wlan0 down
		ifconfig p2p0 down
		echo 0 > /sys/module/scsc_mx/parameters/enable_auto_sense
		echo 1 > /sys/module/scsc_mx/parameters/use_new_fw_structure
		echo -n "mx140_t" > /sys/module/scsc_mx/parameters/firmware_variant
		echo 1 > /sys/module/scsc_wlan/parameters/EnableTestMode
		echo Y > /sys/module/scsc_bt/parameters/disable_service
		echo 0xDEADDEAD > /sys/module/scsc_bt/parameters/force_crash
		sleep 5
		echo Y > /proc/driver/mx140_clk0/restart
		# Start WLAN without Android framework, in test mode.
		ifconfig wlan0 up
	else
		echo "Invalid value $1 for input parameter"
		echo "One input parameter must be provided: 1 - test mode, or 0 - production mode"
	fi
else
	echo "One input parameter must be provided: 1 - test mode, or 0 - production mode"
fi
