#!/system/bin/sh

DIR=/data/misc/wifi
PIDFILE=${DIR}/mxlogger.pid
MXPROG=mxlogger

if [ ! -d ${DIR} ]; then
	mkdir -p ${DIR}
fi

if [ -f $PIDFILE ]; then
	pidn="`cat $PIDFILE`"
	exe_name="`readlink /proc/${pidn}/exe`"
	echo "$exe_name" | grep -q $MXPROG
	# Avoiding innocent victims
	[ x"$?" == "x0" ] && kill -9 $pidn
fi

mx_logger_dump.sh

$MXPROG -d ${DIR}

exit 0
