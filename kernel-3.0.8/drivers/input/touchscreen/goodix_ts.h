/*
 * include/linux/goodix_touch.h
 *
 * Copyright (C) 2010 - 2011 Goodix, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef 	_LINUX_GOODIX_TOUCH_H
#define		_LINUX_GOODIX_TOUCH_H

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>

//*************************TouchScreen Work Part*****************************

#define GOODIX_I2C_NAME "Goodix-TS"
//define resolution of the touchscreen
#define TOUCH_MAX_HEIGHT 	800			
#define TOUCH_MAX_WIDTH		480

//#define INT_PORT		((GPIOA_bank_bit0_27(16)<<16) | GPIOA_bit_bit0_27(16))		//S3C64XX_GPN(15)	//Int IO port  S3C64XX_GPL(10)
#define INT_PORT 1988
#ifdef INT_PORT
//	#define TS_INT 		INT_GPIO_2  //Interrupt Number,EINT18(119)
        #define TS_INT         18
#else
	#define TS_INT	0
#endif	

//whether need send cfg?
//#define DRIVER_SEND_CFG

//set trigger mode
#define INT_TRIGGER			3

#define POLL_TIME		10	//actual query spacing interval:POLL_TIME+6

#define GOODIX_MULTI_TOUCH

#ifdef GOODIX_MULTI_TOUCH
	#define MAX_FINGER_NUM	10	
#else
	#define MAX_FINGER_NUM	1	
#endif

//#define swap(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)

struct goodix_gpio {
	struct jztsc_pin *irq;
	struct jztsc_pin *wake;
	struct jztsc_pin *power;
};

struct goodix_ts_data {
	uint16_t addr;
	uint8_t bad_data;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_reset;		//use RESET flag
	int use_irq;		//use EINT flag
	int read_mode;		//read moudle mode,20110221 by andrew
	struct hrtimer timer;
	struct work_struct  work;
	char phys[32];
	int retry;
	struct early_suspend early_suspend;
	int (*power)(struct i2c_client * client, struct goodix_ts_data * ts, int on);
	uint16_t abs_x_max;
	uint16_t abs_y_max;
	uint8_t max_touch_num;
	uint8_t int_trigger_type;
	bool first_irq;
	struct goodix_gpio gpio;
};

//*****************************End of Part I *********************************

//*************************Touchkey Surpport Part*****************************
//#define HAVE_TOUCH_KEY
#undef HAVE_TOUCH_KEY
#ifdef HAVE_TOUCH_KEY
	#define READ_COOR_ADDR 0x00
	static uint16_t tp_key_array2[]={
									  KEY_NUMLOCK,				//MENU
									  KEY_SYSRQ,				//HOME
									  KEY_DELETE				//CALL
									 }; 
	#define MAX_KEY_NUM	 (sizeof(tp_key_array2)/sizeof(tp_key_array2[0]))
#else
	#define READ_COOR_ADDR 0x01
#endif
//*****************************End of Part II*********************************

//*************************Firmware Update part*******************************
#define CONFIG_TOUCHSCREEN_GOODIX_IAP
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
//#define UPDATE_NEW_PROTOCOL

#define	MAX_XX 9600
#define MAX_YY 6400

#define PACK_SIZE 					64					//update file package size
#define MAX_TIMEOUT					60000				//update time out conut
#define MAX_I2C_RETRIES				20					//i2c retry times

//I2C buf address
#define ADDR_CMD					80
#define ADDR_STA					81
#ifdef UPDATE_NEW_PROTOCOL
	#define ADDR_DAT				0
#else
	#define ADDR_DAT				82
#endif

//moudle state
#define NEW_UPDATE_START			0x01
#define UPDATE_START				0x02
#define SLAVE_READY					0x08
#define UNKNOWN_ERROR				0x00
#define FRAME_ERROR					0x10
#define CHECKSUM_ERROR				0x20
#define TRANSLATE_ERROR				0x40
#define FLASH_ERROR					0X80

//error no
#define ERROR_NO_FILE				2	//ENOENT
#define ERROR_FILE_READ				23	//ENFILE
#define ERROR_FILE_TYPE				21	//EISDIR
#define ERROR_GPIO_REQUEST			4	//EINTR
#define ERROR_I2C_TRANSFER			5	//EIO
#define ERROR_NO_RESPONSE			16	//EBUSY
#define ERROR_TIMEOUT				110	//ETIMEDOUT

//update steps
#define STEP_SET_PATH              1
#define STEP_CHECK_FILE            2
#define STEP_WRITE_SYN             3
#define STEP_WAIT_SYN              4
#define STEP_WRITE_LENGTH          5
#define STEP_WAIT_READY            6
#define STEP_WRITE_DATA            7
#define STEP_READ_STATUS           8
#define FUN_CLR_VAL                9
#define FUN_CMD                    10
#define FUN_WRITE_CONFIG           11

//fun cmd
#define CMD_DISABLE_TP             0
#define CMD_ENABLE_TP              1
#define CMD_READ_VER               2
#define CMD_READ_RAW               3
#define CMD_READ_DIF               4
#define CMD_READ_CFG               5
#define CMD_SYS_REBOOT             101

//read mode
#define MODE_RD_VER                1
#define MODE_RD_RAW                2
#define MODE_RD_DIF                3
#define MODE_RD_CFG                4


#endif
//*****************************End of Part III********************************
//struct goodix_i2c_platform_data {
//	uint32_t version;	/* Use this entry for panels with */
	//reservation
//};

struct goodix_i2c_rmi_platform_data  
{
	uint32_t version;
	int (*power)(int on);
	void (*rst)();
	void (*irq_init)();
	unsigned long irq;
	uint32_t flags;

};



#endif /* _LINUX_GOODIX_TOUCH_H */
