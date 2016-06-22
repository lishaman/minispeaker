#!/bin/sh

MDIR=/mnt/sdcard
source /etc/mozart.path

if [ "$ACTION" = "add" ];
then
	# At mdev -s state during boot.
	if [ -z $DEVTYPE ];
	then
		COUNT=`find /dev -name "mmcblk*" | wc -l`
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

	echo "Auto mount SDCARD(/dev/$MDEV) to $MDIR"
	if ! mount /dev/$MDEV $MDIR; then
		exit 1
	fi

	# build essential dir right now.
	mkdir -p $MDIR/music/download
	mkdir -p $MDIR/music/playlist

	# notify localplayer sdcard insert event.
	if type setshm > /dev/null; then
		setshm -d SDCARD_DOMAIN -s STATUS_INSERT
	fi

	# notify dms sdcard insert event.
	if type dms_refresh_on_storage_hotplug > /dev/null; then
		dms_refresh_on_storage_hotplug
	fi

	if [ -e $MDIR/X1000_CANNA_hard_test_program/x1000_canna_hard_test_program.sh ]; then
		chmod +x $MDIR/X1000_CANNA_hard_test_program/x1000_canna_hard_test_program.sh
		$MDIR/X1000_CANNA_hard_test_program/x1000_canna_hard_test_program.sh
	fi
else
	echo "umount SDCARD from $MDIR"
	umount -l $MDIR

	# notify localplayer sdcard extract event.
	if type setshm > /dev/null; then
		setshm -d SDCARD_DOMAIN -s STATUS_EXTRACT
	fi

	# notify dms sdcard extract event.
	if type dms_refresh_on_storage_hotplug > /dev/null; then
		dms_refresh_on_storage_hotplug
	fi
fi
