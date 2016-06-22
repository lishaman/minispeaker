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

static struct snd_soc_ops canna_i2s_ops = {

};

#ifndef GPIO_PG
#define GPIO_PG(n)      (5*32 + 23 + n)
#endif

static struct snd_soc_dai_link canna_dais[] = {
	[0] = {
		.name = "CANNA ICDC",
		.stream_name = "CANNA ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-spdif",
		.codec_dai_name = "spdif dump dai",
		.codec_name = "spdif dump",
		.ops = &canna_i2s_ops,
	},
	[1] = {
		.name = "CANNA PCMBT",
		.stream_name = "CANNA PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
	[2] = {
		.name = "CANNA DMIC",
		.stream_name = "CANNA DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-dmic",
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
	canna_snd_device = platform_device_alloc("soc-audio", -1);
	if (!canna_snd_device)
		return -ENOMEM;

	platform_set_drvdata(canna_snd_device, &canna);
	ret = platform_device_add(canna_snd_device);
	if (ret) {
		platform_device_put(canna_snd_device);
	}

	dev_info(&canna_snd_device->dev, "Alsa sound card:canna init ok!!!\n");
	return ret;
}

static void canna_exit(void)
{
	platform_device_unregister(canna_snd_device);
}

module_init(canna_init);
module_exit(canna_exit);

MODULE_AUTHOR("shicheng.cheng<shicheng.cheng@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC canna Snd Card");
MODULE_LICENSE("GPL");
