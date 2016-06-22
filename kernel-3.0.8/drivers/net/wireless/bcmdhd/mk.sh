#!/bin/sh
make -C /home/zrxu/android-4.0/pi3800/android-4.1/kernel/ M=`pwd` ARCH=mips CROSS_COMPILE=mips-linux-gnu- $1

