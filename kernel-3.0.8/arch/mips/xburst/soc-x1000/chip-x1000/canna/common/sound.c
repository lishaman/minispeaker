
#include <mach/jzsnd.h>
#include "board_base.h"

struct snd_codec_data codec_data = {
	.codec_sys_clk = 0 ,  //0:12M  1:13M
	.codec_dmic_clk = 1,
	/* volume */

	.record_volume_base = 0,
	.record_digital_volume_base = 0,
	.replay_digital_volume_base = 50,

	/* default route */
	.replay_def_route = {
		.route = SND_ROUTE_REPLAY_SPK,
		.gpio_spk_en_stat = STATE_ENABLE,
		.replay_digital_volume_base = 50,
	},

	.record_def_route = {
		.route = SND_ROUTE_RECORD_AMIC,
		.gpio_spk_en_stat = STATE_DISABLE,
		.record_digital_volume_base = 50,
	},

	.record_buildin_mic_route = {
		.route = SND_ROUTE_NONE,
	},

	.record_linein_route = {
		.route = SND_ROUTE_LINEIN_REPLAY_MIXER_LOOPBACK,
		.gpio_spk_en_stat = STATE_ENABLE,
		.record_digital_volume_base = 50,
	},

	.replay_speaker_route = {
		.route = SND_ROUTE_REPLAY_SOUND_MIXER_LOOPBACK,
		.gpio_spk_en_stat = STATE_ENABLE,
		.replay_digital_volume_base = 50,
	},

	.replay_speaker_record_buildin_mic_route = {
		.route = SND_ROUTE_RECORD_AMIC_AND_REPLAY_SPK,
		.gpio_spk_en_stat = STATE_ENABLE,
		.replay_digital_volume_base = 50,
		.record_digital_volume_base = 50,
	},

	.gpio_spk_en = {.gpio = GPIO_SPEAKER_EN, .active_level = GPIO_SPEAKER_EN_LEVEL},
	.gpio_amp_power = {.gpio = GPIO_AMP_POWER_EN, .active_level = GPIO_AMP_POWER_EN_LEVEL},
	.gpio_hp_mute = {.gpio = GPIO_HP_MUTE, .active_level = GPIO_HP_MUTE_LEVEL},
	.gpio_hp_detect = {.gpio = GPIO_HP_DETECT, .active_level = GPIO_HP_INSERT_LEVEL},
	.gpio_linein_detect = {.gpio = GPIO_LINEIN_DETECT, .active_level = GPIO_LINEIN_INSERT_LEVEL},
};

/* There is no AKM4345 on canna board, you should modify the code on your board */
#ifdef CONFIG_AKM4345_EXTERNAL_CODEC
static struct snd_board_gpio power_down = {
        .gpio = GPIO_AKM4345_PDN,
        .active_level = HIGH_ENABLE,
};
static struct snd_board_gpio spi_csn = {
        .gpio = GPIO_AKM4345_CSN,
        .active_level = LOW_ENABLE,
};
static struct snd_board_gpio spi_cclk = {
        .gpio = GPIO_AKM4345_CCLK,
        .active_level = LOW_ENABLE,
};
static struct snd_board_gpio spi_cdti = {
        .gpio = GPIO_AKM4345_CDTI,
        .active_level = LOW_ENABLE,
};
static struct snd_board_gpio spi_cdto = {
        .gpio = GPIO_AKM4345_CDTO,
        .active_level = LOW_ENABLE,
};

struct akm4345_platform_data akm4345_spi_data = {
        .pdn = &power_down,
	.csn = &spi_csn,
	.cclk = &spi_cclk,
	.cdti = &spi_cdti,
	.cdto = &spi_cdto,
};

struct platform_device akm4345_spi_device = {
        .name = "akm4345_spi",
};
#endif
