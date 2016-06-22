#!/bin/sh

create_dir()
{
    if [ ! -d $1 ];then
        mkdir -m 755 $1
    fi
}

rebuild_usrdata()
{
    flash_erase -j /dev/mtd1 0 19
    mount -t USRDATAFS /dev/mtdblock1 /usr/data/
    cp /usr/share/data/* usr/data/ -a
}

echo "Setting hostname ..."
hostname -F /etc/hostname

echo "Mounting all filesystems ... "
mount -t tmpfs tmpfs /dev
create_dir /dev/pts
create_dir /dev/shm
create_dir /dev/input
mount -a

create_dir /lib/modules/`uname -r`

mknod /dev/null c 1 3
mknod /dev/zero c 1 5
mknod /dev/console c 5 1

ifconfig lo up

echo "Starting mdev ..."
echo /sbin/mdev > /proc/sys/kernel/hotplug
mdev -s

echo "Mounting /usr/data as rw filesystem..."
mount -t USRDATAFS /dev/mtdblock1 /usr/data/
if [ $? -ne 0 ]; then
    rebuild_usrdata
fi

if ota_state; then
    echo "Mounting /usr/fs as app filesystem..."
    mount -t USRFSFS /dev/mtdblock4 /usr/fs
fi

create_dir /var/run/wpa_supplicant
create_dir /var/run/hostapd
create_dir /var/run/mplayer
create_dir /var/run/player-server
create_dir /var/run/doug
create_dir /var/lib/misc
create_dir /mnt/usb
create_dir /mnt/sdcard
bt_type=`inirw -f /usr/data/system.ini -r -s product -k bt`
if [ $bt_type = "rtk" ]; then
    create_dir /var/run/dbus
    create_dir /var/lib/dbus
elif [ $bt_type = "bcm" ]; then
    create_dir /usr/data/bsa
elif [ $bt_type = "Undefined" ]; then
    echo "BT funcs are closed!!!"
else
    echo "$bt_type: Unsupported bt type!!!!"
fi

if [ ! -f /var/lib/misc/udhcpd.lease ];then
    /bin/touch /var/lib/misc/udhcpd.lease
fi
if [ ! -f /var/run/mplayer/infifo ];then
    /usr/bin/mkfifo -m 666 /var/run/mplayer/infifo
fi
if [ ! -f /var/run/mplayer/infifo ];then
    /usr/bin/mkfifo -m 666 /var/run/mplayer/outfifo
fi
audio_type=`inirw -f /usr/data/system.ini -r -s audio -k type`
volume=`inirw -f /usr/data/system.ini -r -s volume -k music`
if [ $audio_type = "alsa" ]; then
    amixer cset numid=13,iface=MIXER,name='AOHP Vmux' 0
    amixer cset numid=15,iface=MIXER,name='AOHP Mux' 5
elif [ $audio_type = "oss" ]; then
    : #do nothing.
else
    echo -e "$0: Unsupport audio type!!!!!!!!!!!!!!\n\n\n\n\n\n\n\n"
fi

if [ $volume = "" ]; then
    echo "$0: Cannot get volume value, force to 20."
    volume=20
fi
volume -w $volume
ifconfig wlan0 up
welcome &
