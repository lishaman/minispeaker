#!/bin/sh

# stop already exist process
kill $(ps | grep -v grep | grep udhcpc | grep wlan0 | awk '{print $3}') > /dev/null
#killall udhcpc > /dev/null
killall udhcpd > /dev/null
killall wpa_supplicant > /dev/null
killall hostapd > /dev/null


# hostapd config file
AP_CONF=/tmp/hostapd.conf


# guess what wifi model we are using(light detect, may not match!!!)
INTERFACE=wlan0
DRIVER=nl80211

if [ -d /proc/net/rtl*/wlan? ]; then
	INTERFACE=`basename /proc/net/rtl*/wlan?`
	DRIVER=rtl871xdrv
fi

# get a lucky channel, ^_^.
while [ "$CHANNEL" == "0" -o "$CHANNEL" == "" ]; do
        CHANNEL=`expr $RANDOM % 12`
done

# delete default Gateway
route del default gw 0.0.0.0 dev $INTERFACE
# release ip address
ifconfig $INTERFACE 0.0.0.0

# get the last 8 char of wifi's MAC
# default,
# 1. use it as the default ssid suffix
# 2. use it as the default passwd
MACID=`ifconfig $INTERFACE | grep HWaddr | tr -s ' ' | cut -d' ' -f5 | tr -d : | cut -c5-12 | tr A-Z a-z`

if [ -f /usr/data/apinfo.ini ]; then
	ssid=`inirw -f /usr/data/apinfo.ini -r -s ap -k ssid`
	passwd=`inirw -f /usr/data/apinfo.ini -r -s ap -k passwd`
	encrypt=`inirw -f /usr/data/apinfo.ini -r -s ap -k encrypt`

	if [ xx$ssid == xx ]; then
		echo "Invalid ssid, create ap mode failed, exit..."
		exit
	else
		if [ xx$encrypt == xxyes -a xx$passwd == xx ]; then
			echo "Invalid password, create ap mode failed, exit..."
			exit
		fi
	fi
else
	ssid=`hostname`-$MACID
	passwd=$MACID
	encrypt=yes
fi

# make a complete hostap config file
cat > $AP_CONF << EOF
# basic setting
interface=$INTERFACE
driver=$DRIVER
channel=$CHANNEL
hw_mode=g
ssid=$ssid

EOF

if [ xx$encrypt == xxyes ]; then
cat >> $AP_CONF << EOF
# wpa or wpa2
auth_algs=3
wpa=3
wpa_passphrase=$passwd
wpa_key_mgmt=WPA-PSK

# encrypt method, Required!!!
# refer to:
# http://en.wikipedia.org/wiki/Wi-Fi_Protected_Access#Encryption_protocol
# wpa
rsn_pairwise=CCMP
# wpa encrypt method2
wpa_pairwise=CCMP

EOF
fi

cat /etc/hostapd.conf_sample >> $AP_CONF


# start service
ifconfig $INTERFACE up

hostapd $AP_CONF -B
ALINK=`echo $ssid | cut -d '_' -f 1`
if [ $ALINK == alink ]; then
	# set ip in ap mode
	ifconfig $INTERFACE 172.31.254.250
	# Add Gateway
	route add default gw 172.31.254.250
	udhcpd /etc/udhcpd-alink.conf
else
	# set ip in ap mode
	ifconfig $INTERFACE 192.168.8.8
	# Add Gateway
	route add default gw 192.168.8.8
	udhcpd /etc/udhcpd.conf
fi

# Add Multicast Router for Apple Airplay
route add -net 224.0.0.0 netmask 224.0.0.0 $INTERFACE

exit 0
