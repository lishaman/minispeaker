#!/bin/sh
#
# Start the adbd....
#

case "$1" in
	start)
		echo "Starting adbd..."
		echo "adb,mass_storage" > /sys/class/android_usb/android0/functions
		echo 1 > /sys/class/android_usb/android0/enable
		adbd &
		;;
	stop)
		echo -n "Stopping adbd..."
		echo 0 > /sys/class/android_usb/android0/enable
		killall adbd
		sleep 1
		;;
	restart|reload)
		"$0" stop
		"$0" start
		;;
	*)
		echo "Usage: $0 {start|stop|restart}"
		exit 1
esac

exit $?
