/*---------------------------------------------------------------------------------------------------------
 * kernel/include/linux/gt801_touch.h
 *
 * Copyright(c) 2010 Goodix Technology Corp. All rights reserved.      
 * Author: Eltonny
 * Date: 2010.11.11                                    
 *                                                                                                         
 *---------------------------------------------------------------------------------------------------------*/

#ifndef 	_LINUX_GT801_TOUCH_H
#define		_LINUX_GT801_TOUCH_H

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>

#define GT801_NAME "gt801_ts"

#define TOUCH_MAX_HEIGHT 	7680	
#define TOUCH_MAX_WIDTH	 	5120

#ifdef CONFIG_TOUCHSCREEN_HD_LCM
#define SCREEN_MAX_HEIGHT	1024				
#define SCREEN_MAX_WIDTH	600
#else
#define SCREEN_MAX_HEIGHT	800		
#define SCREEN_MAX_WIDTH	480
#endif

#define FLAG_UP 	0
#define FLAG_DOWN 	1
#define FLAG_MASK 0X01
#define FLAG_INVALID 2


#ifndef CONFIG_TOUCHSCREEN_JZ_GT801_MULTI_TOUCH
	#define MAX_FINGER_NUM 1
#else
	#define MAX_FINGER_NUM 5					//最大支持手指数(<=5)
#endif

#if MAX_FINGER_NUM <= 3
#define READ_BYTES_NUM 1+2+MAX_FINGER_NUM*5
#elif MAX_FINGER_NUM == 4
#define READ_BYTES_NUM 1+28
#elif MAX_FINGER_NUM == 5
#define READ_BYTES_NUM 1+34
#endif  
 
#undef GT801_TS_DEBUG

#define swap_xy(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)

struct gt801_gpio {
	struct jztsc_pin *irq;
	struct jztsc_pin *shutdown;
	struct jztsc_pin *drven;
};

struct gt801_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	uint8_t use_irq;
	uint8_t use_shutdown;
	struct hrtimer timer;
	struct work_struct  work;
	struct work_struct  work_init;
	char phys[32];
	int bad_data;
	int retry;
	struct early_suspend early_suspend;
	int (*power)(struct gt801_ts_data * ts, int on);
	struct timer_list penup_timeout_timer;
	int is_suspend;
        int work_count;
	struct gt801_gpio gpio;
};

struct gt801_ts_platform_data {
	u16 intr;
	uint32_t version;	/* Use this entry for panels with */
};


struct point_node
{
	uint8_t num;
	uint8_t id;
	uint8_t state;
	uint8_t pressure;
	unsigned int x;
	unsigned int y;
};

struct point_queue
{
	int length;
	struct point_node pointer[MAX_FINGER_NUM];
};


#endif /* _LINUX_GT801_TOUCH_H */
