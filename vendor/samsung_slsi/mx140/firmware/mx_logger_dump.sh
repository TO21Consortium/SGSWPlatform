#!/system/bin/sh

dir=/data/misc/wifi/log
[ $# -gt 1 -a x"$1" == "x-d" ] && dir=$2
filename=${dir}/mx.dump.`date +%Y_%m_%d__%H_%M_%S`.log
max_logs=4
SAMLOG=/sys/kernel/debug/scsc/ring0/samlog
ENABLE_LOGENTRY="/sys/module/scsc_logring/parameters/enable"

mkdir -p $dir
## Getting rid of old logs
sync
nlogs="`ls ${dir}/mx.dump* 2>/dev/null | wc -l`"
while [ $nlogs -ge $max_logs ]
do
	oldest="`ls ${dir}/mx.dump* | head -n 1`"
	rm -f $oldest
	nlogs="`ls ${dir}/mx.dump* | wc -l`"
done
sync

if [ -f $SAMLOG ]
then
	cat /proc/driver/mxman_info/mx_release > $filename
	LOGRING_WAS_ENABLED="`cat $ENABLE_LOGENTRY`"
	[ "x$LOGRING_WAS_ENABLED" != "x0" ] && echo 0 > $ENABLE_LOGENTRY
	cat $SAMLOG | mxdecoder >> $filename
	[ "x$LOGRING_WAS_ENABLED" != "x0" ] && echo 1 > $ENABLE_LOGENTRY
fi
sync

exit 0
