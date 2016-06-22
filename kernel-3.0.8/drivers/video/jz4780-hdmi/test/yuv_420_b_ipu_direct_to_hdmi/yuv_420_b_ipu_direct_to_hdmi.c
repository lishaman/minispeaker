#include<stdio.h>
#include<linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>


#include "../../../../../../hardware/xb4780/libjzipu/android_jz_ipu.h"
#include "../../../../../hardware/xb4780/libdmmu/dmmu.h"


#define SOURCE_BUFFER_SIZE 0x200000 /* 2M */
#define START_ADDR_ALIGN 0x1000 /* 4096 byte */
#define STRIDE_ALIGN 0x800 /* 2048 byte */
#define PIXEL_ALIGN 16 /* 16 pixel */

#define FRAME_SIZE  70 /* 70 frame of y and u/v video data */

/* LCDC ioctl commands */
#define JZFB_GET_MODENUM		_IOR('F', 0x100, int)
#define JZFB_GET_MODELIST		_IOR('F', 0x101, int)
#define JZFB_SET_VIDMEM			_IOW('F', 0x102, unsigned int *)
#define JZFB_SET_MODE			_IOW('F', 0x103, int)
#define JZFB_ENABLE			_IOW('F', 0x104, int)
#define JZFB_GET_RESOLUTION		_IOWR('F', 0x105, struct jzfb_mode_res)

#define JZFB_SET_ALPHA			_IOW('F', 0x123, struct jzfb_fg_alpha)
#define JZFB_SET_COLORKEY		_IOW('F', 0x125, struct jzfb_color_key)
#define JZFB_SET_BACKGROUND		_IOW('F', 0x124, struct jzfb_bg)

#define JZFB_ENABLE_FG0			_IOW('F', 0x139, int)

/* hdmi ioctl commands */
#define HDMI_POWER_OFF			_IO('F', 0x301)
#define	HDMI_VIDEOMODE_CHANGE		_IOW('F', 0x302, int)
#define	HDMI_POWER_ON			_IO('F', 0x303)

static unsigned int tlb_base_phys;

struct source_info {
	unsigned int width;
	unsigned int height;
	void *y_buf_v;
	void *u_buf_v;
	void *v_buf_v;
};

struct dest_info {
	unsigned int width;
	unsigned int height;
	unsigned long size;
	int y_stride;
	int u_stride;
	int v_stride;
	void *out_buf_v;
};

struct jzfb_fg_alpha {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 enable;
	__u32 mode; /* 0:global alpha, 1:pixel alpha */
	__u32 value; /* 0x00-0xFF */
};
struct jzfb_color_key {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 enable;
	__u32 mode; /* 0:color key, 1:mask color key */
	__u32 red;
	__u32 green;
	__u32 blue;
};

struct jzfb_bg {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 red;
	__u32 green;
	__u32 blue;
};

struct jzfb_mode_res {
	__u32 index; /* 1-64 */
	__u32 w;
	__u32 h;
};

int initIPUSourceBuffer(struct ipu_image_info * ipu_img, struct source_info *source_info)
{
	int ret = 0;
	struct source_data_info *srcInfo;
	struct ipu_data_buffer *srcBuf;


	srcInfo = &ipu_img->src_info;
	srcBuf = &ipu_img->src_info.srcBuf;
	memset(srcInfo, 0, sizeof(struct source_data_info));

	srcInfo->fmt = HAL_PIXEL_FORMAT_JZ_YUV_420_B;
//	srcInfo->fmt = HAL_PIXEL_FORMAT_BGRX_8888;
	srcInfo->width = source_info->width;
	srcInfo->height = source_info->height;
	srcInfo->is_virt_buf = 1;
	srcInfo->stlb_base = tlb_base_phys;

	srcBuf->y_buf_v = source_info->y_buf_v;
	srcBuf->u_buf_v = source_info->u_buf_v;
	srcBuf->v_buf_v = source_info->v_buf_v;

	srcBuf->y_stride = (source_info->width * 16 +
			    (STRIDE_ALIGN - 1)) & (~(STRIDE_ALIGN-1));
	srcBuf->u_stride = (source_info->width * 16 / 2 +
			    (STRIDE_ALIGN - 1)) & (~(STRIDE_ALIGN-1));
	srcBuf->v_stride = (source_info->width * 16 / 2 +
			    (STRIDE_ALIGN - 1)) & (~(STRIDE_ALIGN-1));

	return 0;
}

int initIPUDestBuffer(struct ipu_image_info * ipu_img, struct dest_info *dst_info)
{
	int ret = 0;
	struct dest_data_info *dstInfo;
	struct ipu_data_buffer *dstBuf;


	dstInfo = &ipu_img->dst_info;
	dstBuf = &ipu_img->dst_info.dstBuf;
	memset(dstInfo, 0, sizeof(struct dest_data_info));


	dstInfo->dst_mode = IPU_OUTPUT_TO_LCD_FG1;
//	dstInfo->fmt = HAL_PIXEL_FORMAT_RGBX_8888;
	dstInfo->fmt = HAL_PIXEL_FORMAT_BGRX_8888;
	dstInfo->left = 0;
	dstInfo->top = 0;
//	dstInfo->left = 100;
//	dstInfo->top = 80;
	dstInfo->lcdc_id = 1;

	dstInfo->width = dst_info->width;
	dstInfo->height = dst_info->height;

	dstBuf->y_stride = dst_info->y_stride;
	printf("%s, %d\n", __func__, __LINE__);

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int i;
	int loop;
	int frameCount = 0;
	struct ipu_image_info * ipu_image_info;
	void *source_buffer;
	struct source_info source_info;
	struct dest_info dst_info;
	void *dest_buffer;
	int ipu_out_width = 320;
	int ipu_out_height = 240;

	struct jzfb_fg_alpha fg0_alpha;
	struct jzfb_color_key color_key;
	struct jzfb_bg fg1_background;
	struct jzfb_mode_res hdmi_resolution;

	int fb_fd1;
	int mode_num;
	int *ml;
	int hdmi_fd = -1;
	int current_mode = 0;
	int enable;

	if (argc == 3) {
		ipu_out_width = atoi(argv[1]);
		ipu_out_height = atoi(argv[2]);
	}

	void *temp_buffer;
	int mIPU_inited = 0;

	struct dmmu_mem_info dmmu_tlb;

	fb_fd1 = open("/dev/graphics/fb1", O_RDWR);
	if (fb_fd1 < 0) {
		printf("open fb 1 fail\n");
		return -1;
	}
	/* get hdmi modes number */
	if (ioctl(fb_fd1, JZFB_GET_MODENUM, &mode_num) < 0) {
		printf("JZFB_GET_MODENUM faild\n");
		return -1;
	}
	printf("API FB mode number is %d\n", mode_num);

	ml = (int *)malloc(sizeof(int) * mode_num);
	if (ioctl(fb_fd1, JZFB_GET_MODELIST, ml) < 0) {
		printf("JZFB_GET_MODELIST faild\n");
		return -1;
	}

	for(i=0;i<mode_num;i++)
		printf("List[%d]: %d\n",i, ml[i]);

#if 1
	printf("please enter the hdmi video mode number\n");
	scanf("%d", &current_mode);
	if (ioctl(fb_fd1, JZFB_SET_MODE, &current_mode) < 0) {
		printf("JZFB set hdmi mode faild\n");
		return -1;
	}
	printf("API set mode name = %d success\n", current_mode);
#endif

	fg0_alpha.fg = 0;
	fg0_alpha.enable = 1;
	fg0_alpha.mode = 0; /* set the global alpha of FG 0 */
	fg0_alpha.value = 0xa0;
	if (ioctl(fb_fd1, JZFB_SET_ALPHA, &fg0_alpha) < 0) {
		printf("set fg 0 global alpha fail\n");
		return -1;
	}

	/* disable LCD controller FG 0 */
	enable = 0;
	if (ioctl(fb_fd1, JZFB_ENABLE_FG0, &enable) < 0) {
		printf("disable LCDC 0 FG 0 faild\n");
		return -1;
	}

	hdmi_resolution.index = current_mode;
	if (ioctl(fb_fd1, JZFB_GET_RESOLUTION, &hdmi_resolution) < 0) {
		printf("JZFB_GET_RESOLUTION faild\n");
		return -1;
	}
	printf("current resolution w = %d, h = %d\n", hdmi_resolution.w, hdmi_resolution.h);

	source_buffer = (void *)malloc(SOURCE_BUFFER_SIZE);
	if (!source_buffer) {
		printf("alloc source_buffer fail\n");
		return -1;
	}
	printf("source_buffer = %p\n", source_buffer);
	memset(source_buffer, 0, SOURCE_BUFFER_SIZE);

	source_info.y_buf_v = (void *)(((unsigned int)source_buffer +
					(START_ADDR_ALIGN - 1)) &
				       (~(START_ADDR_ALIGN -1)));
	source_info.u_buf_v = (void *)((unsigned int)source_info.y_buf_v + 0x100000);
	source_info.v_buf_v = (void *)((unsigned int)source_info.u_buf_v);
	source_info.width = 320;
	source_info.height = 240;

	dst_info.width = hdmi_resolution.w;
	dst_info.height = hdmi_resolution.h;

	dst_info.size = ((dst_info.width + (PIXEL_ALIGN - 1)) &
		(~(PIXEL_ALIGN - 1))) * dst_info.height
		* (32 >> 3);

	dst_info.y_stride = ((dst_info.width + (PIXEL_ALIGN - 1)) &
			     (~(PIXEL_ALIGN - 1))) * (32 >> 3);
	dst_info.u_stride = 0;
	dst_info.v_stride = 0;

	temp_buffer = (void *)malloc(SOURCE_BUFFER_SIZE);
	if (!temp_buffer) {
		printf("alloc temp buffer fail\n");
		return -1;
	}
	memset(source_buffer, 0, SOURCE_BUFFER_SIZE);

	ret = dmmu_init();
	if (ret < 0) {
		printf("dmmu_init failed\n");
		return -1;
	}
	ret = dmmu_get_page_table_base_phys(&tlb_base_phys);
	if (ret < 0) {
		printf("dmmu_get_page_table_base_phys failed!\n");
		return -1;
	}

	memset(&dmmu_tlb, 0, sizeof(struct dmmu_mem_info));
	dmmu_tlb.vaddr = source_buffer;
	dmmu_tlb.size = SOURCE_BUFFER_SIZE; /* test */

	ret = dmmu_map_user_memory(&dmmu_tlb);
	if (ret < 0) {
		printf("src dmmu_map_user_memory failed!\n");
		return -1;
	}

	ipu_image_info = NULL;
	if (open_ipu1(&ipu_image_info) < 0) {
		printf("open ipu fail\n");
	}

	initIPUDestBuffer(ipu_image_info, &dst_info);

	loop = 1000;
	while (loop--) {
		if (frameCount > FRAME_SIZE)
			frameCount = 0;
		char filename1[20] = {0};
		sprintf(filename1, "/data/yuv/y%d.raw", frameCount);
		FILE* sfd1 = fopen(filename1, "rb");
		if(sfd1){
			printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!fread\n");
			fread(temp_buffer, 320 * 240, 1, sfd1);
			fclose(sfd1);
			sfd1 = NULL;
		}else
			printf("fail to open nals.txt\n");

		printf("re-align Y Buffer\n");
		void * dst;
		void * src;
		int hhh;
		dst = source_info.y_buf_v;
		src = temp_buffer;
		int dstStride = 320 * 16;
		int srcStride = 320 * 16;

		dstStride = (dstStride + (STRIDE_ALIGN - 1))&(~(STRIDE_ALIGN-1));
		printf("dstStride=%d, srcStride=%d\n", dstStride, srcStride);
		for (hhh=0; hhh< (240/16); hhh++) {
			memcpy(dst, src, 320*16);
			dst = (void *)((unsigned int)dst + dstStride);
			src = (void *)((unsigned int)src + srcStride);
		}

		char filename2[20] = {0};
		sprintf(filename2, "/data/yuv/u%d.raw", ++frameCount); /* uv file from u1.raw */
		FILE* sfd2 = fopen(filename2, "rb");
		if (sfd2) {
			printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!fread");
			fread(temp_buffer, 320 * 240 / 2, 1, sfd2);
			fclose(sfd2);
			sfd2 = NULL;
		} else {
			printf("fail to open nals.txt\n");
		}

		printf("re-align UV Buffer\n" );
		dst = source_info.u_buf_v;
		src = temp_buffer;
		dstStride = 320 * 16 / 2;
		srcStride = 320 * 16 / 2;

		dstStride = (dstStride + (STRIDE_ALIGN - 1))&(~(STRIDE_ALIGN-1));

		for (hhh = 0; hhh < (240/16); hhh++) {
			memcpy(dst, src, 320*16/2);
			dst = (void *)((unsigned int)dst + dstStride);
			src = (void *)((unsigned int)src + srcStride);
		}

		initIPUSourceBuffer(ipu_image_info, &source_info);

		if (mIPU_inited == 0) {
			int enable;
			if ((ret = ipu_init(ipu_image_info)) < 0) {
				printf("ipu_init() failed mIPUHandler=%p\n", ipu_image_info);
				return -1;
			} else {
				mIPU_inited = 1;
				printf("mIPU_inited == true\n");
			}

			ipu_postBuffer(ipu_image_info);

			/* enable LCD controller 0 */
			enable = 1;
			if (ioctl(fb_fd1, JZFB_ENABLE, &enable) < 0) {
				printf(" JZFB_ENABLE faild\n");
				return -1;
			}

			/* init HDMI interface */
			hdmi_fd = open("/dev/hdmi", O_RDWR);
			printf("open hdmi device hdmi_fd = %d\n", hdmi_fd);
			if (hdmi_fd < 0)
				printf("open hdmi device fail\n");

			if (ioctl(hdmi_fd, HDMI_VIDEOMODE_CHANGE, &current_mode) < 0) {
				printf("HDMI_VIDEOMODE_CHANGE faild\n");
				return -1;
			}
		}
		ipu_postBuffer(ipu_image_info);
		printf("ipu_postBuffer success\n");

		usleep(25*1000);
//		frameCount++;
		printf("while() loop = %d\n", loop);
	}
	ret = ipu_close(&ipu_image_info);
	if (ret < 0) {
		printf("IPU close failed\n");
		return -1;
	}

	fg0_alpha.fg = 0;
	fg0_alpha.enable = 1;
	fg0_alpha.mode = 0; /* recover the original global alpha value */
	fg0_alpha.value = 0xff;
	if (ioctl(fb_fd1, JZFB_SET_ALPHA, &fg0_alpha) < 0) {
		printf("set FG 0 original global alpha fail\n");
		return -1;
	}

	/* disable LCD controller 0 */
	enable = 0;
	if (ioctl(fb_fd1, JZFB_ENABLE, &enable) < 0) {
		printf(" JZFB_ENABLE faild\n");
		return -1;
	}

	close(fb_fd1);

	close(hdmi_fd);

	return 0;
}
