#!/bin/bash

mplayer -slave -quiet -ao oss -input file=/var/run/mplayer/tonefifo /usr/share/vr/tone/factory_reseting.mp3 > /dev/null

killall mozart
killall shairport
killall lapsule
killall key_manager
killall network_manager
killall player
killall ushare

umount -f /dev/mtdblock1
flash_erase -j /dev/mtd1 0 20
mount -t jffs2 /dev/mtdblock1 /usr/data/
cp /usr/share/data/* /usr/data/
sync

mplayer -slave -quiet -ao oss -input file=/var/run/mplayer/tonefifo /usr/share/vr/tone/factory_reset_success.mp3 > /dev/null

reboot

exit 0
