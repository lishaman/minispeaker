/*
 * Linux/sound/oss/jz4760_route_conf.h
 *
 * DLV CODEC driver for Ingenic Jz4760 MIPS processor
 *
 * 2010-11-xx   jbbi <jbbi@ingenic.cn>
 *
 * Copyright (c) Ingenic Semiconductor Co., Ltd.
 */

#ifndef __M200_ROUTE_CONF_H__
#define __M200_ROUTE_CONF_H__

#include <mach/jzsnd.h>

typedef struct __route_conf_base {
	/*--------route-----------*/
	int route_ready_mode;

	int route_record_mux_mode;
	int route_record_mixer_mode;
	int route_input_mode;
	int route_adc_mode;
	int route_mic_mode;

	int route_replay_mux_mode;
	int route_replay_mixer_mode;
	int route_output_mode;
	int route_dac_mode;
	/*--------attibute-------*/
	int attibute_agc_mode;

	//NOTE excet mixer gain,other gain better set in board config
	int attibute_mic_gain;			//val:0 ~ 5  => 0(db) ~ 20(db)   | 4db/step;
	int attibute_adc_gain;			//val:0 ~ 43 => 0(db) ~ 43(db)   | 1db/step;
	int attibute_dac_gain;			//val:0 ~ 63 => -31(db) ~ 32(db) | 1db/step;
	int attibute_record_mixer_gain; //val:0 ~ 32 => -31(db) ~ 0(db)  | 1db/step;
	int attibute_replay_mixer_gain; //val:0 ~ 32 => -31(db) ~ 0(db)  | 1db/step;
} route_conf_base;

struct __codec_route_info {
	enum snd_codec_route_t route_name;
	route_conf_base const *route_conf;
};

/*================ route conf ===========================*/
#define DISABLE								99
/*-------------- route part selection -------------------*/
#define ROUTE_READY_FOR_ADC					1
#define ROUTE_READY_FOR_DAC					2
#define ROUTE_READY_FOR_ADC_DAC				3

#define DMIC_ENABLE 4
#define LINEIN_ENABLE 3
#define AMIC_ENABLE 2
#define MIC_DISABLE 1

#define INPUT_TO_ADC_ENABLE 2
#define INPUT_TO_ADC_DISABLE 1

#define ADC_ENABLE_WITH_AMIC 2
#define ADC_ENABLE_WITH_DMIC 1
#define ADC_DISABLE 3

#define RECORD_NORMAL_INPUT 1
#define RECORD_CROSS_INPUT 2
#define RECORD_INPUT_AND_MIXER_NORMAL 3
#define RECORD_INPUT_AND_MIXER_CROSS 4	
#define RECORD_INPUT_AND_MIXER_L_CROSS_R_NORMAL 5
#define RECORD_INPUT_AND_MIXER_L_NORMAL_R_CROSS 6

#define RECORD_MIXER_ENABLE 1
#define RECORD_MIXER_DISABLE 2
#define RECORD_MIXER_NORMAL_INPUT 3
#define RECORD_MIXER_CROSS_INPUT 4
#define RECORD_MIXER_L_MIX_R_0_INPUT 5
#define RECORD_MIXER_L_0_R_MIX_INPUT 6
#define RECORD_MIXER_L_0_R_0_INPUT 7
#define RECORD_MIXER_L_NORMAL_R_CROSS_INPUT 8
#define RECORD_MIXER_L_CROSS_R_NORMAL_INPUT 9

#define REPLAY_NORMAL_INPUT 1
#define REPLAY_CROSS_INPUT 2
#define REPLAY_INPUT_AND_MIXER_NORMAL 3
#define REPLAY_INPUT_AND_MIXER_CROSS 4	
#define REPLAY_INPUT_AND_MIXER_L_CROSS_R_NORMAL 5
#define REPLAY_INPUT_AND_MIXER_L_NORMAL_R_CROSS 6

#define REPLAY_MIXER_ENABLE 1
#define REPLAY_MIXER_DISABLE 2
#define REPLAY_MIXER_NORMAL_INPUT 3
#define REPLAY_MIXER_CROSS_INPUT 4
#define REPLAY_MIXER_L_MIX_R_0_INPUT 5
#define REPLAY_MIXER_L_0_R_MIX_INPUT 6
#define REPLAY_MIXER_L_0_R_0_INPUT 7

#define OUTPUT_FROM_DAC_ENABLE 2
#define OUTPUT_FROM_DAC_DISABLE 1

#define DAC_ENABLE 2
#define DAC_DISABLE 1

#define AGC_ENABLE 2
#define AGC_DISABLE 1



extern struct __codec_route_info codec_route_info[];

#endif /*__M200_ROUTE_CONF_H__*/
