#!/system/bin/sh
# Enable disable WLAN f/w debug level
#
set -e

if [ "$#" -eq 1 ] ; then
	level=$1
	echo "Set WLAN debug to $level"
	for module in $(seq 0 1 46)
	do
		slsi_wlan_mib 5029.$module=$level
	done
else
	echo "Usage: wlan_debug_level.sh <level: 0 for silent>"
	echo "Control WLAN f/w debug level"
	echo "WLAN must be on before running the command"
fi
