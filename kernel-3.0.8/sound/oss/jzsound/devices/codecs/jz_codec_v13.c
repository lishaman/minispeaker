/*
 * Linux/sound/oss/xb47XX/codec/jz_codec_v12.c
 *
 * CODEC CODEC driver for Ingenic m200 MIPS processor
 *
 * 2010-11-xx   jbbi <jbbi@ingenic.cn>
 *
 * Copyright (c) Ingenic Semiconductor Co., Ltd.
 */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/sound.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <linux/proc_fs.h>
#include <linux/soundcard.h>
#include <linux/dma-mapping.h>
#include <linux/earlysuspend.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/gpio.h>
#include <linux/vmalloc.h>
#include <linux/semaphore.h>

#include "../xb47xx_i2s_v13.h"
#include "jz_codec_v13.h"
#include "jz_route_conf_v13.h"

/*###############################################*/
#define CODEC_DUMP_IOC_CMD		0
#define CODEC_DUMP_ROUTE_REGS		0
#define CODEC_DUMP_ROUTE_PART_REGS	0
#define CODEC_DUMP_GAIN_PART_REGS	0
#define CODEC_DUMP_ROUTE_NAME		0
#define CODEC_DUMP_GPIO_STATE		0
/*##############################################*/

/***************************************************************************************\
 *global variable and structure interface                                              *
\***************************************************************************************/

static struct snd_board_route *cur_route = NULL;
static struct snd_board_route *keep_old_route = NULL;
static struct snd_codec_data *codec_platform_data = NULL;
static int user_replay_volume = 50;
static int user_record_volume = 50;

unsigned int cur_out_device = -1;


#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
#endif

extern int i2s_register_codec(char *name, void *codec_ctl,unsigned long codec_clk,enum codec_mode mode);
extern int i2s_register_codec_2(struct codec_info *codec_dev);
static int codec_set_record_volume(struct codec_info *codec_dev, int *val);

/*==============================================================*/
/**
 * codec_sleep
 *
 *if use in suspend and resume, should use delay
 */

static int g_codec_sleep_mode = 1;

void codec_sleep(struct codec_info *codec_dev, int ms)
{
	if(!g_codec_sleep_mode)
		mdelay(ms);
	else
		msleep(ms);
}

/***************************************************************************************\
 *debug part                                                                           *
\***************************************************************************************/

#if CODEC_DUMP_IOC_CMD
static void codec_print_ioc_cmd(int cmd)
{
	int i;

	int cmd_arr[] = {
		CODEC_INIT,					CODEC_TURN_OFF,
		CODEC_SHUTDOWN,				CODEC_RESET,
		CODEC_SUSPEND,				CODEC_RESUME,
		CODEC_ANTI_POP,				CODEC_SET_DEFROUTE,
		CODEC_SET_DEVICE,			CODEC_SET_RECORD_RATE,
		CODEC_SET_RECORD_DATA_WIDTH,	CODEC_SET_MIC_VOLUME,
		CODEC_SET_RECORD_CHANNEL,		CODEC_SET_REPLAY_RATE,
		CODEC_SET_REPLAY_DATA_WIDTH,	CODEC_SET_REPLAY_VOLUME,
		CODEC_SET_REPLAY_CHANNEL,	CODEC_DAC_MUTE,
		CODEC_DEBUG_ROUTINE,		CODEC_SET_STANDBY,
		CODEC_GET_RECORD_FMT_CAP,		CODEC_GET_RECORD_FMT,
		CODEC_GET_REPLAY_FMT_CAP,	CODEC_GET_REPLAY_FMT,
		CODEC_IRQ_HANDLE,			CODEC_DUMP_REG,
		CODEC_DUMP_GPIO
	};

	char *cmd_str[] = {
		"CODEC_INIT", 			"CODEC_TURN_OFF",
		"CODEC_SHUTDOWN", 		"CODEC_RESET",
		"CODEC_SUSPEND",		"CODEC_RESUME",
		"CODEC_ANTI_POP",		"CODEC_SET_DEFROUTE",
		"CODEC_SET_DEVICE",		"CODEC_SET_RECORD_RATE",
		"CODEC_SET_RECORD_DATA_WIDTH", 	"CODEC_SET_MIC_VOLUME",
		"CODEC_SET_RECORD_CHANNEL", 	"CODEC_SET_REPLAY_RATE",
		"CODEC_SET_REPLAY_DATA_WIDTH", 	"CODEC_SET_REPLAY_VOLUME",
		"CODEC_SET_REPLAY_CHANNEL", 	"CODEC_DAC_MUTE",
		"CODEC_DEBUG_ROUTINE",		"CODEC_SET_STANDBY",
		"CODEC_GET_RECORD_FMT_CAP",		"CODEC_GET_RECORD_FMT",
		"CODEC_GET_REPLAY_FMT_CAP",	"CODEC_GET_REPLAY_FMT",
		"CODEC_IRQ_HANDLE",		"CODEC_DUMP_REG",
		"CODEC_DUMP_GPIO"
	};

	for ( i = 0; i < sizeof(cmd_arr) / sizeof(int); i++) {
		if (cmd_arr[i] == cmd) {
			printk("CODEC IOC: Command name : %s\n", cmd_str[i]);
			return;
		}
	}

	if (i == sizeof(cmd_arr) / sizeof(int)) {
		printk("CODEC IOC: command is not under control\n");
	}
}
#endif //CODEC_DUMP_IOC_CMD

#if CODEC_DUMP_ROUTE_NAME
static void codec_print_route_name(int route)
{
	int i;
	int route_arr[] = {
		SND_ROUTE_NONE,
		SND_ROUTE_ALL_CLEAR,
		SND_ROUTE_REPLAY_CLEAR,
		SND_ROUTE_RECORD_CLEAR,
		SND_ROUTE_RECORD_AMIC,
		SND_ROUTE_RECORD_DMIC,
		SND_ROUTE_REPLAY_SPK,
		SND_ROUTE_RECORD_AMIC_AND_REPLAY_SPK,
		SND_ROUTE_REPLAY_SOUND_MIXER_LOOPBACK,
		SND_ROUTE_LINEIN_REPLAY_MIXER_LOOPBACK,
		SND_ROUTE_AMIC_RECORD_MIX_REPLAY_LOOPBACK,
		SND_ROUTE_DMIC_RECORD_MIX_REPLAY_LOOPBACK
	};

	char *route_str[] = {
		"SND_ROUTE_NONE",
		"SND_ROUTE_ALL_CLEAR",
		"SND_ROUTE_REPLAY_CLEAR",
		"SND_ROUTE_RECORD_CLEAR",
		"SND_ROUTE_RECORD_AMIC",
		"SND_ROUTE_RECORD_DMIC",
		"SND_ROUTE_REPLAY_SPK",
		"SND_ROUTE_RECORD_AMIC_AND_REPLAY_SPK",
		"SND_ROUTE_REPLAY_SOUND_MIXER_LOOPBACK",
		"SND_ROUTE_LINEIN_REPLAY_MIXER_LOOPBACK",
		"SND_ROUTE_AMIC_RECORD_MIX_REPLAY_LOOPBACK",
		"SND_ROUTE_DMIC_RECORD_MIX_REPLAY_LOOPBACK"
	};

	for ( i = 0; i < sizeof(route_arr) / sizeof(unsigned int); i++) {
		if (route_arr[i] == route) {
			printk("\nCODEC SET ROUTE: Route name : %s,%d\n", route_str[i],i);
			return;
		}
	}

	if (i == sizeof(route_arr) / sizeof(unsigned int)) {
		printk("\nCODEC SET ROUTE: Route %d is not configed yet! \n",route);
	}
}
#endif //CODEC_DUMP_ROUTE_NAME
static void dump_gpio_state(struct codec_info *codec_dev)
{
	int val = -1;

	if(codec_platform_data &&
	   (codec_platform_data->gpio_spk_en.gpio != -1)) {
		val = __gpio_get_value(codec_platform_data->gpio_spk_en.gpio);
		printk("gpio speaker enable %d statue is %d.\n",codec_platform_data->gpio_spk_en.gpio, val);
	}
}

static void dump_codec_regs(struct codec_info *codec_dev)
{
	unsigned int i;
        unsigned char data;
        printk("codec register list:\n");
        for (i = 0; i <= 0x39; i++) {
		data = icdc_d3_hw_read(codec_dev, i);
                printk("address = 0x%02x, data = 0x%02x\n", i, data);
        }
	for (i = 0; i <= 4; i++){
		data = icdc_d3_hw_read(codec_dev, (SCODA_MIX_0 + i));
		printk("mix_%02x data = 0x%02x\n", i, data);
	}
}

#if CODEC_DUMP_ROUTE_PART_REGS
static void dump_codec_route_regs(struct codec_info *codec_dev)
{
	unsigned int i;
	unsigned char data;
	for (i = 0xb; i < 0x1d; i++) {
		data = __read_codec_reg(codec_dev, i);
		printk("address = 0x%02x, data = 0x%02x\n", i, data);
	}
}
#endif

#if CODEC_DUMP_GAIN_PART_REGS
static void dump_codec_gain_regs(struct codec_info *codec_dev)
{
	unsigned int i;
	unsigned char data;
	for (i = 0x28; i < 0x37; i++) {
		data = __read_codec_reg(codec_dev, i);
		printk("address = 0x%02x, data = 0x%02x\n", i, data);
	}
}
#endif
/*=========================================================*/

#if CODEC_DUMP_ROUTE_NAME
#define DUMP_ROUTE_NAME(route) codec_print_route_name(route)
#else
#define DUMP_ROUTE_NAME(route)
#endif

/*-------------------*/
#if CODEC_DUMP_IOC_CMD
#define DUMP_IOC_CMD(value)						\
	do {								\
		printk("[codec IOCTL]++++++++++++++++++++++++++++\n");	\
		printk("%s  cmd = %d, arg = %lu-----%s\n", __func__, cmd, arg, value); \
		codec_print_ioc_cmd(cmd);				\
		printk("[codec IOCTL]----------------------------\n");	\
	} while (0)
#else //CODEC_DUMP_IOC_CMD
#define DUMP_IOC_CMD(value)
#endif //CODEC_DUMP_IOC_CMD

#if CODEC_DUMP_GPIO_STATE
#define DUMP_GPIO_STATE(codec_dev)			\
	do {					\
		dump_gpio_state(codec_dev);		\
	} while (0)
#else
#define DUMP_GPIO_STATE(codec_dev)
#endif

#if CODEC_DUMP_ROUTE_REGS
#define DUMP_ROUTE_REGS(codec_dev, value)						\
	do {								\
		printk("codec register dump,%s\tline:%d-----%s:\n",	\
		       __func__, __LINE__, value);			\
		dump_codec_regs(codec_dev);					\
	} while (0)
#else //CODEC_DUMP_ROUTE_REGS
#define DUMP_ROUTE_REGS(codec_dev, value)				\
	do {						\
		if (!strcmp("enter",value))		\
			ENTER_FUNC();			\
		else if (!strcmp("leave",value))	\
			LEAVE_FUNC();			\
	} while (0)
#endif //CODEC_DUMP_ROUTE_REGS

#if CODEC_DUMP_ROUTE_PART_REGS
#define DUMP_ROUTE_PART_REGS(codec_dev, value)					\
	do {								\
		if (mode != DISABLE) {					\
			printk("codec register dump,%s\tline:%d-----%s:\n", \
			       __func__, __LINE__, value);		\
			dump_codec_route_regs(codec_dev);			\
		}							\
	} while (0)
#else //CODEC_DUMP_ROUTE_PART_REGS
#define DUMP_ROUTE_PART_REGS(codec_dev, value)			\
	do {						\
		if (!strcmp("enter",value))		\
			ENTER_FUNC();			\
		else if (!strcmp("leave",value))	\
			LEAVE_FUNC();			\
	} while (0)
#endif //CODEC_DUMP_ROUTE_PART_REGS

#if CODEC_DUMP_GAIN_PART_REGS
#define DUMP_GAIN_PART_REGS(codec_dev, value)					\
	do {								\
		printk("codec register dump,%s\tline:%d-----%s:\n",	\
		       __func__, __LINE__, value);			\
		dump_codec_gain_regs(codec_dev);					\
	} while (0)
#else //CODEC_DUMP_GAIN_PART_REGS
#define DUMP_GAIN_PART_REGS(codec_dev, value)			\
	do {						\
		if (!strcmp("enter",value))		\
			ENTER_FUNC();			\
		else if (!strcmp("leave",value))	\
			LEAVE_FUNC();			\
	} while (0)
#endif //CODEC_DUMP_GAIN_PART_REGS

/***************************************************************************************\
 *route part and attibute                                                              *
\***************************************************************************************/

#ifdef CONFIG_HAS_EARLYSUSPEND
static void codec_early_suspend(struct early_suspend *handler)
{
	//  __codec_switch_sb_micbias1(codec_dev, POWER_OFF);
	//  __codec_switch_sb_micbias2(codec_dev, POWER_OFF);
}

static void codec_late_resume(struct early_suspend *handler)
{
        //__codec_switch_sb_micbias1(codec_dev, POWER_ON);
	// __codec_switch_sb_micbias2(codec_dev, POWER_ON);
}
#endif
/***************************************************************************************\
 *route part and attibute                                                              *
\***************************************************************************************/
/*=========================codec prepare==========================*/
static void codec_prepare_ready(struct codec_info * codec_dev, int mode)
{
	int tmpval;
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");
	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_VIC);
	tmpval &= ~0x3;
	icdc_d3_hw_write(codec_dev,SCODA_REG_CR_VIC,tmpval);

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_CK);
	tmpval &= ~(0x1f|1<<4);
	tmpval |= 1<<6;
	icdc_d3_hw_write(codec_dev,SCODA_REG_CR_CK,tmpval);

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_AICR_ADC);
	tmpval |= 0x3;
	icdc_d3_hw_write(codec_dev,SCODA_REG_AICR_ADC,tmpval);

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_AICR_DAC);
	tmpval |= 0x3;
	tmpval &= ~0x20;
	icdc_d3_hw_write(codec_dev,SCODA_REG_AICR_DAC,tmpval);

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_IMR);
	tmpval |= (1|1<<2|1<<7);
	icdc_d3_hw_write(codec_dev,SCODA_REG_IMR,tmpval);

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_IMR2);
	tmpval |= (1<<4);
	icdc_d3_hw_write(codec_dev,SCODA_REG_IMR2,tmpval);
	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}

/*=================route part functions======================*/

static void codec_set_micbais(struct codec_info *codec_dev, int val){

	int tmpval;

	DUMP_ROUTE_PART_REGS(codec_dev, "enter");
	if(val){
		tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIC1);
		tmpval &= ~(1<<5);
		icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIC1,tmpval);
	}else{
		tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIC1);
		tmpval |= (1<<5);
		icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIC1,tmpval);
	}
	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}


/* select mic mode */
static void codec_set_mic(struct codec_info *codec_dev, int mode)
{
	int tmpval;
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");
	switch(mode){
	case AMIC_ENABLE:
		tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIC1);
		tmpval &= ~(7<<3);
		tmpval |= 1<<6;
		icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIC1,tmpval);
		msleep(100);
		break;
	case DMIC_ENABLE:
		tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_DMIC);
		tmpval |= 1<<7;
		icdc_d3_hw_write(codec_dev,SCODA_REG_CR_DMIC,tmpval);
		msleep(100);
		break;
	case LINEIN_ENABLE:
		tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIC1);
		tmpval &= ~(0xf<<3);
		tmpval |= 1<<5;
		icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIC1,tmpval);
		msleep(100);
		break;

	case MIC_DISABLE:
		tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIC1);
		tmpval |= (3<<4);
		icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIC1,tmpval);
		tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_DMIC);
		tmpval &= ~(1<<7);
		icdc_d3_hw_write(codec_dev,SCODA_REG_CR_DMIC,tmpval);
		break;

	default:
		printk("JZ_CODEC: line: %d, mic1 mode error!\n", __LINE__);
	}
	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}


static void codec_set_aic_adc(struct codec_info *codec_dev, int mode)
{
	int tmpval;
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");
	switch (mode) {
	case INPUT_TO_ADC_ENABLE:
		tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_AICR_ADC);
		tmpval &= ~(1<<4);
		icdc_d3_hw_write(codec_dev,SCODA_REG_AICR_ADC,tmpval);
		break;
	case INPUT_TO_ADC_DISABLE:
		tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_AICR_ADC);
		tmpval |= (1<<4);
		icdc_d3_hw_write(codec_dev,SCODA_REG_AICR_ADC,tmpval);
		break;
	default :
		printk("JZ_CODEC: line: %d, inputl mode error!\n", __LINE__);
	}
	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}

/*adc mux*/
static void codec_set_record_mux_mode(struct codec_info *codec_dev, int mode)
{
	int tmpval;
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");

	switch(mode){
		case RECORD_NORMAL_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_2);
			tmpval &= ~(0xf<<4 | 0x1);
			tmpval |= 0x1<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_2, tmpval);
			break;

		case RECORD_CROSS_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_2);
			tmpval &= ~(0xf<<4 | 0x1);
			tmpval |= 0x5<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_2, tmpval);
			break;

		case RECORD_INPUT_AND_MIXER_NORMAL:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_2);
			tmpval &= ~(0xf<<4);
			tmpval |= 0x1;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_2, tmpval);
			break;

		case RECORD_INPUT_AND_MIXER_CROSS:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_2);
			tmpval &= ~(0xf<<4);
			tmpval |= (0x5<<4 | 0x1);
			icdc_d3_hw_write(codec_dev,SCODA_MIX_2,tmpval);
			break;

		case RECORD_INPUT_AND_MIXER_L_CROSS_R_NORMAL:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_2);
			tmpval &= ~(0xf<<4);
			tmpval |= (0x4<<4 | 0x1);
			icdc_d3_hw_write(codec_dev,SCODA_MIX_2,tmpval);
			break;

		case RECORD_INPUT_AND_MIXER_L_NORMAL_R_CROSS:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_2);
			tmpval &= ~(0xf<<4);
			tmpval |= (0x1<<4 | 0x1);
			icdc_d3_hw_write(codec_dev,SCODA_MIX_2,tmpval);
			break;

		default:
			printk("JZ_CODEC: line: %d, record mux mode error!\n", __LINE__);
	}

	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}

/*record mixer mux*/
static void codec_set_record_mixer_mode(struct codec_info *codec_dev, int mode)
{
	int tmpval;
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");

	switch(mode){
		case RECORD_MIXER_ENABLE:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case RECORD_MIXER_DISABLE:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval&(~0x80));
			break;

		case RECORD_MIXER_NORMAL_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_3);
			tmpval &= ~(0xf<<4);
			icdc_d3_hw_write(codec_dev,SCODA_MIX_3,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case RECORD_MIXER_CROSS_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_3);
			tmpval &= ~(0xf<<4);
			tmpval |= 0x5<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_3,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case RECORD_MIXER_L_MIX_R_0_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_3);
			tmpval &= ~(0xf<<4);
			tmpval |= 0xb<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_3,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case RECORD_MIXER_L_0_R_MIX_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_3);
			tmpval &= ~(0xf<<4);
			tmpval |= 0xe<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_3,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case RECORD_MIXER_L_0_R_0_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_3);
			tmpval &= ~(0xf<<4);
			tmpval |= 0xf<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_3,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case RECORD_MIXER_L_NORMAL_R_CROSS_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_3);
			tmpval &= ~(0xf<<4);
			tmpval |= 0x1<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_3,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case RECORD_MIXER_L_CROSS_R_NORMAL_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_3);
			tmpval &= ~(0xf<<4);
			tmpval |= 0x4<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_3,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;
		default:
			printk("JZ_CODEC: line: %d, record mixer mode error!\n", __LINE__);
	}

	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}

/*adc mode*/
static void codec_set_adc(struct codec_info *codec_dev, int mode)
{
	int regval;
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");

	switch(mode){
		case ADC_ENABLE_WITH_DMIC:
			regval = icdc_d3_hw_read(codec_dev,SCODA_REG_FCR_ADC);
			icdc_d3_hw_write(codec_dev,SCODA_REG_FCR_ADC,regval|1<<6);

			regval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_ADC);
			icdc_d3_hw_write(codec_dev, SCODA_REG_CR_ADC, regval&(~(1<<4)));
			regval = icdc_d3_hw_read(codec_dev, SCODA_REG_CR_ADC);
			icdc_d3_hw_write(codec_dev, SCODA_REG_CR_ADC, regval|(1<<6));
			mdelay(1);
			break;
		case ADC_ENABLE_WITH_AMIC:
			regval = icdc_d3_hw_read(codec_dev,SCODA_REG_FCR_ADC);
			icdc_d3_hw_write(codec_dev,SCODA_REG_FCR_ADC,regval|1<<6);

			regval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_ADC);
			icdc_d3_hw_write(codec_dev, SCODA_REG_CR_ADC, regval&(~(1<<4)));
			regval = icdc_d3_hw_read(codec_dev, SCODA_REG_CR_ADC);
			icdc_d3_hw_write(codec_dev, SCODA_REG_CR_ADC, regval&(~(1<<6)));
			mdelay(1);
			break;
		case ADC_DISABLE:
			regval = icdc_d3_hw_read(codec_dev, SCODA_REG_CR_ADC);
			icdc_d3_hw_write(codec_dev, SCODA_REG_CR_ADC, regval|(1<<4));
			break;

		default:
			printk("JZ_CODEC: line: %d, adc mode error!\n", __LINE__);
	}

	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}

static void codec_set_replay_mux(struct codec_info *codec_dev, int mode)
{
	int tmpval;
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");

	switch(mode){
		case REPLAY_NORMAL_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_0);
			tmpval &= ~(0xf<<4|0x1);
			tmpval |= 0x5<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_0,tmpval);
			break;

		case REPLAY_CROSS_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_0);
			tmpval &= ~(0xf<<4|0x1);
			icdc_d3_hw_write(codec_dev,SCODA_MIX_0,tmpval);
			break;

                case REPLAY_INPUT_AND_MIXER_NORMAL:
                        tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_0);
                        tmpval &= ~(0xf<<4);
                        tmpval |= (0x5<<4 | 0x1);
                        icdc_d3_hw_write(codec_dev,SCODA_MIX_0, tmpval);
                        break;

                case REPLAY_INPUT_AND_MIXER_CROSS:
                        tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_0);
                        tmpval &= ~(0xf<<4);
                        tmpval |= 0x1; 
                        icdc_d3_hw_write(codec_dev,SCODA_MIX_0,tmpval);
                        break;

                case REPLAY_INPUT_AND_MIXER_L_CROSS_R_NORMAL:
                        tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_0);
                        tmpval &= ~(0xf<<4);
                        tmpval |= (0x4<<4 | 0x1);
			icdc_d3_hw_write(codec_dev,SCODA_MIX_0,tmpval);
                        break;

                case REPLAY_INPUT_AND_MIXER_L_NORMAL_R_CROSS:
                        tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_0);
                        tmpval &= ~(0xf<<4);
                        tmpval |= (0x1<<4 | 0x1);
			icdc_d3_hw_write(codec_dev,SCODA_MIX_0,tmpval);
                        break;

		default:
			printk("mode  = %d\n",mode);
			printk("JZ_CODEC: line: %d, replay mux mode error!\n", __LINE__);
	}

	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}

static void codec_set_replay_mixer_mode(struct codec_info *codec_dev, int mode)
{
	int tmpval;
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");

	switch(mode){
		case REPLAY_MIXER_ENABLE:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case REPLAY_MIXER_DISABLE:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval&(~0x80));
			break;

		case REPLAY_MIXER_NORMAL_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_1);
			tmpval &= ~(0xf<<4);
			icdc_d3_hw_write(codec_dev,SCODA_MIX_1,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case REPLAY_MIXER_CROSS_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_1);
			tmpval &= ~(0xf<<4);
			tmpval |= 0x5<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_1,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case REPLAY_MIXER_L_MIX_R_0_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_1);
			tmpval &= ~(0xf<<4);
			tmpval |= 0xb<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_1,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case REPLAY_MIXER_L_0_R_MIX_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_1);
			tmpval &= ~(0xf<<4);
			tmpval |= 0xe<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_1,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;

		case REPLAY_MIXER_L_0_R_0_INPUT:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_MIX_1);
			tmpval &= ~(0xf<<4);
			tmpval |= 0xf<<4;
			icdc_d3_hw_write(codec_dev,SCODA_MIX_1,tmpval);

			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_MIX);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_MIX,tmpval|(0x80));
			break;
		default:
			printk("mode  = %d\n",mode);
			printk("JZ_CODEC: line: %d, replay mixer mode error!\n", __LINE__);
	}

	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}

static void codec_set_aic_dac(struct codec_info *codec_dev, int mode)
{
	int tmpval;
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");
	switch (mode) {
		case OUTPUT_FROM_DAC_ENABLE:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_AICR_DAC);
			tmpval &= ~(1<<4);
			icdc_d3_hw_write(codec_dev,SCODA_REG_AICR_DAC,tmpval);
			break;
		case OUTPUT_FROM_DAC_DISABLE:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_AICR_DAC);
			tmpval |= (1<<4);
			icdc_d3_hw_write(codec_dev,SCODA_REG_AICR_DAC,tmpval);
			break;
		default :
			printk("JZ_CODEC: line: %d, inputl mode error!\n", __LINE__);
	}
	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}

static void codec_set_dac(struct codec_info *codec_dev, int mode)
{
	int tmpval;
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");

	switch(mode){
		case DAC_ENABLE:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_DAC);
			tmpval &= ~(1<<4|1<<7);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_DAC,tmpval);
			break;
		case DAC_DISABLE:
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_DAC);
			tmpval |= (1<<4);
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_DAC,tmpval);
			break;

		default:
			printk("JZ_CODEC: line: %d, dac mode error!\n", __LINE__);
	}

	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}



void codec_set_agc(struct codec_info *codec_dev, int mode)
{
	DUMP_ROUTE_PART_REGS(codec_dev, "enter");

	switch(mode){
		case AGC_ENABLE:
			break;

		case AGC_DISABLE:
			break;

		default:
			printk("JZ_CODEC: line: %d, agc mode error!\n", __LINE__);
	}

	DUMP_ROUTE_PART_REGS(codec_dev, "leave");
}
/*=================route attibute(gain) functions======================*/
static int codec_set_gain_mic(struct codec_info *codec_dev, int gain){
	int tmpval;
	gain = gain*5/100;
	if(gain > 5 || gain < 0){
		printk("set gain out of range [0 - 5]\n");
		return 0;
	}
	DUMP_GAIN_PART_REGS(codec_dev, "enter");

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_GCR_MIC1);
	tmpval &= ~0x7;
	icdc_d3_hw_write(codec_dev,SCODA_REG_GCR_MIC1,tmpval|gain);
	DUMP_GAIN_PART_REGS(codec_dev, "leave");
	return gain;
}

static int codec_set_gain_adc(struct codec_info *codec_dev, int gain)
{
	int tmpval;
	/* We just use 0dB ~ +23dB to avoid of the record sound cut top */
	gain = gain*23/100;
	if(gain > 23 || gain < 0){
		printk("set gain out of range [0 - 23]\n");
		return 0;
	}

	DUMP_GAIN_PART_REGS(codec_dev, "enter");

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_GCR_ADCL);
	tmpval &= ~0x3f;
	icdc_d3_hw_write(codec_dev,SCODA_REG_GCR_ADCL,tmpval|gain);

	DUMP_GAIN_PART_REGS(codec_dev, "leave");

	return tmpval;
}

//--------------------- record mixer
void codec_set_gain_record_mixer(struct codec_info *codec_dev, int gain)
{
	int tmpval;
	gain = gain*31/100;
	if(gain > 31 || gain < 0){
		printk("set gain out of range [0 - 31]\n");
		return;
	}
	DUMP_GAIN_PART_REGS(codec_dev, "enter");
	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_GCR_MIXADCL);
	tmpval &= ~0x3f;
	icdc_d3_hw_write(codec_dev,SCODA_REG_GCR_MIXADCL,tmpval|(31-gain));

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_GCR_MIXADCR);
	tmpval &= ~0x3f;
	icdc_d3_hw_write(codec_dev,SCODA_REG_GCR_MIXADCR,tmpval|(31-gain));

	DUMP_GAIN_PART_REGS(codec_dev, "leave");
}

//--------------------- replay mixer
static void codec_set_gain_replay_mixer(struct codec_info *codec_dev, unsigned int gain)
{
	int tmpval;
	gain = gain*31/100;
	if(gain > 31 || gain < 0){
		printk("set gain out of range [0 - 31]\n");
		return;
	}
	DUMP_GAIN_PART_REGS(codec_dev, "enter");

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_GCR_MIXDACL);
	tmpval &= ~0x3f;
	icdc_d3_hw_write(codec_dev,SCODA_REG_GCR_MIXDACL,tmpval|(31-gain));

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_GCR_MIXDACR);
	tmpval &= ~0x3f;
	icdc_d3_hw_write(codec_dev,SCODA_REG_GCR_MIXDACR,tmpval|(31-gain));

	DUMP_GAIN_PART_REGS(codec_dev, "leave");
}

//--------------------- dac

static int codec_set_gain_dac(struct codec_info *codec_dev, int gain)
{
	int tmpval,tmp_gain;
	/* We just use -31dB ~ 0dB to avoid of the replay sound cut top */
	gain = gain*31/100;
	if(gain > 31 || gain < 0){
		printk("set gain out of range [0 - 31]\n");
		return 0;
	}
	tmp_gain = 31 - gain;
	DUMP_GAIN_PART_REGS(codec_dev, "enter");
	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_GCR_DACL);
	tmpval &= ~0x3f;
	icdc_d3_hw_write(codec_dev,SCODA_REG_GCR_DACL,tmpval|tmp_gain);

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_GCR_DACR);
	tmpval &= ~0x3f;
	icdc_d3_hw_write(codec_dev,SCODA_REG_GCR_DACR,tmpval|tmp_gain);

	DUMP_GAIN_PART_REGS(codec_dev, "leave");
	return gain;
}

/***************************************************************************************\
 *codec route                                                                            *
 \***************************************************************************************/
static void codec_set_route_base(struct codec_info *codec_dev, const void *arg)
{
	route_conf_base *conf = (route_conf_base *)arg;

	if (conf->route_ready_mode)
		codec_prepare_ready(codec_dev, conf->route_ready_mode);

	/* record path */
	if (conf->route_record_mux_mode)
		codec_set_record_mux_mode(codec_dev, conf->route_record_mux_mode);

	if (conf->route_record_mixer_mode)
		codec_set_record_mixer_mode(codec_dev, conf->route_record_mixer_mode);

	if (conf->route_mic_mode)
		codec_set_mic(codec_dev, conf->route_mic_mode);

	if (conf->route_input_mode)
		codec_set_aic_adc(codec_dev, conf->route_input_mode);

	if (conf->route_adc_mode)
		codec_set_adc(codec_dev, conf->route_adc_mode);




	/* replay path */
	if (conf->route_replay_mux_mode){
		codec_set_replay_mux(codec_dev, conf->route_replay_mux_mode);
	}

	if (conf->route_replay_mixer_mode)
		codec_set_replay_mixer_mode(codec_dev, conf->route_replay_mixer_mode);

	if (conf->route_output_mode){
		codec_set_aic_dac(codec_dev, conf->route_output_mode);
	}

	if (conf->route_dac_mode){
		codec_set_dac(codec_dev, conf->route_dac_mode);
	}


	/* all gcr set with 0 ~ 100 */

	if (conf->attibute_record_mixer_gain) {
		/* the true gain val is 0 ~ 32*/
		codec_set_gain_record_mixer(codec_dev, conf->attibute_record_mixer_gain);
	}

	if (conf->attibute_replay_mixer_gain) {
		/* the true gain val is 0 ~ 32*/
		codec_set_gain_replay_mixer(codec_dev, conf->attibute_replay_mixer_gain);
	}

	/*Note it should not be set in andriod ,below */
	if (conf->attibute_adc_gain) {
		/* the true gain val is 0 - 43 */
		codec_set_gain_adc(codec_dev, conf->attibute_adc_gain);
	}

	if (conf->attibute_dac_gain) {
		/* the true gain val is 0 - 63 */
		codec_set_gain_dac(codec_dev, conf->attibute_dac_gain);
	}
}

static int codec_set_mic_volume(struct codec_info *codec_dev, int* val);
static int codec_set_replay_volume(struct codec_info *codec_dev, int *val);

/***************************************************************************************\
 *ioctl support function                                                               *
\***************************************************************************************/

static void gpio_enable_spk_en(struct codec_info *codec_dev)
{
	int val = -1;
	if(codec_platform_data && (codec_platform_data->gpio_spk_en.gpio != -1)) {
		val = gpio_get_value(codec_platform_data->gpio_spk_en.gpio);

		if (val != codec_platform_data->gpio_spk_en.active_level){
			if (codec_platform_data->gpio_spk_en.active_level) {
				gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 1);
			} else {
				gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 0);
			}
		}
	}
}

static void gpio_disable_spk_en(struct codec_info *codec_dev)
{
	int val = -1;
	if(codec_platform_data && (codec_platform_data->gpio_spk_en.gpio != -1)) {
		val = gpio_get_value(codec_platform_data->gpio_spk_en.gpio);

		if (val == codec_platform_data->gpio_spk_en.active_level){
			if (codec_platform_data->gpio_spk_en.active_level) {
				gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 0);
			} else {
				gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 1);
			}
			/* Wait for analog amplifier shutdown */
			codec_sleep(codec_dev, 80);
		}
	}
}



/*-----------------main fun-------------------*/
static int codec_set_board_route(struct codec_info *codec_dev, struct snd_board_route *broute)
{
	int i = 0;

	if (broute == NULL)
		return 0;

	/* set route */
	DUMP_ROUTE_NAME(broute->route);
	if (broute && ((cur_route == NULL) || (cur_route->route != broute->route))) {
		for (i = 0; codec_route_info[i].route_name != SND_ROUTE_NONE ; i ++) {
			if (broute->route == codec_route_info[i].route_name) {
#if 0
				/* Do nothing here, just for x1000 anti pop */
#else
                                /* Shutdown analog amplifier, just for anti pop */
                                if (broute->gpio_spk_en_stat != KEEP_OR_IGNORE){
					gpio_disable_spk_en(codec_dev);
				}
#endif
				/* set route */
				codec_set_route_base(codec_dev, codec_route_info[i].route_conf);
				break;
			}
		}
		if (codec_route_info[i].route_name == SND_ROUTE_NONE) {
			printk("SET_ROUTE: codec set route error!, undecleard route, route = %d\n", broute->route);
			goto err_unclear_route;
		}
	} else
		printk("SET_ROUTE: waring: route not be setted!\n");

        {
                keep_old_route = cur_route;
                cur_route = broute;
        }

	/* set gpio after set route */

	if (broute->gpio_spk_en_stat == STATE_ENABLE){
		gpio_enable_spk_en(codec_dev);
	}else if(broute->gpio_spk_en_stat == STATE_DISABLE){
		gpio_disable_spk_en(codec_dev);
	}
	DUMP_ROUTE_REGS(codec_dev, "leave");

	return broute ? broute->route : SND_ROUTE_NONE;
err_unclear_route:
	return SND_ROUTE_NONE;
}

static int codec_set_default_route(struct codec_info *codec_dev, int mode)
{
	int ret = 0;
	if (codec_platform_data) {
		if (codec_platform_data->replay_def_route.route == SND_ROUTE_NONE){
			codec_platform_data->replay_def_route.route = SND_ROUTE_REPLAY_SPK;
		}

		if (codec_platform_data->record_def_route.route == SND_ROUTE_NONE) {
			codec_platform_data->record_def_route.route = SND_ROUTE_RECORD_AMIC;
		}
	}
	if (mode == CODEC_RWMODE) {
		ret =  codec_set_board_route(codec_dev, &codec_platform_data->replay_speaker_record_buildin_mic_route);
	} else if (mode == CODEC_WMODE) {
		ret =  codec_set_board_route(codec_dev, &codec_platform_data->replay_def_route);
	} else if (mode == CODEC_RMODE){
		ret =  codec_set_board_route(codec_dev, &codec_platform_data->record_def_route);
	}

	return 0;
}


static int codec_set_route(struct codec_info *codec_dev, enum snd_codec_route_t route)
{
	struct snd_board_route tmp_broute;

	tmp_broute.route = route;

	if (route == SND_ROUTE_ALL_CLEAR || route == SND_ROUTE_REPLAY_CLEAR) {
		tmp_broute.gpio_spk_en_stat = STATE_DISABLE;
	}else{
		tmp_broute.gpio_spk_en_stat = KEEP_OR_IGNORE;
	}

	return codec_set_board_route(codec_dev, &tmp_broute);
}

/*----------------------------------------*/
/****** codec_init ********/
static int codec_init(struct codec_info *codec_dev)
{
	/* disable speaker and enable hp mute */
	gpio_disable_spk_en(codec_dev);

	codec_prepare_ready(codec_dev, CODEC_WMODE);

	codec_set_micbais(codec_dev, 1);

	if(codec_platform_data->replay_digital_volume_base)
		codec_set_gain_dac(codec_dev, codec_platform_data->replay_digital_volume_base);

	if(codec_platform_data->record_volume_base)
		codec_set_gain_mic(codec_dev,codec_platform_data->record_volume_base);

	if(codec_platform_data->record_digital_volume_base)
		codec_set_gain_adc(codec_dev, codec_platform_data->record_digital_volume_base);

	return 0;
}

/****** codec_turn_off ********/
static int codec_turn_off(struct codec_info *codec_dev, int mode)
{
	int ret = 0;

	if (mode & CODEC_RMODE) {
#if 1
		/* Do nothing here, for AEC.
		 * Because of if shutdown SB_ADC and SB_MIC, mixer loopback mode can't work.
		 */
#else
		ret = codec_set_route(codec_dev, SND_ROUTE_RECORD_CLEAR);
		if(ret != SND_ROUTE_RECORD_CLEAR)
		{
			printk("JZ CODEC: codec_turn_off_part record mode error!\n");
			return -1;
		}
#endif
	}
	if (mode & CODEC_WMODE) {
#ifdef CONFIG_BOARD_X1000_CANNA_V10
		/* Do nothing here, for anti pop and AEC.
		 * Because of if shutdown SB_DAC, mixer loopback mode can't work.
		 * When enable SB_DAC, there will be a pop sound.
		 * There will be a pop sound when you shutdown canna board amplifier.
		 */
#else
		gpio_disable_spk_en(codec_dev);
#endif

/*
		ret = codec_set_route(codec_dev, SND_ROUTE_REPLAY_CLEAR);
		if(ret != SND_ROUTE_REPLAY_CLEAR)
		{
			printk("JZ CODEC: codec_turn_off_part replay mode error!\n");
			return -1;
		}
*/
	}

	return ret;
}

/****** codec_shutdown *******/
static int codec_shutdown(struct codec_info *codec_dev)
{
	return 0;
}

/****** codec_reset **********/
static int codec_reset(struct codec_info *codec_dev)
{
	return 0;
}

/******** codec_anti_pop ********/
static int codec_anti_pop_start(struct codec_info *codec_dev, int mode)
{
	gpio_enable_spk_en(codec_dev);
	return 0;
}

/******** codec_suspend ************/
static int codec_suspend(struct codec_info *codec_dev)
{
        int ret = 10;
	int tmpval;

        g_codec_sleep_mode = 0;

        ret = codec_set_route(codec_dev, SND_ROUTE_ALL_CLEAR);
        if(ret != SND_ROUTE_ALL_CLEAR)
        {
                printk("JZ CODEC: codec_suspend_part error!\n");
                return 0;
        }

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_VIC);
	tmpval |= 0x2;
	icdc_d3_hw_write(codec_dev,SCODA_REG_CR_VIC,tmpval);
        codec_sleep(codec_dev, 10);

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_VIC);
	tmpval |= 0x1;
	icdc_d3_hw_write(codec_dev,SCODA_REG_CR_VIC,tmpval);
	return 0;
}

static int codec_resume(struct codec_info *codec_dev)
{
	int ret,tmp_route = 0;
	int tmpval;

	g_codec_sleep_mode = 1;
	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_VIC);
	tmpval &= (~0x1);
	icdc_d3_hw_write(codec_dev,SCODA_REG_CR_VIC,tmpval);

	codec_sleep(codec_dev, 250);

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_VIC);
	tmpval &= (~0x2);
	icdc_d3_hw_write(codec_dev,SCODA_REG_CR_VIC,tmpval);
	/*
	 * codec_sleep(400);
	 * this time we ignored becase other device resume waste time
	 */
	if (keep_old_route) {
		tmp_route = keep_old_route->route;
		ret = codec_set_board_route(codec_dev, keep_old_route);
		if(ret != tmp_route) {
			printk("JZ CODEC: codec_resume_part error!\n");
			return 0;
		}
	}
	return 0;
}

/*---------------------------------------*/

/**
 * CODEC set device
 *
 * this is just a demo function, and it will be use as default
 * if it is not realized depend on difficent boards
 *
 */
static int codec_set_device(struct codec_info *codec_dev, enum snd_device_t device)
{
	int ret = 0;
	int iserror = 0;

	switch (device) {
	case SND_DEVICE_SPEAKER:
		if (codec_platform_data && codec_platform_data->replay_speaker_route.route) {
			ret = codec_set_board_route(codec_dev, &(codec_platform_data->replay_speaker_route));
			if(ret != codec_platform_data->replay_speaker_route.route) {
				return -1;
			}
		}
		break;

	case SND_DEVICE_BUILDIN_MIC:
		if (codec_platform_data && codec_platform_data->record_buildin_mic_route.route) {
			ret = codec_set_board_route(codec_dev, &(codec_platform_data->record_buildin_mic_route));
			if (ret != codec_platform_data->record_buildin_mic_route.route) {
				return -1;
			}
		}
		break;

	case SND_DEVICE_LINEIN_RECORD:
		if (codec_platform_data && codec_platform_data->record_linein_route.route) {
			ret = codec_set_board_route(codec_dev, &(codec_platform_data->record_linein_route));
			if (ret != codec_platform_data->record_linein_route.route) {
				return -1;
			}
		}
		break;
	case SND_DEVICE_DEFAULT:
	case SND_DEVICE_BUILDIN_MIC_AND_SPEAKER:
		if (codec_platform_data && codec_platform_data->replay_speaker_record_buildin_mic_route.route) {
			ret = codec_set_board_route(codec_dev, &(codec_platform_data->replay_speaker_record_buildin_mic_route));
			if (ret != codec_platform_data->replay_speaker_record_buildin_mic_route.route) {
				return -1;
			}
		}
		break;

	default:
		iserror = 1;
		printk("JZ CODEC: Unkown ioctl argument %d in SND_SET_DEVICE\n",device);
	};

	if (!iserror)
		cur_out_device = device;

	return ret;
}

/*---------------------------------------*/

/**
 * CODEC set standby
 *
 * this is just a demo function, and it will be use as default
 * if it is not realized depend on difficent boards
 *
 */

static int codec_set_standby(struct codec_info *codec_dev, unsigned int sw)
{
	return 0;
}

/*---------------------------------------*/
/**
 * CODEC set record rate & data width & volume & channel
 *
 */

static int codec_set_record_rate(struct codec_info *codec_dev, unsigned long *rate)
{
	int speed = 0, val;
	unsigned long mrate[13] = {
		8000,  11025, 12000, 16000, 22050,
		24000, 32000, 44100, 48000, 88200,
		96000, 176400,192000
	};
	for (val = 0; val < 13; val++) {
		if (*rate <= mrate[val]) {
			speed = val;
			break;
		}
	}
	if (*rate > mrate[13 - 1]) {
		speed = 13 - 1;
	}

	val = icdc_d3_hw_read(codec_dev,SCODA_REG_FCR_ADC);
	val &= ~0xf;
	icdc_d3_hw_write(codec_dev,SCODA_REG_FCR_ADC,val|speed);
	return *rate;
}

static int codec_set_record_data_width(struct codec_info *codec_dev, int width)
{
	int supported_width[4] = {16, 18, 20, 24};
	int fixwidth,tmpval;
	for(fixwidth=0; fixwidth<4; fixwidth++)
		if(supported_width[fixwidth]==width)
			break;
	if(fixwidth == 4)
		return -1;

	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_AICR_ADC);
	tmpval &= ~(3<<6);
	icdc_d3_hw_write(codec_dev,SCODA_REG_AICR_ADC,tmpval|fixwidth<<6);
	return 0;
}

/*---------------------------------------*/
/**
 * CODEC set mute
 *
 * set dac mute used for anti pop
 *
 */
static int codec_mute(struct codec_info *codec_dev, int val,int mode)
{
	int tmpval;
	if (mode & CODEC_WMODE) {
		if (val) {
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_DAC);
			tmpval |= 0x80;
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_DAC,tmpval);
		} else {
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_DAC);
			tmpval &= ~0x80;
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_DAC,tmpval);
		}
	}
	if (mode & CODEC_RMODE) {
		if (val) {
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_ADC);
			tmpval |= 0x80;
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_ADC,tmpval);
		} else {
			tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_CR_ADC);
			tmpval &= ~0x80;
			icdc_d3_hw_write(codec_dev,SCODA_REG_CR_ADC,tmpval);
		}
	}
	return 0;
}

static int codec_set_record_volume(struct codec_info *codec_dev, int *val)
{
	if (*val >100 || *val < 0) {
		*val = user_record_volume;
		printk("Record volume out of range [ 0 ~ 100 ].\n");
	}

	codec_set_gain_adc(codec_dev, *val);

        if (*val == 0)
                codec_mute(codec_dev, 1, (int)CODEC_RMODE);
        else
                codec_mute(codec_dev, 0, (int)CODEC_RMODE);

	user_record_volume = *val;
	return *val;
}

static int codec_set_mic_volume(struct codec_info *codec_dev, int* val)
{
	if (*val >100 || *val < 0) {
		return -EINVAL;
	}

	*val = codec_set_gain_mic(codec_dev,*val);
	return *val;
}

static int codec_set_record_channel(struct codec_info *codec_dev, int *channel)
{
	if (*channel != 1)
		*channel = 2;
	return 1;
}

/*---------------------------------------*/
/**
 * CODEC set replay rate & data width & volume & channel
 *
 */

static int codec_set_replay_rate(struct codec_info *codec_dev, unsigned long *rate)
{
	int speed = 0, val;
	unsigned long mrate[13] = {
		8000,  11025, 12000, 16000, 22050,
		24000, 32000, 44100, 48000, 88200,
		96000, 176400,192000
	};
	for (val = 0; val < 13; val++) {
		if (*rate <= mrate[val]) {
			speed = val;
			*rate = mrate[val];
			break;
		}
	}
	if (*rate > mrate[13 - 1]) {
		speed = 13 - 1;
		*rate = mrate[13 - 1];
	}

	val = icdc_d3_hw_read(codec_dev,SCODA_REG_FCR_DAC);
	val &= ~0xf;
	icdc_d3_hw_write(codec_dev,SCODA_REG_FCR_DAC,val|speed);

	return 0;
}

static int codec_set_replay_data_width(struct codec_info *codec_dev, int width)
{
	int supported_width[4] = {16, 18, 20, 24};
	int fixwidth,tmpval;
	for(fixwidth=0; fixwidth<4; fixwidth++)
		if(supported_width[fixwidth]>=width)
			break;
	if(fixwidth == 4)
		return -1;
	tmpval = icdc_d3_hw_read(codec_dev,SCODA_REG_AICR_DAC);
	tmpval &= ~(3<<6);
	icdc_d3_hw_write(codec_dev,SCODA_REG_AICR_DAC,tmpval|fixwidth<<6);
	return 0;
}

static int codec_set_replay_volume(struct codec_info *codec_dev, int *val)
{
	if (*val >100 || *val < 0) {
		*val = user_replay_volume;
		printk("Replay volume out of range [ 0 ~ 100 ].\n");
	}

	codec_set_gain_dac(codec_dev, *val);
	if (*val == 0)
		codec_mute(codec_dev, 1, (int)CODEC_WMODE);
	else
		codec_mute(codec_dev, 0, (int)CODEC_WMODE);

	user_replay_volume = *val;

	return *val;
}

static int codec_set_replay_channel(struct codec_info *codec_dev, int* channel)
{
	if (*channel != 1)
		*channel = 2;
	return 0;
}

/*---------------------------------------*/
static int codec_debug_routine(struct codec_info *codec_dev, void *arg)
{
	return 0;
}

/**
 * CODEC short circut handler
 */
static inline void codec_short_circut_handler(struct codec_info *codec_dev)
{
	printk("JZ CODEC: Short circut restart CODEC short circut out finish.\n");
}

/**
 * IRQ routine
 */
static int codec_irq_handle(struct codec_info *codec_dev, struct work_struct *detect_wjork)
{
	return 0;
}

static int codec_get_hp_state(struct codec_info *codec_dev, int *state)
{
	*state = 0;
	return 0;
}

static void codec_get_format_cap(struct codec_info *codec_dev, unsigned long *format)
{
	*format = AFMT_S24_LE|AFMT_U24_LE|AFMT_U16_LE|AFMT_S16_LE|AFMT_S8|AFMT_U8;
}

static void codec_debug_default(struct codec_info *codec_dev)
{
	int ret = 4;
	while(ret--) {
		gpio_disable_spk_en(codec_dev);
		printk("disable %d\n",ret);
		mdelay(10000);
		gpio_enable_spk_en(codec_dev);
		printk("enable %d\n",ret);
		mdelay(10000);
	}
	ret = 4;
	while(ret--) {
		printk("============\n");
		printk("spk gpio disable\n");
		gpio_disable_spk_en(codec_dev);
		mdelay(10000);
		printk("spk gpio enable\n");
		gpio_enable_spk_en(codec_dev);
		mdelay(10000);

	}
}

static void codec_debug(struct codec_info *codec_dev, char arg)
{
	switch(arg) {
		/*...*/
	default:
		codec_debug_default(codec_dev);
	}
}

/***************************************************************************************\
 *                                                                                     *
 *control interface                                                                    *
 *                                                                                     *
 \***************************************************************************************/
/**
 * CODEC ioctl (simulated) routine
 *
 * Provide control interface for i2s driver
 */
static int jzcodec_ctl(struct codec_info *codec_dev, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	DUMP_IOC_CMD("enter");
	{
		switch (cmd) {

		case CODEC_INIT:
			ret = codec_init(codec_dev);
			break;

		case CODEC_TURN_OFF:
			ret = codec_turn_off(codec_dev, arg);
			break;

		case CODEC_SHUTDOWN:
			ret = codec_shutdown(codec_dev);
			break;

		case CODEC_RESET:
			ret = codec_reset(codec_dev);
			break;

		case CODEC_SUSPEND:
			ret = codec_suspend(codec_dev);
			break;

		case CODEC_RESUME:
			ret = codec_resume(codec_dev);
			break;

		case CODEC_ANTI_POP:
			ret = codec_anti_pop_start(codec_dev, (int)arg);
			break;

		case CODEC_SET_DEFROUTE:
			ret = codec_set_default_route(codec_dev, (int )arg);
			break;

		case CODEC_SET_DEVICE:
			ret = codec_set_device(codec_dev, *(enum snd_device_t *)arg);
			break;

		case CODEC_SET_STANDBY:
			ret = codec_set_standby(codec_dev, *(unsigned int *)arg);
			break;

		case CODEC_SET_RECORD_RATE:
			ret = codec_set_record_rate(codec_dev, (unsigned long *)arg);
			break;

		case CODEC_SET_RECORD_DATA_WIDTH:
			ret = codec_set_record_data_width(codec_dev, (int)arg);
			break;

		case CODEC_SET_MIC_VOLUME:
			ret = codec_set_mic_volume(codec_dev, (int *)arg);
			break;

		case CODEC_SET_RECORD_VOLUME:
			ret = codec_set_record_volume(codec_dev, (int *)arg);
			break;

		case CODEC_SET_RECORD_CHANNEL:
			ret = codec_set_record_channel(codec_dev, (int*)arg);
			break;

		case CODEC_SET_REPLAY_RATE:
			ret = codec_set_replay_rate(codec_dev, (unsigned long*)arg);
			break;

		case CODEC_SET_REPLAY_DATA_WIDTH:
			ret = codec_set_replay_data_width(codec_dev, (int)arg);
			break;

		case CODEC_SET_REPLAY_VOLUME:
			ret = codec_set_replay_volume(codec_dev, (int*)arg);
			break;

		case CODEC_SET_REPLAY_CHANNEL:
			ret = codec_set_replay_channel(codec_dev, (int*)arg);
			break;

		case CODEC_GET_REPLAY_FMT_CAP:
		case CODEC_GET_RECORD_FMT_CAP:
			codec_get_format_cap(codec_dev, (unsigned long *)arg);
			break;

		case CODEC_DAC_MUTE:
			if ((int)arg == 0 && user_replay_volume == 0){
				ret = -1;
				break;
			}
			ret = codec_mute(codec_dev, (int)arg,(int)CODEC_WMODE);
			break;
		case CODEC_ADC_MUTE:
			if ((int)arg == 0 && user_record_volume == 0){
				ret = -1;
				break;
			}
			ret = codec_mute(codec_dev, (int)arg,(int)CODEC_RMODE);
			break;
		case CODEC_DEBUG_ROUTINE:
			ret = codec_debug_routine(codec_dev, (void *)arg);
			break;

		case CODEC_IRQ_HANDLE:
			ret = codec_irq_handle(codec_dev, (struct work_struct*)arg);
			break;

		case CODEC_GET_HP_STATE:
			ret = codec_get_hp_state(codec_dev, (int*)arg);
			break;

		case CODEC_DUMP_REG:
			dump_codec_regs(codec_dev);
		case CODEC_DUMP_GPIO:
			dump_gpio_state(codec_dev);
			ret = 0;
			break;
		case CODEC_CLR_ROUTE:
			if ((int)arg == CODEC_RMODE)
				codec_set_route(codec_dev, SND_ROUTE_RECORD_CLEAR);
			else if ((int)arg == CODEC_WMODE)
				codec_set_route(codec_dev, SND_ROUTE_REPLAY_CLEAR);
			else {
				codec_set_route(codec_dev, SND_ROUTE_ALL_CLEAR);
			}
			ret = 0;
			break;
		case CODEC_DEBUG:
			codec_debug(codec_dev, (char)arg);
			ret = 0;
			break;
		default:
			printk("JZ CODEC:%s:%d: Unkown IOC commond\n", __FUNCTION__, __LINE__);
			ret = -1;
		}
	}

	DUMP_IOC_CMD("leave");
	return ret;
}

static int jz_codec_probe(struct platform_device *pdev)
{
	codec_platform_data = pdev->dev.platform_data;

        jz_set_hp_detect_type(SND_SWITCH_TYPE_GPIO,
                              &codec_platform_data->gpio_hp_detect,
                              NULL,
                              NULL,
                              NULL,
			      &codec_platform_data->gpio_linein_detect,
                              -1);

	if (codec_platform_data->gpio_amp_power.gpio != -1) {
		if (gpio_request(codec_platform_data->gpio_amp_power.gpio, "gpio_amp_power") < 0) {
			gpio_free(codec_platform_data->gpio_amp_power.gpio);
			gpio_request(codec_platform_data->gpio_amp_power.gpio,"gpio_amp_power");
		}
		/* power on amplifier */
		gpio_direction_output(codec_platform_data->gpio_amp_power.gpio, codec_platform_data->gpio_amp_power.active_level);
	}

	if (codec_platform_data->gpio_spk_en.gpio != -1 )
		if (gpio_request(codec_platform_data->gpio_spk_en.gpio,"gpio_spk_en") < 0) {
			gpio_free(codec_platform_data->gpio_spk_en.gpio);
			gpio_request(codec_platform_data->gpio_spk_en.gpio,"gpio_spk_en");
		}
	return 0;
}

static int __devexit jz_codec_remove(struct platform_device *pdev)
{
	if (codec_platform_data->gpio_spk_en.gpio != -1 )
		gpio_free(codec_platform_data->gpio_spk_en.gpio);

	codec_platform_data = NULL;

	return 0;
}

static struct platform_driver jz_codec_driver = {
	.probe		= jz_codec_probe,
	.remove		= __devexit_p(jz_codec_remove),
	.driver		= {
		.name	= "jz_codec",
		.owner	= THIS_MODULE,
	},
};

void codec_irq_set_mask(struct codec_info *codec_dev)
{
	return;
};

/***************************************************************************************\
 *module init                                                                          *
\***************************************************************************************/
/**
 * Module init
 */
static int __init init_codec(void)
{
	int retval;
	struct codec_info * codec_dev;
	codec_dev = kzalloc(sizeof(struct codec_info), GFP_KERNEL);
	if(!codec_dev) {
		printk("alloc codec device error\n");
		return -1;
	}
	sprintf(codec_dev->name, "internal_codec");
	codec_dev->codec_ctl_2 = jzcodec_ctl;
	codec_dev->codec_clk = 24000000;
	codec_dev->codec_mode = CODEC_MASTER;

	i2s_register_codec_2(codec_dev);

	retval = platform_driver_register(&jz_codec_driver);
	if (retval) {
		printk("JZ CODEC: Could net register jz_codec_driver\n");
		return retval;
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.suspend = codec_early_suspend;
	early_suspend.resume = codec_late_resume;
	early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	register_early_suspend(&early_suspend);
#endif
	return 0;
}

/**
 * Module exit
 */
static void __exit cleanup_codec(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&early_suspend);
#endif
	platform_driver_unregister(&jz_codec_driver);
}
arch_initcall(init_codec);
module_exit(cleanup_codec);
