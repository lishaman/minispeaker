
/*
 * I2C adapter for the INGENIC I2C bus access.
 *
 * Copyright (C) 2006 - 2009 Ingenic Semiconductor Inc.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <asm/mipsregs.h>
#include <asm/atomic.h>

#include <mach/jzdma.h>

#define	I2C_CTRL		(0x00)
#define	I2C_TAR     		(0x04)
#define	I2C_SAR     		(0x08)
#define	I2C_DC      		(0x10)
#define	I2C_SHCNT		(0x14)
#define	I2C_SLCNT		(0x18)
#define	I2C_FHCNT		(0x1C)
#define	I2C_FLCNT		(0x20)
#define	I2C_INTST		(0x2C)
#define	I2C_INTM		(0x30)
#define I2C_RXTL		(0x38)
#define I2C_TXTL		(0x3c)
#define	I2C_CINTR		(0x40)
#define	I2C_CRXUF		(0x44)
#define	I2C_CRXOF		(0x48)
#define	I2C_CTXOF		(0x4C)
#define	I2C_CRXREQ		(0x50)
#define	I2C_CTXABRT		(0x54)
#define	I2C_CRXDONE		(0x58)
#define	I2C_CACT		(0x5C)
#define	I2C_CSTP		(0x60)
#define	I2C_CSTT		(0x64)
#define	I2C_CGC    		(0x68)
#define	I2C_ENB     		(0x6C)
#define	I2C_STA     		(0x70)
#define I2C_SDAHD		(0x7C)
#define	I2C_TXABRT		(0x80)
#define I2C_DMACR            	(0x88)
#define I2C_DMATDLR          	(0x8c)
#define I2C_DMARDLR     	(0x90)
#define	I2C_SDASU		(0x94)
#define	I2C_ACKGC		(0x98)
#define	I2C_ENSTA		(0x9C)
#define I2C_FLT			(0xA0)

/* I2C Control Register (I2C_CTRL) */
#define I2C_CTRL_SLVDIS		(1 << 6) /* after reset slave is disabled*/
#define I2C_CTRL_REST		(1 << 5)
#define I2C_CTRL_MATP		(1 << 4) /* 1: 10bit address 0: 7bit addressing*/
#define I2C_CTRL_SATP		(1 << 3) /* 1: 10bit address 0: 7bit address*/
#define I2C_CTRL_SPDF		(2 << 1) /* fast mode 400kbps */
#define I2C_CTRL_SPDS		(1 << 1) /* standard mode 100kbps */
#define I2C_CTRL_MD		(1 << 0) /* master enabled*/

/* I2C Status Register (I2C_STA) */
#define I2C_STA_SLVACT		(1 << 6) /* Slave FSM is not in IDLE state */
#define I2C_STA_MSTACT		(1 << 5) /* Master FSM is not in IDLE state */
#define I2C_STA_RFF		(1 << 4) /* RFIFO if full */
#define I2C_STA_RFNE		(1 << 3) /* RFIFO is not empty */
#define I2C_STA_TFE		(1 << 2) /* TFIFO is empty */
#define I2C_STA_TFNF		(1 << 1) /* TFIFO is not full  */
#define I2C_STA_ACT		(1 << 0) /* I2C Activity Status */

/* I2C Transmit Abort Status Register (I2C_TXABRT) */
char *abrt_src_dma[] = {
	"I2C_TXABRT_ABRT_7B_ADDR_NOACK",
	"I2C_TXABRT_ABRT_10ADDR1_NOACK",
	"I2C_TXABRT_ABRT_10ADDR2_NOACK",
	"I2C_TXABRT_ABRT_XDATA_NOACK",
	"I2C_TXABRT_ABRT_GCALL_NOACK",
	"I2C_TXABRT_ABRT_GCALL_READ",
	"I2C_TXABRT_ABRT_HS_ACKD",
	"I2C_TXABRT_SBYTE_ACKDET",
	"I2C_TXABRT_ABRT_HS_NORSTRT",
	"I2C_TXABRT_SBYTE_NORSTRT",
	"I2C_TXABRT_ABRT_10B_RD_NORSTRT",
	"I2C_TXABRT_ABRT_MASTER_DIS",
	"I2C_TXABRT_ARB_LOST",
	"I2C_TXABRT_SLVFLUSH_TXFIFO",
	"I2C_TXABRT_SLV_ARBLOST",
	"I2C_TXABRT_SLVRD_INTX",
};

/* i2c interrupt status (I2C_INTST) */
#define I2C_INTST_IGC                   (1 << 11)
#define I2C_INTST_ISTT                  (1 << 10)
#define I2C_INTST_ISTP                  (1 << 9)
#define I2C_INTST_IACT                  (1 << 8)
#define I2C_INTST_RXDN                  (1 << 7)
#define I2C_INTST_TXABT                 (1 << 6)
#define I2C_INTST_RDREQ                 (1 << 5)
#define I2C_INTST_TXEMP                 (1 << 4)
#define I2C_INTST_TXOF                  (1 << 3)
#define I2C_INTST_RXFL                  (1 << 2)
#define I2C_INTST_RXOF                  (1 << 1)
#define I2C_INTST_RXUF                  (1 << 0)

/* i2c interrupt mask status (I2C_INTM) */
#define I2C_INTM_MIGC			(1 << 11)
#define I2C_INTM_MISTT			(1 << 10)
#define I2C_INTM_MISTP			(1 << 9)
#define I2C_INTM_MIACT			(1 << 8)
#define I2C_INTM_MRXDN			(1 << 7)
#define I2C_INTM_MTXABT			(1 << 6)
#define I2C_INTM_MRDREQ			(1 << 5)
#define I2C_INTM_MTXEMP			(1 << 4)
#define I2C_INTM_MTXOF			(1 << 3)
#define I2C_INTM_MRXFL			(1 << 2)
#define I2C_INTM_MRXOF			(1 << 1)
#define I2C_INTM_MRXUF			(1 << 0)

#define I2C_DC_REST			(1 << 10)
#define I2C_DC_STP			(1 << 9)

#define I2C_DC_READ    			(1 << 8)
#define I2C_DC_WRITE   			(0 << 8)

#define I2C_ENB_I2CENB			(1 << 0) /* Enable the i2c */

/* I2C standard mode high count register(I2CSHCNT) */
#define I2CSHCNT_ADJUST(n)      (((n) - 8) < 6 ? 6 : ((n) - 8))
/* I2C standard mode low count register(I2CSLCNT) */
#define I2CSLCNT_ADJUST(n)      (((n) - 1) < 8 ? 8 : ((n) - 1))
/* I2C fast mode high count register(I2CFHCNT) */
#define I2CFHCNT_ADJUST(n)      (((n) - 8) < 6 ? 6 : ((n) - 8))
/* I2C fast mode low count register(I2CFLCNT) */
#define I2CFLCNT_ADJUST(n)      (((n) - 1) < 8 ? 8 : ((n) - 1))

/*#define WRITE_CMD_BY_CPU*/
/*#define USE_ONE_DESC*/
#define USE_DESC_LINK
#define I2C_FIFO_LEN 64
#define TIMEOUT	0xf
struct i2c_sg_data {
	struct scatterlist *sg;
	int sg_len;
	int flag;
};
struct jz_i2c {
	void __iomem *iomem;
	int irq;
	struct clk *clk;
	struct i2c_adapter adap;
	unsigned short *rbuf;
	unsigned short *wbuf;
	int buf_len;
	int r_len;
	int w_len;
	int is_write;
	int speed;

	struct completion r_complete;
	struct completion w_complete;
	struct completion dma_r_complete;
	struct completion dma_w_complete;
	struct i2c_sg_data *w_data;
	struct i2c_sg_data *r_data;

	struct dma_chan *rx_chan;
	struct dma_chan *tx_chan;
	struct dma_async_tx_descriptor  *tx_desc;
	struct dma_slave_config rx_dma_config;
	struct dma_slave_config tx_dma_config;
	enum jzdma_type dma_type;
};

/*#define I2C_DEBUG*/
/*static int h_i2c_test = 1;*/
static atomic_t complete_count;
static atomic_t complete_r_count;
static atomic_t complete_w_count;

static inline unsigned short i2c_readl(struct jz_i2c *i2c,unsigned short offset);
#ifdef I2C_DEBUG
void jzdma_dump(struct dma_chan *chan);

#define PRINT_REG_WITH_ID(reg_name, id) \
	dev_info(&(i2c->adap.dev),"--"#reg_name "    	0x%08x\n",i2c_readl(id,reg_name))
static void jz_dump_i2c_regs(struct jz_i2c *i2c) {
	struct jz_i2c *i2c_id = i2c;

	PRINT_REG_WITH_ID(I2C_CTRL, i2c_id);
	PRINT_REG_WITH_ID(I2C_TAR, i2c_id);
	PRINT_REG_WITH_ID(I2C_STA, i2c_id);
	PRINT_REG_WITH_ID(I2C_DMATDLR, i2c_id);
	PRINT_REG_WITH_ID(I2C_DMARDLR, i2c_id);
	PRINT_REG_WITH_ID(I2C_DMACR, i2c_id);
	return;

	PRINT_REG_WITH_ID(I2C_SAR, i2c_id);
//	PRINT_REG_WITH_ID(I2C_DC, i2c_id);
	PRINT_REG_WITH_ID(I2C_SHCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_SLCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_FHCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_FLCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_INTST, i2c_id);
	PRINT_REG_WITH_ID(I2C_INTM, i2c_id);
	PRINT_REG_WITH_ID(I2C_RXTL, i2c_id);
	PRINT_REG_WITH_ID(I2C_TXTL, i2c_id);
	PRINT_REG_WITH_ID(I2C_CINTR, i2c_id);
	PRINT_REG_WITH_ID(I2C_CRXUF, i2c_id);
	PRINT_REG_WITH_ID(I2C_CRXOF, i2c_id);
	PRINT_REG_WITH_ID(I2C_CTXOF, i2c_id);
	PRINT_REG_WITH_ID(I2C_CRXREQ, i2c_id);
	PRINT_REG_WITH_ID(I2C_CTXABRT, i2c_id);
	PRINT_REG_WITH_ID(I2C_CRXDONE, i2c_id);
	PRINT_REG_WITH_ID(I2C_CACT, i2c_id);
	PRINT_REG_WITH_ID(I2C_CSTP, i2c_id);
	PRINT_REG_WITH_ID(I2C_CSTT, i2c_id);
	PRINT_REG_WITH_ID(I2C_CGC, i2c_id);
	PRINT_REG_WITH_ID(I2C_ENB, i2c_id);
/*debug trans & recive fifo count */
	PRINT_REG_WITH_ID(0x74, i2c_id);
	PRINT_REG_WITH_ID(0x78, i2c_id);

	PRINT_REG_WITH_ID(I2C_TXABRT, i2c_id);
	PRINT_REG_WITH_ID(I2C_SDASU, i2c_id);
	PRINT_REG_WITH_ID(I2C_ACKGC, i2c_id);
	PRINT_REG_WITH_ID(I2C_ENSTA, i2c_id);
	PRINT_REG_WITH_ID(I2C_SDAHD, i2c_id);
}
#endif
static inline unsigned short i2c_readl(struct jz_i2c *i2c, unsigned short offset)
{
	//return (*(volatile unsigned short *)(i2c->iomem + offset));
	return readl(i2c->iomem + offset);
}

static inline void i2c_writel(struct jz_i2c *i2c, unsigned short  offset, unsigned short value)
{
	//(*(volatile unsigned short *) (i2c->iomem + offset)) = (value);
	writel(value,i2c->iomem + offset);
}

static int jz_i2c_disable(struct jz_i2c *i2c)
{
	int timeout = TIMEOUT;
	i2c_writel(i2c,I2C_ENB,0);
	while((i2c_readl(i2c,I2C_ENSTA) & I2C_ENB_I2CENB) && (--timeout > 0))msleep(1);

	if (timeout)
		return 0;

	printk("enable i2c%d failed\n", i2c->adap.nr);
	return -ETIMEDOUT;
}

static int jz_i2c_enable(struct jz_i2c *i2c)
{
	int timeout = TIMEOUT;

	i2c_writel(i2c,I2C_ENB,1);
	while(!(i2c_readl(i2c,I2C_ENSTA) & I2C_ENB_I2CENB) && (--timeout > 0))msleep(1);

	if (timeout)
		return 0;

	printk("enable i2c%d failed\n", i2c->adap.nr);
	return -ETIMEDOUT;
}

static void jz_i2c_reset(struct jz_i2c *i2c)
{
	unsigned short tmp;

	i2c_readl(i2c,I2C_CTXABRT);

	tmp = i2c_readl(i2c, I2C_CTRL);
	tmp &= ~(1<<0);
	i2c_writel(i2c, I2C_CTRL, tmp);
	udelay(10);
	tmp |= (1<<0);
	i2c_writel(i2c, I2C_CTRL, tmp);

	jz_i2c_disable(i2c);
	udelay(10);
	jz_i2c_enable(i2c);
}

static irqreturn_t jz_i2c_irq(int irqno, void *dev_id)
{
	unsigned short tmp,intst;
	int i = 0;
	struct jz_i2c *i2c = dev_id;

	intst = i2c_readl(i2c, I2C_INTST);
	if(intst & I2C_INTST_RXFL) {
		tmp = i2c_readl(i2c, I2C_INTM);
		tmp &= ~(I2C_INTM_MRXFL);
		i2c_writel(i2c, I2C_INTM, tmp);
		complete(&i2c->r_complete);
	}
	if(intst & I2C_INTST_TXEMP) {
		tmp = i2c_readl(i2c, I2C_INTM);
		tmp &= ~I2C_INTM_MTXEMP;
		i2c_writel(i2c, I2C_INTM, tmp);
		while(i2c->w_len) {
			if (i2c->w_len == 1)
				i2c->wbuf[i] |= I2C_DC_STP;
			if (i2c_readl(i2c, I2C_STA) & I2C_STA_TFNF) {
				i2c_writel(i2c, I2C_DC, i2c->wbuf[i++]);
				i2c->w_len -= 1;
			} else
				break;
			if (i2c->w_len == 0)
				complete(&i2c->w_complete);
		}
		if (i2c->w_len > 0) {
				tmp = i2c_readl(i2c, I2C_INTM);
				tmp |= I2C_INTM_MTXEMP;
				i2c_writel(i2c, I2C_INTM, tmp);
		}
	}
	if(intst & I2C_INTST_TXABT) {
		tmp = i2c_readl(i2c, I2C_INTM);
		tmp &= ~I2C_INTM_MTXABT;
		i2c_writel(i2c, I2C_INTM, tmp);
		complete(&i2c->r_complete);
		complete(&i2c->w_complete);
	}
	if(intst & I2C_INTST_ISTP) {
		tmp = i2c_readl(i2c, I2C_INTM);
		tmp &= ~I2C_INTM_MISTP;
		i2c_writel(i2c, I2C_INTM, tmp);
		i2c_readl(i2c, I2C_CSTP);
	}
	if(intst & I2C_INTST_ISTT) {
		tmp = i2c_readl(i2c, I2C_INTM);
		tmp &= ~I2C_INTM_MISTT;
		i2c_writel(i2c, I2C_INTM, tmp);

		tmp = i2c_readl(i2c, I2C_CSTT);
	}

	return IRQ_HANDLED;
}

static void jz_txabrt(struct jz_i2c *i2c, int src)
{
	int i;
	for(i=0; i<16; i++) {
		if(src & (0x1 << i))
			dev_err(&(i2c->adap.dev),"--I2C TXABRT[%d]=%s\n",i,abrt_src_dma[i]);
	}

#ifdef I2C_DEBUG
	jz_dump_i2c_regs(i2c);
#endif
}

static void jz_i2c_complete(void *arg)
{
	struct completion *comp;
	if(arg != NULL){
		comp = (struct completion *)arg;;
		complete(comp);
#ifdef I2C_DEBUG
		atomic_inc(&complete_count);
#endif
	}
}

static inline int init_dma_write(struct jz_i2c *i2c, unsigned short *buf, int len)
{
	struct dma_async_tx_descriptor *desc = NULL;
	dma_addr_t dma_src;
	dma_addr_t dma_dst;
	unsigned char wr_flag = 0x0;
	unsigned char burst_len[] = {32,16,4,2,1};
	unsigned char burst_val = 32;
	int div, offset = 0, length = 2 * len;

	dma_cache_wback((unsigned long)buf, len * 2);
	/* use dma */

#if 1
	/* use a desc link*/
	i2c->tx_dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	i2c->tx_dma_config.dst_maxburst = 32;
	dmaengine_slave_config(i2c->tx_chan, &i2c->tx_dma_config);

	while(length) {
		div = length / burst_len[0];
		offset = len - length/2;
		if (!div) {
			burst_val = length;
			length = 0;
			wr_flag = 1;
		} else {
			burst_val = div;
			length =  length % burst_len[0];
			wr_flag = 0;
		}

		dma_src = CPHYSADDR((unsigned short*)buf + offset);
		dma_dst = CPHYSADDR(i2c->iomem + I2C_DC);
		desc = i2c->tx_chan->device->device_add_desc(i2c->tx_chan,
				dma_src, dma_dst, burst_val, DMA_TO_DEVICE,wr_flag);

		if(desc == NULL ){
			dev_err(&(i2c->adap.dev),"--I2C dma write get desc is NULL\n");
			return -EBADR;
		}
	}
#else
	/* use one desc */
	i2c->tx_dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	i2c->tx_dma_config.dst_maxburst = 2;
	dmaengine_slave_config(i2c->tx_chan, &i2c->tx_dma_config);

	dma_src = CPHYSADDR((unsigned short*)buf);
	dma_dst = CPHYSADDR(i2c->iomem + I2C_DC);
	desc = i2c->tx_chan->device->device_add_desc(i2c->tx_chan,
			dma_src, dma_dst, length, DMA_TO_DEVICE,1);

	if(desc == NULL ){
		dev_err(&(i2c->adap.dev),"--I2C dma write get desc is NULL\n");
		return -EBADR;
	}

#endif
	desc->callback = jz_i2c_complete;
	desc->callback_param = &i2c->dma_w_complete;

	/* tx_submit */
	dmaengine_submit(desc);
	dma_async_issue_pending(i2c->tx_chan);

	return 0;
}

#ifdef USE_ONE_DESC
static inline int init_dma_read(struct jz_i2c *i2c, unsigned char *dma_map_trigval, unsigned char *dma_map_buf, int len, int cnt)
{
	struct dma_async_tx_descriptor *desc = NULL;
	dma_addr_t dma_src;
	dma_addr_t dma_dst;

	dma_cache_wback_inv((unsigned long)buf, len * sizeof(unsigned char));

	/* use one desc */
	i2c->rx_dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	i2c->rx_dma_config.src_maxburst = 1;
	i2c->rx_dma_config.direction = DMA_FROM_DEVICE;
	dmaengine_slave_config(i2c->rx_chan, &i2c->rx_dma_config);

	dma_src = CPHYSADDR(i2c->iomem + I2C_DC);
	dma_dst = CPHYSADDR((unsigned char*)dma_map_buf);
	desc = i2c->rx_chan->device->device_add_desc(i2c->rx_chan,
			dma_src, dma_dst, len, DMA_FROM_DEVICE, 0);
	if (desc == NULL) {
		dev_err(&(i2c->adap.dev),"--I2C dma read desc apply is NULL");
		return -EBADR;
	}

	/*printk("----> dma rd desc add successed! \n");*/
	desc->callback = jz_i2c_complete;
	desc->callback_param = &i2c->dma_r_complete;
	dmaengine_submit(desc);
	dma_async_issue_pending(i2c->rx_chan);

	return 0;
}

#elif (defined(USE_DESC_LINK))
static inline int init_dma_read(struct jz_i2c *i2c, unsigned char *dma_map_trigval,
		unsigned char *dma_map_buf, int len, int cnt)
{
	int  length = len;
	int div = 0, offset=0;
	int i = 0;
	/*int count = 0;*/
	struct dma_async_tx_descriptor *desc = NULL;
	unsigned char burst_len[] = {32,16,4,2,1};
	/*unsigned short burst_val = 32;*/
	unsigned char rd_flag = 0x0;
	dma_addr_t dma_src;
	dma_addr_t dma_dst;

	dma_cache_wback_inv((unsigned long)dma_map_buf, length * sizeof(unsigned char));
	dma_cache_wback((unsigned long)dma_map_trigval, 5 * sizeof(unsigned char));

	/* use a desc link*/
	while(length) {
		div = length / burst_len[i];
		offset = len - length;
		if (!div) {
			i++;
			continue;
		} else {
			length = len % burst_len[i];
			rd_flag = 0x0;
		}
		dma_map_trigval[i] = (unsigned char)burst_len[i] - 1;
		i2c->rx_dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		i2c->rx_dma_config.dst_maxburst = 1;
		i2c->rx_dma_config.direction = DMA_TO_DEVICE;
		dmaengine_slave_config(i2c->rx_chan, &i2c->rx_dma_config);

		dma_src = CPHYSADDR((unsigned char*)dma_map_trigval + i);
		dma_dst = CPHYSADDR(i2c->iomem + I2C_DMARDLR);
		desc = i2c->rx_chan->device->device_add_desc(i2c->rx_chan,
				dma_src, dma_dst, 1, DMA_TO_DEVICE, 1);
		if (desc == NULL) {
			dev_err(&(i2c->adap.dev),"--I2C dma write rd_tri_len desc apply is NULL");
			return -EBADR;
		}

		i2c->rx_dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		i2c->rx_dma_config.src_maxburst = burst_len[i];
		i2c->rx_dma_config.direction = DMA_FROM_DEVICE;
		dmaengine_slave_config(i2c->rx_chan, &i2c->rx_dma_config);

		dma_src = CPHYSADDR(i2c->iomem + I2C_DC);
		dma_dst = CPHYSADDR((unsigned char*)dma_map_buf + offset);
		desc = i2c->rx_chan->device->device_add_desc(i2c->rx_chan,
				dma_src, dma_dst, div, DMA_FROM_DEVICE, rd_flag);
		if (desc == NULL) {
			dev_err(&(i2c->adap.dev),"--I2C dma read desc apply is NULL");
			return -EBADR;
		}
	}


	/*printk("----> dma rd desc add successed! len: %d,i: %d, trig_value[i]: %d,burst_len[i]: %d, div: %d\n",
					len, i, trig_value[i], burst_len[i], div);*/
	desc->callback = jz_i2c_complete;
	desc->callback_param = &i2c->dma_r_complete;
	dmaengine_submit(desc);
	dma_async_issue_pending(i2c->rx_chan);

	return 0;
}
#endif

static inline int xfer_read(struct jz_i2c *i2c, unsigned char *buf, int len, int cnt, int idx)
{
	int i, length = len;
	int chan;
	int ret = 0;
	long timeout;
	unsigned short tmp;
	unsigned short *rcmd;
	unsigned char *wbuf;

	tmp = i2c_readl(i2c,I2C_INTM);
	tmp |= I2C_INTM_MISTP | I2C_INTM_MISTT;
	i2c_writel(i2c,I2C_INTM,tmp);

	rcmd = (unsigned short *)kzalloc(sizeof(unsigned short) * len, GFP_KERNEL);
	wbuf = (unsigned char *)kzalloc(sizeof(unsigned char) * len, GFP_KERNEL);
	for (i = 0; i < len; i++) {
		rcmd[i] = I2C_DC_READ;
		if (i == len - 1)
			rcmd[i] |= I2C_DC_STP;
	}
	wbuf[0] = 31;
	wbuf[1] = 15;
	wbuf[2] = 3;
	wbuf[3] = 1;
	wbuf[4] = 0;

	memset((void *)buf, 0, length * sizeof(unsigned char));

	chan = init_dma_read(i2c, wbuf, buf, len, cnt);
	if (chan) {
		dev_err(&(i2c->adap.dev),"init dma read failed %d\n", chan);
		return chan;
	}

#ifndef WRITE_CMD_BY_CPU
	chan = init_dma_write(i2c, rcmd, len);
	if (chan) {
		dev_err(&(i2c->adap.dev),"init dma write failed %d\n", chan);
		return chan;
	}
#else
	for (i = 0; i < len; i++) {
		if (i2c_readl(i2c,I2C_STA) | I2C_STA_TFNF)
			i2c_writel(i2c,I2C_DC,rcmd[i]);
	}
#endif

	timeout = wait_for_completion_timeout(&i2c->dma_r_complete,
			msecs_to_jiffies(CONFIG_I2C_JZ4775_WAIT_MS));
	if(!timeout){
		dev_err(&(i2c->adap.dev),"--I2C xfer_read timeout len:%d rd: %d wr: %d count: %d\n", len, complete_r_count.counter, complete_w_count.counter,complete_count.counter);
#ifdef I2C_DEBUG
		jz_dump_i2c_regs(i2c);
#endif
		dmaengine_terminate_all(i2c->rx_chan);
		dmaengine_terminate_all(i2c->tx_chan);
		ret = -ETIMEDOUT;
	} else {

#ifdef I2C_DEBUG
	atomic_inc(&complete_w_count);
	atomic_inc(&complete_r_count);
#endif
	}

	tmp = i2c_readl(i2c,I2C_TXABRT);
	if(tmp) {
		jz_txabrt(i2c,tmp);
		if (tmp > 0x1 && tmp < 0x10)
			ret = -ENXIO;
		else
			ret = -EIO;
	}
	if(ret < 0)
		jz_i2c_reset(i2c);


	kfree(rcmd);
	kfree(wbuf);

	return ret;

}

static inline int xfer_write(struct jz_i2c *i2c, unsigned char *buf, int len, int cnt, int idx)
{
	long timeout = TIMEOUT;
	int i, chan = 0;
	int ret = 0;
	unsigned short  tmp;
	unsigned short *wbuf;

	wbuf = (unsigned short *)kzalloc(sizeof(unsigned short) * len, GFP_KERNEL);
	for(i = 0; i < len; i++) {
		wbuf[i] = I2C_DC_WRITE | buf[i];
		if ((i == len-1) && (!idx) )
			wbuf[i] |= I2C_DC_STP;
#ifdef NEED_RESTART
		/* if you want to create a restart signal between send the data, you will open this Macro definition*/
		if (i != 0)
			wbuf[i] |= I2C_DC_REST;
#endif
	}

	i2c_writel(i2c, I2C_INTM, 0);
	chan = init_dma_write(i2c, wbuf, len);
	if (chan) {
		dev_err(&(i2c->adap.dev),"init dma write failed %d\n",chan);
	}

	timeout = wait_for_completion_timeout(&i2c->dma_w_complete,
			msecs_to_jiffies(CONFIG_I2C_JZ4775_WAIT_MS));
	if(!timeout){
		dev_err(&(i2c->adap.dev),"--I2C dma write wait timeout\n");
		dmaengine_terminate_all(i2c->tx_chan);
#ifdef I2C_DEBUG
		jz_dump_i2c_regs(i2c);
#endif
	}
	tmp = i2c_readl(i2c, I2C_TXABRT);
	if(tmp) {
		jz_txabrt(i2c, tmp);
		if (tmp > 0x1 && tmp < 0x10)
			ret = -ENXIO;
		else
			ret = -EIO;
	}
	if(ret < 0)
		jz_i2c_reset(i2c);

	kfree(wbuf);

#ifdef I2C_DEBUG
	atomic_inc(&complete_w_count);
#endif

	return ret;
}

static int disable_i2c_clk(struct jz_i2c *i2c)
{
	int timeout = 10;
	int tmp = 0;

	do {
		udelay(90);
		tmp = i2c_readl(i2c,I2C_STA);
	} while ((tmp & I2C_STA_MSTACT) && (--timeout > 0));

	if (timeout > 0) {
		clk_disable(i2c->clk);
		return 0;
	} else {
		dev_err(&(i2c->adap.dev),"--I2C disable clk timeout, I2C_STA = 0x%x\n", tmp);
		jz_i2c_reset(i2c);
		clk_disable(i2c->clk);
		return -ETIMEDOUT;
	}
}

static int jz_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msg, int count)
{
	int i,ret;
	struct jz_i2c *i2c = adap->algo_data;

	clk_enable(i2c->clk);

	if (msg->addr != i2c_readl(i2c,I2C_TAR)) {
		i2c_writel(i2c, I2C_TAR, msg->addr);
	}
	for (i=0; i<count; i++, msg++) {
		if (msg->flags & I2C_M_RD){
			ret = xfer_read(i2c, msg->buf, msg->len, count, (count > 1));
		}else{
			ret = xfer_write(i2c, msg->buf, msg->len, count, (count > 1));
		}
		if (ret)
			return ret;
	}

	if (disable_i2c_clk(i2c))
		return -ETIMEDOUT;

	return i;
}

static u32 jz_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm jz_i2c_algorithm = {
	.master_xfer	= jz_i2c_xfer,
	.functionality	= jz_i2c_functionality,
};

static bool jz_dma_filter(struct dma_chan *chan, void *data)
{
	struct jz_i2c *i2c = data;
	return (void *)i2c->dma_type == chan->private;
}

static int jz_i2c_set_speed(struct jz_i2c *i2c)
{
	/*ns*/
	long dev_clk = clk_get_rate(i2c->clk)/1000;
	long cnt_high = 0;	/* HIGH period count of the SCL clock */
	long cnt_low = 0;	/* LOW period count of the SCL clock */
	long cnt_period = 0;
	long setup_time = 0;
	long hold_time = 0;
	unsigned short tmp;
	int rate = i2c->speed;


	if(jz_i2c_disable(i2c))
		dev_info(&(i2c->adap.dev),"i2c not disable\n");
	cnt_period = dev_clk / rate;
	if (rate <= 100) {
		tmp = 0x43 | (1<<5);      /* standard speed mode*/
		i2c_writel(i2c,I2C_CTRL,tmp);
		cnt_high = (cnt_period * 4000) / (4700 + 4000);
	} else {
		tmp = 0x45 | (1<<5);      /* fast speed mode*/
		i2c_writel(i2c,I2C_CTRL,tmp);
		cnt_high = (cnt_period * 600) / (1300 + 600);
	}
	cnt_low = cnt_period - cnt_high;

	if (rate <= 100) { /* standard mode */
		setup_time = 300;
		hold_time = 400;
	} else {
		setup_time = 450;
		hold_time = 450;
	}

	hold_time = (hold_time / (1000000 / dev_clk) - 1);
	setup_time = (setup_time / (1000000 / dev_clk) + 1);

	dev_info(&(i2c->adap.dev),"set:%ld  hold:%ld dev=%ld h=%ld l=%ld\n",
			setup_time, hold_time, dev_clk, cnt_high, cnt_low);
	if (setup_time > 255)
		setup_time = 255;
	if (setup_time <= 0)
		setup_time = 1;
	if (hold_time > 0xFFFF)
		hold_time = 0xFFFF;

	if (rate <= 100) {
		i2c_writel(i2c, I2C_SHCNT, I2CSHCNT_ADJUST(cnt_high));
		i2c_writel(i2c, I2C_SLCNT, I2CSLCNT_ADJUST(cnt_low));
	} else {
		i2c_writel(i2c, I2C_FHCNT, I2CFHCNT_ADJUST(cnt_high));
		i2c_writel(i2c, I2C_FLCNT, I2CFLCNT_ADJUST(cnt_low));
	}

	i2c_writel(i2c, I2C_SDASU, setup_time & 0xff);
	i2c_writel(i2c, I2C_SDAHD, hold_time);

	return 0;
}

static void jz_i2c_init_dma(struct jz_i2c *i2c, int direction)
{
	if (direction == DMA_TO_DEVICE) {
		init_completion(&i2c->dma_w_complete);
		i2c->tx_dma_config.src_addr = 0;
		i2c->tx_dma_config.dst_addr = (unsigned long)(i2c->dma_type + I2C_DC);
		i2c->tx_dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		i2c->tx_dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		i2c->tx_dma_config.src_maxburst = 64;
		i2c->tx_dma_config.dst_maxburst = 64;
		i2c->tx_dma_config.direction = direction;
	} else {
		init_completion(&i2c->dma_r_complete);
		i2c->rx_dma_config.src_addr = (unsigned long)(i2c->dma_type + I2C_DC);
		i2c->rx_dma_config.dst_addr = 0;
		i2c->rx_dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		i2c->rx_dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		i2c->rx_dma_config.src_maxburst = 64;
		i2c->rx_dma_config.dst_maxburst = 64;
		i2c->rx_dma_config.direction = direction;
	}
}

static int jz_i2c_dma_probe(struct platform_device *dev)
{
	int ret = 0;
	unsigned char tmp;
	struct resource *r;
	dma_cap_mask_t mask;

	struct jz_i2c *i2c = kzalloc(sizeof(struct jz_i2c), GFP_KERNEL);
	printk("======> start %s i2c-%d\n", __func__, dev->id);
	if (!i2c) {
		ret = -ENOMEM;
		goto no_mem;
	}

#ifdef I2C_DEBUG
	atomic_set(&complete_count, 0);
	atomic_set(&complete_r_count, 0);
	atomic_set(&complete_w_count, 0);
#endif

	i2c->adap.owner   	= THIS_MODULE;
	i2c->adap.algo    	= &jz_i2c_algorithm;
	i2c->adap.retries 	= 3;
	i2c->adap.algo_data 	= i2c;
	i2c->adap.dev.parent 	= &dev->dev;
	i2c->adap.nr 		= dev->id;
	sprintf(i2c->adap.name, "i2c%u", dev->id);

	i2c->clk = clk_get(&dev->dev, i2c->adap.name);
	if(!i2c->clk) {
		ret = -ENODEV;
		goto clk_failed;
	}

	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	i2c->iomem = ioremap(r->start, resource_size(r));
	if (!i2c->iomem) {
		ret = -ENOMEM;
		goto io_failed;
	}

	i2c->irq = platform_get_irq(dev, 0);
	ret = request_irq(i2c->irq, jz_i2c_irq, IRQF_DISABLED, dev_name(&dev->dev), i2c);
	if(ret) {
		ret = -ENODEV;
		goto irq_failed;
	}

	clk_enable(i2c->clk);

	r = platform_get_resource(dev, IORESOURCE_BUS, 0);
	i2c->speed = r->start;

	jz_i2c_set_speed(i2c);

	i2c_writel(i2c, I2C_DMATDLR, 32);	/*set trans fifo level*/
	i2c_writel(i2c, I2C_DMARDLR, 0);	/*set recive fifo level*/
	i2c_writel(i2c, I2C_DMACR, 3);	/*enable i2c dma*/
#ifdef I2C_DEBUG
	PRINT_REG_WITH_ID(I2C_DMARDLR, i2c);
#endif

	i2c_writel(i2c, I2C_INTM, 0x0);	/*disable i2c interrupt*/

	i2c_writel(i2c, I2C_FLT, 0xF);	/*set filter*/

	tmp = i2c_readl(i2c, I2C_CTRL);
	tmp |= I2C_CTRL_REST;
	i2c_writel(i2c, I2C_CTRL, tmp);

	i2c_writel(i2c, I2C_INTM, 0x0);


	init_completion(&i2c->r_complete);
	init_completion(&i2c->w_complete);

	r = platform_get_resource(dev, IORESOURCE_DMA, 0);
	i2c->dma_type = r->start;

	jz_i2c_init_dma(i2c, DMA_TO_DEVICE);
	jz_i2c_init_dma(i2c, DMA_FROM_DEVICE);
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	i2c->rx_chan = dma_request_channel(mask, jz_dma_filter, i2c);
	if(i2c->rx_chan <= 0){
		dev_err(&(i2c->adap.dev),"i2c dma request channel  failed\n");
		ret = -ENODEV;
		goto dma_failed;
	}
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	i2c->tx_chan = dma_request_channel(mask, jz_dma_filter, i2c);
	if (i2c->tx_chan <= 0) {
		dev_err(&(i2c->adap.dev), "i2c dma request channel failed");
		ret = -ENODEV;
		goto dma_failed;
	}
	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		dev_err(&(i2c->adap.dev),KERN_INFO "I2C: Failed to add bus\n");
		goto adapt_failed;
	}

	platform_set_drvdata(dev, i2c);

	jz_i2c_enable(i2c);
	clk_disable(i2c->clk);
	printk("======> %s i2c-%d successed! \n", __func__, i2c->adap.nr);

	return 0;

adapt_failed:
	free_irq(i2c->irq,i2c);
dma_failed:
irq_failed:
	iounmap(i2c->iomem);
io_failed:
	clk_put(i2c->clk);
clk_failed:
	kfree(i2c);
no_mem:
	printk(" i2c dma probe failed\n\n\n");
	return ret;
}

static int jz_i2c_remove(struct platform_device *dev)
{
	struct jz_i2c *i2c = platform_get_drvdata(dev);
	free_irq(i2c->irq,i2c);
	iounmap(i2c->iomem);
	clk_put(i2c->clk);
	i2c_del_adapter(&i2c->adap);
	kfree(i2c);
	return 0;
}

static struct platform_driver jz_i2c_dma_driver = {
	.probe		= jz_i2c_dma_probe,
	.remove		= jz_i2c_remove,
	.driver		= {
		.name	= "jz-i2c-dma",
	},
};

static int __init jz4775_i2c_dma_init(void)
{
	int ret;
	ret = platform_driver_register(&jz_i2c_dma_driver);
	return  ret;
}

static void __exit jz4775_i2c_dma_exit(void)
{
	platform_driver_unregister(&jz_i2c_dma_driver);
}

subsys_initcall(jz4775_i2c_dma_init);
module_exit(jz4775_i2c_dma_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("i2c driver for JZ47XX SoCs");

