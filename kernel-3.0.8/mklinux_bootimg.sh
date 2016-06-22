#!/bin/bash

make zImage -j8
rm boot.img

../../../binaries/rootfs/rootfs-dorado/ramdisk_for_linux/build_linux/mkbootimg  --kernel arch/mips/boot/compressed/zImage --ramdisk ../../../binaries/rootfs/rootfs-dorado/ramdisk_for_linux/ramdisk.cpio.img  --base 0x80000000  --output boot.img

echo "make $PWD/boot.img ok !"
