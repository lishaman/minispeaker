/*
 * act8600-private.h - Head file for PMU ACT8600.
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Author: Large Dipper <ykli@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_MFD_AXP173_PRIV_H
#define __LINUX_MFD_AXP173_PRIV_H

#include <jz_notifier.h>

#define   POWER_STATUS				(0x00)
#define   POWER_MODE_CHGSTATUS			(0x01)
#define   POWER_OTG_STATUS			(0x02)
#define   POWER_DATA_BUFFER1			(0x06)
#define   POWER_DATA_BUFFER2			(0x07)
#define   POWER_DATA_BUFFER3			(0x08)
#define   POWER_DATA_BUFFER4			(0x09)
#define   POWER_VERSION				(0x0C)
#define   POWER_LDO3_DC2_CTL			(0x10)
#define   POWER_LDO24_DC13_CTL			(0x12)
#define   POWER_DC2OUT_VOL			(0x23)
#define   POWER_LDO3_DC2_VRC			(0x25)
#define   POWER_DC1OUT_VOL			(0x26)
#define   POWER_DC3OUT_VOL			(0x27)
#define   POWER_LDO24OUT_VOL			(0x28)
#define   POWER_LDO3OUT_VOL			(0x29)
#define   POWER_IPS_SET				(0x30)
#define   POWER_VOFF_SET			(0x31)
#define   POWER_OFF_CTL				(0x32)
#define   POWER_CHARGE1				(0x33)
#define   POWER_CHARGE2				(0x34)
#define   POWER_BACKUP_CHG			(0x35)
#define   POWER_PEK_SET				(0x36)
#define   POWER_DCDC_FREQSET			(0x37)
#define   POWER_VLTF_CHGSET			(0x38)
#define   POWER_VHTF_CHGSET			(0x39)
#define   POWER_APS_WARNING1			(0x3A)
#define   POWER_APS_WARNING2			(0x3B)
#define   POWER_VLTF_DISCHGSET			(0x3C)
#define   POWER_VHTF_DISCHGSET			(0x3D)
#define   POWER_DCDC_MODESET			(0x80)
#define   POWER_VOUT_MONITOR			(0x81)
#define   POWER_ADC_EN1				(0x82)
#define   POWER_ADC_EN2				(0x83)
#define   POWER_ADC_SPEED			(0x84)
#define   POWER_ADC_INPUTRANGE			(0x85)
#define   POWER_TIMER_CTL			(0x8A)
#define   POWER_VBUS_DET_SRP			(0x8B)
#define   POWER_HOTOVER_CTL			(0x8F)
#define   POWER_GPIO0_CTL			(0x90)
#define   POWER_GPIO0_VOL			(0x91)
#define   POWER_GPIO1_CTL			(0x92)
#define   POWER_GPIO2_CTL			(0x93)
#define   POWER_GPIO_SIGNAL			(0x94)
#define   POWER_SENSE_CTL			(0x95)
#define   POWER_SENSE_SIGNAL			(0x96)
#define   POWER_GPIO20_PDCTL			(0x97)
#define   POWER_PWM1_FREQ			(0x98)
#define   POWER_PWM1_DUTYDE			(0x99)
#define   POWER_PWM1_DUTY			(0x9A)
#define   POWER_PWM2_FREQ			(0x9B)
#define   POWER_PWM2_DUTYDE			(0x9C)
#define   POWER_PWM2_DUTY			(0x9D)
#define   POWER_RSTO_CTL			(0x9E)
#define   POWER_GPIO67_CTL			(0x9F)
#define   POWER_INTEN1				(0x40)
#define   POWER_INTEN2				(0x41)
#define   POWER_INTEN3				(0x42)
#define   POWER_INTEN4				(0x43)
#define   POWER_INTEN5                          (0x4a)
#define   POWER_INTSTS1				(0x44)
#define   POWER_INTSTS2				(0x45)
#define   POWER_INTSTS3				(0x46)
#define   POWER_INTSTS4				(0x47)
#define   POWER_INTSTS5                         (0x4d)
#define   POWER_COULOMB_CTL			(0xB8)

//adc data register
#define   POWER_ACIN_VOL_H8			(0x56)
#define   POWER_ACIN_VOL_L4			(0x57)
#define   POWER_ACIN_CUR_H8			(0x58)
#define   POWER_ACIN_CUR_L4			(0x59)
#define   POWER_VBUS_VOL_H8			(0x5A)
#define   POWER_VBUS_VOL_L4			(0x5B)
#define   POWER_VBUS_CUR_H8			(0x5C)
#define   POWER_VBUS_CUR_L4			(0x5D)
#define   POWER_INT_TEMP_H8			(0x5E)
#define   POWER_INT_TEMP_L4			(0x5F)
#define   POWER_TS_VOL_H8			(0x62)
#define   POWER_TS_VOL_L4			(0x63)
#define   POWER_GPIO0_VOL_H8			(0x64)
#define   POWER_GPIO0_VOL_L4			(0x65)
#define   POWER_GPIO1_VOL_H8			(0x66)
#define   POWER_GPIO1_VOL_L4			(0x67)
#define   POWER_GPIO2_VOL_H8			(0x68)
#define   POWER_GPIO2_VOL_L4			(0x69)
#define   POWER_BATSENSE_VOL_H8			(0x6A)
#define   POWER_BATSENSE_VOL_L4			(0x6B)

#define   POWER_BAT_AVERVOL_H8                  (0x78)
#define   POWER_BAT_AVERVOL_L4                  (0x79)
#define   POWER_BAT_AVERCHGCUR_H8               (0x7A)
#define   POWER_BAT_AVERCHGCUR_L5               (0x7B)
#define   POWER_BAT_AVERDISCHGCUR_H8            (0x7C)
#define   POWER_BAT_AVERDISCHGCUR_L5            (0x7D)
#define   POWER_APS_AVERVOL_H8                  (0x7E)
#define   POWER_APS_AVERVOL_L4                  (0x7F)
#define   POWER_BAT_CHGCOULOMB3                 (0xB0)
#define   POWER_BAT_CHGCOULOMB2                 (0xB1)
#define   POWER_BAT_CHGCOULOMB1                 (0xB2)
#define   POWER_BAT_CHGCOULOMB0                 (0xB3)
#define   POWER_BAT_DISCHGCOULOMB3              (0xB4)
#define   POWER_BAT_DISCHGCOULOMB2              (0xB5)
#define   POWER_BAT_DISCHGCOULOMB1              (0xB6)
#define   POWER_BAT_DISCHGCOULOMB0              (0xB7)
#define   POWER_BAT_POWERH8			(0x70)
#define   POWER_BAT_POWERM8			(0x71)
#define   POWER_BAT_POWERL8			(0x72)

#define POWER_ON_OFF_REG			(0x12)
#define EXTEN					(1<<6)
#define DC2_ON					(1<<4)
#define LDO3_ON					(1<<3)
#define LDO2_ON					(1<<2)
#define LDO4_ON					(1<<1)
#define DC1_ON					(1<<0)
#define POWER_OFF				(1<<7)



#define DC2_REG					(0x23)
#define DC1_REG					(0X26)
#define LDO4_REG				(0X27)
#define LDO2_REG				(0X28)
#define LDO3_REG				(0X28)
#define VBUS_IPSOUT_REG				(0X30)
#define VOFF_REG				(0x31)
#define POWER_BATDETECT_CHGLED_REG		(0X32)

#define VBUS_VHOLD_EN                           (1<<6)
#define VBUS_VHOLD_VAL(x)                       (x<<3)
#define VBUS_CL_EN                              (1<<1)
#define BATTERY_DETE                            (1<<6)
#define BAT_ADC_EN_V                            (1<<7)
#define BAT_ADC_EN_C                            (1<<6)
#define AC_ADC_EN_V                             (1<<5)
#define AC_ADC_EN_C                             (1<<4)
#define USB_ADC_EN_V                            (1<<3)
#define USB_ADC_EN_C                            (1<<2)
#define APS_ADC_EN_V                            (1<<1)
#define ADC_SPEED_BIT1                          (1<<7)
#define ADC_SPEED_BIT2                          (1<<6)
#define INT1_AC_IN                              (1<<6)
#define INT1_AC_OUT                             (1<<5)
#define INT1_USB_IN                             (1<<3)
#define INT1_USB_OUT                            (1<<2)
#define INT1_VBUS_VHOLD                         (1<<1)
#define INT2_BATTERY_IN                         (1<<7)
#define INT2_BATTERY_OUT                        (1<<6)
#define INT2_BATTERY_CHARGING                   (1<<3)
#define INT2_BATTERY_CHARGED                    (1<<2)
#define INT2_BATTERY_HIGH_TEMP			(1<<1)
#define INT2_BATTERY_LOW_TEMP			(1<<0)
#define INT3_SHORTPRESS_PEK			(1<<1)
#define INT3_LONGPRESS_PEK			(1<<0)
#define INT3_LOWVOLTAGE_WARNING			(1<<4)
#define INT4_LOWVOLTAGE_WARNING1		(1<<1)
#define INT4_LOWVOLTAGE_WARNING2		(1<<0)
#define INT5_TIMER                              (1<<7)

#define AC_IN                                   (1<<7)
#define USB_IN                                  (1<<5)
#define BATTERY_IN                              (1<<5)
#define CHARGED_STATUS                          (1<<6)
#define AC_AVAILABLE                            (1<<6)
#define USB_AVAILABLE                           (1<<4)
#define CHARGE_ENABLE                           (1<<7)
#define CHARGE_DISABLE                          (~(1<<7))
#define VBUS_CHARGE_DISABLE                     (~(1<<0))

#define VSEL_MASK   0
#define ADC_TS_OUT(x)				(x<<4)
#define ADC_TS_OUT_TYPE(x)			(x<<0)
#define TS_ADC_EN				(1<<0)

struct axp173 {
	struct device 		*dev;
	struct i2c_client 	*client;
	struct jz_notifier      axp173_notifier;
};

struct charger_board_info {
	short gpio;
	short enable_level;
};

extern int axp173_read_reg(struct i2c_client *client, unsigned char reg, unsigned char *val);
extern int axp173_write_reg(struct i2c_client *client, unsigned char reg, unsigned char val);

#endif /* __LINUX_MFD_AXP173_PRIV_H */
