/*
 *  Copyright (C) 2013 Fighter Sun <wanmyqawdr@126.com>
 *  Flash translation layer upon UBI subsystem
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/mtd/blktrans.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>

#include <linux/kernel.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/hdreg.h>
#include <linux/vmalloc.h>
#include <linux/blkpg.h>
#include <asm/uaccess.h>

static int __init ftl_init(void)
{
	return 0;
}

static void __exit ftl_exit(void)
{
}

rootfs_initcall(ftl_init);
module_exit(ftl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fighter Sun <wanmyqawdr@126.com>");
MODULE_DESCRIPTION("Flash Translation layer upon UBI subsystem");
