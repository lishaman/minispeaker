#include<stdio.h>
#include<linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>


#include "../../../../../hardware/xb4780/libjzipu/android_jz_ipu.h"
#include "../../../../../hardware/xb4780/libdmmu/dmmu.h"


#define SOURCE_BUFFER_SIZE 0x1000000 /* 16M */
#define START_ADDR_ALIGN 0x1000 /* 4096 byte */
#define STRIDE_ALIGN 0x800 /* 2048 byte */
#define PIXEL_ALIGN 16 /* 16 pixel */

#define FRAME_SIZE  70 /* 70 frame of y and u/v video data */

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

int initIPUSourceBuffer(struct ipu_image_info * ipu_img, struct source_info *source_info)
{
	int ret = 0;
	struct source_data_info *srcInfo;
	struct ipu_data_buffer *srcBuf;

	srcInfo = &ipu_img->src_info;
	srcBuf = &ipu_img->src_info.srcBuf;
	memset(srcInfo, 0, sizeof(struct source_data_info));

	srcInfo->fmt = HAL_PIXEL_FORMAT_JZ_YUV_420_B;
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

	dstInfo->dst_mode = IPU_OUTPUT_TO_FRAMEBUFFER | IPU_OUTPUT_BLOCK_MODE;
//	dstInfo->fmt = HAL_PIXEL_FORMAT_RGB_565;
//	dstInfo->fmt = HAL_PIXEL_FORMAT_RGBX_8888;
	dstInfo->fmt = HAL_PIXEL_FORMAT_BGRX_8888;
	dstInfo->left = 0;
	dstInfo->top = 0;
	dstInfo->width = dst_info->width;
	dstInfo->height = dst_info->height;
	dstInfo->dtlb_base = tlb_base_phys;
	dstInfo->lcdc_id = 0;

	dstInfo->out_buf_v = dst_info->out_buf_v;
	dstBuf->y_stride = dst_info->y_stride;

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int i;
	int frameCount = 0;
	int fbfd;
	struct ipu_image_info * ipu_image_info;
	struct fb_var_screeninfo *var_info;
	void *source_buffer;
	struct source_info source_info;
	struct dest_info dst_info;
	void *dest_buffer;
	int ipu_out_width = 320;
	int ipu_out_height = 240;

	if ( argc == 3 ) {
		ipu_out_width = atoi(argv[1]);
		ipu_out_height = atoi(argv[2]);
	}

	void *temp_buffer;
	int mIPU_inited = 0;

	struct dmmu_mem_info dmmu_tlb;

	var_info = malloc(sizeof(struct fb_var_screeninfo));
	if (!var_info) {
		printf("alloc framebuffer var screen info fail\n");
		return -1;
	}
	memset(var_info, 0, sizeof(struct fb_var_screeninfo));
	fbfd = open("/dev/graphics/fb0", O_RDWR);
	if (fbfd < 0) {
		printf("open fb 0 fail\n");
		return -1;
	}
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, var_info) < 0) {
		printf("get framebuffer var screen info fail\n");
		return -1;
	}


	source_buffer = (void *)malloc(SOURCE_BUFFER_SIZE);
	if (!source_buffer) {
		printf("alloc source_buffer fail\n");
		return -1;
	}

	memset(source_buffer, 0, SOURCE_BUFFER_SIZE);

	source_info.y_buf_v = (void *)(((unsigned int)source_buffer +
					(START_ADDR_ALIGN - 1)) &
				       (~(START_ADDR_ALIGN -1)));
	source_info.u_buf_v = (void *)((unsigned int)source_info.y_buf_v + 0x100000);
	source_info.v_buf_v = (void *)((unsigned int)source_info.u_buf_v);
	source_info.width = 320;
	source_info.height = 240;

	dst_info.width = ipu_out_width;
	dst_info.height = ipu_out_height;
//	dst_info.width = var_info->xres;
//	dst_info.height = var_info->yres;

	dst_info.size = ((var_info->xres + (PIXEL_ALIGN - 1)) &
		(~(PIXEL_ALIGN - 1))) * var_info->yres
		* (var_info->bits_per_pixel >> 3) * 3; /* 3 frame buffers */

	dst_info.y_stride = ((var_info->xres + (PIXEL_ALIGN - 1)) &
			     (~(PIXEL_ALIGN - 1))) * var_info->bits_per_pixel >> 3;
	dst_info.u_stride = 0;
	dst_info.v_stride = 0;
	dst_info.out_buf_v = mmap(0, dst_info.size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if (!dst_info.out_buf_v) {
		printf("mmap framebuffer fail\n");
		return -1;
	}

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

	memset(&dmmu_tlb, 0, sizeof(struct dmmu_mem_info));
	dmmu_tlb.vaddr = dst_info.out_buf_v;
	dmmu_tlb.size = dst_info.y_stride * dst_info.height * 3; /* 3 frame buffers */

	ret = dmmu_map_user_memory(&dmmu_tlb);
	if (ret < 0) {
		printf("dst dmmu_map_user_memory failed!\n");
		return -1;
	}


	i = FRAME_SIZE;
	ipu_image_info = NULL;
	if (ipu_open(&ipu_image_info) < 0) {
		printf("open ipu fail\n");
	}

	while (i--) {
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
		printf("dstStride=%d, srcStride=%d\n", dstStride, srcStride);
		for (hhh = 0; hhh < (240/16); hhh++) {
			memcpy(dst, src, 320*16/2);
			dst = (void *)((unsigned int)dst + dstStride);
			src = (void *)((unsigned int)src + srcStride);
		}


		initIPUSourceBuffer(ipu_image_info, &source_info);

		initIPUDestBuffer(ipu_image_info, &dst_info);

		if (mIPU_inited == 0) {

			if ((ret = ipu_init(ipu_image_info)) < 0) {
				printf("ipu_init() failed mIPUHandler=%p\n", ipu_image_info);
				return -1;
			} else {

				mIPU_inited = 1;
				printf("mIPU_inited == true\n");
			}
		}

		ipu_postBuffer(ipu_image_info);
		printf("ipu_postBuffer success\n");
		var_info->yoffset = 0;
		if (ioctl(fbfd, FBIOPAN_DISPLAY, var_info) < 0)
			printf("pan display fail\n");
		sleep(1);
//		frameCount++;
	}

	return 0;
}
