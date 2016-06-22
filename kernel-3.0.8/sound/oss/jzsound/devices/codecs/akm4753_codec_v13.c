/*
 * Linux/sound/oss/jzsound/devices/codecs/akm4753_codec_v13.c
 *
 * CODEC driver for akm4753 i2s external codec
 *
 * 2015-10-xx   tjin <tao.jin@ingenic.com>
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
#define AKM4753_EXTERNAL_CODEC_CLOCK 24000000
//#define CODEC_MODE  CODEC_SLAVE
#define CODEC_MODE  CODEC_MASTER

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
#endif
static struct i2c_client *akm4753_client = NULL;
static int user_replay_volume = 100;
static unsigned long user_replay_rate = DEFAULT_REPLAY_SAMPLE_RATE;
static struct snd_codec_data *codec_platform_data = NULL;
static struct mutex i2c_access_lock;
static struct mutex switch_dev_lock;

extern int i2s_register_codec(char*, void *,unsigned long,enum codec_mode);


static void codec_get_format_cap(unsigned long *format)
{
	*format = AFMT_S24_LE | AFMT_U24_LE;
}

/* Below is akm4753 registers value from 0x05 to 0x7d */
unsigned char akm4753_registers[][2] = {
	{ 0x05 ,0xff },
	{ 0x06 ,0xff },
	{ 0x07 ,0x00 },
	{ 0x08 ,0x00 },
	{ 0x09 ,0x00 },
	{ 0x0A ,0xF1 },
	{ 0x0B ,0x00 },
	{ 0x0C ,0x00 },
	{ 0x0D ,0xF1 },
	{ 0x0E ,0x00 },
	{ 0x0F ,0x00 },
	{ 0x10 ,0x00 },
	{ 0x11 ,0x00 },
	{ 0x12 ,0x00 },
	{ 0x13 ,0x00 },
	{ 0x14 ,0x00 },
	{ 0x15 ,0x00 },
	{ 0x16 ,0x00 },
	{ 0x17 ,0x00 },
	{ 0x18 ,0x00 },
	{ 0x19 ,0x00 },
	{ 0x1A ,0x00 },
	{ 0x1B ,0x00 },
	{ 0x1C ,0x00 },
	{ 0x1D ,0x00 },
	{ 0x1E ,0x00 },
	{ 0x1F ,0x00 },
	{ 0x20 ,0x00 },
	{ 0x21 ,0x00 },
	{ 0x22 ,0x00 },
	{ 0x23 ,0x00 },
	{ 0x24 ,0x00 },
	{ 0x25 ,0x00 },
	{ 0x26 ,0x00 },
	{ 0x27 ,0x00 },
	{ 0x28 ,0x00 },
	{ 0x29 ,0x00 },
	{ 0x2A ,0x00 },
	{ 0x2B ,0x00 },
	{ 0x2C ,0x00 },
	{ 0x2D ,0x00 },
	{ 0x2E ,0x00 },
	{ 0x2F ,0x00 },
	{ 0x30 ,0x00 },
	{ 0x31 ,0x00 },
	{ 0x32 ,0x00 },
	{ 0x33 ,0x00 },
	{ 0x34 ,0x00 },
	{ 0x35 ,0x00 },
	{ 0x36 ,0x00 },
	{ 0x37 ,0x00 },
	{ 0x38 ,0x00 },
	{ 0x39 ,0x00 },
	{ 0x3A ,0x00 },
	{ 0x3B ,0x00 },
	{ 0x3C ,0x00 },
	{ 0x3D ,0x00 },
	{ 0x3E ,0x00 },
	{ 0x3F ,0x00 },
	{ 0x40 ,0x00 },
	{ 0x41 ,0x00 },
	{ 0x42 ,0x00 },
	{ 0x43 ,0x00 },
	{ 0x44 ,0x00 },
	{ 0x45 ,0x00 },
	{ 0x46 ,0x00 },
	{ 0x47 ,0x00 },
	{ 0x48 ,0x00 },
	{ 0x49 ,0x00 },
	{ 0x4A ,0x00 },
	{ 0x4B ,0x00 },
	{ 0x4C ,0x00 },
	{ 0x4D ,0x00 },
	{ 0x4E ,0x00 },
	{ 0x4F ,0x00 },
	{ 0x50 ,0x00 },
	{ 0x51 ,0x00 },
	{ 0x52 ,0x00 },
	{ 0x53 ,0x00 },
	{ 0x54 ,0x00 },
	{ 0x55 ,0x00 },
	{ 0x56 ,0x00 },
	{ 0x57 ,0x00 },
	{ 0x58 ,0x00 },
	{ 0x59 ,0x00 },
	{ 0x5A ,0x00 },
	{ 0x5B ,0x00 },
	{ 0x5C ,0x00 },
	{ 0x5D ,0x00 },
	{ 0x5E ,0x00 },
	{ 0x5F ,0x00 },
	{ 0x60 ,0x00 },
	{ 0x61 ,0x00 },
	{ 0x62 ,0x00 },
	{ 0x63 ,0x00 },
	{ 0x64 ,0x00 },
	{ 0x65 ,0x00 },
	{ 0x66 ,0x00 },
	{ 0x67 ,0x00 },
	{ 0x68 ,0x00 },
	{ 0x69 ,0x00 },
	{ 0x6A ,0x00 },
	{ 0x6B ,0x00 },
	{ 0x6C ,0x00 },
	{ 0x6D ,0x00 },
	{ 0x6E ,0x00 },
	{ 0x6F ,0x00 },
	{ 0x70 ,0x00 },
	{ 0x71 ,0x00 },
	{ 0x72 ,0x00 },
	{ 0x73 ,0x00 },
	{ 0x74 ,0x00 },
	{ 0x75 ,0x00 },
	{ 0x76 ,0x00 },
	{ 0x77 ,0x00 },
	{ 0x78 ,0x00 },
	{ 0x79 ,0x00 },
	{ 0x7A ,0x00 },
	{ 0x7B ,0x00 },
	{ 0x7C ,0x00 },
	{ 0x7D ,0x00 }
};

static int akm4753_i2c_write_regs(unsigned char reg, unsigned char* data, unsigned int len)
{
	int ret = -1;
	int i = 0;
	unsigned char buf[127] = {0};
	struct i2c_client *client = akm4753_client;

	if (client == NULL) {
		printk("===>NOTE: %s error\n", __func__);
		return ret;
	}

	if(!data || len <= 0){
		printk("===>ERROR: %s\n", __func__);
		return ret;
	}

	mutex_lock(&i2c_access_lock);
	buf[0] = reg;
	for(; i < len; i++){
		buf[i+1] = *data;
		data++;
	}

	ret = i2c_master_send(client, buf, len+1);
	if (ret < len+1)
		printk("%s 0x%02x err %d!\n", __func__, reg, ret);
	mutex_unlock(&i2c_access_lock);

	return ret < len+1 ? ret : 0;
}

static int akm4753_i2c_read_reg(unsigned char reg, unsigned char* data, unsigned int len)
{
	int ret = -1;
	struct i2c_client *client = akm4753_client;

	if (client == NULL) {
		printk("===>NOTE: %s error\n", __func__);
		return ret;
	}

	if(!data || len <= 0){
		printk("===>ERROR: %s\n", __func__);
		return ret;
	}

	mutex_lock(&i2c_access_lock);
	ret = i2c_master_send(client, &reg, 1);
	if (ret < 1) {
		printk("%s 0x%02x err\n", __func__, reg);
		mutex_unlock(&i2c_access_lock);
		return -1;
	}

	ret = i2c_master_recv(client, data, len);
	if (ret < 0)
		printk("%s 0x%02x err\n", __func__, reg);
	mutex_unlock(&i2c_access_lock);

	return ret < len ? ret : 0;
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
	int i = 0;
	unsigned char data = 0x0;
	unsigned long mrate[12] = {
		8000, 12000, 16000, 24000, 7350, 11025,
		14700,22050, 32000, 48000, 29400,44100
	};
	unsigned char reg[12] = {
		0, 1, 2, 3, 4, 5,
		6, 7, 10,11,14,15
	};

	for (i = 0; i < 12; i++) {
		if (*rate == mrate[i])
			break;
	}
	if (i == 12){
		printk("Replay rate %ld is not support by akm4753, we fix it to 48000\n", *rate);
		*rate = 48000;
		i = 9;
	}

	user_replay_rate = *rate;
	akm4753_i2c_read_reg(0x02, &data, 1);
	data &= 0xf;
	data |= reg[i]<<4;
	akm4753_i2c_write_regs(0x02, &data, 1);
	msleep(50);            //This delay is to wait for akm4753 i2s clk stable.
	return 0;
}

static int codec_set_device(enum snd_device_t device)
{
	int ret = 0;
	unsigned char data;
	mutex_lock(&switch_dev_lock);
	switch (device) {
		case SND_DEVICE_HEADSET:
			gpio_disable_spk_en();
			msleep(5);
			akm4753_i2c_read_reg(0x01, &data, 1);
			data &= 0xf0;
			data |= 0x1;	
			akm4753_i2c_write_regs(0x01, &data, 1);
			codec_set_replay_rate(&user_replay_rate);
			break;
		case SND_DEVICE_SPEAKER:
			gpio_disable_spk_en();
			msleep(5);
			akm4753_i2c_read_reg(0x01, &data, 1);
			data &= 0xf0;
			data |= 0x1;	
			akm4753_i2c_write_regs(0x01, &data, 1);
			msleep(5);
			codec_set_replay_rate(&user_replay_rate);
			gpio_enable_spk_en();
			break;
		case SND_DEVICE_LINEIN_RECORD:
			gpio_disable_spk_en();
			msleep(5);
			akm4753_i2c_read_reg(0x01, &data, 1);
			data &= 0xf0;
			akm4753_i2c_write_regs(0x01, &data, 1);
			msleep(5);
			/* fix to 44100 sample rate */
			akm4753_i2c_read_reg(0x02, &data, 1);
			data |= 0xf0;
			akm4753_i2c_write_regs(0x02, &data, 1);
			msleep(5);
			gpio_enable_spk_en();
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
#ifdef CONFIG_BOARD_X1000_HL01_V10
	*channel = 1;
#else
	*channel = (*channel >= 2) + 1;
#endif
	return 0;
}

static int codec_set_record_channel(int* channel)
{
	*channel = (*channel >= 2) + 1;

	return 0;
}

static int codec_dac_setup(void)
{
	int i;
	int ret = 0;
	char data = 0x0;
	/* Disable SAR and SAIN */	
	data = 0x0;
	ret |= akm4753_i2c_write_regs(0x0, &data, 1);

#ifdef CONFIG_BOARD_X1000_HL01_V10
	/* Set up stereo mode(HPF, LPF individual mode) */
	data = 0x11;
	ret |= akm4753_i2c_write_regs(0x01, &data, 1);
#else
	/* Set up 2.1-channels mode */
	data = 0x21;
	ret |= akm4753_i2c_write_regs(0x01, &data, 1);
#endif
	
	/* Init 0x05 ~ 0x7d registers */
        for (i = 0; i < sizeof(akm4753_registers) / sizeof(akm4753_registers[0]); i++){
                ret |= akm4753_i2c_write_regs(akm4753_registers[i][0], &akm4753_registers[i][1], 1);
	}
	
	/* Power on DAC ADC DSP */
	data = 0xbd;
	ret |= akm4753_i2c_write_regs(0x04, &data, 1);

	if (ret)
		printk("===>ERROR: akm4753 init error!\n");
        return ret;
}

static int codec_clk_setup(void)
{
	int ret = 0;
	char data = 0x0;
	/* I2S clk setup */
	data = 0xf7;
	ret |= akm4753_i2c_write_regs(0x02, &data, 1);
	data = 0xc3;
	ret |= akm4753_i2c_write_regs(0x03, &data, 1);
	data = 0x84;
	ret |= akm4753_i2c_write_regs(0x04, &data, 1);
	mdelay(5);

	return ret;
}

static int codec_init(void)
{
	int ret = 0;
	struct device *dev;
	struct akm4753_platform_data *akm4753;

	if (akm4753_client == NULL){
		printk("The i2c is not register, akm4753 can't init\n");
		return -1;
	}
	dev = &akm4753_client->dev;
	akm4753 = dev->platform_data;

	/* reset PDN pin */
	if (akm4753->pdn->gpio != -1){
		gpio_set_value(akm4753->pdn->gpio, akm4753->pdn->active_level);
		mdelay(20);
		gpio_set_value(akm4753->pdn->gpio, !akm4753->pdn->active_level);
		mdelay(1);
	}

	/* clk setup */
	ret |= codec_clk_setup();

	/* DAC outputs setup */
	ret |= codec_dac_setup();

	return ret;
}

static int dump_codec_regs(void)
{
	int i;
	int ret = 0;
	unsigned char reg_val[126] = {0};

	printk("akm4753 registers:\n");

	ret = akm4753_i2c_read_reg(0x0, reg_val, 126);
	for (i = 0; i < 126; i++){
		printk("reg 0x%02x = 0x%02x\n", i, reg_val[i]);
	}
	return ret;
}

static int codec_set_replay_volume(int *val)
{
	char data = 0x0;
	/* get current volume */
	if (*val < 0) {
		*val = user_replay_volume;
		return *val;
	} else if (*val > 100)
		*val = 100;

	if (*val) {
#ifdef CONFIG_BOARD_X1000_HL01_V10
		/* use -16dB ~ -49dB for 3w speaker */
		data = (16 + (100-*val)/3) *2;
		akm4753_i2c_write_regs(0x05, &data, 1);
		akm4753_i2c_write_regs(0x06, &data, 1);
#else
		/* use 0dB ~ -50dB */
		data = 100 - *val;
		akm4753_i2c_write_regs(0x05, &data, 1);
		akm4753_i2c_write_regs(0x06, &data, 1);
#endif
		gpio_enable_spk_en();
	} else{
		/* Digital Volume mute */
		data = 0xff;
		akm4753_i2c_write_regs(0x05, &data, 1);
		akm4753_i2c_write_regs(0x06, &data, 1);
		gpio_disable_spk_en();
	}
	user_replay_volume = *val;
	return *val;
}

static int codec_suspend(void)
{
	int ret = 0;
	unsigned char data;
	gpio_disable_spk_en();
	msleep(5);
	data = 0x84;
	ret |= akm4753_i2c_write_regs(0x04, &data, 1);
	return 0;
}

static int codec_resume(void)
{
	int ret = 0;
	unsigned char data;
	data = 0xbd;
	ret |= akm4753_i2c_write_regs(0x04, &data, 1);
	msleep(5);
	gpio_enable_spk_en();
	return 0;
}

static int codec_shutdown(void)
{
	int ret = 0;
	unsigned char data;
	struct device *dev;
	struct akm4753_platform_data *akm4753;

	if (akm4753_client == NULL){
		printk("The i2c is not register, akm4753 can't init\n");
		return -1;
	}
	dev = &akm4753_client->dev;
	akm4753 = dev->platform_data;

	gpio_disable_spk_en();
	mdelay(3);

	akm4753_i2c_read_reg(0x04, &data, 1);
	data = 0x0;
	ret |= akm4753_i2c_write_regs(0x04, &data, 1);

	if (akm4753->pdn->gpio != -1){
		gpio_set_value(akm4753->pdn->gpio, akm4753->pdn->active_level);
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

static int __devinit akm4753_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        int ret = 0;
        struct device *dev = &client->dev;
        struct akm4753_platform_data *akm4753 = dev->platform_data;

        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
                ret = -ENODEV;
                return ret;
	}

	if (akm4753->pdn->gpio != -1) {
		ret = gpio_request_one(akm4753->pdn->gpio,
				GPIOF_DIR_OUT, "akm4753-pdn");
		if (ret != 0) {
			dev_err(dev, "Can't request pdn pin = %d\n",
					akm4753->pdn->gpio);
			return ret;
		} else
			gpio_set_value(akm4753->pdn->gpio, akm4753->pdn->active_level);
	}

        akm4753_client = client;

        return 0;
}

static int __devexit akm4753_i2c_remove(struct i2c_client *client)
{
        struct device *dev = &client->dev;
        struct akm4753_platform_data *akm4753 = dev->platform_data;

	if (akm4753->pdn->gpio != -1)
		gpio_free(akm4753->pdn->gpio);

        akm4753_client = NULL;

        return 0;
}

static int jz_codec_probe(struct platform_device *pdev)
{
	codec_platform_data = pdev->dev.platform_data;

	codec_platform_data->codec_sys_clk = AKM4753_EXTERNAL_CODEC_CLOCK;

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

static const struct i2c_device_id akm4753_id[] = {
        { "akm4753", 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, akm4753_id);

static struct i2c_driver akm4753_i2c_driver = {
        .driver.name = "akm4753",
        .probe = akm4753_i2c_probe,
        .remove = __devexit_p(akm4753_i2c_remove),
        .id_table = akm4753_id,
};

static struct platform_driver jz_codec_driver = {
        .probe          = jz_codec_probe,
        .remove         = __devexit_p(jz_codec_remove),
        .driver         = {
                .name   = "akm4753_codec",
                .owner  = THIS_MODULE,
        },
};

/**
 * Module init
 */
static int __init init_akm4753_codec(void)
{
	int ret = 0;

	mutex_init(&i2c_access_lock);
	mutex_init(&switch_dev_lock);

	ret = i2s_register_codec("i2s_external_codec", (void *)jzcodec_ctl, AKM4753_EXTERNAL_CODEC_CLOCK, CODEC_MODE);
	if (ret < 0){
		printk("i2s audio is not support\n");
		return ret;
	}

        ret = platform_driver_register(&jz_codec_driver);
        if (ret) {
                printk("JZ CODEC: Could not register jz_codec_driver\n");
                return ret;
        }

        ret = i2c_add_driver(&akm4753_i2c_driver);
        if (ret) {
                printk("JZ CODEC: Could not register akm4753 i2c driver\n");
		platform_driver_unregister(&jz_codec_driver);
                return ret;
        }

#ifdef CONFIG_HAS_EARLYSUSPEND
        early_suspend.suspend = codec_early_suspend;
        early_suspend.resume = codec_late_resume;
        early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
        register_early_suspend(&early_suspend);
#endif
	printk("audio codec akm4753 register success\n");
	return 0;
}

/**
 * Module exit
 */
static void __exit cleanup_akm4753_codec(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
        unregister_early_suspend(&early_suspend);
#endif
	i2c_del_driver(&akm4753_i2c_driver);
	platform_driver_unregister(&jz_codec_driver);
}
module_init(init_akm4753_codec);
module_exit(cleanup_akm4753_codec);
MODULE_LICENSE("GPL");
