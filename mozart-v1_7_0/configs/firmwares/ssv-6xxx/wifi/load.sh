#!/bin/sh

echo "12582912" > /proc/sys/net/core/wmem_max
echo "12582912" > /proc/sys/net/core/rmem_max
echo "10240 87380 12582912" > /proc/sys/net/ipv4/tcp_rmem
echo "10240 87380 12582912" > /proc/sys/net/ipv4/tcp_wmem
echo "1" > /proc/sys/net/ipv4/tcp_window_scaling
echo "1" > /proc/sys/net/ipv4/tcp_timestamps
echo "1" > /proc/sys/net/ipv4/tcp_sack
echo "1" > /proc/sys/net/ipv4/tcp_no_metrics_save
echo "5000" > /proc/sys/net/core/netdev_max_backlog



mount -t debugfs none /sys/kernel/debug	> /dev/null 2>&1 

insmod /lib/firmware/ssv6051.ko stacfgpath=/lib/firmware/ssv6051-wifi.cfg cfgfirmwarepath=/lib/firmware/

while [ ! /sys/class/net/wlan0 ]; do
	echo "waiting for wifi driver insmod done."
	sleep 0.2
done
