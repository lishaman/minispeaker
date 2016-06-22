/*
 * Linux/sound/oss/jzsound/devices/codecs/akm4345_codec_v13.c
 *
 * CODEC driver for akm4345 i2s external codec
 *
 * 2016-4-xx   tjin <tao.jin@ingenic.com>
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
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <mach/jzsnd.h>
#include "../xb47xx_i2s_v13.h"

#define DEFAULT_REPLAY_SAMPLE_RATE   48000
#define AKM4345_EXTERNAL_CODEC_CLOCK DEFAULT_REPLAY_SAMPLE_RATE * 256
#define CODEC_MODE  CODEC_SLAVE

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
#endif
static int user_replay_volume = 100;
static unsigned long user_replay_rate = DEFAULT_REPLAY_SAMPLE_RATE;
static struct snd_codec_data *codec_platform_data = NULL;
static struct akm4345_platform_data *akm4345_spi = NULL;
static spinlock_t spi_irq_lock;
static struct mutex switch_dev_lock;

extern int i2s_register_codec(char*, void *,unsigned long,enum codec_mode);

static void codec_get_format_cap(unsigned long *format)
{
	*format = AFMT_S24_LE | AFMT_U24_LE;
}

/* Below is akm4345 registers value from 0x0 to 0x9 */
unsigned char akm4345_registers[][2] = {
	{ 0x00 ,0x8f },
	{ 0x01 ,0x42 },  //normal mode
	{ 0x02 ,0x00 },
	{ 0x03 ,0x81 },
	{ 0x04 ,0x04 },
	{ 0x05 ,0x00 },
	{ 0x06 ,0x10 },
	{ 0x07 ,0x00 },
	{ 0x08 ,0x00 },
	{ 0x09 ,0x00 },
};

static int spi_cs_en(int enable)
{
	if (enable){
		gpio_set_value(akm4345_spi->csn->gpio, 0);
		ndelay(10);
	}else{
		gpio_set_value(akm4345_spi->csn->gpio, 1);
		ndelay(200);
	}
	return 0;
}

static int spi_write_one_bit(int value)
{
	gpio_set_value(akm4345_spi->cclk->gpio, 0);
	if (value){
		gpio_set_value(akm4345_spi->cdto->gpio, 1);
	}else{
		gpio_set_value(akm4345_spi->cdto->gpio, 0);
	}
	ndelay(200);
	gpio_set_value(akm4345_spi->cclk->gpio, 1);
	ndelay(200);
	return 0;
}

static int spi_read_one_bit(void)
{
	int value;
	gpio_set_value(akm4345_spi->cclk->gpio, 0);
	ndelay(200);
	value = gpio_get_value(akm4345_spi->cdti->gpio);

	gpio_set_value(akm4345_spi->cclk->gpio, 1);
	ndelay(200);
	return value;
}

static int akm4345_spi_write_reg(unsigned char reg, unsigned char* data, unsigned int len)
{
	int i;
	unsigned long flags;
	unsigned char buf[2] = {0};

	if (akm4345_spi == NULL) {
		printk("===>NOTE: %s error\n", __func__);
		return -1;
	}

	if (akm4345_spi->cclk->gpio == -1 || akm4345_spi->cdto->gpio == -1 || akm4345_spi->csn->gpio == -1) {
		printk("===>NOTE: %s gpio error\n", __func__);
		return -1;
	}

	if(!data || len <= 0){
		printk("===>ERROR: %s\n", __func__);
		return -1;
	}

	spin_lock_irqsave(&spi_irq_lock, flags);
	buf[0] = (reg & 0x1f) | 0x60;
	buf[1] = *data;
	spi_cs_en(1);
	for(i = 7; i >= 0; i--){
		spi_write_one_bit((buf[0]>>i) & 0x1);
	}
	for(i = 7; i >= 0; i--){
		spi_write_one_bit((buf[1]>>i) & 0x1);
	}
	spi_cs_en(0);
	spin_unlock_irqrestore(&spi_irq_lock, flags);
	
	return 0;
}

static int akm4345_spi_read_reg(unsigned char reg, unsigned char* data, unsigned int len)
{
	int i;
	unsigned long flags;
	unsigned char buf[2] = {0};

	if (akm4345_spi == NULL) {
		printk("===>NOTE: %s error\n", __func__);
		return -1;
	}

	if (akm4345_spi->cclk->gpio == -1 || akm4345_spi->cdto->gpio == -1 || akm4345_spi->csn->gpio == -1 || akm4345_spi->cdti->gpio == -1) {
		printk("===>NOTE: %s gpio error\n", __func__);
		return -1;
	}

	if(!data || len <= 0){
		/* Len must equal to 1, because akm4345 can read one register every time */
		printk("===>ERROR: %s\n", __func__);
		return -1;
	}

	spin_lock_irqsave(&spi_irq_lock, flags);
	buf[0] = (reg & 0x1f) | 0x40;
	spi_cs_en(1);
	for(i = 7; i >= 0; i--){
		spi_write_one_bit((buf[0]>>i) & 0x1);
	}
	for(i = 7; i >= 0; i--){
		buf[1] |= (spi_read_one_bit() << i);
	}
	spi_cs_en(0);
	*data = buf[1];
	spin_unlock_irqrestore(&spi_irq_lock, flags);

	return 0;
}

static void gpio_enable_spk_en(void)
{
	if(codec_platform_data && (codec_platform_data->gpio_spk_en.gpio != -1)) {
		if (codec_platform_data->gpio_spk_en.active_level) {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 1);
		} else {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 0);
		}
	}
}

static void gpio_disable_spk_en(void)
{
	if(codec_platform_data && (codec_platform_data->gpio_spk_en.gpio != -1)) {
		if (codec_platform_data->gpio_spk_en.active_level) {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 0);
		} else {
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio , 1);
		}
	}
}

static int codec_set_replay_rate(unsigned long *rate)
{
	int i;
	unsigned char data;
	unsigned long mrate[11] = {
		8000, 11025, 12000, 16000, 22050, 24000,
		32000,44100, 48000, 88200, 96000
	};

	for (i = 0; i < 11; i++) {
		if (*rate == mrate[i])
			break;
	}
	if (i == 11){
		printk("Replay rate %ld is not support by akm4345, we fix it to 48000\n", *rate);
		*rate = 48000;
	}

	user_replay_rate = *rate;
	akm4345_spi_read_reg(0x01, &data, 1);
	if (user_replay_rate > 48000){
		/* double mode */
		data &= 0xe7;
		data |= 1<<3;
	} else {
		/* normal mode */
		data &= 0xe7;
	}
	akm4345_spi_write_reg(0x01, &data, 1);
	msleep(50);            //This delay is to wait for akm4345 i2s clk stable.
	
	return 0;
}

static int codec_set_device(enum snd_device_t device)
{
	int ret = 0;
	mutex_lock(&switch_dev_lock);
	switch (device) {
		case SND_DEVICE_HEADSET:
			gpio_disable_spk_en();
			msleep(5);
			break;
		case SND_DEVICE_SPEAKER:
			gpio_enable_spk_en();
			msleep(5);
			break;
		case SND_DEVICE_LINEIN_RECORD:
			/* There is not linein route in akm4345.*/
			break;
		default:
			printk("JZ CODEC: Unkown ioctl argument %d in SND_SET_DEVICE\n",device);
			ret = -1;
	};
	mutex_unlock(&switch_dev_lock);
	return ret;
}

static int codec_set_replay_channel(int* channel)
{
	*channel = (*channel >= 2) + 1;

	return 0;
}

static int codec_set_record_channel(int* channel)
{
	*channel = (*channel >= 2) + 1;

	return 0;
}

static int codec_setup(void)
{
	int i;
	int ret = 0;
	/* Init registers */
        for (i = 0; i < sizeof(akm4345_registers) / sizeof(akm4345_registers[0]); i++){
                ret |= akm4345_spi_write_reg(akm4345_registers[i][0], &akm4345_registers[i][1], 1);
	}
	
	if (ret)
		printk("===>ERROR: akm4345 init error!\n");
        return ret;
}

static int codec_init(void)
{
	int ret;
	if (akm4345_spi == NULL){
		printk("The spi is not register, akm4345 can't init\n");
		return -1;
	}
	/* mute the speaker to anti-pop */
	gpio_disable_spk_en();

	/* reset PDN pin */
	if (akm4345_spi->pdn->gpio != -1){
		gpio_set_value(akm4345_spi->pdn->gpio, akm4345_spi->pdn->active_level);
		mdelay(20);
		gpio_set_value(akm4345_spi->pdn->gpio, !akm4345_spi->pdn->active_level);
		mdelay(1);
	}

	/* All codec register setup */
	ret = codec_setup();

	return ret;
}

static int dump_codec_regs(void)
{
	int i;
	unsigned char data;
	printk("akm4345 registers:\n");
	for(i = 0; i <= 9; i++){
		akm4345_spi_read_reg(i, &data, 1);
		printk("reg 0x%02x = 0x%02x\n", i, data);
	}
	return 0;
}

static int codec_set_replay_volume(int *val)
{
	/* get current volume */
	if (*val < 0) {
		*val = user_replay_volume;
		return *val;
	} else if (*val > 100)
		*val = 100;
	/* Because akm4345 has not the gain control register, we can only set the volume by software.*/
	if (*val) {
		gpio_enable_spk_en();
	} else{
		gpio_disable_spk_en();
	}
	user_replay_volume = *val;
	return *val;
}

static int codec_suspend(void)
{
	unsigned char data;
	gpio_disable_spk_en();
	msleep(5);
	/* akm4345 RSTN and PW set to 0 */
	akm4345_spi_read_reg(0x0, &data, 1);
	data &= 0xfc;
	akm4345_spi_write_reg(0x0, &data, 1);
	return 0;
}

static int codec_resume(void)
{
	unsigned char data;
	/* akm4345 RSTN and PW set to 1 */
	akm4345_spi_read_reg(0x0, &data, 1);
	data |= 0x3;
	akm4345_spi_write_reg(0x0, &data, 1);
	msleep(5);
	gpio_enable_spk_en();
	return 0;
}

static int codec_shutdown(void)
{
	if (akm4345_spi == NULL){
		printk("The spi is not register, akm4345 can't shutdown\n");
		return -1;
	}

	gpio_disable_spk_en();
	mdelay(3);

	if (akm4345_spi->pdn->gpio != -1){
		gpio_set_value(akm4345_spi->pdn->gpio, akm4345_spi->pdn->active_level);
	}
	return 0;
}

static int jzcodec_ctl(unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	{
		switch (cmd) {
		case CODEC_INIT:
			codec_init();
			break;

		case CODEC_TURN_OFF:
			break;

		case CODEC_SHUTDOWN:
			ret = codec_shutdown();
			break;

		case CODEC_RESET:
			break;

		case CODEC_SUSPEND:
			ret = codec_suspend();
			break;

		case CODEC_RESUME:
			ret = codec_resume();
			break;

		case CODEC_ANTI_POP:
			break;

		case CODEC_SET_DEFROUTE:
			break;

		case CODEC_SET_DEVICE:
			ret = codec_set_device(*(enum snd_device_t *)arg);
			break;

		case CODEC_SET_STANDBY:
			break;

		case CODEC_SET_REPLAY_RATE:
			ret = codec_set_replay_rate((unsigned long*)arg);
			break;

		case CODEC_SET_RECORD_RATE:
			/*
			 * Record sample rate is follow the replay sample rate. Here set is invalid.
			 * Because record and replay i2s use the same BCLK and SYNC pin.
			*/
			break;

		case CODEC_SET_REPLAY_DATA_WIDTH:
			break;

		case CODEC_SET_RECORD_DATA_WIDTH:
			break;

		case CODEC_SET_REPLAY_VOLUME:
			ret = codec_set_replay_volume((int*)arg);
			break;

		case CODEC_SET_RECORD_VOLUME:
			ret = *(int*)arg;
			break;

		case CODEC_SET_MIC_VOLUME:
			ret = *(int*)arg;
			break;

		case CODEC_SET_REPLAY_CHANNEL:
			ret = codec_set_replay_channel((int*)arg);
			break;

		case CODEC_SET_RECORD_CHANNEL:
			ret = codec_set_record_channel((int*)arg);
			break;

		case CODEC_GET_REPLAY_FMT_CAP:
			codec_get_format_cap((unsigned long *)arg);
			break;

		case CODEC_GET_RECORD_FMT_CAP:
			*(unsigned long *)arg = 0;
			break;

		case CODEC_DAC_MUTE:
			break;

		case CODEC_ADC_MUTE:
			break;

		case CODEC_DEBUG_ROUTINE:
			break;

		case CODEC_CLR_ROUTE:
			break;

		case CODEC_DEBUG:
			break;

		case CODEC_DUMP_REG:
			dump_codec_regs();
			break;
		default:
			printk("JZ CODEC:%s:%d: Unkown IOC commond\n", __FUNCTION__, __LINE__);
			ret = -1;
		}
	}

	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void codec_early_suspend(struct early_suspend *handler)
{
}
static void codec_late_resume(struct early_suspend *handler)
{
}
#endif

static int jz_codec_probe(struct platform_device *pdev)
{
	codec_platform_data = pdev->dev.platform_data;

	codec_platform_data->codec_sys_clk = AKM4345_EXTERNAL_CODEC_CLOCK;

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

	if (codec_platform_data->gpio_spk_en.gpio != -1) {
		if (gpio_request(codec_platform_data->gpio_spk_en.gpio, "gpio_spk_en") < 0) {
			gpio_free(codec_platform_data->gpio_spk_en.gpio);
			gpio_request(codec_platform_data->gpio_spk_en.gpio,"gpio_spk_en");
		}
		gpio_disable_spk_en();
	}

	return 0;
}

static int __devexit jz_codec_remove(struct platform_device *pdev)
{
	if (codec_platform_data->gpio_spk_en.gpio != -1)
		gpio_free(codec_platform_data->gpio_spk_en.gpio);
	codec_platform_data = NULL;

	return 0;
}

static struct platform_driver jz_codec_driver = {
        .probe          = jz_codec_probe,
        .remove         = __devexit_p(jz_codec_remove),
        .driver         = {
                .name   = "akm4345_codec",
                .owner  = THIS_MODULE,
        },
};

static int akm4345_spi_probe(struct platform_device *pdev)
{
	int ret = 0;

	akm4345_spi = pdev->dev.platform_data;

	if (akm4345_spi->pdn->gpio != -1) {
                ret = gpio_request_one(akm4345_spi->pdn->gpio,
                                GPIOF_DIR_OUT, "akm4345-pdn");
                if (ret != 0) {
                        printk("Can't request pdn pin = %d\n",
                                        akm4345_spi->pdn->gpio);
                        return ret;
                } else
                        gpio_set_value(akm4345_spi->pdn->gpio, akm4345_spi->pdn->active_level);
        }
	if (akm4345_spi->csn->gpio != -1) {
                ret = gpio_request_one(akm4345_spi->csn->gpio,
                                GPIOF_DIR_OUT, "akm4345-csn");
                if (ret != 0) {
                        printk("Can't request csn pin = %d\n",
                                        akm4345_spi->csn->gpio);
                        return ret;
                } else
                        gpio_set_value(akm4345_spi->csn->gpio, 1);
        }
	if (akm4345_spi->cclk->gpio != -1) {
                ret = gpio_request_one(akm4345_spi->cclk->gpio,
                                GPIOF_DIR_OUT, "akm4345-cclk");
                if (ret != 0) {
                        printk("Can't request cclk pin = %d\n",
                                        akm4345_spi->cclk->gpio);
                        return ret;
                } else
                        gpio_set_value(akm4345_spi->cclk->gpio, 1);
        }
	if (akm4345_spi->cdto->gpio != -1) {
                ret = gpio_request_one(akm4345_spi->cdto->gpio,
                                GPIOF_DIR_OUT, "akm4345-cdto");
                if (ret != 0) {
                        printk("Can't request cdto pin = %d\n",
                                        akm4345_spi->cdto->gpio);
                        return ret;
                } else
                        gpio_set_value(akm4345_spi->cdto->gpio, 1);
        }
	if (akm4345_spi->cdti->gpio != -1) {
                ret = gpio_request_one(akm4345_spi->cdti->gpio,
                                GPIOF_DIR_IN, "akm4345-cdti");
                if (ret != 0) {
                        printk("Can't request cdti pin = %d\n",
                                        akm4345_spi->cdti->gpio);
                        return ret;
                }
        }

	return 0;
}

static int __devexit akm4345_spi_remove(struct platform_device *pdev)
{
	if (akm4345_spi->pdn->gpio != -1){
		gpio_set_value(akm4345_spi->pdn->gpio, akm4345_spi->pdn->active_level);
		gpio_free(akm4345_spi->pdn->gpio);
	}
	if (akm4345_spi->csn->gpio != -1)
		gpio_free(akm4345_spi->csn->gpio);
	if (akm4345_spi->cclk->gpio != -1)
		gpio_free(akm4345_spi->cclk->gpio);
	if (akm4345_spi->cdti->gpio != -1)
		gpio_free(akm4345_spi->cdti->gpio);
	if (akm4345_spi->cdto->gpio != -1)
		gpio_free(akm4345_spi->cdto->gpio);

	akm4345_spi = NULL;

	return 0;
}

static struct platform_driver akm4345_spi_driver = {
        .probe          = akm4345_spi_probe,
        .remove         = __devexit_p(akm4345_spi_remove),
        .driver         = {
                .name   = "akm4345_spi",
                .owner  = THIS_MODULE,
        },
};

/**
 * Module init
 */
static int __init init_akm4345_codec(void)
{
	int ret = 0;

	spin_lock_init(&spi_irq_lock);
	mutex_init(&switch_dev_lock);

	ret = i2s_register_codec("i2s_external_codec", (void *)jzcodec_ctl, AKM4345_EXTERNAL_CODEC_CLOCK, CODEC_MODE);
	if (ret < 0){
		printk("i2s audio is not support\n");
		return ret;
	}

        ret = platform_driver_register(&akm4345_spi_driver);
        if (ret) {
                printk("JZ CODEC: Could not register akm4345_spi_driver\n");
                return ret;
        }

	/* Now we have not use the jz_codec_driver on akm4345, but we retain it for future.*/
        ret = platform_driver_register(&jz_codec_driver);
        if (ret) {
                printk("JZ CODEC: Could not register jz_codec_driver\n");
                return ret;
        }

#ifdef CONFIG_HAS_EARLYSUSPEND
        early_suspend.suspend = codec_early_suspend;
        early_suspend.resume = codec_late_resume;
        early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
        register_early_suspend(&early_suspend);
#endif
	printk("audio codec akm4345 register success\n");
	return 0;
}

/**
 * Module exit
 */
static void __exit cleanup_akm4345_codec(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
        unregister_early_suspend(&early_suspend);
#endif
	platform_driver_unregister(&jz_codec_driver);

	platform_driver_unregister(&akm4345_spi_driver);
}
module_init(init_akm4345_codec);
module_exit(cleanup_akm4345_codec);
MODULE_LICENSE("GPL");
