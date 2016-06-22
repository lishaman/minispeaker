#!/bin/sh

MDIR=/mnt/usb
source /etc/mozart.path

if [ $ACTION = "add" ];
then
	# At mdev -s state during boot.
	if [ -z $DEVTYPE ];
	then
		COUNT=`find /dev -name "sd*" | wc -l`
		if [ $COUNT -gt 1 ];
		then
			umount $MDIR > /dev/null 2>&1
		fi
		# Device has partition, not mount at disk node
	elif [ "$DEVTYPE" = "disk" -a $NPARTS -gt 0 ];
	then
		exit 0
	fi

	exec 1>/dev/console
	exec 2>/dev/console

	echo "Auto mount UDISK(/dev/$MDEV) to $MDIR"
	if ! mount /dev/$MDEV $MDIR; then
		exit 1
	fi

	# build essential dir right now.
	mkdir -p $MDIR/music/download
	mkdir -p $MDIR/music/playlist

	# notify localplayer udisk insert event.
	if type setshm > /dev/null; then
		setshm -d UDISK_DOMAIN -s STATUS_INSERT
	fi

	# notify dms udisk insert event.
	if type dms_refresh_on_storage_hotplug > /dev/null; then
		dms_refresh_on_storage_hotplug
	fi
else
	echo "umount UDISK from $MDIR"
	umount -l $MDIR

	# notify localplayer udisk extract event.
	if type setshm > /dev/null; then
		setshm -d UDISK_DOMAIN -s STATUS_EXTRACT
	fi

	# notify dms udisk extract event.
	if type dms_refresh_on_storage_hotplug > /dev/null; then
		dms_refresh_on_storage_hotplug
	fi
fi
