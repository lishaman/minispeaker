/*
 * kernel/drivers/video/jz4775_android_epd.h
 *
 * Copyright (C) 2005-2013, Ingenic Semiconductor Inc.
 * http://www.ingenic.cn/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __JZ4775_ANDROID_EPD_H__
#define __JZ4775_ANDROID_EPD_H__

#include <linux/fb.h>
#include <linux/earlysuspend.h>
#include "epdc_regs.h"

#ifdef CONFIG_TWO_FRAME_BUFFERS
#define NUM_FRAME_BUFFERS 2
#endif

#ifdef CONFIG_THREE_FRAME_BUFFERS
#define NUM_FRAME_BUFFERS 3
#endif

typedef enum {
	MODE_INIT,
	MODE_DU,
	MODE_GC16,
	MODE_EMPTY,
	MODE_A2,
	MODE_GL16,
	MODE_A2IN,
	MODE_A2OUT,
	MODE_GC16GU,
	MODE_AUTO_DU,
	MODE_AUTO_DUGC4,
}epd_mode_e;

typedef enum{
	PARALLEL_REF,
	PARTIAL_REF
}ref_mode_e;

struct compression_lut{
	unsigned int subscript;
	unsigned int number;
};

struct jz_epd {
	int id;
	int is_enabled;   /* 0:disable  1:enable */
	int irq;          /* EPDC interrupt num */

	int open_cnt;
	int irq_cnt;

	char clk_name[16];
	char pclk_name[16];
	char irq_name[16];

	struct fb_info *fb;
	struct device *dev;
	struct jz_epd_platform_data *pdata;
	void __iomem *base;
	struct resource *mem;

	size_t vidmem_size;
	void *vidmem;
	dma_addr_t vidmem_phys;

	int frm_size;
	int current_buffer;

	/* Hardware Vsync */
	wait_queue_head_t vsync_wq;
	ktime_t	vsync_timestamp;
	struct task_struct *vsync_thread;
	int timer_vsync;

	int epd_power;
	struct regulator *vblk;/* eink backlight power */

	struct mutex lock;
	struct mutex suspend_lock;

	/* epdc clk and pix clk */
	struct clk *epdc_clk;
	struct clk *pclk;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	int is_suspend;
};

static inline unsigned long reg_read(struct jz_epd *jz_epd, int offset)
{
	return readl(jz_epd->base + offset);
}

static inline void reg_write(struct jz_epd *jz_epd, int offset, unsigned long val)
{
	writel(val, jz_epd->base + offset);
}

/* ioctl commands base fb.h FBIO_XXX */
#define JZFB_ENABLE				_IOW('F', 0x104, int)
/* Reserved for future extend */
#define JZFB_SET_VSYNCINT		_IOW('F', 0x210, int)
#define JZFB_SET_PAN_SYNC		_IOW('F', 0x220, int)
#define JZFB_SET_NEED_SYSPAN	_IOR('F', 0x310, int)
#define NOGPU_PAN				_IOR('F', 0x311, int)
/* Jz EPDFB supported I/O controls. */
#define FBIOGET_FBSCREENINFO0		0x4619
#define FBIOGET_FBSCREENINFO1		0x461a

#define FBIOSETBACKLIGHT		0x4688 /* set back light level */
#define FBIODISPON				0x4689 /* display on */
#define FBIODISPOFF				0x468a /* display off */
#define FBIORESET				0x468b /* lcd reset */
#define FBIOPRINT_REG			0x468c /* print lcd registers(debug) */
#define FBIOROTATE				0x46a0 /* rotated fb */
#define FBIOGETBUFADDRS			0x46a1 /* get buffers addresses */
#define FBIO_GET_MODE			0x46a2 /* get lcd info */
#define FBIO_SET_MODE			0x46a3 /* set osd mode */
#define FBIO_DEEP_SET_MODE		0x46a4 /* set panel and osd mode */
#define FBIO_MODE_SWITCH		0x46a5 /* switch mode between EPD and TVE */
#define FBIO_GET_TVE_MODE		0x46a6 /* get tve info */
#define FBIO_SET_TVE_MODE		0x46a7 /* set tve mode */
#define FBIODISON_FG			0x46a8 /* FG display on */
#define FBIODISOFF_FG			0x46a9 /* FG display on */
#define FBIO_SET_EPD_TO_TVE		0x46b0 /* set lcd to tve mode */
#define FBIO_SET_FRM_TO_EPD		0x46b1 /* set framebuffer to lcd */
#define FBIO_SET_IPU_TO_EPD		0x46b2 /* set ipu to lcd directly */
#define FBIO_CHANGE_SIZE		0x46b3 /* change FG size */
#define FBIO_CHANGE_POSITION	0x46b4 /* change FG starts position */
#define FBIO_SET_BG_COLOR		0x46b5 /* set background color */
#define FBIO_SET_IPU_RESTART_VAL	0x46b6 /* set ipu restart value */
#define FBIO_SET_IPU_RESTART_ON		0x46b7 /* set ipu restart on */
#define FBIO_SET_IPU_RESTART_OFF	0x46b8 /* set ipu restart off */
#define FBIO_ALPHA_ON			0x46b9 /* enable alpha */
#define FBIO_ALPHA_OFF			0x46c0 /* disable alpha */
#define FBIO_SET_ALPHA_VAL		0x46c1 /* set alpha value */
#define FBIO_WAIT_EPD_SWAP_SIGNAL 	0x46c2 /* wait until end of frambuffer interrupt signal,Rill add for surfaceFlinger 100719*/
#define FBIO_GET_EINK_WRITE_MODE 	0x46c3 /* get epd write mode */
#define FBIO_SET_EINK_WRITE_MODE 	0x46c4 /* set epd write mode */
#define FBIO_SET_EINK_WRITE_REGION 	0x46c5
#define FBIO_GET_PANEL_TYPE	 	0x46c6
#define FBIO_GET_WFM_LOTNUM		0x46c7
#define FBIO_SET_EINK_FRAME_RATE 	0x46c8
#define FBIO_GET_EINK_REFRESH_MODE 	0x46c9 /* get epd refresh mode */
#define FBIO_SET_EINK_REFRESH_MODE 	0x46ca /* set epd refresh mode */
#define FBIO_SET_EINK_CLEAR_SCREEN	0x46cb /* clear screen 0:white 1:black */
#define FBIO_GET_EINK_PARALLEL_REF_TIMES 0x46cc	/* get eink parallel refresh interval times */
#define FBIO_SET_EINK_PARALLEL_REF_TIMES 0x46cd	/* set eink parallel refresh interval times */
#define FBIO_GET_BACKLIGHT_POWER_STATUS  0x46ce /* eink get backlight power status 0:is power off 1:is power on */
#define FBIO_SET_BACKLIGHT_POWER		 0x46cf	/* eink set backlight power 0:power off 1:power on */
#define FBIO_REFRESH_FB					 0x46d0	/* refresh fb, can exec pan_display function */
#define FBIO_REFRESH_A2OUT				 0x46d1	/* if epd_mode is MODE_A2, you can refresh all by FBIO_REFRESH_A2OUT */

#endif /* __JZ4775_ANDROID_EPD_H__ */
