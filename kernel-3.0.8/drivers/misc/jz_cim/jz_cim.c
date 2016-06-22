/*
 * linux/drivers/misc/cim.c -- Ingenic CIM driver
 *
 * Copyright (C) 2005-2010, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * NOTICE AND WARNNING: 
 * JZ4780-CIM YUV422 SUPPORT AUTO RECOVER.
 * NOTICE AND WARNNING: JZ4780-CIM YUV420 !NOT SUPPORT AUTO RECOVER.
 * IN YUV420 FORMAT, WHEN OVERFLOW OCCUR, CIM BEHAVIOUR IS UNEXPECTED. MAY CAUSE SYSTEM CRASH.
 *
 */
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/gpio.h>

#include <linux/regulator/consumer.h>

#include <mach/jz_cim.h>
#include "cim_reg.h"

//#define PDESC_NR	4
#define GET_BUF 2
#define SWAP_BUF 3
#define SWAP_NR (GET_BUF+SWAP_BUF)
#define RESERVE_BUF SWAP_NR
#define RESERVED_NR 1
#define CDESC_NR	1

//#define RESERVE_FRAME_DEBUG 1
//#define KERNEL_INFO_PRINT

//#define DUMP_CIM_REGESITER_BEFORE_START

LIST_HEAD(sensor_list);

enum cim_state {
	CS_IDLE,
	CS_PREVIEW,
	CS_CAPTURE,
};

#if 0
struct jz_cim_dma_desc {
	unsigned int next;
	unsigned int id;
	unsigned int yf_buf;
	unsigned int yf_cmd;
	unsigned int ycb_buf;
	unsigned int ycb_cmd;
	unsigned int ycr_buf;
	unsigned int ycr_cmd;
} __attribute__ ((aligned (16)));
#else
struct jz_cim_dma_desc {
	dma_addr_t next;
	unsigned int id;
	unsigned int buf;
	unsigned int cmd;
	/* only used when SEP = 1 */
	#if 1
	unsigned int cb_frame;
	unsigned int cb_len;
	unsigned int cr_frame;
	unsigned int cr_len;
	#endif
} __attribute__ ((aligned (32)));
#endif
struct cim_buf_info
{
	unsigned int paddr;
	unsigned int vaddr;
	int is_vbuf;
};
struct jz_cim {
	int irq;
	void __iomem *iomem;
	struct device *dev;
	struct clk *clk;
	struct clk *mclk;
	struct regulator * power;

	wait_queue_head_t wait;

	struct list_head list;

	volatile int frm_id;
	enum cim_state state;

	int sensor_count;

	void *pdesc_vaddr;
	void *cdesc_vaddr;
	struct jz_cim_dma_desc *preview;
	struct jz_cim_dma_desc *capture;

	struct cim_sensor *desc;
	struct miscdevice misc_dev;

	void (*power_on)(void);
	void (*power_off)(void);

	spinlock_t lock;
	struct frm_size psize;
	struct frm_size csize;
#ifdef CONFIG_GC0307
	struct frm_size offset;
#endif
	int tlb_flag;
	unsigned long tlb_base;
	unsigned long preview_output_format;
	unsigned long capture_output_format;
    	CameraYUVMeta p_yuv_meta_data[SWAP_NR];
	//CameraYUVMeta p_yuv_meta_data[PDESC_NR];
	CameraYUVMeta c_yuv_meta_data[CDESC_NR];
	int is_clock_enabled;
	int is_mclk_enabled;

	unsigned char num;
};

static unsigned int right_num = 0;
static unsigned int err_num = 0;
static int fresh_buf __read_mostly;


static void cim_dump_reg(struct jz_cim *cim);
static unsigned long inline reg_read(struct jz_cim *cim,int offset)
{
  	volatile unsigned long dummy_read;
	dummy_read = readl(cim->iomem + offset); //read twice
	udelay(10); //this is necessary
	return readl(cim->iomem+ offset);
}

static void inline reg_write(struct jz_cim *cim,int offset, unsigned long val)
{
	writel(val, cim->iomem + offset); //write twice
	udelay(10);
	writel(val, cim->iomem + offset);
}

static void inline bit_set(struct jz_cim *cim ,int offset,int bit)
{
	unsigned long val;
	val = reg_read(cim,offset);
	val |= bit;
	reg_write(cim,offset,val);
}

static void inline bit_clr(struct jz_cim *cim ,int offset,int bit)
{
	unsigned long val;
	val = reg_read(cim,offset);
	val &= ~bit;
	reg_write(cim,offset,val);
}

static inline void cim_enable(struct jz_cim *cim)
{
#ifdef DUMP_CIM_REGESITER_BEFORE_START
	cim_dump_reg(cim);
#endif
	bit_set(cim,CIM_CTRL,CIM_CTRL_ENA);
}

static inline void cim_disable(struct jz_cim *cim)
{
	bit_clr(cim,CIM_CTRL,CIM_CTRL_ENA);
}

static inline void cim_reset(struct jz_cim *cim)
{
	bit_set(cim,CIM_CTRL,CIM_CTRL_CIM_RST);
}

static inline void cim_enable_dma(struct jz_cim *cim)
{
	bit_set(cim,CIM_CTRL,CIM_CTRL_DMA_EN);
}

static inline void cim_disable_dma(struct jz_cim *cim)
{
	bit_clr(cim,CIM_CTRL,CIM_CTRL_DMA_EN);
}

static inline void cim_clear_rfifo(struct jz_cim *cim)
{
	bit_set(cim,CIM_CTRL,CIM_CTRL_RXF_RST);
	udelay(100);   //clear rxfifo is necessary
	bit_clr(cim,CIM_CTRL,CIM_CTRL_RXF_RST);
}

static inline void cim_clear_state(struct jz_cim *cim)
{
	reg_write(cim,CIM_STATE,0);
}

static inline void cim_enable_fsc_intr(struct jz_cim *cim)
{
	bit_clr(cim,CIM_IMR,CIM_IMR_FSEM);
}

static inline void cim_enable_eof_intr(struct jz_cim *cim)
{
	bit_clr(cim,CIM_IMR,CIM_IMR_EOFM);
}

static inline void cim_enable_rxfifo_overflow_intr(struct jz_cim *cim)
{
	bit_clr(cim,CIM_IMR,CIM_IMR_RFIFO_OFM);
}

static inline void cim_enable_priority_control(struct jz_cim *cim)
{
	bit_set(cim,CIM_CTRL2,CIM_CTRL2_APM);
}

static inline void cim_enable_emergency(struct jz_cim *cim)
{
	bit_set(cim,CIM_CTRL2,CIM_CTRL2_EME);
}

static inline void cim_enable_tlb_error_intr(struct jz_cim *cim)
{
	bit_clr(cim,CIM_IMR,CIM_IMR_TLBEM);
}

static inline void cim_disable_tlb_error_intr(struct jz_cim *cim)
{
	bit_set(cim,CIM_IMR,CIM_IMR_TLBEM);
}

static inline void cim_set_da(struct jz_cim *cim,void * addr)
{
	printk("__%s__\n", __func__);

	reg_write(cim,CIM_DA,(unsigned long)addr);
}

static inline bool cim_get_and_check_cmd(struct jz_cim *cim)
{
	unsigned long cmd_len, tmp;
	int time_out = 0;
	bool ret;

	while(1) {
		cmd_len = reg_read(cim,CIM_CMD);
		cmd_len &= 0xffffff;
		mdelay(1);
		tmp = reg_read(cim,CIM_CMD);
		tmp &= 0xffffff;
		if(tmp == cmd_len) {
			ret = true;
			break;
		}
		if(100 == (++time_out)) {
			ret = false;
			break;
		}
	}

	return ret;
}

static inline unsigned long cim_get_iid(struct jz_cim *cim)
{
	return reg_read(cim,CIM_IID);
}


static inline unsigned long check_iid(struct jz_cim *cim, unsigned long fid)
{
    //if ( fid >= 0 && fid < PDESC_NR)
    if ( fid >= 0 && fid < SWAP_BUF)
        return 1;
    else 
        return 0;
}

static inline unsigned long cim_get_and_check_iid(struct jz_cim *cim)
{
    unsigned long fid;
    int loop = 4;

    do {
        unsigned long fid2;
        fid = reg_read(cim,CIM_IID);;  //we'd better read IID as fast as we can after we go into irq
        fid2 = reg_read(cim,CIM_IID);;  //we'd better read IID as fast as we can after we go into irq

        //if(fid == fid2 && fid >= 0 && fid < PDESC_NR) {
        if(fid == fid2 && fid >= 0 && fid < SWAP_BUF) {
            break;
        }

    } while (--loop);

    //if(fid < 0 || fid >= PDESC_NR) {
    if(fid < 0 || fid >= SWAP_BUF) {
        dev_info(cim->dev, " ----------cim_get_and_check_iid iid %08lx, REG_CIM_IID = 0x%08lx \n",
                 fid, reg_read(cim,CIM_IID));
        fid = 0;
    }

    return fid;
}


static inline unsigned long cim_get_fid(struct jz_cim *cim)
{
	return reg_read(cim,CIM_FID);
}

static inline unsigned long cim_read_state(struct jz_cim *cim)
{
	return reg_read(cim,CIM_STATE);
}

static inline void cim_reset_tlb(struct jz_cim *cim)
{
	bit_set(cim,CIM_TC,CIM_TC_RST);
	bit_clr(cim,CIM_TC,CIM_TC_RST);
}

static inline void cim_enable_tlb(struct jz_cim *cim)
{
	bit_set(cim,CIM_TC,CIM_TC_ENA);
}

static inline void cim_disable_tlb(struct jz_cim *cim)
{
	bit_clr(cim,CIM_TC,CIM_TC_ENA);
}

int cim_set_tlbbase(struct jz_cim *cim)
{
	unsigned long regval = 0;

#ifdef KERNEL_INFO_PRINT
	dev_info(cim->dev,"cim set tlb base is %lx\n",cim->tlb_base);
#endif
	if(cim->tlb_base & 0x3){//bit[0:1] must be 0.double word aligned
		dev_err(cim->dev,"cim tlb base is not valid address\n");
		return -1;
	}
	regval = reg_read(cim,CIM_TC);
	regval = cim->tlb_base | (regval & 0x3);
	reg_write(cim,CIM_TC,regval);
	return 0;
}

static void cim_dump_dma_desc(struct jz_cim *cim)
{
    struct jz_cim_dma_desc * desc;
    int ddd, iii;
    const int dma_desc_size = sizeof(struct jz_cim_dma_desc)/sizeof(unsigned int);

    desc  = (struct jz_cim_dma_desc *)cim->pdesc_vaddr;
    if (!desc) return;

    printk("-----------------------------------------------------\n");
    //printk("\tcim->pdesc_vaddr= %p, PDESC_NR=%d\n", desc, PDESC_NR);
    printk("\tcim->pdesc_vaddr= %p, SWAP_BUF=%d\n", desc, SWAP_BUF);
   // for (ddd=0; ddd<PDESC_NR; ddd++, desc++) {
     for (ddd=0; ddd<SWAP_BUF; ddd++, desc++) { // 3
        unsigned int *ppp = (unsigned int *)desc;
        printk("\tdesc%d= %p\n", ddd, desc);
        for (iii=0;iii<dma_desc_size; iii++)
            printk("\t\t: %08x\n", *ppp++);
    }

    desc  = (struct jz_cim_dma_desc *)cim->cdesc_vaddr;
    if (!desc) return;
    printk("-----------------------------------------------------\n");
    printk("\tcim->cdesc_vaddr= %p CDESC_NR=%d\n", desc, CDESC_NR);
    for (ddd=0; ddd<CDESC_NR; ddd++, desc++) {
        unsigned int *ppp = (unsigned int *)desc;
        printk("\tdesc%d= %p\n", ddd, desc);
        for (iii=0;iii<dma_desc_size; iii++)
            printk("\t\t: %08x\n", *ppp++);
    }

}

static void cim_dump_reg(struct jz_cim *cim)
{
    printk("-----------------------------------------------------\n");
	dev_info(cim->dev,"REG_CIM_CFG \t= \t0x%08lx\n", reg_read(cim,CIM_CFG));
	dev_info(cim->dev,"REG_CIM_CTRL \t= \t0x%08lx\n", reg_read(cim,CIM_CTRL));
	dev_info(cim->dev,"REG_CIM_CTRL2 \t= \t0x%08lx\n", reg_read(cim,CIM_CTRL2));
	dev_info(cim->dev,"REG_CIM_STATE \t= \t0x%08lx\n", reg_read(cim,CIM_STATE));
	dev_info(cim->dev,"REG_CIM_IMR \t= \t0x%08lx\n", reg_read(cim,CIM_IMR));
	dev_info(cim->dev,"REG_CIM_IID \t= \t0x%08lx\n", reg_read(cim,CIM_IID));
	dev_info(cim->dev,"REG_CIM_DA \t= \t0x%08lx\n", reg_read(cim,CIM_DA));
	dev_info(cim->dev,"REG_CIM_FA \t= \t0x%08lx\n", reg_read(cim,CIM_FA));
	dev_info(cim->dev,"REG_CIM_FID \t= \t0x%08lx\n", reg_read(cim,CIM_FID));
	dev_info(cim->dev,"REG_CIM_CMD \t= \t0x%08lx\n", reg_read(cim,CIM_CMD));
	dev_info(cim->dev,"REG_CIM_WSIZE \t= \t0x%08lx\n", reg_read(cim,CIM_SIZE));
	dev_info(cim->dev,"REG_CIM_WOFFSET \t= \t0x%08lx\n", reg_read(cim,CIM_OFFSET));
	dev_info(cim->dev,"REG_CIM_FS \t= \t0x%08lx\n", reg_read(cim,CIM_FS));
	dev_info(cim->dev,"REG_CIM_YFA \t= \t0x%08lx\n", reg_read(cim,CIM_YFA));
	dev_info(cim->dev,"REG_CIM_YCMD \t= \t0x%08lx\n", reg_read(cim,CIM_YCMD));
	dev_info(cim->dev,"REG_CIM_CBFA \t= \t0x%08lx\n", reg_read(cim,CIM_CBFA));
	dev_info(cim->dev,"REG_CIM_CBCMD \t= \t0x%08lx\n", reg_read(cim,CIM_CBCMD));
	dev_info(cim->dev,"REG_CIM_CRFA \t= \t0x%08lx\n", reg_read(cim,CIM_CRFA));
	dev_info(cim->dev,"REG_CIM_CRCMD \t= \t0x%08lx\n", reg_read(cim,CIM_CRCMD));
	dev_info(cim->dev,"REG_CIM_TC \t= \t0x%08lx\n", reg_read(cim,CIM_TC));
	dev_info(cim->dev,"REG_CIM_TINX \t= \t0x%08lx\n", reg_read(cim,CIM_TINX));
	dev_info(cim->dev,"REG_CIM_TCNT \t= \t0x%08lx\n", reg_read(cim,CIM_TCNT));

        cim_dump_dma_desc(cim);

    printk("-----------------------------------------------------\n");
}

static inline void cim_enable_mclk(struct jz_cim *cim)
{
	if(!cim->is_mclk_enabled) {
		if(cim->mclk)
			clk_enable(cim->mclk);
		cim->is_mclk_enabled = 1;
	}
}

static inline void cim_disable_mclk(struct jz_cim *cim)
{
	if(cim->is_mclk_enabled) {
		if(cim->mclk)
			clk_disable(cim->mclk);
		cim->is_mclk_enabled = 0;
	}
}

void cim_power_on(struct jz_cim *cim)
{
	if(!cim->is_clock_enabled) {
		if(cim->clk)
			clk_enable(cim->clk);
		
		if(cim->mclk) {
#define CIM_CLOCK_TEST (24000000)
                    clk_set_rate(cim->mclk, CIM_CLOCK_TEST);
                    clk_enable(cim->mclk);
		}
		cim->is_clock_enabled = 1;
		cim->is_mclk_enabled = 1;
	}
		
	if(cim->power) {
		if (!regulator_is_enabled(cim->power)) {
			regulator_enable(cim->power);
			dev_info(cim->dev, "cim power on\n");
		}
	}

	msleep(10);
}

void cim_power_off(struct jz_cim *cim)
{
	if(cim->is_clock_enabled) {
		if(cim->clk)
			clk_disable(cim->clk);
		
		if(cim->mclk)
			cim_disable_mclk(cim);
		cim->is_clock_enabled = 0;
	}
	
	if(cim->power) {
		if (regulator_is_enabled(cim->power)) {
			regulator_disable(cim->power);
			dev_info(cim->dev, "cim power off\n");
		}
	}
}

void cim_set_default(struct jz_cim *cim)
{
	unsigned long cfg = 0;
	unsigned long ctrl = 0;
	unsigned long ctrl2 = 0;
	unsigned long fs = 0;
	int w = 0,h = 0;

        /* WARNNING */
        if(cim->preview_output_format != CIM_BYPASS_YUV422I) {
            dev_err(cim->dev,"NOTICE AND WARNNING: JZ4780-CIM YUV422 SUPPORT AUTO RECOVER.\n");
            dev_err(cim->dev,"NOTICE AND WARNNING: JZ4780-CIM YUV420 !NOT SUPPORT AUTO RECOVER.\n");
            dev_err(cim->dev,"\tIN YUV420 FORMAT, WHEN OVERFLOW OCCUR, CIM BEHAVIOUR IS UNEXPECTED, MAY CAUSE SYSTEM CRASH.\n");
        }

	if(cim->state == CS_PREVIEW) {
		w = cim->psize.w;
		h = cim->psize.h;

		if(cim->preview_output_format == CIM_CSC_YUV420P) {
			ctrl2 |= CIM_CTRL2_CSC_YUV420;
			cfg |= CIM_CFG_SEP;
	
		} else if(cim->preview_output_format == CIM_CSC_YUV420B) {
			ctrl |= CIM_CTRL_MBEN;
			ctrl2 |= CIM_CTRL2_CSC_YUV420;
			cfg |= CIM_CFG_SEP;	
		} 
		
	} else if (cim->state == CS_CAPTURE) {
		w = cim->csize.w;
		h = cim->csize.h;
		
		if(cim->capture_output_format == CIM_CSC_YUV420P) {
			ctrl2 |= CIM_CTRL2_CSC_YUV420;
			cfg |= CIM_CFG_SEP;
			
		} else if(cim->capture_output_format == CIM_CSC_YUV420B) {
			ctrl |= CIM_CTRL_MBEN;		//macro(tile) mode
			ctrl2 |= CIM_CTRL2_CSC_YUV420;
			cfg |= CIM_CFG_SEP;	
		}
	}

        /* NOTICE AND WARNNING
         * only package YUV422I CIM_CTRL_DMA_SYNC and CIM_CMD_OFRCV;
         * YUV420B and YUV420P must not use CIM_CTRL_DMA_SYNC and CIM_CMD_OFRCV.
         */
        //if(cim->preview_output_format == CIM_BYPASS_YUV422I)
            ctrl |= CIM_CTRL_DMA_SYNC;
	ctrl |= CIM_CTRL_FRC_1;

	ctrl2 |= CIM_CTRL2_APM | CIM_CTRL2_EME | CIM_CTRL2_OPE | (2 << CIM_CTRL2_OPG_BIT);

        // If delete "CIM_CTRL2_FSC | CIM_CTRL2_ARIF", maybe cause overflow on warrior(npm706) board.
        //if(cim->preview_output_format == CIM_BYPASS_YUV422I)
#if defined(CONFIG_TVP5150) || defined(CONFIG_ADV7180)
	ctrl2 |= CIM_CTRL2_ARIF;
#else
	ctrl2 |= CIM_CTRL2_FSC | CIM_CTRL2_ARIF;
#endif
	cfg |= cim->desc->cim_cfg | CIM_CFG_DF_YUV422;

        /* NOTICE AND WARNNING
         * 2013-01-31, Preview size: 640x480 30fps, dma burst length:
         *     CIM_BYPASS_YUV422I: DMA_BURST_INCR32 stable
         *     YUV420_Tile:  DMA_BURST_INCR32 not stable, DMA_BURST_INCR16 stable.
         * 2013-02-01, Preview size: 1280x720 16fps, dma burst length:
         *     CIM_BYPASS_YUV422I: DMA_BURST_INCR32 stable
         *     YUV420_Tile:  DMA_BURST_INCR32 much stable, DMA_BURST_INCR16 not stable.
         */
	cfg &= ~(CIM_CFG_DMA_BURST_TYPE_MASK);
        if(cim->preview_output_format == CIM_BYPASS_YUV422I)
            cfg |= CIM_CFG_DMA_BURST_INCR32;
        else 
            cfg |= CIM_CFG_DMA_BURST_INCR32; /* 16? 32? */

#ifdef CONFIG_OV5640_RAW_BAYER
	reg_write(cim, CIM_CFG, 0x00001c42);
	reg_write(cim, CIM_CTRL, 0x00000084);
//	reg_write(cim,CIM_CTRL2, 0x00c00027);	//bypass
	reg_write(cim,CIM_CTRL2, 0x00000027);	//bypass
	fs = (640 -1)<< CIM_FS_FHS_BIT | (480 -1)<< CIM_FS_FVS_BIT | 2 << CIM_FS_BPP_BIT;
	reg_write(cim,CIM_FS,fs);	// VGA & one byte per pixel
#else
	fs = (w -1)<< CIM_FS_FHS_BIT | (h -1)<< CIM_FS_FVS_BIT | 1<< CIM_FS_BPP_BIT;

	reg_write(cim,CIM_CFG,cfg);
	reg_write(cim,CIM_CTRL,ctrl);
	reg_write(cim,CIM_CTRL2,ctrl2);
	reg_write(cim,CIM_FS,fs);
#endif

//	cim_enable_fsc_intr(cim);
	cim_enable_eof_intr(cim);
	cim_enable_rxfifo_overflow_intr(cim);
	cim_enable_tlb_error_intr(cim);
}



int camera_sensor_register(struct cim_sensor *desc)
{
	if(!desc) 
		return -EINVAL;
	desc->id = 0xffff;
	list_add_tail(&desc->list,&sensor_list);
	return 0;
}

void cim_scan_sensor(struct jz_cim *cim)
{
	struct cim_sensor *desc;
	struct list_head *tmp;
	cim->sensor_count  = 0;
	cim_power_on(cim);
	
	list_for_each_entry(desc, &sensor_list, list) {
		//desc->power_on(desc);
		if(desc->probe(desc)){
			tmp = desc->list.prev;
			list_del(&desc->list);
			desc = list_entry(tmp, struct cim_sensor, list);
		}
		//desc->shutdown(desc);
	}

	list_for_each_entry(desc, &sensor_list, list) {
			desc->first_used = true;
			if(desc->facing == CAMERA_FACING_BACK && 
			   ((desc->pos == 0) ? 1 : ((desc->pos - 1) == cim->num)) ) {
				desc->id = cim->sensor_count;
				cim->sensor_count++;
				cim->desc = desc;
				dev_info(cim->dev,"sensor_name:%s\t\tid:%d facing:%d pos:%d\n",
						desc->name,desc->id,desc->facing,desc->pos);
			}
	}

	list_for_each_entry(desc, &sensor_list, list) {
			if(desc->facing == CAMERA_FACING_FRONT &&
			   ((desc->pos == 0) ? 1 : ((desc->pos - 1) == cim->num)) ) {
				desc->id = cim->sensor_count;
				cim->sensor_count++;
				cim->desc = desc;
				dev_info(cim->dev,"sensor_name:%s\t\tid:%d facing:%d pos:%d\n",
						desc->name,desc->id,desc->facing,desc->pos);
			}
	}

	cim_power_off(cim);
}

static int cim_select_sensor(struct jz_cim *cim,int id)
{
	struct cim_sensor *desc;

	if(cim->state != CS_IDLE)
		return -EBUSY;
	list_for_each_entry(desc, &sensor_list, list) {
		if(desc->id == id && ((desc->pos == 0) ? 1 : ((desc->pos - 1) == cim->num))) {
			printk(" %s() found desc->id:%d id:%d pos:%d num:%d \n", __func__,desc->id,id,desc->pos,cim->num);
			cim->desc = desc;
			desc = NULL;
			break;
		} 
	}

	if(desc != NULL) {
		printk(" [WARNING!!!] %s failed\n", __func__);
		return -EFAULT;
	}
	cim->desc->first_used = true;
	return 0;
}
static int cim_enable_image_mode(struct jz_cim *cim,int image_w,int image_h,int width,int height)
{
	int voffset = 0;
	int hoffset = 0;
	int half_words_per_line = 0;
	unsigned int ctrl = reg_read(cim,CIM_CTRL);

	hoffset = 0;
        half_words_per_line = image_w;
	unsigned int wsize = 0;
	unsigned int woffset = 0;

	wsize = image_h << 16 | half_words_per_line;

	reg_write(cim,CIM_SIZE,wsize);

	woffset = reg_read(cim,CIM_OFFSET);
	woffset &= ~(0x1fff << 16);
	woffset &= ~(0x1fff);

#ifdef CONFIG_ADV7180
	if(cim->desc->para.standard == 1 && image_h == 480) //PAL
	{
		voffset = 24;
	}
	else if(cim->desc->para.standard == 2 && image_h == 480)     //NTSC
	{
		voffset = 16;
		hoffset = 24;
	}
#endif
#ifdef CONFIG_GC0307
	voffset = cim->offset.w;
	hoffset = cim->offset.h;
#endif
	woffset |= (voffset << 16) | hoffset;
	reg_write(cim,CIM_OFFSET,woffset);

	ctrl |= (1 << 14);
	reg_write(cim,CIM_CTRL,ctrl);
	printk("enable image mode (real size %d x %d) - %d x %d\n", width, height, image_w, image_h);
	return 0;
}

static long cim_set_capture_size(struct jz_cim *cim)
{
#if (!defined(CONFIG_TVP5150) && !defined(CONFIG_ADV7180) && !defined(CONFIG_GC0307))
	int i =0;
	struct frm_size * p = cim->desc->capture_size;
	for(i=0;i<cim->desc->cap_resolution_nr;i++){	 
		if(cim->csize.w == p->w && cim->csize.h == p->h){
#ifdef KERNEL_INFO_PRINT
			dev_info(cim->dev,"Found the capture size %d * %d in sensor table\n",cim->csize.w,cim->csize.h);
#endif
			break;
		}
		p++;
	}
	
	if(i>= cim->desc->cap_resolution_nr){
		dev_err(cim->dev,"Cannot found the capture size %d * %d in sensor table\n",cim->csize.w,cim->csize.h);
		return -1;
	}
	if(cim->state == CS_CAPTURE)
		cim->desc->set_resolution(cim->desc,cim->csize.w,cim->csize.h);
	return 0;
#else /*tvp5150 or adv7180*/
	int i =0;
	int fs_w, fs_h, fs_bpp;
	struct frm_size * p = &(cim->desc->capture_size[cim->desc->cap_resolution_nr - 1]);

	for(i=0;i<cim->desc->cap_resolution_nr;i++){
		if(cim->csize.w == p->w && cim->csize.h == p->h){
#ifdef KERNEL_INFO_PRINT
			dev_info(cim->dev,"Found the capture size %d * %d in sensor table\n",cim->csize.w,cim->csize.h);
#endif
			break;
		}
		p = &(cim->desc->capture_size[cim->desc->cap_resolution_nr -1- i]);
	}
#ifdef CONFIG_ADV7180
	if(cim->desc->para.standard == 1)
	{
		p->w = cim->desc->preview_size[0].w;
		p->h = cim->desc->preview_size[0].h;
	}
	else if(cim->desc->para.standard == 2)
	{
		p->w = cim->desc->preview_size[1].w;
		p->h = cim->desc->preview_size[1].h;
	}

	cim_enable_image_mode(cim,cim->csize.w,cim->csize.h,p->w,p->h);
	fs_w = p->w;
	fs_h = p->h;
	fs_bpp = 16;

#else

	if(i>= cim->desc->cap_resolution_nr){
	dev_err(cim->dev,"Cannot found the capture size %d * %d in sensor table\n",cim->csize.w,cim->csize.h);
			printk("use window\n");
      printk("i = %d\n,cim->desc->cap_resolution_nr = %d\n",i,cim->desc->cap_resolution_nr);
      cim_enable_image_mode(
                      cim,
                      cim->csize.w,
                      cim->csize.h,
                      p->w,
                      p->h);
      fs_w = p->w;//cim->desc->preview_size[cim->desc->prev_resolution_n];
      fs_h = p->h;
      fs_bpp = 16;
	}
#endif
	unsigned int fs = 0;
	fs = ((fs_h - 1) << 16) | ((fs_bpp/8-1) << 14)| ((fs_w-1) << 0);
	reg_write(cim,CIM_FS,fs);
	return 0;
#endif
}

static long cim_set_preview_size(struct jz_cim *cim)
{
#if (!defined(CONFIG_TVP5150) && !defined(CONFIG_ADV7180) && !defined(CONFIG_GC0307))
	int i =0;
	struct frm_size * p = cim->desc->preview_size;
	for(i=0;i<cim->desc->prev_resolution_nr;i++){	 
		if(cim->psize.w == p->w && cim->psize.h == p->h){			
#ifdef KERNEL_INFO_PRINT
			dev_info(cim->dev,"Found the preview size %d * %d in sensor table\n",cim->psize.w,cim->psize.h);
#endif
			break;
		}
		p++;
	}
	
	if(i>= cim->desc->prev_resolution_nr){
		dev_err(cim->dev,"Cannot found the preview size %d * %d in sensor table\n",cim->psize.w,cim->psize.h);
		return -1;
	}
	if(cim->state == CS_PREVIEW)
		cim->desc->set_resolution(cim->desc,cim->psize.w,cim->psize.h);
	return 0;
#else
	int i =0;
	int index = 0;
	int fs_w, fs_h, fs_bpp;
	unsigned int max_width = 0,max_height = 0;

#ifdef CONFIG_ADV7180
	if(cim->desc->para.standard == 1)
	{
		printk("\n***PAL MODE***\n");
		max_width = cim->desc->preview_size[0].w;
		max_height = cim->desc->preview_size[0].h;
	}
	else if(cim->desc->para.standard == 2){
		printk("\n***NTSC MODE***\n");
		max_width = cim->desc->preview_size[1].w;
		max_height = cim->desc->preview_size[1].h;
	}
#else	// Maybe CONFIG_TVP5150
	max_width = cim->desc->preview_size[0].w;
	max_height = cim->desc->preview_size[0].h;
#endif
	printk("\n*******cim_set_preview_size********\n");
	printk("cim->psize.w = %d\tcim->psize.h = %d\n",cim->psize.w,cim->psize.h);
	struct frm_size * p ;

	for(i = 0; i<cim->desc->prev_resolution_nr; i++) {
		if( cim->psize.w == cim->desc->preview_size[i].w &&
			cim->psize.h == cim->desc->preview_size[i].h ) {
			printk("This sensor support the request preview size: %d x %d\n", cim->psize.w, cim->psize.h);
			break;
		}
	}

#ifdef CONFIG_ADV7180
	if(cim->desc->para.standard == 2)  i = i + 1; // ntsc use window
#else
	if(i>= cim->desc->prev_resolution_nr){
		printk("Cannot found the preview size %d * %d in sensor table\n",cim->psize.w,cim->psize.h);

		index = cim->desc->prev_resolution_nr - 1;
		do {
			p = &(cim->desc->preview_size[index]);
			if (p->w >= cim->psize.w && p->h >= cim->psize.h) {
				printk("We will use the size(%d,%d) preview. index:%d\n", p->w, p->h, index);
				break;
			}
		} while (--index >= 0);

		if (index < 0) {
			printk("preview size greater than sensor support size.\n");
			p = &(cim->desc->preview_size[0]);
			// FIXME : I don't know this operation is right.(twxie)
		}
	}
#endif

	if(i>= cim->desc->prev_resolution_nr){
		printk("use window\n");
		printk("i = %d\t,cim->desc->prev_resolution_nr = %d\n",i,cim->desc->prev_resolution_nr);

		cim_enable_image_mode(
				cim,
				cim->psize.w,
				cim->psize.h,
				p->w,
				p->h);

		fs_w = p->w;//cim->desc->preview_size[cim->desc->prev_resolution_n];
		fs_h = p->h;
		fs_bpp = 16;
	}
	else
	{
		unsigned int ctrl = reg_read(cim,CIM_CTRL);
		ctrl &= ~(1 << 14);
		reg_write(cim,CIM_CTRL,ctrl);
		fs_w = cim->desc->preview_size[i].w;//p->w;
		fs_h = cim->desc->preview_size[i].h;//p->h;
		fs_bpp =16;
	}
	unsigned int fs = 0;
	fs = ((fs_h - 1) << 16) | ((fs_bpp/8-1) << 14)| ((fs_w-1) << 0);
	reg_write(cim,CIM_FS,fs);

	return 0;
#endif
}
static irqreturn_t cim_irq_handler(int irq, void *data)
{
	struct jz_cim * cim = (struct jz_cim * )data;
	unsigned long state_reg = 0;
	unsigned long flags;
	static int wait_count = 0;
	int fid;
	static int irq_count = 0;
        irq_count++;

	state_reg = cim_read_state(cim);
	fid = reg_read(cim,CIM_IID);

        //if ( ( !(state_reg & CIM_STATE_DMA_EOF) ) || state_reg == fid || fid < 0 || fid > 3 ) {
        if ( ( !(state_reg & CIM_STATE_DMA_EOF) ) || state_reg == fid || fid < 0 || fid > (SWAP_BUF-1) ) {
            dev_err(cim->dev,"%s L%d: %d, state=%#x, fid=%#x, CIM_IID=%#x\n", __func__, __LINE__,
                     irq_count, (unsigned int)state_reg,fid, (unsigned int)reg_read(cim,CIM_IID));
        }

	if ((state_reg & CIM_STATE_RXF_OF)
	    || (state_reg & CIM_STATE_Y_RXF_OF)
	    || (state_reg & CIM_STATE_CB_RXF_OF)
	    || (state_reg & CIM_STATE_CR_RXF_OF)
	    ){
            dev_err(cim->dev," -------irq_count=%d Rx FIFO OverFlow interrupt! \tstate_reg= %#lx\n", irq_count, state_reg);
            //printk("bit_clr CIM_STATE_RXF_OF\n");
            //bit_clr(cim,CIM_STATE,CIM_STATE_RXF_OF|CIM_STATE_Y_RXF_OF|CIM_STATE_CB_RXF_OF|CIM_STATE_CR_RXF_OF);
            cim_disable(cim);

            /* printk("cim_disable  \n"); */
            printk(" CIM_YFA =%08lx CIM_YCMD =%08lx \n", reg_read(cim, CIM_YFA), reg_read(cim, CIM_YCMD));
            printk(" CIM_CBFA=%08lx CIM_CBCMD=%08lx \n", reg_read(cim, CIM_CBFA), reg_read(cim, CIM_CBCMD));
            printk(" CIM_CRFA=%08lx CIM_CRCMD=%08lx \n", reg_read(cim, CIM_CRFA), reg_read(cim, CIM_CRCMD));
            cim_dump_dma_desc(cim);
            //printk("cim_clear_rfifo\n");
            cim_clear_rfifo(cim);
            //printk("cim_clear_state\n");
            cim_clear_state(cim);	// clear state register
            /* reset da */
            {
                if(cim->state == CS_PREVIEW){
                    printk("cim_set_da cim->preview=%p\n", cim->preview);
                    cim_set_da(cim,cim->preview);
                }
                else if(cim->state == CS_CAPTURE){
                    printk("cim_set_da cim->capture=%p\n", cim->capture);
                    cim_set_da(cim,cim->capture);
                }
            }
            //printk("cim_enable\n");
            cim_enable(cim);
            //printk("return IRQ_HANDLED\n");
            return IRQ_HANDLED;
	}
	
	if(state_reg & CIM_STATE_TLB_ERR){
		
		dev_err(cim->dev," ------- TLB Error interrupt!\n");
		
		cim->state = CS_IDLE;
		cim_disable(cim);
		cim_disable_dma(cim);
		cim_reset_tlb(cim);	
		if(waitqueue_active(&cim->wait))
			wake_up_interruptible(&cim->wait);
		return IRQ_HANDLED;
	}

	if(state_reg & CIM_STATE_DMA_EOF){
		if(cim->state == CS_PREVIEW){
			bit_clr(cim,CIM_STATE,CIM_STATE_DMA_EOF);

           spin_lock_irqsave(&cim->lock,flags);
			fid = cim_get_and_check_iid(cim);  //we'd better read IID as fast as we can after we go into irq
			dev_dbg(cim->dev,"irq eof preview, iid=%d fid %ld iid %ld\n", fid, reg_read(cim,CIM_FID),reg_read(cim,CIM_IID));

			fid--;   //make sure it is not the buffer being used
			if(fid == -1) {
               //fid = PDESC_NR -1; // 4 - 1
				fid = SWAP_BUF -1; // 3 - 1
            }
			//spin_lock_irqsave(&cim->lock,flags);
			cim->frm_id  = fid;
			spin_unlock_irqrestore(&cim->lock,flags);
			
			if(waitqueue_active(&cim->wait))
				wake_up_interruptible(&cim->wait);

			return IRQ_HANDLED;
		}
		else if(cim->state == CS_CAPTURE){
			wait_count ++;
			//dev_info(cim->dev,"capture frame wait : %d\n",wait_count);

#ifdef KERNEL_INFO_PRINT
			dev_info(cim->dev,"capture frame wait : %d\n",wait_count);
#endif
			if(wait_count == cim->desc->cap_wait_frame)
			{
				wait_count = 0;
				cim_disable(cim);
				cim_clear_rfifo(cim);
				cim_clear_state(cim);
				wake_up_interruptible(&cim->wait);
				return IRQ_HANDLED;
			}

			cim_disable(cim);
			cim_clear_rfifo(cim);
			bit_clr(cim,CIM_STATE,CIM_STATE_DMA_EOF);
			cim_enable(cim);
			return IRQ_HANDLED;		
		}
	}
	
	reg_write(cim,CIM_STATE, 0); // !!! DO NOT CLEAN OVER FLOW IF NOT HANDLE IT.

	return IRQ_HANDLED;
}

static long cim_shutdown(struct jz_cim *cim)
{
	if(cim->state == CS_IDLE)
		return 0;
	cim->state = CS_IDLE;
	dev_dbg(cim->dev," -----cim shut down\n");

	cim_disable_mclk(cim);
#ifndef CONFIG_BOARD_4775_MENSA
	if(!cim_get_and_check_cmd(cim))
		dev_err(cim->dev," -----cim disable mclk timeout !\n");
#endif
	cim_disable(cim);
	cim_disable_dma(cim);
	cim_enable_mclk(cim);
	cim_reset(cim);
	cim_clear_state(cim);	// clear state register
	cim_clear_rfifo(cim);	// resetting rxfifo
	
	//cim_dump_reg(cim);
	
	wake_up_interruptible(&cim->wait);
	return 0;
}

static long cim_start_preview(struct jz_cim *cim)
{
	dev_dbg(cim->dev,"__%s__\n", __func__);

	cim->state = CS_PREVIEW;
	cim->frm_id = -1;
	fresh_buf = 0;

	cim_disable(cim);
	cim_power_on(cim);
	cim_set_default(cim);

	if ( cim->desc->first_used ) {

		cim->desc->power_on(cim->desc);		// sensor power up
		cim->desc->reset(cim->desc);
		cim->desc->init(cim->desc);

		cim->desc->set_antibanding(cim->desc,cim->desc->para.antibanding);
		cim->desc->set_balance(cim->desc,cim->desc->para.balance);
		cim->desc->set_effect(cim->desc,cim->desc->para.effect);
		cim->desc->set_flash_mode(cim->desc,cim->desc->para.flash_mode);
//		cim->desc->set_focus_mode(cim->desc,cim->desc->para.focus_mode);
		cim->desc->set_fps(cim->desc,cim->desc->para.fps);
		cim->desc->set_scene_mode(cim->desc,cim->desc->para.scene_mode);
		cim->desc->first_used = false;
	}

	cim->desc->set_preivew_mode(cim->desc);

	cim_set_preview_size(cim);

	if(cim->tlb_flag == 1) {
		cim_reset_tlb(cim);
		cim_enable_tlb_error_intr(cim);
		cim_set_tlbbase(cim);
		cim_enable_tlb(cim);
	} else {
        cim_disable_tlb_error_intr(cim);
		cim_disable_tlb(cim);
    }
	cim_set_da(cim,cim->preview);
	cim_clear_state(cim);	// clear state register
	cim_enable_dma(cim);
	cim_clear_rfifo(cim);	// resetting rxfifo
#ifdef DUMP_CIM_REGESITER_BEFORE_START
        cim_dump_reg(cim);
#endif
	cim_enable(cim);

	return 0;
}

static long cim_start_capture(struct jz_cim *cim)
{
	struct jz_cim_dma_desc * dmadesc = (struct jz_cim_dma_desc *)cim->cdesc_vaddr;
	static int wait_count = 0;

	dev_dbg(cim->dev,"__%s__\n", __func__);

	cim->state = CS_CAPTURE;
	wait_count = 0;
	cim_disable(cim);
	cim_clear_state(cim);	// clear state register
	cim_set_da(cim,cim->capture);
	cim_clear_rfifo(cim);	// resetting rxfifo
	cim_set_default(cim);
//	dev_info(cim->dev,"%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
	cim->desc->set_capture_mode(cim->desc);
	cim_set_capture_size(cim);
	if(cim->tlb_flag == 1) {
		cim_reset_tlb(cim);
		cim_enable_tlb_error_intr(cim);
		cim_set_tlbbase(cim);
		cim_enable_tlb(cim);
	} else {
        cim_disable_tlb_error_intr(cim);
		cim_disable_tlb(cim);
    }
	cim_enable_dma(cim);
	cim_clear_rfifo(cim);	// resetting rxfifo
	cim_enable(cim);

	if(!interruptible_sleep_on_timeout(&cim->wait,msecs_to_jiffies(3000))) {
			cim_dump_reg(cim);
			dev_err(cim->dev,"cim ---------------capture wait timeout\n");
			cim_disable(cim);
			cim_clear_rfifo(cim);
			cim_clear_state(cim);
			return 0;
	}
	cim->state = CS_IDLE;
	return dmadesc[CDESC_NR-1].buf;
}
void convert_frame(struct jz_cim * cim,unsigned long addr)
{
	int width = 0,height = 0;
	int standard;
	unsigned int *low_addr;
	unsigned int *pBuf = NULL;

	low_addr = (unsigned int *)addr;
	switch(cim->state){
	case CS_PREVIEW:
		width = cim->psize.w;
		height = cim->psize.h;
		break;
	case CS_CAPTURE:
		width = cim->csize.w;
		height = cim->csize.h;
		break;
	default:
		dev_err(cim->dev,"cim state is neither CS_PREVIEW nor CS_CAPTURE, so return\n");
		return;
	}

	standard = cim->psize.h/2;

	int i,j;
	int var = height - standard;

	for(i = 0; i < standard - var;i++)
        {
                for(j = 0;j < width / 2 ;j++)
                {
                        *(low_addr + (height - (standard - var) + i) * width /2 + j) = *(low_addr + (var + i) * width / 2 + j);
                }
        }

	for(i = var -1; i > 0 ; i--)
	{
		for(j = 0;j <width /2;j++)
		{
			*(low_addr + (i * 2) * width / 2 + j) = *(low_addr + i * width /2 + j);
		}
	}

	for(i = 1; i < var * 2;i = i + 2)
	{
		for(j = 0;j < width / 2 ;j++)
		{
			*(low_addr +  i * width /2 + j) = (*(low_addr + ( i - 1) * width / 2 + j));// + *(low_addr + (i + 1) * width / 2 + j))/2;
		}
	}
}
static unsigned long cim_get_preview_buf(struct jz_cim *cim)
{
 unsigned long addr ;
	unsigned int cb_frame;
	unsigned int cb_len;
	unsigned int cr_frame;
	unsigned int cr_len;

    unsigned long flags;
    int fid, loop;
    struct jz_cim_dma_desc * desc;

    if(cim->state  != CS_PREVIEW){
        dev_err(cim->dev,"cim state is not CS_PREVIEW,so return\n");
        //return 0;
    }

    loop = 1;
    do {
        int got_id = 0;
        spin_lock_irqsave(&cim->lock,flags);
        fid = cim->frm_id;
        //if( fid > -1 && fid < PDESC_NR) { //4
        if( fid > -1 && fid < SWAP_BUF) { //3
            got_id = 1;
        }
        if (got_id) {
            cim->frm_id = -1;
        }
        spin_unlock_irqrestore(&cim->lock,flags);

        if (got_id) {
            break;
        }
        else {
            if(!interruptible_sleep_on_timeout(&cim->wait,msecs_to_jiffies(800))){
                dev_err(cim->dev,"wait preview queue 200ms timeout! loop:%d\n", loop);
                cim_dump_reg(cim);
            }
        }
    } while (loop--);

    //if( fid < 0 || fid >= PDESC_NR) { // 4
    if( fid < 0 || fid >= SWAP_BUF) { // 3
        dev_err(cim->dev, " ---------- cim_get_preview_buf warnning fid=%d\n", fid);
        fid = 0;
    }
    spin_lock_irqsave(&cim->lock,flags);
    desc  = (struct jz_cim_dma_desc *)cim->pdesc_vaddr;

    // copy desc[fid] data
    addr =  desc[fid].buf;

    //swap desc[fid] and desc[SWAP_BUF + fresh_buf] data
    desc[fid].buf = desc[SWAP_BUF + fresh_buf].buf; // 3 + ?

    //copy desc[fid] data
    desc[SWAP_BUF + fresh_buf].buf = addr;

    if (cim->preview_output_format != CIM_BYPASS_YUV422I) {
        cb_frame = desc[fid].cb_frame;
        cr_frame = desc[fid].cr_frame;
        cb_len = desc[fid].cb_len;
        cr_len = desc[fid].cr_len;

        //swap desc[fid] and desc[SWAP_BUF + fresh_buf] data
        desc[fid].cb_frame = desc[SWAP_BUF + fresh_buf].cb_frame;
        desc[fid].cr_frame = desc[SWAP_BUF + fresh_buf].cr_frame;
        desc[fid].cb_len = desc[SWAP_BUF + fresh_buf].cb_len;
        desc[fid].cr_len = desc[SWAP_BUF + fresh_buf].cr_len;

         //copy desc[fid] data
        desc[SWAP_BUF + fresh_buf].cb_frame = cb_frame;
        desc[SWAP_BUF + fresh_buf].cr_frame = cr_frame;
        desc[SWAP_BUF + fresh_buf].cb_len = cb_len;
        desc[SWAP_BUF + fresh_buf].cr_len = cr_len;
    }

   // dma_cache_wback((unsigned long)(&desc[fid]), sizeof(struct jz_cim_dma_desc));

    fresh_buf++; // 1
    if (fresh_buf == GET_BUF) { //2
        fresh_buf = 0;
    }
#if defined(CONFIG_TVP5150) || defined(CONFIG_ADV7180)
	convert_frame(cim,addr);
#endif
    spin_unlock_irqrestore(&cim->lock,flags);

    //if(addr == 0 || fid < 0 || fid >= PDESC_NR)
    if(addr == 0) {
        dev_info(cim->dev, " -------------  frm id %d  buf addr=%08lx, desc=%p\n", fid, addr, desc);
    }

    return addr;
}

void cim_get_sensor_info(struct jz_cim *cim, struct sensor_info *info)
{
	struct cim_sensor *desc;
	list_for_each_entry(desc, &sensor_list, list) {
		if(desc->id == info->sensor_id)
			break;
	}

	strcpy(info->name, desc->name);
	info->facing = desc->facing;
	info->orientation = desc->orientation;
	info->cap_resolution_nr = desc->cap_resolution_nr;
	info->prev_resolution_nr = desc->prev_resolution_nr;
	memcpy(&info->modes, &desc->modes, sizeof(struct mode_bit_map));
}

#if 0
static long cim_get_support_psize(struct jz_cim *cim,void __user *arg)//get preview resolutions
{
	long ret = 0;
	ret =  copy_to_user(arg,cim->desc->preview_size,
			(sizeof(struct frm_size) * cim->desc->prev_resolution_nr));
	return ret;
}

static long cim_get_support_csize(struct jz_cim *cim,void __user *arg)//get capture resolutions
{
	long ret = 0;
	ret =  copy_to_user(arg,cim->desc->capture_size,
			(sizeof(struct frm_size) * cim->desc->cap_resolution_nr));
	return ret;
}
#endif
static long cim_set_param(struct jz_cim *cim, int arg)
{
	// used app should use this ioctl like this :
	// ioctl(fd, CIMIO_SET_PARAM, CPCMD_SET_BALANCE | WHITE_BALANCE_AUTO);
	int  cmd,param_arg;
	cmd = arg & 0xffff0000;
	param_arg = arg & 0xffff;

	switch(cmd) {
		case CPCMD_SET_BALANCE:
			cim->desc->para.balance = (unsigned short)param_arg;
			if(cim->state == CS_IDLE)
				break;
			return cim->desc->set_balance(cim->desc,param_arg);
		case CPCMD_SET_EFFECT:
			cim->desc->para.effect= (unsigned short)param_arg;
			if(cim->state == CS_IDLE)
				break;
			return cim->desc->set_effect(cim->desc,param_arg);
		case CPCMD_SET_ANTIBANDING:
			cim->desc->para.antibanding= (unsigned short)param_arg;
			if(cim->state == CS_IDLE)
				break;
			return cim->desc->set_antibanding(cim->desc,param_arg);
		case CPCMD_SET_FLASH_MODE:
			cim->desc->para.flash_mode= (unsigned short)param_arg;
			if(cim->state == CS_IDLE)
				break;
			return cim->desc->set_flash_mode(cim->desc,param_arg);
		case CPCMD_SET_SCENE_MODE:
			cim->desc->para.scene_mode= (unsigned short)param_arg;
			if(cim->state == CS_IDLE)
				break;
			return cim->desc->set_scene_mode(cim->desc,param_arg);
		case CPCMD_SET_FOCUS_MODE:
			cim->desc->para.focus_mode= (unsigned short)param_arg;
			if(cim->state == CS_IDLE)
				break;
			return cim->desc->set_focus_mode(cim->desc,param_arg);
		case CPCMD_SET_FPS:
			cim->desc->para.fps= (unsigned short)param_arg;
			if(cim->state == CS_IDLE)
				break;
			return cim->desc->set_fps(cim->desc,param_arg);
		case CPCMD_SET_NIGHTSHOT_MODE:
			return cim->desc->set_nightshot(cim->desc,param_arg);
		case CPCMD_SET_LUMA_ADAPTATION:
			return cim->desc->set_luma_adaption(cim->desc,param_arg);
		case CPCMD_SET_BRIGHTNESS:
			return cim->desc->set_brightness(cim->desc,param_arg);
		case CPCMD_SET_CONTRAST:
			return cim->desc->set_contrast(cim->desc,param_arg);
	}
	return 0;
}

static void cim_free_mem(struct jz_cim *cim)
{
	printk("__%s__\n", __func__);

	if(cim->pdesc_vaddr) {
		//dma_free_coherent(cim->dev, sizeof(*cim->preview) * PDESC_NR,
		//		cim->pdesc_vaddr, (dma_addr_t)cim->preview);
        dma_free_coherent(cim->dev, sizeof(*cim->preview) * SWAP_NR,
				cim->pdesc_vaddr, (dma_addr_t)cim->preview);
    }
	if(cim->cdesc_vaddr)
		dma_free_coherent(cim->dev, sizeof(*cim->capture) * CDESC_NR,
				cim->cdesc_vaddr, (dma_addr_t)cim->capture);
}

static int cim_alloc_mem(struct jz_cim *cim)
{
	//cim->pdesc_vaddr = dma_alloc_coherent(cim->dev,
	//		sizeof(*cim->preview) * PDESC_NR,(dma_addr_t *)&cim->preview, GFP_KERNEL);

	cim->pdesc_vaddr = dma_alloc_coherent(cim->dev,
			sizeof(*cim->preview) * (SWAP_NR + RESERVED_NR),(dma_addr_t *)&cim->preview, GFP_KERNEL);

	///cim->preview = kzalloc(sizeof(struct jz_cim_dma_desc) * PDESC_NR,GFP_KERNEL);
	if (!cim->preview)
		return -ENOMEM;

	cim->cdesc_vaddr = dma_alloc_coherent(cim->dev,
			sizeof(*cim->capture) * CDESC_NR,(dma_addr_t *)&cim->capture, GFP_KERNEL);
	//cim->capture = kzalloc(sizeof(struct jz_cim_dma_desc) * CDESC_NR,GFP_KERNEL);

	if (!cim->capture)
		return -ENOMEM;

	return 0;
}
static int reserve_frame(unsigned int reserve_buffer, struct jz_cim *cim)
{
	unsigned long addr ;
	unsigned int cb_frame;
	unsigned int cb_len;
	unsigned int cr_frame;
	unsigned int cr_len;
	unsigned long flags;
	int fid;
	int i = 0;
	struct jz_cim_dma_desc * desc;
	desc  = (struct jz_cim_dma_desc *)cim->pdesc_vaddr;
	if ((desc[RESERVE_BUF].buf == 0) || (desc[RESERVE_BUF].cb_frame == 0) || (desc[RESERVE_BUF].cr_frame == 0)) {
		printk("desc[RESERVE_BUF] not maloc !\n");
		return -1;
	}
	for (fid = 0; fid < SWAP_NR; fid++) {
		if (reserve_buffer == desc[fid].buf) {
			if ((fid >= 0) && (fid <= 2))
				return -1;
#ifdef RESERVE_FRAME_DEBUG
			for (i = 0;i < (RESERVE_BUF + 1);i++) {
				printk("desc[i].buf = 0x%x\n",desc[i].buf);
			}
			printk("fid = %d, desc[RESERVE_BUF].buf = 0x%x, desc[fid].buf = 0x%x\n",
					fid, desc[RESERVE_BUF].buf, desc[fid].buf);
#endif

			spin_lock_irqsave(&cim->lock,flags);

			/*copy desc[fid] data*/
			addr =  desc[fid].buf;

			/*swap desc[fid] and desc[RESERVE_BUF] data*/
			desc[fid].buf = desc[RESERVE_BUF].buf;

			/*copy desc[fid] data*/
			desc[RESERVE_BUF].buf = addr;

			if (cim->preview_output_format != CIM_BYPASS_YUV422I) {
				cb_frame = desc[fid].cb_frame;
				cr_frame = desc[fid].cr_frame;
				cb_len = desc[fid].cb_len;
				cr_len = desc[fid].cr_len;

				/*swap desc[fid] and desc[RESERVE_BUF] data*/
				desc[fid].cb_frame = desc[RESERVE_BUF].cb_frame;
				desc[fid].cr_frame = desc[RESERVE_BUF].cr_frame;
				desc[fid].cb_len = desc[RESERVE_BUF].cb_len;
				desc[fid].cr_len = desc[RESERVE_BUF].cr_len;

				/*copy desc[fid] data*/
				desc[RESERVE_BUF].cb_frame = cb_frame;
				desc[RESERVE_BUF].cr_frame = cr_frame;
				desc[RESERVE_BUF].cb_len = cb_len;
				desc[RESERVE_BUF].cr_len = cr_len;
			}
			/*dma_cache_wback((unsigned long)(&desc[fid]), sizeof(struct jz_cim_dma_desc));*/
#if defined(CONFIG_TVP5150) || defined(CONFIG_ADV7180)
			convert_frame(cim,addr);
#endif
			spin_unlock_irqrestore(&cim->lock,flags);
#ifdef RESERVE_FRAME_DEBUG
			printk("fid = %d, desc[RESERVE_BUF].buf = 0x%x, desc[fid].buf = 0x%x\n",
					fid, desc[RESERVE_BUF].buf, desc[fid].buf);
#endif

		}
	}

	return 1;
}

static int cim_prepare_reserve_pdma(struct jz_cim *cim, unsigned long addr){
	unsigned int preview_imgsize = cim->psize.w * cim->psize.h;
	struct jz_cim_dma_desc * desc = (struct jz_cim_dma_desc *) cim->pdesc_vaddr;
	CameraYUVMeta *yuv_meta_data = (CameraYUVMeta *) addr;

	if(cim->state != CS_IDLE)
		return -EBUSY;

	desc[RESERVE_BUF].next = virt_to_phys(NULL);
	desc[RESERVE_BUF].id = RESERVE_BUF;
	if (cim->tlb_flag == 1) {
		desc[RESERVE_BUF].buf = yuv_meta_data->yAddr;
	} else {
		desc[RESERVE_BUF].buf = yuv_meta_data->yPhy;
	}
	if(cim->preview_output_format != CIM_BYPASS_YUV422I)
		desc[RESERVE_BUF].cmd = (preview_imgsize >> 2) | CIM_CMD_EOFINT; //YUV420Tile don't need auto recover
	else {
#ifdef CONFIG_OV5640_RAW_BAYER
		desc[RESERVE_BUF].cmd = (preview_imgsize >> 2) | CIM_CMD_EOFINT | CIM_CMD_OFRCV;
#else
		desc[RESERVE_BUF].cmd = ((preview_imgsize << 1) >> 2) | CIM_CMD_EOFINT | CIM_CMD_OFRCV;
#endif
	}

#ifdef KERNEL_INFO_PRINT
	dev_info(cim->dev,"cim set Y buffer[%d] phys is: %x\n", RESERVE_BUF, desc[RESERVE_BUF].buf);
#endif
	if(cim->preview_output_format != CIM_BYPASS_YUV422I) {
		if(cim->preview_output_format == CIM_CSC_YUV420P) {
			if (cim->tlb_flag == 1) {
				desc[RESERVE_BUF].cb_frame = yuv_meta_data->uAddr;
				desc[RESERVE_BUF].cr_frame = yuv_meta_data->vAddr;
			} else {
				desc[RESERVE_BUF].cb_frame = yuv_meta_data->uPhy;
				desc[RESERVE_BUF].cr_frame = yuv_meta_data->vPhy;
			}
		} else if(cim->preview_output_format == CIM_CSC_YUV420B) {
			if (cim->tlb_flag == 1) {
				desc[RESERVE_BUF].cb_frame = yuv_meta_data->uAddr;
			} else {
				desc[RESERVE_BUF].cb_frame = yuv_meta_data->uPhy;
			}
			desc[RESERVE_BUF].cr_frame = desc[RESERVE_BUF].cb_frame + 64;
		}
		desc[RESERVE_BUF].cb_len = (cim->psize.w >> 1) * (cim->psize.h >> 1) >> 2;
		desc[RESERVE_BUF].cr_len = (cim->psize.w >> 1) * (cim->psize.h >> 1) >> 2;
#ifdef KERNEL_INFO_PRINT
		dev_info(cim->dev,"cim set C buffer[%d] phys is: %x\n", RESERVE_BUF, desc[RESERVE_BUF].cb_frame);
#endif
	}
	return 1;
}

static int cim_prepare_pdma(struct jz_cim *cim, unsigned long addr)
{
	int i;
	unsigned int preview_imgsize = cim->psize.w * cim->psize.h;
	struct jz_cim_dma_desc * desc = (struct jz_cim_dma_desc *) cim->pdesc_vaddr;
	CameraYUVMeta *yuv_meta_data = (CameraYUVMeta *) addr;

	if(cim->state != CS_IDLE)
		return -EBUSY;
    for(i=0;i<SWAP_NR;i++) {
		desc[i].next = (dma_addr_t)(&cim->preview[i+1]);
		desc[i].id      = i;
	 if (cim->tlb_flag == 1) {
	     desc[i].buf = yuv_meta_data[i].yAddr;
	 } else {
	     desc[i].buf = yuv_meta_data[i].yPhy;
	 }

		if(cim->preview_output_format != CIM_BYPASS_YUV422I)
			desc[i].cmd = (preview_imgsize >> 2) | CIM_CMD_EOFINT; //YUV420Tile don't need auto recover
		else {
#ifdef CONFIG_OV5640_RAW_BAYER
			desc[i].cmd = (preview_imgsize >> 2) | CIM_CMD_EOFINT | CIM_CMD_OFRCV;
#else
			desc[i].cmd = ((preview_imgsize << 1) >> 2) | CIM_CMD_EOFINT | CIM_CMD_OFRCV;
#endif
		}

#ifdef KERNEL_INFO_PRINT
		dev_info(cim->dev,"cim set Y buffer[%d] phys is: %x\n", i, desc[i].buf);
#endif		
		if(cim->preview_output_format != CIM_BYPASS_YUV422I) {
			if(cim->preview_output_format == CIM_CSC_YUV420P) {
                               if (cim->tlb_flag == 1) {
				    desc[i].cb_frame = yuv_meta_data[i].uAddr;
			            desc[i].cr_frame = yuv_meta_data[i].vAddr;
				} else {
				    desc[i].cb_frame = yuv_meta_data[i].uPhy;
				    desc[i].cr_frame = yuv_meta_data[i].vPhy;
				}
			} else if(cim->preview_output_format == CIM_CSC_YUV420B) {
				if (cim->tlb_flag == 1) {
				    desc[i].cb_frame = yuv_meta_data[i].uAddr;
				} else {
				    desc[i].cb_frame = yuv_meta_data[i].uPhy;
				}
				desc[i].cr_frame = desc[i].cb_frame + 64;
			}

			desc[i].cb_len = (cim->psize.w >> 1) * (cim->psize.h >> 1) >> 2;
			desc[i].cr_len = (cim->psize.w >> 1) * (cim->psize.h >> 1) >> 2;
#ifdef KERNEL_INFO_PRINT
			dev_info(cim->dev,"cim set C buffer[%d] phys is: %x\n", i, desc[i].cb_frame);
#endif
		}
	}

	//desc[PDESC_NR-1].next = (dma_addr_t)cim->preview; // 4 - 1
    desc[SWAP_BUF-1].next = (dma_addr_t)(&cim->preview[0]); // 3 - 1

    for (i = 0; i < GET_BUF; i++) { // 2
	 desc[SWAP_BUF + i].next = virt_to_phys(NULL);
    }

	//dma_cache_wback((unsigned long)(&cim->preview[0]), sizeof(struct jz_cim_dma_desc) *PDESC_NR);
	return 0;
}

static int cim_prepare_cdma(struct jz_cim *cim, unsigned long addr)
{
	int i;
	unsigned int capture_imgsize = cim->csize.w * cim->csize.h;
	struct jz_cim_dma_desc * desc = (struct jz_cim_dma_desc *) cim->cdesc_vaddr;
	CameraYUVMeta *yuv_meta_data = (CameraYUVMeta *) addr;

	if(!strcmp(cim->desc->name, "ov2650") && capture_imgsize > 1600 * 1200){ 
		cim->csize.w = 1600;
		cim->csize.h = 1200;
		capture_imgsize = cim->csize.w * cim->csize.h;
	}

	for(i = 0; i < CDESC_NR; i++) {
		desc[i].next = (dma_addr_t)(&cim->capture[i+1]);
		desc[i].id 	= i;
        if (cim->tlb_flag == 1) {
            desc[i].buf = yuv_meta_data[i].yAddr;
        } else {
            desc[i].buf = yuv_meta_data[i].yPhy;
        }
		if(cim->capture_output_format != CIM_BYPASS_YUV422I)
			desc[i].cmd = (capture_imgsize >> 2) | CIM_CMD_EOFINT;//YUV420Tile don't need auto recover
		else
			desc[i].cmd = ((capture_imgsize << 1) >> 2) | CIM_CMD_EOFINT | CIM_CMD_OFRCV;
	
#ifdef KERNEL_INFO_PRINT
		dev_info(cim->dev,"cim set Y buffer[%d] phys is: %x\n",i,desc[i].buf);
#endif		
		if(cim->capture_output_format != CIM_BYPASS_YUV422I) {
			if(cim->capture_output_format == CIM_CSC_YUV420P) {
                if (cim->tlb_flag == 1) {
                    desc[i].cb_frame = yuv_meta_data[i].uAddr;
                    desc[i].cr_frame = yuv_meta_data[i].vAddr;
                } else {
                    desc[i].cb_frame = yuv_meta_data[i].uPhy;
                    desc[i].cr_frame = yuv_meta_data[i].vPhy;
                }
			} else if(cim->capture_output_format == CIM_CSC_YUV420B) {
                if (cim->tlb_flag == 1) {
                    desc[i].cb_frame = yuv_meta_data[i].uAddr;
                } else {
                    desc[i].cb_frame = yuv_meta_data[i].uPhy;
                }
				desc[i].cr_frame = desc[i].cb_frame + 64;	
			}
			
			desc[i].cb_len = (cim->psize.w >> 1) * (cim->psize.h >> 1) >> 2;
			desc[i].cr_len = (cim->psize.w >> 1) * (cim->psize.h >> 1) >> 2;
#ifdef KERNEL_INFO_PRINT
			dev_info(cim->dev,"cim set C buffer[%d] phys is: %x\n",i,desc[i].cb_frame);
#endif
		}
	}
	
	desc[CDESC_NR-1].next = (dma_addr_t)cim->capture;
	if(cim->capture_output_format != CIM_BYPASS_YUV422I)
		desc[CDESC_NR-1].cmd = (capture_imgsize >> 2) | CIM_CMD_EOFINT;//YUV420Tile don't need auto recover
	else
		desc[CDESC_NR-1].cmd = ((capture_imgsize << 1) >> 2) | CIM_CMD_OFRCV | CIM_CMD_EOFINT;
		
	//dma_cache_wback((unsigned long)(&cim->capture[0]), sizeof(struct jz_cim_dma_desc) *CDESC_NR);
	return 0;
}

static int cim_set_output_format(struct jz_cim *cim, unsigned int cmd, unsigned int format)
{
	unsigned char *format_desc;
	if((format == HAL_PIXEL_FORMAT_YV12) || (format == HAL_PIXEL_FORMAT_JZ_YUV_420_P)) {
		format_desc = "yuv420p";
		if(cmd == CIMIO_SET_PREVIEW_FMT) {
			cim->preview_output_format = CIM_CSC_YUV420P;
		} else if(cmd == CIMIO_SET_CAPTURE_FMT) {
			cim->capture_output_format = CIM_CSC_YUV420P;
		}
		
	} else if(format == HAL_PIXEL_FORMAT_JZ_YUV_420_B) {
		format_desc = "jz_yuv420b";
		if(cmd == CIMIO_SET_PREVIEW_FMT) {
			cim->preview_output_format = CIM_CSC_YUV420B;
		} else if(cmd == CIMIO_SET_CAPTURE_FMT) {
			cim->capture_output_format = CIM_CSC_YUV420B;
		}
		
	} else if (format == HAL_PIXEL_FORMAT_YCbCr_422_I) {
		format_desc = "yuv422i";
		if(cmd == CIMIO_SET_PREVIEW_FMT) {
			cim->preview_output_format = CIM_BYPASS_YUV422I;
		} else if(cmd == CIMIO_SET_CAPTURE_FMT) {
			cim->capture_output_format = CIM_BYPASS_YUV422I;
		}
	} else {
		dev_err(cim->dev,"unknow format, code: 0x%x\n", format);
		return -1;
	}

#ifdef KERNEL_INFO_PRINT
	if(cmd == CIMIO_SET_PREVIEW_FMT) {
		dev_info(cim->dev,"set preview format to %s\n", format_desc);
	} else if(cmd == CIMIO_SET_CAPTURE_FMT) {
		dev_info(cim->dev,"set capture format to %s\n", format_desc);
	}
#endif	
	return 0;
}

static int cim_open(struct inode *inode, struct file *file)
{
	//struct miscdevice *dev = file->private_data;
	//struct jz_cim *cim = container_of(dev, struct jz_cim, misc_dev);
	return 0;
}

static int cim_close(struct inode *inode, struct file *file)
{
	struct miscdevice *dev = file->private_data;
	struct jz_cim *cim = container_of(dev, struct jz_cim, misc_dev);
	cim_shutdown(cim);
	cim_power_off(cim);
	/*if you want Switching betwen two sensor ,
	 * must close one sensor, then open another sensor */
	if(cim->desc->shutdown)
		cim->desc->shutdown(cim->desc);
	cim->state = CS_IDLE;
	cim->tlb_flag = 0;
	cim->tlb_base = 0;
	cim->psize.h = 0;
	cim->psize.w = 0;
	cim->csize.w = 0;
	cim->csize.h = 0;

	printk("right_num = %d, err_num = %d\n", right_num, err_num);

	return 0;
}

static long cim_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *dev = file->private_data;
	struct jz_cim *cim = container_of(dev, struct jz_cim, misc_dev);
	void __user *argp = (void __user *)arg;
	long ret = 0;
	unsigned long addr = 0;
	unsigned long j1, j2;
	static long long idx = 0;

	switch (cmd) {
		case CIMIO_SHUTDOWN:
			return cim_shutdown(cim);
		case CIMIO_START_PREVIEW:
			return cim_start_preview(cim);
		case CIMIO_START_CAPTURE:
			return cim_start_capture(cim);
		case CIMIO_GET_FRAME:
			addr = cim_get_preview_buf(cim);
			return addr;
			break;
		case CIMIO_GET_SENSORINFO:
			{
				struct sensor_info info;
				copy_from_user(&info, (void __user *)arg, sizeof(struct sensor_info));
				cim_get_sensor_info(cim, &info);
				return copy_to_user(argp,&info,sizeof(struct sensor_info));
			}
		case CIMIO_GET_VAR:
			break;
		case CIMIO_GET_SUPPORT_PSIZE:
			return copy_to_user(argp,cim->desc->preview_size,
					(sizeof(struct frm_size) * cim->desc->prev_resolution_nr));
		case CIMIO_GET_SUPPORT_CSIZE:
			return copy_to_user(argp,cim->desc->capture_size,
					(sizeof(struct frm_size) * cim->desc->cap_resolution_nr));	
		case CIMIO_SET_PARAM:
			return cim_set_param(cim,arg);
		case CIMIO_SET_PREVIEW_MEM:
			return cim_prepare_pdma(cim,arg);	
		case CIMIO_SET_CAPTURE_MEM:
			return cim_prepare_cdma(cim,arg);
							
		case CIMIO_SELECT_SENSOR:
			return cim_select_sensor(cim,arg);
		case CIMIO_SET_PREVIEW_SIZE:
			return copy_from_user(&cim->psize, (void __user *)arg, sizeof(struct frm_size));
		case CIMIO_SET_CAPTURE_SIZE:
			return copy_from_user(&cim->csize, (void __user *)arg, sizeof(struct frm_size));
		case CIMIO_DO_FOCUS:
			break;
		case CIMIO_AF_INIT:
			break;
		case CIMIO_SET_VIDEO_MODE:
			ret = cim->desc->set_video_mode(cim->desc);
			break;
    case CIMIO_SET_TLB_BASE: {
            if (arg != 0) {
			    cim->tlb_flag = 1;
			    cim->tlb_base = arg;
            } else {
                cim->tlb_flag = 0;
            }
            }
			break;
		case CIMIO_GET_SENSOR_COUNT:
			return cim->sensor_count;
		case CIMIO_SET_PREVIEW_FMT:
		case CIMIO_SET_CAPTURE_FMT:
			return cim_set_output_format(cim, cmd, arg);
#if(defined(CONFIG_GC0307) && defined(CONFIG_SOC_4775))
		case CIMIO_SET_OFFSET:
			return copy_from_user(&cim->offset, (void __user *)arg, sizeof(struct frm_size));
			break;
#endif
		case CIMIO_SET_RESERVE_MEM:
			return cim_prepare_reserve_pdma(cim,arg);
		case CIMIO_RESERVE_FRAME:
			if(reserve_frame(arg, cim) == -1){
				printk("reserve_frame failed!,fid >= 0 && fid <= 2\n");
				return -1;
			}
			break;

	}
	return ret;
}

static struct file_operations cim_fops = {
	.open 		= cim_open,
	.release 	= cim_close,
	.unlocked_ioctl = cim_ioctl,
};

void cim_dummy_power(void) {}


static ssize_t dump_cim_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_cim *cim  = dev_get_drvdata(dev);
    cim_dump_reg(cim);
    return 0;
}

static struct device_attribute cim_sysfs_attrs[] = {
    __ATTR(dump_cim_reg, S_IRUGO|S_IWUSR, dump_cim_reg, NULL),
};

static int cim_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *r = NULL;
	struct jz_cim_platform_data *pdata;
	struct jz_cim *cim = kzalloc(sizeof(struct jz_cim), GFP_KERNEL);
	if(!cim) {
		dev_err(&pdev->dev,"no memory!\n");
		ret = -ENOMEM;
		goto no_mem;
	}

	cim->dev = &pdev->dev;
	cim->num = pdev->id;

	pdata = pdev->dev.platform_data;

	cim->power_on = cim_dummy_power;
	cim->power_off = cim_dummy_power;

	if(pdata && pdata->power_on)
		cim->power_on = pdata->power_on;

	if(pdata && pdata->power_off)
		cim->power_off = pdata->power_off;

	if(pdev->id == 0)
		cim->clk = clk_get(&pdev->dev,"cim0");
	else if(pdev->id == 1)
		cim->clk = clk_get(&pdev->dev,"cim1");
	else
		cim->clk = clk_get(&pdev->dev,"cim");
	
	if(IS_ERR(cim->clk)) {
		ret = -ENODEV;
		goto no_desc;
	}

#ifdef CONFIG_CAMERA_SENSOR_NO_MCLK_CTL
	cim->mclk = NULL;
#else
	if(pdev->id == 0)
		cim->mclk = clk_get(&pdev->dev,"cgu_cimmclk0");
	else if(pdev->id == 1)
		cim->mclk = clk_get(&pdev->dev,"cgu_cimmclk1");
	else
		cim->mclk = clk_get(&pdev->dev,"cgu_cimmclk");

	if(IS_ERR(cim->mclk)) {
		ret = -ENODEV;
		goto io_failed;
	}
#endif

#ifdef CONFIG_CAMERA_SENSOR_NO_POWER_CTL
	cim->power = NULL;
#else
	cim->power = regulator_get(&pdev->dev, "vcim");
	if(IS_ERR(cim->power)){
		dev_err(&pdev->dev,"get regulator fail!\n");
		ret = -ENODEV;
		goto io_failed;
	}
#endif

 	cim_scan_sensor(cim); 
	
	if(!cim->desc) {
		dev_err(&pdev->dev,"no sensor!\n");
		ret = -ENOMEM;
		goto io_failed1;
	}	
	
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	r = request_mem_region(r->start, resource_size(r), pdev->name);
	if (!r) {
		dev_err(&pdev->dev, "request memory region failed\n");
		ret = -EBUSY;
	}

	cim->iomem = ioremap(r->start,resource_size(r));
	if (!cim->iomem) {
		ret = -ENODEV;
		goto io_failed2;
	}

	printk("==>%s L%d: iomem=%p\n", __func__, __LINE__, cim->iomem);

	if(cim_alloc_mem(cim)) {
		dev_err(&pdev->dev,"request mem failed!\n");
		goto mem_failed;
	}

	cim->irq = platform_get_irq(pdev, 0);
	ret = request_irq(cim->irq, cim_irq_handler, IRQF_DISABLED,dev_name(&pdev->dev), cim);
	if(ret) {
		dev_err(&pdev->dev,"request irq failed!\n");
		goto irq_failed;
	}

	init_waitqueue_head(&cim->wait);

	cim->misc_dev.minor = MISC_DYNAMIC_MINOR;

	if (pdev->id == 0)
		cim->misc_dev.name = "cim0";
	else if (pdev->id == 1)
		cim->misc_dev.name = "cim1";
	else 
		cim->misc_dev.name = "cim";

	cim->misc_dev.fops = &cim_fops;
	spin_lock_init(&cim->lock);

	ret = misc_register(&cim->misc_dev);
	if(ret) {
		dev_err(&pdev->dev,"request misc device failed!\n");
		goto misc_failed;
	}

	platform_set_drvdata(pdev,cim);
	
	dev_info(&pdev->dev,"ingenic camera interface module registered.\n");

        {
            int i;
            for (i = 0; i < ARRAY_SIZE(cim_sysfs_attrs); i++) {
		ret = device_create_file(&pdev->dev, &cim_sysfs_attrs[i]);
		if (ret)
                    break;
            }
        }
	return 0;

misc_failed:	
	free_irq(cim->irq,cim);
irq_failed:
	cim_free_mem(cim);
mem_failed:
	iounmap(cim->iomem);
io_failed2:
	release_mem_region(r->start, resource_size(r));	
io_failed1:
	clk_put(cim->mclk);
io_failed:
	clk_put(cim->clk);
no_desc:
	kfree(cim);
no_mem:
	return ret;
}

static int __devexit cim_remove(struct platform_device *pdev)
{
	struct jz_cim *cim = platform_get_drvdata(pdev);
	iounmap(cim->iomem);
	clk_put(cim->mclk);
	clk_put(cim->clk);
	free_irq(cim->irq,cim);
	misc_deregister(&cim->misc_dev);
	kfree(cim);
	return 0;
}

static struct platform_driver cim_driver = {
	.driver.name	= "jz-cim",
	.driver.owner	= THIS_MODULE,
	.probe		= cim_probe,
	.remove		= cim_remove,
};

static int __init cim_init(void)
{
	return platform_driver_register(&cim_driver);
}

static void __exit cim_exit(void)
{
	platform_driver_unregister(&cim_driver);
}

late_initcall(cim_init);
module_exit(cim_exit);

MODULE_AUTHOR("sonil<ztyan@ingenic.cn>");
MODULE_DESCRIPTION("Ingenic Camera interface module driver");
MODULE_LICENSE("GPL");


