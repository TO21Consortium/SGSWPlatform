#!/system/bin/sh
dir=/sdcard/log
filename=${dir}/bt_dump_`date +%Y_%m_%d__%H_%M_%S`.log
samlog=/sys/kernel/debug/scsc/ring0/samlog

echo "Start BT_DUMP : LOGCAT" > $filename
logcat -v time -d >> $filename
echo "End BT_DUMP : LOGCAT\n" >> $filename
echo "Start BT_DUMP : Kernel LOG" >> $filename
dmesg >> $filename
echo "End BT_DUMP : Kernel LOG\n" >> $filename
echo "Start BT_DUMP : Driver+Firmware state" >> $filename
cat /proc/driver/scsc_bt/stats >> $filename
echo "End BT_DUMP : Driver+Firmware state\n" >> $filename

if [ -f $samlog ]
then
	echo "Start BT_DUMP : S.LSI LOG" >> $filename
	cat $samlog >> $filename
	echo "End BT_DUMP : S.LSI LOG\n" >> $filename
fi

exit 0
