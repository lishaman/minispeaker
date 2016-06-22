/*
 * V4L2 Driver for JZ4780 camera host
 *
 * Copyright (C) 2013, ptkang <ptkang@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/gpio.h>

#include <media/v4l2-common.h>
#include <media/v4l2-dev.h>
#include <media/videobuf-dma-sg.h>
#include <media/soc_camera.h>
#include <media/soc_mediabus.h>

#include <linux/videodev2.h>

#include <mach/jz4780_camera.h>

#define JZ4780_CAM_VERSION_CODE KERNEL_VERSION(0, 0, 1)
#define JZ4780_CAM_DRV_NAME		"jz4780-cim"
#define SOC_DMA_BUF_NUM			3
#define MAX_VIDEO_MEM			16
//#define DEBUG				1
//#define DUMP

#define CIM_BUS_FLAGS	(SOCAM_MASTER | SOCAM_VSYNC_ACTIVE_HIGH | \
			SOCAM_VSYNC_ACTIVE_LOW | SOCAM_HSYNC_ACTIVE_HIGH | \
			SOCAM_HSYNC_ACTIVE_LOW | SOCAM_PCLK_SAMPLE_RISING | \
			SOCAM_PCLK_SAMPLE_FALLING | SOCAM_DATAWIDTH_8)

/* the usage cim registers */
#define	CIM_CFG			(0x00)
#define	CIM_CTRL		(0x04)
#define	CIM_STATE		(0x08)
#define	CIM_IID			(0x0c)
#define	CIM_DA			(0x20)
#define	CIM_FA			(0x24)
#define	CIM_FID			(0x28)
#define	CIM_CMD			(0x2c)
#define	CIM_SIZE		(0x30)
#define	CIM_OFFSET		(0x34)
#define CIM_YFA			(0x38)
#define CIM_YCMD		(0x3c)
#define CIM_CBFA		(0x40)
#define CIM_CBCMD		(0x44)
#define CIM_CRFA		(0x48)
#define CIM_CRCMD		(0x4c)
#define CIM_CTRL2		(0x50)
#define CIM_FS			(0x54)
#define CIM_IMR			(0x58)
#define CIM_TC			(0x5c)
#define CIM_TINX		(0x60)
#define CIM_TCNT		(0x64)

/* the registers context */
/*CIM Configuration Register (CIMCFG)*/
#define CIM_CFG_SEP				(1<<20)
#define	CIM_CFG_ORDER			18

#define	CIM_CFG_ORDER_YUYV		(0 << 	CIM_CFG_ORDER)
#define	CIM_CFG_ORDER_YVYU		(1 << 	CIM_CFG_ORDER)
#define	CIM_CFG_ORDER_UYVY		(2 << 	CIM_CFG_ORDER)
#define	CIM_CFG_ORDER_VYUY		(3 << 	CIM_CFG_ORDER)

#define	CIM_CFG_DF_BIT			16
#define	CIM_CFG_DF_MASK		 	(0x3 << CIM_CFG_DF_BIT)
#define CIM_CFG_DF_YUV444	  	(0x1 << CIM_CFG_DF_BIT) 	/* YCbCr444 */
#define CIM_CFG_DF_YUV422	  	(0x2 << CIM_CFG_DF_BIT)	/* YCbCr422 */
#define CIM_CFG_DF_ITU656	  	(0x3 << CIM_CFG_DF_BIT)	/* ITU656 YCbCr422 */
#define	CIM_CFG_VSP				(1 << 14) /* VSYNC Polarity:0-rising edge active,1-falling edge active */
#define	CIM_CFG_HSP				(1 << 13) /* HSYNC Polarity:0-rising edge active,1-falling edge active */
#define	CIM_CFG_PCP				(1 << 12) /* PCLK working edge: 0-rising, 1-falling */
#define	CIM_CFG_DMA_BURST_TYPE_BIT	10
#define	CIM_CFG_DMA_BURST_TYPE_MASK	(0x3 << CIM_CFG_DMA_BURST_TYPE_BIT)
#define	CIM_CFG_DMA_BURST_INCR4		(0 << CIM_CFG_DMA_BURST_TYPE_BIT)
#define	CIM_CFG_DMA_BURST_INCR8		(1 << CIM_CFG_DMA_BURST_TYPE_BIT)	/* Suggested */
#define	CIM_CFG_DMA_BURST_INCR16	(2 << CIM_CFG_DMA_BURST_TYPE_BIT)	/* Suggested High speed AHB*/
#define	CIM_CFG_DMA_BURST_INCR32	(3 << CIM_CFG_DMA_BURST_TYPE_BIT)	/* Suggested High speed AHB*/
#define	CIM_CFG_PACK_BIT			4
#define	CIM_CFG_PACK_MASK			(0x7 << CIM_CFG_PACK_BIT)
#define CIM_CFG_PACK_VY1UY0	  		(0 << CIM_CFG_PACK_BIT)
#define CIM_CFG_PACK_Y0VY1U	  		(1 << CIM_CFG_PACK_BIT)
#define CIM_CFG_PACK_UY0VY1	  		(2 << CIM_CFG_PACK_BIT)
#define CIM_CFG_PACK_Y1UY0V	  		(3 << CIM_CFG_PACK_BIT)
#define CIM_CFG_PACK_Y0UY1V	  		(4 << CIM_CFG_PACK_BIT)
#define CIM_CFG_PACK_UY1VY0	  		(5 << CIM_CFG_PACK_BIT)
#define CIM_CFG_PACK_Y1VY0U	  		(6 << CIM_CFG_PACK_BIT)
#define CIM_CFG_PACK_VY0UY1	  		(7 << CIM_CFG_PACK_BIT)
#define	CIM_CFG_DSM_BIT				0
#define	CIM_CFG_DSM_MASK			(0x3 << CIM_CFG_DSM_BIT)
#define CIM_CFG_DSM_CPM	  			(0 << CIM_CFG_DSM_BIT) /* CCIR656 Progressive Mode */
#define CIM_CFG_DSM_CIM	  			(1 << CIM_CFG_DSM_BIT) /* CCIR656 Interlace Mode */
#define CIM_CFG_DSM_GCM	  			(2 << CIM_CFG_DSM_BIT) /* Gated Clock Mode */

/*CIM Control Register (CIMCR)*/
#define	CIM_CTRL_FRC_BIT		16
#define	CIM_CTRL_FRC_MASK		(0xf << CIM_CTRL_FRC_BIT)
#define CIM_CTRL_FRC_1	        (0x0 << CIM_CTRL_FRC_BIT) /* Sample every frame */
#define CIM_CTRL_FRC_10         (10 << CIM_CTRL_FRC_BIT)

#define CIM_CTRL_MBEN           (1 << 6)    /* 16x16 yuv420  macro blocks */
#define	CIM_CTRL_DMA_SYNC		(1 << 7)	/*when change DA, do frame sync */

#define CIM_CTRL_CIM_RST	(1 << 3)
#define	CIM_CTRL_DMA_EN		(1 << 2) /* Enable DMA */
#define	CIM_CTRL_RXF_RST	(1 << 1) /* RxFIFO reset */
#define	CIM_CTRL_ENA		(1 << 0) /* Enable CIM */

/* CIM State Register  (CIM_STATE) */
#define	CIM_STATE_CR_RXF_OF	(1 << 21) /* Y RXFIFO over flow irq */
#define	CIM_STATE_CB_RXF_OF	(1 << 19) /* Y RXFIFO over flow irq */
#define	CIM_STATE_Y_RXF_OF	(1 << 17) /* Y RXFIFO over flow irq */

#define	CIM_STATE_DMA_EEOF	(1 << 11) /* DMA Line EEOf irq */
#define	CIM_STATE_DMA_STOP	(1 << 10) /* DMA stop irq */
#define	CIM_STATE_DMA_SOF	(1 << 8) /* DMA start irq */
#define	CIM_STATE_DMA_EOF	(1 << 9) /* DMA end irq */
#define	CIM_STATE_TLB_ERR	(1 << 4) /* TLB error */
#define	CIM_STATE_SIZE_ERR	(1 << 4) /* Frame size check error */
#define	CIM_STATE_RXF_OF	(1 << 2) /* RXFIFO over flow irq */
#define	CIM_STATE_RXF_EMPTY	(1 << 1) /* RXFIFO empty irq */
#define	CIM_STATE_VDD		(1 << 0) /* CIM disabled irq */

#define CIM_STATE_OK_RXF_OF	(CIM_STATE_RXF_OF | CIM_STATE_DMA_STOP | CIM_STATE_DMA_EOF)

/* CIM DMA Command Register (CIM_CMD) */
#define	CIM_CMD_SOFINT		(1 << 31) /* enable DMA start irq */
#define	CIM_CMD_EOFINT		(1 << 30) /* enable DMA end irq */
#define	CIM_CMD_EEOFINT		(1 << 29) /* enable DMA EEOF irq */
#define	CIM_CMD_STOP		(1 << 28) /* enable DMA stop irq */
#define CIM_CMD_OFRCV       (1 << 27)

/* cim control2 */
#define CIM_CTRL2_FRAGHE	(1 << 25)	/* horizontal size remainder ingore */
#define CIM_CTRL2_FRAGVE	(1 << 24)	/* vertical size remainder ingore */
#define CIM_CTRL2_FSC		(1 << 23)	/* enable frame size check */
#define CIM_CTRL2_ARIF		(1 << 22)	/* enable auto-recovery for incomplete frame */
#define CIM_CTRL2_HRS_BIT	20		/* horizontal ratio for down scale */
#define CIM_CTRL2_HRS_MASK	(0x3 << CIM_CTRL2_HRS_BIT)
#define CIM_CTRL2_HRS_0		(0x0 << CIM_CTRL2_HRS_BIT)	/* no scale */
#define CIM_CTRL2_HRS_1		(0x1 << CIM_CTRL2_HRS_BIT)	/* 1/2 down scale */
#define CIM_CTRL2_HRS_2		(0x2 << CIM_CTRL2_HRS_BIT)	/* 1/4 down scale */
#define CIM_CTRL2_HRS_3		(0x3 << CIM_CTRL2_HRS_BIT)	/* 1/8 down scale */
#define CIM_CTRL2_VRS_BIT	18		/* vertical ratio for down scale */
#define CIM_CTRL2_VRS_MASK	(0x3 << CIM_CTRL2_VRS_BIT)
#define CIM_CTRL2_VRS_0		(0x0 << CIM_CTRL2_VRS_BIT)	/* no scale */
#define CIM_CTRL2_VRS_1		(0x1 << CIM_CTRL2_VRS_BIT)	/* 1/2 down scale */
#define CIM_CTRL2_VRS_2		(0x2 << CIM_CTRL2_VRS_BIT)	/* 1/4 down scale */
#define CIM_CTRL2_VRS_3		(0x3 << CIM_CTRL2_VRS_BIT)	/* 1/8 down scale */
#define CIM_CTRL2_CSC_BIT		16		/* CSC Mode Select */
#define CIM_CTRL2_CSC_MASK		(0x3 << CIM_CTRL2_CSC_BIT)
#define CIM_CTRL2_CSC_BYPASS	(0x0 << CIM_CTRL2_CSC_BIT)	/* Bypass mode */
#define CIM_CTRL2_CSC_YUV422	(0x2 << CIM_CTRL2_CSC_BIT)	/* CSC to YCbCr422 */
#define CIM_CTRL2_CSC_YUV420	(0x3 << CIM_CTRL2_CSC_BIT)	/* CSC to YCbCr420 */
#define CIM_CTRL2_OPG_BIT		4		/* option priority configuration */
#define CIM_CTRL2_OPG_MASK		(0x3 << CIM_CTRL2_OPG_BIT)
#define CIM_CTRL2_OPE			(1 << 2)	/* optional priority mode enable */
#define CIM_CTRL2_EME			(1 << 1)	/* emergency priority enable */
#define CIM_CTRL2_APM			(1 << 0)	/* auto priority mode enable*/

/* CIM Frame Size Register (CIM_FS) */
#define CIM_FS_FVS_BIT		16	/* vertical size of the frame */
#define CIM_FS_FVS_MASK		(0x1fff << CIM_FS_FVS_BIT)
#define CIM_FS_BPP_BIT		14	/* bytes per pixel */
#define CIM_FS_BPP_MASK		(0x3 << CIM_FS_BPP_BIT)
#define CIM_FS_FHS_BIT		0	/* horizontal size of the frame */
#define CIM_FS_FHS_MASK		(0x1fff << CIM_FS_FHS_BIT)

/*CIM Interrupt Mask Register (CIMIMR)*/
#define CIM_IMR_STPM		(1<<10)
#define CIM_IMR_EOFM		(1<<9)
#define CIM_IMR_SOFM		(1<<8)
#define CIM_IMR_TLBEM		(1<<4)
#define CIM_IMR_FSEM		(1<<3)
#define CIM_IMR_RFIFO_OFM	(1<<2)

/*
 * Because scatter/gatther dma has size and number uncertain.
 * And the yuv separator transfer need cooperation to format.
 * Also the discriptor has limit to discribe this.
 * We must only support package format to adjust s/g dma.
 */
struct jz4780_camera_dma_desc {
	dma_addr_t	next;
	unsigned int	id;
	unsigned int	buf;
	unsigned int	cmd;
} __attribute__ ((aligned (16)));

/* descriptor needed for the jz4780 cim DMA engine */
struct jz4780_camera_dma {
	dma_addr_t			sg_dma;
	struct jz4780_camera_dma_desc	*sg_cpu;
	size_t				sg_size;
	int				sglen;
};

/* buffer for one video frame */
struct jz4780_camera_buffer {
	/* common v4l buffer stuff -- must be first */
	struct videobuf_buffer		vb;
	enum v4l2_mbus_pixelcode	code;
	/* jz4780 cim controller have only one chanel */
	struct jz4780_camera_dma	*dma;
	int				inwork;
};

struct jz4780_camera_dev {
	struct soc_camera_host		soc_host;
	struct soc_camera_device	*icd[MAX_SOC_CAM_NUM];

	struct jz4780_camera_pdata	*pdata;
	struct resource			*res;

	void __iomem			*base;
	int				irq;

	struct clk			*clk;
	struct clk			*mclk;
	unsigned long			mclk_freq;

	struct list_head		capture;
	spinlock_t			lock;

	unsigned int			buf_cnt;
};

static unsigned long jiff_temp = 0;

#ifdef DUMP
void cim_dump_dma_desc_single(struct jz4780_camera_dev *pcdev, struct jz4780_camera_buffer *buf)
{
	int i = 0;
	struct device *dev = pcdev->soc_host.v4l2_dev.dev;

	dev_info(dev, "------------------desc dump start---------------------\n");
	dev_info(dev, "buf->dma->sg_dma \t= \t%08x\n", buf->dma->sg_dma);
	dev_info(dev, "buf->dma->sg_cpu \t= \t%08x\n", (unsigned int)buf->dma->sg_cpu);
	dev_info(dev, "buf->dma->sg_size \t= \t%08x\n", buf->dma->sg_size);
	dev_info(dev, "buf->dma->sglen \t= \t%08x\n", buf->dma->sglen);
	for (i = 0; i < buf->dma->sglen; i++) {
		dev_info(dev, "buf->dma->sg_cpu[%02d].next \t= \t%08x\n", i, buf->dma->sg_cpu[i].next);
		dev_info(dev, "buf->dma->sg_cpu[%02d].id \t= \t%08x\n", i, buf->dma->sg_cpu[i].id);
		dev_info(dev, "buf->dma->sg_cpu[%02d].buf \t= \t%08x\n", i, buf->dma->sg_cpu[i].buf);
		dev_info(dev, "buf->dma->sg_cpu[%02d].cmd \t= \t%08x\n", i, buf->dma->sg_cpu[i].cmd);
	}
	dev_info(dev, "------------------desc dump end---------------------\n");
}

void cim_dump_dma_desc(struct jz4780_camera_dev *pcdev)
{
	struct jz4780_camera_buffer *pos_buf;
	struct list_head *pos;
	list_for_each(pos, &pcdev->capture) {
		pos_buf = list_entry(pos, struct jz4780_camera_buffer, vb.queue);
		cim_dump_dma_desc_single(pcdev, pos_buf);
	}
}

static void cim_dump_reg(struct jz4780_camera_dev *pcdev)
{
	struct device *dev = pcdev->soc_host.v4l2_dev.dev;

	dev_info(dev, "------------------registers dump start---------------------\n");
	dev_info(dev,"REG_CIM_CFG \t= \t0x%08x\n", readl(pcdev->base + CIM_CFG));
	dev_info(dev,"REG_CIM_CTRL \t= \t0x%08x\n", readl(pcdev->base + CIM_CTRL));
	dev_info(dev,"REG_CIM_CTRL2 \t= \t0x%08x\n", readl(pcdev->base + CIM_CTRL2));
	dev_info(dev,"REG_CIM_STATE \t= \t0x%08x\n", readl(pcdev->base + CIM_STATE));
	dev_info(dev,"REG_CIM_IMR \t= \t0x%08x\n", readl(pcdev->base + CIM_IMR));
	dev_info(dev,"REG_CIM_IID \t= \t0x%08x\n", readl(pcdev->base + CIM_IID));
	dev_info(dev,"REG_CIM_DA \t= \t0x%08x\n", readl(pcdev->base + CIM_DA));
	dev_info(dev,"REG_CIM_FA \t= \t0x%08x\n", readl(pcdev->base + CIM_FA));
	dev_info(dev,"REG_CIM_FID \t= \t0x%08x\n", readl(pcdev->base + CIM_FID));
	dev_info(dev,"REG_CIM_CMD \t= \t0x%08x\n", readl(pcdev->base + CIM_CMD));
	dev_info(dev,"REG_CIM_WSIZE \t= \t0x%08x\n", readl(pcdev->base + CIM_SIZE));
	dev_info(dev,"REG_CIM_WOFFSET = \t0x%08x\n", readl(pcdev->base + CIM_OFFSET));
	dev_info(dev,"REG_CIM_FS \t= \t0x%08x\n", readl(pcdev->base + CIM_FS));
	dev_info(dev,"REG_CIM_YFA \t= \t0x%08x\n", readl(pcdev->base + CIM_YFA));
	dev_info(dev,"REG_CIM_YCMD \t= \t0x%08x\n", readl(pcdev->base + CIM_YCMD));
	dev_info(dev,"REG_CIM_CBFA \t= \t0x%08x\n", readl(pcdev->base + CIM_CBFA));
	dev_info(dev,"REG_CIM_CBCMD \t= \t0x%08x\n", readl(pcdev->base + CIM_CBCMD));
	dev_info(dev,"REG_CIM_CRFA \t= \t0x%08x\n", readl(pcdev->base + CIM_CRFA));
	dev_info(dev,"REG_CIM_CRCMD \t= \t0x%08x\n", readl(pcdev->base + CIM_CRCMD));
	dev_info(dev,"REG_CIM_TC \t= \t0x%08x\n", readl(pcdev->base + CIM_TC));
	dev_info(dev,"REG_CIM_TINX \t= \t0x%08x\n", readl(pcdev->base + CIM_TINX));
	dev_info(dev,"REG_CIM_TCNT \t= \t0x%08x\n", readl(pcdev->base + CIM_TCNT));
	dev_info(dev, "------------------registers dump end---------------------\n");
}
#endif

#if 0
static int jz4780_get_next_prev_buf(struct jz4780_camera_dev *pcdev,
		struct videobuf_buffer *vb,
		struct jz4780_camera_buffer **next_buf,
		struct jz4780_camera_buffer **prev_buf)
{
	int list_long = 0;
	struct list_head *pos;

	list_for_each(pos, &pcdev->capture) {
		list_long++;
	}

	switch (list_long) {
	case 0: *next_buf = NULL; *prev_buf = NULL; break;
	case 1:
		*next_buf = list_entry(vb->queue.next->next,
				struct jz4780_camera_buffer, vb.queue);
		*prev_buf = list_entry(vb->queue.prev->prev,
				struct jz4780_camera_buffer, vb.queue);
		break;
	default:
		*next_buf = list_entry(vb->queue.next->next,
				struct jz4780_camera_buffer, vb.queue);
		*prev_buf = list_entry(vb->queue.prev,
				struct jz4780_camera_buffer, vb.queue);
	}
	dev_info(pcdev->soc_host.v4l2_dev.dev,
		"list_long = %d, *next_buf = %x, *prev_buf = %x\n", list_long,
			(unsigned int)(*next_buf), (unsigned int)(*prev_buf));

	return list_long;
}
#endif

static int is_cim_disabled(struct jz4780_camera_dev *pcdev)
{
	unsigned int regval = 0;

	regval = readl(pcdev->base + CIM_CTRL);
	return  ((regval & 0x1) == 0x0);
}

static void cim_enable_with_dma(struct jz4780_camera_dev *pcdev)
{
	unsigned int regval = 0;
	/* here active is impossibly to be null */
	struct jz4780_camera_buffer *active = list_first_entry(&pcdev->capture,
			struct jz4780_camera_buffer, vb.queue);
	if (list_empty(&pcdev->capture)) {
#if 0
		/* this is no need to tell userspace */
		dev_warn(pcdev->soc_host.v4l2_dev.dev,
			"%s:cim_enable_with_dma failed for no active buf\n",
			__func__);
#endif
		return;
	}

	writel(active->dma->sg_dma, pcdev->base + CIM_DA);
	writel(0, pcdev->base + CIM_STATE);

	regval = readl(pcdev->base + CIM_CTRL);
	regval |= CIM_CTRL_DMA_EN | CIM_CTRL_RXF_RST;
	regval &= ~CIM_CTRL_ENA;
	writel(regval, pcdev->base + CIM_CTRL);

	regval = readl(pcdev->base + CIM_CTRL);
	regval |= CIM_CTRL_DMA_EN;
	regval &= (~CIM_CTRL_RXF_RST & ~CIM_CTRL_ENA);
	writel(regval, pcdev->base + CIM_CTRL);

	regval = readl(pcdev->base + CIM_CTRL);
	regval |= CIM_CTRL_DMA_EN | CIM_CTRL_ENA;
	regval &= ~CIM_CTRL_RXF_RST;
	writel(regval, pcdev->base + CIM_CTRL);
}

void cim_disable_without_dma(struct jz4780_camera_dev *pcdev)
{
	unsigned long temp = 0;

	/* clear status register */
	writel(0, pcdev->base + CIM_STATE);

	/* disable cim */
	temp = readl(pcdev->base + CIM_CTRL);
	temp &= ~CIM_CTRL_ENA;
	writel(temp, pcdev->base + CIM_CTRL);

	/* clear rx fifo */
	temp = readl(pcdev->base + CIM_CTRL);
	temp |= CIM_CTRL_RXF_RST;
	writel(temp, pcdev->base + CIM_CTRL);

	temp = readl(pcdev->base + CIM_CTRL);
	temp &= ~CIM_CTRL_RXF_RST;
	writel(temp, pcdev->base + CIM_CTRL);
}

/* clear dma interrupt status */
void cim_clear_dma_irq_status(struct jz4780_camera_dev *pcdev)
{
	unsigned long temp = 0;

	temp = readl(pcdev->base + CIM_STATE);
	temp &= ~CIM_STATE_DMA_EOF;
	writel(temp, pcdev->base + CIM_STATE);
}

static void jz4780_camera_wakeup(struct jz4780_camera_dev *pcdev,
			      struct videobuf_buffer *vb,
			      struct jz4780_camera_buffer *buf) {
	/* delete from pcdev->capture and set the capture dma */
	list_del_init(&vb->queue);
	vb->state = VIDEOBUF_DONE;
	do_gettimeofday(&vb->ts);
	vb->field_count++;
	wake_up(&vb->done);
	dev_dbg(pcdev->soc_host.v4l2_dev.dev, "%s:after wake up cost %d ms\n",
			__func__, jiffies_to_msecs(jiffies - jiff_temp));
}

static irqreturn_t jz4780_camera_irq_handler(int irq, void *data)
{
	struct jz4780_camera_dev *pcdev = (struct jz4780_camera_dev *)data;
	unsigned long status = 0;
	unsigned long flags = 0;
	unsigned int index = 0;

	/*
	 * this may not needed for if icd is null, then no sensor connect to
	 * the host, this is impossibly. but I'm helpless to keep it for no
	 * much time to prove
	 */
	for (index = 0; index < ARRAY_SIZE(pcdev->icd); index++) {
		if (pcdev->icd[index]) {
			break;
		}
	}

	if(index == MAX_SOC_CAM_NUM)
		return IRQ_HANDLED;

	spin_lock_irqsave(&pcdev->lock, flags);
	/* read interrupt status register */
	status = readl(pcdev->base + CIM_STATE);
	if (!status) {
		return IRQ_NONE;
	}

	cim_disable_without_dma(pcdev);

	if (((status & CIM_STATE_OK_RXF_OF) != CIM_STATE_OK_RXF_OF)
		&& (status & CIM_STATE_RXF_OF)) {
		cim_enable_with_dma(pcdev);
		spin_unlock_irqrestore(&pcdev->lock, flags);
		dev_warn(pcdev->soc_host.v4l2_dev.dev,
			"%s:Rx FIFO OverFlow interrupt!, status=%x\n",
			__func__, (unsigned int)status);
	} else {   /* ((status & CIM_STATE_OK_RXF_OF) == CIM_STATE_OK_RXF_O) */
		struct jz4780_camera_buffer *active = NULL;
		struct jz4780_camera_buffer *buf = NULL;
		struct videobuf_buffer *vb = NULL;

		active = list_first_entry(&pcdev->capture,
				struct jz4780_camera_buffer, vb.queue);
		/* this shouldn't be happened here */
		if (list_empty(&pcdev->capture)) {
#if 0
			/* this is no need to tell userspace */
			dev_warn(pcdev->soc_host.v4l2_dev.dev,
				"%s:DMA_EOF irq with no active buffer\n",
				__func__);
#endif
			spin_unlock_irqrestore(&pcdev->lock, flags);

			return IRQ_HANDLED;
		}
		vb = &active->vb;
		buf = container_of(vb, struct jz4780_camera_buffer, vb);
		WARN_ON(buf->inwork || list_empty(&vb->queue));
		jz4780_camera_wakeup(pcdev, vb, buf);
		cim_enable_with_dma(pcdev);
		spin_unlock_irqrestore(&pcdev->lock, flags);
	}

	dev_dbg(pcdev->soc_host.v4l2_dev.dev,
		"%s:Camera interrupt status 0x%lx\n", __func__, status);
	return IRQ_HANDLED;
}

/******************************************************************************
 * videobuf_buffer related function:
 *****************************************************************************/
static int jz4780_videobuf_setup(struct videobuf_queue *vq, unsigned int *count,
			      unsigned int *size)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;
	int bytes_per_line = soc_mbus_bytes_per_line(icd->user_width,
						icd->current_fmt->host_fmt);

	if (bytes_per_line < 0)
		return bytes_per_line;

	*size = bytes_per_line * icd->user_height;

	if (!*count)
		*count = 32;

	if (*size * *count > MAX_VIDEO_MEM * 1024 * 1024)
		*count = (MAX_VIDEO_MEM * 1024 * 1024) / *size;

	pcdev->buf_cnt = *count;

	dev_dbg(icd->dev.parent, "%s:count=%d, size=%d\n",
			__func__, *count, *size);

	return 0;
}

static int jz4780_camera_reqbufs(struct soc_camera_device *icd,
			      struct v4l2_requestbuffers *p)
{
	int i;
	/*
	 * This is for locking debugging only. I removed spinlocks and now I
	 * check whether .prepare is ever called on a linked buffer, or whether
	 * a dma IRQ can occur for an in-work or unlinked buffer. Until now
	 * it hadn't triggered
	 */
	for (i = 0; i < p->count; i++) {
		struct jz4780_camera_buffer *buf = container_of(
			icd->vb_vidq.bufs[i],struct jz4780_camera_buffer, vb);
		buf->inwork = 0;
		INIT_LIST_HEAD(&buf->vb.queue);
	}

	return 0;
}

static int calculate_dma_sglen(struct scatterlist *sglist, int sglen,
			       int sg_first_ofs, int size)
{
	int i, offset, dma_len, xfer_len;
	struct scatterlist *sg;

	offset = sg_first_ofs;
	for_each_sg(sglist, sg, sglen, i) {
		dma_len = sg_dma_len(sg);

		/* PXA27x Developer's Manual 27.4.4.1: round up to 8 bytes */
		xfer_len = roundup(min(dma_len - offset, size), 32 * 4);

		size = max(0, size - xfer_len);
		offset = 0;
		if (size == 0)
			break;
	}

	BUG_ON(size != 0);
	return i + 1;
}

 /* Returns 0 or -ENOMEM if no coherent memory is available */
static int jz4780_init_sg_dma(struct videobuf_queue *vq,
		struct videobuf_buffer *vb)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;
	struct device *dev = pcdev->soc_host.v4l2_dev.dev;
	struct jz4780_camera_buffer *buf = list_entry(vb,
				struct jz4780_camera_buffer, vb);
	struct videobuf_dmabuf *dma = videobuf_to_dma(vb);
	struct scatterlist *sg = NULL;
	int i = 0, sglen = 0;
	int dma_len = 0, xfer_len = 0;
	int xfer_size = 0, req_size = 0;

	if (buf->dma) {
		dma_free_coherent(dev, buf->dma->sg_size,
				  buf->dma->sg_cpu, buf->dma->sg_dma);
		buf->dma = NULL;
	}
	buf->dma = kzalloc(sizeof(struct jz4780_camera_dma), GFP_KERNEL);
	if (!buf->dma) {
		dev_err(dev, "%s:Could not allocate buf->dma\n", __func__);
		return -ENOMEM;
	}

	sglen = calculate_dma_sglen(dma->sglist, dma->sglen, 0, vb->size);

	buf->dma->sg_size = (sglen + 1) * sizeof(struct jz4780_camera_dma_desc);
	buf->dma->sg_cpu = dma_alloc_coherent(dev, buf->dma->sg_size,
					     &buf->dma->sg_dma, GFP_KERNEL);
	if (!buf->dma->sg_cpu) {
		kfree(buf->dma);
		buf->dma = NULL;
		return -ENOMEM;
	}

	buf->dma->sglen = sglen;

	dev_dbg(dev, "%s:DMA: buf->dma=%p, buf->dma->sg_dma = %x,"
			"buf->dma->sg_cpu = %p, buf->dma->sg_size = %u,"
			"buf->dma->sglen = %d\n", __func__,
			buf->dma, buf->dma->sg_dma, buf->dma->sg_cpu,
			buf->dma->sg_size, buf->dma->sglen);


	if (icd->current_fmt->host_fmt->fourcc == V4L2_PIX_FMT_YUYV) {
		req_size = icd->sizeimage;
	} else { /* I'm not sure other format ruquirement, need to complete*/
		req_size = icd->sizeimage;
	}

	/*
	 * vb->size come from buf_setup, so this can not to be a good choose to
	 * set it to a actual dma len, we need to get the exactly image size to dma transfered
	 */
	if (req_size > vb->size) {	/* this is impossibly to happened */
		dev_err(dev, "%s:request size(%d) big than vb->size(%u)\n",
				__func__, req_size, (unsigned int)vb->size);
		dma_free_coherent(dev, buf->dma->sg_size, buf->dma->sg_cpu,
				     buf->dma->sg_dma);
		kfree(buf->dma);
		buf->dma = NULL;
		return -1;
	} else {
		xfer_size = req_size;
	}

	for_each_sg(dma->sglist, sg, sglen, i) {
		dma_len = sg_dma_len(sg);

		/* frame buf addr must be aligned to 32-word boundary */
		xfer_len = roundup(min(dma_len, xfer_size), 32 * 4);
		xfer_size = max(0, xfer_size - xfer_len);

		if ((i == (sglen - 1)) && (xfer_size != 0)) {
			dev_err(dev, "%s:the allocated sg space is too small to"
					" dma, xfer_size = %d, sglen = %d\n",
					__func__, xfer_size, sglen);
			dma_free_coherent(dev, buf->dma->sg_size, buf->dma->sg_cpu,
					     buf->dma->sg_dma);
			kfree(buf->dma);
			buf->dma = NULL;
			return -1;
		}

		/* I supposed frame id equal to vb id */
		buf->dma->sg_cpu[i].id = vb->i;
		//buf->dma->sg_cpu[i].buf = sg_dma_address(sg);
		buf->dma->sg_cpu[i].buf = sg_phys(sg);

		/* i <= (sglen - 1) is certainly */
		if (xfer_size == 0) {
			/* for vb isn't still queued into pcdev->capture */
			buf->dma->sg_cpu[i].next = ((dma_addr_t)buf->dma->sg_dma);
			buf->dma->sg_cpu[i].cmd = xfer_len >> 2 |
						CIM_CMD_EOFINT | CIM_CMD_OFRCV
						| CIM_CMD_STOP;
			buf->dma->sglen = i + 1;
			break;
		} else {
			/* buf->dma->sg_cpu[i].next must be the physical addr */
			buf->dma->sg_cpu[i].next = buf->dma->sg_dma +
				(i + 1) * sizeof(struct jz4780_camera_dma_desc);
			buf->dma->sg_cpu[i].cmd = xfer_len >> 2 | CIM_CMD_OFRCV;
		}
	}

	return 0;
}

static void free_buffer(struct videobuf_queue *vq, struct jz4780_camera_buffer *buf)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct videobuf_dmabuf *dma = videobuf_to_dma(&buf->vb);

	BUG_ON(in_interrupt());

	dev_dbg(icd->dev.parent, "%s:(vb=0x%p) 0x%08lx %d\n",
		__func__, &buf->vb, buf->vb.baddr, buf->vb.bsize);

	/*
	 * This waits until this buffer is out of danger, i.e., until it is no
	 * longer in STATE_QUEUED or STATE_ACTIVE
	 */
	videobuf_waiton(vq, &buf->vb, 0, 0);
	videobuf_dma_unmap(vq->dev, dma);
	videobuf_dma_free(dma);

	if (buf->dma && buf->dma->sg_cpu) {
		dma_free_coherent(ici->v4l2_dev.dev,
				  buf->dma->sg_size,
				  buf->dma->sg_cpu,
				  buf->dma->sg_dma);
		kfree(buf->dma);
		buf->dma = NULL;
	}

	buf->vb.state = VIDEOBUF_NEEDS_INIT;
}

static int jz4780_videobuf_prepare(struct videobuf_queue *vq,
		struct videobuf_buffer *vb, enum v4l2_field field)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;
	struct device *dev = pcdev->soc_host.v4l2_dev.dev;
	struct jz4780_camera_buffer *buf = container_of(vb,
				struct jz4780_camera_buffer, vb);
	int ret = -1;
	int bytes_per_line = soc_mbus_bytes_per_line(icd->user_width,
						icd->current_fmt->host_fmt);

	if (bytes_per_line < 0)
		return bytes_per_line;

	dev_dbg(dev, "%s:(vb=0x%p) 0x%08lx %d\n",
			__func__, vb, vb->baddr, vb->bsize);

	/* Added list head initialization on alloc */
	WARN_ON(!list_empty(&vb->queue));

#ifdef DEBUG
	/*
	 * This can be useful if you want to see if we actually fill
	 * the buffer with something
	 */
	memset((void *)vb->baddr, 0xaa, vb->bsize);
#endif

	BUG_ON(NULL == icd->current_fmt);

	/*
	 * I think, in buf_prepare you only have to protect global data,
	 * the actual buffer is yours
	 */
	buf->inwork = 1;

	if (buf->code	!= icd->current_fmt->code ||
	    vb->width	!= icd->user_width ||
	    vb->height	!= icd->user_height ||
	    vb->field	!= field) {
		buf->code	= icd->current_fmt->code;
		vb->width	= icd->user_width;
		vb->height	= icd->user_height;
		vb->field	= field;
		vb->state	= VIDEOBUF_NEEDS_INIT;
	}

	vb->size = bytes_per_line * vb->height;
	if (0 != vb->baddr && vb->bsize < vb->size) {
		ret = -EINVAL;
		goto out;
	}

	if (vb->state == VIDEOBUF_NEEDS_INIT) {
		ret = videobuf_iolock(vq, vb, NULL);
		if (ret) {
			goto fail;
		}

		ret = jz4780_init_sg_dma(vq, vb);
		if (ret) {
			dev_err(dev, "%s:DMA initialization for Y/RGB failed\n"
					, __func__);
			goto fail;
		}

		vb->state = VIDEOBUF_PREPARED;
	}

	buf->inwork = 0;

	return 0;

fail:
	free_buffer(vq, buf);
out:
	buf->inwork = 0;
	return ret;
}

static void jz4780_videobuf_queue(struct videobuf_queue *vq,
					struct videobuf_buffer *vb)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;

	list_add_tail(&vb->queue, &pcdev->capture);

	vb->state = VIDEOBUF_ACTIVE;

	if (is_cim_disabled(pcdev)) {
		cim_enable_with_dma(pcdev);
	}
}

static void jz4780_videobuf_release(struct videobuf_queue *vq,
				 struct videobuf_buffer *vb)
{
	struct jz4780_camera_buffer *buf = container_of(vb,
			struct jz4780_camera_buffer, vb);
	struct soc_camera_device *icd = vq->priv_data;
	struct device *dev = icd->dev.parent;

	dev_dbg(dev, "%s:(vb=0x%p) 0x%08lx %d\n", __func__,
		vb, vb->baddr, vb->bsize);

	switch (vb->state) {
	case VIDEOBUF_ACTIVE:
		dev_dbg(dev, "%s:(active)\n", __func__);
		break;
	case VIDEOBUF_QUEUED:
		dev_dbg(dev, "%s:(queued)\n", __func__);
		break;
	case VIDEOBUF_PREPARED:
		dev_dbg(dev, "%s:(prepared)\n", __func__);
		break;
	default:
		dev_dbg(dev, "%s:(unknown)\n", __func__);
		break;
	}

	free_buffer(vq, buf);
}

static struct videobuf_queue_ops jz4780_videobuf_ops = {
	.buf_setup	= jz4780_videobuf_setup,
	.buf_prepare	= jz4780_videobuf_prepare,
	.buf_queue	= jz4780_videobuf_queue,
	.buf_release	= jz4780_videobuf_release,
};

static void jz4780_camera_init_videobuf(struct videobuf_queue *q,
				     struct soc_camera_device *icd)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;

	videobuf_queue_sg_init(q, &jz4780_videobuf_ops, icd->dev.parent,
				&pcdev->lock, V4L2_BUF_TYPE_VIDEO_CAPTURE,
				V4L2_FIELD_NONE,
				sizeof(struct jz4780_camera_buffer),
				icd, &icd->video_lock);
}

static unsigned int jz4780_camera_poll(struct file *file, poll_table *pt)
{
	struct soc_camera_device *icd = file->private_data;
	struct jz4780_camera_buffer *buf = list_entry(icd->vb_vidq.stream.next,
			struct jz4780_camera_buffer, vb.stream);

	jiff_temp = jiffies;
	poll_wait(file, &buf->vb.done, pt);

	if ((buf->vb.state == VIDEOBUF_DONE) ||
	    (buf->vb.state == VIDEOBUF_ERROR)) {
		return POLLIN | POLLRDNORM;
	}

	return 0;
}
/************************end of videobuf_buffer related function **************/

/*
 * jz4780_camera_activate - open clk of cim
 * here I only do clk issure for configuring the cim is done by set_bus_param.
 * I should hold to avoid repeated-code athough it's the best place to
 * initialize the cim.
 */
static int jz4780_camera_activate(struct jz4780_camera_dev *pcdev, int icd_index)
{
	if(pcdev->clk) {
		if (clk_enable(pcdev->clk) < 0) {
			dev_err(pcdev->icd[icd_index]->dev.parent,
					"%s: enable clk failed!\n", __func__);
			goto err_enable_clk;
		}
	}

	if(pcdev->mclk) {
		if (clk_set_rate(pcdev->mclk, pcdev->mclk_freq) < 0) {
			dev_err(pcdev->icd[icd_index]->dev.parent,
					"%s: set mclk rate failed!\n", __func__);
			goto err_set_mclk_rate;
		}

		if (clk_enable(pcdev->mclk) < 0) {
			dev_err(pcdev->icd[icd_index]->dev.parent,
					"%s: enable mclk failed!\n", __func__);
			goto err_enable_mclk;
		}
	}

	msleep(10);

	dev_dbg(pcdev->icd[icd_index]->dev.parent, "%s: Activate device\n",
			__func__);

	return 0;

err_enable_mclk:
err_set_mclk_rate:
	clk_disable(pcdev->clk);
err_enable_clk:
	return -1;
}

/*
 * jz4780_camera_deactivate - disable clk and configures
 * Athough I only can simply disable clk and cim.
 * To cautious, We I should recover other configures like interrupt, dma.
 */
static void jz4780_camera_deactivate(struct jz4780_camera_dev *pcdev)
{
	unsigned long temp = 0;

	if(pcdev->clk) {
		clk_disable(pcdev->clk);
	}

	if(pcdev->mclk) {
		clk_disable(pcdev->mclk);
	}

	writel(0, pcdev->base + CIM_STATE);

	/* disable end of frame interrupt */
	temp = readl(pcdev->base + CIM_IMR);
	temp |= CIM_IMR_EOFM;
	writel(temp, pcdev->base + CIM_IMR);

	/* disable rx overflow interrupt */
	temp = readl(pcdev->base + CIM_IMR);
	temp |= CIM_IMR_RFIFO_OFM;
	writel(temp, pcdev->base + CIM_IMR);

	/* disable dma */
	temp = readl(pcdev->base + CIM_CTRL);
	temp &= ~CIM_CTRL_DMA_EN;
	writel(temp, pcdev->base + CIM_CTRL);

	/* clear rx fifo */
	temp = readl(pcdev->base + CIM_CTRL);
	temp |= CIM_CTRL_RXF_RST;
	writel(temp, pcdev->base + CIM_CTRL);

	temp = readl(pcdev->base + CIM_CTRL);
	temp &= ~CIM_CTRL_RXF_RST;
	writel(temp, pcdev->base + CIM_CTRL);

	/* disable cim */
	temp = readl(pcdev->base + CIM_CTRL);
	temp &= ~CIM_CTRL_ENA;
	writel(temp, pcdev->base + CIM_CTRL);
}

/*
 * The following two functions absolutely depend on the fact, that
 * there can be only one camera on PXA quick capture interface
 * Called with .video_lock held
 */
static int jz4780_camera_add_device(struct soc_camera_device *icd)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;

	int icd_index = icd->devnum;

	if (pcdev->icd[icd_index]) {
		return -EBUSY;
	}

	pcdev->icd[icd_index] = icd;
	if (jz4780_camera_activate(pcdev, icd_index) < 0) {
		return -1;
	}

	dev_dbg(icd->dev.parent, "%s:JZ4780 Camera driver attached "
			"to camera %d\n", __func__, icd->devnum);

	return 0;
}

static void jz4780_camera_remove_device(struct soc_camera_device *icd)
{

	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;

	int icd_index = icd->devnum;

	BUG_ON(icd != pcdev->icd[icd_index]);

	jz4780_camera_deactivate(pcdev);

	dev_info(icd->dev.parent, "%s: jz4780 Camera driver detached from "
			"camera %d\n", __func__, icd->devnum);

	pcdev->icd[icd_index] = NULL;
}

static int jz4780_camera_set_bus_param(struct soc_camera_device *icd,
		__u32 pixfmt) {

	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;
	unsigned long camera_flags, common_flags;
	unsigned long cfg_reg = 0;
	unsigned long ctrl_reg = 0;
	unsigned long ctrl2_reg = 0;
	unsigned long fs_reg = 0;
	unsigned long temp = 0;
	int ret;

	camera_flags = icd->ops->query_bus_param(icd);

	common_flags = soc_camera_bus_param_compatible(camera_flags,
						       CIM_BUS_FLAGS);
	if (!common_flags)
		return -EINVAL;

	/* Make choises, based on platform choice, this is important */
	if ((common_flags & SOCAM_VSYNC_ACTIVE_HIGH) &&
		(common_flags & SOCAM_VSYNC_ACTIVE_LOW)) {
			if (!pcdev->pdata ||
			     pcdev->pdata->flags & JZ4780_CAMERA_VSYNC_HIGH)
				common_flags &= ~SOCAM_VSYNC_ACTIVE_LOW;
			else
				common_flags &= ~SOCAM_VSYNC_ACTIVE_HIGH;
	}

	if ((common_flags & SOCAM_PCLK_SAMPLE_RISING) &&
		(common_flags & SOCAM_PCLK_SAMPLE_FALLING)) {
			if (!pcdev->pdata ||
			     pcdev->pdata->flags & JZ4780_CAMERA_PCLK_RISING)
				common_flags &= ~SOCAM_PCLK_SAMPLE_FALLING;
			else
				common_flags &= ~SOCAM_PCLK_SAMPLE_RISING;
	}

	if ((common_flags & SOCAM_DATA_ACTIVE_HIGH) &&
		(common_flags & SOCAM_DATA_ACTIVE_LOW)) {
			if (!pcdev->pdata ||
			     pcdev->pdata->flags & JZ4780_CAMERA_DATA_HIGH)
				common_flags &= ~SOCAM_DATA_ACTIVE_LOW;
			else
				common_flags &= ~SOCAM_DATA_ACTIVE_HIGH;
	}

	ret = icd->ops->set_bus_param(icd, common_flags);
	if (ret < 0)
		return ret;


	cfg_reg = (common_flags & SOCAM_PCLK_SAMPLE_FALLING) ?
			cfg_reg | CIM_CFG_PCP : cfg_reg & (~CIM_CFG_PCP);

	cfg_reg = (common_flags & SOCAM_VSYNC_ACTIVE_LOW) ?
				cfg_reg | CIM_CFG_VSP : cfg_reg & (~CIM_CFG_VSP);

	cfg_reg = (common_flags & SOCAM_HSYNC_ACTIVE_LOW) ?
				cfg_reg | CIM_CFG_HSP : cfg_reg & (~CIM_CFG_HSP);

	cfg_reg |= CIM_CFG_DMA_BURST_INCR32 | CIM_CFG_DF_YUV422
			| CIM_CFG_ORDER_YUYV | CIM_CFG_DSM_GCM
			| CIM_CFG_PACK_Y0UY1V;
	/* To sg dma, we can only support package data */
	cfg_reg &= ~CIM_CFG_SEP;

	ctrl_reg |= CIM_CTRL_DMA_SYNC | CIM_CTRL_FRC_1;

	ctrl2_reg |= CIM_CTRL2_APM | CIM_CTRL2_EME | CIM_CTRL2_OPE |
			(1 << CIM_CTRL2_OPG_BIT) | CIM_CTRL2_FSC | CIM_CTRL2_ARIF;
	fs_reg = (icd->user_width -1) << CIM_FS_FHS_BIT | (icd->user_height -1)
				<< CIM_FS_FVS_BIT | 1<< CIM_FS_BPP_BIT;

#if 0
	if((pixfmt == V4L2_PIX_FMT_YUV420) ||
			(pixfmt == V4L2_PIX_FMT_JZ420B)) {
		ctrl2_reg |= CIM_CTRL2_CSC_YUV420;
		cfg_reg |= CIM_CFG_SEP | CIM_CFG_ORDER_YUYV;
	}

	if(pixfmt == V4L2_PIX_FMT_JZ420B)
		ctrl_reg |= CIM_CTRL_MBEN;
#endif

	writel(cfg_reg, pcdev->base + CIM_CFG);
	writel(ctrl_reg, pcdev->base + CIM_CTRL);
	writel(ctrl2_reg, pcdev->base + CIM_CTRL2);
	writel(fs_reg, pcdev->base + CIM_FS);

	/* enable end of frame interrupt */
	temp = readl(pcdev->base + CIM_IMR);
	temp &= ~CIM_IMR_EOFM;
	writel(temp, pcdev->base + CIM_IMR);

	/* enable rx overflow interrupt */
	temp = readl(pcdev->base + CIM_IMR);
	temp &= ~CIM_IMR_RFIFO_OFM;
	writel(temp, pcdev->base + CIM_IMR);

#ifdef DEBUG_SOTP
	/* enable stop interrupt */
	temp = readl(pcdev->base + CIM_IMR);
	temp &= ~CIM_IMR_STPM;
	writel(temp, pcdev->base + CIM_IMR);
#endif

	return 0;
}

static int jz4780_camera_set_crop(struct soc_camera_device *icd,
			       struct v4l2_crop *a)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);

	return v4l2_subdev_call(sd, video, s_crop, a);
}

static int jz4780_camera_set_fmt(struct soc_camera_device *icd,
			      struct v4l2_format *f)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	int ret, buswidth;

	xlate = soc_camera_xlate_by_fourcc(icd, pix->pixelformat);
	if (!xlate) {
		dev_err(icd->dev.parent, "%s: Format %x not found\n",
			 __func__, pix->pixelformat);
		return -EINVAL;
	}

	buswidth = xlate->host_fmt->bits_per_sample;
	if (buswidth > 8) {
		dev_err(icd->dev.parent,
			"%s: bits-per-sample %d for format %x unsupported\n",
			 __func__, buswidth, pix->pixelformat);
		return -EINVAL;
	}

	mf.width	= pix->width;
	mf.height	= pix->height;
	mf.field	= pix->field;
	mf.colorspace	= pix->colorspace;
	mf.code		= xlate->code;

	ret = v4l2_subdev_call(sd, video, s_mbus_fmt, &mf);
	if (ret < 0)
		return ret;

	if (mf.code != xlate->code)
		return -EINVAL;

	pix->width		= mf.width;
	pix->height		= mf.height;
	pix->field		= mf.field;
	pix->colorspace		= mf.colorspace;
	icd->current_fmt	= xlate;

	return ret;
}

static int jz4780_camera_try_fmt(struct soc_camera_device *icd,
			      struct v4l2_format *f)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	int ret;
	/* TODO: limit to JZ4780 hardware capabilities */

	xlate = soc_camera_xlate_by_fourcc(icd, pix->pixelformat);
	if (!xlate) {
		dev_err(icd->dev.parent, "Format %x not found\n",
			 pix->pixelformat);
		return -EINVAL;
	}

	mf.width = pix->width;
	mf.height = pix->height;
	mf.field = pix->field;
	mf.colorspace = pix->colorspace;
	mf.code	= xlate->code;

	/* limit to sensor capabilities */
	ret = v4l2_subdev_call(sd, video, try_mbus_fmt, &mf);
	if (ret < 0)
		return ret;

	pix->width = mf.width;
	pix->height	= mf.height;
	pix->field = mf.field;
	pix->colorspace	= mf.colorspace;

	return 0;
}

static int jz4780_camera_querycap(struct soc_camera_host *ici,
			       struct v4l2_capability *cap)
{
	/* cap->name is set by the friendly caller:-> */
	strlcpy(cap->card, "JZ4780-Camera", sizeof(cap->card));
	cap->version = JZ4780_CAM_VERSION_CODE;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

	return 0;
}

static struct soc_camera_host_ops jz4780_soc_camera_host_ops = {
	.owner = THIS_MODULE,
	.add = jz4780_camera_add_device,
	.remove	= jz4780_camera_remove_device,
	.set_bus_param = jz4780_camera_set_bus_param,
	.set_crop = jz4780_camera_set_crop,
	.set_fmt = jz4780_camera_set_fmt,
	.try_fmt = jz4780_camera_try_fmt,
	.init_videobuf = jz4780_camera_init_videobuf,
	.reqbufs = jz4780_camera_reqbufs,
	.poll = jz4780_camera_poll,
	.querycap = jz4780_camera_querycap,
#if 0
	.set_tlb_base = jz4780_camera_set_tlb_base,
#endif
	.num_controls = 0,
};

/* initial camera sensor reset and power gpio */
static int camera_sensor_gpio_init(struct platform_device *pdev) {
	int ret, i, cnt = 0;
	struct jz4780_camera_pdata *pcdev = pdev->dev.platform_data;
	char gpio_name[20];

	if(!pcdev) {
		dev_err(&pdev->dev, "%s:have not cim platform data\n", __func__);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(pcdev->cam_sensor_pdata); i++) {
		if(pcdev->cam_sensor_pdata[i].gpio_rst) {
			sprintf(gpio_name, "sensor_rst%d", i);
			ret = gpio_request(pcdev->cam_sensor_pdata[i].gpio_rst,
					gpio_name);
			if(ret) {
				dev_err(&pdev->dev, "%s:request cim%d reset gpio fail\n",
						__func__, i);
				goto err_request_gpio;
			}
			gpio_direction_output(pcdev->cam_sensor_pdata[i].gpio_rst, 0);
			cnt++;
		}

		if(pcdev->cam_sensor_pdata[i].gpio_power) {
			sprintf(gpio_name, "sensor_en%d", i);
			ret = gpio_request(pcdev->cam_sensor_pdata[i].gpio_power,
					gpio_name);
			if(ret) {
				dev_err(&pdev->dev, "%s:request cim%d en gpio fail\n",
						__func__, i);
				goto err_request_gpio;
			}
			gpio_direction_output(pcdev->cam_sensor_pdata[i].gpio_power, 1);
			cnt++;
		}
	}

	return 0;

err_request_gpio:
	for (i = 0; i < cnt / 2; i++) {
		gpio_free(pcdev->cam_sensor_pdata[i].gpio_rst);
		gpio_free(pcdev->cam_sensor_pdata[i].gpio_power);
	}

	if ((cnt % 2) != 0) {
		gpio_free(pcdev->cam_sensor_pdata[i].gpio_rst);
	}

	return ret;
}

static int __devinit jz4780_camera_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct jz4780_camera_dev *pcdev;
	void __iomem *base;
	int irq;
	int err = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);
	if (!res || irq <= 0) {
		err = -ENODEV;
		dev_err(&pdev->dev, ":%s: %s%s", __func__,  !res ?
			"can't get resource\n":"", irq<0?"can't get irq\n":"");
		goto err_get_resource_irq;
	}

	pcdev = kzalloc(sizeof(*pcdev), GFP_KERNEL);
	if (!pcdev) {
		dev_err(&pdev->dev, "%s:Could not allocate pcdev\n", __func__);
		err = -ENOMEM;
		goto err_kzalloc_pcdev;
	}
#if defined(CONFIG_BOARD_4775_MENSA)
	pcdev->clk = clk_get(&pdev->dev, "cim1");
	if (IS_ERR(pcdev->clk)) {
		err = PTR_ERR(pcdev->clk);
		dev_err(&pdev->dev, "%s:can't get clk %s\n", __func__,  "cim1");
		goto err_get_clk_cim;
	}
#else
	pcdev->clk = clk_get(&pdev->dev, "cim");
	if (IS_ERR(pcdev->clk)) {
		err = PTR_ERR(pcdev->clk);
		dev_err(&pdev->dev, "%s:can't get clk %s\n", __func__,  "cim");
		goto err_get_clk_cim;
	}
#endif

#if defined(CONFIG_BOARD_4775_MENSA)
	pcdev->mclk = clk_get(&pdev->dev, "cgu_cimmclk1");
	if (IS_ERR(pcdev->mclk)) {
		err = PTR_ERR(pcdev->mclk);
		dev_err(&pdev->dev, "%s:can't get clk %s\n",
				__func__, "cgu_cimmclk1");
		goto err_get_clk_cgu_cimmclk;
	}
#else
	pcdev->mclk = clk_get(&pdev->dev, "cgu_cimmclk");
	if (IS_ERR(pcdev->mclk)) {
		err = PTR_ERR(pcdev->mclk);
		dev_err(&pdev->dev, "%s:can't get clk %s\n",
				__func__, "cgu_cimmclk");
		goto err_get_clk_cgu_cimmclk;
	}
#endif
	pcdev->soc_host.regul = regulator_get(&pdev->dev, "vcim");
	if(IS_ERR(pcdev->soc_host.regul)){
		err = -ENODEV;
		dev_err(&pdev->dev, "%s:can't get regulator %s!\n",
				__func__, "vcim");
		pcdev->soc_host.regul = NULL;
	}

	pcdev->res = res;

	pcdev->pdata = pdev->dev.platform_data;
	if (pcdev->pdata) {
		pcdev->mclk_freq = pcdev->pdata->mclk_10khz * 10000;
	}

	if (!pcdev->mclk_freq) {
		dev_warn(&pdev->dev, "%s: mclk_10khz == 0! Please, fix your "
			"platform data. Using default 24MHz\n", __func__);
		pcdev->mclk_freq = 24000000;
	}

	INIT_LIST_HEAD(&pcdev->capture);
	spin_lock_init(&pcdev->lock);

	if (!request_mem_region(res->start, resource_size(res), JZ4780_CAM_DRV_NAME)) {
		err = -EBUSY;
		dev_err(&pdev->dev, "%s:request_mem_region failed\n", __func__);
		goto err_request_mem_region;
	}

	base = ioremap(res->start, resource_size(res));
	if (!base) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "%s:ioremap resource failed\n", __func__);
		goto err_ioremap_resource;
	}
	pcdev->irq = irq;
	pcdev->base = base;

	/* request irq */
	err = request_irq(pcdev->irq, jz4780_camera_irq_handler, IRQF_DISABLED,
						dev_name(&pdev->dev), pcdev);
	if(err) {
		dev_err(&pdev->dev, "%s:request irq failed!\n", __func__);
		goto err_request_irq;
	}

	/* request sensor reset and power gpio */
	err = camera_sensor_gpio_init(pdev);
	if(err) {
		dev_err(&pdev->dev, "%s:camera_sensor_gpio_init failed !\n",
				__func__);
		goto err_gpio_init;
	}

	pcdev->soc_host.drv_name	= JZ4780_CAM_DRV_NAME;
	pcdev->soc_host.ops		= &jz4780_soc_camera_host_ops;
	pcdev->soc_host.priv		= pcdev;
	pcdev->soc_host.v4l2_dev.dev	= &pdev->dev;
	pcdev->soc_host.nr		= pdev->id;
	err = soc_camera_host_register(&pcdev->soc_host);
	if (err) {
		dev_err(&pdev->dev, "%s:soc_camera_host_register failed\n",
				__func__);
		goto err_camera_host_register;
	}

	dev_info(&pdev->dev, "%s:JZ4780 cim driver probe ok\n", __func__);

	return 0;

err_camera_host_register:
err_gpio_init:
	free_irq(pcdev->irq, pcdev);
err_request_irq:
	iounmap(base);
err_ioremap_resource:
	release_mem_region(res->start, resource_size(res));
err_request_mem_region:
	clk_put(pcdev->mclk);
err_get_clk_cgu_cimmclk:
	clk_put(pcdev->clk);
err_get_clk_cim:
	kfree(pcdev);
err_kzalloc_pcdev:
err_get_resource_irq:
	return err;
}

static int __devexit jz4780_camera_remove(struct platform_device *pdev)
{
	struct soc_camera_host *soc_host = to_soc_camera_host(&pdev->dev);
	struct jz4780_camera_dev *pcdev = container_of(soc_host,
					struct jz4780_camera_dev, soc_host);
	struct resource *res;

	free_irq(pcdev->irq, pcdev);

	clk_put(pcdev->clk);
	clk_put(pcdev->mclk);

	soc_camera_host_unregister(soc_host);

	iounmap(pcdev->base);

	res = pcdev->res;
	release_mem_region(res->start, resource_size(res));

	kfree(pcdev);

	return 0;
}

static struct platform_driver jz4780_camera_driver = {
	.driver 	= {
		.name	= JZ4780_CAM_DRV_NAME,
	},
	.remove		= __devexit_p(jz4780_camera_remove),
};

static int __init jz4780_camera_init(void)
{
	return platform_driver_probe(&jz4780_camera_driver, jz4780_camera_probe);
}

static void __exit jz4780_camera_exit(void)
{
	return platform_driver_unregister(&jz4780_camera_driver);
}

late_initcall(jz4780_camera_init);
module_exit(jz4780_camera_exit);

MODULE_DESCRIPTION("JZ4780 SoC Camera Host driver");
MODULE_AUTHOR("ptkang <ptkang@ingenic.cn>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" JZ4780_CAM_DRV_NAME);
