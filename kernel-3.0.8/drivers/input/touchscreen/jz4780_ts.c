 /*
 * JZ Touch Screen Driver
 *
 ** Copyright (c) 2005 -2013  Ingenic Semiconductor Inc.vincent<junyang@ingenic.cn>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/freezer.h>

#include <asm/irq.h>
#include <asm/gpio.h>
#include <asm/delay.h>
#include <linux/mfd/core.h>
#include <linux/jz4780-adc.h>
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/jz4780_ts.h>
#undef DEBUG

#undef SLEEPMODE
#ifdef DEBUG
#define dprintk(msg...)	printk("jz-sadc: " msg)
#else
#define dprintk(msg...)
#endif

#define TS_NAME "jz-ts"


/* Sample times in one sample process	*/
//#define SAMPLE_TIMES		3

#define SAMPLE_TIMES		3
#define DROP_SAMPLE_TIMES	 0  /* min drop 1 sample */
#define CAL_SAMPLE_TIMES	(SAMPLE_TIMES - DROP_SAMPLE_TIMES)
#define VIRTUAL_SAMPLE		3 /* min >= 2 */
/* Min pressure value. If less than it, filt the point.
 * Mask it if it is not useful for you
 */
//#define MIN_PRESSURE		0x100

/* Max delta x distance between current point and last point.	*/
#define MAX_DELTA_X_OF_2_POINTS	200
/* Max delta x distance between current point and last point.	*/
#define MAX_DELTA_Y_OF_2_POINTS	120

/* Max delta between points in one sample process
 * Verify method :
 * 	(diff value / min value) * 100 <= MAX_DELTA_OF_SAMPLING
 */
#define MAX_DELTA_OF_SAMPLING	20


#define TS_ABS(x)       ((x) > 0 ? (x): -(x))
#define DIFF(a,b)		(((a)>(b))?((a)-(b)):((b)-(a)))
#define MIN(a,b)		(((a)<(b))?(a):(b))

/*
 *------------------------------the fllowing macro is for platform_device ----------
 */

#define				TS_REG_ADSAME_OFFSET		0x0
#define				TS_REG_ADWAIT_OFFSET		0x4
#define				TS_REG_ADTCH_OFFSET		0x8
#define				TS_REG_ADCMD_OFFSET		0x14
#define				SOFTWARE_CMD			0
#define				HARDWARE_CMD			1

#define				REG_ADC_ENABLE			0x00
#define				REG_ADC_CFG			0x04
#define				REG_ADC_CTRL			0x08
#define				REG_ADC_STATUS			0x0c

/************************************************************************/
/*	SAR ADC OPS							*/
/************************************************************************/

typedef struct datasource {
	/*
	u16 xbuf;
	u16 ybuf;
	*/
	int xbuf;
	int ybuf;
	u16 zbuf;
	u16 reserve;
}datasource_t;
struct ts_event {
	u16 status;
	/*
	u16 x;
	u16 y;
	*/
	long x;
	long y;
	u16 pressure;
	u16 pad;
};
#define TOUCH_TYPE 1

//sadc touch fifo size 2 * 32bit
#define FIFO_MAX_SIZE 2


static struct ts_calibration{
	int a0;
	int a1;
	int a2;
	int a3;
	int a4;
	int a5;
	int a6;
};
struct jz_ts_t {
	int touch_cal_count;

	unsigned int ts_fifo[FIFO_MAX_SIZE][CAL_SAMPLE_TIMES];
	datasource_t data_s;
	struct ts_event event;
	int event_valid;
	unsigned int x_max;
	unsigned int x_min;
	unsigned int y_max;
	unsigned int y_min;
	unsigned int z_max;
	unsigned int z_min;
	unsigned short x_resolution;
	unsigned short y_resolution;
	unsigned short	y_r_plate;
	unsigned short	x_r_plate;
	unsigned short  pressure_max;

	int cal_type; /* current calibrate type */
	spinlock_t lock;


	char	phys[32];
	struct input_dev *input_dev; /* for touch screen */

	struct delayed_work slpmd_work;/*for monitor sleep mode*/
	int	pen_down;//for sleep mode
	int	last_pen_down;//for sleep mode
	int	is_sleep_mode;

	struct platform_device *pdev;
	struct resource *mem;
	void   __iomem  *io_base;
	struct resource *adc_cmd_mem;//for ADC CMD register
	void   __iomem  *io_cmd_base;
	struct work_struct touch_work;
	int	pendw_irq;/*pendown irq*/
	int	penup_irq;/*pen up irq*/
	int	dtch_irq;/*touch data ready irq*/
	int	slpendw_irq;/*pen down irq for sleep mode */
	unsigned short	same_time_usec;/*for ADSAME register */
	unsigned short  wait_time_msec;/*for ADWAIT register */
	unsigned int	sample_num;
	unsigned int	slmd_monitor_delay;
	int		command_type;/*for set hardware or software cmd*/
	const	struct mfd_cell *cell;
	int	is_pen_down;
};

static struct jz_ts_t *jz_ts;
static struct ts_calibration jz_cal;//it is't included by jz_ts,beacause jz_ts pointer may be not init;

/*
 *---------------------------------the following method is for manager interrupt for ts(2013-07-20)----
 */

static inline void jz_enable_ts(struct device *dev)
{
	uint8_t val = 0;
	#ifdef SLEEPMODE
	val |= ADSTATE_PENU | ADSTATE_DTCH | ADSTATE_PEND | ADSTATE_SLPEND;//clear interrupt flags
	//jz_adc_set_adenable(dev,(uint8_t)(ADENA_SLP_MD | ADENA_PENDEN),(uint8_t)(ADENA_SLP_MD | ADENA_PENDEN));
	adc_write_reg(dev,REG_ADC_ENABLE,(ADENA_SLP_MD | ADENA_PENDEN),(ADENA_SLP_MD | ADENA_PENDEN));
	adc_write_reg(dev,REG_ADC_STATUS,val,val); //write 1 to clear this bit
	#else
	//jz_adc_set_adenable(dev,(uint8_t)ADENA_PENDEN,(uint8_t)ADENA_PENDEN);
	adc_write_reg(dev,REG_ADC_ENABLE,ADENA_PENDEN,ADENA_PENDEN);
	val |= ADSTATE_PEND | ADSTATE_PENU | ADSTATE_DTCH; //clear interrupt flags
	//jz_adc_set_state(dev,val,val);
	adc_write_reg(dev,REG_ADC_STATUS,val,val);//write 1 to clear status register
	#endif
}

static inline void  jz_sadc_start_ts(struct device *dev,struct jz_ts_t *ts)
{
	uint32_t  param;
	#if 0
	if(!ts || !dev)
		return -ENODEV;
	#endif
	writew(ts->same_time_usec,ts->io_base + TS_REG_ADSAME_OFFSET);
	writew(ts->wait_time_msec,ts->io_base + TS_REG_ADWAIT_OFFSET);
	switch(ts->command_type)
	{
		case SOFTWARE_CMD:
			//jz_adc_set_config(dev,(ADCFG_CMD_SEL|ADCFG_SNUM(ts->sample_num)),(ADCFG_CMD_SEL|ADCFG_SNUM(ts->sample_num)));
			adc_write_reg(dev,REG_ADC_CFG,(ADCFG_CMD_SEL|ADCFG_SNUM(ts->sample_num)),(ADCFG_CMD_SEL|ADCFG_SNUM(ts->sample_num)));
			//active the command logic
			param = readl(ts->io_cmd_base);
			param = 0x00000000;
			param |= ADCMD_XPSUP | ADCMD_XNGRU | ADCMD_VREFNXN | ADCMD_VREFPXP | ADCMD_YPADC;
			writel(param,ts->io_cmd_base);
			param = 0x00000000;
			param |= ADCMD_YPSUP | ADCMD_YNGRU | ADCMD_VREFNYN | ADCMD_VREFPYP | ADCMD_XPADC;
			writel(param,ts->io_cmd_base);
			break;
		case HARDWARE_CMD:
			//jz_adc_set_config(dev,(ADCFG_SPZZ|ADCFG_XYZ_XYZ1Z2 |ADCFG_SNUM(ts->sample_num)),(ADCFG_SPZZ | ADCFG_XYZ_XYZ1Z2 | ADCFG_SNUM(ts->sample_num)));
			adc_write_reg(dev,REG_ADC_CFG,(ADCFG_SPZZ|ADCFG_XYZ_XYZ1Z2 |ADCFG_SNUM(ts->sample_num)),(ADCFG_SPZZ | ADCFG_XYZ_XYZ1Z2 | ADCFG_SNUM(ts->sample_num)));

			break;
		default:
			//jz_adc_set_config(dev,(ADCFG_SPZZ|ADCFG_XYZ_XYZ1Z2 |ADCFG_SNUM(ts->sample_num)),(ADCFG_SPZZ | ADCFG_XYZ_XYZ1Z2 | ADCFG_SNUM(ts->sample_num)));
			adc_write_reg(dev,REG_ADC_CFG,(ADCFG_SPZZ|ADCFG_XYZ_XYZ1Z2 |ADCFG_SNUM(ts->sample_num)),(ADCFG_SPZZ | ADCFG_XYZ_XYZ1Z2 | ADCFG_SNUM(ts->sample_num)));
			break;


	}
	pr_info("config register = %d",(uint32_t)(adc_read_reg(dev,REG_ADC_CFG)));
	jz_enable_ts(dev);


}

/*
 *the following  are the attribute for adding this driver
 */
static ssize_t store_cal_a0(struct device_driver *drv,const char *buf,size_t count)
{
	int n;
	if((count > 0) && (sscanf(buf,"%d",&n) == 1))
	{
		jz_cal.a0 = n;
		return count;
	}
	return -EINVAL;
}

static ssize_t show_cal_a0(struct device_driver *drv,char *buf)
{
	return scnprintf(buf,PAGE_SIZE,"%d\n",jz_cal.a0);
}

DRIVER_ATTR(cal_a0,S_IRUGO|S_IWUSR,show_cal_a0,store_cal_a0);

static ssize_t store_cal_a1(struct device_driver *drv,const char *buf,size_t count)
{
	int n;
	if((count > 0) && (sscanf(buf,"%d",&n) == 1))
	{
		jz_cal.a1 = n;
		return count;

	}
	return -EINVAL;
}

static ssize_t show_cal_a1(struct device_driver *drv,char *buf)
{
	return scnprintf(buf,PAGE_SIZE,"%d\n",jz_cal.a1);
}

DRIVER_ATTR(cal_a1,S_IRUGO | S_IWUSR,show_cal_a1,store_cal_a1);

static ssize_t store_cal_a2(struct device_driver *drv,const char *buf,size_t count)
{
	int n;
	if((count > 0) && (sscanf(buf,"%d",&n) == 1))
	{
		jz_cal.a2 = n;
		return count;
	}
	return -EINVAL;
}

static ssize_t show_cal_a2(struct device_driver *drv,char *buf)
{
	return scnprintf(buf,PAGE_SIZE,"%d\n",jz_cal.a2);
}
DRIVER_ATTR(cal_a2,S_IRUGO | S_IWUSR,show_cal_a2,store_cal_a2);

static ssize_t store_cal_a3(struct device_driver *drv,const char *buf,size_t count)
{
	int n;
	if((count >0) && (sscanf(buf,"%d",&n) == 1))
	{
		jz_cal.a3 = n;
		return count;
	}
	return -EINVAL;

}
static ssize_t show_cal_a3(struct device_driver *drv,char *buf)
{
	return scnprintf(buf,PAGE_SIZE,"%d\n",jz_cal.a3);
}
DRIVER_ATTR(cal_a3,S_IRUGO | S_IWUSR,show_cal_a3,store_cal_a3);
static ssize_t store_cal_a4(struct device_driver *drv,const char *buf,size_t count)
{
	int n;
	if((count > 0) && (sscanf(buf,"%d",&n) == 1))
	{
		jz_cal.a4 = n;
		return count;
	}
	return -EINVAL;
}

static ssize_t show_cal_a4(struct device_driver *drv,char *buf)
{
	return scnprintf(buf,PAGE_SIZE,"%d\n",jz_cal.a4);
}

DRIVER_ATTR(cal_a4,S_IRUGO | S_IWUSR,show_cal_a4,store_cal_a4);

static ssize_t store_cal_a5(struct device_driver *drv,const char *buf,size_t count)
{
	int n;
	if((count > 0) && (sscanf(buf,"%d",&n) == 1))
	{
		jz_cal.a5 = n;
		return count;
	}
	return -EINVAL;
}

static ssize_t show_cal_a5(struct device_driver *drv,char *buf)
{
	return scnprintf(buf,PAGE_SIZE,"%d\n",jz_cal.a5);
}

DRIVER_ATTR(cal_a5,S_IRUGO | S_IWUSR,show_cal_a5,store_cal_a5);


static ssize_t store_cal_a6(struct device_driver *drv,const char *buf,size_t count)
{
	int n;
	if((count > 0) && (sscanf(buf,"%d",&n) == 1))
	{
		jz_cal.a6 = n;
		return count;
	}
	return -EINVAL;
}

static ssize_t show_cal_a6(struct device_driver *drv,char *buf)
{
	return scnprintf(buf,PAGE_SIZE,"%d\n",jz_cal.a6);
}

DRIVER_ATTR(cal_a6,S_IRUGO | S_IWUSR,show_cal_a6,store_cal_a6);


/************************************************************************/
/*	Touch Screen module						*/
/************************************************************************/

#define TSMAXX		3920
#define TSMAXY		3700
#define TSMAXZ		(1024) /* measure data */

#define TSMINX		150
#define TSMINY		270
#define TSMINZ		0


#define SCREEN_MAXX	800//1023
#define SCREEN_MAXY	480//1023
#define PRESS_MAXZ      256


#undef Yr_PLANE
#undef Xr_PLANE
#define Yr_PLANE  480
#define Xr_PLANE  800
/*
 *---------the fllowing macro is for ts (2013-07-20)
 */

//#define		HARDWARE_CMD	0
//#define		SOFTWARE_CMD	1
#define		SAME_TIME	1 //can change it ,about 0.02ms
#define		WAIT_TIME	500 //about 3.33ms
#define		SAMPLE_NUM	3
#define		COMD_TYPE	HARDWARE_CMD
#define		MONITOR_TIME	10000
/**
 *The following macro is defined for address offset of adc control register
 */


static unsigned long transform_to_screen_x(struct jz_ts_t *ts, unsigned long x,int origin_x_change )
{
	if (x < ts->x_min) x = ts->x_min;
	if (x > ts->x_max) x = ts->x_max;
	if(origin_x_change)
	{
		int tmp = ts->x_max - x + ts->x_min;
		return (tmp - ts->x_min) * ts->x_resolution / (ts->x_max - ts->x_min);
	}
	return (x - ts->x_min) * ts->x_resolution / (ts->x_max - ts->x_min);
}

static unsigned long transform_to_screen_y(struct jz_ts_t *ts, unsigned long y,int origin_y_change)
{
	if (y < ts->y_min) y = ts->y_min;
	if (y > ts->y_max) y = ts->y_max;
	if(origin_y_change)
	{
		int tmp = ts->y_max - y + ts->y_min;
		return (tmp - ts->y_min) * ts->y_resolution / (ts->y_max - ts->y_min);
	}
	return (y- ts->y_min) * ts->y_resolution / (ts->y_max - ts->y_min);
}

static unsigned long transform_to_screen_z(struct jz_ts_t *ts, unsigned long z){
	if(z < ts->z_min) z = ts->z_min;
	if (z > ts->z_max) z = ts->z_max;
	return (ts->z_max - z) * ts->pressure_max / (ts->z_max - ts->z_min);
}

/*
 *this method is add for adjust using tslib ---(2013-07-26)
 */

static int ts_adjust(struct jz_ts_t *ts,long *x,long *y,int xp,int yp,int swap)
{
	if(jz_cal.a6 == 0)
		return 1;
	*x = (long)((jz_cal.a2 + jz_cal.a0 * xp + jz_cal.a1 * yp)/jz_cal.a6);
	*y = (long)((jz_cal.a5 + jz_cal.a3 * xp + jz_cal.a4 * yp)/jz_cal.a6);
	if(swap)
	{
		long tmp = *x;
		*x = *y;
		*y = tmp;
	}
	return 0;
}



/* R plane calibrate,please look up spec 11th page*/


#define Touch_Formula_One(z1,z2,ref,r) ({	\
		int z;					\
		if((z1) > 0){				\
		z = ((ref) * (z2)) / (z1);		\
		if((z2) >= (z1)) z = (z * r - (ref) * r) / (4096);	\
		else z = 0;				\
		}else					\
		z = 4095;				\
		z;					\
		})


static int ts_data_filter(struct jz_ts_t *ts){
	int i,xt = 0,yt = 0,zt1 = 0,zt2 = 0,zt3 = 0,zt4 = 0,t1_count = 0,t2_count = 0,z;

	datasource_t *ds = &ts->data_s;
	int t,xmin = 0x0fff,ymin = 0x0fff,xmax = 0,ymax = 0;//,z1min = 0xfff,z1max = 0,z2min = 0xfff,z2max = 0;

	/* fifo high 16 bit = y,fifo low 16 bit = x */

	for(i = 0;i < CAL_SAMPLE_TIMES;i++){

		t = (ts->ts_fifo[0][i] & 0x0fff);
#if (CAL_SAMPLE_TIMES >= 3)
		if(t > xmax) xmax = t;
		if(t < xmin) xmin = t;
#endif
		xt += t;
		t = (ts->ts_fifo[0][i] >> 16) & 0x0fff;
#if (CAL_SAMPLE_TIMES >= 3)
		if(t > ymax) ymax = t;
		if(t < ymin) ymin = t;
#endif

		yt += t;
		if(ts->ts_fifo[1][i] & 0x8000)
		{
			t = (ts->ts_fifo[1][i] & 0x0fff);
			zt1 += t;

			t = (ts->ts_fifo[1][i] >> 16) & 0x0fff;
			zt2 += t;

			t1_count++;
		}else
		{
			t = (ts->ts_fifo[1][i] & 0x0fff);
			zt3 += t;

			t = (ts->ts_fifo[1][i] >> 16) & 0x0fff;
			zt4 += t;

			t2_count++;
		}
	}
#if (CAL_SAMPLE_TIMES >= 3)
	xt = xt - xmin - xmax;
	yt = yt - ymin - ymax;

#endif
	xt /= (CAL_SAMPLE_TIMES - 2);
	yt /= (CAL_SAMPLE_TIMES - 2);


	if(t1_count > 0)
	{
		zt1 /= t1_count;
		zt2 /= t1_count;
		zt1 = Touch_Formula_One(zt1,zt2,xt,ts->x_r_plate);
	}
	if(t2_count)
	{
		zt3 /= t2_count;
		zt4 /= t2_count;
		zt3 = Touch_Formula_One(zt3,zt4,yt,jz_ts->y_r_plate);
	}
	if((t1_count) && (t2_count))
		z = (zt1 + zt3) / 2;
	else if(t1_count)
		z = zt1;
	else if(t2_count)
		z = zt3;
	else
		z = 0;
	ds->xbuf = xt;
	ds->ybuf = yt;
	ds->zbuf = z;

	return 1;

}

static void ts_transform_data(struct jz_ts_t *ts){
	#define FALSE	0
	#define TRUE    1
	struct ts_event *event = &ts->event;

	int x =transform_to_screen_x(ts,ts->data_s.xbuf,FALSE);
	int y =transform_to_screen_y(ts,ts->data_s.ybuf,TRUE);
	//pr_info("ts->data_s.xbuf = %d\n",ts->data_s.xbuf);
	//pr_info("ts->data_s.ybuf = %d\n",ts->data_s.ybuf);

	//ts_adjust(ts,&event->x,&event->y,ts->data_s.xbuf,ts->data_s.ybuf,FALSE,FALSE);
	ts_adjust(ts,&event->x,&event->y,x,y,FALSE);
	if(event->x > ts->x_resolution )
		event->x = ts->x_resolution;
	else if(event->x < 0)
		event->x = 0;
	if(event->y > ts->y_resolution)
		event->y = ts->y_resolution;
	else if(event->y < 0)
		event->y = 0;
	event->pressure = transform_to_screen_z(ts,ts->data_s.zbuf);
	if(event->pressure == 0) event->pressure = 1;
	#undef TRUE
	#undef FALSE
}

static void handle_ts_event(struct jz_ts_t *ts){
	struct ts_event *event = &ts->event;
	input_report_abs(ts->input_dev, ABS_X, event->x);
	input_report_abs(ts->input_dev, ABS_Y, event->y);
	input_report_abs(ts->input_dev, ABS_PRESSURE, event->pressure);

	input_report_key(ts->input_dev, BTN_TOUCH, 1);
	input_sync(ts->input_dev);

	//printk("event->x = %ld,event->y = %ld event->pressure = %d\n",event->x,event->y,event->pressure);
}

static void handle_touch(struct jz_ts_t *ts, unsigned int *data, int size){
	/* drop no touch calibrate points */
	if(ts->cal_type & (~TOUCH_TYPE))
		ts->cal_type |= ~TOUCH_TYPE;
	if(ts->event_valid){
		handle_ts_event(ts);
		ts->event_valid = 0;
	}

	if(ts->touch_cal_count >= DROP_SAMPLE_TIMES)
	{
		if(ts->touch_cal_count < ts->sample_num){
			ts->ts_fifo[0][ts->touch_cal_count - DROP_SAMPLE_TIMES] = data[0];
			ts->ts_fifo[1][ts->touch_cal_count - DROP_SAMPLE_TIMES] = data[1];

		}else
		{
			/* drop sample*/
			if(ts->cal_type & TOUCH_TYPE){
				if(ts_data_filter(ts)){
					ts->event_valid = 1;
					ts_transform_data(ts);
				}

			}
			ts->touch_cal_count = 0;
		}
	}
	ts->touch_cal_count++;
}





/*
 *---------------------new init method for platform_device (2013-07-20)--------
 */

/*
 *jz_pendown_irq_handler is used to handle pendown interrupt
 */
static irqreturn_t jz_pendown_irq_handler(int irq,void *devid)
{
	//pr_info("pen down irq : %d",jz_ts->pendw_irq);
	disable_irq_nosync(jz_ts->pendw_irq);
	//pr_info("pen down disable return");
	enable_irq(jz_ts->penup_irq);
	//pr_info("pen down get state %d",jz_adc_get_state(jz_ts->pdev->dev.parent));
	//pr_info("pen down get control %d",jz_adc_get_control(jz_ts->pdev->dev.parent));
	if(jz_ts->is_pen_down == 0)
	{
		jz_ts->is_pen_down = 1;
		#ifdef SLEEPMODE
		jz_ts->pen_down = 1;
		#endif
		jz_ts->event_valid = 0;
		jz_ts->cal_type |= TOUCH_TYPE;
		jz_ts->touch_cal_count = 0;
	}
	pr_info("pen down interrupt end");
	return IRQ_HANDLED;

}

/*
 *jz_penup_irq_handler is used to handle pen up interrupt
 */

static irqreturn_t jz_penup_irq_handler(int irq,void *devid)
{
	//pr_info("jz4780ts is entering pen up interrupt now ");
	disable_irq_nosync(jz_ts->penup_irq);
	enable_irq(jz_ts->pendw_irq);
	if(jz_ts->is_pen_down == 1)
	{
		input_report_abs(jz_ts->input_dev,ABS_PRESSURE,0);
		input_report_key(jz_ts->input_dev,BTN_TOUCH,0);
		input_sync(jz_ts->input_dev);
		jz_ts->cal_type &= ~TOUCH_TYPE;
		jz_ts->event_valid = 0;
	}
	jz_ts->is_pen_down = 0; //always set to //_ts_clear_penup_interrupt_flag(jz_ts->pdev->dev.parent); //clear pen up interrupt state

	return IRQ_HANDLED;
}
/*
 *jz_touch_work is the bh for the jz_touch_irq_handler
 */
static void jz_touch_work(struct work_struct *work)
{
	unsigned int fifo[FIFO_MAX_SIZE];
	struct jz_ts_t *ts = container_of(work,struct jz_ts_t,touch_work);
	pr_info("jz4780 touch work");
	fifo[0] = readl(jz_ts->io_base + TS_REG_ADTCH_OFFSET);
	fifo[1] = readl(jz_ts->io_base + TS_REG_ADTCH_OFFSET);
	if(jz_ts->is_pen_down)
		handle_touch(ts,fifo,2);

}

/*
 *jz_touch_irq_handler is used to handle touch interrupt
 */
static irqreturn_t jz_touch_irq_handler(int irq,void *devid)
{
	struct jz_ts_t *ts = (struct jz_ts_t *)devid;
	schedule_work(&ts->touch_work);
	return IRQ_HANDLED;
}

static irqreturn_t jz_slpdw_irq_handler(int irq,void *devid)//here maybe use a work queue or tasklet is better,i will modify if if need
{
	struct jz_ts_t  *ts =(struct jz_ts_t *)devid;
	uint32_t var = 0;
	pr_info("jz4780ts have entered pen down interrupt of sleep mode");
	if(ts->is_sleep_mode == 1)
	{
		adc_write_reg(ts->pdev->dev.parent,REG_ADC_CTRL,ADCTRL_SLPENDM,ADCTRL_SLPENDM);
		adc_write_reg(ts->pdev->dev.parent,REG_ADC_STATUS,ADSTATE_SLP_RDY,var);
		adc_write_reg(ts->pdev->dev.parent,REG_ADC_ENABLE,ADENA_SLP_MD,var);
		mdelay(1);
		var = adc_read_reg(jz_ts->pdev->dev.parent,REG_ADC_STATUS);
		if(var & ADSTATE_SLP_RDY)
			jz_ts->is_sleep_mode = 0;
	}
	return IRQ_HANDLED;
}
static void jz_slpd_monitor_work(struct work_struct *work)
{
	#if 0
	struct jz_ts_t *ts = container_of(work,struct jz_ts_t,slpmd_work.work);
	#endif
	uint32_t val = 0;
	struct delayed_work *delay_work = container_of(work,struct delayed_work,work);
	struct jz_ts_t *ts = container_of(delay_work,struct jz_ts_t,slpmd_work);
	if(ts->pen_down != 1 && ts->is_sleep_mode == 0)
	{

		//jz_adc_set_state(ts->pdev->dev.parent,(uint8_t)ADSTATE_SLP_RDY,val);
		adc_write_reg(ts->pdev->dev.parent,REG_ADC_STATUS,ADSTATE_SLP_RDY,val);
		adc_write_reg(ts->pdev->dev.parent,REG_ADC_ENABLE,ADENA_SLP_MD,ADENA_SLP_MD);

		mdelay(10);
		val = adc_read_reg(ts->pdev->dev.parent,REG_ADC_STATUS);
		if(val & ADSTATE_SLP_RDY)
		{
			ts->is_sleep_mode = 1;
			val = 0;
			adc_write_reg(ts->pdev->dev.parent,REG_ADC_STATUS,ADSTATE_SLPEND,ADSTATE_SLPEND);//write 1 to clear

			adc_write_reg(ts->pdev->dev.parent,REG_ADC_CTRL,ADCTRL_SLPENDM,val);

			val |= ADCTRL_PENDM | ADCTRL_PENUM |ADCTRL_DTCHM | ADCTRL_ARDYM | ADCTRL_VRDYM;
			adc_write_reg(ts->pdev->dev.parent,REG_ADC_CTRL,val,val);//mask all other interrupt ,it is not good idea,but must to do
		}
	}
	ts->pen_down = 0;
	pr_info("jz4780ts is entering sleep mode monitor Delayed work");
	schedule_delayed_work(&ts->slpmd_work,MONITOR_TIME *HZ);

}
static int __devinit jz4780ts_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct input_dev *input_dev;
	struct jz_ts_info *info ;//= pdev->dev.platform_data;
	jz_ts = kzalloc(sizeof(struct jz_ts_t), GFP_KERNEL);
	if(!jz_ts)
	{
		dev_err(&pdev->dev,"No platform device supplied\n");
		return -ENOMEM;
	}
	jz_ts->cell = mfd_get_cell(pdev);
	info = jz_ts->cell->platform_data;
	if(!info)
	{
		dev_err(&pdev->dev,"no cell  data,can't attach\n");
		return -EINVAL;
	}

	jz_ts->pdev = pdev;
	jz_ts->x_max = info->x_max;
	jz_ts->x_min = info->x_min;
	jz_ts->y_max = info->y_max;
	jz_ts->y_min = info->y_min;
	jz_ts->z_max = info->z_max;
	jz_ts->z_min = info->z_min;
	jz_ts->x_resolution = info->x_resolution;
	jz_ts->y_resolution = info->y_resolution;
	jz_ts->x_r_plate = info->x_r_plate;
	jz_ts->y_r_plate = info->y_r_plate;
	jz_ts->pressure_max = info->pressure_max;
	jz_ts->same_time_usec = SAME_TIME;
	jz_ts->wait_time_msec = WAIT_TIME;
	jz_ts->sample_num = SAMPLE_NUM;
	jz_ts->command_type = HARDWARE_CMD;
	jz_ts->is_pen_down = 0;
	jz_ts->cal_type = 0;
	input_dev = input_allocate_device();
	if(!input_dev)
	{
		dev_err(&pdev->dev,"alloc input device failed");
		return -ENOMEM;
	}
	input_dev->name = "jz4780_touchscreen";
	input_dev->phys = jz_ts->phys;
	set_bit(INPUT_PROP_DIRECT,input_dev->propbit);
	set_bit(EV_ABS,input_dev->evbit);
	set_bit(ABS_X,input_dev->absbit);
	set_bit(ABS_Y,input_dev->absbit);
	set_bit(ABS_PRESSURE,input_dev->absbit);
	set_bit(EV_KEY,input_dev->evbit);
	set_bit(EV_SYN,input_dev->evbit);
	set_bit(BTN_TOUCH,input_dev->keybit);

	input_set_abs_params(input_dev,ABS_X,0,jz_ts->x_resolution,0,0);
	input_set_abs_params(input_dev,ABS_Y,0,jz_ts->y_resolution,0,0);
	input_set_abs_params(input_dev,ABS_PRESSURE,0,jz_ts->pressure_max,0,0);
	input_set_drvdata(input_dev,jz_ts);
	ret = input_register_device(input_dev);
	if(ret)
	{
		dev_err(&pdev->dev,"ts failed to register input dev");
		goto err_free_dev;
	}
	strcpy(jz_ts->phys,"input/ts0");
	spin_lock_init(&jz_ts->lock);
	jz_ts->input_dev = input_dev;
	jz_ts->pendw_irq = platform_get_irq(pdev,0);
	if(jz_ts->pendw_irq < 0)
	{
		dev_err(&pdev->dev,"ts failed to get pen down irq: %d",jz_ts->pendw_irq);
		goto err_free;
	}
	//pr_info("init pen down irq :%d ",jz_ts->pendw_irq);
	jz_ts->penup_irq = platform_get_irq(pdev,1);
	if(jz_ts->penup_irq < 0)
	{
		dev_err(&pdev->dev,"ts failed to get pen up irq:%d",jz_ts->penup_irq);
		goto err_free;

	}
	jz_ts->dtch_irq = platform_get_irq(pdev,2);
	if(jz_ts->dtch_irq < 0)
	{
		dev_err(&pdev->dev,"ts failed to get touch irq:%d",jz_ts->dtch_irq);
		goto err_free;
	}
	#ifdef SLEEPMODE
	jz_ts->slpendw_irq = platform_get_irq(pdev,3);
	if(jz_ts->slpendw_irq < 0)
	{
		dev_err(&pdev->dev,"ts failed to get sleep mode pendown irq:%d",jz_ts->slpendw_irq);
		goto err_free;
	}
	#endif
	jz_ts->mem = platform_get_resource(pdev,IORESOURCE_MEM,0);
	if(!jz_ts->mem)
	{
		ret = -ENOENT;
		dev_err(&pdev->dev,"ts failed to get MEMIO resource");
		goto err_free;
	}
	jz_ts->mem = request_mem_region(jz_ts->mem->start,resource_size(jz_ts->mem),pdev->name);
	if(!jz_ts->mem)
	{
		ret = -EBUSY;
		dev_err(&pdev->dev,"ts failed to request MEMIO region");
		goto err_free;
	}
	jz_ts->io_base = ioremap_nocache(jz_ts->mem->start,resource_size(jz_ts->mem));
	if(!jz_ts->io_base)
	{
		ret = -EBUSY;
		dev_err(&pdev->dev,"ts failed to ioremap mmio");
		goto err_release_mem;
	}
	jz_ts->adc_cmd_mem = platform_get_resource(pdev,IORESOURCE_MEM,1);
	if(!jz_ts->adc_cmd_mem)
	{
		ret = -ENOENT;
		dev_err(&pdev->dev,"ts failed to get adc command register  io mem resource");
		goto err_release_mem;
	}
	jz_ts->adc_cmd_mem = request_mem_region(jz_ts->adc_cmd_mem->start,resource_size(jz_ts->adc_cmd_mem),"jz4780-ts-adc-command");
	if(!jz_ts->adc_cmd_mem)
	{
		ret = -EBUSY;
		dev_err(&pdev->dev,"ts failed to request adc command register IO memory resource ");
		goto err_release_mem;
	}
	jz_ts->io_cmd_base = ioremap_nocache(jz_ts->adc_cmd_mem->start,resource_size(jz_ts->adc_cmd_mem));
	if(!jz_ts->io_cmd_base)
	{
		ret = -EBUSY;
		dev_err(&pdev->dev,"ts failed to ioremap adc command register memory io");
		goto err_release_command_mem;
	}
	#ifdef SLEEPMODE
	INIT_DELAYED_WORK(&jz_ts->slpmd_work,jz_slpd_monitor_work);
	jz_ts->pen_down = 0;
	jz_ts->last_pen_down = 0;
	jz_ts->is_sleep_mode = 0;
	#endif
	pr_info("before init work for touch jz4780");
	INIT_WORK(&jz_ts->touch_work,jz_touch_work);
	pr_info("after init work for touch jz4780");

	jz_cal.a0 = 1;
	jz_cal.a1 = 0;
	jz_cal.a2 = 0;
	jz_cal.a3 = 0;
	jz_cal.a4 = 1;
	jz_cal.a5 = 0;
	jz_cal.a6 = 1;

	ret = request_irq(jz_ts->pendw_irq,jz_pendown_irq_handler,0,"jz4780_ts_pendw_irq",jz_ts);
	if(ret)
	{
		dev_err(&pdev->dev,"ts failed to request irq  for pendown");
		goto err_free_pendw_irq;

	}
	ret = request_irq(jz_ts->penup_irq,jz_penup_irq_handler,0,"jz4780_ts_penup_irq",jz_ts);
	if(ret)
	{
		dev_err(&pdev->dev,"ts failed to request irq for pen up" );
		goto err_free_penup_irq;
	}
	disable_irq(jz_ts->penup_irq);
	ret = request_irq(jz_ts->dtch_irq,jz_touch_irq_handler,0,"jz4780_ts_touch_irq",jz_ts);
	if(ret)
	{
		dev_err(&pdev->dev,"ts failed to request irq for touch");
		goto err_free_dtch_irq;
	}

	#ifdef SLEEPMODE
	ret = request_irq(jz_ts->slpendw_irq,jz_slpdw_irq_handler,0,"jz4780_ts_sleepmode_irq",jz_ts);

	if(ret)
	{
		dev_err(&pdev->dev,"ts failed to request irq for sleep mode");
		goto err_free_slpendw_irq;
	}
	#endif
	//pr_info("probe get state one is %d",jz_adc_get_state(pdev->dev.parent));
	jz_ts->cell->enable(pdev);
	//pr_info("probe get state two is %d",jz_adc_get_state(pdev->dev.parent));
	jz_sadc_start_ts(pdev->dev.parent,jz_ts);
	#ifdef SLEEPMODE
	schedule_delayed_work(&jz_ts->slpmd_work,MONITOR_TIME * HZ);
	#endif
	pr_info("jz4780ts  init success by proble method!");
	return 0;
#ifdef SLEEPMODE
err_free_slpendw_irq:
	free_irq(jz_ts->slpendw_irq,jz_ts);
	cancel_delayed_work_sync(&jz_ts->slpmd_work);
#endif
err_free_dtch_irq:
	free_irq(jz_ts->dtch_irq,jz_ts);
err_free_penup_irq:
	free_irq(jz_ts->penup_irq,jz_ts);
err_free_pendw_irq:
	free_irq(jz_ts->pendw_irq,jz_ts);
err_iounmap:
	platform_set_drvdata(pdev,NULL);
	iounmap(jz_ts->io_cmd_base);
	iounmap(jz_ts->io_base);
err_release_command_mem:
	release_mem_region(jz_ts->adc_cmd_mem->start,resource_size(jz_ts->adc_cmd_mem));
err_release_mem:
	release_mem_region(jz_ts->mem->start,resource_size(jz_ts->mem));
err_free:
	kfree(jz_ts);
err_free_dev:
	input_free_device(input_dev);
	return ret;



}

static int  __devexit jz4780ts_remove(struct platform_device *pdev)
{
	jz_ts->cell->disable(pdev);
	free_irq(jz_ts->pendw_irq,jz_ts);
	free_irq(jz_ts->penup_irq,jz_ts);
	free_irq(jz_ts->dtch_irq,jz_ts);
	iounmap(jz_ts->io_base);
	iounmap(jz_ts->io_cmd_base);
	release_mem_region(jz_ts->mem->start,resource_size(jz_ts->mem));
	release_mem_region(jz_ts->adc_cmd_mem->start,resource_size(jz_ts->adc_cmd_mem));
	input_unregister_device(jz_ts->input_dev);
	#ifdef SLEEPMODE
	free_irq(jz_ts->slpendw_irq,jz_ts);
	cancel_delayed_work_sync(&jz_ts->slpmd_work);
	#endif
	cancel_work_sync(&jz_ts->touch_work);
	kfree(jz_ts);
	return 0;

}

static struct platform_driver jz4780_ts_driver ={
	.driver = {
		.name = "jz4780-ts",
		.owner = THIS_MODULE,
	},
	.probe = jz4780ts_probe,
	.remove = __devexit_p(jz4780ts_remove),
};

static int __init jz4780ts_init(void)
{
	int ret;
	ret = platform_driver_register(&jz4780_ts_driver);
	if(ret)
	{
		printk(KERN_ERR"ts could't register platform touch screen driver\n");
		goto err_dev_register;
	}
	ret = driver_create_file(&jz4780_ts_driver.driver,&driver_attr_cal_a0);
	if(ret)
	{
		printk(KERN_ERR"ts create Sysfs failed for cal_a0\n");
		goto err_create_cal_a0;
	}
	ret = driver_create_file(&jz4780_ts_driver.driver,&driver_attr_cal_a1);
	if(ret)
	{
		printk(KERN_ERR"ts create Sysfs failed for cal_a1\n");
		goto err_create_cal_a1;
	}
	ret = driver_create_file(&jz4780_ts_driver.driver,&driver_attr_cal_a2);
	if(ret)
	{
		printk(KERN_ERR"ts create Sysfs failed for cal_a2\n");
		goto err_create_cal_a2;
	}
	ret = driver_create_file(&jz4780_ts_driver.driver,&driver_attr_cal_a3);
	if(ret)
	{
		printk(KERN_ERR"ts create Sysfs failed for cal_a3\n");
		goto err_create_cal_a3;
	}
	ret = driver_create_file(&jz4780_ts_driver.driver,&driver_attr_cal_a4);
	if(ret)
	{
		printk(KERN_ERR"ts create Sysfs failed for cal_a4\n");
		goto err_create_cal_a4;
	}
	ret = driver_create_file(&jz4780_ts_driver.driver,&driver_attr_cal_a5);
	if(ret)
	{
		printk(KERN_ERR"ts create Sysfs failed for cal_a5\n");
		goto err_create_cal_a5;
	}
	ret = driver_create_file(&jz4780_ts_driver.driver,&driver_attr_cal_a6);
	if(ret)
	{
		printk(KERN_ERR"ts create Sysfs failed for cal_a6\n");
		goto err_create_cal_a6;
	}
	return ret;
err_create_cal_a6:
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a5);
err_create_cal_a5:
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a4);
err_create_cal_a4:
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a3);
err_create_cal_a3:
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a2);
err_create_cal_a2:
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a1);
err_create_cal_a1:
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a0);
err_create_cal_a0:
err_dev_register:
	return ret;

}

static void __exit jz4780ts_exit(void)
{
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a0);
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a1);
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a2);
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a3);
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a4);
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a5);
	driver_remove_file(&jz4780_ts_driver.driver,&driver_attr_cal_a6);
	platform_driver_unregister(&jz4780_ts_driver);
}

module_init(jz4780ts_init);
module_exit(jz4780ts_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("JZ TouchScreen Driver");
MODULE_AUTHOR("vincent <junyang@ingenic.com>");
