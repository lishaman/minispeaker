/*
 * kernel/drivers/video/panel/epd_panel/PVI_ED097OC5.c for PVI ED097OC5 panel
 *
 * According to the panel param to init the
 * struct fb_var_screeninfo and struct reg_timing for epd driver.
 *
 * Copyright (C) 2005-2013, Ingenic Semiconductor Inc.
 * http://www.ingenic.cn/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/pci.h>

#include <asm/irq.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/processor.h>
#include <asm/jzsoc.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <asm/cacheflush.h>
#include <linux/platform_device.h>


#include "../../jz4775_android_epd.h"

int wfm_trc;				/* waveform temperature region count */
int wfm_tr[] = {0x0, 0x3, 0x6, 0x9, 0xc, 0xf, 0x12, 0x15, 0x18, 0x1b, 0x1e, 0x21, 0x26, 0x2b, 0x30, 0x47};

struct fb_var_screeninfo fb_var_epd;
struct reg_timing reg_timing_epd ;

EXPORT_SYMBOL(wfm_trc);
EXPORT_SYMBOL(wfm_tr);
EXPORT_SYMBOL(fb_var_epd);
EXPORT_SYMBOL(reg_timing_epd);

int panel_info(void)
{
	wfm_trc = 0xd;

	/* epd panel param definition */
	unsigned int xres	= 2400;		/* resolution in x coordinate */
	unsigned int yres	= 1650;		/* resolution in y coordinate */
	unsigned int LSL	= 6;		/* line sync length */
	unsigned int LBL	= 6;		/* line begin length */
	unsigned int LDL	= 600;		/* line data length */
	unsigned int LEL	= 38;		/* line end length */
	unsigned int LGONL	= 606;		/* line gate on length? */
	unsigned int FSL	= 2;		/* frame sync length */
	unsigned int FBL	= 4;		/* frame begin length */
	unsigned int FDL	= 1650;		/* frame data length */
	unsigned int FEL	= 5;		/* frame end length */

	/* fb_var_epd init for fb_info->var of jz47xxfb_set_info in epd driver */
	fb_var_epd.xres			= xres;
	fb_var_epd.yres			= yres;

	fb_var_epd.width		= (xres * 0.0845);
	fb_var_epd.height		= (yres * 0.0845);

	fb_var_epd.nonstd		= 0;
	fb_var_epd.activate		= FB_ACTIVATE_NOW;
	fb_var_epd.accel_flags		= FB_ACCELF_TEXT;

	fb_var_epd.bits_per_pixel	= 16;
	fb_var_epd.grayscale		= 0;
	fb_var_epd.red.offset		= 11;
	fb_var_epd.red.length		= 5;
	fb_var_epd.red.msb_right	= 0;
	fb_var_epd.green.offset		= 5;
	fb_var_epd.green.length		= 6;
	fb_var_epd.green.msb_right	= 0;
	fb_var_epd.blue.offset		= 0;
	fb_var_epd.blue.length		= 5;
	fb_var_epd.blue.msb_right	= 0;
	fb_var_epd.transp.offset	= 0;
	fb_var_epd.transp.length	= 0;
	fb_var_epd.transp.msb_right	= 0;
	/* Timing: All values in pixclocks; except pixclock (of course) */
	//.pixclock		= KHZ2PICOS(__cpm_get_pixclk()/1000);	/* pixel clock in ps (pico seconds) */
	fb_var_epd.left_margin		= LBL;		/* time from sync to picture	*/
	fb_var_epd.right_margin		= LEL;		/* time from picture to sync	*/
	fb_var_epd.upper_margin		= FBL;		/* time from sync to picture	*/
	fb_var_epd.lower_margin		= FEL;
	fb_var_epd.hsync_len		= LSL;		/* length of horizontal sync	*/
	fb_var_epd.vsync_len		= FSL;		/* length of vertical sync	*/
	fb_var_epd.rotate		= 0;		/* angle we rotate counter clockwise */

	/* timing variable init for jz47xxfb_epd_controller_init registers in epd driver */
	reg_timing_epd.vt		= FSL + FBL + FDL + FEL;
	reg_timing_epd.ht		= LSL + LBL + LDL + LEL;
	reg_timing_epd.hpe		= LSL;
	reg_timing_epd.hps		= 0;
	reg_timing_epd.sdspe		= LSL + 5;
	reg_timing_epd.sdspd		= LSL + LBL;
	reg_timing_epd.hds		= LSL + LBL;
	reg_timing_epd.hde		= LSL + LBL + LDL;
	reg_timing_epd.sdoee		= LSL;
	reg_timing_epd.sdoed		= LSL + LBL + LDL + LEL;
	reg_timing_epd.gdclke		= LSL + (LBL / 2);
	reg_timing_epd.gdclkd		= LSL + (LBL / 2) + LGONL;
	reg_timing_epd.vps		= 0;
	reg_timing_epd.vpe		= FSL;
	reg_timing_epd.gdspe		= ((LSL + LBL + LDL + 1) / 2);
	reg_timing_epd.gdspd		= ((LSL + LBL + LDL + 1) / 2);
	reg_timing_epd.vds		= FSL + FBL;
	reg_timing_epd.vde		= FSL + FBL + FDL;
	reg_timing_epd.gdoee		= FSL;
	reg_timing_epd.gdoed		= FSL + FBL + FDL + FEL;

	return 0;
}
EXPORT_SYMBOL(panel_info);
