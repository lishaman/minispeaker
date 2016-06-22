 /*
  2  * adv7180.c Analog Devices ADV7180 video decoder driver
  3  * Copyright (c) 2009 Intel Corporation
  */
 /*
  5  * This program is free software; you can redistribute it and/or modify
  6  * it under the terms of the GNU General Public License version 2 as
  7  * published by the Free Software Foundation.
  8  *
  9  * This program is distributed in the hope that it will be useful,
 10  * but WITHOUT ANY WARRANTY; without even the implied warranty of
 11  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 12  * GNU General Public License for more details.
 13  *
 14  * You should have received a copy of the GNU General Public License
 15  * along with this program; if not, write to the Free Software
 16  * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 17  */

#define PAL  1
#define NTSC 2

#define DRIVER_NAME "adv7180"
#define ADV7180_INPUT_CONTROL_REG                       0x00
#define ADV7180_INPUT_CONTROL_AD_PAL_BG_NTSC_J_SECAM    0x00
#define ADV7180_INPUT_CONTROL_AD_PAL_BG_NTSC_J_SECAM_PED 0x10
#define ADV7180_INPUT_CONTROL_AD_PAL_N_NTSC_J_SECAM     0x20
#define ADV7180_INPUT_CONTROL_AD_PAL_N_NTSC_M_SECAM     0x30
#define ADV7180_INPUT_CONTROL_NTSC_J                    0x40
#define ADV7180_INPUT_CONTROL_NTSC_M                    0x50
#define ADV7180_INPUT_CONTROL_PAL60                     0x60
#define ADV7180_INPUT_CONTROL_NTSC_443                  0x70
#define ADV7180_INPUT_CONTROL_PAL_BG                    0x80
#define ADV7180_INPUT_CONTROL_PAL_N                     0x90
#define ADV7180_INPUT_CONTROL_PAL_M                     0xa0
#define ADV7180_INPUT_CONTROL_PAL_M_PED                 0xb0
#define ADV7180_INPUT_CONTROL_PAL_COMB_N                0xc0
#define ADV7180_INPUT_CONTROL_PAL_COMB_N_PED            0xd0
#define ADV7180_INPUT_CONTROL_PAL_SECAM                 0xe0
#define ADV7180_INPUT_CONTROL_PAL_SECAM_PED             0xf0
#define ADV7180_EXTENDED_OUTPUT_CONTROL_REG             0x04
#define ADV7180_EXTENDED_OUTPUT_CONTROL_NTSCDIS         0xC5
#define ADV7180_AUTODETECT_ENABLE_REG                   0x07
#define ADV7180_AUTODETECT_DEFAULT                      0x7f
#define ADV7180_ADI_CTRL_REG                            0x0e
#define ADV7180_ADI_CTRL_IRQ_SPACE                      0x20
#define ADV7180_STATUS1_REG                             0x10
#define ADV7180_STATUS1_IN_LOCK         0x01
#define ADV7180_STATUS1_AUTOD_MASK      0x70
#define ADV7180_STATUS1_AUTOD_NTSM_M_J  0x00
#define ADV7180_STATUS1_AUTOD_NTSC_4_43 0x10
#define ADV7180_STATUS1_AUTOD_PAL_M     0x20
#define ADV7180_STATUS1_AUTOD_PAL_60    0x30
#define ADV7180_STATUS1_AUTOD_PAL_B_G   0x40
#define ADV7180_STATUS1_AUTOD_SECAM     0x50
#define ADV7180_STATUS1_AUTOD_PAL_COMB  0x60
#define ADV7180_STATUS1_AUTOD_SECAM_525 0x70
#define ADV7180_IDENT_REG 0x11
#define ADV7180_ID_7180 0x18
#define ADV7180_ICONF1_ADI              0x40
#define ADV7180_ICONF1_ACTIVE_LOW       0x01
#define ADV7180_ICONF1_PSYNC_ONLY       0x10
#define ADV7180_ICONF1_ACTIVE_TO_CLR    0xC0
#define ADV7180_IRQ1_LOCK       0x01
#define ADV7180_IRQ1_UNLOCK     0x02
#define ADV7180_ISR1_ADI        0x42
#define ADV7180_ICR1_ADI        0x43
#define ADV7180_IMR1_ADI        0x44
#define ADV7180_IMR2_ADI        0x48
#define ADV7180_IRQ3_AD_CHANGE  0x08
#define ADV7180_ISR3_ADI        0x4A
#define ADV7180_ICR3_ADI        0x4B
#define ADV7180_IMR3_ADI        0x4C
#define ADV7180_IMR4_ADI        0x50


#define ADV7180_FILTER_CTL   0x17
#define ADV7180_FILED_CTL    0x31
#define ADV7180_MANUAL_WINDOW_CTL 0x3D
#define ADV7180_BLM_OPT     0X3E
#define ADV7180_BGB_OPT     0x3F
#define ADV7180_ADC_CFG     0x55
#define ADV7180_USR_SPA     0x0E







