/*
 * jz4775_epdce.c - EPD color engine interfaces for Ingenic jz4775_epd.c driver
 *
 * Copyright (C) 2005-2013, Ingenic Semiconductor Inc.
 * http://www.ingenic.cn/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/pm.h>
#include <linux/leds.h>
#include <linux/clk.h>
#include <soc/irq.h>

#include "jz4775_epdce.h"
#include "color_remap_lut/cr_lut.h"
#include "vee_lut/vee_lut.h"
#include "epdce_regs.h"

#ifndef CONFIG_FB_JZ4775_EPDCE
#define CONFIG_FB_JZ4775_EPDCE
#endif
#ifndef CONFIG_FB_JZ4775_EPDCE_OUTPUT_16_GRAYSCALE
#define CONFIG_FB_JZ4775_EPDCE_OUTPUT_16_GRAYSCALE
#endif
#ifndef CONFIG_FB_JZ4775_EPDCE_CSC_RGB565_TO_YUV444
#define CONFIG_FB_JZ4775_EPDCE_CSC_RGB565_TO_YUV444
#endif
#ifndef CONFIG_FB_JZ4775_EPDCE_CSC_RGB565_TO_YUV444_709_NARROW
#define CONFIG_FB_JZ4775_EPDCE_CSC_RGB565_TO_YUV444_709_NARROW
#endif
#ifndef CONFIG_FB_JZ4775_EPDCE_Y_VEE
#define CONFIG_FB_JZ4775_EPDCE_Y_VEE
#endif
#ifndef CONFIG_FB_JZ4775_EPDCE_DITHER_Y
#define CONFIG_FB_JZ4775_EPDCE_DITHER_Y
#endif

#define WORD_ALIGN(addr,words_num)	(((unsigned int)(addr)+(((words_num) * 4)-1))&(~(((words_num) * 4)-1)))

#define EPDC_WAIT_MDELAY	2000	/* Wait 2000ms max.*/

#define DEBUG 0
#if DEBUG
#define printk(format, arg...) printk(KERN_ALERT format, ## arg)
#else
#define printk(format, arg...) do {} while (0)
#endif

static struct clk *epdce_clk;

static volatile unsigned int is_oeof;
static unsigned int snp_buf_size;
static unsigned int *snp_buf	= NULL;
static 	wait_queue_head_t interrupt_status_wq;

extern struct fb_var_screeninfo fb_var_epd;

struct dma_descriptor
{
	unsigned int next_des_addr;	/* Next descriptor address */
	unsigned int fb_addr;		/* Frame buffer address */
	unsigned int dma_cmd;		/* DMA command register */
	unsigned int frm_id;		/* Interrupt frame identifier */
};

static struct dma_descriptor *ides_addr = NULL;
static struct dma_descriptor *odes_addr = NULL;

struct dma_descriptor_color
{
	unsigned int next_des_addr;	/* Next descriptor address */
	unsigned int fb0_addr;		/* Frame buffer0 address */
	unsigned int dma_cmd0;		/* DMA command0 register */
	unsigned int frm_id;		/* Interrupt frame identifier */
	unsigned int fb1_addr;		/* Frame buffer1 address */
	unsigned int dma_cmd1;		/* DMA command1 register */
};

static struct dma_descriptor_color *odes_addr_color = NULL;

struct rgb_data
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct yuv_data
{
	unsigned char y;
	unsigned char cb;
	unsigned char cr;
};

static void print_epdce_registers(void)
{
	printk("\n\nREG_EPDCE_CTRL		= 0x%08x\n", REG_EPDCE_CTRL);
	printk("REG_EPDCE_IFS		= 0x%08x\n", REG_EPDCE_IFS);
	printk("REG_EPDCE_IFOF		= 0x%08x\n", REG_EPDCE_IFOF);
	printk("REG_EPDCE_IFA		= 0x%08x\n", REG_EPDCE_IFA);
	printk("REG_EPDCE_OFA0		= 0x%08x\n", REG_EPDCE_OFA0);
	printk("REG_EPDCE_OFA1		= 0x%08x\n", REG_EPDCE_OFA1);
	printk("REG_EPDCE_IDMADES	= 0x%08x\n", REG_EPDCE_IDMADES);
	printk("REG_EPDCE_ODMADES	= 0x%08x\n", REG_EPDCE_ODMADES);
	printk("REG_EPDCE_IDMACMD	= 0x%08x\n", REG_EPDCE_IDMACMD);
	printk("REG_EPDCE_ODMACMD0	= 0x%08x\n", REG_EPDCE_ODMACMD0);
	printk("REG_EPDCE_ODMACMD1	= 0x%08x\n", REG_EPDCE_ODMACMD1);
	printk("REG_EPDCE_DMAC		= 0x%08x\n", REG_EPDCE_DMAC);
	printk("REG_EPDCE_DMAS		= 0x%08x\n", REG_EPDCE_DMAS);
	printk("REG_EPDCE_IFWS		= 0x%08x\n", REG_EPDCE_IFWS);
	printk("REG_EPDCE_OFS		= 0x%08x\n", REG_EPDCE_OFS);
	//	printk("REG_EPDCE_OFOF		= 0x%08x\n", REG_EPDCE_OFOF);
	printk("REG_EPDCE_OFC		= 0x%08x\n", REG_EPDCE_OFC);
	printk("REG_EPDCE_IS		= 0x%08x\n", REG_EPDCE_IS);
	printk("REG_EPDCE_IM		= 0x%08x\n", REG_EPDCE_IM);
	printk("REG_EPDCE_IC		= 0x%08x\n", REG_EPDCE_IC);
	printk("REG_EPDCE_DTC		= 0x%08x\n", REG_EPDCE_DTC);
	printk("REG_EPDCE_XRID		= 0x%08x\n", REG_EPDCE_XRID);
	printk("REG_EPDCE_XWID		= 0x%08x\n", REG_EPDCE_XWID);
	printk("REG_EPDCE_CRC		= 0x%08x\n", REG_EPDCE_CRC);

	printk("REG_EPDCE_IIFID		= 0x%08x\n", REG_EPDCE_IIFID);
	printk("REG_EPDCE_IOFID		= 0x%08x\n", REG_EPDCE_IOFID);
	printk("REG_EPDCE_IFID		= 0x%08x\n", REG_EPDCE_IFID);
	printk("REG_EPDCE_OFID		= 0x%08x\n", REG_EPDCE_OFID);
	printk("REG_EPDCE_CSCC		= 0x%08x\n", REG_EPDCE_CSCC);

	printk("REG_EPDCE_CFC		= 0x%08x\n", REG_EPDCE_CFC);
	printk("REG_EPDCE_CFRCE0	= 0x%08x\n", REG_EPDCE_CFRCE0);
	printk("REG_EPDCE_CFRCE1	= 0x%08x\n", REG_EPDCE_CFRCE1);
	printk("REG_EPDCE_CFRCE2	= 0x%08x\n", REG_EPDCE_CFRCE2);
	printk("REG_EPDCE_VEEC		= 0x%08x\n", REG_EPDCE_VEEC);
	printk("REG_EPDCE_HUEC		= 0x%08x\n", REG_EPDCE_HUEC);
	printk("REG_EPDCE_HUECE		= 0x%08x\n", REG_EPDCE_HUECE);
	printk("REG_EPDCE_CSC		= 0x%08x\n", REG_EPDCE_CSC);
	printk("REG_EPDCE_CSCE		= 0x%08x\n", REG_EPDCE_CSCE);
	//printk("REG_EPDCE_OFWS		= 0x%08x\n", REG_EPDCE_OFWS);
	printk("REG_EPDCE_CFGCE0	= 0x%08x\n", REG_EPDCE_CFGCE0);
	printk("REG_EPDCE_CFGCE1	= 0x%08x\n", REG_EPDCE_CFGCE1);
	printk("REG_EPDCE_CFGCE2	= 0x%08x\n", REG_EPDCE_CFGCE2);
	printk("REG_EPDCE_CFBCE0	= 0x%08x\n", REG_EPDCE_CFBCE0);
	printk("REG_EPDCE_CFBCE1	= 0x%08x\n", REG_EPDCE_CFBCE1);
	printk("REG_EPDCE_CFBCE2	= 0x%08x\n\n\n", REG_EPDCE_CFBCE2);
	return;
}

void jz4775_epdce_csc_rgb565_to_yuv444_601_wide_mode(void)
{
	REG_EPDCE_CSCC = EPDCE_CSCC_R2Y_601W | EPDCE_CSCC_R2Y_ENA;
}

void jz4775_epdce_csc_rgb565_to_yuv444_601_narrow_mode(void)
{
	REG_EPDCE_CSCC = EPDCE_CSCC_R2Y_601N | EPDCE_CSCC_R2Y_ENA;
}

void jz4775_epdce_csc_rgb565_to_yuv444_709_wide_mode(void)
{
	REG_EPDCE_CSCC = EPDCE_CSCC_R2Y_709W | EPDCE_CSCC_R2Y_ENA;
}

void jz4775_epdce_csc_rgb565_to_yuv444_709_narrow_mode(void)
{
	REG_EPDCE_CSCC = EPDCE_CSCC_R2Y_709N | EPDCE_CSCC_R2Y_ENA;
}


int jz4775_uv_hue(int hue_sin, int hue_cos)
{
	if(hue_sin > 256 || hue_sin < -256)
		return -1;
	if(hue_cos > 256 || hue_cos < -256)
		return -1;

	REG_EPDCE_HUECE = ((hue_cos << EPDCE_HUECE_HUE_COS) | (hue_sin << EPDCE_HUECE_HUE_SIN));
	REG_EPDCE_HUEC = EPDCE_HUEC_ENA;
}

int jz4775_uv_saturation(int k)
{
	if((k > 511) || (k < 0))
		return -1;

	REG_EPDCE_CSCE = k << EPDCE_CSCE_CS_K;
	REG_EPDCE_CSC = EPDCE_CSC_ENA;
	return 0;
}


void jz4775_epdce_csc_yuv444_to_rgb888_601_wide_mode(void)
{
	REG_EPDCE_CSCC = EPDCE_CSCC_Y2R_601W | EPDCE_CSCC_Y2R_ENA;
}

void jz4775_epdce_csc_yuv444_to_rgb888_601_narrow_mode(void)
{
	REG_EPDCE_CSCC = EPDCE_CSCC_Y2R_601N | EPDCE_CSCC_Y2R_ENA;
}

void jz4775_epdce_csc_yuv444_to_rgb888_709_wide_mode(void)
{
	REG_EPDCE_CSCC = EPDCE_CSCC_Y2R_709W | EPDCE_CSCC_Y2R_ENA;
}

void jz4775_epdce_csc_yuv444_to_rgb888_709_narrow_mode(void)
{
	REG_EPDCE_CSCC = EPDCE_CSCC_Y2R_709N | EPDCE_CSCC_Y2R_ENA;
}

void jz4775_epdce_y_33_color_filter(cfce k)
{
	REG_EPDCE_CFRCE0 = k[3] << EPDCE_CFRCE0_K3 | k[2] << EPDCE_CFRCE0_K2 | k[1] << EPDCE_CFRCE0_K1 | k[0] << EPDCE_CFRCE0_K0;
	REG_EPDCE_CFRCE1 = k[7] << EPDCE_CFRCE1_K7 | k[6] << EPDCE_CFRCE1_K6 | k[5] << EPDCE_CFRCE1_K5 | k[4] << EPDCE_CFRCE1_K4;
	REG_EPDCE_CFRCE2 = k[8] << EPDCE_CFRCE2_K8;
	REG_EPDCE_CFC = EPDCE_CFC_YCF_ENA;
}

void jz4775_epdce_r_33_color_filter(cfce k)
{
	REG_EPDCE_CFRCE0 = k[3] << EPDCE_CFRCE0_K3 | k[2] << EPDCE_CFRCE0_K2 | k[1] << EPDCE_CFRCE0_K1 | k[0] << EPDCE_CFRCE0_K0;
	REG_EPDCE_CFRCE1 = k[7] << EPDCE_CFRCE1_K7 | k[6] << EPDCE_CFRCE1_K6 | k[5] << EPDCE_CFRCE1_K5 | k[4] << EPDCE_CFRCE1_K4;
	REG_EPDCE_CFRCE2 = k[8] << EPDCE_CFRCE2_K8;
}

void jz4775_epdce_g_33_color_filter(cfce k)
{
	REG_EPDCE_CFGCE0 = k[3] << EPDCE_CFGCE0_K3 | k[2] << EPDCE_CFGCE0_K2 | k[1] << EPDCE_CFGCE0_K1 | k[0] << EPDCE_CFGCE0_K0;
	REG_EPDCE_CFGCE1 = k[7] << EPDCE_CFGCE1_K7 | k[6] << EPDCE_CFGCE1_K6 | k[5] << EPDCE_CFGCE1_K5 | k[4] << EPDCE_CFGCE1_K4;
	REG_EPDCE_CFGCE2 = k[8] << EPDCE_CFGCE2_K8;
}

void jz4775_epdce_b_33_color_filter(cfce k)
{
	REG_EPDCE_CFBCE0 = k[3] << EPDCE_CFBCE0_K3 | k[2] << EPDCE_CFBCE0_K2 | k[1] << EPDCE_CFBCE0_K1 | k[0] << EPDCE_CFBCE0_K0;
	REG_EPDCE_CFBCE1 = k[7] << EPDCE_CFBCE1_K7 | k[6] << EPDCE_CFBCE1_K6 | k[5] << EPDCE_CFBCE1_K5 | k[4] << EPDCE_CFBCE1_K4;
	REG_EPDCE_CFBCE2 = k[8] << EPDCE_CFBCE2_K8;
}

void jz4775_epdce_rgb_33_clolor_filter_ena(void)
{
	REG_EPDCE_CFC = EPDCE_CFC_RGBCF_ENA;
}

void jz4775_epdce_y_33_clolor_filter_ena(void)
{
}

void jz4775_epdce_dither_y()
{
	int dth_bits_default = 4;
	REG_EPDCE_DTC &= ~EPDCE_DTC_DTH_SEL_RSV;

	switch(dth_bits_default)
	{
		case 2:
			REG_EPDCE_DTC |= EPDCE_DTC_DTH_SEL_2BITS;
			break;

		case 3:
			REG_EPDCE_DTC |= EPDCE_DTC_DTH_SEL_3BITS;
			break;

		case 4:
			REG_EPDCE_DTC |= EPDCE_DTC_DTH_SEL_4BITS;
			break;

		default:
			REG_EPDCE_DTC |= EPDCE_DTC_DTH_SEL_4BITS;
			break;

	}

	REG_EPDCE_DTC |= EPDCE_DTC_DTHY_ENA;
}

void jz4775_epdce_dither_r(void)
{
	REG_EPDCE_DTC |= EPDCE_DTC_DTHR_ENA;
}

void jz4775_epdce_dither_g(void)
{
	REG_EPDCE_DTC |= EPDCE_DTC_DTHG_ENA;
}

void jz4775_epdce_dither_b(void)
{
	REG_EPDCE_DTC |= EPDCE_DTC_DTHB_ENA;
}

void jz4775_epdce_color_remap_cfa(void)
{
	REG_EPDCE_CRC = EPDCE_CRC_CFA_MODE | EPDCE_CRC_ENA;
}

void jz4775_epdce_color_remap_pa(void)
{
	REG_EPDCE_CRC = EPDCE_CRC_PA_MODE | EPDCE_CRC_ENA;
}

void jz4775_epdce_output_rgbw(void)
{
	REG_EPDCE_OFC = EPDCE_OFC_SEL_RGBW;
}

void jz4775_epdce_output_y(void)
{
	REG_EPDCE_OFC = EPDCE_OFC_SEL_Y;
}

void jz4775_epdce_output_rgb(void)
{
	REG_EPDCE_OFC = EPDCE_OFC_SEL_RGB | EPDCE_OFC_SEL0_R | EPDCE_OFC_SEL1_G | EPDCE_OFC_SEL2_B;
}

#if 0
/*
 *
 *2. Verify Color Remap
 *
 * */

static int jz4775_epdce_load_cr_lut(unsigned short * cr_lut)
{
	int i;
	unsigned char msdb;
	unsigned int lut_addr;
	unsigned int entry;
	unsigned short *lut = cr_lut;
	for(i = 0; i < 4096; i++)
	{

		lut_addr = (unsigned int)(&REG_EPDCE_CR_LUT(i));
		msdb = (lut_addr & (0x3));
		entry = ((msdb << 30) | lut[i]);
		//if(i >= 4080 || i < 160)
		//printk("%d lut 0x%08x --> 0x%08x  entry = 0x%08x\n", i, lut_addr, &(REG_EPDCE_CR_LUT((i / 4) * 4)), entry);
		//memcpy(&REG_EPDCE_CR_LUT(i), &entry, 4);
		REG_EPDCE_CR_LUT((i / 4) * 4) = entry;
	}
	return 0;
}

static int jz4775_epdce_controller_init_color(void)
{
	int outputbits = 4;

	printk("%s %d\n", __FUNCTION__, __LINE__);
	REG_EPDCE_CTRL = EPDCE_CTRL_EPDCE_RST;
	REG_EPDCE_IFS =  299 << EPDCE_IFS_IN_FRM_VS		|	399 << EPDCE_IFS_IN_FRM_HS;
	REG_EPDCE_OFS =  400 << EPDCE_OFS_O_FRM_HS;
	printk("cr_lut = 0x%08x\n", (unsigned int)cr_lut);
	jz4775_epdce_load_cr_lut(cr_lut);
	REG_EPDCE_XRID = 0xf;
	REG_EPDCE_XWID = 0xf;

	jz4775_dither_sel(outputbits);
	jz4775_color_remap_cfa();
	//jz4775_color_remap_pa();
	return 0;
}

static int jz4775_epdce_ena_epdce_color(unsigned char *frame_buffer, int fb_len, unsigned char *current_buffer, int cb_len)
{
	printk("---%s fb_addr = 0x%08x fb_len = %d current_buffer = 0x%08x cb_len = %d\n\n", __FUNCTION__, (unsigned int)frame_buffer, fb_len, (unsigned int)current_buffer, cb_len);

	memset(ides_addr, 0, sizeof(struct dma_descriptor));
	memset(odes_addr, 0, sizeof(struct dma_descriptor));
	memset(odes_addr_color, 0, sizeof(struct dma_descriptor_color));

	ides_addr->next_des_addr = (unsigned int)virt_to_phys(ides_addr);
	ides_addr->fb_addr = (unsigned int)virt_to_phys(frame_buffer);
	ides_addr->dma_cmd |= EPDCE_IDMACMD_EOF_ENA | EPDCE_IDMACMD_STOP_ENA | fb_len;
	ides_addr->frm_id = 0;
	dma_cache_wback_inv((long unsigned int)ides_addr, sizeof(struct dma_descriptor));
	REG_EPDCE_IDMADES = (unsigned int)virt_to_phys(ides_addr);

	printk("ides_addr->fb_addr = 0x%08x\n", ides_addr->fb_addr);
	printk("ides_addr->dma_cmd = 0x%08x\n", ides_addr->dma_cmd);
	printk("REG_EPDCE_IDMADES		= 0x%08x\n", REG_EPDCE_IDMADES);
	printk("ides_addr->next_des_addr	= 0x%08x\n", ides_addr->next_des_addr);
	printk("double word aligned ides_addr	= 0x%08x\n\n",(unsigned int)WORD_ALIGN(ides_addr, 2));


	if(REG_EPDCE_CRC & EPDCE_CRC_PA_MODE)
	{
		printk("Color remap output PA mode!\n");
		odes_addr_color->next_des_addr = (unsigned int)virt_to_phys(odes_addr_color);
		odes_addr_color->fb0_addr = (unsigned int)virt_to_phys(current_buffer);
		odes_addr_color->dma_cmd0 |= EPDCE_ODMACMD0_EOF_ENA | EPDCE_ODMACMD0_STOP_ENA | (cb_len / 2);
		odes_addr_color->frm_id = 0;
		odes_addr_color->fb1_addr = (unsigned int)virt_to_phys(current_buffer + 400);
		odes_addr_color->dma_cmd1 = (cb_len / 2);
		dma_cache_wback_inv((long unsigned int)odes_addr_color, sizeof(struct dma_descriptor_color));
		REG_EPDCE_ODMADES = (unsigned int)virt_to_phys(odes_addr_color);

		REG_EPDCE_OFC = EPDCE_OFC_SEL_RGBW | EPDCE_OFC_SEL0_R | EPDCE_OFC_SEL1_G | EPDCE_OFC_SEL2_R | EPDCE_OFC_SEL3_G | EPDCE_OFC_SEL4_B | EPDCE_OFC_SEL5_W | EPDCE_OFC_SEL6_B | EPDCE_OFC_SEL7_W;

		printk("odes_addr_color->fb0_addr = 0x%08x\n", odes_addr_color->fb0_addr);
		printk("odes_addr_color->dma_cmd0 = 0x%08x\n", odes_addr_color->dma_cmd0);
		printk("odes_addr_color->fb1_addr = 0x%08x\n", odes_addr_color->fb1_addr);
		printk("odes_addr_color->dma_cmd1 = 0x%08x\n", odes_addr_color->dma_cmd1);
		printk("REG_EPDCE_ODMADES		= 0x%08x\n", REG_EPDCE_ODMADES);
		printk("odes_addr_color->next_des_addr	= 0x%08x\n", odes_addr_color->next_des_addr);
		printk("double word aligned odes_addr	= 0x%08x\n\n",(unsigned int)WORD_ALIGN(odes_addr_color, 2));
	}
	else
	{
		printk("Color remap output CFA mode!\n");
		odes_addr->next_des_addr = (unsigned int)virt_to_phys(odes_addr);
		odes_addr->fb_addr =(unsigned int)virt_to_phys(current_buffer);
		odes_addr->dma_cmd |= EPDCE_ODMACMD0_EOF_ENA | EPDCE_ODMACMD0_STOP_ENA | (cb_len / 4);
		odes_addr->frm_id = 0;
		dma_cache_wback_inv((long unsigned int)odes_addr, sizeof(struct dma_descriptor));
		REG_EPDCE_ODMADES = (unsigned int)virt_to_phys(odes_addr);
		REG_EPDCE_OFC = EPDCE_OFC_SEL_RGBW | EPDCE_OFC_SEL0_R | EPDCE_OFC_SEL1_G | EPDCE_OFC_SEL2_B | EPDCE_OFC_SEL3_W | EPDCE_OFC_SEL4_R | EPDCE_OFC_SEL5_G | EPDCE_OFC_SEL6_B | EPDCE_OFC_SEL7_W;

		printk("odes_addr->fb_addr = 0x%08x\n", odes_addr->fb_addr);
		printk("odes_addr->dma_cmd = 0x%08x\n", odes_addr->dma_cmd);
		printk("REG_EPDCE_ODMADES		= 0x%08x\n", REG_EPDCE_ODMADES);
		printk("odes_addr->next_des_addr	= 0x%08x\n", odes_addr->next_des_addr);
		printk("double word aligned odes_addr	= 0x%08x\n\n",(unsigned int)WORD_ALIGN(odes_addr, 2));
	}

	dma_cache_wback_inv((long unsigned int)frame_buffer, fb_len);
	REG_EPDCE_DMAC = EPDCE_DMAC_CDMA_ENA;
	REG_EPDCE_CTRL = EPDCE_CTRL_EPDCE_EN;

	print_epdce_registers();
	printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
	return 0;
}
static int jz4775_epdce_auto_stop_(unsigned char *current_buffer, int cb_len)
{

	if(!(REG_EPD_CFG & EPD_CFG_AUTO_STOP_ENA))
		return -1;

	if(REG_EPD_CFG & EPD_CFG_PARTIAL)
	{
		int time_out = 20;
		while(!(REG_EPDCE_DMAS & EPDCE_DMAS_OSOF) && time_out--)
		{
			printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
			schedule_timeout(5);
		}
		//dma_cache_wback_inv((long unsigned int)current_buffer, cb_len);
		printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
		memset(snp_buf, 0xff, 4);
		dma_cache_wback_inv((long unsigned int)snp_buf, 4);
	}
	else
	{
		int time_out = 20;
		while(!(REG_EPDCE_DMAS & EPDCE_DMAS_OEOF) && time_out--)
		{
			printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
			schedule_timeout(5);
		}

		//dma_cache_wback_inv((long unsigned int)current_buffer, cb_len);
		printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
		memset(snp_buf, 0xff, 4);
		dma_cache_wback_inv((long unsigned int)snp_buf, 4);
	}
	return 0;
}

static int jz4775_epdce_dis_epdce_color(unsigned char *current_buffer, int cb_len)
{
	printk("%s %d\n", __FUNCTION__, __LINE__);
	printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
	int time_out = 20;
	while(!(REG_EPDCE_DMAS & EPDCE_DMAS_OEOF) && time_out--)
	{
		printk("-line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
		schedule_timeout(5);
	}

	//dma_cache_wback_inv((long unsigned int)current_buffer, cb_len);
	printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
	REG_EPDCE_IC |= EPDCE_IC_OSTOP | EPDCE_IC_OEOF | EPDCE_IC_OSOF | EPDCE_IC_ISTOP | EPDCE_IC_IEOF | EPDCE_IC_ISOF;
	printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
	REG_EPDCE_DMAC &= ~EPDCE_DMAC_CDMA_ENA;
	REG_EPDCE_CTRL &= ~EPDCE_CTRL_EPDCE_EN;
	printk("Epdce work over!\n\n");

	return 0;
}

static int jz4775epd_pan_display_color(unsigned char *frame_buffer, int fb_len, unsigned char *current_buffer, int cb_len)
{
	jz4775_epdce_controller_init_color();
	jz4775_epdce_ena_epdce_color(frame_buffer, fb_len, current_buffer, cb_len);
	//jz4775_epdce_ena_epdce(frame_buffer, fb_len, current_buffer, cb_len);
	//jz4775_epdce_auto_stop(current_buffer, cb_len);
	jz4775_epdce_dis_epdce_color(current_buffer, cb_len);
	printk("%s %d\n", __FUNCTION__, __LINE__);

	return 0;
}


/*
 *
 *3. Verify HUE, Saturation, Filter, Dither, RGB output.
 *
 * */

static int jz4775_epdce_ena_epdce_rgb(unsigned char *frame_buffer, int fb_len, unsigned char *current_buffer, int cb_len)
{
	memset(ides_addr, 0, sizeof(struct dma_descriptor));
	memset(odes_addr, 0, sizeof(struct dma_descriptor));

	printk("fb_addr = 0x%08x fb_len = %d current_buffer = 0x%08x cb_len = %d\n\n", (unsigned int)frame_buffer, fb_len, (unsigned int)current_buffer, cb_len);
	ides_addr->next_des_addr = (unsigned int)virt_to_phys(ides_addr);
	ides_addr->fb_addr = (unsigned int)virt_to_phys(frame_buffer);
	ides_addr->dma_cmd |= EPDCE_IDMACMD_EOF_ENA | EPDCE_IDMACMD_STOP_ENA | fb_len;
	ides_addr->frm_id = 0;
	dma_cache_wback_inv((long unsigned int)ides_addr, sizeof(struct dma_descriptor));
	REG_EPDCE_IDMADES = (unsigned int)virt_to_phys(ides_addr);

	printk("ides_addr->fb_addr = 0x%08x\n", ides_addr->fb_addr);
	printk("ides_addr->dma_cmd = 0x%08x\n", ides_addr->dma_cmd);
	printk("REG_EPDCE_IDMADES		= 0x%08x\n", REG_EPDCE_IDMADES);
	printk("ides_addr->next_des_addr	= 0x%08x\n", ides_addr->next_des_addr);
	printk("double word aligned ides_addr	= 0x%08x\n\n",(unsigned int)WORD_ALIGN(ides_addr, 2));

	odes_addr->next_des_addr = (unsigned int)virt_to_phys(odes_addr);
	odes_addr->fb_addr =(unsigned int)virt_to_phys(current_buffer);
	odes_addr->dma_cmd |= EPDCE_ODMACMD0_EOF_ENA | EPDCE_ODMACMD0_STOP_ENA | cb_len;
	odes_addr->frm_id = 0;
	dma_cache_wback_inv((long unsigned int)odes_addr, sizeof(struct dma_descriptor));
	REG_EPDCE_ODMADES = (unsigned int)virt_to_phys(odes_addr);

	printk("odes_addr->fb_addr = 0x%08x\n", odes_addr->fb_addr);
	printk("odes_addr->dma_cmd = 0x%08x\n", odes_addr->dma_cmd);
	printk("REG_EPDCE_ODMADES		= 0x%08x\n", REG_EPDCE_ODMADES);
	printk("odes_addr->next_des_addr	= 0x%08x\n", odes_addr->next_des_addr);
	printk("double word aligned odes_addr	= 0x%08x\n\n",(unsigned int)WORD_ALIGN(odes_addr, 2));

	dma_cache_wback_inv((long unsigned int)frame_buffer, fb_len);
	REG_EPDCE_DMAC = EPDCE_DMAC_CDMA_ENA;
	REG_EPDCE_CTRL = EPDCE_CTRL_EPDCE_EN;
	printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);

	return 0;
}

static int jz4775_epdce_auto_stop_rgb(unsigned char *current_buffer, int cb_len)
{

	if(!(REG_EPD_CFG & EPD_CFG_AUTO_STOP_ENA))
		return -1;

	if(REG_EPD_CFG & EPD_CFG_PARTIAL)
	{
		int time_out = 20;
		while(!(REG_EPDCE_DMAS & EPDCE_DMAS_OSOF) && time_out--)
		{
			printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
			schedule_timeout(5);
		}

		//dma_cache_wback_inv((long unsigned int)current_buffer, cb_len);
		printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
		memset(snp_buf, 0xff, 4);
		dma_cache_wback_inv((long unsigned int)snp_buf, 4);
	}
	else
	{
		int time_out = 20;
		while(!(REG_EPDCE_DMAS & EPDCE_DMAS_OEOF) && time_out--)
		{
			printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
			schedule_timeout(5);
		}

		//dma_cache_wback_inv((long unsigned int)current_buffer, cb_len);
		printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
		memset(snp_buf, 0xff, 4);
		dma_cache_wback_inv((long unsigned int)snp_buf, 4);
	}
	return 0;
}

static int jz4775_epdce_dis_epdce_rgb(unsigned char *current_buffer, int cb_len)
{
	int time_out = 20;
	while(!(REG_EPDCE_DMAS & EPDCE_DMAS_OEOF) && time_out--)
	{
		printk("--line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
		schedule_timeout(5);
	}

	//dma_cache_wback_inv((long unsigned int)current_buffer, cb_len);
	printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
	REG_EPDCE_IC |= EPDCE_IC_OSTOP | EPDCE_IC_OEOF | EPDCE_IC_OSOF | EPDCE_IC_ISTOP | EPDCE_IC_IEOF | EPDCE_IC_ISOF;

	printk("line: %d REG_EPDCE_DMAS = 0x%08x\n",__LINE__, REG_EPDCE_DMAS);
	REG_EPDCE_DMAC &= ~EPDCE_DMAC_CDMA_ENA;
	REG_EPDCE_CTRL &= ~EPDCE_CTRL_EPDCE_EN;
	printk("Epdce work over!\n\n");

	return 0;
}

static int jz4775_epdce_controller_init_rgb(void)
{
	int outputbits = 4;

	printk("%s %d\n", __FUNCTION__, __LINE__);
	REG_EPDCE_CTRL = EPDCE_CTRL_EPDCE_RST;
	REG_EPDCE_IFS =  799 << EPDCE_IFS_IN_FRM_VS | 199 << EPDCE_IFS_IN_FRM_HS;
	REG_EPDCE_OFS =  300 << EPDCE_OFS_O_FRM_HS;
	REG_EPDCE_XRID = 0xf;
	REG_EPDCE_XWID = 0xf;
	//REG_EPDCE_IM = EPDCE_IM_OSTOP_ENA | EPDCE_IM_OEOF_ENA | EPDCE_IM_OSOF_ENA | EPDCE_IM_ISTOP_ENA | EPDCE_IM_IEOF_ENA | EPDCE_IM_ISOF_ENA;

#if 1
	jz4775_epdce_csc_rgb565_to_yuv444_601_wide_mode();
	//jz4775_epdce_vee(vee_lut_16gs);
	jz4775_epdce_csc_yuv444_to_rgb888_601_wide_mode();
	//jz4775_y_33_clolor_filter_ena();
	jz4775_output_rgb();
	//jz4775_epdce_dither_y();
#else
	REG_EPDCE_IFS =  599 << EPDCE_IFS_IN_FRM_VS		|	799 << EPDCE_IFS_IN_FRM_HS;
	REG_EPDCE_OFS =  400 << EPDCE_OFS_O_FRM_HS;
	REG_EPDCE_OFC = EPDCE_OFC_SEL_Y;
	jz4775_epdce_csc_rgb565_to_yuv444_601_wide_mode();
	jz4775_epdce_vee(vee_lut_16gs);
	jz4775_dither_sel(outputbits);
#endif

	return 0;
}

static int jz4775epd_pan_display_rgb(unsigned char *frame_buffer, int fb_len, unsigned char *current_buffer, int cb_len)
{
	jz4775_epdce_controller_init_rgb();
	jz4775_epdce_ena_epdce_rgb(frame_buffer, fb_len, current_buffer, cb_len);
	print_epdce_registers();
	jz4775_epdce_auto_stop_rgb(current_buffer, cb_len);
	jz4775_epdce_dis_epdce_rgb(current_buffer, cb_len);

	return 0;
}
#endif


static int jz4775_epdce_alloc_dma_des(void)
{
	ides_addr = kmalloc(sizeof(struct dma_descriptor), GFP_KERNEL);
	odes_addr = kmalloc(sizeof(struct dma_descriptor), GFP_KERNEL);
	odes_addr_color = kmalloc(sizeof(struct dma_descriptor_color), GFP_KERNEL);

	if((ides_addr == NULL) || (odes_addr == NULL) || (odes_addr_color == NULL))
		return -ENOMEM;

	memset(ides_addr, 0, sizeof(struct dma_descriptor));
	memset(odes_addr, 0, sizeof(struct dma_descriptor));
	memset(odes_addr_color, 0, sizeof(struct dma_descriptor_color));

	return 0;
}

static int jz4775_epdce_free_dma_des(void)
{
	if(ides_addr)
		kfree(ides_addr);
	if(odes_addr)
		kfree(odes_addr);
	if(odes_addr_color)
		kfree(odes_addr_color);
	return 0;
}


static irqreturn_t jz4775_epdce_interrupt_handler(int irq, void *dev_id)
{
	unsigned int status = REG_EPDCE_IS;


	if(status & EPDCE_IS_ISOF)
	{
		REG_EPDCE_IC |= EPDCE_IC_ISOF;
		printk("EPDCE_IS_ISOF REG_EPDCE_IS = 0x%08x\n", REG_EPDCE_IS);
	}

	if(status & EPDCE_IS_IEOF)
	{
		REG_EPDCE_IC |= EPDCE_IC_IEOF;
		printk("EPDCE_IS_IEOF REG_EPDCE_IS = 0x%08x\n", REG_EPDCE_IS);
	}

	if(status & EPDCE_IS_ISTOP)
	{
		REG_EPDCE_IC |= EPDCE_IC_ISTOP;
		printk("EPDCE_IS_ISTOP REG_EPDCE_IS = 0x%08x\n", REG_EPDCE_IS);
	}

	if(status & EPDCE_IS_OSOF)
	{
		REG_EPDCE_IC |= EPDCE_IC_OSOF;
		printk("EPDCE_IS_OSOF REG_EPDCE_IS = 0x%08x\n", REG_EPDCE_IS);
	}

	if(status & EPDCE_IS_OEOF)
	{
		REG_EPDCE_IC |= EPDCE_IC_OEOF;
		printk("EPDCE_IS_OEOF REG_EPDCE_IS = 0x%08x\n", REG_EPDCE_IS);
		is_oeof = 1;
		wake_up(&interrupt_status_wq);
	}

	if(status & EPDCE_IS_OSTOP)
	{
		REG_EPDCE_IC |= EPDCE_IC_OSTOP;
		printk("EPDCE_IS_OSTOP REG_EPDCE_IS = 0x%08x\n", REG_EPDCE_IS);
		//REG_EPDCE_CTRL |= EPDCE_CTRL_EPDCE_RST;
	}

	printk("__%s__\n", __func__);
	return IRQ_HANDLED;
}

int jz4775_epdce_vee(unsigned char * vee_lut)
{
	int i;
	unsigned char msdb;
	unsigned int lut_addr;
	unsigned int entry;
	unsigned char *lut = vee_lut;
	for(i = 0; i < 256; i++)
	{

		lut_addr = (unsigned int)(&REG_EPDCE_VEE_LUT(i));
		msdb = (lut_addr & (0x3));
		entry = ((msdb << 30) | lut[i]);
		//if(i >= 4080 || i < 160)
		//printk("%d lut 0x%08x --> 0x%08x  entry = 0x%08x\n", i, lut_addr, &(REG_EPDCE_CR_LUT((i / 4) * 4)), entry);
		//memcpy(&REG_EPDCE_CR_LUT(i), &entry, 4);
		REG_EPDCE_VEE_LUT((i / 4) * 4) = entry;
		//printk("0x%08x\t0x%08x\n",&REG_EPDCE_VEE_LUT((i / 4) * 4), entry);
	}
	REG_EPDCE_VEEC = EPDCE_VEEC_ENA;
	return 0;
}

static int jz4775_epdce_controller_init(void)
{

/* EPDCE convert RGB565 to 16 grayscale data for EPD panel #ifdef */
#ifdef CONFIG_FB_JZ4775_EPDCE_OUTPUT_16_GRAYSCALE
	jz4775_epdce_output_y();

#ifdef CONFIG_FB_JZ4775_EPDCE_CSC_RGB565_TO_YUV444
/*#ifdef CONFIG_FB_JZ4775_EPDCE_CSC_RGB565_TO_YUV444_601_WIDE
	jz4775_epdce_csc_rgb565_to_yuv444_601_wide_mode();
#elif	CONFIG_FB_JZ4775_EPDCE_CSC_RGB565_TO_YUV444_601_NARROW
	jz4775_epdce_csc_rgb565_to_yuv444_601_narrow_mode();
#elif	CONFIG_FB_JZ4775_EPDCE_CSC_RGB565_TO_YUV444_709_WIDE
	jz4775_epdce_csc_rgb565_to_yuv444_709_wide_mode();
#elif	CONFIG_FB_JZ4775_EPDCE_CSC_RGB565_TO_YUV444_709_NARROW*/
#ifdef	CONFIG_FB_JZ4775_EPDCE_CSC_RGB565_TO_YUV444_709_NARROW
	jz4775_epdce_csc_rgb565_to_yuv444_709_narrow_mode();
#endif
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_Y_VEE
	jz4775_epdce_vee(vee_lut_16gs);
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_UV_HUE
	int hue_cos, hue_sin;
	hue_cos = CONFIG_FB_JZ4775_EPDCE_UV_HUE_COS;
	hue_sin = CONFIG_FB_JZ4775_EPDCE_UV_HUE_SIN;
	jz4775_uv_hue(hue_sin, hue_cos);
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_UV_SAT
	int k;
	k = CONFIG_FB_JZ4775_EPDCE_UV_SAT_CS_K;
	jz4775_uv_saturation(k);
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_CSC_YUV444_TO_RGB888
#if defined(CONFIG_FB_JZ4775_EPDCE_CSC_YUV444_TO_RGB888_601_WIDE)
	jz4775_epdce_csc_yuv444_to_rgb888_601_wide_mode();
#elif defined(CONFIG_FB_JZ4775_EPDCE_CSC_YUV444_TO_RGB888_601_NARROW)
	jz4775_epdce_csc_yuv444_to_rgb888_601_narrow_mode();
#elif defined(CONFIG_FB_JZ4775_EPDCE_CSC_YUV444_TO_RGB888_709_WIDE)
	jz4775_epdce_csc_yuv444_to_rgb888_709_wide_mode();
#elif defined(CONFIG_FB_JZ4775_EPDCE_CSC_YUV444_TO_RGB888_709_NARROW)
	jz4775_epdce_csc_yuv444_to_rgb888_709_narrow_mode();
#endif
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_Y
	int i;
	cfce ky;
	ky[0] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_Y_K0;
	ky[1] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_Y_K1;
	ky[2] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_Y_K2;
	ky[3] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_Y_K3;
	ky[4] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_Y_K4;
	ky[5] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_Y_K5;
	ky[6] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_Y_K6;
	ky[7] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_Y_K7;
	ky[8] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_Y_K8;

	for(i = 0; i < 9; i++)
		printk("Ky[%d] = %d\n",i, ky[i]);
	jz4775_epdce_y_33_color_filter(ky);
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_R
	int i;
	cfce kr;
	kr[0] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_R_K0;
	kr[1] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_R_K1;
	kr[2] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_R_K2;
	kr[3] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_R_K3;
	kr[4] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_R_K4;
	kr[5] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_R_K5;
	kr[6] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_R_K6;
	kr[7] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_R_K7;
	kr[8] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_R_K8;

	for(i = 0; i < 9; i++)
		printk("Kr[%d] = %d\n",i, kr[i]);
	jz4775_epdce_r_33_color_filter(kr);
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_G
	int i;
	cfce kg;
	kg[0] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_G_K0;
	kg[1] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_G_K1;
	kg[2] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_G_K2;
	kg[3] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_G_K3;
	kg[4] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_G_K4;
	kg[5] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_G_K5;
	kg[6] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_G_K6;
	kg[7] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_G_K7;
	kg[8] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_G_K8;

	for(i = 0; i < 9; i++)
		printk("Kg[%d] = %d\n",i, kg[i]);
	jz4775_epdce_g_33_color_filter(kg);
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_B
	int i;
	cfce kb;
	kb[0] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_B_K0;
	kb[1] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_B_K1;
	kb[2] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_B_K2;
	kb[3] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_B_K3;
	kb[4] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_B_K4;
	kb[5] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_B_K5;
	kb[6] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_B_K6;
	kb[7] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_B_K7;
	kb[8] = CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_B_K8;

	for(i = 0; i < 9; i++)
		printk("Kb[%d] = %d\n",i, kb[i]);
	jz4775_epdce_b_33_color_filter(kb);
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_DITHER_Y
	jz4775_epdce_dither_y();
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_DITHER_R
	jz4775_epdce_dither_r();
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_DITHER_G
	jz4775_epdce_dither_g();
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_DITHER_B
	jz4775_epdce_dither_b();
#endif

#ifdef CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_REMAP
#if defined(CONFIG_FB_JZ4775_EPDCE_COLOR_FILTER_REMAP_CFA)
	jz4775_color_remap_cfa();
#elif
	jz4775_color_remap_pa();
#endif
#endif


/* EPDCE convert RGB565 to RGB data for EPD color panel #elif */
#elif defined(CONFIG_FB_JZ4775_EPDCE_OUTPUT_RGB)


/* EPDCE convert RGB565 to RGBW data for EPD color panel #elif */
#elif defined(CONFIBG_FB_JZ4775_EPDCE_OUTPUT_RGBW)

#endif

	REG_EPDCE_XRID = 0xf;
	REG_EPDCE_XWID = 0xf;
	REG_EPDCE_IM &= ~EPDCE_IM_OEOF_MSK;

	return 0;
}

int jz4775_epdce_rgb565_to_16_grayscale(struct fb_var_screeninfo *fb_var, unsigned char *frame_buffer, unsigned char *current_buffer)
{
	int ret = 0;
	int fb_size, cb_size;
	int xres, yres, fb_bpp, cb_bpp;
	unsigned long delay;

	xres = fb_var->xres;
	yres = fb_var->yres;
	fb_bpp = fb_var->bits_per_pixel;
	cb_bpp = 4;

	fb_size = xres * yres * fb_bpp / 8;
	cb_size = xres * yres * cb_bpp / 8;

	REG_EPDCE_IFS =  (yres - 1) << EPDCE_IFS_IN_FRM_VS | (xres - 1) << EPDCE_IFS_IN_FRM_HS;
	REG_EPDCE_OFS =  xres / 2 << EPDCE_OFS_O_FRM_HS;

	memset(ides_addr, 0, sizeof(struct dma_descriptor));
	memset(odes_addr, 0, sizeof(struct dma_descriptor));
	ides_addr->next_des_addr = (unsigned int)virt_to_phys(ides_addr);
	ides_addr->fb_addr = (unsigned int)virt_to_phys(frame_buffer);
	ides_addr->dma_cmd |= EPDCE_IDMACMD_EOF_ENA | EPDCE_IDMACMD_STOP_ENA | fb_size;
	ides_addr->frm_id = 0;
	dma_cache_wback_inv((unsigned long)ides_addr, sizeof(struct dma_descriptor));
	REG_EPDCE_IDMADES = (unsigned int)virt_to_phys(ides_addr);

	printk("ides_addr->fb_addr = 0x%08x\n", ides_addr->fb_addr);
	printk("ides_addr->dma_cmd = 0x%08x\n", ides_addr->dma_cmd);
	printk("REG_EPDCE_IDMADES		= 0x%08x\n", REG_EPDCE_IDMADES);
	printk("ides_addr->next_des_addr	= 0x%08x\n", ides_addr->next_des_addr);
	printk("double word aligned ides_addr	= 0x%08x\n\n",(unsigned int)WORD_ALIGN(ides_addr, 2));

	odes_addr->next_des_addr = (unsigned int)virt_to_phys(odes_addr);
	odes_addr->fb_addr =(unsigned int)virt_to_phys(current_buffer);
	odes_addr->dma_cmd |= EPDCE_ODMACMD0_EOF_ENA | EPDCE_ODMACMD0_STOP_ENA | cb_size;
	odes_addr->frm_id = 0;
	dma_cache_wback_inv((unsigned long)odes_addr, sizeof(struct dma_descriptor));
	REG_EPDCE_ODMADES = (unsigned int)virt_to_phys(odes_addr);

	printk("odes_addr->fb_addr = 0x%08x\n", odes_addr->fb_addr);
	printk("odes_addr->dma_cmd = 0x%08x\n", odes_addr->dma_cmd);
	printk("REG_EPDCE_ODMADES		= 0x%08x\n", REG_EPDCE_ODMADES);
	printk("odes_addr->next_des_addr	= 0x%08x\n", odes_addr->next_des_addr);
	printk("double word aligned odes_addr	= 0x%08x\n\n",(unsigned int)WORD_ALIGN(odes_addr, 2));

	is_oeof = 0;
	REG_EPDCE_DMAC = EPDCE_DMAC_CDMA_ENA;
	REG_EPDCE_CTRL = EPDCE_CTRL_EPDCE_EN;

#ifdef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
	mdelay(5);
#else
	delay = jiffies + msecs_to_jiffies(EPDC_WAIT_MDELAY);
	while (time_before(jiffies, delay))
	{
		ret = wait_event_timeout(interrupt_status_wq, is_oeof == 1, msecs_to_jiffies(1));
		printk("ret = %d is_oeof = %d REG_EPDCE_IS = 0x%08x\n", ret, is_oeof, REG_EPDCE_IS);
		if (ret)
			break;
	}
#endif

	dma_cache_wback_inv((unsigned long)current_buffer, cb_size);
	//REG_EPDCE_IC |= EPDCE_IC_OSTOP | EPDCE_IC_OEOF | EPDCE_IC_OSOF | EPDCE_IC_ISTOP | EPDCE_IC_IEOF | EPDCE_IC_ISOF;

	REG_EPDCE_DMAC &= ~EPDCE_DMAC_CDMA_ENA;
	REG_EPDCE_CTRL &= ~EPDCE_CTRL_EPDCE_EN;

	return 0;
}
EXPORT_SYMBOL(jz4775_epdce_rgb565_to_16_grayscale);

static int __init jz4775_epdce_probe(struct platform_device *pdev)
{
	int err = 0;
	struct clk *epdce_clk;
	if(!pdev)
		goto failed;

	epdce_clk = clk_get(&pdev->dev, "epde");
	if(IS_ERR(epdce_clk)) {
		printk("epdce get clk is error\n");
		goto failed;
	}
	clk_enable(epdce_clk);

	init_waitqueue_head(&interrupt_status_wq);
	if(request_irq(IRQ_EPDCE, jz4775_epdce_interrupt_handler, IRQF_DISABLED,"lcd1", 0))
	{
		printk("%s %d request irq failed\n",__FUNCTION__,__LINE__);
		err = -EBUSY;
		goto failed_request_irq;
	}

	if(jz4775_epdce_alloc_dma_des())
		goto failed_alloc_dma_des;

	jz4775_epdce_controller_init();

	print_epdce_registers();
	return 0;

failed_alloc_dma_des:
	jz4775_epdce_free_dma_des();

failed_request_irq:
	free_irq(IRQ_EPDCE, NULL);

failed:
	return err;
}

static int jz4775_epdce_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int jz4775_epdce_suspend(struct platform_device *pdev, pm_message_t state)
{
	clk_disable(epdce_clk);
	return 0;
}

static int jz4775_epdce_resume(struct platform_device *pdev)
{
	clk_enable(epdce_clk);
	return 0;
}
#else
#define  jz4775_epdce_suspend NULL
#define  jz4775_epdce_resume NULL
#endif /* CONFIG_PM */

static struct platform_driver jz_epdce_driver =
{
	.probe = jz4775_epdce_probe,
	.remove = jz4775_epdce_remove,
	.suspend =	jz4775_epdce_suspend,
	.resume =	jz4775_epdce_resume,
	.driver =
	{
		.name = "jz4775-epdce",
	},
};

static int __init jz4775_epdce_init(void)
{
	return platform_driver_register(&jz_epdce_driver);
}

static void __exit jz4775_epdce_exit(void)
{
	platform_driver_unregister(&jz_epdce_driver);
}
module_init(jz4775_epdce_init);
module_exit(jz4775_epdce_exit);

MODULE_AUTHOR("Watson Zhu <wjzhu@ingenic.cn>");
MODULE_DESCRIPTION("JZ4775 EPD Color Engine Driver");
MODULE_LICENSE("GPL");
