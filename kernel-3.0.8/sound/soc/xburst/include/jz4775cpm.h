/*
 * jz4775cpm.h
 * JZ4775 CPM register definition
 * Copyright (C) 2010 Ingenic Semiconductor Co., Ltd.
 *
 * Author: whxu@ingenic.cn
 */

#ifndef __JZ4775CPM_H__
#define __JZ4775CPM_H__

/*
 * Clock reset and power controller module(CPM) address definition
 */
#define	CPM_BASE	0xb0000000

#define JZ_EXTAL        24000000
#define JZ_EXTAL2       32768

/*
 * CPM registers offset address definition
 */
#define CPM_CPCCR_OFFSET	(0x00)  /* rw, 32, 0x95000000 */
#define CPM_CPCSR_OFFSET	(0xd4)  /* rw, 32, 0x00000000 */
#define CPM_DDCDR_OFFSET	(0x2c)  /* rw, 32, 0x40000000 */
#define CPM_VPUCDR_OFFSET	(0x30)  /* rw, 32, 0x00000000 */
#define CPM_I2SCDR_OFFSET	(0x60)  /* rw, 32, 0x00000000 */
#define CPM_LPCDR_OFFSET	(0x64)  /* rw, 32, 0x00000000 */
#define CPM_MSC0CDR_OFFSET	(0x68)  /* rw, 32, 0x40000000 */
#define CPM_MSC1CDR_OFFSET	(0xa4)  /* rw, 32, 0x00000000 */
#define CPM_MSC2CDR_OFFSET	(0xa8)  /* rw, 32, 0x00000000 */
#define CPM_USBCDR_OFFSET	(0x50)  /* rw, 32, 0x00000000 */
#define CPM_UHCCDR_OFFSET	(0x6c)  /* rw, 32, 0x00000000 */
#define CPM_SSICDR_OFFSET	(0x74)  /* rw, 32, 0x00000000 */
#define CPM_CIMCDR_OFFSET	(0x7c)  /* rw, 32, 0x00000000 */
#define CPM_CIM1CDR_OFFSET	(0x80)  /* rw, 32, 0x00000000 */
#define CPM_PCMCDR_OFFSET	(0x84)  /* rw, 32, 0x00000000 */
#define CPM_BCHCDR_OFFSET	(0xac)  /* rw, 32, 0x40000000 */
#define CPM_MPHYC_OFFSET	(0xe0)  /* rw, 32, 0x40000000 */
#define CPM_INTR_OFFSET		(0xb0)  /* rw, 32, 0x00000000 */
#define CPM_INTRE_OFFSET	(0xb4)  /* rw, 32, 0x00000000 */
#define CPM_CPSPR_OFFSET	(0x34)  /* rw, 32, 0x???????? */
#define CPM_CPSPPR_OFFSET	(0x38)  /* rw, 32, 0x0000a5a5 */
#define CPM_USBPCR_OFFSET	(0x3c)  /* rw, 32, 0x429919b8 */
#define CPM_USBRDT_OFFSET	(0x40)  /* rw, 32, 0x02000096 */
#define CPM_USBVBFIL_OFFSET	(0x44)  /* rw, 32, 0x00ff0080 */
#define CPM_USBPCR1_OFFSET	(0x48)  /* rw, 32, 0x00000004 */
#define CPM_CPAPCR_OFFSET	(0x10)  /* rw, 32, 0x00000000 */
#define CPM_CPMPCR_OFFSET	(0x14)  /* rw, 32, 0x00000000 */
#define CPM_LCR_OFFSET		(0x04)  /* rw, 32, 0x000000f8 */
#define CPM_SPCR0_OFFSET	(0xb8)  /* rw, 32, 0x00000000 */
#define CPM_SPCR1_OFFSET	(0xbc)  /* rw, 32, 0x00000000 */
#define CPM_PSWC0ST_OFFSET	(0x90)  /* rw, 32, 0x00000000 */
#define CPM_PSWC1ST_OFFSET	(0x94)  /* rw, 32, 0x00000000 */
#define CPM_PSWC2ST_OFFSET	(0x98)  /* rw, 32, 0x00000000 */
#define CPM_PSWC3ST_OFFSET	(0x9c)  /* rw, 32, 0x00000000 */
#define CPM_CLKGR0_OFFSET	(0x20)  /* rw, 32, 0xffffffe0 */
#define CPM_SRBC_OFFSET		(0xc4)  /* rw, 32, 0x00000000 */
#define CPM_SLBC_OFFSET		(0xc8)  /* rw, 32, 0x00000000 */
#define CPM_SLPC_OFFSET		(0xcc)  /* rw, 32, 0x???????? */
#define CPM_OPCR_OFFSET		(0x24)  /* rw, 32, 0x000015c0 */
#define CPM_RSR_OFFSET		(0x08)  /* rw, 32, 0x???????? */

/*
 * CPM registers address definition
 */
#define CPM_CPCCR	(CPM_BASE + CPM_CPCCR_OFFSET)
#define CPM_CPCSR	(CPM_BASE + CPM_CPCSR_OFFSET)
#define CPM_DDCDR	(CPM_BASE + CPM_DDCDR_OFFSET)
#define CPM_VPUCDR	(CPM_BASE + CPM_VPUCDR_OFFSET)
#define CPM_I2SCDR	(CPM_BASE + CPM_I2SCDR_OFFSET)
#define CPM_LPCDR0	(CPM_BASE + CPM_LPCDR_OFFSET)
#define CPM_MSC0CDR	(CPM_BASE + CPM_MSC0CDR_OFFSET)
#define CPM_MSC1CDR	(CPM_BASE + CPM_MSC1CDR_OFFSET)
#define CPM_MSC2CDR	(CPM_BASE + CPM_MSC2CDR_OFFSET)
#define CPM_USBCDR	(CPM_BASE + CPM_USBCDR_OFFSET)
#define CPM_UHCCDR	(CPM_BASE + CPM_UHCCDR_OFFSET)
#define CPM_SSICDR	(CPM_BASE + CPM_SSICDR_OFFSET)
#define CPM_CIMCDR	(CPM_BASE + CPM_CIMCDR_OFFSET)
#define CPM_CIM1CDR	(CPM_BASE + CPM_CIM1CDR_OFFSET)
#define CPM_PCMCDR	(CPM_BASE + CPM_PCMCDR_OFFSET)
#define CPM_BCHCDR	(CPM_BASE + CPM_BCHCDR_OFFSET)
#define CPM_MPHYC	(CPM_BASE + CPM_MPHYC_OFFSET)
#define CPM_INTRCDR	(CPM_BASE + CPM_INTRCDR_OFFSET)
#define CPM_CPSPR	(CPM_BASE + CPM_CPSPR_OFFSET)
#define CPM_CPSPPR	(CPM_BASE + CPM_CPSPPR_OFFSET)
#define CPM_USBPCR	(CPM_BASE + CPM_USBPCR_OFFSET)
#define CPM_USBRDT	(CPM_BASE + CPM_USBRDT_OFFSET)
#define CPM_USBVBFIL	(CPM_BASE + CPM_USBVBFIL_OFFSET)
#define CPM_USBPCR1	(CPM_BASE + CPM_USBPCR1_OFFSET)
#define CPM_CPAPCR	(CPM_BASE + CPM_CPAPCR_OFFSET)
#define CPM_CPMPCR	(CPM_BASE + CPM_CPMPCR_OFFSET)
#define CPM_LCR		(CPM_BASE + CPM_LCR_OFFSET)
#define CPM_SPCR0	(CPM_BASE + CPM_SPCR0_OFFSET)
#define CPM_SPCR1	(CPM_BASE + CPM_SPCR1_OFFSET)
#define CPM_PSWC0ST	(CPM_BASE + CPM_PSWC0ST_OFFSET)
#define CPM_PSWC1ST	(CPM_BASE + CPM_PSWC1ST_OFFSET)
#define CPM_PSWC2ST	(CPM_BASE + CPM_PSWC2ST_OFFSET)
#define CPM_PSWC3ST	(CPM_BASE + CPM_PSWC3ST_OFFSET)
#define CPM_CLKGR0	(CPM_BASE + CPM_CLKGR0_OFFSET)
#define CPM_SRBC	(CPM_BASE + CPM_SRBC_OFFSET)
#define CPM_SLBC	(CPM_BASE + CPM_SLBC_OFFSET)
#define CPM_SLPC	(CPM_BASE + CPM_SLPC_OFFSET)
#define CPM_OPCR	(CPM_BASE + CPM_OPCR_OFFSET)
#define CPM_RSR		(CPM_BASE + CPM_RSR_OFFSET)


/*
 * CPM registers common define
 */

/* Clock control register(CPCCR) */
#define CPCCR_SEL_SRC_LSB	30
#define CPCCR_SEL_SRC_MASK	BITS_H2L(31, CPCCR_SEL_SRC_LSB)

#define CPCCR_SEL_CPLL_LSB	28
#define CPCCR_SEL_CPLL_MASK	BITS_H2L(29, CPCCR_SEL_CPLL_LSB)

#define CPCCR_SEL_H0PLL_LSB	26
#define CPCCR_SEL_H0PLL_MASK	BITS_H2L(27, CPCCR_SEL_H0PLL_LSB)

#define CPCCR_SEL_H2PLL_LSB	24
#define CPCCR_SEL_H2PLL_MASK	BITS_H2L(25, CPCCR_SEL_H2PLL_LSB)

#define CPCCR_CE_CPU		BIT22
#define CPCCR_CE_AHB0		BIT21
#define CPCCR_CE_AHB2		BIT20
#define CPCCR_CE	(CPCCR_CE_CPU | CPCCR_CE_AHB0 | CPCCR_CE_AHB2)

#define CPCCR_PDIV_LSB		16
#define CPCCR_PDIV_MASK		BITS_H2L(19, CPCCR_PDIV_LSB)

#define CPCCR_H2DIV_LSB		12
#define CPCCR_H2DIV_MASK	BITS_H2L(15, CPCCR_H2DIV_LSB)

#define CPCCR_H0DIV_LSB		8
#define CPCCR_H0DIV_MASK	BITS_H2L(11, CPCCR_H0DIV_LSB)

#define CPCCR_L2DIV_LSB		4
#define CPCCR_L2DIV_MASK	BITS_H2L(7,  CPCCR_L2DIV_LSB)

#define CPCCR_CDIV_LSB		0
#define CPCCR_CDIV_MASK		BITS_H2L(3,  CPCCR_CDIV_LSB)

/* Clock Status register(CPCSR) */
#define CPCSR_H2DIV_BUSY              BIT2
#define CPCSR_H0DIV_BUSY              BIT1
#define CPCSR_CDIV_BUSY               BIT0
#define CPCSR_DIV_BUSY	(CPCSR_H2DIV_BUSY | CPCSR_H0DIV_BUSY | CPCSR_CDIV_BUSY)

/* Low power control register(LCR) */
#define LCR_PD_SCPU             BIT31
#define LCR_PD_VPU              BIT30
#define LCR_PD_GPU              BIT29
#define LCR_PD_GPS              BIT28
#define LCR_SCPU_ST             BIT27
#define LCR_VPU_ST              BIT26
#define LCR_GPU_ST              BIT25
#define LCR_GPS_ST              BIT24
#define LCR_GPU_IDLE            BIT20

#define LCR_PST_LSB             8
#define LCR_PST_MASK            BITS_H2L(19, LCR_PST_LSB)

#define LCR_DUTY_LSB            3
#define LCR_DUTY_MASK           BITS_H2L(7, LCR_DUTY_LSB)

#define LCR_LPM_LSB             0
#define LCR_LPM_MASK            BITS_H2L(1, LCR_LPM_LSB)
#define LCR_LPM_IDLE            (0x0 << LCR_LPM_LSB)
#define LCR_LPM_SLEEP           (0x1 << LCR_LPM_LSB)

/* Reset status register(RSR) */
#define RSR_P0R         BIT2
#define RSR_WR          BIT1
#define RSR_PR          BIT0

/* APLL control register (CPXPCR) */
#define CPAPCR_M_LSB	     24
#define CPAPCR_M_MASK	     BITS_H2L(30, CPAPCR_M_LSB)

#define CPAPCR_N_LSB	     18
#define CPAPCR_N_MASK	     BITS_H2L(22, CPAPCR_N_LSB)

#define CPAPCR_OD_LSB	     16
#define CPAPCR_OD_MASK	     BITS_H2L(17, CPAPCR_OD_LSB)

#define CPAPCR_LOCK		BIT15   /* LOCK bit */
#define CPAPCR_ON		BIT10
#define CPAPCR_BP		BIT9
#define CPAPCR_EN		BIT8

/* MPLL control register (CPXPCR) */
#define CPMPCR_M_LSB	     24
#define CPMPCR_M_MASK	     BITS_H2L(30, CPAPCR_M_LSB)

#define CPMPCR_N_LSB	     18
#define CPMPCR_N_MASK	     BITS_H2L(22, CPAPCR_N_LSB)

#define CPMPCR_OD_LSB	     16
#define CPMPCR_OD_MASK	     BITS_H2L(17, CPAPCR_OD_LSB)

#define CPMPCR_BP		BIT6
#define CPMPCR_EN		BIT7
#define CPMPCR_LOCK		BIT1	/* LOCK bit */
#define CPMPCR_ON		BIT0

/* Clock gate register 0(CGR0) */
#define CLKGR0_DDR              BIT31
#define CLKGR0_EPDE             BIT27
#define CLKGR0_EPDC             BIT26
#define CLKGR0_LCD0             BIT25
#define CLKGR0_CIM1             BIT24
#define CLKGR0_CIM0             BIT23
#define CLKGR0_UHC              BIT22
#define CLKGR0_GMAC             BIT21
#define CLKGR0_DMAC             BIT20
#define CLKGR0_VPU              BIT19
#define CLKGR0_UART3            BIT18
#define CLKGR0_UART2            BIT17
#define CLKGR0_UART1            BIT16
#define CLKGR0_UART0            BIT15
#define CLKGR0_SADC             BIT14
#define CLKGR0_PCM              BIT13
#define CLKGR0_MSC2             BIT12
#define CLKGR0_MSC1             BIT11
#define CLKGR0_AHB_MON          BIT10
#define CLKGR0_X2D              BIT9
#define CLKGR0_AIC              BIT8
#define CLKGR0_I2C2		BIT7
#define CLKGR0_I2C1		BIT6
#define CLKGR0_I2C0		BIT5
#define CLKGR0_SSI0             BIT4
#define CLKGR0_MSC0             BIT3
#define CLKGR0_OTG              BIT2
#define CLKGR0_BCH              BIT1
#define CLKGR0_NEMC             BIT0

/* Oscillator and power control register(OPCR) */
#define OPCR_IDLE_DIS           BIT31
#define OPCR_GPU_CLK_EN         BIT30
#define OPCR_L2CM_PD            (0x2 << 26)
#define OPCR_O1ST_LSB           8
#define OPCR_O1ST_MASK          BITS_H2L(15, OPCR_O1ST_LSB)
#define OPCR_OTGPHY0_ENABLE    BIT7    /* otg */
#define OPCR_OTGPHY1_ENABLE    BIT6    /* uhc */
#define OPCR_USBPHY_ENABLE     (OPCR_OTGPHY0_ENABLE | OPCR_OTGPHY1_ENABLE)
#define OPCR_O1SE               BIT4
#define OPCR_PD                 BIT3
#define OPCR_ERCS               BIT2

/* CPM scratch pad protected register(CPSPPR) */
#define CPSPPR_CPSPR_WRITABLE   (0x00005a5a)

/* OTG parameter control register(USBPCR) */
#define USBPCR_USB_MODE         BIT31
#define USBPCR_AVLD_REG         BIT30
#define USBPCR_INCRM            BIT27   /* INCR_MASK bit */
#define USBPCR_CLK12_EN         BIT26
#define USBPCR_COMMONONN        BIT25
#define USBPCR_VBUSVLDEXT       BIT24
#define USBPCR_VBUSVLDEXTSEL    BIT23
#define USBPCR_POR              BIT22
#define USBPCR_SIDDQ            BIT21
#define USBPCR_OTG_DISABLE      BIT20
#define USBPCR_TXPREEMPHTUNE    BIT6

#define USBPCR_IDPULLUP_LSB             28   /* IDPULLUP_MASK bit */
#define USBPCR_IDPULLUP_MASK            BITS_H2L(29, USBPCR_IDPULLUP_LSB)

#define USBPCR_COMPDISTUNE_LSB          17
#define USBPCR_COMPDISTUNE_MASK         BITS_H2L(19, USBPCR_COMPDISTUNE_LSB)

#define USBPCR_OTGTUNE_LSB              14
#define USBPCR_OTGTUNE_MASK             BITS_H2L(16, USBPCR_OTGTUNE_LSB)

#define USBPCR_SQRXTUNE_LSB             11
#define USBPCR_SQRXTUNE_MASK            BITS_H2L(13, USBPCR_SQRXTUNE_LSB)

#define USBPCR_TXFSLSTUNE_LSB           7
#define USBPCR_TXFSLSTUNE_MASK          BITS_H2L(10, USBPCR_TXFSLSTUNE_LSB)

#define USBPCR_TXRISETUNE_LSB           4
#define USBPCR_TXRISETUNE_MASK          BITS_H2L(5, USBPCR_TXRISETUNE_LSB)

#define USBPCR_TXVREFTUNE_LSB           0
#define USBPCR_TXVREFTUNE_MASK          BITS_H2L(3, USBPCR_TXVREFTUNE_LSB)

/* OTG parameter control register(USBPCR) */
#define USBPCR1_USB_SEL		BIT28
#define USBPCR1_DMPD1		BIT23
#define USBPCR1_DPPD1		BIT22
#define USBPCR1_PORT0_RST	BIT21
#define USBPCR1_PORT1_RST	BIT20
#define USBPCR1_WORD_IF0	BIT19
#define USBPCR1_WORD_IF1	BIT18
#define USBPCR1_TXPREEMPHTUNE1	BIT7
#define USBPCR1_TXRISETUNE1	BIT0

#define USBPCR1_TXVREFTUNE1_LSB		1
#define USBPCR1_TXVREFTUNE1_MASK	BITS_H2L(4, USBPCR1_TXVREFTUNE1_LSB)

#define USBPCR1_TXHSXVTUNE1_LSB		5
#define USBPCR1_TXHSXVTUNE1_MASK	BITS_H2L(6, USBPCR1_TXHSXVTUNE1_LSB)

#define USBPCR1_TXFSLSTUNE1_LSB		8
#define USBPCR1_TXFSLSTUNE1_MASK	BITS_H2L(11, USBPCR1_TXFSLSTUNE1_LSB)

#define USBPCR1_SQRXTUNE1_LSB		12
#define USBPCR1_SQRXTUNE1_MASK		BITS_H2L(14, USBPCR1_SQRXTUNE1_LSB)

#define USBPCR1_COMPDISTUNE1_LSB	15
#define USBPCR1_COMPDISTUNE1_MASK	BITS_H2L(17, USBPCR1_COMPDISTUNE1_LSB)

#define USBPCR1_REFCLKDIV_LSB		24
#define USBPCR1_REFCLKDIV_MASK		BITS_H2L(25, USBPCR1_REFCLKDIV_LSB)

#define USBPCR1_REFCLKSEL_LSB		26
#define USBPCR1_REFCLKSEL_MASK		BITS_H2L(27, USBPCR1_REFCLKSEL_LSB)

/* OTG reset detect timer register(USBRDT) */
#define USBRDT_VBFIL_LD_EN      BIT25
#define USBRDT_IDDIG_EN         BIT24
#define USBRDT_IDDIG_REG        BIT23

#define USBRDT_USBRDT_LSB       0
#define USBRDT_USBRDT_MASK      BITS_H2L(22, USBRDT_USBRDT_LSB)

/* OTG PHY clock divider register(USBCDR) */
#define USBCDR_UCS              BIT31
#define USBCDR_UPCS             BIT30
#define USBCDR_CE_USB           BIT29
#define USBCDR_USB_BUSY         BIT28
#define USBCDR_USB_STOP         BIT27

#define USBCDR_OTGDIV_LSB       0       /* USBCDR bit */
#define USBCDR_OTGDIV_MASK      BITS_H2L(7, USBCDR_OTGDIV_LSB)

/* DDR clock divider register(DDCDR) */
#define DDCDR_DCS_LSB		30
#define DDCDR_DCS_MASK	      	BITS_H2L(31, DDCDR_DCS_LSB)
#define DDCDR_DCS_STOP		(0 << DDCDR_DCS_LSB)
#define DDCDR_DCS_APLL		(1 << DDCDR_DCS_LSB)
#define DDCDR_DCS_MPLL		(2 << DDCDR_DCS_LSB)
#define DDCDR_CE_DDR	       	BIT29
#define DDCDR_DDR_BUSY	       	BIT28
#define DDCDR_DDR_STOP	       	BIT27
#define DDCDR_DDRDIV_LSB       	0
#define DDCDR_DDRDIV_MASK      	BITS_H2L(3, DDCDR_DDRDIV_LSB)

/* VPU clock divider register(VPUCDR) */
#define VPUCDR_VCS_LSB		30
#define VPUCDR_VCS_MASK	       	BITS_H2L(31, VPUCDR_VCS_LSB)
#define VPUCDR_VCS_APLL		(0 << VPUCDR_VCS_LSB)
#define VPUCDR_VCS_MPLL		(1 << VPUCDR_VCS_LSB)
#define VPUCDR_VCS_EPLL		(2 << VPUCDR_VCS_LSB)
#define VPUCDR_CE_VPU           BIT29
#define VPUCDR_VPU_BUSY         BIT28
#define VPUCDR_VPU_STOP         BIT27
#define VPUCDR_VPUDIV_LSB       0
#define VPUCDR_VPUDIV_MASK      BITS_H2L(3, VPUCDR_VPUDIV_LSB)

/* I2S device clock divider register(I2SCDR) */
#define I2SCDR_I2CS             BIT31
#define I2SCDR_I2PCS            BIT30
#define I2SCDR_CE_I2S           BIT29
#define I2SCDR_I2S_BUSY         BIT28
#define I2SCDR_I2S_STOP         BIT27

#define I2SCDR_I2SDIV_LSB       0       /* I2SCDR bit */
#define I2SCDR_I2SDIV_MASK      BITS_H2L(7, I2SCDR_I2SDIV_LSB)

/* LCD pix clock divider register(LPCDR) */
#define LPCDR_LPCS_LSB		31
#define LPCDR_LPCS_MASK	       	BITS_H2L(31, LPCDR_LPCS_LSB)
#define LPCDR_LPCS_APLL		(0 << LPCDR_LPCS_LSB)
#define LPCDR_LPCS_MPLL		(1 << LPCDR_LPCS_LSB)
#define LPCDR_LPCS_VPLL		(2 << LPCDR_LPCS_LSB)
#define LPCDR_CE_LCD            BIT28
#define LPCDR_LCD_BUSY          BIT27
#define LPCDR_LCD_STOP          BIT26

#define LPCDR_PIXDIV_LSB        0       /* LPCDR bit */
#define LPCDR_PIXDIV_MASK       BITS_H2L(7, LPCDR_PIXDIV_LSB)

/* MSC clock divider register(MSCCDR) */
#define MSCCDR_MPCS_LSB		30       /* MPCS bit */
#define MSCCDR_MPCS_MASK        BITS_H2L(31, MSCCDR_MPCS_LSB)
#define MSCCDR_MPCS_STOP	(0 << MSCCDR_MPCS_LSB)
#define MSCCDR_MPCS_APLL	(1 << MSCCDR_MPCS_LSB)
#define MSCCDR_MPCS_MPLL	(2 << MSCCDR_MPCS_LSB)

#define MSCCDR_CE_MSC           BIT29
#define MSCCDR_MSC_BUSY         BIT28
#define MSCCDR_MSC_STOP		BIT27
#define MSCCDR_S_CLK0_SEL	BIT15

#define MSCCDR_MSCDIV_LSB       0       /* MSCCDR bit */
#define MSCCDR_MSCDIV_MASK      BITS_H2L(7, MSCCDR_MSCDIV_LSB)

/* UHC device clock divider register(UHCCDR) */
#define UHCCDR_UHCS_LSB		30
#define UHCCDR_UHCS_MASK        BITS_H2L(31, UHCCDR_UHCS_LSB)
#define UHCCDR_UHCS_APLL	(0 << UHCCDR_UHCS_LSB)
#define UHCCDR_UHCS_MPLL	(1 << UHCCDR_UHCS_LSB)
#define UHCCDR_UHCS_EPLL	(2 << UHCCDR_UHCS_LSB)
#define UHCCDR_UHCS_OTG		(3 << UHCCDR_UHCS_LSB)

#define UHCCDR_CE_UHC		BIT29
#define UHCCDR_UHC_BUSY		BIT28
#define UHCCDR_UHC_STOP		BIT27

#define UHCCDR_UHCDIV_LSB       0       /* UHCCDR bit */
#define UHCCDR_UHCDIV_MASK      BITS_H2L(7, UHCCDR_UHCDIV_LSB)

/* SSI clock divider register(SSICDR) */
#define SSICDR_SCS              BIT31
#define SSICDR_SPCS             BIT30
#define SSICDR_SSI_STOP		BIT27

#define SSICDR_SSIDIV_LSB       0       /* SSICDR bit */
#define SSICDR_SSIDIV_MASK      BITS_H2L(7, SSICDR_SSIDIV_LSB)

/* CIM mclk clock divider register(CIMCDR) */
#define CIMCDR_CIMPCS_APLL	(0 << 31)
#define CIMCDR_CIMPCS_MPLL	BIT31
#define CIMCDR_CE_CIM		BIT30
#define CIMCDR_CIM_BUSY		BIT29
#define CIMCDR_CIM_STOP		BIT28

#define CIMCDR_CIMDIV_LSB       0       /* CIMCDR bit */
#define CIMCDR_CIMDIV_MASK      BITS_H2L(7, CIMCDR_CIMDIV_LSB)

/* GPS clock divider register(GPSCDR) */
#define GPSCDR_GPCS             BIT31

#define GPSCDR_GPSDIV_LSB       0       /* GPSCDR bit */
#define GSPCDR_GPSDIV_MASK      BITS_H2L(3, GPSCDR_GPSDIV_LSB)

/* PCM device clock divider register(PCMCDR) */
#define PCMCDR_PCMS             BIT31
#define PCMCDR_PCMPCS           BIT30
#define PCMCDR_CE_PCM           BIT28
#define PCMCDR_PCM_BUSY         BIT27
#define PCMCDR_PCM_STOP         BIT26

#define PCMCDR_PCMDIV_LSB       0       /* PCMCDR bit */
#define PCMCDR_PCMDIV_MASK      BITS_H2L(7, PCMCDR_PCMDIV_LSB)

/* BCH clock divider register */
#define BCHCDR_BPCS_LSB         30
#define BCHCDR_BPCS_MASK        BITS_H2L(31, BCHCDR_BPCS_LSB)
#define BCHCDR_BPCS_STOP	(0 << BCHCDR_BPCS_LSB)
#define BCHCDR_BPCS_APLL	(1 << BCHCDR_BPCS_LSB)
#define BCHCDR_BPCS_MPLL	(2 << BCHCDR_BPCS_LSB)
#define BCHCDR_BPCS_EPLL	(3 << BCHCDR_BPCS_LSB)

#define BCHCDR_CE_BCH		BIT29
#define BCHCDR_BCH_BUSY		BIT28
#define BCHCDR_STOP		BIT27
#define BCHCDR_BCHDIV_LSB	0
#define BCHCDR_BCHDIV_MASK	BITS_H2L(3, BCHCDR_BCHDIV_LSB)

/* MAC PHY Control register */
#define MPHYC_TXCLK_SEL		BIT31
#define MPHYC_MAC_SPEED_LSB	29
#define MPHYC_MAC_SPEED_MASK	BITS_H2L(30, MPHYC_MAC_SPEED_LSB)
#define MPHYC_MAC_SPEED_1000M	(1 << MPHYC_MAC_SPEED_LSB)
#define MPHYC_MAC_SPEED_10M	(2 << MPHYC_MAC_SPEED_LSB)
#define MPHYC_MAC_SPEED_100M	(3 << MPHYC_MAC_SPEED_LSB)

#define MPHYC_MAC_SOFTRST	BIT3
#define MPHYC_MAC_PHYINTF_LSB	0
#define MPHYC_MAC_PHYINTF_MASK	BITS_H2L(2, MPHYC_MAC_PHYINTF_LSB)
#define MPHYC_MAC_PHYINTF_G_MII	(0 << MPHYC_MAC_PHYINTF_LSB)
#define MPHYC_MAC_PHYINTF_RGMII	(1 << MPHYC_MAC_PHYINTF_LSB)
#define MPHYC_MAC_PHYINTF_RMII	(4 << MPHYC_MAC_PHYINTF_LSB)

#ifndef __MIPS_ASSEMBLER

#define REG_CPM_CPCCR           REG32(CPM_CPCCR)
#define REG_CPM_CPCSR           REG32(CPM_CPCSR)
#define REG_CPM_DDRCDR          REG32(CPM_DDRCDR)
#define REG_CPM_VPUCDR          REG32(CPM_VPUCDR)
#define REG_CPM_I2SCDR          REG32(CPM_I2SCDR)
#define REG_CPM_I2S1CDR         REG32(CPM_I21SCDR)
#define REG_CPM_USBCDR          REG32(CPM_USBCDR)
#define REG_CPM_LPCDR0          REG32(CPM_LPCDR0)
#define REG_CPM_LPCDR1          REG32(CPM_LPCDR1)
#define REG_CPM_MSC0CDR         REG32(CPM_MSC0CDR)
#define REG_CPM_MSC1CDR         REG32(CPM_MSC1CDR)
#define REG_CPM_MSC2CDR         REG32(CPM_MSC2CDR)
#define REG_CPM_UHCCDR          REG32(CPM_UHCCDR)
#define REG_CPM_SSICDR          REG32(CPM_SSICDR)
#define REG_CPM_CIMCDR          REG32(CPM_CIMCDR)
#define REG_CPM_CIM1CDR         REG32(CPM_CIM1CDR)
#define REG_CPM_PCMCDR          REG32(CPM_PCMCDR)
#define REG_CPM_GPUCDR          REG32(CPM_GPUCDR)
#define REG_CPM_HDMICDR         REG32(CPM_HDMICDR)
#define REG_CPM_BCHCDR          REG32(CPM_BCHCDR)
#define REG_CPM_INTRCDR         REG32(CPM_INTRCDR)
#define REG_CPM_CPSPR           REG32(CPM_CPSPR)
#define REG_CPM_CPSPPR          REG32(CPM_CPSPPR)
#define REG_CPM_USBPCR          REG32(CPM_USBPCR)
#define REG_CPM_USBRDT          REG32(CPM_USBRDT)
#define REG_CPM_USBVBFIL        REG32(CPM_USBVBFIL)
#define REG_CPM_USBPCR1         REG32(CPM_USBPCR1)
#define REG_CPM_CPPCR           REG32(CPM_CPPCR)
#define REG_CPM_CPAPCR          REG32(CPM_CPAPCR)
#define REG_CPM_CPMPCR          REG32(CPM_CPMPCR)
#define REG_CPM_CPEPCR          REG32(CPM_CPEPCR)
#define REG_CPM_CPVPCR          REG32(CPM_CPVPCR)

#define REG_CPM_LCR             REG32(CPM_LCR)
#define REG_CPM_SPCR0           REG32(CPM_SPCR0)
#define REG_CPM_SPCR1           REG32(CPM_SPCR1)
#define REG_CPM_PSWC0ST         REG32(CPM_PSWC0ST)
#define REG_CPM_PSWC1ST         REG32(CPM_PSWC1ST)
#define REG_CPM_PSWC2ST         REG32(CPM_PSWC2ST)
#define REG_CPM_PSWC3ST         REG32(CPM_PSWC3ST)

#define REG_CPM_CLKGR0          REG32(CPM_CLKGR0)
#define REG_CPM_SRBC            REG32(CPM_SRBC)
#define REG_CPM_SLBC            REG32(CPM_SLBC)
#define REG_CPM_SLPC            REG32(CPM_SLPC)
#define REG_CPM_OPCR            REG32(CPM_OPCR)
#define REG_CPM_RSR             REG32(CPM_RSR)

typedef enum {
	SCLK_APLL = 0,
	SCLK_MPLL,
} src_clk;

typedef enum {
	CGM_NEMC  = 0,
	CGM_BCH   = 1,
	CGM_OTG   = 2,
	CGM_MSC0  = 3,
	CGM_SSI0  = 4,
	CGM_I2C0 	= 5,
	CGM_I2C1 	= 6,
	CGM_I2C2  	= 7,
	CGM_AIC0        = 8,
	CGM_X2D         = 9,
	CGM_AHB_MON     = 10,
	CGM_MSC1  = 11,
	CGM_MSC2  = 12,
	CGM_PCM   = 13,
	CGM_SADC  = 14,
	CGM_UART0       = 15,
	CGM_UART1 	= 16,
	CGM_UART2 	= 17,
	CGM_UART3 	= 18,
	CGM_VPU	 	= 19,
	CGM_DMAC  = 20,
	CGM_MAC   = 21,
	CGM_UHC   = 22,
	CGM_CIM0  = 23,
	CGM_CIM1  = 24,
	CGM_LCD0        = 25,
	CGM_EPDC        = 26,
	CGM_EPDE        = 27,
	CGM_DDR   = 31,
	CGM_ALL_MODULE,
} clock_gate_module;


#define __CGU_CLOCK_BASE__      0x1000

typedef enum {
	/* Clock source is pll0 */
	CGU_CCLK = __CGU_CLOCK_BASE__ + 0,
	CGU_L2CLK,
	CGU_HCLK,
	CGU_H2CLK,
	CGU_PCLK,

	/* Clock source is apll or mpll */
	CGU_MCLK,
	CGU_MSC0CLK,
	CGU_MSC1CLK,
	CGU_MSC2CLK,
	CGU_CIMCLK,
	CGU_CIM1CLK,

	/* Clock source is apll, mpll or exclk */
	CGU_SSICLK,
	CGU_OTGCLK,

	/* Clock source is apll, mpll or vpll */
	CGU_LPCLK0,
	CGU_LPCLK1,
	CGU_HDMICLK,

	/* Clock source is apll, mpll or epll */
	CGU_VPUCLK,
	CGU_GPUCLK,

	/* Clock source is apll, epll or exclk */
	CGU_I2SCLK,

	/* Clock source is apl, mpll, vpll, epll or exclk*/
	CGU_PCMCLK,

	/* Clock source is apll, mpll, epll or otg phy  */
	CGU_UHCCLK,

	/* Clock source is exclk */
	CGU_UARTCLK,
	CGU_SADCCLK,
	CGU_TCUCLK,

	/* Clock source is external rtc clock */
	CGU_RTCCLK,

	/* Clock source is pll0, pll0/2 or pll1 */
	CGU_GPSCLK,

	CGU_CLOCK_MAX,
} cgu_clock;

/*
 * JZ4775 clocks structure
 */
typedef struct {
	unsigned int cclk;	/* CPU clock				*/
	unsigned int l2clk;	/* L2Cache clock			*/
	unsigned int hclk;	/* System bus clock: AHB0		*/
	unsigned int h2clk;	/* System bus clock: AHB2		*/
	unsigned int pclk;	/* Peripheral bus clock			*/
	unsigned int mclk;	/* DDR controller clock			*/
	unsigned int vpuclk;	/* VPU controller clock			*/
	unsigned int sclk;	/* NEMC controller clock		*/
	unsigned int cko;	/* SDRAM or DDR clock			*/
	unsigned int pixclk0;	/* LCD0 pixel clock			*/
	unsigned int pixclk1;	/* LCD1 pixel clock			*/
	unsigned int tveclk;	/* TV encoder 27M  clock		*/
	unsigned int cimmclk;	/* Clock output from CIM module		*/
	unsigned int cimpclk;	/* Clock input to CIM module		*/
	unsigned int gpuclk;	/* GPU clock				*/
	unsigned int gpsclk;	/* GPS clock				*/
	unsigned int i2sclk;	/* I2S codec clock			*/
	unsigned int bitclk;	/* AC97 bit clock			*/
	unsigned int pcmclk;	/* PCM clock				*/
	unsigned int msc0clk;	/* MSC0 clock				*/
	unsigned int msc1clk;	/* MSC1 clock				*/
	unsigned int msc2clk;	/* MSC2 clock				*/
	unsigned int ssiclk;	/* SSI clock				*/
	unsigned int tssiclk;	/* TSSI clock				*/
	unsigned int otgclk;	/* USB OTG clock			*/
	unsigned int uhcclk;	/* USB UHCI clock			*/
	unsigned int extalclk;	/* EXTAL clock for
				   UART,I2C,TCU,USB2.0-PHY,AUDIO CODEC	*/
	unsigned int rtcclk;	/* RTC clock for CPM,INTC,RTC,TCU,WDT	*/
} jz_clocks_t;

void cpm_start_clock(clock_gate_module module_name);
void cpm_stop_clock(clock_gate_module module_name);

unsigned int cpm_set_clock(cgu_clock clock_name, unsigned int clock_hz);
unsigned int cpm_get_clock(cgu_clock clock_name);
unsigned int cpm_get_xpllout(src_clk);

void cpm_uhc_phy(unsigned int en);

/**************remove me if android's kernel support these operations********start*********  */
#define __cpm_stop_lcd1()	(REG_CPM_CLKGR0 |= CLKGR0_LCD1)
#define __cpm_start_lcd1()	(REG_CPM_CLKGR0 &= ~CLKGR0_LCD1)

#define __cpm_stop_lcd0()	(REG_CPM_CLKGR0 |= CLKGR0_LCD0)
#define __cpm_start_lcd0()	(REG_CPM_CLKGR0 &= ~CLKGR0_LCD0)

#define cpm_get_pllout()	cpm_get_xpllout(SCLK_APLL)
#define cpm_get_pllout1()	cpm_get_xpllout(SCLK_MPLL)
static __inline__ unsigned int __cpm_get_pllout2(void)
{
#if defined(CONFIG_FPGA)
	return JZ_EXTAL;
#else
	return cpm_get_xpllout(SCLK_APLL);
#endif
}

/* EXTAL clock for UART,I2C,SSI,SADC,USB-PHY */
static __inline__ unsigned int __cpm_get_extalclk(void)
{
	return JZ_EXTAL;
}

/* RTC clock for CPM,INTC,RTC,TCU,WDT */
static __inline__ unsigned int __cpm_get_rtcclk(void)
{
	return JZ_EXTAL2;
}

extern jz_clocks_t jz_clocks;

#define __cpm_select_i2sclk_exclk()	(REG_CPM_I2SCDR &= ~I2SCDR_I2CS)
#define __cpm_enable_pll_change()	(REG_CPM_CPCCR |= CPCCR_CE)

#define __cpm_suspend_otg_phy()		(REG_CPM_OPCR &= ~OPCR_OTGPHY0_ENABLE)
#define __cpm_enable_otg_phy()		(REG_CPM_OPCR |=  OPCR_OTGPHY0_ENABLE)

#define __cpm_suspend_uhc_phy()		(REG_CPM_USBPCR1 &= ~(1 << 17))
#define __cpm_enable_uhc_phy()		(REG_CPM_USBPCR1 |=  (1 << 17))

#define __cpm_disable_osc_in_sleep()	(REG_CPM_OPCR &= ~OPCR_O1SE)
#define __cpm_select_rtcclk_rtc()	(REG_CPM_OPCR |= OPCR_ERCS)

#define __cpm_get_pllm() \
	((REG_CPM_CPPCR0 & CPPCR0_PLLM_MASK) >> CPPCR0_PLLM_LSB)
#define __cpm_get_plln() \
	((REG_CPM_CPPCR0 & CPPCR0_PLLN_MASK) >> CPPCR0_PLLN_LSB)
#define __cpm_get_pllod() \
	((REG_CPM_CPPCR0 & CPPCR0_PLLOD_MASK) >> CPPCR0_PLLOD_LSB)

#define __cpm_get_pll1m() \
	((REG_CPM_CPPCR1 & CPPCR1_PLL1M_MASK) >> CPPCR1_PLL1M_LSB)
#define __cpm_get_pll1n() \
	((REG_CPM_CPPCR1 & CPPCR1_PLL1N_MASK) >> CPPCR1_PLL1N_LSB)
#define __cpm_get_pll1od() \
	((REG_CPM_CPPCR1 & CPPCR1_PLL1OD_MASK) >> CPPCR1_PLL1OD_LSB)

#define __cpm_get_cdiv() \
	((REG_CPM_CPCCR & CPCCR_CDIV_MASK) >> CPCCR_CDIV_LSB)
#define __cpm_get_hdiv() \
	((REG_CPM_CPCCR & CPCCR_H0DIV_MASK) >> CPCCR_H0DIV_LSB)
#define __cpm_get_h2div() \
	((REG_CPM_CPCCR & CPCCR_H2DIV_MASK) >> CPCCR_H2DIV_LSB)
#define __cpm_get_otgdiv() \
	((REG_CPM_USBCDR & USBCDR_OTGDIV_MASK) >> USBCDR_OTGDIV_LSB)
#define __cpm_get_pdiv() \
	((REG_CPM_CPCCR & CPCCR_PDIV_MASK) >> CPCCR_PDIV_LSB)

#define __cpm_get_mdiv() __cpm_get_hdiv()

#define __cpm_get_sdiv() \
	((REG_CPM_CPCCR & CPCCR_H1DIV_MASK) >> CPCCR_H1DIV_LSB)
#define __cpm_get_i2sdiv() \
	((REG_CPM_I2SCDR & I2SCDR_I2SDIV_MASK) >> I2SCDR_I2SDIV_LSB)
#define __cpm_get_pixdiv() \
	((REG_CPM_LPCDR & LPCDR_PIXDIV_MASK) >> LPCDR_PIXDIV_LSB)
#define __cpm_get_ssidiv() \
	((REG_CPM_SSICDR & SSICDR_SSIDIV_MASK) >> SSICDR_SSIDIV_LSB)
#define __cpm_get_pcmdiv() \
	((REG_CPM_PCMCDR & PCMCDR_PCMCD_MASK) >> PCMCDR_PCMCD_LSB)
#define __cpm_get_pll1div() \
	((REG_CPM_CPPCR1 & CPCCR1_P1SDIV_MASK) >> CPCCR1_P1SDIV_LSB)


#define __cpm_set_ssidiv(v) \
	(REG_CPM_SSICDR = (REG_CPM_SSICDR & ~SSICDR_SSIDIV_MASK) | ((v) << (SSICDR_SSIDIV_LSB)))

#define __cpm_exclk_direct()		(REG_CPM_CPCCR &= ~CPM_CPCCR_ECS)
#define __cpm_exclk_div2()          (REG_CPM_CPCCR |= CPM_CPCCR_ECS)
#define __cpm_pllout_direct()		(REG_CPM_CPCCR |= CPM_CPCCR_PCS)
#define __cpm_pllout_div2()			(REG_CPM_CPCCR &= ~CPM_CPCCR_PCS)

#define cpm_get_scrpad()	INREG32(CPM_CPSPR)
#define cpm_set_scrpad(data)				\
do {							\
	OUTREG32(CPM_CPSPPR, CPSPPR_CPSPR_WRITABLE);	\
	OUTREG32(CPM_CPSPR, data);			\
	OUTREG32(CPM_CPSPPR, ~CPSPPR_CPSPR_WRITABLE);	\
} while (0)

#define __cpm_get_bchdiv(n) \
	((REG_CPM_BCHCDR(n) & CPM_BCHCDR_BCHDIV_MASK) >> CPM_BCHCDR_BCHDIV_BIT)
#define __cpm_set_bchdiv(v) \
	(REG_CPM_BCHCDR = (REG_CPM_BCHCDR & ~CPM_BCHCDR_BCHDIV_MASK) | ((v) << (CPM_BCHCDR_BCHDIV_BIT)))

/**************remove me if android's kernel support these operations********end*********  */
extern int jz_pm_init(void);

#endif /* __MIPS_ASSEMBLER */

#endif /* __JZ4775CPM_H__ */
