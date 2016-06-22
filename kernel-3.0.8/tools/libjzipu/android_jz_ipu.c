/* hardware/xb4770/libjzipu/android_jz4780_ipu.c
 *
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Hal file for Ingenic IPU device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define LOG_TAG "JzIPUHardware"
#include "Log.h"
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "sys/stat.h"
#ifdef HAVE_ANDROID_OS
#include "fcntl.h"
#else
#include "sys/fcntl.h"
#endif
#include "sys/ioctl.h"
#include "sys/mman.h"
#include <linux/fb.h>
#include <unistd.h>

#include "android_jz_ipu.h"

#define LOGD(fmt,args...) ((void)0);
#define LOGE(fmt, args...) printf(fmt, ##args)

#define WORDALIGN(x) (((x) + 3) & ~0x3)
//#define IPU_DBG
#define IPU_SCALE_FACTOR 512.0

static int ipu_open_cnt;
#ifdef IPU_DBG 
static void dump_ipu_img_param(struct ipu_img_param *t)
{
	printf("-----------------------------\n");
	printf("ipu_cmd = %x\n", t->ipu_cmd);
	printf("lcdc_id = %d\n", t->lcdc_id);
	printf("output_mode = %x\n", t->output_mode);
	printf("in_width = %x(%d)\n", t->in_width, t->in_width);
	printf("in_height = %x(%d)\n", t->in_height, t->in_height);
	printf("in_bpp = %x\n", t->in_bpp);
	printf("in_fmt = %x\n", t->in_fmt);
	printf("out_fmt = %x\n",t->out_fmt);
	printf("out_x = %x\n",t->out_x);
	printf("out_y = %x\n",t->out_y);
	printf("out_width = %x(%d)\n", t->out_width, t->out_width);
	printf("out_height = %x(%d)\n", t->out_height, t->out_height);
	printf("y_buf_v = %x\n", (unsigned int)t->y_buf_v);
	printf("u_buf_v = %x\n", (unsigned int)t->u_buf_v);
	printf("v_buf_v = %x\n", (unsigned int)t->v_buf_v);
	printf("out_buf_v = %x\n", (unsigned int)t->out_buf_v);
	printf("y_buf_p = %x\n", (unsigned int)t->y_buf_p);
	printf("u_buf_p = %x\n", (unsigned int)t->u_buf_p);
	printf("v_buf_p = %x\n", (unsigned int)t->v_buf_p);
	printf("out_buf_p = %x\n", (unsigned int)t->out_buf_p);
	printf("src_page_mapping = %x\n", (unsigned int)t->src_page_mapping);
	printf("dst_page_mapping = %x\n", (unsigned int)t->dst_page_mapping);
	printf("y_t_addr = %x\n", (unsigned int)t->y_t_addr);
	printf("u_t_addr = %x\n", (unsigned int)t->u_t_addr);
	printf("v_t_addr = %x\n", (unsigned int)t->v_t_addr);
	printf("out_t_addr = %x\n", (unsigned int)t->out_t_addr);
	printf("csc = %x\n", (unsigned int)t->csc);
	printf("stride.y = %x\n", (unsigned int)t->stride.y);
	printf("stride.u = %x\n", (unsigned int)t->stride.u);
	printf("stride.v = %x\n", (unsigned int)t->stride.v);
	printf("stride.out = %x\n", (unsigned int)t->stride.out);
	printf("Wdiff = %x\n", (unsigned int)t->Wdiff);
	printf("Hdiff = %x\n", (unsigned int)t->Hdiff);
	printf("zoom_mode = %x\n", (unsigned int)t->zoom_mode);
	printf("hcoef = %x\n", (unsigned int)t->h_coef);
	printf("vcoef = %x\n", (unsigned int)t->v_coef);
	printf("-----------------------------\n");
}
#endif

int isOsd2LayerMode(struct ipu_image_info *ipu_img)
{
	struct dest_data_info *dst;
	
	dst = &ipu_img->dst_info;

	if ((dst->dst_mode & IPU_OUTPUT_TO_LCD_FG1) && 
		!(dst->dst_mode & IPU_OUTPUT_MODE_FG0_OFF)) 
		return 1;
	
	return 0;
}

static int ipu_resize(struct ipu_native_info *ipu, int *outWp, int *outHp, int *Wdiff, int *Hdiff)
{
	struct ipu_img_param *img;
	unsigned int rsize_w = 0, rsize_h = 0;
    float h_coef = 0, v_coef = 0;
	img = &ipu->img;
	if (img->output_mode & IPU_OUTPUT_MODE_FG1_TVOUT) {
		rsize_w = img->out_width;
		rsize_h = img->out_height;
	} else if (img->output_mode & IPU_OUTPUT_TO_LCD_FG1) {
		LOGD("Fix me, ipu output to lcd fg1");
		rsize_w = img->out_width;
		rsize_h = img->out_height;
	} else {
		rsize_w = img->out_width;
		rsize_h = img->out_height;
	}

	*Wdiff = *Hdiff = 0;

	img->out_height = rsize_h;
	img->out_width	= rsize_w;

	h_coef = (float)img->in_width / (float)img->out_width;
	v_coef = (float)img->in_height / (float)img->out_height;

	img->h_coef = (int)(h_coef * IPU_SCALE_FACTOR);
	img->v_coef = (int)(v_coef * IPU_SCALE_FACTOR);

	return (0);
}

static int jz47_ipu_init(struct ipu_image_info *ipu_img)
{
	int ret = 0;

	if (ipu_img == NULL) {
		LOGE("ipu_img is NULL\n");
		return -1;
	}

	struct source_data_info * src = &ipu_img->src_info;
	struct dest_data_info *dst = &ipu_img->dst_info;
	struct ipu_native_info * ipu = (struct ipu_native_info*)ipu_img->native_data;
	struct ipu_img_param *img = &ipu->img;

	img->in_width = src->width;
	img->in_height = src->height;

	img->out_x = dst->left;
	img->out_y = dst->top;
	img->out_width = dst->width;
	img->out_height = dst->height;
	img->in_fmt = src->fmt;
	img->out_fmt = dst->fmt;
	//img->lcdc_id = dst->lcdc_id; /* no used */

	img->in_bpp = 16;
	img->stride.y = src->srcBuf.y_stride;
	img->stride.u = src->srcBuf.u_stride;
	img->stride.v = src->srcBuf.v_stride;
	img->stride.out = dst->dstBuf.y_stride;

	img->output_mode = dst->dst_mode;
	img->alpha = dst->alpha;
	img->colorkey = dst->colorkey;
	img->src_page_mapping = 1;
	img->dst_page_mapping = 1;
	img->stlb_base = src->stlb_base;
	img->dtlb_base = dst->dtlb_base;
	if (img->in_height/img->out_height > 3) {
		img->zoom_mode = ZOOM_MODE_BILINEAR; /* 0 bilinear, 1 bicube, 2 enhance bilinear*/
	} else {	
		img->zoom_mode = ZOOM_MODE_BICUBE; /* 0 bilinear, 1 bicube, 2 enhance bilinear*/
	}
	ipu_resize(ipu, (int *)&img->out_width, (int *)&img->out_height, &img->Wdiff, &img->Hdiff);

//	dump_ipu_img_param(img);

	if (img->output_mode & IPU_OUTPUT_TO_LCD_FG1) {
		/*printf("------------ipu out mode to lcd fg1\n");*/
		if (!(img->output_mode & IPU_OUTPUT_MODE_FG0_OFF)) {
			/* enable compress decompress mode */
		}
#if 0
		ret = jz47_ipu_fg1_init(ipu);
		if (ret < 0) {
			printf("jz47_ipu_fg1_init failed\n");
			return -1;
		}
#endif
	} else {
		/*printf("------------ipu out mode to fb\n");*/
	}

	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_INIT, (void *)img);
	if (ret < 0) {
		LOGE("ipu first init failed\n");
		return -1;
	}

	ipu->state &= ~IPU_STATE_STARTED;

//	LOGD("Exit: %s", __FUNCTION__);

	return ret;
}

static int jz47_put_image(struct ipu_image_info *ipu_img, struct ipu_data_buffer *data_buf)
{
	int ret;

//	LOGD("Enter: %s", __FUNCTION__);
	if (ipu_img == NULL) {
		LOGE("ipu_img is NULL\n");
		return -1;
	}

	struct source_data_info * src = &ipu_img->src_info;
	struct dest_data_info *dst = &ipu_img->dst_info;
	struct ipu_native_info * ipu = (struct ipu_native_info*)ipu_img->native_data;
	struct ipu_img_param * img = &ipu->img;


	if (data_buf) {
		img->y_buf_p = WORDALIGN(data_buf->y_buf_phys);
		img->u_buf_p = WORDALIGN(data_buf->u_buf_phys);
		img->v_buf_p = WORDALIGN(data_buf->v_buf_phys);
		img->stride.y = WORDALIGN(data_buf->y_stride);
		img->stride.u = WORDALIGN(data_buf->u_stride);
		img->stride.v = WORDALIGN(data_buf->v_stride);
	} else {
		img->y_buf_p = WORDALIGN(src->srcBuf.y_buf_phys);
		img->u_buf_p = WORDALIGN(src->srcBuf.u_buf_phys);
		img->v_buf_p = WORDALIGN(src->srcBuf.v_buf_phys);

		img->y_buf_v = (unsigned char *)WORDALIGN((unsigned int)src->srcBuf.y_buf_v);
		img->u_buf_v = (unsigned char *)WORDALIGN((unsigned int)src->srcBuf.u_buf_v);
		img->v_buf_v = (unsigned char *)WORDALIGN((unsigned int)src->srcBuf.v_buf_v);

		img->stride.y = WORDALIGN(src->srcBuf.y_stride);
		img->stride.u = WORDALIGN(src->srcBuf.u_stride);
		img->stride.v = WORDALIGN(src->srcBuf.v_stride);
		if (!(img->output_mode & IPU_OUTPUT_TO_LCD_FG1)) {
			img->out_buf_p = WORDALIGN(dst->dstBuf.y_buf_phys);
			img->out_buf_v = (unsigned char *)WORDALIGN((unsigned int)dst->out_buf_v);
			img->stride.out = (dst->dstBuf.y_stride);
		}
	}

	/*LOGD("img->y_buf_v: %08x, img->u_buf_v: %08x, img->v_buf_v: %08x\n", 
			(unsigned int)img->y_buf_v, (unsigned int)img->u_buf_v, (unsigned int)img->v_buf_v); 
	LOGD("img->stride.y: %d, img->stride.u: %d, img->stride.v: %d\n",
			img->stride.y, img->stride.u, img->stride.v);
	*/
	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_SET_BUFF, img);
	if (ret < 0) {
		LOGE("jz47_put_image driver_ioctl(ipu, IOCTL_IPU_SET_BUFF) ret err=%d\n", ret);
		return ret;
	}

//	LOGD("Exit: %s", __FUNCTION__);
	return 0;
}

int alloc_img_info(struct ipu_image_info **ipu_img_p)
{
	struct ipu_image_info *ipu_img;
	struct ipu_native_info *ipu;
	struct ipu_img_param *img;

	ipu_img = *ipu_img_p;
	if (ipu_img == NULL) {
		ipu_img = (struct ipu_image_info *)calloc(sizeof(struct ipu_image_info) + 
												  sizeof(struct ipu_native_info), sizeof(char));
		if (ipu_img == NULL) {
			LOGE("ipu_open() no mem.\n");
			return -1;
		}
		ipu_img->native_data = (void*)((struct ipu_image_info *)ipu_img + 1);
		*ipu_img_p = ipu_img;
	}

	ipu = (struct ipu_native_info *)ipu_img->native_data;
	img = &ipu->img;
	img->version = sizeof(struct ipu_img_param);
	
	return 0;
}

static void free_img_info(struct ipu_image_info **ipu_img_p)
{
	struct ipu_image_info *ipu_img;
	struct ipu_native_info *ipu;

	ipu_img = *ipu_img_p;
	if (ipu_img == NULL) {
		LOGE("ipu_img is NULL+++++\n");
		return;
	}

	ipu = (struct ipu_native_info *)ipu_img->native_data;
	//mvom_lcd_exit(ipu);
	free(ipu_img);
	ipu_img = NULL;
	*ipu_img_p = ipu_img;

	return;
}

void set_open_state(struct ipu_native_info *ipu, int fd)
{
	ipu->ipu_fd = fd;
	ipu->state = IPU_STATE_OPENED;
}

int ipu_open(struct ipu_image_info **ipu_img_p)
{
	int ret, fd;
	int clk_en = 0;
	struct ipu_native_info *ipu;
	struct dest_data_info *dst;

	LOGD("Enter: %s\n", __FUNCTION__);
	if (ipu_img_p == NULL) {
		LOGE("ipu_img_p is NULL\n");
		return -1;
	}

	ret = alloc_img_info(ipu_img_p);
	if (ret < 0) {
		LOGE("alloc_img_info failed!\n");
		return -1;
	}
	ipu = (struct ipu_native_info *)(* ipu_img_p)->native_data;
	dst = &((*ipu_img_p)->dst_info);
	if ((fd = open(IPU_DEVNAME, O_RDONLY)) < 0) {
		LOGE("open ipu failed!\n");
		free_img_info(ipu_img_p);
		return -1;
	}

	if ((ret = ioctl(fd, IOCTL_IPU_SET_BYPASS)) < 0) {
		close(fd);
		LOGE(" Ipu set bypass is failed\n");
		return -1;
	}else
		//android_atomic_inc(&ipu_open_cnt);
	ipu_open_cnt++;

	set_open_state(ipu, fd);

	clk_en = 1;
	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_ENABLE_CLK, &clk_en);
	if (ret < 0) {
		LOGE("enable IPU %d clock failed\n", ipu->id);
		free_img_info(ipu_img_p);
		return -1;
	}

	LOGD("Exit: %s\n", __FUNCTION__);

	return fd;
}

int ipu_init(struct ipu_image_info * ipu_img)
{
	int ret;
	struct ipu_native_info *ipu;
	struct ipu_img_param *img;

	LOGD("Enter: %s\n", __FUNCTION__);
	if (ipu_img == NULL)
		return -1;

	ipu = (struct ipu_native_info *)ipu_img->native_data;
	img = &ipu->img;

	if (ipu->state & IPU_STATE_STARTED) {
		//printf("RESET IPU, STOP IPU FIRST.");
		//ipu_stop(ipu_img);
	}

	ret = jz47_ipu_init(ipu_img);
	if (ret < 0) {
		LOGE("jz47_ipu_init() failed.\n");
		return -1;
	}

	ipu->state |= IPU_STATE_INITED;

	LOGD("Exit: %s\n", __FUNCTION__);
	return ret;
}

int ipu_dump_regs(struct ipu_image_info * ipu_img,int is_dump_reg)
{
	int ret = 0;

	if (ipu_img == NULL) {
		LOGE("ipu_img is NULL\n");
		return -1;
	}
	struct ipu_native_info * ipu = (struct ipu_native_info *)ipu_img->native_data;
	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_DUMP_REGS, &is_dump_reg);
	if (ret < 0) {
		LOGE("IOCTL_IPU_DUMP_REGS failed\n");
		return -1;
	}
	return 0;
}

int ipu_start(struct ipu_image_info * ipu_img)
{
	int ret;
	LOGD("Enter: %s\n", __FUNCTION__);

	if (ipu_img == NULL) {
		LOGE("ipu_img is NULL\n");
		return -1;
	}

	struct ipu_native_info * ipu = (struct ipu_native_info *)ipu_img->native_data;
	struct ipu_img_param * img = &ipu->img;

#ifdef IPU_DBG
	ipu_dump_regs(true);
#endif
//	dump_ipu_img_param(img);

	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_START, img);
	if (ret < 0) {
		LOGE("IOCTL_IPU_START failed\n");
		return -1;
	}

	ipu->state &= ~IPU_STATE_STOPED;
	ipu->state |= IPU_STATE_STARTED;
	LOGD("Exit: %s\n", __FUNCTION__);

	return ret;
}

int ipu_stop(struct ipu_image_info * ipu_img)
{
	int ret;

	if (ipu_img == NULL) {
		LOGE("ipu_img is NULL!\n");
		return -1;
	}

	struct ipu_native_info *ipu = (struct ipu_native_info *)ipu_img->native_data;
	struct ipu_img_param *img = &ipu->img;

	if (!(ipu->state & IPU_STATE_STARTED)) {
		LOGE("ipu not started\n");
		return 0;
	}

	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_STOP, img);
	if (ret < 0) {
		LOGE("IOCTL_IPU_STOP failed!\n");
		return -1;
	}

	ipu->state &= ~IPU_STATE_STARTED;
	ipu->state |= IPU_STATE_STOPED;

	return ret;
}

int ipu_close(struct ipu_image_info ** ipu_img_p)
{
	int ret;
//	int fb_index;
	int clk_en = 0;
	struct ipu_image_info *ipu_img;
	struct ipu_native_info *ipu;
	struct ipu_img_param *img;

	LOGD("Enter: %s\n", __FUNCTION__);

	if (ipu_img_p == NULL) {
		LOGE("ipu_img_p is NULL\n");
		return -1;
	}

	ipu_img = *ipu_img_p;
	if (ipu_img == NULL) {
		LOGE("ipu_img is NULL\n");
		return -1;
	}

	ipu = (struct ipu_native_info *)ipu_img->native_data;
	img = &ipu->img;

	if (ipu->state == IPU_STATE_CLOSED) {
		LOGE("ipu already closed!\n");
		free_img_info(ipu_img_p);
		return -1;
	}

	ipu_stop(ipu_img);
	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_CLR_BYPASS);
	if (ret < 0) {
		LOGE("IOCTL_IPU_CLR_BYPASS failed!!!\n");
		free_img_info(ipu_img_p);
		return -1;
	}

	if (ipu_open_cnt)
		ipu_open_cnt--;
		//android_atomic_dec(&ipu_open_cnt);
	if (!ipu_open_cnt) {
		ret = ioctl(ipu->ipu_fd, IOCTL_IPU_ENABLE_CLK, &clk_en);
		if (ret < 0) {
			LOGE("Disable IPU %d clock failed\n", ipu->id);
			free_img_info(ipu_img_p);
			return -1;
		}
	}
	ipu->state = IPU_STATE_CLOSED;
	free_img_info(ipu_img_p);

	if (ipu->ipu_fd)
		close(ipu->ipu_fd);

	LOGD("Exit: %s\n", __FUNCTION__);
	return ret;
}

/* post video frame(YUV buffer) to ipu */
int ipu_postBuffer(struct ipu_image_info* ipu_img)
{
	int ret;

	if (ipu_img == NULL) {
		LOGE("ipu_img is NULL\n");
		return -1;
	}
	ret = jz47_put_image(ipu_img, NULL);
	if (ret < 0) {
		LOGE("jz47_put_image ret error=%d, return\n", ret);
		return ret;
	}

	struct ipu_native_info * ipu = (struct ipu_native_info*)ipu_img->native_data;
	struct ipu_img_param * img = &ipu->img;

	if (!(img->output_mode & IPU_OUTPUT_TO_LCD_FG1) || 
		!(ipu->state & IPU_STATE_STARTED)) {
		ret = ipu_start(ipu_img);
		if (ret < 0) {
			LOGE("ipu_start failed!\n");
			return ret;
		}
	}

	return ret;
}
