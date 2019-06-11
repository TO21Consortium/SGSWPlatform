#/system/bin/sh
# Switch between flight mode and normal.
# On TouchWiz, BT is always on unless in flight mode.

# Exit on any error
set -e

if [ "$#" -eq 1 ] ; then
	if [ "$1" -eq 0 ] ; then
		echo "Disable flight mode"
		settings put global airplane_mode_on 0
		am broadcast -a android.intent.action.AIRPLANE_MODE --ez state false
	elif [ "$1" -eq 1 ] ; then
		echo "Enable flight mode"
		settings put global airplane_mode_on 1
		am broadcast -a android.intent.action.AIRPLANE_MODE --ez state true
	else
		echo "Invalid value $1 for input parameter"
		echo "One input parameter must be provided: 1 - flight mode, or 0 - normal mode"
	fi
else
	echo "One input parameter must be provided: 1 - flight mode, or 0 - normal mode"
fi
