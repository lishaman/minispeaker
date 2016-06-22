/*
 * kernel/drivers/video/panel/epd_panel/PVI_ED097OC1.c for PVI ED097OC1 panel
 *
 * According to the panel param to init the struct epd_panel_info for epd driver.
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

#include "epd_panel_info.h"

/*
 * GDRL		GPC0
 * GDSP		GPC1
 * GDOE		GPC2
 * SDSHR	GPC3
 * SDDO0	GPC4
 * SDDO1	GPC5
 * SDDO2	GPC6
 * SDDO3	GPC7
 * SDCK		GPC8
 * SDOE		GPC9
 * SDCEL0	GPC10
 * SDCEL1	GPC11
 * SDDO4	GPC12
 * SDDO5	GPC13
 * SDDO6	GPC14
 * SDDO7	GPC15
 * SDLE		GPC18
 * GDCLK	GPC19
 */
static int epd_pin_mapping()
{
	REG_GPIO_PXINTC(2) = 0x000cffff;
	REG_GPIO_PXMASKC(2)  = 0x000cffff;
	REG_GPIO_PXPAT0C(2) = 0x000cffff;
	REG_GPIO_PXPAT1C(2) = 0x000cffff;
}

struct epd_panel_info epd_panel_info_init(void)
{
	/* epd panel param definition */
	unsigned int xres	= 1200;		/* resolution in x coordinate */
	unsigned int yres	= 825;		/* resolution in y coordinate */
	unsigned int LSL	= 6;		/* line sync length */
	unsigned int LBL	= 6;		/* line begin length */
	unsigned int LDL	= 300;		/* line data length */
	unsigned int LEL	= 38;		/* line end length */
	unsigned int LGONL	= 306;		/* line gate on length? */
	unsigned int FSL	= 2;		/* frame sync length */
	unsigned int FBL	= 4;		/* frame begin length */
	unsigned int FDL	= 825;		/* frame data length */
	unsigned int FEL	= 5;		/* frame end length */

	unsigned int wtr[20] = {0x0, 0x3, 0x6, 0x9, 0xc, 0xf, 0x12, 0x15, 0x18, 0x1b, 0x1e, 0x21, 0x26, 0x2b, 0x30, 0x47};

	struct epd_timing oc1_tm;
	struct fb_var_screeninfo oc1_var;
	struct epd_panel_info oc1_info;

	epd_pin_mapping();
	strcpy(oc1_info.model, "PVI_ED097OC1");
	oc1_info.wfm_trc = 0xd;	/* waveform temperature region count, this value could smaller than the real region count */
	memcpy(oc1_info.wfm_tr,	wtr, sizeof(wtr));	/* waveform temperature region */

	/* oc1_tm struct init for jz47xxfb_epd_controller_init registers in epd driver */
	oc1_tm.vt	= FSL + FBL + FDL + FEL;
	oc1_tm.ht	= LSL + LBL + LDL + LEL;
	oc1_tm.hpe	= LSL;
	oc1_tm.hps	= 0;
	oc1_tm.sdspe	= LSL + 5;
	oc1_tm.sdspd	= LSL + LBL;
	oc1_tm.hds	= LSL + LBL;
	oc1_tm.hde	= LSL + LBL + LDL;
	oc1_tm.sdoee	= LSL;
	oc1_tm.sdoed	= LSL + LBL + LDL + LEL;
	oc1_tm.gdclke	= LSL + (LBL / 2);
	oc1_tm.gdclkd	= LSL + (LBL / 2) + LGONL;
	oc1_tm.vps	= 0;
	oc1_tm.vpe	= FSL;
	oc1_tm.gdspe	= ((LSL + LBL + LDL + 1) / 2);
	oc1_tm.gdspd	= ((LSL + LBL + LDL + 1) / 2);
	oc1_tm.vds	= FSL + FBL;
	oc1_tm.vde	= FSL + FBL + FDL;
	oc1_tm.gdoee	= FSL;
	oc1_tm.gdoed	= FSL + FBL + FDL + FEL;

	/* oc1_var init for fb_info->var of jz47xxfb_set_info() in epd driver */
	oc1_var.xres		= xres;
	oc1_var.yres		= yres;
	oc1_var.width		= (xres * 0.169);
	oc1_var.height		= (yres * 0.169);
	oc1_var.nonstd		= 0;
	oc1_var.activate	= FB_ACTIVATE_NOW;
	oc1_var.accel_flags	= FB_ACCELF_TEXT;
	oc1_var.bits_per_pixel	= 16;
	oc1_var.grayscale	= 0;
	oc1_var.red.offset	= 11;
	oc1_var.red.length	= 5;
	oc1_var.red.msb_right	= 0;
	oc1_var.green.offset	= 5;
	oc1_var.green.length	= 6;
	oc1_var.green.msb_right	= 0;
	oc1_var.blue.offset	= 0;
	oc1_var.blue.length	= 5;
	oc1_var.blue.msb_right	= 0;
	oc1_var.transp.offset	= 0;
	oc1_var.transp.length	= 0;
	oc1_var.transp.msb_right = 0;
	/* Timing: All values in pixclocks; except pixclock (of course) */
	//.pixclock		= KHZ2PICOS(__cpm_get_pixclk()/1000);	/* pixel clock in ps (pico seconds) */
	oc1_var.left_margin	= LBL;		/* time from sync to picture	*/
	oc1_var.right_margin	= LEL;		/* time from picture to sync	*/
	oc1_var.upper_margin	= FBL;		/* time from sync to picture	*/
	oc1_var.lower_margin	= FEL;
	oc1_var.hsync_len	= LSL;		/* length of horizontal sync	*/
	oc1_var.vsync_len	= FSL;		/* length of vertical sync	*/
	oc1_var.rotate		= 0;		/* angle we rotate counter clockwise */

	oc1_info.epd_tm		= oc1_tm;
	oc1_info.epd_var	= oc1_var;
	return oc1_info;
}
