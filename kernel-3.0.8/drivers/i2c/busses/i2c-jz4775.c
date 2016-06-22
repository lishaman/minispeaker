
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
#include <linux/interrupt.h>
#include <linux/gpio.h>

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

#define I2C_DC_REST			(1 << 10)
#define I2C_DC_STP			(1 << 9)
#define I2C_DC_READ    			(1 << 8)

#define I2C_ENB_I2CENB			(1 << 0) /* Enable the i2c */

/* I2C standard mode high count register(I2CSHCNT) */
#define I2CSHCNT_ADJUST(n)      (((n) - 8) < 6 ? 6 : ((n) - 8))
/* I2C standard mode low count register(I2CSLCNT) */
#define I2CSLCNT_ADJUST(n)      (((n) - 1) < 8 ? 8 : ((n) - 1))
/* I2C fast mode high count register(I2CFHCNT) */
#define I2CFHCNT_ADJUST(n)      (((n) - 8) < 6 ? 6 : ((n) - 8))
/* I2C fast mode low count register(I2CFLCNT) */
#define I2CFLCNT_ADJUST(n)      (((n) - 1) < 8 ? 8 : ((n) - 1))


#define I2C_FIFO_LEN 64
#define TX_LEVEL 32
#define RX_LEVEL 31
#define TIMEOUT	0xff

#define BUFSIZE 200

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
	unsigned char *wbuf;
	int rd_len;
	int wt_len;

	int rdcmd_len;
	int w_flag;

	int data_buf[BUFSIZE];
	int cmd_buf[BUFSIZE];
	int cmd;

	struct i2c_sg_data *data;
	struct completion r_complete;
	struct completion w_complete;

	int enabled;
	unsigned int rate;
	struct delayed_work clk_work;
};

//#define CONFIG_I2C_DEBUG
static inline unsigned short i2c_readl(struct jz_i2c *i2c,unsigned short offset);
#ifdef CONFIG_I2C_DEBUG

#define PRINT_REG_WITH_ID(reg_name, id)					\
	dev_info(&(i2c->adap.dev),"--"#reg_name "    	0x%08x\n",i2c_readl(id,reg_name))

static void jz_dump_i2c_regs(struct jz_i2c *i2c) {
	struct jz_i2c *i2c_id = i2c;

	PRINT_REG_WITH_ID(I2C_CTRL, i2c_id);
	PRINT_REG_WITH_ID(I2C_INTM, i2c_id);
	PRINT_REG_WITH_ID(I2C_RXTL, i2c_id);
	PRINT_REG_WITH_ID(I2C_TXTL, i2c_id);
	PRINT_REG_WITH_ID(0x78, i2c_id);
	return;

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
#ifdef CONFIG_I2C_DEBUG
	if(offset == I2C_DC){
		i2c->data_buf[i2c->cmd % BUFSIZE] += 1;
	}
#endif
	//return (*(volatile unsigned short *)(i2c->iomem + offset));
	return readl(i2c->iomem + offset);
}

static inline void i2c_writel(struct jz_i2c *i2c, unsigned short  offset, unsigned short value)
{
	//(*(volatile unsigned short *) (i2c->iomem + offset)) = (value);
	writel(value, i2c->iomem + offset);
}


static int jz_i2c_disable(struct jz_i2c *i2c)
{
	int timeout = TIMEOUT;
	i2c_writel(i2c, I2C_ENB, 0);
	while((i2c_readl(i2c, I2C_ENSTA) & I2C_ENB_I2CENB) && (--timeout > 0))
		msleep(1);

	if (timeout)
		return 0;

	printk("enable i2c%d failed\n", i2c->adap.nr);
	return -ETIMEDOUT;
}

static int jz_i2c_enable(struct jz_i2c *i2c)
{
	int timeout = TIMEOUT;

	i2c_writel(i2c, I2C_ENB, 1);
	while(!(i2c_readl(i2c, I2C_ENSTA) & I2C_ENB_I2CENB) && (--timeout > 0))
		msleep(1);

	if (timeout)
		return 0;

	printk("enable i2c%d failed\n", i2c->adap.nr);
	return -ETIMEDOUT;
}

static void jz_i2c_reset(struct jz_i2c *i2c)
{
	unsigned short tmp;

	i2c_readl(i2c, I2C_CTXABRT);

	i2c_readl(i2c, I2C_INTST);

	jz_i2c_disable(i2c);
	udelay(10);
	jz_i2c_enable(i2c);
}

void i2c_send_rcmd(struct jz_i2c *i2c, int cmd_count)
{
	int i;

	for(i=0; i<cmd_count; i++) {
		i2c_writel(i2c, I2C_DC, I2C_DC_READ);
#ifdef CONFIG_I2C_DEBUG
		i2c->cmd_buf[i2c->cmd % BUFSIZE] += 1;
#endif
	}
}

static irqreturn_t jz_i2c_irq(int irqno, void *dev_id)
{
	unsigned short tmp,tmp1,intst,intmsk;
	struct jz_i2c *i2c = dev_id;

	intst = i2c_readl(i2c, I2C_INTST);
	intmsk = i2c_readl(i2c, I2C_INTM);

#ifdef CONFIG_I2C_DEBUG
	//dev_info(&(i2c->adap.dev),"--I2C irq reg INTST:%x\n",intst);
#endif
        if((intst & I2C_INTST_TXABT) && (intmsk & I2C_INTM_MTXABT)) {
		// all data require'll abort,when trans data abort
                i2c_writel(i2c, I2C_INTM, 0);
                if(!i2c->w_flag)
                        complete(&i2c->r_complete);
                else
                        complete(&i2c->w_complete);
		return IRQ_HANDLED;
        }

	if((intst & I2C_INTST_RXFL) && (intmsk & I2C_INTM_MRXFL)) {
		while ((i2c_readl(i2c, I2C_STA) & I2C_STA_RFNE)){
			tmp = i2c_readl(i2c, I2C_DC) & 0xff;
			*i2c->rbuf++=tmp;
			i2c->rd_len--;
			if(i2c->rd_len == 0){
				complete(&i2c->r_complete);
				break;
			}
		}
		if(i2c->rd_len > 0 &&  i2c->rd_len <= I2C_FIFO_LEN){
			i2c_writel(i2c, I2C_RXTL, i2c->rd_len-1);
		}
	}

	if ((intst & I2C_INTST_TXEMP) && (intmsk & I2C_INTM_MTXEMP)) {
		tmp = i2c_readl(i2c, I2C_INTM);
		tmp &= ~I2C_INTM_MTXEMP;
		i2c_writel(i2c, I2C_INTM, tmp);

		if (i2c->w_flag == 0) {
			if(i2c->rdcmd_len > (I2C_FIFO_LEN - TX_LEVEL)){
				i2c_send_rcmd(i2c, I2C_FIFO_LEN - TX_LEVEL);
				i2c->rdcmd_len -= (I2C_FIFO_LEN - TX_LEVEL);

				tmp = i2c_readl(i2c, I2C_INTM);
				tmp |= I2C_INTM_MTXEMP;
				i2c_writel(i2c, I2C_INTM, tmp);
			} else {
				if(i2c->rdcmd_len > 1)
					i2c_send_rcmd(i2c, i2c->rdcmd_len-1);
				i2c->rdcmd_len = 0;

				tmp = I2C_DC_STP | I2C_DC_READ;
				i2c_writel(i2c, I2C_DC, tmp);
			}
		} else {
			while ((i2c_readl(i2c, I2C_STA) & I2C_STA_TFNF) && (i2c->wt_len > 0)) {
				tmp = *i2c->wbuf++;
				if (i2c->wt_len == 1)
					tmp |= I2C_DC_STP;
				i2c_writel(i2c, I2C_DC, tmp);
				i2c->wt_len -= 1;
			}
			if(i2c->wt_len) {
				tmp = i2c_readl(i2c, I2C_INTM);
				tmp |= I2C_INTM_MTXEMP;
				i2c_writel(i2c, I2C_INTM, tmp);
			}
			tmp1 = i2c_readl(i2c, I2C_STA);
		}
	}
	// wait stop bit trans.
	if(intst & I2C_INTST_ISTP) {
		tmp = i2c_readl(i2c, I2C_INTM);
                tmp &= ~I2C_INTST_ISTP;
                i2c_writel(i2c, I2C_INTM, tmp);
		tmp1 = i2c_readl(i2c, I2C_STA);
		if(i2c->w_flag){
			complete(&i2c->w_complete);
		}
	}

	return IRQ_HANDLED;
}

static void txabrt(struct jz_i2c *i2c, int src)
{
	int i;

	dev_err(&(i2c->adap.dev),"--I2C txabrt:\n");
#ifdef CONFIG_I2C_DEBUG
	dev_err(&(i2c->adap.dev), "--I2C device addr=%x\n", i2c_readl(i2c, I2C_TAR));
	dev_err(&(i2c->adap.dev), "--I2C send cmd count:%d  %d\n", i2c->cmd, i2c->cmd_buf[i2c->cmd]);
	dev_err(&(i2c->adap.dev), "--I2C receive data count:%d  %d\n", i2c->cmd, i2c->data_buf[i2c->cmd]);
	jz_dump_i2c_regs(i2c);
#endif
	for(i=0; i<16; i++) {
		if(src & (0x1 << i))
			dev_info(&(i2c->adap.dev), "--I2C TXABRT[%d]=%s\n", i, abrt_src[i]);
	}
}

static inline int xfer_read(struct jz_i2c *i2c, unsigned char *buf, int len, int cnt, int idx)
{
	int ret = 0;
	long timeout;
	unsigned short tmp;
        unsigned int wait_complete_timeout_ms;
        wait_complete_timeout_ms = len  * 1000 * 9 * 2/ i2c->rate + CONFIG_I2C_JZ4775_WAIT_MS;

	i2c->w_flag = 0;
	memset(buf, 0, len);
	i2c->rd_len = len;
	i2c->rdcmd_len = len;
	i2c->rbuf = buf;
	if(len <=  I2C_FIFO_LEN){
		i2c_writel(i2c, I2C_RXTL, len-1);
	}else{
#ifdef CONFIG_I2C_DEBUG
		i2c->cmd++;
		if(i2c->cmd >= BUFSIZE){
			i2c->cmd = 0;
		}
		i2c->cmd_buf[i2c->cmd % BUFSIZE] = 0;
		i2c->data_buf[i2c->cmd % BUFSIZE] = 0;
#endif
		i2c_writel(i2c, I2C_RXTL, RX_LEVEL);
	}
	i2c_writel(i2c, I2C_TXTL, TX_LEVEL);

	tmp = I2C_INTM_MRXFL | I2C_INTM_MTXEMP | I2C_INTM_MTXABT;
	i2c_writel(i2c,I2C_INTM,tmp);
	timeout = wait_for_completion_timeout(&i2c->r_complete,
					      msecs_to_jiffies(wait_complete_timeout_ms));
	if(!timeout){
		dev_err(&(i2c->adap.dev), "--I2C irq read timeout\n");
#ifdef CONFIG_I2C_DEBUG
		dev_err(&(i2c->adap.dev), "--I2C send cmd count:%d  %d\n", i2c->cmd,i2c->cmd_buf[i2c->cmd]);
		dev_err(&(i2c->adap.dev), "--I2C receive data count:%d  %d\n", i2c->cmd, i2c->data_buf[i2c->cmd]);
		jz_dump_i2c_regs(i2c);
#endif
		ret = -ETIMEDOUT;
	}
	tmp = i2c_readl(i2c, I2C_TXABRT);
	if(tmp) {
		txabrt(i2c, tmp);
		if (tmp > 0x1 && tmp < 0x10)
			ret = -ENXIO;
		else
			ret = -EIO;
		if(tmp & 8) {
			ret = -EAGAIN;
		}
		i2c_readl(i2c,I2C_CTXABRT);
	}
	if(ret < 0)
		jz_i2c_reset(i2c);

	return ret;
}

static inline int xfer_write(struct jz_i2c *i2c, unsigned char *buf, int len, int cnt, int idx)
{
	int ret = 0;
	long timeout = TIMEOUT;
	unsigned short  tmp;
	unsigned int wait_complete_timeout_ms;
	wait_complete_timeout_ms = len  * 1000 * 9 * 2/ i2c->rate + CONFIG_I2C_JZ4775_WAIT_MS;


	i2c->w_flag = 1;
	i2c->wbuf = buf;
	i2c->wt_len = len;
	i2c_writel(i2c, I2C_TXTL, TX_LEVEL);

	while ((i2c_readl(i2c, I2C_STA) & I2C_STA_TFNF) && (i2c->wt_len > 0)) {
		tmp = *i2c->wbuf++;
                if (i2c->wt_len == 1)
			tmp |= I2C_DC_STP;
		i2c_writel(i2c, I2C_DC, tmp);
		i2c->wt_len -= 1;
	}
	if(i2c->wt_len == 0) {
		i2c_writel(i2c, I2C_TXTL, 0);
	}
	i2c_readl(i2c,I2C_CSTP);  // clear stop bit
	tmp = I2C_INTM_MTXEMP | I2C_INTM_MTXABT | I2C_INTST_ISTP;
	i2c_writel(i2c, I2C_INTM, tmp);

	timeout = wait_for_completion_timeout(&i2c->w_complete,
					      msecs_to_jiffies(wait_complete_timeout_ms));

	if (!timeout) {
		dev_err(&(i2c->adap.dev),"--I2C pio write wait timeout\n");
#ifdef CONFIG_I2C_DEBUG
		jz_dump_i2c_regs(i2c);
#endif
		ret = -ETIMEDOUT;
	}
	tmp = i2c_readl(i2c, I2C_TXABRT);
	if (tmp) {
		txabrt(i2c, tmp);
		if (tmp > 0x1 && tmp < 0x10)
			ret = -ENXIO;
		else
			ret = -EIO;
		if(tmp & 8) {
                        ret = -EAGAIN;
                }
		i2c_readl(i2c,I2C_CTXABRT);
	}
	if(ret < 0)
		jz_i2c_reset(i2c);
	return ret;
}

static int disable_i2c_clk(struct jz_i2c *i2c)
{
	int timeout = 10;
	int tmp = i2c_readl(i2c, I2C_STA);

	while ((tmp & I2C_STA_MSTACT) && (--timeout > 0))
	{
		udelay(90);
		tmp = i2c_readl(i2c, I2C_STA);
	}
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

static int i2c_jz_xfer(struct i2c_adapter *adap, struct i2c_msg *msg, int count)
{
	int i,ret;
	struct jz_i2c *i2c = adap->algo_data;

	clk_enable(i2c->clk);

	if (msg->addr != i2c_readl(i2c, I2C_TAR)) {
		i2c_writel(i2c, I2C_TAR, msg->addr);
	}

	for (i = 0; i < count; i++, msg++) {
		if (msg->flags & I2C_M_RD) {
			ret = xfer_read(i2c, msg->buf, msg->len, count, i);
		} else {
			ret = xfer_write(i2c, msg->buf, msg->len, count, i);
		}
		if (ret < 0) {
			clk_disable(i2c->clk);
			return ret;
		}
	}

	if (disable_i2c_clk(i2c))
		return -ETIMEDOUT;

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

static int i2c_set_speed(struct jz_i2c *i2c, int rate)
{
	/*ns*/
	long dev_clk = clk_get_rate(i2c->clk);
	long cnt_high = 0;	/* HIGH period count of the SCL clock */
	long cnt_low = 0;	/* LOW period count of the SCL clock */
	long setup_time = 0;
	long hold_time = 0;
	unsigned short tmp;
        i2c->rate = rate;
	if(jz_i2c_disable(i2c))
		dev_info(&(i2c->adap.dev),"i2c not disable\n");
	if (rate <= 100000) {
		tmp = 0x43 | (1<<5);      /* standard speed mode*/
		i2c_writel(i2c,I2C_CTRL,tmp);
	} else {
		tmp = 0x45 | (1<<5);      /* fast speed mode*/
		i2c_writel(i2c,I2C_CTRL,tmp);
	}

	setup_time = (10000000/(rate*4)) + 1;
	//hold_time =  (10000000/(rate*4)) - 1;
	hold_time =  (dev_clk/(rate*4)) - 1;

	cnt_high = dev_clk/(rate*2);
	cnt_low = dev_clk/(rate*2);

	dev_info(&(i2c->adap.dev),"set:%ld  hold:%ld dev=%ld h=%ld l=%ld\n",
		 setup_time, hold_time, dev_clk, cnt_high, cnt_low);
	if (setup_time > 255)
		setup_time = 255;
	if (setup_time <= 0)
		setup_time = 1;
	if (hold_time > 0xFFFF)
		hold_time = 0xFFFF;

	if (rate <= 100000) {
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

static void i2c_clk_work(struct work_struct *work)
{
	struct jz_i2c *i2c = container_of(work, struct jz_i2c, clk_work.work);
	clk_disable(i2c->clk);
	i2c->enabled = 0;
}

static int i2c_jz_probe(struct platform_device *dev)
{
	int ret = 0;
	unsigned short tmp;
	struct resource *r;

	struct jz_i2c *i2c = kzalloc(sizeof(struct jz_i2c), GFP_KERNEL);
	if (!i2c) {
		ret = -ENOMEM;
		goto no_mem;
	}

	i2c->adap.owner   	= THIS_MODULE;
	i2c->adap.algo    	= &i2c_jz_algorithm;
	i2c->adap.retries 	= 5;
	i2c->adap.timeout 	= 5;
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

	INIT_DELAYED_WORK(&i2c->clk_work, i2c_clk_work);

	clk_enable(i2c->clk);

	r = platform_get_resource(dev, IORESOURCE_BUS, 0);

	i2c_set_speed(i2c, r->start * 1000);

	tmp = i2c_readl(i2c, I2C_DC);
	tmp &= ~I2C_DC_STP;
	i2c_writel(i2c, I2C_DC, tmp);

	tmp = i2c_readl(i2c, I2C_CTRL);
	tmp |= I2C_CTRL_REST;
	i2c_writel(i2c, I2C_CTRL, tmp);

	i2c_writel(i2c, I2C_INTM, 0x0);
	// for jgao
	//  i2c_writel(i2c, I2C_FLT, 0xF);	/*set filter*/

	init_completion(&i2c->r_complete);
	init_completion(&i2c->w_complete);

	i2c->cmd = 0;
	memset(i2c->cmd_buf, 0, BUFSIZE);
	memset(i2c->data_buf, 0, BUFSIZE);

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		dev_err(&(i2c->adap.dev), KERN_INFO "I2C: Failed to add bus\n");
		goto adapt_failed;
	}

	platform_set_drvdata(dev, i2c);

	jz_i2c_enable(i2c);

	clk_disable(i2c->clk);
	i2c->enabled = 0;

	return 0;

adapt_failed:
	free_irq(i2c->irq, i2c);
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
	free_irq(i2c->irq, i2c);
	iounmap(i2c->iomem);
	clk_put(i2c->clk);
	i2c_del_adapter(&i2c->adap);
	kfree(i2c);
	return 0;
}

static int i2c_jz_suspend(struct platform_device *dev, pm_message_t state)
{
#if 0
	struct jz_i2c *i2c = platform_get_drvdata(dev);
	cancel_delayed_work(&i2c->clk_work);
	clk_disable(i2c->clk);
	i2c->enabled = 0;
#endif
	return 0;
}

static struct platform_driver jz_i2c_driver = {
	.probe		= i2c_jz_probe,
	.remove		= i2c_jz_remove,
	.suspend 	= i2c_jz_suspend,
	.driver		= {
		.name	= "jz-i2c",
	},
};

static int __init jz4775_i2c_init(void)
{
	return platform_driver_register(&jz_i2c_driver);
}

static void __exit jz4775_i2c_exit(void)
{
	platform_driver_unregister(&jz_i2c_driver);
}

subsys_initcall(jz4775_i2c_init);
module_exit(jz4775_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ztyan<ztyan@ingenic.cn>");
MODULE_DESCRIPTION("i2c driver for JZ47XX SoCs");
