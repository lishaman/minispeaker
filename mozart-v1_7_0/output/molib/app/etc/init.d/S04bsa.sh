#!/bin/bash

# start bsa server

case "$1" in
  start)
	echo -n "Starting bsa server ..."
	echo 1 > /sys/class/rfkill/rfkill0/state
	sleep 1

	bsa_server_mips -all=0 -r 14 -d /dev/ttyS1 -u /var/run/ -p /lib/firmware/BCM_bt_firmware.hcd &
	;;
  stop)
	echo -n "Stopping bsa server ..."
	killall bsa_server_mips
	echo 0 > /sys/class/rfkill/rfkill0/state
	;;
  restart|reload)
	;;
  *)
	echo "Usage: $0 {start|stop|restart}"
	exit 1
esac

exit $?
