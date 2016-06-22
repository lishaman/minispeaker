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
#include <linux/syscalls.h>
#include <linux/reboot.h>

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
/*#define TX_LEVEL (I2C_FIFO_LEN / 2)*/
#define TX_LEVEL	3
/*#define RX_LEVEL (I2C_FIFO_LEN / 2 - 1)*/
#define RX_LEVEL	(I2C_FIFO_LEN - TX_LEVEL - 1)
#define TIMEOUT	0x64

#define BUFSIZE 200

struct jz_i2c {
	void __iomem		*iomem;
	int			 irq;
	struct clk		*clk;
	struct i2c_adapter	 adap;

	/* lock to protect rbuf and wbuf between xfer_read/write and irq handler */
	spinlock_t		lock;

	/* begin of lock scope */
	unsigned char	*rbuf;
	int		 rd_total_len;
	int		 rd_data_xfered;
	int		 rd_cmd_xfered;

	unsigned char	*wbuf;
	int		 wt_len;

	int	is_write;
	int	stop_hold;
	int speed;

	int data_buf[BUFSIZE];
	int cmd_buf[BUFSIZE];
	int cmd;

	atomic_t		trans_done;
	/* end of lock scope */

	wait_queue_head_t	trans_waitq;
};

struct jzi2c_watchdog {
	struct list_head list;
	int dev_addr;
	int fail_times_max;
	int (*reset_callback)(void *data);
	void *data;

	int fail_times;
	int reset_times;
};

static LIST_HEAD(jzi2c_wd_list);
static DEFINE_SPINLOCK(jzi2c_wd_list_lock);

#define msecs_to_loops(t) (loops_per_jiffy / 1000 * HZ * t)

/* ms */
#define CONFIG_JZI2C_TIMEOUT_PER_BYTE	300

/*#define CONFIG_I2C_DEBUG*/

static inline unsigned short i2c_readl(struct jz_i2c *i2c,unsigned short offset);

#define PRINT_REG_WITH_ID(reg_name, id) \
	dev_info(&(i2c->adap.dev),"--"#reg_name "    	0x%08x\n",i2c_readl(id,reg_name))

static __attribute__((unused)) void jz_dump_i2c_regs(struct jz_i2c *i2c) {
	struct jz_i2c *i2c_id = i2c;

#if 1
	PRINT_REG_WITH_ID(I2C_CTRL, i2c_id);
	PRINT_REG_WITH_ID(I2C_TAR, i2c_id);
	PRINT_REG_WITH_ID(I2C_SAR, i2c_id);
	PRINT_REG_WITH_ID(I2C_SHCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_SLCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_FHCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_FLCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_INTST, i2c_id);
	PRINT_REG_WITH_ID(I2C_INTM, i2c_id);
	PRINT_REG_WITH_ID(I2C_RXTL, i2c_id);
	PRINT_REG_WITH_ID(I2C_TXTL, i2c_id);
	PRINT_REG_WITH_ID(0x78, i2c_id);
	PRINT_REG_WITH_ID(I2C_STA, i2c_id);
	return;
#endif

	PRINT_REG_WITH_ID(I2C_CTRL, i2c_id);
	PRINT_REG_WITH_ID(I2C_TAR, i2c_id);
	PRINT_REG_WITH_ID(I2C_SAR, i2c_id);
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

static inline
unsigned short i2c_readl(struct jz_i2c *i2c,
			unsigned short offset) {
#ifdef CONFIG_I2C_DEBUG
	if(offset == I2C_DC){
		i2c->data_buf[i2c->cmd % BUFSIZE] += 1;
	}
#endif

	return readl(i2c->iomem + offset);
}

static inline
void i2c_writel(struct jz_i2c *i2c,
		unsigned short offset,
		unsigned short value) {
	writel(value, i2c->iomem + offset);
}

static int jz_i2c_disable(struct jz_i2c *i2c)
{
	unsigned short regval;
	unsigned long loops = msecs_to_loops(50);

	i2c_writel(i2c, I2C_ENB, 0);

	do {
		regval = i2c_readl(i2c, I2C_ENSTA);
		if (!(regval & I2C_ENB_I2CENB))
			break;
	} while (--loops);

	if (loops)
		return 0;

	printk("disable i2c%d failed: I2C_ENSTA=0x%04x\n", i2c->adap.nr, regval);
	return -1;
}

static int jz_i2c_enable(struct jz_i2c *i2c)
{
	unsigned short regval;
	unsigned long loops = msecs_to_loops(50);

	i2c_writel(i2c, I2C_ENB, 1);


	do {
		regval = i2c_readl(i2c, I2C_ENSTA);
		if (regval & I2C_ENB_I2CENB)
			break;
	} while (--loops);

	if (loops)
		return 0;

	printk("enable i2c%d failed\n", i2c->adap.nr);
	return -1;
}

static int i2c_set_target(struct jz_i2c *i2c, unsigned char address)
{
	unsigned short regval;
	unsigned long loops = msecs_to_loops(50);

	do {
		regval = i2c_readl(i2c, I2C_STA);
		if ( (regval & I2C_STA_TFE) && !(regval & I2C_STA_MSTACT) )
			break;

	} while (--loops);

	if (loops) {
		i2c_writel(i2c, I2C_TAR, address);
		return 0;
	}

	printk("set i2c%d to address 0x%02x failed, I2C_STA=0x%04x\n", i2c->adap.nr, address, regval);

	return -1;
}

static int i2c_set_speed(struct jz_i2c *i2c)
{
	int dev_clk_khz = clk_get_rate(i2c->clk) / 1000;
	int cnt_high = 0;	/* HIGH period count of the SCL clock */
	int cnt_low = 0;	/* LOW period count of the SCL clock */
	int cnt_period = 0;    /* period count of the SCL clock */
	int setup_time = 0;
	int hold_time = 0;
	unsigned short tmp;
	int i2c_clk = i2c->speed;

	if(jz_i2c_disable(i2c))
		printk("i2c not disable\n");

	/* 1 I2C cycle equals to cnt_period PCLK(i2c_clk) */
        cnt_period = dev_clk_khz / i2c_clk;
        if (i2c_clk <= 100) {
                /* i2c standard mode, the min LOW and HIGH period are 4700 ns and 4000 ns */
                cnt_high = (cnt_period * 4000) / (4700 + 4000);
        } else {
                /* i2c fast mode, the min LOW and HIGH period are 1300 ns and 600 ns */
                cnt_high = (cnt_period * 600) / (1300 + 600);
        }

        cnt_low = cnt_period - cnt_high;

#if 0
	printk("dev_clk = %d, i2c_clk = %d cnt_period = %d, cnt_high = %d, cnt_low = %d, \n",
		dev_clk_khz, i2c_clk, cnt_period, cnt_high, cnt_low);
#endif

	/*
	 * NOTE: I2C_CTRL_REST can't set when i2c enabled,
	 * because normal read are 2 messages, we cannot disable
	 * i2c controller between these two messages,
	 * this means that we must always set I2C_CTRL_REST when init I2C_CTRL
	 **/
        if (i2c_clk <= 100) {
		tmp = 0x43 | (1<<5);      /* standard speed mode*/
		i2c_writel(i2c,I2C_CTRL,tmp);

		i2c_writel(i2c, I2C_SHCNT, I2CSHCNT_ADJUST(cnt_high));
		i2c_writel(i2c, I2C_SLCNT, I2CSLCNT_ADJUST(cnt_low));
        } else {
		tmp = 0x45 | (1<<5);      /* fast speed mode*/
		i2c_writel(i2c, I2C_CTRL, tmp);

		i2c_writel(i2c, I2C_FHCNT, I2CFHCNT_ADJUST(cnt_high));
		i2c_writel(i2c, I2C_FLCNT, I2CFLCNT_ADJUST(cnt_low));
        }

	/*
	 * a i2c device must internally provide a hold time at least 300ns
	 * tHD:DAT
	 *	Standard Mode: min=300ns, max=3450ns
	 *	Fast Mode: min=0ns, max=900ns
	 * tSU:DAT
	 *	Standard Mode: min=250ns, max=infinite
	 *	Fast Mode: min=100(250ns is recommanded), max=infinite
	 *
	 * 1i2c_clk = 10^6 / dev_clk_khz
	 * on FPGA, dev_clk_khz = 12000, so 1i2c_clk = 1000/12 = 83ns
	 * on Pisces(1008M), dev_clk_khz=126000, so 1i2c_clk = 1000 / 126 = 8ns
	 *
	 * The actual hold time is (SDAHD + 1) * (i2c_clk period).
	 *
	 * The length of setup time is calculated using (SDASU - 1) * (ic_clk_period)
	 *
	 */
	if (i2c_clk <= 100) { /* standard mode */
		setup_time = 300;
		hold_time = 400;
	} else {
		setup_time = 450;
		hold_time = 450;
	}

	hold_time = (hold_time / (1000000 / dev_clk_khz) - 1);
	setup_time = (setup_time / (1000000 / dev_clk_khz) + 1);

	if (setup_time > 255)
		setup_time = 255;

	if (setup_time <= 0)
		setup_time = 1;

	i2c_writel(i2c,I2C_SDASU, setup_time);

	if (hold_time > 255)
		hold_time = 255;

	// printk("=====>hold_time = %d, setup_time = %d\n", hold_time, setup_time);

	if (hold_time >= 0) {
		hold_time |= I2C_SDAHD_HDENB;		/*i2c hold time enable */
		i2c_writel(i2c, I2C_SDAHD, hold_time);
	} else {
		/* disable hold time */
		i2c_writel(i2c, I2C_SDAHD, 0);
	}

#if 0
	printk("==============i2c%d==============\n", i2c->adap.nr);
	printk("====>I2C_CTRL  = 0x%08x\n", i2c_readl(i2c, I2C_CTRL));
	printk("====>I2C_SHCNT = 0x%08x\n", i2c_readl(i2c, I2C_SHCNT));
	printk("====>I2C_SLCNT = 0x%08x\n", i2c_readl(i2c, I2C_SLCNT));
	printk("====>I2C_FHCNT = 0x%08x\n", i2c_readl(i2c, I2C_FHCNT));
	printk("====>I2C_FLCNT = 0x%08x\n", i2c_readl(i2c, I2C_FLCNT));
	printk("====>I2C_SDASU = 0x%08x\n", i2c_readl(i2c, I2C_SDASU));
	printk("====>I2C_SDAHD = 0x%08x\n", i2c_readl(i2c, I2C_SDAHD));
#endif

	return 0;
}

#if 0
#define JZ_I2C_MAX_CLIENT_NUM	20

struct i2c_client_info {
	struct list_head list;
	int bus_id;
	unsigned char addr;
	unsigned int speed;
};

static struct i2c_client_info client_info[JZ_I2C_MAX_CLIENT_NUM];
static LIST_HEAD(client_list);
#endif

/* NOTE: caller must take i2c->lock */
static int __jz_i2c_cleanup(struct jz_i2c *i2c) {
	unsigned short	tmp;
	int		ret;

	/* can send stop now if need */
	tmp = i2c_readl(i2c, I2C_CTRL);
	tmp &= ~I2C_CTRL_STPHLD;
	i2c_writel(i2c, I2C_CTRL, tmp);

	/* disable all interrupts first */
	i2c_writel(i2c, I2C_INTM, 0);

	/* then clear all interrupts */
	i2c_readl(i2c,I2C_CTXABRT);
	i2c_readl(i2c, I2C_CINTR);

	/* then disable the controller */
	tmp = i2c_readl(i2c,I2C_CTRL);
	tmp &= ~(1<<0);
	i2c_writel(i2c,I2C_CTRL,tmp);
	udelay(10);
	tmp |= (1<<0);
	i2c_writel(i2c,I2C_CTRL,tmp);

	ret = jz_i2c_disable(i2c);
	if (ret)
		printk("unable to disable i2c%d when do cleanup\n", i2c->adap.nr);

	if (unlikely(i2c_readl(i2c,I2C_INTM) & i2c_readl(i2c,I2C_INTST))) {
		panic("i2c%d has interrupts after a completely cleanup!\n", i2c->adap.nr);
	}

	return ret;
}

static int jz_i2c_cleanup(struct jz_i2c *i2c) {
	int		ret;
	unsigned long	flags;

	spin_lock_irqsave(&i2c->lock, flags);
	ret = __jz_i2c_cleanup(i2c);
	spin_unlock_irqrestore(&i2c->lock, flags);

	return ret;
}

static int jz_i2c_prepare(struct jz_i2c *i2c)
{
	i2c_set_speed(i2c);
	udelay(10);

	return jz_i2c_enable(i2c);
}

void i2c_send_rcmd(struct jz_i2c *i2c, int cmd_count)
{
	int i;

	for (i = 0; i < cmd_count; i++) {
		i2c_writel(i2c, I2C_DC, I2C_DC_READ);
#ifdef CONFIG_I2C_DEBUG
		i2c->cmd_buf[i2c->cmd % BUFSIZE] += 1;
#endif
	}
}

static void jz_i2c_trans_done(struct jz_i2c *i2c) {
	i2c_writel(i2c, I2C_INTM, 0);
	atomic_set(&i2c->trans_done, 1);
	wake_up(&i2c->trans_waitq);
}

static irqreturn_t jz_i2c_irq(int irqno, void *dev_id)
{
	unsigned short	 tmp;
	unsigned short	 intst;
	unsigned short	 intmsk;
	struct jz_i2c	*i2c = dev_id;
	unsigned long	 flags;

	spin_lock_irqsave(&i2c->lock, flags);
	intmsk = i2c_readl(i2c,I2C_INTM);
	intst = i2c_readl(i2c,I2C_INTST);

	intst &= intmsk;
#ifdef CONFIG_I2C_DEBUG
	/*dev_info(&(i2c->adap.dev),"--I2C irq reg INTST:%x  INTM: %x \n",intst,intmsk);*/
#endif

	if (intst & I2C_INTST_TXABT) {
		jz_i2c_trans_done(i2c);
		goto done;
	}

	if (intst & I2C_INTST_RXOF) {
		printk("----> receive fifo overflow!\n");
        jz_i2c_trans_done(i2c);
		goto done;
	}

	/* when read, always drain RX FIFO before we send more Read Commands to avoid fifo overrun */
	if (i2c->is_write == 0) {
		int rd_left;

		while ((i2c_readl(i2c,I2C_STA) & I2C_STA_RFNE)){
			*(i2c->rbuf++) = i2c_readl(i2c, I2C_DC) & 0xff;
			i2c->rd_data_xfered++;
			if (i2c->rd_data_xfered == i2c->rd_total_len) {
				jz_i2c_trans_done(i2c);
				goto done;
			}
		}

		rd_left = i2c->rd_total_len - i2c->rd_data_xfered;
		/*
		 * if rd_left > I2C_FIFO_LEN, I2C_RXTL remains RX_LEVEL,
		 * but if rd_left < I2C_FIFO_LEN, set I2C_RXTL to (rd_left - 1)
		 * so when all transfer complete, we will receive a RXFL interrupt
		 */
		if (rd_left <= I2C_FIFO_LEN){
			i2c_writel(i2c, I2C_RXTL, rd_left - 1);
		}
	}

#if 0			      /* for debug */
	if ( (i2c->is_write == 0) & (i2c->adap.nr == 0) ) {
		printk("===>total: %d data_xfered = %d cmd_xfered = %d intm=0x%08x intst=0x%08x\n",
			i2c->rd_total_len, i2c->rd_data_xfered, i2c->rd_cmd_xfered,
			i2c_readl(i2c,I2C_INTM), i2c_readl(i2c,I2C_INTST));
	}
#endif

	if (intst & I2C_INTST_TXEMP) {
		if (i2c->is_write == 0) {
			int cmd_left = i2c->rd_total_len - i2c->rd_cmd_xfered;
			int max_send = (I2C_FIFO_LEN - 1) - (i2c->rd_cmd_xfered - i2c->rd_data_xfered);
			int cmd_to_send = min(cmd_left, max_send);

			if (i2c->rd_cmd_xfered != 0)
				cmd_to_send = min(cmd_to_send, I2C_FIFO_LEN - TX_LEVEL - 1);

			/*
			 * at this time:
			 * 1. we have read rd_data_xfered bytes
			 * 2. we have send rd_cmd_xfered commands
			 *
			 * we have (rd_total_len - rd_cmd_xfered) commands to send, BUT:
			 *  if rd_cmd_xfered > rd_data_xfered:
			 *     (rd_cmd_xfered - rd_data_xfered) are transfering
			 *     we can only send max of (I2C_FIFO_LEN -  (rd_cmd_xfered - rd_data_xfered)) RD commands
			 *  else rd_cmd_xfered will be equal rd_data_xfered
			 *
			 * So: we can send
			 *    min( (I2C_FIFO_LEN - (rd_cmd_xfered - rd_data_xfered)), (rd_total_len - rd_cmd_xfered))
			 *
			 */

			if (cmd_to_send) {
				i2c_send_rcmd(i2c, cmd_to_send);
				i2c->rd_cmd_xfered += cmd_to_send;
			}

			cmd_left = i2c->rd_total_len - i2c->rd_cmd_xfered;
			if (cmd_left == 0) {
				/*
				 * clear TXEMP interrupt, we are finish, do not need it any more!
				 * NOTE: I2C_INTM_MTXABT is still needed, because cmd_left ==0
				 *       do not mean all command are send to line
				 */
				intmsk = i2c_readl(i2c, I2C_INTM);
				intmsk &= ~I2C_INTM_MTXEMP;
				i2c_writel(i2c, I2C_INTM, intmsk);

				tmp = i2c_readl(i2c,I2C_CTRL);
				tmp &= ~I2C_CTRL_STPHLD;
				i2c_writel(i2c,I2C_CTRL,tmp);
			}
		} else {
			unsigned char data;
			volatile unsigned short i2c_sta;

			do {
				i2c_sta = i2c_readl(i2c, I2C_STA);
				if ( (i2c_sta & I2C_STA_TFNF) && (i2c->wt_len > 0) ) {
					data = *i2c->wbuf;
					data &= (~I2C_DC_READ);
					i2c_writel(i2c, I2C_DC, data);

					i2c->wbuf ++;
					i2c->wt_len --;
				} else
					break;
			} while (1);

			if(i2c->wt_len == 0) {
				if (!i2c->stop_hold) {
					tmp = i2c_readl(i2c,I2C_CTRL);
					tmp &= ~I2C_CTRL_STPHLD;
					i2c_writel(i2c,I2C_CTRL,tmp);
				}

				jz_i2c_trans_done(i2c);
				goto done;
            }
		}
	}

done:
	spin_unlock_irqrestore(&i2c->lock, flags);
	return IRQ_HANDLED;
}

static void txabrt(struct jz_i2c *i2c, int src)
{
	int i;

	dev_err(&(i2c->adap.dev),"--I2C txabrt: 0x%08x\n", src);
	dev_err(&(i2c->adap.dev),"--I2C device addr=0x%x\n",i2c_readl(i2c,I2C_TAR));
#ifdef CONFIG_I2C_DEBUG
	dev_err(&(i2c->adap.dev),"--I2C device addr=%x\n",i2c_readl(i2c,I2C_TAR));
	dev_err(&(i2c->adap.dev),"--I2C send cmd count:%d  %d\n",i2c->cmd,i2c->cmd_buf[i2c->cmd]);
	dev_err(&(i2c->adap.dev),"--I2C receive data count:%d  %d\n",i2c->cmd,i2c->data_buf[i2c->cmd]);
	jz_dump_i2c_regs(i2c);
#endif
	for(i = 0; i< 16; i++) {
		if(src & (0x1 << i))
			dev_info(&(i2c->adap.dev),"--I2C TXABRT[%d]=%s\n",i,abrt_src[i]);
	}
}



static inline int xfer_read(struct jz_i2c *i2c,unsigned char *buf,int len,int cnt,int idx)
{
	int		ret	  = 0;
	long		timeout;
	int		wait_time = CONFIG_JZI2C_TIMEOUT_PER_BYTE * (len + 5);
	unsigned short	tmp;
	unsigned long	flags;

	memset(buf, 0, len);

	spin_lock_irqsave(&i2c->lock, flags);

	/* FIXME: if we need to support >2 message when read, we need set stop_hold properly */
	i2c->stop_hold = 0;

	i2c->is_write = 0;
	i2c->rbuf = buf;
	i2c->rd_total_len = len;
	i2c->rd_data_xfered = 0;
	i2c->rd_cmd_xfered = 0;

	if (len <= I2C_FIFO_LEN) {
		i2c_writel(i2c, I2C_RXTL, len - 1);
	} else {
		i2c_writel(i2c, I2C_RXTL, RX_LEVEL);
#ifdef CONFIG_I2C_DEBUG
		i2c->cmd++;
		if(i2c->cmd >= BUFSIZE){
			i2c->cmd = 0;
		}
		i2c->cmd_buf[i2c->cmd % BUFSIZE] = 0;
		i2c->data_buf[i2c->cmd % BUFSIZE] = 0;
#endif
	}
	i2c_writel(i2c, I2C_TXTL, TX_LEVEL);

	atomic_set(&i2c->trans_done, 0);

	i2c_writel(i2c, I2C_INTM,
		I2C_INTM_MRXFL | I2C_INTM_MTXEMP | I2C_INTM_MTXABT | I2C_INTM_MRXOF);

	tmp = i2c_readl(i2c, I2C_CTRL);
	tmp |= I2C_CTRL_STPHLD;
	i2c_writel(i2c, I2C_CTRL, tmp);

	spin_unlock_irqrestore(&i2c->lock, flags);

	timeout = wait_event_timeout(i2c->trans_waitq,
				atomic_read(&i2c->trans_done),
				msecs_to_jiffies(wait_time));

	if(!timeout){
		dev_err(&(i2c->adap.dev),"--I2C irq read timeout\n");
#ifdef CONFIG_I2C_DEBUG
		dev_err(&(i2c->adap.dev),"--I2C send cmd count:%d  %d\n",i2c->cmd,i2c->cmd_buf[i2c->cmd]);
		dev_err(&(i2c->adap.dev),"--I2C receive data count:%d  %d\n",i2c->cmd,i2c->data_buf[i2c->cmd]);
		jz_dump_i2c_regs(i2c);
#endif
		ret = -EIO;
	}

	tmp = i2c_readl(i2c,I2C_TXABRT);
	if(tmp) {
		txabrt(i2c,tmp);
		ret = -EIO;
	}

	return ret;
}

static inline int xfer_write(struct jz_i2c *i2c,
			unsigned char *buf, int len,
			int cnt, int idx)
{
	int		ret	  = 0;
	int		wait_time = CONFIG_JZI2C_TIMEOUT_PER_BYTE * (len + 5);
	long		timeout;
	unsigned short	tmp;
	unsigned long	flags;

	spin_lock_irqsave(&i2c->lock, flags);

	if (idx < (cnt - 1))
		i2c->stop_hold = 1;
	else
		i2c->stop_hold = 0;

	i2c->is_write = 1;
	i2c->wbuf = buf;
	i2c->wt_len = len;

	i2c_writel(i2c, I2C_TXTL, TX_LEVEL);

	atomic_set(&i2c->trans_done, 0);

	i2c_writel(i2c,I2C_INTM, I2C_INTM_MTXEMP | I2C_INTM_MTXABT);

	tmp = i2c_readl(i2c, I2C_CTRL);
	tmp |= I2C_CTRL_STPHLD;
	i2c_writel(i2c, I2C_CTRL, tmp);

	spin_unlock_irqrestore(&i2c->lock, flags);

	timeout = wait_event_timeout(i2c->trans_waitq,
				atomic_read(&i2c->trans_done),
				msecs_to_jiffies(wait_time));
	if (timeout && !i2c->stop_hold) {
		/*
		 * transfer successfull, and not for read operation
		 * we must ensure the transfer is actually finished before we return
		 *
		 * if this write is part of a read operation,
		 * we do not need to wait to save some time
		 */
		volatile unsigned short i2c_sta;
		volatile int write_in_process;
		timeout = 50000;

		do {
			i2c_sta = i2c_readl(i2c, I2C_STA);

			write_in_process = (i2c_sta & I2C_STA_MSTACT) ||
				!(i2c_sta & I2C_STA_TFE);
			if (!write_in_process)
				break;

			udelay(10);
			timeout --;

			if (timeout == 0)
				break;
		} while (1);
	}

#if 0
	if (!timeout && (i2c->adap.nr == 2) && (len == 1)) {
		jz_dump_i2c_regs(i2c);
	}
#endif

	if(!timeout){
		dev_err(&(i2c->adap.dev),"--I2C write wait timeout\n");
#ifdef CONFIG_I2C_DEBUG
		jz_dump_i2c_regs(i2c);
#endif
		ret = -EIO;
	}

	tmp = i2c_readl(i2c,I2C_TXABRT);
	if (tmp) {
		txabrt(i2c,tmp);
		ret = -EIO;
	}

	return ret;
}


static struct jzi2c_watchdog *jzi2c_get_watchdog_by_addr(int dev_addr)
{
	struct jzi2c_watchdog *jzi2c_wd = NULL;

	spin_lock(&jzi2c_wd_list_lock);
	list_for_each_entry(jzi2c_wd, &jzi2c_wd_list, list) {
		if (jzi2c_wd->dev_addr == dev_addr) {
			goto found;
		}
	}
	jzi2c_wd = NULL;
 found:
	spin_unlock(&jzi2c_wd_list_lock);
	return jzi2c_wd;
}

void jzi2c_watchdog_proc(int xfer_addr, int xfer_result)
{
	struct jzi2c_watchdog *jzi2c_wd = jzi2c_get_watchdog_by_addr(xfer_addr);

	if (jzi2c_wd == NULL) {
		return;
	}
	//printk("debug==>addr:%d, result:%d\n", xfer_addr, xfer_result);
	if (xfer_result) {
		jzi2c_wd->fail_times++;
		//printk("debug==>fail times:%d\n", jzi2c_wd->fail_times);

		if (jzi2c_wd->fail_times > jzi2c_wd->fail_times_max) {
			if (!jzi2c_wd->reset_times) {
				if (jzi2c_wd->reset_callback) {
					jzi2c_wd->reset_callback(jzi2c_wd->data);
				}
				jzi2c_wd->fail_times = 0;
				jzi2c_wd->reset_times ++;
			} else {
				//reboot system
				printk("i2c xfer error, system reboot!\n");
				sys_sync();
				msleep(200);
				kernel_restart(NULL);
			}
		}
	} else {
		jzi2c_wd->reset_times = 0;
		jzi2c_wd->fail_times = 0;
	}
}


void jzi2c_watchdog_register(int dev_addr, int fail_times, int (*reset_callback)(void *data), void *data)
{
	struct jzi2c_watchdog *jzi2c_wd = jzi2c_get_watchdog_by_addr(dev_addr);

	if (jzi2c_wd != NULL) {
		printk("i2c addr:%d already register watchdog", dev_addr);
		return;
	}

	jzi2c_wd = kzalloc(sizeof(*jzi2c_wd), GFP_KERNEL);
	if (!jzi2c_wd) {
		return;
	}

	jzi2c_wd->dev_addr = dev_addr;
	jzi2c_wd->fail_times_max = fail_times;
	jzi2c_wd->reset_callback = reset_callback;
	jzi2c_wd->data = data;

	jzi2c_wd->fail_times = 0;
	jzi2c_wd->reset_times = 0;

	spin_lock(&jzi2c_wd_list_lock);
	list_add_tail(&jzi2c_wd->list, &jzi2c_wd_list);
	spin_unlock(&jzi2c_wd_list_lock);
	printk("i2c register watchdog ok\n");
}
EXPORT_SYMBOL_GPL(jzi2c_watchdog_register);


void jzi2c_watchdog_unregister(int dev_addr)
{
	struct jzi2c_watchdog *jzi2c_wd = jzi2c_get_watchdog_by_addr(dev_addr);

	if (jzi2c_wd != NULL) {
		spin_lock(&jzi2c_wd_list_lock);
		list_del(&jzi2c_wd->list);
		spin_unlock(&jzi2c_wd_list_lock);

		kfree(jzi2c_wd);
	}
}
EXPORT_SYMBOL_GPL(jzi2c_watchdog_unregister);


static int i2c_jz_xfer(struct i2c_adapter *adap, struct i2c_msg *msg, int count)
{
	int i = -EIO;
	int ret = 0;
	struct jz_i2c *i2c = adap->algo_data;

	BUG_ON(in_irq());

	/*
	 * sorry, our driver currently can not support more than two message
	 * if you have such requirements, contact Ingenic for support
	 */
	if (count > 2)
		return -EINVAL;

	ret = jz_i2c_prepare(i2c);
	if (ret) {
		printk("enter %s:%d\n", __func__, __LINE__);
		goto out;
	}

	if (msg->addr != i2c_readl(i2c,I2C_TAR)) {
		ret = i2c_set_target(i2c, msg->addr);
		if (ret) {
			printk("enter %s:%d\n", __func__, __LINE__);
			goto out;
		}
	}
	for (i = 0; i < count; i++, msg++) {
		if (msg->flags & I2C_M_RD){
			ret = xfer_read(i2c, msg->buf, msg->len, count, i);
		}else{
			ret = xfer_write(i2c, msg->buf, msg->len, count, i);
		}


		//monitor i2c xfer fail
		jzi2c_watchdog_proc(msg->addr, ret);

		if (ret) {
			printk("enter %s:%d\n", __func__, __LINE__);
			ret = -EAGAIN;
			goto out;
		}
	}

	ret = i;

out :
	jz_i2c_cleanup(i2c);
	return ret;
}

static u32 i2c_jz_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm i2c_jz_algorithm = {
	.master_xfer	= i2c_jz_xfer,
	.functionality	= i2c_jz_functionality,
};

static int i2c_jz_probe(struct platform_device *dev)
{
	int ret = 0;
	unsigned short tmp;
	struct resource *r;
	struct jz_i2c *i2c;

	dev_info(&dev->dev, "probe i2c%d", dev->id);

	i2c	= kzalloc(sizeof(struct jz_i2c), GFP_KERNEL);
	if (!i2c) {
		ret = -ENOMEM;
		goto no_mem;
	}

	i2c->adap.owner   	= THIS_MODULE;
	i2c->adap.algo    	= &i2c_jz_algorithm;
	i2c->adap.algo_data 	= i2c;
	i2c->adap.timeout 	= 5;
	i2c->adap.retries 	= 5;
	i2c->adap.dev.parent 	= &dev->dev;
	i2c->adap.nr 		= dev->id;
	sprintf(i2c->adap.name, "i2c%u", dev->id);

	init_waitqueue_head(&i2c->trans_waitq);
	atomic_set(&i2c->trans_done, 0);
	spin_lock_init(&i2c->lock);

	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	i2c->iomem = ioremap(r->start,resource_size(r));
	if (!i2c->iomem) {
		ret = -ENOMEM;
		goto io_failed;
	}

	platform_set_drvdata(dev, i2c);

	i2c->clk = clk_get(&dev->dev, i2c->adap.name);
	if(!i2c->clk) {
		ret = -ENODEV;
		goto clk_failed;
	}

	clk_enable(i2c->clk);
	r = platform_get_resource(dev, IORESOURCE_BUS, 0);
	i2c->speed = r->start;
#if 0 //clivia tmp set
	if(i2c->adap.nr == 1)
		i2c->speed = 400;
#endif
	printk("------>i2c%d i2c->speed %d \n",i2c->adap.nr, i2c->speed);

	i2c_set_speed(i2c);


	tmp = i2c_readl(i2c,I2C_CTRL);
	tmp &= ~I2C_CTRL_STPHLD;
	i2c_writel(i2c,I2C_CTRL,tmp);

	i2c_writel(i2c, I2C_INTM, 0x0);

	i2c->cmd = 0;
	memset(i2c->cmd_buf,0,BUFSIZE);
	memset(i2c->data_buf,0,BUFSIZE);

	i2c->irq = platform_get_irq(dev, 0);
	ret = request_irq(i2c->irq, jz_i2c_irq, IRQF_DISABLED,dev_name(&dev->dev), i2c);
	if(ret) {
		ret = -ENODEV;
		goto irq_failed;
	}

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		dev_err(&(i2c->adap.dev),KERN_INFO "I2C: Failed to add bus\n");
		goto adapt_failed;
	}

	return 0;

adapt_failed:
irq_failed:
	clk_put(i2c->clk);
clk_failed:
	iounmap(i2c->iomem);
io_failed:
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

static int i2c_jz_suspend(struct platform_device *dev, pm_message_t state)
{
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

