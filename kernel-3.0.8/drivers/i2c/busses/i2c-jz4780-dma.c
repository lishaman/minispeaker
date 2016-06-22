
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
#define	I2C_TXABRT		(0x80)
#define I2C_DMACR            	(0x88)
#define I2C_DMATDLR          	(0x8c)
#define I2C_DMARDLR     	(0x90)
#define	I2C_SDASU		(0x94)
#define	I2C_ACKGC		(0x98)
#define	I2C_ENSTA		(0x9C)
#define I2C_SDAHD		(0xD0)

/* I2C Control Register (I2C_CTRL) */
#define I2C_CTRL_STPHLD		(1 << 7) /* Stop Hold Enable bit: when tx fifo empty, 0: send stop 1: never send stop*/
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
char *abrt_src[] = {
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

#define I2C_DC_READ    			(1 << 8)

#define I2C_SDAHD_HDENB			(1 << 8)

#define I2C_ENB_I2CENB			(1 << 0) /* Enable the i2c */ 

/* I2C standard mode high count register(I2CSHCNT) */
#define I2CSHCNT_ADJUST(n)      (((n) - 8) < 6 ? 6 : ((n) - 8))
/* I2C standard mode low count register(I2CSLCNT) */
#define I2CSLCNT_ADJUST(n)      (((n) - 1) < 8 ? 8 : ((n) - 1))
/* I2C fast mode high count register(I2CFHCNT) */
#define I2CFHCNT_ADJUST(n)      (((n) - 8) < 6 ? 6 : ((n) - 8))
/* I2C fast mode low count register(I2CFLCNT) */
#define I2CFLCNT_ADJUST(n)      (((n) - 1) < 8 ? 8 : ((n) - 1))


#define I2C_FIFO_LEN 16
#define TIMEOUT	0xff
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
	unsigned char *rbuf;
	int buf_len;
	int r_len;
	
	struct completion r_complete;
	struct completion w_complete;
	struct completion dma_r_complete;
	struct completion dma_w_complete;
	struct i2c_sg_data *w_data;
	struct i2c_sg_data *r_data;


	struct dma_chan *chan;
	struct dma_async_tx_descriptor  *tx_desc;
	struct dma_slave_config dma_config;
	enum jzdma_type dma_type;
};

//#define I2CDEBUG
static inline unsigned short i2c_readl(struct jz_i2c *i2c,unsigned short offset);
#ifdef I2CDEBUG 
void jzdma_dump(struct dma_chan *chan);

#define PRINT_REG_WITH_ID(reg_name, id) \
	dev_info(&(i2c->adap.dev),"--"#reg_name "    	0x%08x\n",i2c_readl(id,reg_name))
static void jz_dump_i2c_regs(struct jz_i2c *i2c) {
	struct jz_i2c *i2c_id = i2c;
	
	PRINT_REG_WITH_ID(I2C_CTRL, i2c_id);
	PRINT_REG_WITH_ID(I2C_TAR, i2c_id);
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
	PRINT_REG_WITH_ID(I2C_STA, i2c_id);
/*debug trans & recive fifo count */
	PRINT_REG_WITH_ID(0x74, i2c_id);
	PRINT_REG_WITH_ID(0x78, i2c_id);
	
	PRINT_REG_WITH_ID(I2C_TXABRT, i2c_id);
	PRINT_REG_WITH_ID(I2C_DMACR, i2c_id);
	PRINT_REG_WITH_ID(I2C_DMATDLR, i2c_id);
	PRINT_REG_WITH_ID(I2C_DMARDLR, i2c_id);
	PRINT_REG_WITH_ID(I2C_SDASU, i2c_id);
	PRINT_REG_WITH_ID(I2C_ACKGC, i2c_id);
	PRINT_REG_WITH_ID(I2C_ENSTA, i2c_id);
	PRINT_REG_WITH_ID(I2C_SDAHD, i2c_id);
}
#endif
static inline unsigned short i2c_readl(struct jz_i2c *i2c,unsigned short offset)
{
	//return (*(volatile unsigned short *)(i2c->iomem + offset)); 
	return readl(i2c->iomem + offset);
}

static inline void i2c_writel(struct jz_i2c *i2c,unsigned short  offset,unsigned short value)
{
	//(*(volatile unsigned short *) (i2c->iomem + offset)) = (value); 
	writel(value,i2c->iomem + offset);
}

static int jz_i2c_disable(struct jz_i2c *i2c)
{
	int timeout = TIMEOUT;
	i2c_writel(i2c,I2C_ENB,0);
	while((i2c_readl(i2c,I2C_ENSTA) & I2C_ENB_I2CENB) && (--timeout > 0))msleep(1);
	
	return timeout?0:1;
}

static int jz_i2c_enable(struct jz_i2c *i2c)
{
	int timeout = TIMEOUT;

	i2c_writel(i2c,I2C_ENB,1);
	while(!(i2c_readl(i2c,I2C_ENSTA) & I2C_ENB_I2CENB) && (--timeout > 0))msleep(1);
	
	return timeout?0:1;
}

static irqreturn_t jz_i2c_irq(int irqno, void *dev_id)
{
	unsigned short tmp,intst;
	struct jz_i2c *i2c = dev_id;

	intst = i2c_readl(i2c,I2C_INTST);
	if(intst & I2C_INTST_RXFL) {
		tmp = i2c_readl(i2c,I2C_INTM);
		tmp &= ~(I2C_INTM_MRXFL);
		i2c_writel(i2c,I2C_INTM,tmp);
		complete(&i2c->r_complete);
	}
	if(intst & I2C_INTST_TXEMP) {
		tmp = i2c_readl(i2c,I2C_INTM);
		tmp &= ~I2C_INTM_MTXEMP;
		i2c_writel(i2c,I2C_INTM,tmp);
		complete(&i2c->w_complete);
	}
	if(intst & I2C_INTST_TXABT) {
		tmp = i2c_readl(i2c,I2C_INTM);
		tmp &= ~I2C_INTM_MTXABT;
		i2c_writel(i2c,I2C_INTM,tmp);
		complete(&i2c->r_complete);
		complete(&i2c->w_complete);
	}

	return IRQ_HANDLED;
}

static void txabrt(struct jz_i2c *i2c,int src)
{
	int i;
	for(i=0;i<16;i++) {
		if(src & (0x1 << i))
			dev_err(&(i2c->adap.dev),"--I2C TXABRT[%d]=%s\n",i,abrt_src[i]);
	}
}
static void i2c_complete(void *arg)
{
	struct completion *comp;
	if(arg != NULL){
		comp = (struct completion *)arg;;
		complete(comp);
	}
}

static int init_dma_write(struct jz_i2c *i2c,unsigned char *buf,int len,void *arg)
{
	struct dma_async_tx_descriptor *desc;
	dma_addr_t dma_src;

	i2c->dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	i2c->dma_config.dst_maxburst = 1;
	/* use dma */
	dmaengine_slave_config(i2c->chan, &i2c->dma_config);
	dma_src = CPHYSADDR(buf);	
	dma_sync_single_for_device(&(i2c->adap.dev),dma_src,len,DMA_TO_DEVICE);
	desc = i2c->chan->device->device_add_desc(i2c->chan,
			dma_src,CPHYSADDR(i2c->iomem + I2C_DC),len,DMA_TO_DEVICE,1);

	if(desc == NULL ){
		dev_err(&(i2c->adap.dev),"--I2C dma write get desc is NULL\n");
		return -EIO;
	}
	desc->callback = i2c_complete;
	desc->callback_param = arg;

	/* tx_submit */
	dmaengine_submit(desc);
	dma_async_issue_pending(i2c->chan);
	return 0;
}
static inline int xfer_read(struct jz_i2c *i2c,unsigned char *buf,int len,int cnt,int idx)
{
	int i;
	long timeout;
	unsigned short tmp;
	unsigned char *rbuf;
	unsigned short rcmd[2];
	struct dma_async_tx_descriptor *desc;
	dma_addr_t dma_src;
	
	tmp = i2c_readl(i2c,I2C_CTRL);
	tmp |= I2C_CTRL_STPHLD;
	i2c_writel(i2c,I2C_CTRL,tmp);

	if(len <= I2C_FIFO_LEN) {
		i2c_writel(i2c,I2C_RXTL,len - 1);
		tmp = i2c_readl(i2c,I2C_INTM);
		tmp |= I2C_INTM_MRXFL | I2C_INTM_MTXABT;
		i2c_writel(i2c,I2C_INTM,tmp);
		memset(buf,0,len);	

		for(i=0;i<len;i++) {	//need wait txfifo is not full ???
			timeout=TIMEOUT;
			while(!(i2c_readl(i2c,I2C_STA) & I2C_STA_TFNF)&& (--timeout > 0))msleep(1);
			if(timeout > 0){
				i2c_writel(i2c,I2C_DC,I2C_DC_READ);
			}else{
				i--;
			}
		}

		tmp = i2c_readl(i2c,I2C_CTRL);
		tmp &= ~I2C_CTRL_STPHLD;
		i2c_writel(i2c,I2C_CTRL,tmp);

		timeout = wait_for_completion_timeout(&i2c->r_complete,HZ);
		if(!timeout){ 
			printk("i2c read wait timeout");
		}
		tmp = i2c_readl(i2c,I2C_TXABRT);
		if(tmp) {
			txabrt(i2c,tmp);
			return -EIO;
		}
		i2c_readl(i2c,I2C_CINTR);
		while (len-- && ((i2c_readl(i2c,I2C_STA) & I2C_STA_RFNE))){
			tmp = i2c_readl(i2c,I2C_DC) & 0xff;
			*buf++=tmp; 
		}
	} else {
		rbuf = buf;
		rcmd[0] = I2C_DC_READ;
		dma_src = CPHYSADDR(rcmd);
		dma_sync_single_for_device(&(i2c->adap.dev),dma_src,4,DMA_TO_DEVICE);
		dma_src = CPHYSADDR(rbuf);
		dma_sync_single_for_device(&(i2c->adap.dev),dma_src,len,DMA_FROM_DEVICE);
		for(i=0;i<len;i++){
			rcmd[1] = i2c_readl(i2c,I2C_SHCNT);
			i2c->dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			i2c->dma_config.dst_maxburst = 4;
			dmaengine_slave_config(i2c->chan, &i2c->dma_config);
			dma_src = CPHYSADDR(rcmd);
			desc = i2c->chan->device->device_add_desc(i2c->chan,
					dma_src,CPHYSADDR(i2c->iomem + I2C_DC),2,DMA_TO_DEVICE,0);
			if(desc == NULL ){
				dev_err(&(i2c->adap.dev),"--I2C dma write cmd desc apply is NULL\n");
				return -EIO;
			}

			i2c->dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
			i2c->dma_config.src_maxburst = 1;
			dmaengine_slave_config(i2c->chan, &i2c->dma_config);
			dma_src = CPHYSADDR(rbuf);
			desc = i2c->chan->device->device_add_desc(i2c->chan,
					CPHYSADDR(i2c->iomem + I2C_DC),dma_src,1,DMA_FROM_DEVICE,1);
			if(desc == NULL ){
				dev_err(&(i2c->adap.dev),"--I2C dma read dscc apply is NULL\n");
				return -EIO;
			}
			rbuf++;
		}

		desc->callback = i2c_complete;
		desc->callback_param = &i2c->dma_r_complete;
		dmaengine_submit(desc);
		dma_async_issue_pending(i2c->chan);
		timeout = wait_for_completion_timeout(&i2c->dma_r_complete,HZ);
		if(!timeout){ 
			dev_err(&(i2c->adap.dev),"--I2C irq read timeout\n");
		}
	}
	tmp = i2c_readl(i2c,I2C_TXABRT);
	if(tmp) {
		txabrt(i2c,tmp);
		return -EIO;
	}
	return 0;
}

static inline int xfer_write(struct jz_i2c *i2c,unsigned char *buf,int len,int cnt,int idx)
{
	long timeout = TIMEOUT;
	unsigned short  tmp;
	tmp = i2c_readl(i2c,I2C_CTRL);
	tmp |= I2C_CTRL_STPHLD;
	i2c_writel(i2c,I2C_CTRL,tmp);

	if(len <= I2C_FIFO_LEN) {
		i2c_writel(i2c,I2C_TXTL,0);
		tmp = i2c_readl(i2c,I2C_INTM);
		tmp |= I2C_INTM_MTXEMP | I2C_INTM_MTXABT;
		i2c_writel(i2c,I2C_INTM,tmp);
		while(len--){
			timeout=TIMEOUT;
			while(!(i2c_readl(i2c,I2C_STA) & I2C_STA_TFNF)&& (--timeout > 0))msleep(1);
			if(timeout > 0){
				tmp = *buf++ | 0 << 8;//& ~(I2C_DC_READ);
				i2c_writel(i2c,I2C_DC,tmp);
			}else{
				len++;
			}
		}

		if(idx == cnt - 1) {
			msleep(10);
			tmp = i2c_readl(i2c,I2C_CTRL);
			tmp &= ~I2C_CTRL_STPHLD;
			i2c_writel(i2c,I2C_CTRL,tmp);
		}

		timeout = wait_for_completion_timeout(&i2c->w_complete,HZ);
		if(!timeout){
			dev_err(&(i2c->adap.dev),"--I2C pio write wait timeout\n");
		}
		tmp = i2c_readl(i2c,I2C_TXABRT);
		if (tmp) {
			txabrt(i2c,tmp);
			return -EIO;
		}
	} else {
		if(idx == cnt - 1) {
			tmp = i2c_readl(i2c,I2C_CTRL);
			tmp &= ~I2C_CTRL_STPHLD;
			i2c_writel(i2c,I2C_CTRL,tmp);
		}
		i2c_writel(i2c,I2C_INTM,0x0);	/*enable i2c dma*/
		init_dma_write(i2c,buf,len,&i2c->dma_w_complete);
		timeout = wait_for_completion_timeout(&i2c->dma_w_complete,HZ);
		if(!timeout){
			dev_err(&(i2c->adap.dev),"--I2C dma write wait timeout\n");
		}
		tmp = i2c_readl(i2c,I2C_TXABRT);
		if(tmp) {
			txabrt(i2c,tmp);
			return -EIO;
		}
	}
	return 0;
}

static int i2c_jz_xfer(struct i2c_adapter *adap, struct i2c_msg *msg, int count)
{
	int i,ret;
	struct jz_i2c *i2c = adap->algo_data;
	if (msg->addr != i2c_readl(i2c,I2C_TAR)) {
		i2c_writel(i2c,I2C_TAR,msg->addr);
	}
	for (i=0;i<count;i++,msg++) {
		if (msg->flags & I2C_M_RD){
			ret = xfer_read(i2c,msg->buf,msg->len,count,i);
		}else{
			ret = xfer_write(i2c,msg->buf,msg->len,count,i);
		}
		if (ret) return ret;
	}

	return i;
}

static u32 i2c_jz_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm i2c_jz_algorithm = {
	.master_xfer	= i2c_jz_xfer,
	.functionality	= i2c_jz_functionality,
};

static bool filter(struct dma_chan *chan, void *data)
{
	struct jz_i2c *i2c = data;
	return (void *)i2c->dma_type == chan->private;
}
#if 1
static int i2c_set_speed(struct jz_i2c *i2c,int rate)
{
	/*ns*/
	long dev_clk = clk_get_rate(i2c->clk);
	long cnt_high = 0;	/* HIGH period count of the SCL clock */
	long cnt_low = 0;	/* LOW period count of the SCL clock */
	long setup_time = 0;
	long hold_time = 0;
	unsigned short tmp;

	if(jz_i2c_disable(i2c))
		dev_info(&(i2c->adap.dev),"i2c not disable\n");
	if (rate <= 100000) {
		tmp = 0x43 | (1<<5);      /* standard speed mode*/
		i2c_writel(i2c,I2C_CTRL,tmp);
	} else {
		tmp = 0x45 | (1<<5);      /* fast speed mode*/
		i2c_writel(i2c,I2C_CTRL,tmp);
	}

#if 0
	setup_time = (((1/(rate*4))*1000000000)/100 + 1);
	hold_time = (((1/(rate*4))*1000000000)/100 + 1);

	cnt_high = dev_clk /(rate*2);
	cnt_low = dev_clk / (rate*2);
#endif

	setup_time = (10000000/(rate*4)) + 1;
	hold_time =  (10000000/(rate*4)) - 1;

	cnt_high = dev_clk/(rate*2);
	cnt_low = dev_clk/(rate*2);
	dev_info(&(i2c->adap.dev),"set:%ld  hold:%ld dev=%ld h=%ld l=%ld\n",setup_time,hold_time,dev_clk,cnt_high,cnt_low);
	if (setup_time > 255)
		setup_time = 255;
	if (setup_time <= 0)
		setup_time = 1;
	if (hold_time > 255)
		hold_time = 255;

	i2c_writel(i2c,I2C_SHCNT,I2CSHCNT_ADJUST(cnt_high));
	i2c_writel(i2c,I2C_SLCNT,I2CSLCNT_ADJUST(cnt_low));

	i2c_writel(i2c,I2C_SDASU,setup_time & 0xff);

	hold_time |= I2C_SDAHD_HDENB;		/*i2c hold time enable */
	i2c_writel(i2c,I2C_SDAHD,hold_time);
	return 0;
}
#else

static int i2c_set_speed(struct jz_i2c *i2c,int rate)
{
	long dev_clk = clk_get_rate(i2c->clk);
	long cnt_high = 0;	/* HIGH period count of the SCL clock */
	long cnt_low = 0;	/* LOW period count of the SCL clock */
	long cnt_period = 0;	/* period count of the SCL clock */
	long setup_time = 0;
	long hold_time = 0;
	unsigned short tmp;

	if (rate <= 0 || rate > 400000)
		goto Set_speed_err;

	cnt_period = dev_clk / rate;
	if (rate <= 100000) {
		/* i2c standard mode, the min LOW and HIGH period are 4700 ns and 4000 ns */
		cnt_high = (cnt_period * 4000) / (4700 + 4000);
	} else {
		/* i2c fast mode, the min LOW and HIGH period are 1300 ns and 600 ns */
		cnt_high = (cnt_period * 600) / (1300 + 600);
	}

	cnt_low = cnt_period - cnt_high;

	if(jz_i2c_disable(i2c))
		dev_info(&(i2c->adap.dev),"i2c not disable\n");

	if (rate <= 100000) {
		tmp = 0x43 | (1<<5);      /* standard speed mode*/
		i2c_writel(i2c,I2C_CTRL,tmp);
		i2c_writel(i2c,I2C_SHCNT,I2CSHCNT_ADJUST(cnt_high));
		i2c_writel(i2c,I2C_SLCNT,I2CSLCNT_ADJUST(cnt_low));
		setup_time = 300;
		hold_time = 400;
	} else {
		tmp = 0x45 | (1<<5);      /* fast speed mode*/
		i2c_writel(i2c,I2C_CTRL,tmp);
		i2c_writel(i2c,I2C_FHCNT,I2CFHCNT_ADJUST(cnt_high));
		i2c_writel(i2c,I2C_FLCNT,I2CFLCNT_ADJUST(cnt_low));
		setup_time = 300;
		hold_time = 400;
	}

	hold_time = (hold_time / (1000000000 / dev_clk) - 1);
	setup_time = (setup_time / (1000000000 / dev_clk) + 1);

	if (setup_time > 255)
		setup_time = 255;
	if (setup_time <= 0)
		setup_time = 1;
	if (hold_time > 255)
		hold_time = 255;
	dev_info(&(i2c->adap.dev),"set:%ld  hold:%ld dev=%ld h=%ld l=%ld\n",setup_time,hold_time,dev_clk,cnt_high,cnt_low);
	i2c_writel(i2c,I2C_SDASU,setup_time & 0xff);
	
	if (hold_time <= 0) {
		tmp = i2c_readl(i2c,I2C_SDAHD);
		tmp &= ~(I2C_SDAHD_HDENB);	/*i2c hold time disable */
		i2c_writel(i2c,I2C_SDAHD,tmp);
	} else {
		tmp = hold_time & 0xff;
		tmp |= I2C_SDAHD_HDENB;		/*i2c hold time enable */
		i2c_writel(i2c,I2C_SDAHD,tmp);
	}

	return 0;

Set_speed_err:
	dev_err(&(i2c->adap.dev),"Faild : i2c set sclk faild,rate=%d KHz,dev_clk=%dKHz.\n", rate, dev_clk);
	return -1;
}
#endif
static int i2c_jz_probe(struct platform_device *dev)
{
	int ret = 0;
	struct resource *r;
	dma_cap_mask_t mask;

	struct jz_i2c *i2c = kzalloc(sizeof(struct jz_i2c), GFP_KERNEL);
	if (!i2c) {
		ret = -ENOMEM;
		goto no_mem;
	}

	i2c->adap.owner   	= THIS_MODULE;
	i2c->adap.algo    	= &i2c_jz_algorithm;
	i2c->adap.retries 	= 6;
	i2c->adap.algo_data 	= i2c;
	i2c->adap.dev.parent 	= &dev->dev;
	i2c->adap.nr 		= dev->id;
	sprintf(i2c->adap.name, "i2c%u", dev->id);

	i2c->clk = clk_get(&dev->dev,i2c->adap.name);
	if(!i2c->clk) {
		ret = -ENODEV;
		goto clk_failed;
	}

	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	i2c->iomem = ioremap(r->start,resource_size(r));
	if (!i2c->iomem) {
		ret = -ENOMEM;
		goto io_failed;
	}

	i2c->irq = platform_get_irq(dev, 0);
	ret = request_irq(i2c->irq, jz_i2c_irq, IRQF_DISABLED,dev_name(&dev->dev), i2c);
	if(ret) {
		ret = -ENODEV;
		goto irq_failed;
	}

	clk_enable(i2c->clk);
	i2c_set_speed(i2c,100000);

	i2c_writel(i2c,I2C_DMATDLR,8);	/*set trans fifo level*/
	i2c_writel(i2c,I2C_DMARDLR,0);	/*set recive fifo level*/ 
	i2c_writel(i2c,I2C_DMACR,3);	/*enable i2c dma*/
	
	i2c_writel(i2c,I2C_INTM,0x0);	/*enable i2c dma*/

	init_completion(&i2c->r_complete);
	init_completion(&i2c->w_complete);
	init_completion(&i2c->dma_r_complete);
	init_completion(&i2c->dma_w_complete);

	i2c->dma_config.src_addr = (unsigned long)(r->start + I2C_DC);
	i2c->dma_config.dst_addr = (unsigned long)(r->start + I2C_DC);
	i2c->dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	i2c->dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	i2c->dma_config.src_maxburst = 1;
	i2c->dma_config.dst_maxburst = 1;

	r = platform_get_resource(dev, IORESOURCE_DMA, 0);
	i2c->dma_type = r->start;

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	i2c->chan = dma_request_channel(mask, filter, i2c);
	if(i2c->chan < 0){
		dev_err(&(i2c->adap.dev),"i2c dma request channel is faild\n");
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
	return ret;
}

static int i2c_jz_remove(struct platform_device *dev)
{
	struct jz_i2c *i2c = platform_get_drvdata(dev);
	free_irq(i2c->irq,i2c);
	iounmap(i2c->iomem);
	clk_put(i2c->clk);
	i2c_del_adapter(&i2c->adap);
	kfree(i2c);
	return 0;
}

static struct platform_driver jz_i2c_driver = {
	.probe		= i2c_jz_probe,
	.remove		= i2c_jz_remove,
	.driver		= {
		.name	= "jz-i2c",
	},
};

static int __init jz4780_i2c_init(void)
{
	return platform_driver_register(&jz_i2c_driver);
}

static void __exit jz4780_i2c_exit(void)
{
	platform_driver_unregister(&jz_i2c_driver);
}

subsys_initcall(jz4780_i2c_init);
module_exit(jz4780_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ztyan<ztyan@ingenic.cn>");
MODULE_DESCRIPTION("i2c driver for JZ47XX SoCs");

