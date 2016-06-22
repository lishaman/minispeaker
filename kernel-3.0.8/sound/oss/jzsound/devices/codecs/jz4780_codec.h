/*
 * Linux/sound/oss/xb47XX/
 *
 * Copyright (c) Ingenic Semiconductor Co., Ltd.
 */

#ifndef __JZ4780_CODEC_H__
#define __JZ4780_CODEC_H__

#include <mach/jzsnd.h>
#include "../xb47xx_i2s.h"
#include <linux/bitops.h>

/* Standby */
#define STANDBY 		1

/* Power setting */
#define POWER_ON		0
#define POWER_OFF		1

/* File ops mode W & R */
#define REPLAY			1
#define RECORD			2

/* Channels */
#define LEFT_CHANNEL		1
#define RIGHT_CHANNEL		2

#define MAX_RATE_COUNT		11

/*
 * JZ4780 CODEC CODEC registers
 */
/*==========================================*/
#define CODEC_REG_SR		0x0
/*------------------*/
 #define SR_PON_ACK		7
 #define SR_IRQ_ACK		6
 #define SR_JACK		5
 #define SR_DAC_LOCKED          4

#define CODEC_REG_SR2           0x1
/*-----------------*/
 #define SR2_DAC_UNKOWN_FS      4

#define CODEC_REG_MR            0x7
 #define MR_ADC_MODE_UPD        5
 #define MR_JADC_MUTE_STATE     3// 2 bits
   #define MR_JADC_MUTE_STATE_MASK (0x3 << MR_JADC_MUTE_STATE)
 #define MR_DAC_MODE_UPD        2
 #define MR_DAC_MUTE_STATE      0
   #define MR_DAC_MUTE_STATE_MASK  (0x3)

#define CODEC_REG_AICR_DAC	0x8
/*------------------*/
 #define AICR_DAC_ADWL		6 // 2 bits
   #define AICR_DAC_ADWL_MASK	(0x3 << AICR_DAC_ADWL)
 #define AICR_DAC_MODE		5
 #define AICR_DAC_SB		4
 #define AICR_DAC_AUDIOIF	0
  #define AICR_DAC_AUDIOIF_MASK	(0x3 << AICR_DAC_AUDIOIF)

#define CODEC_REG_AICR_ADC      0x9
/*------------------*/
 #define AICR_ADC_ADWL          6
  #define AICR_ADC_ADWL_MASK   (0x3 << AICR_ADC_ADWL)
 #define AICR_ADC_SB           4
 #define AICR_ADC_AUDIOIF       0
  #define AICR_ADC_AUDIOIF_MASK (0x3 << AICR_ADC_AUDIOIF)

#define CODEC_REG_CR_LO         0xb
/*-----------------*/
 #define CR_LO_MUTE             7
 #define CR_SB_LO               4
 #define CR_LO_SEL              0// 2 bits /FIXME CODEC_QUE/
  #define CR_LO_SEL_MASK 		(0x7 << CR_LO_SEL)

#define CODEC_REG_CR_HP         0xd
/*------------------*/
 #define CR_HP_MUTE             7
 #define CR_SB_HP               4
 #define CR_HP_SEL              0// 3 bits
  #define CR_HP_SEL_MASK            (0x7 << CR_HP_SEL)

#define CODEC_REG_CR_DMIC       0x10
/*------------------*/
 #define CR_DMIC_CLKON          7


#define CODEC_REG_CR_MIC1       0x11
/*------------------*/
 #define CR_MIC1_MICSTEREO      7
 #define CR_MIC1_MIC1DFF		6
 #define CR_SB_MICBIAS1			5
 #define CR_SB_MIC1				4
 #define CR_MIC1_SEL			0


#define CODEC_REG_CR_MIC2       0x12
/*------------------*/
 #define CR_MIC2_MIC2DFF        6
 #define CR_SB_MICBIAS2         5
 #define CR_SB_MIC2             4
 #define CR_MIC2_SEL            0

#define CODEC_REG_CR_LI1        0x13
/*------------------*/
 #define CR_LI1_LIN1DIF         6
 #define CR_SB_LIBY1            4
 #define CR_LI1_SEL             0


#define CODEC_REG_CR_LI2        0x14
/*------------------*/
 #define CR_LI2_LIN2DIF         6
 #define CR_SB_LIBY2            4
 #define CR_LI2_SEL             0

#define CODEC_REG_CR_DAC        0x17
/*------------------*/
 #define CR_DAC_MUTE            7
 #define CR_DAC_LEFT_ONLY       5
 #define CR_SB_DAC              4

#define CODEC_REG_CR_ADC        0x18
/*------------------*/
 #define CR_ADC_MUTE            7
 #define CR_ADC_DMIC_SEL        6
 #define CR_ADC_LEFT_ONLY       5
 #define CR_SB_ADC              4
 #define CR_ADC_IN_SEL          0// 2 bits
  #define CR_ADC_IN_SEL_MASK    (0x3 << CR_ADC_IN_SEL)

#define CODEC_REG_CR_MIX        0x19
/*------------------*/
 #define CR_MIX_EN              7
 #define CR_MIX_LOAD            6
 #define CR_MIX_ADD             0//2 bits
  #define CR_MIX_ADD_MASK       (0x3 << CR_MIX_ADD)
/*MIX 0*/
		#define CR_MIX0			0X0
/*MIX 1*/
		#define CR_MIX1			0X1
/*MIX 2*/
		#define CR_MIX2			0x2
/*MIX 3*/
		#define CR_MIX3			0X3

#define CODEC_REG_DR_MIX        0x1A
/*------------------*/
 #define DR_MIX_DATA            0//8 bits
  #define DR_MIX_DATA_MASK      (0xff << DR_MIX_DATA)
/*MIX 0 - 4 register*/
	#define MIX_L_SEL			6
	#define MIX_L_SEL_MASK		(0xf << MIX_L_SEL)
	#define MIX_R_SEL			4
	#define MIX_R_SEL_MASK		(0xf << MIX_R_SEL)
	#define MIX_MODE_SEL		0

#define CODEC_REG_CR_VIC        0x1b
/*------------------*/
 #define CR_VIC_SB_SLEEP        1
 #define CR_VIC_SB              0

#define CODEC_REG_CR_CK         0x1c
/*------------------*/
 #define CR_CK_SHUTDOWN_CLOCK   4
 #define CR_CK_CRYSTAL          0// 4 bits
  #define CR_CK_CRYSTAL_MASK    (0xf << CR_CK_CRYSTAL)

#define CODEC_REG_FCR_DAC       0x1d
/*------------------*/
 #define FCR_DAC_FREQ           0
  #define FCR_DAC_FREQ_MASK     (0xf << FCR_DAC_FREQ)

#define CODEC_REG_FCR_ADC       0x20
/*------------------*/
 #define FCR_ADC_HPF            6
 #define FCR_ADC_WNF            4// 2 bits
  #define FCR_ADC_WNF_MASK      (0x3 << FCR_ADC_WNF)
 #define FCR_ADC_FREQ           0
  #define FCR_ADC_FREQ_MASK     (0xf << FCR_ADC_FREQ)

#define CODEC_REG_CR_TIMER_MSB  0x21
/*------------------*/
 #define CR_TIMER_MSB           0
  #define CR_TIMER_MSB_MASK     (0xff << CR_TIMER_MSB)

#define CODEC_REG_CR_TIMER_LSB  0x22
/*------------------*/
 #define CR_TIMER_LSB           0
  #define CR_TIMER_LSB_MASK     (0xff << CR_TIMER_LSB)

#define CODEC_REG_ICR           0x23
/*------------------*/
 #define ICR_INT_FORM           6
  #define ICR_INT_FORM_MASK     (0x3 << ICR_INT_FORM)

#define CODEC_REG_IMR           0x24
/*------------------*/
 #define IMR_LOCK_MASK          (1 << 7)
 #define IMR_SCLR_MASK          (1 << 6)
 #define IMR_JACK_MASK          (1 << 5)
 #define IMR_ADC_MUTE_MASK      (1 << 2)
 #define IMR_DAC_MODE_MASK      (1 << 1)
 #define IMR_DAC_MUTE_MASK      (1 << 0)

#define CODEC_REG_IFR           0x25
/*------------------*/
 #define IFR_LOCK_EVENT         7
 #define IFR_SCLR_EVENT         6
 #define IFR_JACK_EVENT         5
 #define IFR_ADC_MUTE_EVENT     2
 #define IFR_DAC_MODE_EVENT     1
 #define IFR_DAC_MUTE_EVENT     0

#define CODEC_REG_IMR2          0x26
/*------------------*/
 #define IMR2_TIMER_END_MASK    4

#define CODEC_REG_IFR2          0x27
/*------------------*/
 #define IFR2_TIMER_END_EVENT   4

#define CODEC_REG_GCR_HPL       0x28
/*------------------*/
 #define GCR_HPL_LRGO           7
 #define GCR_HPL_GOL            0
  #define GCR_HPL_GOL_MASK      (0x1f << GCR_HPL_GOL)

#define CODEC_REG_GCR_HPR       0x29
/*------------------*/
 #define GCR_HPR_GOR            0
  #define GCR_HPR_GOR_MASK      (0x1f << GCR_HPR_GOR)

#define CODEC_REG_GCR_LIBYL     0x2A
/*------------------*/
 #define GCR_LIBYL_LRGI         7
 #define GCR_LIBYL_GIL          0
  #define GCR_LIBYL_GIL_MASK    (0x1f << GCR_LIBYL_GIL)

#define CODEC_REG_GCR_LIBYR     0x2b
/*------------------*/
#define GCR_LIBYR_GIR           0
 #define GCR_LIBYR_GIR_MASK     (0x1f <<  GCR_LIBYR_GIR)

#define CODEC_REG_GCR_DACL      0x2c
/*------------------*/
 #define GCR_DACL_RLGOD         7
 #define GCR_DACL_GODL          0
  #define GCR_DACL_GODL_MASK    (0x1f <<  GCR_DACL_GODL)

#define CODEC_REG_GCR_DACR      0x2d
/*------------------*/
 #define GCR_DACR_GODR          0
  #define GCR_DACR_GODR_MASK    (0x1f <<  GCR_DACR_GODR)


#define CODEC_REG_GCR_MIC1      0x2e
/*------------------*/
 #define GCR_MIC1_GIM1          0
  #define GCR_MIC1_GIM1_MASK    (0x7 << GCR_MIC1_GIM1)


#define CODEC_REG_GCR_MIC2      0x2f
/*------------------*/
 #define GCR_MIC2_GIM2          0
  #define GCR_MIC2_GIM2_MASK    (0x7 << GCR_MIC2_GIM2)

#define CODEC_REG_GCR_ADCL      0x30
/*------------------*/
 #define GCR_ADCL_LRGID         7
 #define GCR_ADCL_GIDL          0
  #define GCR_ADCL_GIDL_MASK    (0x3f << GCR_ADCL_GIDL)

#define CODEC_REG_GCR_ADCR      0x31
/*------------------*/
 #define GCR_ADCR_GIDR          0
  #define GCR_ADCR_GIDR_MASK    (0x3f << GCR_ADCR_GIDR)

#define CODEC_REG_GCR_MIXDACL   0x34
/*------------------*/
 #define GCR_MIXDAC_LRGOMIX     7
 #define GCR_MIXDAC_GOMIXL      0
  #define GCR_MIXDAC_GOMIXL_MASK	(0x1f << GCR_MIXDAC_GOMIXL)

#define CODEC_REG_GCR_MIXDACR   0x35
/*------------------*/
 #define GCR_MIXDAC_GOMIXR      0
  #define GCR_MIXDAC_GOMIXR_MASK	(0x1f << GCR_MIXDAC_GOMIXR)

#define CODEC_REG_GCR_MIXADCL   0x36
/*------------------*/
 #define GCR_MIXADC_LRGIMIX     7
 #define GCR_MIXADC_GIMIXL		0
  #define GCR_MIXADC_GIMIXL_MASK	(0x1f << GCR_MIXADC_GIMIXL)


#define CODEC_REG_GCR_MIXADCR   0x37
/*------------------*/
 #define GCR_MIXADC_GIMIXR		0
  #define GCR_MIXADC_GIMIXR_MASK	(0x1f << GCR_MIXADC_GIMIXR)

#define CODEC_REG_CR_ADC_AGC    0x3a
/*------------------*/
 #define CR_ADC_AGC_EN          7
 #define CR_ADC_AGC_LOAD        6
 #define CR_ADC_AGC_ADD         0
  #define CR_ADC_AGC_ADD_MASK   (0x3f << CR_ADC_AGC_ADD)

#define CODEC_REG_DR_ADC_AGC    0x3b
/*------------------*/
 #define DR_ADC_AGC_DATA        0
  #define DR_ADC_AGC_DATA_MASK  (0xff << DR_ADC_AGC_DATA)
/*AGC0*/
        #define AGC0_STEREO             6
        #define AGC0_TARGET             1
         #define AGC0_TARGET_MAS        (0x1f << AGC0_TARGET)
/*AGC1*/
        #define AGC1_NG_EN              7
        #define AGC1_NG_THR             4
         #define ACC1_NG_THR_MASK       (0x7 << AGC1_NG_THR)
        #define AGC1_HOLD               0
         #define AGC1_HOLD_MASK         (0xf << AGC1_HOLD)
/*AGC2*/
        #define AGC2_ATK                4
         #define AGC2_ATK_MASK          (0xf << AGC2_ATK)
        #define AGC2_DCY                0
         #define AGC2_DCY_MASK          (0xf << AGC2_DCY)
/*AGC3*/
        #define AGC3_MAX                0
         #define AGC3_MAX_MASK          (0x1f << AGC3_MAX)
/*AGC4*/
        #define AGC4_MIN                0
         #define AGC4_MIN_MASK          (0x1f << AGC4_MIN)


/*==========================CODEC SR===================================*/
/*SR,SR2*/
#define CODEC_PON_ACK_MASK		BIT(SR_PON_ACK)
#define CODEC_IRQ_ACK_MASK		BIT(SR_IRQ_ACK)
#define CODEC_JACK_MASK			BIT(SR_JACK)
#define CODEC_DAC_LOCKED_MASK	BIT(SR_DAC_LOCKED)
#define CODEC_DAC_UNKOWN_FS_MASK	BIT(SR2_DAC_UNKOWN_FS)

#define __codec_get_sr()	read_inter_codec_reg(CODEC_REG_SR)
#define __codec_get_sr2()	read_inter_codec_reg(CODEC_REG_SR2)
/* ops */
/* misc ops*/

/*============================== ADC/DAC ==============================*/

/*AICR_ADC,AICR_DAC : adc ,dac configure*/
#define CODEC_SLAVE_MODE	1
#define CODEC_MASTER_MODE	0

#define __codec_select_slave_mode()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_AICR_DAC,CODEC_SLAVE_MODE ,AICR_DAC_MODE);	\
} while (0)
#define __codec_select_master_mode()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_AICR_DAC,CODEC_MASTER_MODE ,AICR_DAC_MODE);	\
} while (0)


/*adc dac interface mode*/
#define CODEC_PARALLEL_INTERFACE		0
#define CODEC_LEFTJUSTIFIED_INTERFACE	1
#define CODEC_DSP_INTERFACE				2
#define CODEC_I2S_INTERFACE				3
#define __codec_select_adc_digital_interface(mode)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_AICR_ADC, mode,	\
				AICR_ADC_AUDIOIF_MASK,AICR_ADC_AUDIOIF);	\
} while (0)

#define __codec_get_adc_digital_interface()	\
	((read_inter_codec_reg(CODEC_REG_AICR_ADC) & AICR_ADC_AUDIOIF_MASK) >> AICR_ADC_AUDIOIF)

#define __codec_select_dac_digital_interface(mode)				\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_AICR_DAC, mode,	\
	            AICR_DAC_AUDIOIF_MASK,AICR_DAC_AUDIOIF);	\
} while (0)

#define __codec_get_dac_digital_interface()	\
	((read_inter_codec_reg(CODEC_REG_AICR_DAC) & AICR_DAC_AUDIOIF_MASK) >> AICR_DAC_AUDIOIF)


#define __codec_enable_adc_interface()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_AICR_ADC, POWER_ON, AICR_ADC_SB);	\
} while (0)

#define __codec_get_adc_interface_state()	\
	((read_inter_codec_reg(CODEC_REG_AICR_ADC) & (1 << AICR_DAC_SB)) >> AICR_DAC_SB)

#define __codec_get_dac_interface_state()	\
	((read_inter_codec_reg(CODEC_REG_AICR_DAC) & (1 << AICR_DAC_SB)) >> AICR_DAC_SB)

#define __codec_disable_adc_interface()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_AICR_ADC, POWER_OFF, AICR_ADC_SB);	\
} while (0)

#define __codec_enable_dac_interface()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_AICR_DAC, POWER_ON, AICR_DAC_SB);	\
} while (0)

#define __codec_disable_dac_interface()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_AICR_DAC, POWER_OFF, AICR_DAC_SB);	\
} while (0)

#define CODEC_ADC_16BIT_SAMPLE  0
#define CODEC_ADC_18BIT_SAMPLE  1
#define CODEC_ADC_20BIT_SAMPLE  2
#define CODEC_ADC_24BIT_SAMPLE  3

#define __codec_select_adc_word_length(width)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_AICR_ADC,width,	\
                          AICR_ADC_ADWL_MASK, AICR_ADC_ADWL);	\
} while (0)

#define __codec_get_adc_word_length()   ((read_inter_codec_reg(CODEC_REG_AICR_ADC) &    \
                                               AICR_ADC_ADWL_MASK) >> AICR_ADC_ADWL)    \

#define __codec_select_dac_word_length(width)		\
do {								\
	write_inter_codec_reg_mask(CODEC_REG_AICR_DAC,width,         \
                         AICR_DAC_ADWL_MASK , AICR_DAC_ADWL);	\
} while (0)

#define __codec_get_dac_word_length()   ((read_inter_codec_reg(CODEC_REG_AICR_DAC) &    \
                                               AICR_DAC_ADWL_MASK) >> AICR_DAC_ADWL)    \
/* FCR_XXX :ADC DAC feq */
#define __codec_select_adc_samp_rate(val)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_FCR_ADC,val,FCR_ADC_FREQ_MASK,FCR_ADC_FREQ);	\
} while (0)

#define __codec_get_adc_samp_rate()     (read_inter_codec_reg(CODEC_REG_FCR_ADC) & \
                                        FCR_ADC_FREQ_MASK)	\

#define __codec_select_dac_samp_rate(val)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_FCR_DAC,val,FCR_DAC_FREQ_MASK,FCR_DAC_FREQ);	\
} while (0)

#define __codec_get_dac_samp_rate()     (read_inter_codec_reg(CODEC_REG_FCR_DAC) &     \
                                        FCR_DAC_FREQ_MASK)            \

/*========================= SB switch =================================*/
/*CR_XXX : power off*/
#define __codec_get_sb()	((read_inter_codec_reg(CODEC_REG_CR_VIC) &	\
                                (1 << CR_VIC_SB)) ?				\
                                POWER_OFF : POWER_ON)

#define __codec_switch_sb(pwrstat)							\
do {															\
	if (__codec_get_sb() != pwrstat) {							\
		write_inter_codec_reg_bit(CODEC_REG_CR_VIC, pwrstat,	\
								  CR_VIC_SB);				    \
		}														\
																\
} while (0)

#define __codec_get_sb_sleep()		((read_inter_codec_reg(CODEC_REG_CR_VIC) &	\
									(1 << CR_VIC_SB_SLEEP)) ?					\
									POWER_OFF : POWER_ON)

#define __codec_switch_sb_sleep(pwrstat)					\
do {															\
	if (__codec_get_sb_sleep() != pwrstat) {					\
		write_inter_codec_reg_bit(CODEC_REG_CR_VIC, pwrstat,	\
				CR_VIC_SB_SLEEP);								\
	}															\
																\
} while (0)

#define __codec_get_sb_dac()		((read_inter_codec_reg(CODEC_REG_CR_DAC) &	\
									(1 << CR_SB_DAC)) ?							\
									POWER_OFF : POWER_ON)

#define __codec_switch_sb_dac(pwrstat)						\
do {													        \
	if (__codec_get_sb_dac() != pwrstat) {				        \
		write_inter_codec_reg_bit(CODEC_REG_CR_DAC, pwrstat,	\
				CR_SB_DAC);										\
	}															\
																\
} while (0)

#define __codec_get_sb_line_out()	((read_inter_codec_reg(CODEC_REG_CR_LO) &	\
									(1 << CR_SB_LO)) ?		                    \
									POWER_OFF : POWER_ON)

#define __codec_switch_sb_line_out(pwrstat)					\
do {															\
	if (__codec_get_sb_line_out() != pwrstat) {					\
		write_inter_codec_reg_bit(CODEC_REG_CR_LO, pwrstat,		\
				CR_SB_LO);										\
	}															\
																\
} while (0)

#define __codec_get_sb_hp()		((read_inter_codec_reg(CODEC_REG_CR_HP) &	\
								(1 << CR_SB_HP)) ?							\
								POWER_OFF : POWER_ON)

#define __codec_switch_sb_hp(pwrstat)						\
do {															\
	if (__codec_get_sb_hp() != pwrstat) {		        		\
		write_inter_codec_reg_bit(CODEC_REG_CR_HP, pwrstat,		\
				CR_SB_HP);										\
	}															\
																\
} while (0)

#define __codec_get_sb_adc()	((read_inter_codec_reg(CODEC_REG_CR_ADC) &	\
								(1 << CR_SB_ADC)) ?							\
								POWER_OFF : POWER_ON)

#define __codec_switch_sb_adc(pwrstat)						\
do {															\
	if (__codec_get_sb_adc() != pwrstat) {						\
		write_inter_codec_reg_bit(CODEC_REG_CR_ADC, pwrstat,	\
								  CR_SB_ADC);					\
	}															\
																\
} while (0)

#define __codec_get_sb_mic1()	((read_inter_codec_reg(CODEC_REG_CR_MIC1) &	\
								(1 << CR_SB_MIC1)) ?						\
								POWER_OFF : POWER_ON)

#define __codec_switch_sb_mic1(pwrstat)						\
do {															\
	if (__codec_get_sb_mic1() != pwrstat) {		        		\
		write_inter_codec_reg_bit(CODEC_REG_CR_MIC1, pwrstat,	\
				CR_SB_MIC1);									\
	}															\
																\
} while (0)

#define __codec_get_sb_mic2()	((read_inter_codec_reg(CODEC_REG_CR_MIC2) &	\
								(1 << CR_SB_MIC2)) ?						\
								POWER_OFF : POWER_ON)

#define __codec_switch_sb_mic2(pwrstat)						\
do {															\
	if (__codec_get_sb_mic2() != pwrstat) {				        \
		write_inter_codec_reg_bit(CODEC_REG_CR_MIC2, pwrstat,	\
								  CR_SB_MIC2);					\
	}															\
																\
} while (0)


#define __codec_get_sb_micbias1()	((read_inter_codec_reg(CODEC_REG_CR_MIC1) &	\
									(1 << CR_SB_MICBIAS1)) ?              		\
									POWER_OFF : POWER_ON)

#define __codec_switch_sb_micbias1(pwrstat)					\
do {															\
	if (__codec_get_sb_micbias1() != pwrstat) {			        \
		write_inter_codec_reg_bit(CODEC_REG_CR_MIC1, pwrstat,	\
								  CR_SB_MICBIAS1);				\
	}															\
																\
} while (0)

#define __codec_get_sb_micbias2()	((read_inter_codec_reg(CODEC_REG_CR_MIC2) &	\
									(1 << CR_SB_MICBIAS2)) ?              		\
									POWER_OFF : POWER_ON)

#define __codec_switch_sb_micbias2(pwrstat)					\
do {															\
	if (__codec_get_sb_micbias2() != pwrstat) {					\
		write_inter_codec_reg_bit(CODEC_REG_CR_MIC2, pwrstat,	\
				CR_SB_MICBIAS2);								\
	}															\
																\
} while (0)


#define __codec_get_sb_linein1_bypass()		((read_inter_codec_reg(CODEC_REG_CR_LI1) &	\
											(1 << CR_SB_LIBY1)) ?		                \
		POWER_OFF : POWER_ON)

#define __codec_switch_sb_linein1_bypass(pwrstat)			\
do {															\
	if (__codec_get_sb_linein1_bypass() != pwrstat) {			\
		write_inter_codec_reg_bit(CODEC_REG_CR_LI1, pwrstat,	\
				CR_SB_LIBY1);									\
	}															\
																\
} while (0)

#define __codec_get_sb_linein2_bypass()		((read_inter_codec_reg(CODEC_REG_CR_LI2) &	\
											(1 << CR_SB_LIBY2))	?			            \
		POWER_OFF : POWER_ON)

#define __codec_switch_sb_linein2_bypass(pwrstat)			\
do {															\
	if (__codec_get_sb_linein2_bypass() != pwrstat) {	        \
		write_inter_codec_reg_bit(CODEC_REG_CR_LI2, pwrstat,	\
				CR_SB_LIBY2);									\
	}															\
																\
} while (0)

/*============================== IRQ ==================================*/
/*IFR ,IMR ,IMR2 ,IFR2 :set irq mask and clear irq flag*/
#define ICR_INT_HIGH		0
#define ICR_INT_LOW		1
#define ICR_INT_HIGH_8CYCLES	2
#define ICR_INT_LOW_8CYCLES	3

#define ICR_ALL_MASK            (IMR_ADC_MUTE_MASK | IMR_DAC_MODE_MASK | IMR_DAC_MUTE_MASK | IMR_LOCK_MASK | IMR_JACK_MASK | IMR_SCLR_MASK)

#ifdef CONFIG_JZ_HP_DETECT_CODEC
 #define ICR_COMMON_MASK        (IMR_ADC_MUTE_MASK | IMR_DAC_MODE_MASK | IMR_DAC_MUTE_MASK)
#else
 #define ICR_COMMON_MASK        (IMR_ADC_MUTE_MASK | IMR_DAC_MODE_MASK | IMR_DAC_MUTE_MASK | IMR_JACK_MASK)
#endif

#define ICR_ALL_MASK2           (IMR2_TIMER_END_MASK)

#define ICR_COMMON_MASK2        (IMR2_TIMER_END_MASK)

#define ICR_ALL_FLAG            (0xff)
#define ICR_ALL_FLAG2           (0xff)
#define REG_IFR_MASK            ICR_ALL_FLAG
#define REG_IFR_MASK2           ICR_ALL_FLAG2

#define REG_IMR_MASK            (0xff)
#define REG_IMR_MASK2           (0xff)

#define __codec_set_int_form(opt)												\
do {																				\
	write_inter_codec_reg_mask(CODEC_REG_ICR, opt,ICR_INT_FORM_MASK,ICR_INT_FORM); 	\
																					\
} while (0)



#define __codec_set_irq_mask(mask)				\
do {												\
	write_inter_codec_reg(CODEC_REG_IMR, mask);     \
													\
} while (0)

#define __codec_set_irq_mask2(mask)				\
do {												\
	write_inter_codec_reg(CODEC_REG_IMR2, mask);	\
													\
} while (0)
#define __codec_set_irq_flag(flag)				\
do {												\
	write_inter_codec_reg(CODEC_REG_IFR, flag);		\
													\
} while (0)

#define __codec_set_irq_flag2(flag)				\
do {												\
	write_inter_codec_reg(CODEC_REG_IFR2, flag);	\
													\
} while (0)
#define __codec_get_irq_flag()		(read_inter_codec_reg(CODEC_REG_IFR) &	\
					 REG_IFR_MASK)

#define __codec_get_irq_mask()		(read_inter_codec_reg(CODEC_REG_IMR) &	\
					 REG_IMR_MASK)

#define __codec_get_irq_flag2()		(read_inter_codec_reg(CODEC_REG_IFR2) &	\
					 REG_IFR_MASK2)

#define __codec_get_irq_mask2()		(read_inter_codec_reg(CODEC_REG_IMR2) &	\
					 REG_IMR_MASK2)

/*MR :mode off irq*/
#define CODEC_OPERATION_MODE    1
#define CODEC_PROGRAME_MODE     0
#define CODEC_IN_MUTE			3
#define CODEC_LEAVING_MUTE      2
#define CODEC_BEING_MUTE        1
#define CODEC_NOT_MUTE          0

#define __codec_test_adc_udp()          ((read_inter_codec_reg(CODEC_REG_MR) &  \
                                        (1 << MR_ADC_MODE_UPD)) ?	\
                                        CODEC_OPERATION_MODE :CODEC_PROGRAME_MODE)
#define __codec_test_dac_udp()          ((read_inter_codec_reg(CODEC_REG_MR) &  \
                                        (1 << MR_DAC_MODE_UPD)) ?	\
                                        CODEC_OPERATION_MODE :CODEC_PROGRAME_MODE)
#define __codec_test_jadc_mute_state()  (read_inter_codec_reg(CODEC_REG_MR) &   \
                                        MR_JADC_MUTE_STATE_MASK) >> MR_JADC_MUTE_STATE
#define __codec_test_dac_mute_state()   (read_inter_codec_reg(CODEC_REG_MR) &   \
                                        MR_DAC_MUTE_STATE_MASK) >> MR_DAC_MUTE_STATE

/*============================== MUTE =================================*/
/*CR_XXX : mute*/
#define CODEC_MUTE_ENABLE		1
#define CODEC_MUTE_DISABLE		0

#define __codec_get_hp_mute()	((read_inter_codec_reg(CODEC_REG_CR_HP) &	\
								(1 << CR_HP_MUTE)) ?	\
								CODEC_MUTE_ENABLE : CODEC_MUTE_DISABLE)

#define __codec_enable_hp_mute()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_HP, CODEC_MUTE_ENABLE, CR_HP_MUTE);	\
} while (0)

#define __codec_disable_hp_mute()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_HP, CODEC_MUTE_DISABLE, CR_HP_MUTE);	\
} while (0)

#define __codec_get_lineout_mute()  ((read_inter_codec_reg(CODEC_REG_CR_LO) &	\
									(1 << CR_LO_MUTE)) ?	\
									CODEC_MUTE_ENABLE : CODEC_MUTE_DISABLE)

#define __codec_enable_lineout_mute()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_LO, CODEC_MUTE_ENABLE, CR_LO_MUTE);	\
} while (0)

#define __codec_disable_lineout_mute()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_LO, CODEC_MUTE_DISABLE, CR_LO_MUTE);	\
} while (0)

#define __codec_get_dac_mute()      ((read_inter_codec_reg(CODEC_REG_CR_DAC) &	\
									(1 << CR_DAC_MUTE)) ?	\
									CODEC_MUTE_ENABLE : CODEC_MUTE_DISABLE)

#define __codec_enable_dac_mute()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_DAC, CODEC_MUTE_ENABLE, CR_DAC_MUTE);	\
} while (0)

#define __codec_disable_dac_mute()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_DAC, CODEC_MUTE_DISABLE, CR_DAC_MUTE);	\
} while (0)

#define __codec_get_adc_mute()      ((read_inter_codec_reg(CODEC_REG_CR_ADC) &	\
									(1 << CR_ADC_MUTE)) ?	\
									CODEC_MUTE_ENABLE : CODEC_MUTE_DISABLE)

#define __codec_enable_adc_mute()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_ADC, CODEC_MUTE_ENABLE, CR_ADC_MUTE);	\
} while (0)

#define __codec_disable_adc_mute()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_ADC, CODEC_MUTE_DISABLE, CR_ADC_MUTE);	\
} while (0)


/*============================== MISC =================================*/

/*CR_HP : hp ops*/
/*line out mux option*/
#define	CODEC_DACRL_TO_HP		0
#define CODEC_DACL_TO_HP		1
#define CODEC_INPUTRL_TO_HP		2
#define CODEC_INPUTL_TO_HP		3
#define CODEC_INPUTR_TO_HP		4

#define __codec_set_hp_mux(opt)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_CR_HP,opt,CR_HP_SEL_MASK,CR_HP_SEL);	\
} while (0)
/*CR_LO : line out ops*/
/*line out mux option*/
#define CODEC_DACL_TO_LO		0
#define CODEC_DACR_TO_LO		1
#define CODEC_DACLR_TO_LO		2
#define CODEC_INPUTL_TO_LO		4
#define CODEC_INPUTR_TO_LO		5
#define	CODEC_INPUTLR_TO_LO		6
#define __codec_set_lineout_mux(opt)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_CR_LO,opt,CR_LO_SEL_MASK,CR_LO_SEL);	\
} while (0)

/*CR_DAC :dac ops*/
/*dac mode option*/
#define CODEC_DAC_STEREO		0
#define CODEC_DAC_LEFT_ONLY		1

#define __codec_get_dac_mode()	\
	((read_inter_codec_reg(CODEC_REG_CR_DAC) & (1<<CR_DAC_LEFT_ONLY))>> CR_DAC_LEFT_ONLY)

#define __codec_set_dac_mode(opt)	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_DAC, opt, CR_DAC_LEFT_ONLY);	\
} while (0)

/*CR_ADC : adc ops*/
/*adc mux option*/
#define CODEC_INPUTL_TO_LR			0
#define CODEC_INPUTR_TO_LR			1
#define CODEC_INPUTLR_NORMAL		0
#define CODEC_INPUTLR_SWAP			1

#define __codec_set_adc_mux(opt)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_CR_ADC,opt,CR_ADC_IN_SEL_MASK,CR_ADC_IN_SEL);	\
} while (0)

#define CODEC_DMIC_SEL_ADC			0
#define CODEC_DMIC_SEL_DIGITAL_MIC	1

#define __codec_set_dmic_mux(opt)	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_ADC, opt, CR_ADC_DMIC_SEL);	\
} while (0)
/*adc mode option*/
#define CODEC_ADC_STEREO		0
#define CODEC_ADC_LEFT_ONLY		1


#define __codec_set_adc_mode(opt)	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_ADC, opt, CR_ADC_LEFT_ONLY);	\
} while (0)

/*CK_DMIC :digtail mic opt*/
#define CODEC_DMIC_CLK_OFF		0
#define CODEC_DMIC_CLK_ON		1

#define __codec_enable_dmic_clk()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_DMIC,CODEC_DMIC_CLK_ON,CR_DMIC_CLKON);	\
} while (0)

#define __codec_disable_dmic_clk()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_DMIC,CODEC_DMIC_CLK_OFF,CR_DMIC_CLKON);	\
} while (0)

/* CR_ADC,MIC1,MIC2: mic opt*/
#define CODEC_MIC_STEREO	1
#define CODEC_MIC_MONO		0

#define __codec_set_mic_stereo()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_MIC1, CODEC_MIC_STEREO, CR_MIC1_MICSTEREO);	\
} while (0)

#define __codec_set_mic_mono()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_MIC1, CODEC_MIC_MONO, CR_MIC1_MICSTEREO);	\
} while (0)

#define CODEC_MIC_DIFF		1
#define CODEC_MIC_SING		0

#define __codec_enable_mic1_diff()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_MIC1, CODEC_MIC_DIFF, CR_MIC1_MIC1DFF);	\
} while (0)

#define __codec_disable_mic1_diff()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_MIC1, CODEC_MIC_SING, CR_MIC1_MIC1DFF);	\
} while (0)

#define __codec_enable_mic2_diff()	\
do {	\
    write_inter_codec_reg_bit(CODEC_REG_CR_MIC2,CODEC_MIC_DIFF,CR_MIC2_MIC2DFF);	\
} while (0)

#define __codec_disable_mic2_diff()	\
do {	\
    write_inter_codec_reg_bit(CODEC_REG_CR_MIC2,CODEC_MIC_SING,CR_MIC2_MIC2DFF);	\
} while (0)

/*mic mux option*/
#define CODEC_MIC2_AN4              1
#define CODEC_MIC2_AN3              0
#define CODEC_MIC1_AN2              1
#define CODEC_MIC1_AN1              0

#define __codec_select_mic1_input(opt)							\
do {																\
	write_inter_codec_reg_bit(CODEC_REG_CR_MIC1,opt,CR_MIC1_SEL);	\
																	\
} while (0)


#define __codec_select_mic2_input(opt)							\
do {																\
    write_inter_codec_reg_bit(CODEC_REG_CR_MIC2,opt,CR_MIC2_SEL);	\
																	\
} while (0)

/*CR_LI  :	line in opt	*/
#define CODEC_LINEIN_DIFF			1
#define CODEC_LINEIN_SING			0

#define __codec_enable_linein1_diff()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_LI1,CODEC_LINEIN_DIFF,CR_LI1_LIN1DIF);	\
} while (0)

#define __codec_disable_linein1_diff()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_LI1,CODEC_LINEIN_SING,CR_LI1_LIN1DIF);	\
} while (0)

#define __codec_enable_linein2_diff()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_LI2,CODEC_LINEIN_DIFF,CR_LI2_LIN2DIF);	\
} while (0)

#define __codec_disable_linein2_diff()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_LI2,CODEC_LINEIN_SING,CR_LI2_LIN2DIF);	\
} while (0)

/*line in mux option*/
#define CODEC_LINEIN2_AN4			1
#define CODEC_LINEIN2_AN3			0
#define CODEC_LINEIN1_AN2			1
#define CODEC_LINEIN1_AN1			0

#define __codec_select_linein1_input(opt)	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_LI1,opt,CR_LI1_SEL);	\
} while (0)

#define __codec_select_linein2_input(opt)	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_LI2,opt,CR_LI2_SEL);	\
} while (0)

/*FCR_ADC : record filter */
#define CODEC_ADC_HPF_ENABLE        1
#define CODEC_ADC_HPF_DISABLE		0

#define __codec_enable_adc_high_pass()	\
do {	\
    write_inter_codec_reg_bit(CODEC_REG_FCR_ADC, CODEC_ADC_HPF_ENABLE,FCR_ADC_HPF); \
} while (0)

#define __codec_disable_adc_high_pass()	\
do {	\
    write_inter_codec_reg_bit(CODEC_REG_FCR_ADC, CODEC_ADC_HPF_DISABLE, FCR_ADC_HPF);	\
} while (0)
/*wind filter option*/
#define CODEC_WIND_FILTER_MODE3     3
#define CODEC_WIND_FILTER_MODE2     2
#define CODEC_WIND_FILTER_MODE1		1
#define CODEC_WIND_FILTER_DISABLE	0

#define __codec_set_wind_filter_mode(opt)	\
do {	\
    write_inter_codec_reg_mask(CODEC_REG_FCR_ADC,opt,FCR_ADC_WNF_MASK,FCR_ADC_WNF);	\
} while (0)
/*CR_AGC,DR_AGC :agc opts	FIXME*/

#define __codec_enable_agc()	\
do {	\
} while (0)

#define __codec_disable_agc()	\
do {	\
} while (0)

/*CR_CK: sysclk opts*/

/*clk option*/
#define CODEC_SYS_CLK_12M		0X00
#define CODEC_SYS_CLK_13M		0X10

#define __codec_set_crystal(opt)	\
do {	\
	write_inter_codec_reg(CODEC_REG_CR_CK, (read_inter_codec_reg(CODEC_REG_CR_CK) &	\
                        ~CR_CK_CRYSTAL_MASK) | 	\
						(opt & CR_CK_CRYSTAL_MASK));	\
} while (0)

#define __codec_set_shutdown_clk(opt)	\
do {	\
    write_inter_codec_reg_bit(CODEC_REG_CR_CK, opt ,CR_CK_SHUTDOWN_CLOCK);  \
} while (0)

/*=============================== MIXER ===============================*/

/*CR_MIX , DR_MIXER :mixer ops*/
#define __codec_mix_enable()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_MIX,1,CR_MIX_EN);	\
} while (0)
#define __codec_mix_disable()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_CR_MIX,0,CR_MIX_EN);	\
} while (0)

#define ___codec_mix_is_enable()	((read_inter_codec_reg(CODEC_REG_CR_MIX) &	\
									(1 << CR_MIX_EN))?	\
									1 : 0)

static int inline __codec_mix_read_reg(int mix_num)
{
	if (!___codec_mix_is_enable())
		write_inter_codec_reg_bit(CODEC_REG_CR_MIX,1,CR_MIX_EN);

	write_inter_codec_reg_bit(CODEC_REG_CR_MIX,0,CR_MIX_LOAD);
	write_inter_codec_reg_mask(CODEC_REG_CR_MIX,mix_num,CR_MIX_ADD_MASK,CR_MIX_ADD);

	return (read_inter_codec_reg(CODEC_REG_DR_MIX) & DR_MIX_DATA_MASK);
}

static void inline __codec_mix_write_reg(int mix_num,unsigned int data)
{
	if (!___codec_mix_is_enable())
		write_inter_codec_reg_bit(CODEC_REG_CR_MIX,1,CR_MIX_EN);

	write_inter_codec_reg_bit(CODEC_REG_CR_MIX,1,CR_MIX_LOAD);
	write_inter_codec_reg_mask(CODEC_REG_CR_MIX,mix_num,CR_MIX_ADD_MASK,CR_MIX_ADD);

	write_inter_codec_reg_mask(CODEC_REG_DR_MIX,data ,DR_MIX_DATA_MASK, DR_MIX_DATA);
}

#define CODEC_RECORD_MIX_INPUT_ONLY     0
#define CODEC_RECORD_MIX_INPUT_AND_DAC  1

#define __codec_set_rec_mix_mode(mode)	\
do {	\
	__codec_mix_write_reg(CR_MIX2,	\
						  ((__codec_mix_read_reg(CR_MIX2) & (~1)) | mode)	\
						 );	\
} while (0)


#define CODEC_PLAYBACK_MIX_DAC_ONLY     0
#define CODEC_PLAYBACK_MIX_DAC_AND_ADC  1

#define __codec_set_rep_mix_mode(mode)	\
do {	\
	__codec_mix_write_reg(CR_MIX0,	\
						  ((__codec_mix_read_reg(CR_MIX0) & (~1)) | mode)	\
						 );	\
} while (0)

#define CODEC_MIX_FUNC_NORMAL		0x00
#define CODEC_MIX_FUNC_CROSS		0x01
#define CODEC_MIX_FUNC_MIXED		0x02
#define CODEC_MIX_FUNC_NOINPUT		0x03

#define __codec_set_mix(mix_num,funcl,funcr)	\
do {	\
	int func = 0;	\
	func = (funcl << MIX_L_SEL) & MIX_L_SEL_MASK;	\
	func |= ((funcr << MIX_R_SEL) & MIX_R_SEL_MASK);	\
	if (mix_num == CR_MIX0 || mix_num == CR_MIX2)	\
		func |= (__codec_mix_read_reg(mix_num) & (1 << MIX_MODE_SEL));	\
	__codec_mix_write_reg(mix_num,func);	\
} while (0)

/*============================== GAIN =================================*/

/*GCR_MIC :gain of input*/

#define __codec_set_gm1(value)	\
do {	\
	write_inter_codec_reg(CODEC_REG_GCR_MIC1,(value&GCR_MIC1_GIM1_MASK));	\
} while (0)

#define __codec_get_gm1()	( read_inter_codec_reg(CODEC_REG_GCR_MIC1) &  GCR_MIC1_GIM1_MASK )

#define __codec_set_gm2(value)	\
do {	\
	write_inter_codec_reg(CODEC_REG_GCR_MIC2, (value & GCR_MIC2_GIM2_MASK));	\
} while (0)

#define __codec_get_gm2()	( read_inter_codec_reg(CODEC_REG_GCR_MIC2) &  GCR_MIC2_GIM2_MASK )
/*GCR_LBYS:gain of bypass*/

#define __codec_set_gil(value)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_GCR_LIBYL,value,GCR_LIBYL_GIL_MASK,GCR_LIBYL_GIL);	\
} while (0)

#define __codec_get_gil() ( read_inter_codec_reg(CODEC_REG_GCR_LIBYL) &  GCR_LIBYL_GIL_MASK )

#define __codec_set_gir(value)	\
do {	\
	write_inter_codec_reg(CODEC_REG_GCR_LIBYR, (value & GCR_LIBYR_GIR_MASK));	\
} while (0)

#define __codec_get_gir() ( read_inter_codec_reg(CODEC_REG_GCR_LIBYR) &  GCR_LIBYR_GIR_MASK )


#define CODEC_BYPASS_SYNC_GAIN_ENABLR	1
#define CODEC_BYPASS_SYNC_GAIN_DISABLE	0

#define __codec_enable_input_bypass_sync_gain()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_GCR_LIBYL,	\
							  CODEC_BYPASS_SYNC_GAIN_ENABLR,	\
							  GCR_LIBYL_LRGI);	\
}

#define __codec_disable_input_bypass_sync_gain()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_GCR_LIBYL,	\
							  CODEC_BYPASS_SYNC_GAIN_DISABLR,	\
							  GCR_LIBYL_LRGI);	\
}

/*GCR_HP :gain of hp*/

#define __codec_set_gol(value)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_GCR_HPL, value ,	\
							   GCR_HPL_GOL_MASK,GCR_HPL_GOL);	\
} while (0)

#define __codec_get_gol() ( read_inter_codec_reg(CODEC_REG_GCR_HPL) &  GCR_HPL_GOL_MASK )

#define __codec_set_gor(value)	\
do{	\
	write_inter_codec_reg(CODEC_REG_GCR_HPR, (value & GCR_HPR_GOR_MASK));	\
} while (0)

#define __codec_get_gor() ( read_inter_codec_reg(CODEC_REG_GCR_HPR) &  GCR_HPR_GOR_MASK )

#define CODEC_HP_SYNC_GAIN_ENABLE	1
#define CODEC_HP_SYNC_GAIN_DISABLE	0

#define __codec_enable_hp_sync_gain()   \
do {	\
    write_inter_codec_reg_bit(CODEC_REG_GCR_HPL,CODEC_HP_SYNC_GAIN_ENABLE,GCR_HPL_LRGO);	\
} while (0)

#define __codec_disable_hp_sync_gain()   \
do {	\
    write_inter_codec_reg_bit(CODEC_REG_GCR_HPL,CODEC_HP_SYNC_GAIN_DISABLE,GCR_HPL_LRGO);	\
} while (0)

/*GCR_ADC :gain of adc*/

#define __codec_set_gidl(value)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_GCR_ADCL, value,	\
							  GCR_ADCL_GIDL_MASK,GCR_ADCL_GIDL);	\
} while (0)

#define __codec_get_gidl() ( read_inter_codec_reg(CODEC_REG_GCR_ADCL) &  GCR_ADCL_GIDL_MASK)

#define __codec_set_gidr(value)	\
do {	\
	write_inter_codec_reg(CODEC_REG_GCR_ADCR, (value & GCR_ADCR_GIDR_MASK));	\
} while (0)

#define __codec_get_gidr() ( read_inter_codec_reg(CODEC_REG_GCR_ADCR) &  GCR_ADCR_GIDR_MASK )

#define CODEC_ADC_SYNC_GAIN_ENABLE		1
#define CODEC_ADC_SYNC_GAIN_DISABLE		0

#define __codec_enable_adc_sync_gain()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_GCR_ADCL,CODEC_ADC_SYNC_GAIN_ENABLE,GCR_ADCL_LRGID);	\
} while (0)

#define __codec_disable_adc_sync_gain()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_GCR_ADCL,CODEC_ADC_SYNC_GAIN_DISABLE,GCR_ADCL_LRGID);	\
} while (0)

/*GCR_DAC :gain of dac*/

#define __codec_set_godl(value)					\
do {									\
	write_inter_codec_reg_mask(CODEC_REG_GCR_DACL, value,           \
                             GCR_DACL_GODL_MASK,GCR_DACL_GODL);	        \
									\
} while (0)

#define __codec_get_godl() (read_inter_codec_reg(CODEC_REG_GCR_DACL) &  GCR_DACL_GODL_MASK)

#define __codec_set_godr(value)							\
do {		                							\
	write_inter_codec_reg(CODEC_REG_GCR_DACR, (value & GCR_DACR_GODR_MASK));	\
                									\
} while (0)
#define __codec_get_godr() (read_inter_codec_reg(CODEC_REG_GCR_DACR) &  GCR_DACR_GODR_MASK)

#define DAC_LR_SYNC_GAIN        1
#define DAC_LR_ASYNC_GAIN       0

#define __codec_set_dac_rlsync_gain(opt)                                \
do {                                                                            \
        write_inter_codec_reg_bit(CODEC_REG_GCR_DACL,opt GCR_DACL_RLGOD);       \
                                                                                \
} while (0)

/*GCR_MIXADCx : mixin gain*/
#define __codec_set_gimixl(value)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_GCR_MIXADCL, value ,	\
							   GCR_MIXADC_GIMIXL_MASK, GCR_MIXADC_GIMIXL);	\
} while (0)

#define __codec_get_gimixl() (read_inter_codec_reg(CODEC_REG_GCR_MIXADCL) & GCR_MIXADC_GIMIXL_MASK)

#define __codec_set_gimixr(value)	\
do {	\
	write_inter_codec_reg(CODEC_REG_GCR_MIXADCR, (value & GCR_MIXADC_GIMIXR_MASK));	\
} while (0)

#define __codec_get_gimixr() (read_inter_codec_reg(CODEC_REG_GCR_MIXADCR) & GCR_MIXADC_GIMIXR_MASK)

#define CODEC_MIX_SYNC_GAIN_ENABLE		1
#define CODEC_MIX_SYNC_GAIN_DISABLE		0

#define __test_mixin_is_sync_gain()	(read_inter_codec_reg(CODEC_REG_GCR_MIXADCL) & BIT(GCR_MIXADC_LRGIMIX))

#define __codec_enable_mixin_sync_gain()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_GCR_MIXADCL,CODEC_MIX_SYNC_GAIN_ENABLE,	\
							  GCR_MIXADC_LRGIMIX);	\
} while (0)

#define __codec_disable_mixin_sync_gain()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_GCR_MIXADCL,CODEC_MIX_SYNC_GAIN_DISABLE,	\
							  GCR_MIXADC_LRGIMIX);	\
} while (0)

/*GCR_MIXDACx : mixout gain*/

#define __codec_set_gomixl(value)	\
do {	\
	write_inter_codec_reg_mask(CODEC_REG_GCR_MIXDACL, value ,	\
							   GCR_MIXDAC_GOMIXL_MASK,GCR_MIXDAC_GOMIXL);	\
} while (0)

#define __codec_get_gomixl() (read_inter_codec_reg(CODEC_REG_GCR_MIXDACL) & GCR_MIXDAC_GOMIXL_MASK)

#define __codec_set_gomixr(value)	\
do {	\
	write_inter_codec_reg(CODEC_REG_GCR_MIXDACR, (value & GCR_MIXDAC_GOMIXR_MASK));	\
} while (0)

#define __codec_get_gomixr() (read_inter_codec_reg(CODEC_REG_GCR_MIXDACR) & GCR_MIXDAC_GOMIXR_MASK)

#define __test_mixout_is_sync_gain()	(read_inter_codec_reg(CODEC_REG_GCR_MIXDACL) & BIT(GCR_MIXDAC_LRGOMIX))

#define __codec_enable_mixout_sync_gain()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_GCR_MIXDACL,CODEC_MIX_SYNC_GAIN_ENABLE,	\
							  GCR_MIXDAC_LRGOMIX);	\
} while (0)

#define __codec_disable_mixout_sync_gain()	\
do {	\
	write_inter_codec_reg_bit(CODEC_REG_GCR_MIXDACL,CODEC_MIX_SYNC_GAIN_DISABLE,	\
							  GCR_MIXDAC_LRGOMIX);	\
} while (0)

#endif /*__JZ4780_CODEC_H__*/
