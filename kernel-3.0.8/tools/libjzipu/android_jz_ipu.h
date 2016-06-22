/* hardware/xb4770/libjzipu/android_jz4780_ipu.h
 *
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Hal head file for Ingenic IPU device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ANDROID_JZ4780_IPU_H__
#define __ANDROID_JZ4780_IPU_H__

#include <linux/fb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPU_DEVNAME    "/dev/ipu"

#define IPU_STATE_OPENED           0x00000001
#define IPU_STATE_CLOSED           0x00000002
#define IPU_STATE_INITED           0x00000004
#define IPU_STATE_STARTED          0x00000008
#define IPU_STATE_STOPED           0x00000010

/* ipu output mode */
#define IPU_OUTPUT_TO_LCD_FG1           0x00000002
#define IPU_OUTPUT_TO_LCD_FB0           0x00000004
#define IPU_OUTPUT_TO_LCD_FB1           0x00000008
#define IPU_OUTPUT_TO_FRAMEBUFFER       0x00000010 /* output to user defined buffer */
#define IPU_OUTPUT_MODE_MASK            0x000000FF
#define IPU_DST_USE_COLOR_KEY           0x00000100
#define IPU_DST_USE_ALPHA               0x00000200
#define IPU_OUTPUT_BLOCK_MODE           0x00000400
#define IPU_OUTPUT_MODE_FG0_OFF         0x00000800
#define IPU_OUTPUT_MODE_FG1_TVOUT       0x00001000
#define IPU_DST_USE_PERPIXEL_ALPHA      0x00010000

enum {
	ZOOM_MODE_BILINEAR = 0,
	ZOOM_MODE_BICUBE,
	ZOOM_MODE_BILINEAR_ENHANCE,
};

#define IPU_LUT_LEN                      (32)

struct YuvCsc
{									// YUV(default)	or	YCbCr
	unsigned int csc0;				//	0x400			0x4A8
	unsigned int csc1;              //	0x59C   		0x662
	unsigned int csc2;              //	0x161   		0x191
	unsigned int csc3;              //	0x2DC   		0x341
	unsigned int csc4;              //	0x718   		0x811
	unsigned int chrom;             //	128				128
	unsigned int luma;              //	0				16
};

struct YuvStride
{
	unsigned int y;
	unsigned int u;
	unsigned int v;
	unsigned int out;
};

struct ipu_img_param
{
	unsigned int 		fb_w;
	unsigned int 		fb_h;
	unsigned int 		version;			/* sizeof(struct ipu_img_param) */
	int			        ipu_cmd;			/* IPU command by ctang 2012/6/25 */
	unsigned int        stlb_base;      /* IPU src tlb table base phys */
	unsigned int        dtlb_base;      /* IPU dst tlb table base phys */
	unsigned int		output_mode;		/* IPU output mode: fb0, fb1, fg1, alpha, colorkey and so on */
	unsigned int		alpha;
	unsigned int		colorkey;
	unsigned int		in_width;
	unsigned int		in_height;
	unsigned int		in_bpp;
	unsigned int		out_x;
	unsigned int		out_y;
	unsigned int		in_fmt;
	unsigned int		out_fmt;
	unsigned int		out_width;
	unsigned int		out_height;
	unsigned char*		y_buf_v;            /* Y buffer virtual address */
	unsigned char*		u_buf_v;
	unsigned char*		v_buf_v;
	unsigned int		y_buf_p;            /* Y buffer physical address */
	unsigned int		u_buf_p;
	unsigned int		v_buf_p;
	unsigned char*		out_buf_v;
	unsigned int		out_buf_p;
	unsigned int 		src_page_mapping;
	unsigned int 		dst_page_mapping;
	unsigned char*		y_t_addr;				// table address
	unsigned char*		u_t_addr;
	unsigned char*		v_t_addr;
	unsigned char*		out_t_addr;
	struct YuvCsc*		csc;
	struct YuvStride	stride;
	int 			    Wdiff;
	int 			    Hdiff;
	unsigned int		zoom_mode;

    int                 v_coef;
    int                 h_coef;
};

enum format_order {
	FORMAT_X8R8G8B8  =  1,
	FORMAT_X8B8G8R8  =  2,
};

struct jzlcd_fg_t {
	int fg;            /* 0, fg0  1, fg1 */
	int bpp;	/* foreground bpp */
	int x;		/* foreground start position x */
	int y;		/* foreground start position y */
	int w;		/* foreground width */
	int h;		/* foreground height */
	unsigned int alpha;     /* ALPHAEN, alpha value */
	unsigned int bgcolor;   /* background color value */
};

struct jzfb_osd_t {
	enum format_order fmt_order;  /* pixel format order */
	int decompress;			      /* enable decompress function, used by FG0 */
	int useIPUasInput;			  /* useIPUasInput, used by FG1 */

	unsigned int osd_cfg;	      /* OSDEN, ALHPAEN, F0EN, F1EN, etc */
	unsigned int osd_ctrl;	      /* IPUEN, OSDBPP, etc */
	unsigned int rgb_ctrl;	      /* RGB Dummy, RGB sequence, RGB to YUV */
	unsigned int colorkey0;	      /* foreground0's Colorkey enable, Colorkey value */
	unsigned int colorkey1;       /* foreground1's Colorkey enable, Colorkey value */
	unsigned int ipu_restart;     /* IPU Restart enable, ipu restart interval time */

	int line_length;              /* line_length, stride, in byte */

	struct jzlcd_fg_t fg0;
	struct jzlcd_fg_t fg1;	
};

struct mvom_data_info {
	int fbfd;
	unsigned int fb_width;
	unsigned int fb_height;
//	void *fb_v_base;			    /* virtual base address of frame buffer */
//	unsigned int fb_p_base;			/* physical base address of frame buffer */
	struct jzfb_osd_t fb_osd;
	struct fb_var_screeninfo fbvar;
//	struct fb_fix_screeninfo fbfix;
};

struct ipu_data_buffer {
	int size;				        /* buffer size */
	void *y_buf_v;				    /* virtual address of y buffer or base address */
	void *u_buf_v;
	void *v_buf_v;
	unsigned int y_buf_phys;		/* physical address of y buffer */
	unsigned int u_buf_phys;
	unsigned int v_buf_phys;
	int y_stride;
	int u_stride;
	int v_stride;
};

struct ipu_native_info {
	void *ipu_buf_base;
	int ipu_fd;
	int id;
	int fb0_lcdc_id;
	struct ipu_img_param img; /* Interface to ipu driver */
	struct mvom_data_info *fb_data[2];
	int ipu_inited;
	unsigned int state;

	/* ipu frame buffers */
	int data_buf_num;	/* 3 buffers in most case */
	struct ipu_data_buffer data_buf[3];
};

/* Source element */
struct source_data_info
{
	/* pixel format definitions in hardware/libhardware/include/hardware/hardware.h */
	int fmt;
	int width;
	int height;
	int is_virt_buf; /* virtual memory or continuous memory */
	unsigned int stlb_base;
	struct ipu_data_buffer srcBuf; /* data, stride */
};


/* Destination element */
struct dest_data_info
{
	unsigned int dst_mode;		/* ipu output to lcd's fg0 or fg1, ipu output to fb0 or fb1 */
	int fmt;			/* pixel format definitions in hardware/libhardware/include/hardware/hardware.h */
	unsigned int colorkey;		/* JZLCD FG0 colorkey */
	unsigned int alpha;		/* JZLCD FG0 alpha */
	int left;
	int top;
	int width;
	int height;
	int lcdc_id; /* no need to config it */
	unsigned int dtlb_base;
	void *out_buf_v;
	struct ipu_data_buffer dstBuf; /* data, stride */
};

struct ipu_image_info
{
	struct source_data_info src_info;
	struct dest_data_info dst_info;
	void * native_data;
};

#define JZIPU_IOC_MAGIC  'I'

/* ioctl command */
#define	IOCTL_IPU_SHUT               _IO(JZIPU_IOC_MAGIC, 102)
#define IOCTL_IPU_INIT               _IOW(JZIPU_IOC_MAGIC, 103, struct ipu_img_param)
#define IOCTL_IPU_SET_BUFF           _IOW(JZIPU_IOC_MAGIC, 105, struct ipu_img_param)
#define IOCTL_IPU_START              _IO(JZIPU_IOC_MAGIC, 106)
#define IOCTL_IPU_STOP               _IO(JZIPU_IOC_MAGIC, 107)
#define IOCTL_IPU_DUMP_REGS          _IO(JZIPU_IOC_MAGIC, 108)
#define IOCTL_IPU_SET_BYPASS         _IO(JZIPU_IOC_MAGIC, 109)
#define IOCTL_IPU_GET_BYPASS_STATE   _IOR(JZIPU_IOC_MAGIC, 110, int)
#define IOCTL_IPU_CLR_BYPASS         _IO(JZIPU_IOC_MAGIC, 111)
#define IOCTL_IPU_ENABLE_CLK         _IO(JZIPU_IOC_MAGIC, 112)
//#define IOCTL_GET_FREE_IPU       _IOR(JZIPU_IOC_MAGIC, 109, int)
#define IOCTL_IPU_GET_PBUFF			 _IO(JZIPU_IOC_MAGIC, 115)

/* ioctl commands to control LCD controller registers */
#define JZFB_GET_LCDC_ID	       	_IOR('F', 0x121, int)
#define JZFB_IPU0_TO_BUF			_IOW('F', 0x128, int)
#define JZFB_ENABLE_IPU_CLK			_IOW('F', 0x129, int)
#define JZFB_ENABLE_LCDC_CLK		_IOW('F', 0x130, int)

int ipu_open(struct ipu_image_info ** ipu_img); /* get a "struct ipu_image_info" handler*/
int ipu_close(struct ipu_image_info ** ipu_img);
int ipu_init(struct ipu_image_info * ipu_img);
int ipu_start(struct ipu_image_info * ipu_img); /* Block if IPU_OUTPUT_BLOCK_MODE set */
int ipu_dump_regs(struct ipu_image_info * ipu_img,int is_dump_reg);
int ipu_stop(struct ipu_image_info * ipu_img);
/* post video frame(YUV buffer) to ipu and start ipu, Block if IPU_OUTPUT_BLOCK_MODE set */
int ipu_postBuffer(struct ipu_image_info* ipu_img);
int isOsd2LayerMode(struct ipu_image_info * ipu_img);

/* match HAL_PIXEL_FORMAT_ in system/core/include/system/graphics.h */
enum {
	HAL_PIXEL_FORMAT_RGBA_8888    = 1,
	HAL_PIXEL_FORMAT_RGBX_8888    = 2,
	HAL_PIXEL_FORMAT_RGB_888      = 3,
	HAL_PIXEL_FORMAT_RGB_565      = 4,
	HAL_PIXEL_FORMAT_BGRA_8888    = 5,
	//HAL_PIXEL_FORMAT_BGRX_8888    = 0x8000, /* Add BGRX_8888, Wolfgang, 2010-07-24 */
	HAL_PIXEL_FORMAT_BGRX_8888  	= 0x1ff, /* 2012-10-23 */
	HAL_PIXEL_FORMAT_RGBA_5551    = 6,
	HAL_PIXEL_FORMAT_RGBA_4444    = 7,
	HAL_PIXEL_FORMAT_YCbCr_422_SP = 0x10,
	HAL_PIXEL_FORMAT_YCbCr_420_SP = 0x11,
	HAL_PIXEL_FORMAT_YCbCr_422_P  = 0x12,
	HAL_PIXEL_FORMAT_YCbCr_420_P  = 0x13,
	HAL_PIXEL_FORMAT_YCbCr_420_B  = 0x24,
	HAL_PIXEL_FORMAT_YCbCr_422_I  = 0x14,
	HAL_PIXEL_FORMAT_YCbCr_420_I  = 0x15,
	HAL_PIXEL_FORMAT_CbYCrY_422_I = 0x16,
	HAL_PIXEL_FORMAT_CbYCrY_420_I = 0x17,

	/* suport for YUV420 */
	HAL_PIXEL_FORMAT_JZ_YUV_420_P       = 0x47700001, // YUV_420_P
	HAL_PIXEL_FORMAT_JZ_YUV_420_B       = 0x47700002, // YUV_420_P BLOCK MODE
};

#ifdef __cplusplus
}
#endif

#endif  //__ANDROID_JZ4780_IPU_H__
