/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __JZ47XX_ICDC_H__
#define __JZ47XX_ICDC_H__

/* JZ internal register space */
enum {
	JZ_ICDC_SR		= 0x00,
	JZ_ICDC_SR2		= 0x01,
	
	JZ_ICDC_MR		= 0x07,
	JZ_ICDC_AICR_DAC	= 0x08,
	JZ_ICDC_AICR_ADC	= 0x09,
	JZ_ICDC_MISSING_REG1,
	JZ_ICDC_CR_LO		= 0x0b,
	JZ_ICDC_MISSING_REG2,
	JZ_ICDC_CR_HP		= 0x0d,
	JZ_ICDC_MISSING_REG3,
	JZ_ICDC_MISSING_REG4,
	JZ_ICDC_CR_DMIC		= 0x10,
	JZ_ICDC_CR_MIC1		= 0x11,
	JZ_ICDC_CR_MIC2		= 0x12,
	JZ_ICDC_CR_LI1		= 0x13,
	JZ_ICDC_CR_LI2		= 0x14,
	JZ_ICDC_MISSING_REG5,
	JZ_ICDC_MISSING_REG6,
	JZ_ICDC_CR_DAC		= 0x17,
	JZ_ICDC_CR_ADC		= 0x18,
	JZ_ICDC_CR_MIX		= 0x19,
	JZ_ICDC_DR_MIX		= 0x1a,
	JZ_ICDC_CR_VIC		= 0x1b,
	JZ_ICDC_CR_CK		= 0x1c,
	JZ_ICDC_FCR_DAC		= 0x1d,
	JZ_ICDC_MISSING_REG7,
	JZ_ICDC_MISSING_REG8,
	JZ_ICDC_FCR_ADC		= 0x20,
	JZ_ICDC_CR_TIMER_MSB	= 0x21,
	JZ_ICDC_CR_TIMER_LSB	= 0x22,
	JZ_ICDC_ICR		= 0x23,
	JZ_ICDC_IMR		= 0x24,
	JZ_ICDC_IFR		= 0x25,
	JZ_ICDC_IMR2		= 0x26,
	JZ_ICDC_IFR2		= 0x27,
	JZ_ICDC_GCR_HPL		= 0x28,
	JZ_ICDC_GCR_HPR		= 0x29,
	JZ_ICDC_GCR_LIBYL	= 0x2a,
	JZ_ICDC_GCR_LIBYR	= 0x2b,
	JZ_ICDC_GCR_DACL	= 0x2c,
	JZ_ICDC_GCR_DACR	= 0x2d,
	JZ_ICDC_GCR_MIC1	= 0x2e,
	JZ_ICDC_GCR_MIC2	= 0x2f,
	JZ_ICDC_GCR_ADCL	= 0x30,
	JZ_ICDC_GCR_ADCR	= 0x31,
	JZ_ICDC_MISSING_REG9,
	JZ_ICDC_MISSING_REG10,
	JZ_ICDC_GCR_MIXDACL	= 0x34,
	JZ_ICDC_GCR_MIXDACR	= 0x35,
	JZ_ICDC_GCR_MIXADCL	= 0x36,
	JZ_ICDC_GCR_MIXADCR	= 0x37,
	JZ_ICDC_MISSING_REG11,
	JZ_ICDC_MISSING_REG12,
	JZ_ICDC_CR_ADC_AGC	= 0x3a,
	JZ_ICDC_DR_ADC_AGC	= 0x3b,

	JZ_ICDC_MAX_REGNUM,

	/* virtual registers */
	JZ_ICDC_LHPSEL  = 0x3c,
	JZ_ICDC_RHPSEL  = 0x3d,

	JZ_ICDC_LLOSEL = 0x3e,
	JZ_ICDC_RLOSEL = 0x3f,

	JZ_ICDC_LINSEL	= 0x40,
	JZ_ICDC_RINSEL	= 0x41,

	JZ_ICDC_MAX_NUM
};

#define ICDC_REG_IS_MISSING(r) 	( (((r) >JZ_ICDC_SR2) && ((r) <JZ_ICDC_MR)) || ( (r) == JZ_ICDC_MISSING_REG1) || ( (r) == JZ_ICDC_MISSING_REG2) || ( (r) == JZ_ICDC_MISSING_REG3)|| ( (r) == JZ_ICDC_MISSING_REG4)|| ( (r) == JZ_ICDC_MISSING_REG5)|| ( (r) == JZ_ICDC_MISSING_REG6)|| ( (r) == JZ_ICDC_MISSING_REG7)|| ( (r) == JZ_ICDC_MISSING_REG8)|| ( (r) == JZ_ICDC_MISSING_REG9)|| ( (r) == JZ_ICDC_MISSING_REG10)|| ( (r) == JZ_ICDC_MISSING_REG11)|| ( (r) == JZ_ICDC_MISSING_REG12))

#define DLC_IFR_LOCK            (1 << 7)
#define DLC_IFR_SCLR            (1 << 6)
#define DLC_IFR_JACK            (1 << 5)
#define DLC_IFR_ADC_MUTE        (1 << 2)
#define DLC_IFR_DAC_MODE        (1 << 1)
#define DLC_IFR_DAC_MUTE        (1 << 0)

#define IFR_ALL_FLAGS   (DLC_IFR_SCLR | DLC_IFR_JACK | DLC_IFR_LOCK | DLC_IFR_ADC_MUTE | DLC_IFR_DAC_MUTE |  DLC_IFR_DAC_MODE)

#define DLC_IMR_LOCK            (1 << 7)
#define DLC_IMR_SCLR            (1 << 6)
#define DLC_IMR_JACK            (1 << 5)
#define DLC_IMR_ADC_MUTE        (1 << 2)
#define DLC_IMR_DAC_MODE        (1 << 1)
#define DLC_IMR_DAC_MUTE        (1 << 0)

#define ICR_ALL_MASK                                                    \
	(DLC_IMR_LOCK | DLC_IMR_SCLR | DLC_IMR_JACK | DLC_IMR_ADC_MUTE | DLC_IMR_DAC_MODE | DLC_IMR_DAC_MUTE)

#define ICR_COMMON_MASK (ICR_ALL_MASK & (~(DLC_IMR_SCLR)))

#define DLV_DEBUG_SEM(x,y...) //printk(x,##y);


#define DLV_LOCK()                                           \
	do{                                                  \
		if(g_dlv_sem)                                \
		if(down_interruptible(g_dlv_sem))            \
		return;		                             \
		DLV_DEBUG_SEM("dlvsemlock lock\n");          \
	}while(0)

#define DLV_UNLOCK()                                         \
	do{                                                  \
		if(g_dlv_sem)                                \
			up(g_dlv_sem);                               \
		DLV_DEBUG_SEM("dlvsemlock unlock\n");        \
	}while(0)

#define DLV_LOCKINIT()                                      \
	do{                                                 \
		if(g_dlv_sem == NULL)                       \
		g_dlv_sem = (struct semaphore *)vmalloc(sizeof(struct semaphore)); \
		if(g_dlv_sem)                               \
			sema_init(g_dlv_sem, 0);               \
		DLV_DEBUG_SEM("dlvsemlock init\n");         \
	}while(0)

#define DLV_LOCKDEINIT()                                     \
	do{                                                  \
		if(g_dlv_sem)                                \
			vfree(g_dlv_sem);                            \
		g_dlv_sem = NULL;                            \
		DLV_DEBUG_SEM("dlvsemlock deinit\n");        \
	}while(0)


#endif	/* __JZ47XX_ICDC_H__ */
