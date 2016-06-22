/*
 * V4L2 Driver for Ingenic JZ4780 camera (CIM) host
 *
 * Copyright (C) 2012, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>

#include <media/soc_camera.h>
#include <media/v4l2-common.h>
#include <media/v4l2-dev.h>
#include <media/videobuf-dma-contig.h>
#include <media/soc_mediabus.h>

#include <asm/dma.h>
#include <mach/jz4780_camera.h>
#include <linux/gpio.h>


static int debug = 3;
module_param(debug, int, 0644);

#define dprintk(level, fmt, arg...)					\
	do {								\
		if (debug >= level)					\
			printk("jz4780-camera: " fmt, ## arg);	\
	} while (0)

/*
 * CIM registers
 */
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

/* CIM State Register  (CIM_STATE) */
#define	CIM_STATE_DMA_EEOF	(1 << 11) /* DMA Line EEOf irq */
#define	CIM_STATE_DMA_STOP	(1 << 10) /* DMA stop irq */
#define	CIM_STATE_DMA_SOF	(1 << 8) /* DMA start irq */
#define	CIM_STATE_DMA_EOF	(1 << 9) /* DMA end irq */
#define	CIM_STATE_TLB_ERR	(1 << 4) /* TLB error */
#define	CIM_STATE_SIZE_ERR	(1 << 4) /* Frame size check error */
#define	CIM_STATE_RXF_OF	(1 << 2) /* RXFIFO over flow irq */
#define	CIM_STATE_RXF_EMPTY	(1 << 1) /* RXFIFO empty irq */
#define	CIM_STATE_VDD		(1 << 0) /* CIM disabled irq */

/* CIM DMA Command Register (CIM_CMD) */
#define	CIM_CMD_SOFINT		(1 << 31) /* enable DMA start irq */
#define	CIM_CMD_EOFINT		(1 << 30) /* enable DMA end irq */
#define	CIM_CMD_EEOFINT		(1 << 29) /* enable DMA EEOF irq */
#define	CIM_CMD_STOP		(1 << 28) /* enable DMA stop irq */
#define CIM_CMD_OFRCV       (1 << 27)

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


/*CIM Interrupt Mask Register (CIMIMR)*/
#define CIM_IMR_EOFM		(1<<9)
#define CIM_IMR_SOFM		(1<<8)
#define CIM_IMR_TLBEM		(1<<4)
#define CIM_IMR_FSEM		(1<<3)
#define CIM_IMR_RFIFO_OFM	(1<<2)

/* CIM Frame Size Register (CIM_FS) */
#define CIM_FS_FVS_BIT		16	/* vertical size of the frame */
#define CIM_FS_FVS_MASK		(0x1fff << CIM_FS_FVS_BIT)
#define CIM_FS_BPP_BIT		14	/* bytes per pixel */
#define CIM_FS_BPP_MASK		(0x3 << CIM_FS_BPP_BIT)
#define CIM_FS_FHS_BIT		0	/* horizontal size of the frame */
#define CIM_FS_FHS_MASK		(0x1fff << CIM_FS_FHS_BIT)

/* CIM TLB Control Register (CIMTC) */
#define CIM_TC_RBA			(1 << 2)
#define CIM_TC_RST			(1 << 1)
#define CIM_TC_ENA			(1 << 0)

#define CIM_BUS_FLAGS	(SOCAM_MASTER | SOCAM_VSYNC_ACTIVE_HIGH | \
			SOCAM_VSYNC_ACTIVE_LOW | SOCAM_HSYNC_ACTIVE_HIGH | \
			SOCAM_HSYNC_ACTIVE_LOW | SOCAM_PCLK_SAMPLE_RISING | \
			SOCAM_PCLK_SAMPLE_FALLING | SOCAM_DATAWIDTH_8)

#define VERSION_CODE KERNEL_VERSION(0, 0, 1)
#define DRIVER_NAME "jz4780-cim"
#define MAX_VIDEO_MEM 16	/* Video memory limit in megabytes */

/*
 * Structures
 */

struct jz4780_camera_dma_desc {
	dma_addr_t next;
	unsigned int id;
	unsigned int buf;
	unsigned int cmd;
	/* only used when SEP = 1 */
	unsigned int cb_frame;
	unsigned int cb_len;
	unsigned int cr_frame;
	unsigned int cr_len;
} __attribute__ ((aligned (32)));

/* buffer for one video frame */
struct jz4780_buffer {
	/* common v4l buffer stuff -- must be first */
	struct videobuf_buffer vb;
	enum v4l2_mbus_pixelcode code;
	int	inwork;
};

struct jz4780_camera_dev {
	struct soc_camera_host soc_host;
	struct soc_camera_device *icd[2];
	struct jz4780_camera_pdata *pdata;
	struct jz4780_buffer *active;
	struct resource	*res;
	struct clk *clk;
	struct clk *mclk;
	struct list_head capture;

	void __iomem *base;
	unsigned int irq;
	unsigned long mclk_freq;

	spinlock_t lock;
	unsigned int buf_cnt;
	struct jz4780_camera_dma_desc *dma_desc;
	void *desc_vaddr;
	int is_first_start;
	unsigned int is_tlb_enabled;
};


static void cim_dump_reg(struct jz4780_camera_dev *pcdev)
{
#define STRING 	"\t=\t0x%08x\n"

	dprintk(6, "REG_CIM_CFG" STRING, readl(pcdev->base + CIM_CFG));
	dprintk(6, "REG_CIM_CTRL" STRING, readl(pcdev->base + CIM_CTRL));
	dprintk(6, "REG_CIM_CTRL2" STRING, readl(pcdev->base + CIM_CTRL2));
	dprintk(6, "REG_CIM_STATE" STRING, readl(pcdev->base + CIM_STATE));

	dprintk(6, "REG_CIM_IMR" STRING, readl(pcdev->base + CIM_IMR));
	dprintk(6, "REG_CIM_IID" STRING, readl(pcdev->base + CIM_IID));
	dprintk(6, "REG_CIM_DA" STRING, readl(pcdev->base + CIM_DA));
	dprintk(6, "REG_CIM_FA" STRING, readl(pcdev->base + CIM_FA));

	dprintk(6, "REG_CIM_FID" STRING, readl(pcdev->base + CIM_FID));
	dprintk(6, "REG_CIM_CMD" STRING, readl(pcdev->base + CIM_CMD));
	dprintk(6, "REG_CIM_WSIZE" STRING, readl(pcdev->base + CIM_SIZE));
	dprintk(6, "REG_CIM_WOFFSET" STRING, readl(pcdev->base + CIM_OFFSET));

	dprintk(6, "REG_CIM_FS" STRING, readl(pcdev->base + CIM_FS));
	dprintk(6, "REG_CIM_YFA" STRING, readl(pcdev->base + CIM_YFA));
	dprintk(6, "REG_CIM_YCMD" STRING, readl(pcdev->base + CIM_YCMD));
	dprintk(6, "REG_CIM_CBFA" STRING, readl(pcdev->base + CIM_CBFA));

	dprintk(6, "REG_CIM_CBCMD" STRING, readl(pcdev->base + CIM_CBCMD));
	dprintk(6, "REG_CIM_CRFA" STRING, readl(pcdev->base + CIM_CRFA));
	dprintk(6, "REG_CIM_CRCMD" STRING, readl(pcdev->base + CIM_CRCMD));
	dprintk(6, "REG_CIM_TC" STRING, readl(pcdev->base + CIM_TC));

	dprintk(6, "REG_CIM_TINX" STRING, readl(pcdev->base + CIM_TINX));
	dprintk(6, "REG_CIM_TCNT" STRING, readl(pcdev->base + CIM_TCNT));
}

/*
 *  Videobuf operations
 */
static int jz4780_videobuf_setup(struct videobuf_queue *vq, unsigned int *count,
			      unsigned int *size)
{
	struct soc_camera_device *icd = vq->priv_data;
	int bytes_per_line = soc_mbus_bytes_per_line(icd->user_width,
						icd->current_fmt->host_fmt);

	if (bytes_per_line < 0)
		return bytes_per_line;

	*size = bytes_per_line * icd->user_height;

	if (!*count)
		*count = 32;

	if (*size * *count > MAX_VIDEO_MEM * 1024 * 1024)
		*count = (MAX_VIDEO_MEM * 1024 * 1024) / *size;

	dprintk(7, "count=%d, size=%d\n", *count, *size);

	return 0;
}

static void free_buffer(struct videobuf_queue *vq, struct jz4780_buffer *buf)
{
	struct videobuf_buffer *vb = &buf->vb;

	BUG_ON(in_interrupt());

	dprintk(7, "%s (vb=0x%p) 0x%08lx %d\n", __func__,
		vb, vb->baddr, vb->bsize);

	/*
	 * This waits until this buffer is out of danger, i.e., until it is no
	 * longer in STATE_QUEUED or STATE_ACTIVE
	 */
	videobuf_waiton(vq, vb, 0, 0);
	videobuf_dma_contig_free(vq, vb);

	vb->state = VIDEOBUF_NEEDS_INIT;
}

static int jz4780_videobuf_prepare(struct videobuf_queue *vq,
		struct videobuf_buffer *vb, enum v4l2_field field)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;
	struct jz4780_buffer *buf = container_of(vb, struct jz4780_buffer, vb);
	int ret;
	int bytes_per_line = soc_mbus_bytes_per_line(icd->user_width,
						icd->current_fmt->host_fmt);

	if (bytes_per_line < 0)
		return bytes_per_line;

	dprintk(7, "%s (vb=0x%p) 0x%08lx %d\n", __func__,
		vb, vb->baddr, vb->bsize);

	/* Added list head initialization on alloc */
	WARN_ON(!list_empty(&vb->queue));

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
		if(pcdev->is_tlb_enabled == 0) {
			ret = videobuf_iolock(vq, vb, NULL);
			if (ret) {
				dprintk(3, "%s error!\n", __FUNCTION__);
				goto fail;
			}
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

static int jz4780_camera_setup_dma(struct jz4780_camera_dev *pcdev,
		unsigned char dev_num)
{
	struct videobuf_buffer *vbuf = &pcdev->active->vb;
	struct soc_camera_device *icd = pcdev->icd[dev_num];
	struct jz4780_camera_dma_desc *dma_desc;
	dma_addr_t dma_address;
	unsigned int i, regval;


	dma_desc = (struct jz4780_camera_dma_desc *) pcdev->desc_vaddr;

	if (unlikely(!pcdev->active)) {
		dprintk(3, "setup dma error with no active buffer\n");
		return -EFAULT;
	}

	if(pcdev->is_tlb_enabled == 0) {
		dma_address = videobuf_to_dma_contig(vbuf);

		/* disable tlb error interrupt */
		regval = readl(pcdev->base + CIM_IMR);
		regval |= CIM_IMR_TLBEM;
		writel(regval, pcdev->base + CIM_IMR);

		/* disable tlb */
		regval = readl(pcdev->base + CIM_TC);
		regval &= ~CIM_TC_ENA;
		writel(regval, pcdev->base + CIM_TC);
	} else {
		dma_address = icd->vb_vidq.bufs[0]->baddr;

		/* enable tlb error interrupt */
		regval = readl(pcdev->base + CIM_IMR);
		regval &= ~CIM_IMR_TLBEM;
		writel(regval, pcdev->base + CIM_IMR);

		/* enable tlb */
		regval = readl(pcdev->base + CIM_TC);
		regval |= CIM_TC_ENA;
		writel(regval, pcdev->base + CIM_TC);
	}

	if(!dma_address) {
		dprintk(3, "Failed to setup DMA address\n");
		return -ENOMEM;
	}

	regval = (unsigned int) (pcdev->dma_desc);
	writel(regval, pcdev->base + CIM_DA);

	for(i = 0; i < (pcdev->buf_cnt); i++) {
		dma_desc[i].id = i;
		dma_desc[i].buf = dma_address + (icd->user_width * icd->user_height << 1) * i;

		dprintk(7, "cim dma desc[i] address is: 0x%x\n", dma_desc[i].buf);

		if(icd->current_fmt->host_fmt->fourcc == V4L2_PIX_FMT_YUYV) {
			dma_desc[i].cmd = icd->sizeimage >> 2 |
					CIM_CMD_EOFINT | CIM_CMD_OFRCV;
		} else {
			dma_desc[i].cmd = (icd->sizeimage * 8 / 12) >> 2 |
					CIM_CMD_EOFINT | CIM_CMD_OFRCV;

			dma_desc[i].cb_len = (icd->user_width >> 1) *
					(icd->user_height >> 1) >> 2;

			dma_desc[i].cr_len = (icd->user_width >> 1) *
					(icd->user_height >> 1) >> 2;
		}

		if(icd->current_fmt->host_fmt->fourcc == V4L2_PIX_FMT_YUV420) {
			dma_desc[i].cb_frame = dma_desc[i].buf + icd->sizeimage * 8 / 12;
			dma_desc[i].cr_frame = dma_desc[i].cb_frame + (icd->sizeimage / 6);

		} else if(icd->current_fmt->host_fmt->fourcc == V4L2_PIX_FMT_JZ420B) {
			dma_desc[i].cb_frame = dma_desc[i].buf + icd->sizeimage;
			dma_desc[i].cr_frame = dma_desc[i].cb_frame + 64;
		}

		if(i == (pcdev->buf_cnt - 1)) {
			dma_desc[i].next = (dma_addr_t) (pcdev->dma_desc);
			break;
		} else {
			dma_desc[i].next = (dma_addr_t) (&pcdev->dma_desc[i + 1]);
		}

		dprintk(7, "cim dma desc[i] address is: 0x%x\n", dma_desc[i].buf);
	}

	return 0;
}

/* Called under spinlock_irqsave(&pcdev->lock, ...) */
static void jz4780_videobuf_queue(struct videobuf_queue *vq,
						struct videobuf_buffer *vb)
{

	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;
	struct jz4780_buffer *buf = container_of(vb, struct jz4780_buffer, vb);

	dprintk(7, "%s (vb=0x%p) 0x%08lx %d\n", __func__,
		vb, vb->baddr, vb->bsize);

	list_add_tail(&vb->queue, &pcdev->capture);

	vb->state = VIDEOBUF_ACTIVE;

	if ((!pcdev->active) && (buf != NULL)) {
		pcdev->active = buf;
		jz4780_camera_setup_dma(pcdev, icd->devnum);
	}
}

static void jz4780_videobuf_release(struct videobuf_queue *vq,
				 struct videobuf_buffer *vb) {

	struct jz4780_buffer *buf = container_of(vb, struct jz4780_buffer, vb);

	dprintk(7, "%s (vb=0x%p) 0x%08lx %d\n", __func__,
		vb, vb->baddr, vb->bsize);

	switch (vb->state) {
	case VIDEOBUF_ACTIVE:
		dprintk(7, "%s (active)\n", __func__);
		break;
	case VIDEOBUF_QUEUED:
		dprintk(7, "%s (queued)\n", __func__);
		break;
	case VIDEOBUF_PREPARED:
		dprintk(7, "%s (prepared)\n", __func__);
		break;
	default:
		dprintk(7, "%s (unknown)\n", __func__);
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

	videobuf_queue_dma_contig_init(q, &jz4780_videobuf_ops, icd->dev.parent,
				&pcdev->lock, V4L2_BUF_TYPE_VIDEO_CAPTURE,
				V4L2_FIELD_NONE,
				sizeof(struct jz4780_buffer), icd, &icd->video_lock);

}

static void jz4780_camera_activate(struct jz4780_camera_dev *pcdev)
{
	int ret = -1;
	dprintk(7, "Activate device\n");

	if(pcdev->clk) {
		ret = clk_enable(pcdev->clk);
	}

	if(pcdev->mclk) {
		ret = clk_set_rate(pcdev->mclk, pcdev->mclk_freq);
		ret = clk_enable(pcdev->mclk);
	}

	if(ret) {
		dprintk(3, "enable clock failed!\n");
	}
	msleep(10);
}

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

	dprintk(7, "Deactivate device\n");
}

/*
 * The following two functions absolutely depend on the fact, that
 * there can be one or two camera on JZ4780 CIM camera sensor interface
 */
static int jz4780_camera_add_device(struct soc_camera_device *icd)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;

	int icd_index = icd->devnum;

	if (pcdev->icd[icd_index])
		return -EBUSY;

	dprintk(6, "JZ4780 Camera driver attached to camera %d\n",
		 icd->devnum);

	pcdev->icd[icd_index] = icd;
	pcdev->is_first_start = 1;

	/* disable tlb when open camera every time */
	pcdev->is_tlb_enabled = 0;

	jz4780_camera_activate(pcdev);

	return 0;
}

static void jz4780_camera_remove_device(struct soc_camera_device *icd)
{

	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;

	int icd_index = icd->devnum;

	BUG_ON(icd != pcdev->icd[icd_index]);

	jz4780_camera_deactivate(pcdev);
	dprintk(6, "JZ4780 Camera driver detached from camera %d\n",
		 icd->devnum);

	pcdev->icd[icd_index] = NULL;
}

static int jz4780_camera_set_crop(struct soc_camera_device *icd,
			       struct v4l2_crop *a)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);

	return v4l2_subdev_call(sd, video, s_crop, a);
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

	/* Make choises, based on platform choice */

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
				| CIM_CFG_DSM_GCM | CIM_CFG_PACK_Y0UY1V;

	ctrl_reg |= CIM_CTRL_DMA_SYNC | CIM_CTRL_FRC_1;

	ctrl2_reg |= CIM_CTRL2_APM | CIM_CTRL2_EME | CIM_CTRL2_OPE |
				(1 << CIM_CTRL2_OPG_BIT) | CIM_CTRL2_FSC | CIM_CTRL2_ARIF;
	fs_reg = (icd->user_width -1) << CIM_FS_FHS_BIT | (icd->user_height -1)
				<< CIM_FS_FVS_BIT | 1<< CIM_FS_BPP_BIT;

	if((pixfmt == V4L2_PIX_FMT_YUV420) ||
			(pixfmt == V4L2_PIX_FMT_JZ420B)) {
		ctrl2_reg |= CIM_CTRL2_CSC_YUV420;
		cfg_reg |= CIM_CFG_SEP | CIM_CFG_ORDER_YUYV;
	}

	if(pixfmt == V4L2_PIX_FMT_JZ420B)
		ctrl_reg |= CIM_CTRL_MBEN;

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

	return 0;
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
		dprintk(4, "Format %x not found\n",
			 pix->pixelformat);
		return -EINVAL;
	}

	buswidth = xlate->host_fmt->bits_per_sample;
	if (buswidth > 8) {
		dprintk(4, "bits-per-sample %d for format %x unsupported\n",
			 buswidth, pix->pixelformat);
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
		dprintk(4, "Format %x not found\n",
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

static int jz4780_camera_alloc_desc(struct jz4780_camera_dev *pcdev,
		struct v4l2_requestbuffers *p) {

	pcdev->buf_cnt = p->count;
	pcdev->desc_vaddr = dma_alloc_coherent(pcdev->soc_host.v4l2_dev.dev,
				sizeof(*pcdev->dma_desc) * pcdev->buf_cnt,
				(dma_addr_t *)&pcdev->dma_desc, GFP_KERNEL);

	if (!pcdev->dma_desc)
		return -ENOMEM;

	return 0;
}


static int jz4780_camera_reqbufs(struct soc_camera_device *icd,
			      struct v4l2_requestbuffers *p)
{
	int i;
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;
	/*
	 * This is for locking debugging only. I removed spinlocks and now I
	 * check whether .prepare is ever called on a linked buffer, or whether
	 * a dma IRQ can occur for an in-work or unlinked buffer. Until now
	 * it hadn't triggered
	 */
	for (i = 0; i < p->count; i++) {
		struct jz4780_buffer *buf = container_of(icd->vb_vidq.bufs[i],
						      struct jz4780_buffer, vb);
		buf->inwork = 0;
		INIT_LIST_HEAD(&buf->vb.queue);
	}

	if(jz4780_camera_alloc_desc(pcdev, p))
		return -ENOMEM;

	return 0;
}

unsigned long jiff_temp;
static unsigned int jz4780_camera_poll(struct file *file, poll_table *pt)
{
	struct soc_camera_device *icd = file->private_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->dev.parent);
	struct jz4780_camera_dev *pcdev = ici->priv;
	struct jz4780_buffer *buf;
	unsigned long temp = 0;
	buf = list_entry(icd->vb_vidq.stream.next, struct jz4780_buffer,
			 vb.stream);

	if(pcdev->is_first_start) {
		writel(0, pcdev->base + CIM_STATE);

		/* enable dma */
		temp = readl(pcdev->base + CIM_CTRL);
		temp |= CIM_CTRL_DMA_EN;
		writel(temp, pcdev->base + CIM_CTRL);

		/* clear rx fifo */
		temp = readl(pcdev->base + CIM_CTRL);
		temp |= CIM_CTRL_RXF_RST;
		writel(temp, pcdev->base + CIM_CTRL);

		temp = readl(pcdev->base + CIM_CTRL);
		temp &= ~CIM_CTRL_RXF_RST;
		writel(temp, pcdev->base + CIM_CTRL);

		/* enable cim */
		temp = readl(pcdev->base + CIM_CTRL);
		temp |= CIM_CTRL_ENA;
		writel(temp, pcdev->base + CIM_CTRL);
		cim_dump_reg(pcdev);
		pcdev->is_first_start = 0;
	}

	jiff_temp = jiffies;
	poll_wait(file, &buf->vb.done, pt);

	if (buf->vb.state == VIDEOBUF_DONE ||
	    buf->vb.state == VIDEOBUF_ERROR) {
		//printk("buf->vb.baddr = %x, dma_address = %x\n", buf->vb.baddr, videobuf_to_dma_contig(&buf->vb));
		return POLLIN | POLLRDNORM;
	}

	return 0;
}

static int jz4780_camera_querycap(struct soc_camera_host *ici,
			       struct v4l2_capability *cap)
{
	/* cap->name is set by the friendly caller:-> */
	strlcpy(cap->card, "JZ4780-Camera", sizeof(cap->card));
	cap->version = VERSION_CODE;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

	return 0;
}

static int jz4780_camera_set_tlb_base(struct soc_camera_host *ici,
			       unsigned int *tlb_base)
{
	unsigned int regval;
	struct jz4780_camera_dev *pcdev = ici->priv;
	pcdev->is_tlb_enabled = 1;

	if(*tlb_base & 0x3) {//bit[0:1] must be 0.double word aligned
		dprintk(3, "cim tlb base is not valid address\n");
		return -1;
	}

	/* reset tlb */
	regval = readl(pcdev->base + CIM_TC);
	regval |= CIM_TC_RST;
	writel(regval, pcdev->base + CIM_TC);

	regval = readl(pcdev->base + CIM_TC);
	regval &= ~CIM_TC_RST;
	writel(regval, pcdev->base + CIM_TC);

	/* set tlb base */
	regval = readl(pcdev->base + CIM_TC);
	regval |= *tlb_base;
	writel(regval, pcdev->base + CIM_TC);

	return 0;
}

static void jz4780_camera_wakeup(struct jz4780_camera_dev *pcdev,
			      struct videobuf_buffer *vb,
			      struct jz4780_buffer *buf) {
	// _init is used to debug races, see comment in jz4780_camera_reqbufs()
	list_del_init(&vb->queue);
	vb->state = VIDEOBUF_DONE;
	do_gettimeofday(&vb->ts);
	vb->field_count++;
	wake_up(&vb->done);
	dprintk(7, "after wake up cost %d ms\n", jiffies_to_msecs(jiffies - jiff_temp));

	if (list_empty(&pcdev->capture)) {
		pcdev->active = NULL;
		return;
	}

	pcdev->active = list_entry(pcdev->capture.next,
				   struct jz4780_buffer, vb.queue);
}


static irqreturn_t jz4780_camera_irq_handler(int irq, void *data) {

	struct jz4780_camera_dev *pcdev = (struct jz4780_camera_dev *)data;
	struct device *dev = NULL;
	struct jz4780_buffer *buf;
	struct videobuf_buffer *vb;
	unsigned long status = 0, temp = 0;
	unsigned long flags = 0;
	unsigned int cam_dev_index;

	for(cam_dev_index = 0; cam_dev_index < 2; cam_dev_index++) {
		if(pcdev->icd[cam_dev_index]) {
			dev = pcdev->icd[cam_dev_index]->dev.parent;
			break;
		}
	}

	if(cam_dev_index == MAX_SOC_CAM_NUM)
		return IRQ_HANDLED;

	/* read interrupt status register */
	status = readl(pcdev->base + CIM_STATE);

	if (status & CIM_STATE_RXF_OF) {
		dprintk(3, "Rx FIFO OverFlow interrupt!\n");

		/* clear rx overflow interrupt */
		temp = readl(pcdev->base + CIM_STATE);
		temp &= ~CIM_STATE_RXF_OF;
		writel(temp, pcdev->base + CIM_STATE);

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

		/* clear status register */
		writel(0, pcdev->base + CIM_STATE);

		/* enable cim */
		temp = readl(pcdev->base + CIM_CTRL);
		temp |= CIM_CTRL_ENA;
		writel(temp, pcdev->base + CIM_CTRL);

		return IRQ_HANDLED;
	}

	if(status & CIM_STATE_DMA_EOF) {
		spin_lock_irqsave(&pcdev->lock, flags);

		if (unlikely(!pcdev->active)) {
			dprintk(3, "DMA End IRQ with no active buffer\n");
			/* clear dma interrupt status */
			temp = readl(pcdev->base + CIM_STATE);
			temp &= ~CIM_STATE_DMA_EOF;
			writel(temp, pcdev->base + CIM_STATE);

			spin_unlock_irqrestore(&pcdev->lock, flags);
			return IRQ_HANDLED;
		}

		vb = &pcdev->active->vb;
		buf = container_of(vb, struct jz4780_buffer, vb);

		WARN_ON(buf->inwork || list_empty(&vb->queue));
		dprintk(7, "%s (vb=0x%p) 0x%08lx %d\n", __func__,
			vb, vb->baddr, vb->bsize);

		spin_unlock_irqrestore(&pcdev->lock, flags);
		jz4780_camera_wakeup(pcdev, vb, buf);

		/* clear dma interrupt status */
		temp = readl(pcdev->base + CIM_STATE);
		temp &= ~CIM_STATE_DMA_EOF;
		writel(temp, pcdev->base + CIM_STATE);

		return IRQ_HANDLED;
	}
	return IRQ_HANDLED;
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
	.set_tlb_base = jz4780_camera_set_tlb_base,
	.num_controls = 0,
};


/* initial camera sensor reset and power gpio */
static int camera_sensor_gpio_init(struct platform_device *pdev) {
	int ret;
	struct jz4780_camera_pdata *pcdev = pdev->dev.platform_data;

	if(!pcdev) {
		dprintk(3, "have not platform data\n");
		return -1;
	}

	if(pcdev->cam_sensor_pdata[0].gpio_rst) {
		ret = gpio_request(pcdev->cam_sensor_pdata[0].gpio_rst,
				"sensor_rst0");
		if(ret) {
			dprintk(3, "request reset gpio fail\n");
			return ret;
		}
		gpio_direction_output(pcdev->cam_sensor_pdata[0].gpio_rst, 0);
	}

	if(pcdev->cam_sensor_pdata[0].gpio_power) {
		ret = gpio_request(pcdev->cam_sensor_pdata[0].gpio_power,
				"sensor_en0");
		if(ret) {
			dprintk(3, "request sensor en gpio fail\n");
			gpio_free(pcdev->cam_sensor_pdata[0].gpio_rst);
			return ret;
		}
		gpio_direction_output(pcdev->cam_sensor_pdata[0].gpio_power, 1);
	}

	if(pcdev->cam_sensor_pdata[1].gpio_rst) {
		if(pcdev->cam_sensor_pdata[0].gpio_rst !=
				pcdev->cam_sensor_pdata[1].gpio_rst) {
			ret = gpio_request(pcdev->cam_sensor_pdata[1].gpio_rst,
					"sensor_rst1");
			if(ret) {
				dprintk(3, "request reset gpio fail\n");
				return ret;
			}
			gpio_direction_output(pcdev->cam_sensor_pdata[1].gpio_rst, 0);
		}
	}

	if(pcdev->cam_sensor_pdata[1].gpio_power) {
	ret = gpio_request(pcdev->cam_sensor_pdata[1].gpio_power, "sensor_en1");
		if(ret) {
			dprintk(3, "request sensor en gpio fail\n");
			if(pcdev->cam_sensor_pdata[0].gpio_rst !=
					pcdev->cam_sensor_pdata[1].gpio_rst) {
				gpio_free(pcdev->cam_sensor_pdata[1].gpio_rst);
			}
			return ret;
		}
		gpio_direction_output(pcdev->cam_sensor_pdata[1].gpio_power, 1);
	}

	return 0;
}

static int __init jz4780_camera_probe(struct platform_device *pdev)
{
	struct jz4780_camera_dev *pcdev;
	struct resource *res;
	void __iomem *base;
	unsigned int irq;
	int err = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);
	if (!res || (int)irq <= 0) {
		err = -ENODEV;
		goto exit;
	}

	pcdev = kzalloc(sizeof(*pcdev), GFP_KERNEL);
	if (!pcdev) {
		dprintk(3, "Could not allocate pcdev\n");
		err = -ENOMEM;
		goto exit_put_clk_cgucim;
	}

	pcdev->clk = clk_get(&pdev->dev, "cim");
	if (IS_ERR(pcdev->clk)) {
		err = PTR_ERR(pcdev->clk);
		goto exit;
	}

	pcdev->mclk = clk_get(&pdev->dev, "cgu_cimmclk");
	if (IS_ERR(pcdev->mclk)) {
		err = PTR_ERR(pcdev->mclk);
		goto exit_put_clk_cim;
	}

	//pcdev->soc_host.regul = regulator_get(&pdev->dev, "vcim");
	pcdev->soc_host.regul = regulator_get(&pdev->dev, "vcim_2_8");
	if(IS_ERR(pcdev->soc_host.regul)){
		dprintk(3, "get regulator fail!\n");
		err = -ENODEV;
		goto exit_put_clk_cim;
	}

	pcdev->res = res;

	pcdev->pdata = pdev->dev.platform_data;

	if (pcdev->pdata)
		pcdev->mclk_freq = pcdev->pdata->mclk_10khz * 10000;

	if (!pcdev->mclk_freq) {
		dprintk(4, "mclk_10khz == 0! Please, fix your platform data."
			 "Using default 24MHz\n");
		pcdev->mclk_freq = 24000000;
	}

	INIT_LIST_HEAD(&pcdev->capture);
	spin_lock_init(&pcdev->lock);

	/*
	 * Request the regions.
	 */
	if (!request_mem_region(res->start, resource_size(res), DRIVER_NAME)) {
		err = -EBUSY;
		goto exit_kfree;
	}

	base = ioremap(res->start, resource_size(res));
	if (!base) {
		err = -ENOMEM;
		goto exit_release;
	}
	pcdev->irq = irq;
	pcdev->base = base;

	/* request irq */
	err = request_irq(pcdev->irq, jz4780_camera_irq_handler, IRQF_DISABLED,
						dev_name(&pdev->dev), pcdev);
	if(err) {
		dprintk(3, "request irq failed!\n");
		goto exit_iounmap;
	}

	/* request sensor reset and power gpio */
	err = camera_sensor_gpio_init(pdev);
	if(err) {
		goto exit_free_irq;
	}

	pcdev->soc_host.drv_name	= DRIVER_NAME;
	pcdev->soc_host.ops		= &jz4780_soc_camera_host_ops;
	pcdev->soc_host.priv		= pcdev;
	pcdev->soc_host.v4l2_dev.dev	= &pdev->dev;
	pcdev->soc_host.nr		= pdev->id;
	err = soc_camera_host_register(&pcdev->soc_host);
	if (err)
		goto exit_free_irq;

	dprintk(6, "JZ4780 Camera driver loaded\n");
	return 0;

exit_free_irq:
	free_irq(pcdev->irq, pcdev);
exit_iounmap:
	iounmap(base);
exit_release:
	release_mem_region(res->start, resource_size(res));
exit_kfree:
	kfree(pcdev);
exit_put_clk_cgucim:
	clk_put(pcdev->mclk);
exit_put_clk_cim:
	clk_put(pcdev->clk);
exit:
	return err;
}

static int __exit jz4780_camera_remove(struct platform_device *pdev)
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

	dprintk(6, "JZ4780 Camera driver unloaded\n");

	return 0;
}

static struct platform_driver jz4780_camera_driver = {
	.driver 	= {
		.name	= DRIVER_NAME,
	},
	.remove		= __exit_p(jz4780_camera_remove),
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
MODULE_AUTHOR("FeiYe <feiye@ingenic.cn>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
