# stop already exist process
kill $(ps | grep -v grep | grep udhcpc | grep wlan0 | awk '{print $3}') > /dev/null
#killall udhcpc > /dev/null
killall udhcpd > /dev/null
killall wpa_supplicant > /dev/null
killall hostapd  > /dev/null

ifconfig wlan0 up > /dev/null
sleep 1
ifconfig wlan0 down > /dev/null
