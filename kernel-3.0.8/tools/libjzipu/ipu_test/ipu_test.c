#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* For Open() */
#include<unistd.h>
#include<fcntl.h>
#include<sys/mman.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <asm/cachectl.h>
#include "../android_jz_ipu.h"
#include "saveBmp.h"
#include "../../libdmmu/dmmu.h"

#include "test_formate/t_352x288_r_420_image_888_gen.h"

#define USETLB 1

/*#define IPU_BUFF (1920*1080*4*2)*/
#define IPU_BUFF (0x2000000)
#define IPU_W 800
#define IPU_H 480
#define IPU_Y1_SIZE (1920*1088*4)
#define IPU_Y_SIZE 	(1920*1200)
#define IPU_U_SIZE 	(1920*1088)
#define IPU_V_SIZE 	(1920*1088)
#define IPU_D_SIZE 	(1920*1088*4)

//#define IPU_Y_OFFSET 	0x100
#define IPU_Y_OFFSET 	0x0
#define IPU_U_OFFSET 	(IPU_Y_OFFSET+IPU_Y_SIZE)
#define IPU_V_OFFSET 	(IPU_U_OFFSET+IPU_U_SIZE)
#define IPU_D_OFFSET	(IPU_Y_SIZE+IPU_U_SIZE+IPU_V_SIZE)

static int out_w = 800,out_h = 480;
static int src_w = 0,src_h = 0;
static int src_tlb = 0,dst_tlb = 0;
static int dst_mode = 0;
static int src_y_stride = 0, src_u_stride = 0, src_v_stride = 0;
static int dst_f = HAL_PIXEL_FORMAT_BGRX_8888;
static int src_f = HAL_PIXEL_FORMAT_BGRX_8888;
static int out_y_stride = 0;
static char input_src_format[50]={0};
static char input_dst_format[50]="RGB888";
static int PIC_STRIDE = 0;
unsigned int tlb_base = 0;
int fy=-1,fu=-1;
static unsigned int runcount = 1;
static unsigned int rcount = 0;
static unsigned int compare_dst = 0;
static unsigned int breakup_tlb = 0;
static unsigned int num_420b = -1;
static int saveBmp = 0;
static int dump_reg = 0;
static int USE_PMEM = 1;

int initIPUSrcBuffer(struct ipu_image_info *mIPUHandler,unsigned int srcbuff_p[3],unsigned char *srcbuff_v[3])
{
	if (mIPUHandler == NULL) {
		printf("----src ipu handler null\n");
		return -1;
	}
	struct source_data_info *src;
	struct ipu_data_buffer *srcBuf;

	src = &mIPUHandler->src_info;
	srcBuf = &mIPUHandler->src_info.srcBuf;
	memset(src, 0, sizeof(struct source_data_info));

	src->fmt			= src_f;//HAL_PIXEL_FORMAT_BGRX_8888;
	src->width			= src_w;
	src->height			= src_h;

	src->stlb_base		= tlb_base;
	srcBuf->y_buf_v		= srcbuff_v[0];
	srcBuf->u_buf_v		= srcbuff_v[1];
	srcBuf->v_buf_v		= srcbuff_v[2];

	srcBuf->y_buf_phys	= srcbuff_p[0];
	srcBuf->u_buf_phys	= srcbuff_p[1];
	srcBuf->v_buf_phys	= srcbuff_p[2];

	srcBuf->y_stride	= src_y_stride;
	srcBuf->u_stride	= src_u_stride;
	srcBuf->v_stride	= src_v_stride;
	printf("----srcBuf:w=%d,h=%d\n----src y_buf_v=0x%08x y_buf_phys=0x%08x\n",
			src->width,src->height,(unsigned int)srcBuf->y_buf_v,srcBuf->y_buf_phys);
	printf("----src y_stride=%d,u_stride=%d,v_stride=%d\n",
			srcBuf->y_stride,srcBuf->u_stride,srcBuf->v_stride);
	return 0;
}

int initIPUDstBuffer(struct ipu_image_info *mIPUHandler,unsigned int dstbuf_p,unsigned char  *dstbuf_v)
{
	struct dest_data_info *dst;
	struct ipu_data_buffer *dstBuf;

	if (mIPUHandler == NULL) {
		printf("----dst ipu handler null\n");
		return -1;
	}

	dst = &mIPUHandler->dst_info;
	dstBuf = &dst->dstBuf;

	memset(dst, 0, sizeof(struct dest_data_info));
	memset(dstBuf, 0, sizeof(struct ipu_data_buffer));

	dst->dst_mode		= dst_mode;
	dst->fmt			= dst_f;
	dst->left			= 0x0;
	dst->top			= 0x0;
	dst->width			= out_w;
	dst->height			= out_h;

	dst->dtlb_base		= tlb_base;
	dst->out_buf_v		= dstbuf_v;
	dstBuf->y_buf_phys	= dstbuf_p;
	dstBuf->u_buf_phys	= 0;
	dstBuf->v_buf_phys	= 0;
	dstBuf->y_stride	= out_y_stride;
	dstBuf->u_stride	= 0;
	dstBuf->v_stride	= 0;

	printf("----dstBuf:l=%d,t=%d,w=%d,h=%d,stride=%d\n----dst dstbuf_v=0x%08x dstbuf_p=0x%08x\n",
			dst->left,dst->top,dst->width,dst->height,dstBuf->y_stride,(unsigned int)dst->out_buf_v,dstBuf->y_buf_phys);
	return 0;
}

static void yuv420title_prepare(unsigned char *y_addr_v,unsigned char *u_addr_v)
{
	volatile unsigned char * pcc0;

	int FIN_H_PIC = src_h;
	int FIN_WW = src_w;

	pcc0 = (volatile unsigned char *)y_addr_v;

	int blk_h = (FIN_H_PIC + 15) /16;
	int blk_w = (FIN_WW + 15) /16;
	int blk_size = 256;
	int blk_str = blk_size * blk_w;

	int fin_y_str = blk_str ;
	int fin_uv_str = blk_str/2 ;
	int fin_y_size = 0 ;
	int i, j;
	int pix_tmp = 0x80;
	int v_pos,h_pos;
	int pix_xy;

	unsigned int count=0;

	for ( j = 0; j < blk_h; j ++){
		for ( i = 0 ; i < blk_w; i ++)
		{
			for ( pix_xy = 0 ; pix_xy < 256; pix_xy ++)
			{
				v_pos = j*16 + pix_xy/16;
				h_pos = i*16 + pix_xy%16;
				unsigned char tmpy;
				if ( v_pos > FIN_H_PIC ||  h_pos > FIN_WW )
					tmpy = 0 ;
				else
					tmpy = fin_y[v_pos][h_pos];

				*pcc0++ = tmpy;
				count++;
			}
		}
	}
	printf("----y finish addr 0x%08x, count=%d\n",(unsigned int)pcc0,count);
#if 0
	FILE *fp;
	if ((fp = fopen("/data/yyy.dat", "wb")) == NULL)
		return (-1);
	for (ct = 0; ct < count; ct++){
		fwrite(y_addr_v + ct, 1, 1, fp);
	}
	fclose(fp);
#endif
	pcc0 = (volatile unsigned char *)u_addr_v;

	blk_h = (FIN_H_PIC + 15) /16;
	blk_w = (FIN_WW + 15) /16;
	blk_size = 256;
	blk_str = blk_size * blk_w;

	fin_y_str = blk_str ;
	fin_uv_str = blk_str/2 ;
	fin_y_size = 0 ;
	pix_tmp=0x80;
	count=0;
	for ( j = 0; j < blk_h; j ++){
		for ( i = 0 ; i < blk_w; i ++)
		{
			for ( pix_xy = 0 ; pix_xy < 16*8; pix_xy ++)
			{
				v_pos = j*8 + pix_xy/16;
				h_pos = i*8 + pix_xy%8;
				unsigned char tmpuv;
				if ( v_pos > FIN_H_PIC/2 ||  h_pos > FIN_WW/2 )
					tmpuv = 0 ;
				else if ( pix_xy%16 < 8 )
					tmpuv = fin_u[v_pos][h_pos] ;
				else
					tmpuv = fin_v[v_pos][h_pos] ;
				count++;
				*pcc0++ = tmpuv;
				count++;
				//*pcc0++ = i*16+0x30;
			}
		}

	}
	printf("----u finish addr 0x%08x, count=%d\n",(unsigned int)pcc0,count);
#if 0
	if ((fp = fopen("/data/yyy1.dat", "wb")) == NULL)
		return (-1);
	for (ct = 0; ct < count; ct++){
		fwrite(u_addr_v + ct, 1, 1, fp);
	}
	fclose(fp);
#endif
}

int yuv420_b_prepare(srcbuf_y_v,srcbuf_u_v,srcbuf_v_v)
{
	int i = 0;
	unsigned char * sy_point;
	unsigned char * su_point;

	sy_point = (unsigned char *)srcbuf_y_v;
	su_point = (unsigned char *)srcbuf_u_v;

	int y_stride = PIC_STRIDE*16;
	int uv_stride =PIC_STRIDE*8;
	unsigned int tmp = ((src_w + 15)/16)*16 * 16;
	unsigned int tmp1 = (src_h + 15)/16;
#if 0
	for (i=0; (i < src_h);i++) {
		read(fy, (void*)sy_point, y_stride);
		sy_point += y_stride;

		read(fu, (void*)su_point, uv_stride);
		su_point += uv_stride;i
	}
#else
	for (i=0; i < tmp1;i++) {
		read(fy, (void*)sy_point, y_stride);
		sy_point += tmp;

		read(fu, (void*)su_point, uv_stride);
		su_point += tmp/2;
	}
	printf("----prepare finish y=%p u=%p\n",sy_point,su_point);
#endif
	close(fy);
	close(fu);
	return 0;
}

int yuv420_p_prepare(srcbuf_y_v,srcbuf_u_v,srcbuf_v_v)
{
	int i = 0;
	int fy = 0,fu = 0,fv = 0;
	char * file_420_p_y = "/data/test_formate/420_P_352x288_14_y.dat";
	char * file_420_p_u = "/data/test_formate/420_P_352x288_14_u.dat";
	char * file_420_p_v = "/data/test_formate/420_P_352x288_14_v.dat";
	unsigned char * sy_point;
	unsigned char * su_point;
	unsigned char * sv_point;

	fy = open(file_420_p_y, O_RDONLY, 0777);
	fu = open(file_420_p_u, O_RDONLY, 0777);
	fv = open(file_420_p_v, O_RDONLY, 0777);
	if ( fy < 0  ||  fu < 0 ||  fv < 0  ) {
		printf("!!!! open %d %d %d error\n",fy,fu,fv);
		return -1;
	}

	sy_point = (unsigned char *)srcbuf_y_v;
	su_point = (unsigned char *)srcbuf_u_v;
	sv_point = (unsigned char *)srcbuf_v_v;

	int y_stride = PIC_STRIDE;
	int uv_stride = PIC_STRIDE/2;
#if 0
	for (i=0; (i < src_h);i++) {
		read(fy, (void*)sy_point, y_stride);
		sy_point += y_stride;

		read(fu, (void*)su_point, uv_stride);
		su_point += uv_stride;

		read(fv, (void*)sv_point, uv_stride);
		sv_point += uv_stride;
	}
#else
	unsigned int tmp = ((src_w + 7)/8)*8;
	for (i=0; (i < src_h);i++) {
		read(fy, (void*)sy_point, y_stride);
		sy_point += tmp;
	}
	for (i=0; (i < src_h/2);i++) {
		read(fu, (void*)su_point, uv_stride);
		su_point += tmp/2;

	}
	for (i=0; (i < src_h/2);i++) {
		read(fv, (void*)sv_point, uv_stride);
		sv_point += tmp/2;
	}
#endif
	close(fy);
	close(fu);
	close(fv);
	return 0;
}

void prompt_printf()
{
	printf("++++++please use:+++++++++++\n");
	printf("./iputest -usepmem 1 -src_w 800 -src_h 480 -out_w 800 -out_h 480 -src_tlb 0 -dst_tlb 0 -tolcd 0 -dst_format RGB888 -src_format PACKRGB\n");
	printf("--src_format: PACKRGB,PACKYUV422,YUV420B(src WxH: 1920x1080,1280x720,640x480,352x288),YUV420P\n");
	printf("--dst_format: RGB888, RGB565 PACKYUV422\n");
	printf("--(some other test) -runcount 100 -saveBmp 1 -num_420b 2 -dump_reg 1 -compare_dst 100  -breakup_tlb 1 -tolcd 2(0:fb,1:tolcd,2:tolcd_2pictute)\n");
}

int main(int argc, char *argv[])
{
	int i = 0,ret = 0;;
	for (i=0; i <argc; i ++  ){
		if (strcmp("-h", argv[i])==0){
			prompt_printf();
			return 0;
		}
	}
#if 0
	printf("\n\n--You run cmd:");
	for (i=0; i <argc; i ++){
		printf(" %s ",argv[i]);
	}
	printf("\n");
#endif
	struct ipu_image_info *ipu_img = NULL;

	unsigned char *srcbuf_y_v0;
	unsigned char *srcbuf_u_v0;
	unsigned char *srcbuf_v_v0;
	unsigned char *dstbuf_y_v0;

	unsigned char *srcbuf_y1_v0;

	unsigned char *srcbuf_y1_v;
	unsigned char *srcbuf_y_v;
	unsigned char *srcbuf_u_v;
	unsigned char *srcbuf_v_v;
	unsigned int srcbuf_y_p;
	unsigned int srcbuf_u_p;
	unsigned int srcbuf_v_p;

	unsigned char *dstbuf_y_v;
	unsigned int dstbuf_y_p;

	unsigned char *srcBuf_v[3]={0};
	unsigned int srcBuf_p[3]={0};
	unsigned char *dstbuf_v;
	unsigned int dstbuf_p;

	unsigned char * srcbuf_point;

	char cmd1[100]={0};
	void *ipumem_user_v;
	unsigned int ipumem_p;
	int ipu_fd;
	int mem_fd;
	int tolcd = 0;
	FILE *fp;
	FILE *fp1;

	unsigned int r,d,save_fd = 0,save_fd_1_1 = 0,save_fd_1_2=0,save_fd_2_1=0;
	unsigned int *dstbuf_y;

	//int result_fd = -1,dst_fd;
	printf("##############ipu test start (buile time : %s)##############\n",__TIME__);
	/*************analyse input paramter*****************/
	if(argc == 1){
		printf("++++++++++++++++++++++\n");
		printf("-Notice:You do add parameter, Default 800x400rgb888->800x400_rgb888\n");
		printf("-Please run './iputest -h'\n");
		printf("++++++++++++++++++++++\n\n\n");
		src_f =  HAL_PIXEL_FORMAT_BGRX_8888;
		memcpy(input_src_format,"PACKRGB",strlen("PACKRGB"));
		src_w = 800;
		src_h = 480;
		out_w = 800;
		out_h = 480;
		saveBmp = 1;
	}else{
		for (i=0; i <argc; i ++  )
		{
			if (strcmp("-usepmem", argv[i])==0){
				USE_PMEM = atoi(argv[i+1]);
			}
			if (strcmp("-src_w", argv[i])==0){
				src_w = atoi(argv[i+1]);
			}
			if (strcmp("-src_h", argv[i])==0){
				src_h = atoi(argv[i+1]);
			}
			if (strcmp("-out_w", argv[i])==0){
				out_w = atoi(argv[i+1]);
			}
			if (strcmp("-out_h", argv[i])==0){
				out_h = atoi(argv[i+1]);
			}
			if (strcmp("-src_tlb", argv[i])==0){
				src_tlb = atoi(argv[i+1]);
			}
			if (strcmp("-dst_tlb", argv[i])==0){
				dst_tlb = atoi(argv[i+1]);
			}
			if (strcmp("-tolcd", argv[i])==0){
				tolcd = atoi(argv[i+1]);
			}
			if (strcmp("-num_420b", argv[i])==0){
				num_420b = atoi(argv[i+1]);
			}
			if (strcmp("-saveBmp", argv[i])==0){
				saveBmp = atoi(argv[i+1]);
			}
			if (strcmp("-dump_reg", argv[i])==0){
				dump_reg = atoi(argv[i+1]);
			}
			if (strcmp("-compare_dst", argv[i])==0){
				compare_dst = atoi(argv[i+1]);
			}
			if (strcmp("-breakup_tlb", argv[i])==0){
				breakup_tlb = atoi(argv[i+1]);
			}
			if ( strcmp("-dst_format", argv[i])==0 )
			{
				memcpy(input_dst_format,argv[i+1],strlen(argv[i+1]));
				if ( strcmp ("RGB888",argv[i+1])== 0){
					printf("----dst format: rgb888\n");
					dst_f = HAL_PIXEL_FORMAT_BGRX_8888;
				}else if ( strcmp ("RGB565",argv[i+1])== 0){
					printf("----dst format: rgb565\n");
					dst_f = HAL_PIXEL_FORMAT_RGB_565;
				}else if ( strcmp ("PACKYUV422",argv[i+1])== 0){
					printf("----dst format: packyuv422\n");
					dst_f = HAL_PIXEL_FORMAT_YCbCr_422_I;
				}else{
					printf("----dst format error\n");
					return -1;
				}
			}

			if ( strcmp("-src_format", argv[i])==0 )
			{
				memcpy(input_src_format,argv[i+1],strlen(argv[i+1]));
				if ( strcmp ("PACKYUV422",argv[i+1])== 0){
					printf("----src format: pack yuv422\n");
					src_f = HAL_PIXEL_FORMAT_YCbCr_422_I;
				}else if ( strcmp ("PACKRGB",argv[i+1])== 0){
					src_f = HAL_PIXEL_FORMAT_BGRX_8888;
					printf("----src format: bgrx888\n");
				}else if ( strcmp ("YUV420B",argv[i+1])== 0){
					src_f = HAL_PIXEL_FORMAT_JZ_YUV_420_B;
					printf("----src format: yuv420 tile\n");
				}else if ( strcmp ("YUV420P",argv[i+1])== 0){
					src_f = HAL_PIXEL_FORMAT_JZ_YUV_420_P;
					printf("----src format: yuv420 P\n");
				}else{
					printf("----src format is error\n");
					return -1;
				}
			}
			if (strcmp("-runcount", argv[i])==0){
				runcount = atoi(argv[i+1]);
			}


		}
	}

	/**************prepare ipu src and dst addr *****************/
	if ((ipu_fd = open("/dev/ipu", O_RDWR)) < 0)
	{
		printf("!!!! Cann't open /dev/ipu\n");
		return -1;
	}

	if (ioctl(ipu_fd,IOCTL_IPU_GET_PBUFF , &ipumem_p)) {
		printf("!!!! ipu get pbuff failed\n");
		return -1;
	}
	printf("----ipumem_p=0x%08x\n",ipumem_p);

	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if ( mem_fd < 0 ) {
		printf("!!!! open /dev/mem failed.\n");
		return(-1);
	}

#ifdef USETLB
	if (dmmu_init() < 0) {
		printf("!!!! dmmu_init failed\n");
		return -1;
	}
#endif

	if(USE_PMEM == 1){ /* PMEM_USE*/

		printf("----use pmem\n");
		ipumem_user_v = mmap(0, IPU_BUFF, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, ipumem_p & 0x1fffffff);


		srcbuf_y_v = (unsigned char *)ipumem_user_v + IPU_Y_OFFSET;
		srcbuf_u_v = (unsigned char *)ipumem_user_v + IPU_U_OFFSET;
		srcbuf_v_v = (unsigned char *)ipumem_user_v + IPU_V_OFFSET;

		srcbuf_y_p = ipumem_p + IPU_Y_OFFSET;
		srcbuf_u_p = ipumem_p + IPU_U_OFFSET;
		srcbuf_v_p = ipumem_p + IPU_V_OFFSET;

		dstbuf_y_v = (unsigned char *)ipumem_user_v + IPU_D_OFFSET;
		dstbuf_y_p = ipumem_p + IPU_D_OFFSET;
		/*memset(srcbuf_y_v,0x88,IPU_BUFF);*/

#ifdef USETLB
		/*src and dst total size */
		dmmu_match_user_mem_tlb(srcbuf_y_v, IPU_BUFF);
		ret = dmmu_map_user_mem(srcbuf_y_v, IPU_BUFF);
		if (ret < 0) {
			printf("!!!! dmmu_map_user_memory failed!~~~~~~~~~>");
			return -1;
		}
#endif

	}else{ /* PMEM_USE*/

		printf("----use malloc  mem\n");
		/*user malloc, can not user phy addr*/
		if(src_tlb == 0 || src_tlb == 0)
			printf("!!!! malloc mem,please do not use phy addr\n");
		src_tlb = 1;
		dst_tlb = 1;

		if(tolcd == 2){
			srcbuf_y1_v0 = malloc(IPU_Y1_SIZE);
			if( srcbuf_y1_v0 == NULL){
				printf("srcbuf_y1_v0 malloc failed\n");
				return -1;
			}
		}

		srcbuf_y_v0 = malloc(IPU_Y_SIZE);
		srcbuf_u_v0 = malloc(IPU_U_SIZE);
		srcbuf_v_v0 = malloc(IPU_V_SIZE);
		dstbuf_y_v0 = malloc(IPU_D_SIZE);

		if(	srcbuf_u_v0 == NULL ||
				srcbuf_u_v0 == NULL ||
				srcbuf_v_v0 == NULL ||
				dstbuf_y_v0 == NULL){
			printf("!!!! malloc failed\n");
			printf("\n--srcbuf_y_v=%p srcbuf_u_v=%p srcbuf_v_v=%p dstbuf_y_v=%p\n",
					srcbuf_y_v0,srcbuf_u_v0,srcbuf_v_v0,dstbuf_y_v0);
			return -1;
		}

		if(tolcd == 2)
			printf("\n----ori srcbuf_y1_v=%p\n",srcbuf_y1_v0);
		printf("----ori srcbuf_y_v=%p srcbuf_u_v=%p srcbuf_v_v=%p\n dstbuf_y_v=%p\n",
				srcbuf_y_v0,srcbuf_u_v0,srcbuf_v_v0,dstbuf_y_v0);


		if(tolcd == 2)
			memset(srcbuf_y1_v0,0xf0,IPU_Y1_SIZE);

		memset(srcbuf_y_v0,0xf0,IPU_Y_SIZE);
		memset(srcbuf_u_v0,0xf0,IPU_U_SIZE);
		memset(srcbuf_v_v0,0xf0,IPU_V_SIZE);
		memset(dstbuf_y_v0,0xf0,IPU_D_SIZE);

#if 1
		if(tolcd == 2)
		srcbuf_y1_v = (unsigned char *)(((unsigned int)((unsigned char *)srcbuf_y1_v0 +255))/256 *256);

		srcbuf_y_v = (unsigned char *)(((unsigned int)((unsigned char *)srcbuf_y_v0 +255))/256 *256);
		srcbuf_u_v = (unsigned char *)(((unsigned int)((unsigned char *)srcbuf_u_v0 +127))/128 *128);
		srcbuf_v_v = (unsigned char *)(((unsigned int)((unsigned char *)srcbuf_v_v0 +127))/128 *128);
		dstbuf_y_v = (unsigned char *)(((unsigned int)((unsigned char *)dstbuf_y_v0 +7))/8 *8);
#else
		srcbuf_y_v = (unsigned char *)(((unsigned int)((unsigned char *)srcbuf_y_v0 +255)/256 *256);
		srcbuf_u_v = (unsigned char *)(((unsigned int)((unsigned char *)srcbuf_u_v0 +255)/256 *256);
		srcbuf_v_v = (unsigned char *)(((unsigned int)((unsigned char *)srcbuf_v_v0 +255)/256 *256);
		dstbuf_y_v = (unsigned char *)(((unsigned int)((unsigned char *)dstbuf_y_v0 +255)/256 *256);
#endif

		/************dmmu handler******************/

		dmmu_conver_uservaddr_to_paddr(srcbuf_y_v,&srcbuf_y_p);
		dmmu_conver_uservaddr_to_paddr(srcbuf_u_v,&srcbuf_u_p);
		dmmu_conver_uservaddr_to_paddr(srcbuf_v_v,&srcbuf_v_p);
		dmmu_conver_uservaddr_to_paddr(dstbuf_y_v,&dstbuf_y_p);

#if 0
		ipumem_user_v = mmap(0, IPU_BUFF, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, ipumem_p & 0x1fffffff);
		srcbuf_y_v = (unsigned char *)ipumem_user_v + IPU_Y_OFFSET;
		/*srcbuf_u_v = (unsigned char *)ipumem_user_v + IPU_U_OFFSET;*/
		/*srcbuf_v_v = (unsigned char *)ipumem_user_v + IPU_V_OFFSET;*/

		srcbuf_y_p = ipumem_p + IPU_Y_OFFSET;
		/*srcbuf_u_p = ipumem_p + IPU_U_OFFSET;*/
		/*srcbuf_v_p = ipumem_p + IPU_V_OFFSET;<]*/

		/*dstbuf_y_v = (unsigned char *)ipumem_user_v + IPU_D_OFFSET;*/
		/*dstbuf_y_p = ipumem_p + IPU_D_OFFSET;*/
		memset(ipumem_user_v,0x88,IPU_BUFF);
#endif

		/*src and dst total size */
		if(tolcd == 2){
			dmmu_match_user_mem_tlb(srcbuf_y1_v, IPU_Y1_SIZE);
			ret = dmmu_map_user_mem(srcbuf_y1_v, IPU_Y1_SIZE);
		}

		dmmu_match_user_mem_tlb(srcbuf_y_v, IPU_Y_SIZE);
		ret = dmmu_map_user_mem(srcbuf_y_v, IPU_Y_SIZE);

		dmmu_match_user_mem_tlb(srcbuf_u_v, IPU_U_SIZE);
		ret |= dmmu_map_user_mem(srcbuf_u_v, IPU_U_SIZE);

		dmmu_match_user_mem_tlb(srcbuf_v_v, IPU_V_SIZE);
		ret |= dmmu_map_user_mem(srcbuf_v_v, IPU_V_SIZE);

		dmmu_match_user_mem_tlb(dstbuf_y_v, IPU_D_SIZE);
		ret |= dmmu_map_user_mem(dstbuf_y_v, IPU_D_SIZE);
		if (ret < 0) {
			printf("!!!! dmmu_map_user_memory in layers failed!~~~~~~~~~>");
			return -1;
		}

	} /*PMEM_USE*/
#ifdef USETLB
	ret = dmmu_get_page_table_base_phys(&tlb_base);
	if (ret < 0 || !tlb_base) {
		printf("!!!! dmmu_get_page_table_base_phys failed\n");
		dmmu_deinit();
		return -1;
	}
#endif

	if(tolcd == 2)
		printf("----srcbuf_y1_v=%p\n",srcbuf_y1_v);
	printf("----srcbuf_y_v=%p srcbuf_u_v=%p srcbuf_v_v=%p\n",
			srcbuf_y_v,srcbuf_u_v,srcbuf_v_v);
	printf("----srcbuf_y_p=0x%08x srcbuf_u_p=0x%08x srcbuf_y_p=0x%08x\n",
			srcbuf_y_p,srcbuf_u_p,srcbuf_v_p);
	printf("----dstbuf_y_v=%p dstbuf_p=0x%08x\n",
			dstbuf_y_v,dstbuf_y_p);
	/****************prepare src data******************/
	int fd_pack422;
	int flag=0;
	char * file_rgb = "/data/test_formate/screen";
	char * file_rgb1 = "/data/test_formate/screen1";
	char * file_pack422 = "/data/test_formate/package422_352x288_bk.dat";
	switch(src_f){
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
			if(src_w == 0 || src_h == 0){
				src_w = 352;
				src_h = 288;
			}
			if(src_w > 352 || src_h > 288){
				printf("!!!! Error:422 test src max w*h 352x288\n");
				return -1;
			}

			srcbuf_point = (unsigned char *)srcbuf_y_v;
			fd_pack422 = open(file_pack422, O_RDONLY, 0777);
			if ( fd_pack422 < 0 ) {
				printf("!!!! open \"%s\" error\n", file_pack422);
				return -1;
			}
			/* the source file is fix 352x288 */
			unsigned int tmp = (src_w%2) ? (src_w+1)*2:src_w*2 ;
			for (i=0; i < src_h;i++) {
				read(fd_pack422, srcbuf_point + tmp*i , 352*2);
			}
			close(fd_pack422);
			break;
		case HAL_PIXEL_FORMAT_BGRX_8888:
			if(src_w == 0 || src_h == 0){
				src_w = 800;
				src_h = 480;
			}
			if(src_w > 800 || src_h > 480){
				printf("!!!! Error:888 test src max w*h 800x480\n");
				return -1;
			}

			if ((fp = fopen(file_rgb, "rb")) == NULL){
				printf("!!!! open %s error\n",file_rgb);
				return (-1);
			}

			if(tolcd == 2){
				if ((fp1 = fopen(file_rgb1, "rb")) == NULL){
					printf("!!!! open %s error\n",file_rgb1);
					return (-1);
				}
			}

			int ori_Stride = 0,src_Stride = 0;
			src_Stride = ori_Stride = 800 *4;
			for(i=0;i<src_h;i++){
				ret = fread(srcbuf_y_v + src_w*4 * i, 1, ori_Stride, fp);
				if(ret < 0){
					printf("----read error\n");
					return ret;
				}
				if(tolcd == 2){
					ret = fread(srcbuf_y1_v + src_w*4 * i, 1, ori_Stride, fp1);
					if(ret < 0){
						printf("----read error\n");
						return ret;
					}
				}
			}
			fclose(fp);
			if(tolcd == 2)
				fclose(fp1);
			break;

		case HAL_PIXEL_FORMAT_JZ_YUV_420_B:

			printf("----start prepare src data (num_420b =%d)----\n",num_420b);
			if(num_420b == -1){
				printf("--Choose src WxH\n");
				printf("-1-WxH 1920x1080\n");
				printf("-2-WxH 1280x720\n");
				printf("-3-WxH 640x480\n");
				printf("-4-WxH 768x352\n");
				printf("++plese input num:\n");

				scanf("%d",&num_420b);
			}
			switch(num_420b){
				case 1:
					fy = open("/data/test_formate/420_B_1920x1080_27_y.dat", O_RDONLY, 0777);
					fu = open("/data/test_formate/420_B_1920x1080_27_u.dat", O_RDONLY, 0777);
					PIC_STRIDE = 1920;
					if(src_w == 0 || src_h == 0){
						src_w = 1920;
						src_h = 1080;
					}
					if(src_w > 1920 || src_h > 1080){
						printf("----max w*h 1920*1080\n");
						return -1;
					}
					break;
				case 2:
					fy = open("/data/test_formate/420_B_1280x720_6_y.dat", O_RDONLY, 0777);
					fu = open("/data/test_formate/420_B_1280x720_6_u.dat", O_RDONLY, 0777);
					PIC_STRIDE = 1280;
					if(src_w == 0 || src_h == 0){
						src_w = 1280;
						src_h = 720;
					}
					if(src_w > 1280 || src_h > 720){
						printf("----max w*h 1280*720\n");
						return -1;
					}
					break;
				case 3:
					fy = open("/data/test_formate/420_B_640x480_6_y.dat", O_RDONLY, 0777);
					fu = open("/data/test_formate/420_B_640x480_6_u.dat", O_RDONLY, 0777);
					PIC_STRIDE = 640;
					if(src_w == 0 || src_h == 0){
						src_w = 640;
						src_h = 480;
					}
					if(src_w > 640 || src_h > 480){
						printf("!!!! max w*h 640*480\n");
						return -1;
					}
					break;
				case 4:
					PIC_STRIDE = src_w;
					if(src_w == 0 || src_h == 0){
						src_w = 768;
						src_h = 352;
					}
					if(src_w > 768 || src_h > 352){
						printf("!!!! max w*h 768*352\n");
						return -1;
					}
					flag = 1;
					yuv420title_prepare(srcbuf_y_v,srcbuf_u_v);
					break;
				default:
					printf("!!!! 420b  num error\n");
					return -1;
			}
			if(flag == 0){
				if ( fy < 0  ||  fu < 0 ) {
					printf("!!!! open 420 B file %d %d error\n",fy,fu);
					return -1;
				}

				if(yuv420_b_prepare(srcbuf_y_v,srcbuf_u_v)<0){
					return -1;
				}
			}
			break;
		case HAL_PIXEL_FORMAT_JZ_YUV_420_P:
			PIC_STRIDE = 352;
			if(src_w == 0 || src_h == 0){
				src_w = 352;
				src_h = 288;
			}
			if(src_w > 352 || src_h > 288){
				printf("----420P test src max w*h 352*288\n");
				return -1;
			}

			if(yuv420_p_prepare(srcbuf_y_v,srcbuf_u_v,srcbuf_v_v)<0)
				return -1;
			break;
		default:
			printf("++++src format is error\n");
			return -1;
	}

	if(USE_PMEM == 1 && breakup_tlb == 1){
		printf("----ipu test breakup mem");
		ret = dmmu_breakup_user_mem(srcbuf_y_v, IPU_BUFF);
		if (ret < 0) {
			printf("!!!! dmmu_breakup_user_memry failed!~~~~~~~~~>");
			return -1;
		}
	}

	/************set src dst addr**********/
	switch(src_f){
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
		case HAL_PIXEL_FORMAT_BGRX_8888:
			if(src_tlb){
				srcBuf_v[0] = srcbuf_y_v;
				srcBuf_v[1] = 0;
				srcBuf_v[2] = 0;
			}else{
				srcBuf_p[0] = srcbuf_y_p;
				srcBuf_p[1] = 0;
				srcBuf_p[2] = 0;
			}
			break;
		case HAL_PIXEL_FORMAT_JZ_YUV_420_B:
			if(src_tlb){
				srcBuf_v[0] = srcbuf_y_v;
				srcBuf_v[1] = srcbuf_u_v;
				srcBuf_v[2] = srcbuf_u_v;
			}else{
				srcBuf_p[0] = srcbuf_y_p;
				srcBuf_p[1] = srcbuf_u_p;
				srcBuf_p[2] = srcbuf_u_p;
			}
			break;
		case HAL_PIXEL_FORMAT_JZ_YUV_420_P:
			if(src_tlb){
				srcBuf_v[0] = srcbuf_y_v;
				srcBuf_v[1] = srcbuf_u_v;
				srcBuf_v[2] = srcbuf_v_v;
			}else{
				srcBuf_p[0] = srcbuf_y_p;
				srcBuf_p[1] = srcbuf_u_p;
				srcBuf_p[2] = srcbuf_v_p;
			}
			break;
	}

	if(dst_tlb){
		dstbuf_v = dstbuf_y_v;
		dstbuf_p = 0;
	}else{
		dstbuf_p = dstbuf_y_p;
		dstbuf_v = 0;
	}

	/***************src stride****************/
	switch(src_f){
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
			src_y_stride = (src_w%2) ? (src_w+1)*2:src_w*2 ;
			break;
		case HAL_PIXEL_FORMAT_BGRX_8888:
			src_y_stride = src_w * 4;
			break;
		case HAL_PIXEL_FORMAT_JZ_YUV_420_B:
			src_y_stride = ((src_w + 15)/16)*16 * 16;
			src_u_stride = src_y_stride / 2;
			src_v_stride = src_y_stride / 2;
			break;
		case HAL_PIXEL_FORMAT_JZ_YUV_420_P:
			src_y_stride = ((src_w + 7)/8)*8;
			src_u_stride = src_y_stride / 2;
			src_v_stride = src_y_stride / 2;
			break;
	}

	/***************dst stride****************/
	switch(dst_f){
		case HAL_PIXEL_FORMAT_BGRX_8888:
			out_y_stride = out_w*4;
			out_y_stride = ((out_y_stride + 7)/8 * 8);
			break;
		case HAL_PIXEL_FORMAT_RGB_565:
			out_w = (out_w+1)/2*2;
			out_y_stride = out_w*2;
			out_y_stride = ((out_y_stride + 7)/8 * 8);
			break;
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
			out_w = (out_w+1)/2*2;
			out_y_stride = out_w*2;
			//out_y_stride = ((out_y_stride + 7)/8 * 8);
			break;
		default :
			printf("!!!! out format is unknow\n");
			return -1;
	}

	/***************dst mode****************/
	if(tolcd > 0)
		dst_mode = IPU_OUTPUT_TO_LCD_FG1;
	else
		dst_mode = IPU_OUTPUT_TO_FRAMEBUFFER | IPU_OUTPUT_BLOCK_MODE;

	/************set ipu parameter and run*************/
	if(ipu_open(&ipu_img) < 0){
		printf("!!!! open ipu failed\n");
		return -1;
	}


	initIPUSrcBuffer(ipu_img,srcBuf_p,srcBuf_v);
	initIPUDstBuffer(ipu_img,dstbuf_p,dstbuf_v);

#if 0
		if(cacheflush(NULL,0x100000,DCACHE)<0){
			printf("------cache flush error\n");
		}
#else
		if(tolcd == 2)
		dmmu_dma_flush_cache(srcbuf_y1_v0,IPU_Y1_SIZE,DMA_TO_DEVICE);

		dmmu_dma_flush_cache(srcbuf_y_v0,IPU_Y_SIZE,DMA_TO_DEVICE);
		dmmu_dma_flush_cache(srcbuf_u_v0,IPU_U_SIZE,DMA_TO_DEVICE);
		dmmu_dma_flush_cache(srcbuf_v_v0,IPU_V_SIZE,DMA_TO_DEVICE);
		dmmu_dma_flush_cache(dstbuf_y_v0,IPU_D_SIZE,DMA_FROM_DEVICE);
#endif

	ipu_dump_regs(ipu_img,dump_reg);

	if (ipu_init(ipu_img) < 0) {
		printf("!!!! ipu init faild\n");
		return -1;
	}

	struct ipu_data_buffer *srcBuf;
	srcBuf = &ipu_img->src_info.srcBuf;


	save_fd_1_1 = open("/data/test_formate/dst_1920x1080_1920x1080.dat",O_RDONLY);
	save_fd_1_2 = open("/data/test_formate/dst_960x540_1920x1080.dat",O_RDONLY);
	save_fd_2_1 = open("/data/test_formate/dst_1920x1080_960x540.dat",O_RDONLY);
	if(save_fd_1_1 < 0 || save_fd_1_2 || save_fd_2_1 < 0){
		printf("!!!! open save dst.dat failed %d %d %d\n",save_fd_1_1,save_fd_1_2,save_fd_2_1);
		save_fd = -1;
	}



	for(rcount=0;rcount<runcount;rcount++){

		if((tolcd == 2) && (src_f ==  HAL_PIXEL_FORMAT_BGRX_8888) && (USE_PMEM == 0)){
			if(rcount%2 == 0){
				printf("----bmp 1\n");
				srcBuf->y_buf_v		= srcbuf_y_v;
			}else{
				printf("----bmp 2\n");
				srcBuf->y_buf_v		= srcbuf_y1_v;
			}
		}
		if((ret = ipu_postBuffer(ipu_img)) < 0){
			printf("---Error: ipu postBuffer\n");
			//return -1;
			if(ret == -2){
				printf("----tlb error\n");
				while(1);
			}
		}
		if(tolcd > 0){
			usleep(500000);
		}
		if(compare_dst > 0 && rcount % 10 == 0){
			if( src_f != HAL_PIXEL_FORMAT_JZ_YUV_420_B ||
				dst_f != HAL_PIXEL_FORMAT_BGRX_8888 ||
				num_420b != 1
				){
				printf("!!!! Only compare:\n");
				printf("-dst_format RGB888 -src_format PACKRGB -num_420b 1\n"); 
				printf("src->dst: 1920x1080->1920x1080,1920x1080->960x540,960x540->1920x1080\n");
			}else{
				dstbuf_y = (unsigned int *)dstbuf_y_v;
				if(src_w==1920 && src_h==1080 && out_w==1920 && out_h==1080){
					save_fd = save_fd_1_1;
				}else if(src_w==960 && src_h==540 && out_w==1920 && out_h==1080){
					save_fd = save_fd_1_2;
				}else if(src_w==1920 && src_h==1080 && out_w==960 && out_h==540){
					save_fd = save_fd_2_1;
				}
				if(save_fd < 0){
					printf("!!!! open save dst.dat failed,break compar\n");
				}else{
					printf("----compare dst...,please wait...\n");
					lseek(save_fd,0,SEEK_SET);
					for(i=0;i<out_w*out_h;i++){
						if(i%compare_dst == 0){
							read(save_fd,&r,4);
							d = *dstbuf_y;
							/*printf("--i=%d-r_b=%x d_b=%x\n",i,r,d);*/
							dstbuf_y++;
							if(r != d){
								printf("#### result[%d]=0x%08x  ok[%d]=0x%08x\n",i,r,i,d);
								printf("#### result is error  while 1\n");
								close(save_fd);
								while(1);
							}
						}
					}
					printf("++++result is ok\n");
					printf("++++result is ok\n");
					printf("++++result is ok\n");
					printf("++++result is ok\n");
					printf("++++result is ok\n");
				}
			}
		}
	}
	//memcpy((unsigned char*)(dstbuf_y_v),(unsigned char*)(srcbuf_y_v),800*480*4);

	ipu_close(&ipu_img);

	if(tolcd == 2)
		printf("----srcbuf_y1_v=%p\n",srcbuf_y1_v);
	printf("----srcbuf_y_v=%p srcbuf_u_v=%p srcbuf_v_v=%p\n",
			srcbuf_y_v,srcbuf_u_v,srcbuf_v_v);
	printf("----srcbuf_y_p=0x%08x srcbuf_u_p=0x%08x srcbuf_y_p=0x%08x\n",
			srcbuf_y_p,srcbuf_u_p,srcbuf_v_p);
	printf("----dstbuf_y_v=%p dstbuf_p=0x%08x\n",
			dstbuf_y_v,dstbuf_y_p);
	/************save result*************/


	if(saveBmp == 1){ /*SaveBmp*/
		/*snprintf(cmd1,100,"/data/src_%s_%dx%d-dst_%s_%dx%d.bmp",input_src_format,src_w,src_h,input_dst_format,out_w,out_h);*/
		snprintf(cmd1,100,"/data/dst.bmp");
		printf("++++save dst to bmp=%s\n",cmd1);
		switch(dst_f){
			case HAL_PIXEL_FORMAT_BGRX_8888:
				SaveBufferToBmp32(cmd1,
						(unsigned char*)(dstbuf_y_v), out_w,out_h,out_y_stride, 0);
#if 1
				snprintf(cmd1,100,"/data/dst.dat");
				SaveBuffer(cmd1,
						(unsigned char*)(dstbuf_y_v), out_w,out_h,out_y_stride);
#endif
				break;
			case HAL_PIXEL_FORMAT_RGB_565:
				SaveBufferToBmp16(cmd1,
						(unsigned char*)(dstbuf_y_v), out_w,out_h,out_y_stride);
				break;
			case HAL_PIXEL_FORMAT_YCbCr_422_I:
#if 0
				snprintf(cmd1,100,"/data/dst888.bmp");
				SaveBufferToBmp32(cmd1,
						(unsigned char*)(dstbuf_y_v), out_w,out_h,out_y_stride, 0);
				snprintf(cmd1,100,"/data/dst565.bmp");
				SaveBufferToBmp16(cmd1,
						(unsigned char*)(dstbuf_y_v), out_w,out_h,out_y_stride);
#endif
				printf("!!!!  out 422 do not save to picture,please lookup *.dat use tools\n");
				snprintf(cmd1,100,"/data/dst422.dat");
				SaveBuffer(cmd1,
						(unsigned char*)(dstbuf_y_v), out_w,out_h,out_y_stride);
				break;
		}
		printf("----save data finish\n");
	} /*SaveBmp*/

#if 0
	if(tolcd){
	//	system("cat /sys/devices/platform/jz-fb.0/dump_lcd");
		printf("-----ipu to lcd wait\n");
		while(1);
	}
#endif

	if(USE_PMEM == 1){ /* PMEM_USE*/
#ifdef USETLB
		dmmu_unmap_user_mem(srcbuf_y_v, IPU_BUFF);
		dmmu_deinit();
#endif

	} else { /* PMEM_USE*/
		if(tolcd == 2)
			dmmu_unmap_user_mem(srcbuf_y1_v, IPU_Y1_SIZE);
		dmmu_unmap_user_mem(srcbuf_y_v, IPU_Y_SIZE);
		dmmu_unmap_user_mem(srcbuf_u_v, IPU_U_SIZE);
		dmmu_unmap_user_mem(srcbuf_v_v, IPU_V_SIZE);
		dmmu_unmap_user_mem(dstbuf_y_v, IPU_D_SIZE);
		dmmu_deinit();

		if(tolcd == 2)
			free(srcbuf_y1_v0);
		free(srcbuf_y_v0);
		free(srcbuf_u_v0);
		free(srcbuf_v_v0);
		free(dstbuf_y_v0);
	} /* PMEM_USE*/
	printf("#################ipu test finish######################\n");
	return 0;
}









