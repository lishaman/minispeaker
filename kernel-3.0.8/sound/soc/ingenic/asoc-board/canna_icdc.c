 /*
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: cli <chen.li@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <linux/gpio.h>
#include "../icodec/icdc_d3.h"

#define GPIO_PG(n)      (5*32 + 23 + n)
#define CANNA_SPK_GPIO GPIO_PC(25)
#define CANNA_SPK_EN 0

unsigned long codec_sysclk = -1;
static int canna_spk_power(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		gpio_direction_output(CANNA_SPK_GPIO, CANNA_SPK_EN);
		printk("gpio speaker enable %d\n", gpio_get_value(CANNA_SPK_GPIO));
	} else {
		gpio_direction_output(CANNA_SPK_GPIO, !CANNA_SPK_EN);
		printk("gpio speaker disable %d\n", gpio_get_value(CANNA_SPK_GPIO));
	}
	return 0;
}

void canna_spk_sdown(struct snd_pcm_substream *sps){
		gpio_direction_output(CANNA_SPK_GPIO, !CANNA_SPK_EN);
		return;
}

int canna_spk_sup(struct snd_pcm_substream *sps){
		gpio_direction_output(CANNA_SPK_GPIO, CANNA_SPK_EN);
		return 0;
}

static struct snd_soc_ops canna_i2s_ops = {
	.startup = canna_spk_sup,
	.shutdown = canna_spk_sdown,
};
static const struct snd_soc_dapm_widget canna_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", canna_spk_power),
	SND_SOC_DAPM_MIC("Mic Buildin", NULL),
	SND_SOC_DAPM_MIC("DMic", NULL),
};

static struct snd_soc_jack canna_icdc_d3_hp_jack;
static struct snd_soc_jack_pin canna_icdc_d3_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

static struct snd_soc_jack_gpio canna_icdc_d3_jack_gpio[] = {
	{
		.name = "Headphone detection",
		.report = SND_JACK_HEADPHONE,
		.debounce_time = 150,
	}
};


/* canna machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	/* ext speaker connected to DO_LO_PWM  */
	{"Speaker", NULL, "DO_LO_PWM"},

	/* mic is connected to AIP/N1 */
	{"MICBIAS", NULL, "Mic Buildin"},
	{"DMIC", NULL, "DMic"},

};

static int canna_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int err;
	err = snd_soc_dapm_new_controls(dapm, canna_dapm_widgets,
			ARRAY_SIZE(canna_dapm_widgets));
	if (err){
		printk("canna dai add controls err!!\n");
		return err;
	}
	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err){
		printk("add phoenix dai routes err!!\n");
		return err;
	}
	snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &canna_icdc_d3_hp_jack);
	snd_soc_jack_add_pins(&canna_icdc_d3_hp_jack,ARRAY_SIZE(canna_icdc_d3_hp_jack_pins),canna_icdc_d3_hp_jack_pins);
#ifdef HAVE_HEADPHONE
	if (gpio_is_valid(DORADO_HP_DET)) {
		canna_icdc_d3_jack_gpio[jack].gpio = PHOENIX_HP_DET;
		canna_icdc_d3_jack_gpio[jack].invert = !PHOENIX_HP_DET_LEVEL;
		snd_soc_jack_add_gpios(&canna_icdc_d3_hp_jack, 1, canna_icdc_d3_jack_gpio);
	}
#else
	snd_soc_dapm_disable_pin(dapm, "Headphone Jack");
#endif

	snd_soc_dapm_force_enable_pin(dapm, "Speaker");
	snd_soc_dapm_force_enable_pin(dapm, "Mic Buildin");
	snd_soc_dapm_force_enable_pin(dapm, "DMic");

	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link canna_dais[] = {
	[0] = {
		.name = "canna ICDC",
		.stream_name = "canna ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = canna_dlv_dai_link_init,
		.codec_dai_name = "icdc-d3-hifi",
		.codec_name = "icdc-d3",
		.ops = &canna_i2s_ops,
	},
	[1] = {
		.name = "canna PCMBT",
		.stream_name = "canna PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
	[2] = {
		.name = "CANNA DMIC",
		.stream_name = "CANNA DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-aic-dmic",
		.codec_dai_name = "dmic dump dai",
		.codec_name = "dmic dump",
	},

};

static struct snd_soc_card canna = {
	.name = "canna",
	.owner = THIS_MODULE,
	.dai_link = canna_dais,
	.num_links = ARRAY_SIZE(canna_dais),
};

static struct platform_device *canna_snd_device;

static int canna_init(void)
{
	/*struct jz_aic_gpio_func *gpio_info;*/
	int ret;
	printk("===>%s\n",__func__);
	ret = gpio_request(CANNA_SPK_GPIO, "Speaker_en");
	if (ret)
		return ret;
	canna_snd_device = platform_device_alloc("soc-audio", -1);
	if (!canna_snd_device) {
		gpio_free(CANNA_SPK_GPIO);
		return -ENOMEM;
	}

	platform_set_drvdata(canna_snd_device, &canna);
	ret = platform_device_add(canna_snd_device);
	if (ret) {
		platform_device_put(canna_snd_device);
		gpio_free(CANNA_SPK_GPIO);
	}
	printk("===>%s,init ok!!!\n",__func__);
	dev_info(&canna_snd_device->dev, "Alsa sound card:canna init ok!!!\n");
	return ret;
}

static void canna_exit(void)
{
	platform_device_unregister(canna_snd_device);
	gpio_free(CANNA_SPK_GPIO);
}

module_init(canna_init);
module_exit(canna_exit);

MODULE_AUTHOR("sccheng<shicheng.cheng@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC canna Snd Card");
MODULE_LICENSE("GPL");
