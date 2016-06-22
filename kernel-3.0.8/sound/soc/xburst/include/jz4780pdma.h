/*
 * linux/include/asm-mips/mach-jz4780/jz4780pdmac.h
 *
 * JZ4780 DMAC register definition.
 *
 * Copyright (C) 2010 Ingenic Semiconductor Co., Ltd.
 */

#ifndef __JZ4780PDMAC_H__
#define __JZ4780PDMAC_H__


#define DMAC_BASE	0xB3420000


/*************************************************************************
 * DMAC (DMA Controller)
 *************************************************************************/

#define MAX_DMA_NUM 	32  /* max 32 channels */

/* DMA Channel Register (0 ~ 31) */
#define DMAC_DSAR(n)  (DMAC_BASE + (0x00 + (n) * 0x20)) /* DMA source address */
#define DMAC_DTAR(n)  (DMAC_BASE + (0x04 + (n) * 0x20)) /* DMA target address */
#define DMAC_DTCR(n)  (DMAC_BASE + (0x08 + (n) * 0x20)) /* DMA transfer count */
#define DMAC_DRSR(n)  (DMAC_BASE + (0x0C + (n) * 0x20)) /* DMA request source */
#define DMAC_DCCSR(n) (DMAC_BASE + (0x10 + (n) * 0x20)) /* DMA control/status */
#define DMAC_DCMD(n)  (DMAC_BASE + (0x14 + (n) * 0x20)) /* DMA command */
#define DMAC_DDA(n)   (DMAC_BASE + (0x18 + (n) * 0x20)) /* DMA descriptor addr */
#define DMAC_DSD(n)   (DMAC_BASE + (0x1C + (n) * 0x04)) /* DMA Stride Address */

#define DMAC_DMACR    (DMAC_BASE + 0x1000)              /* DMA control register */
#define DMAC_DMAIPR   (DMAC_BASE + 0x1004)              /* DMA interrupt pending */
#define DMAC_DMADBR   (DMAC_BASE + 0x1008)              /* DMA doorbell */
#define DMAC_DMADBSR  (DMAC_BASE + 0x100C)              /* DMA doorbell set */
#define DMAC_DMACK    (DMAC_BASE + 0x1010)
#define DMAC_DMACKS   (DMAC_BASE + 0x1014)
#define DMAC_DMACKC   (DMAC_BASE + 0x1018)
#define DMAC_DMACP    (DMAC_BASE + 0x101C)		/* DMA Channel Programmable */
#define DMAC_DSIRQP   (DMAC_BASE + 0x1020)		/* DMA Soft IRQ Pending */
#define DMAC_DSIRQM   (DMAC_BASE + 0x1024)		/* DMA Soft IRQ Mask */
#define DMAC_DCIRQP   (DMAC_BASE + 0x1028)		/* DMA Channel IRQ Pending to MCU */
#define DMAC_DCIRQM   (DMAC_BASE + 0x102C)		/* DMA Channel IRQ Mask to MCU */

/* PDMA MCU Control Register */
#define DMAC_DMCSR    (DMAC_BASE + 0x1030)		/* DMA MCU Control/Status */
#define DMAC_DMNMB    (DMAC_BASE + 0x1034)		/* DMA MCU Normal Mailbox */
#define DMAC_DMSMB    (DMAC_BASE + 0x1038)		/* DMA MCU Security Mailbox */
#define DMAC_DMINT    (DMAC_BASE + 0x103C)		/* DMA MCU Interrupt */

#define REG_DMAC_DSAR(n)	REG32(DMAC_DSAR(n))
#define REG_DMAC_DTAR(n)	REG32(DMAC_DTAR(n))
#define REG_DMAC_DTCR(n)	REG32(DMAC_DTCR(n))
#define REG_DMAC_DRSR(n)	REG32(DMAC_DRSR(n))
#define REG_DMAC_DCCSR(n)	REG32(DMAC_DCCSR(n))
#define REG_DMAC_DCMD(n)	REG32(DMAC_DCMD(n))
#define REG_DMAC_DDA(n)		REG32(DMAC_DDA(n))
#define REG_DMAC_DSD(n)		REG32(DMAC_DSD(n))
#define REG_DMAC_DMACR		REG32(DMAC_DMACR)
#define REG_DMAC_DMAIPR		REG32(DMAC_DMAIPR)
#define REG_DMAC_DMADBR		REG32(DMAC_DMADBR)
#define REG_DMAC_DMADBSR	REG32(DMAC_DMADBSR)
#define REG_DMAC_DMACK		REG32(DMAC_DMACK)
#define REG_DMAC_DMACKS		REG32(DMAC_DMACKS)
#define REG_DMAC_DMACKC		REG32(DMAC_DMACKC)
#define REG_DMAC_DMACP		REG32(DMAC_DMACP)
#define REG_DMAC_DSIRQP		REG32(DMAC_DSIRQP)
#define REG_DMAC_DSIRQM		REGRE(DMAC_DSIRQM)
#define REG_DMAC_DCIRQP		REG32(DMAC_DCIRQP)
#define REG_DMAC_DCIRQM		REG32(DMAC_DCIRQM)
#define REG_DMAC_DMCSR		REG32(DMAC_DMCSR)
#define REG_DMAC_DMNMB		REG32(DMAC_DMNMB)
#define REG_DMAC_DMSMB		REG32(DMAC_DMNMB)
#define REG_DMAC_DMINT		REG32(DMAC_DMINT)

// DMA request source register
#define DMAC_DRSR_RS_BIT	0
#define DMAC_DRSR_RS_MASK	(0x3f << DMAC_DRSR_RS_BIT)
/* 0~7 is reserved */
#define DMAC_DRSR_RS_AICOUT	(6 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_AICIN	(7 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_AUTO	(8 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_SDACIN	(9 << DMAC_DRSR_RS_BIT)

#define DMAC_DRSR_RS_PM1OUT	(0xa << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_PM1IN	(0xb << DMAC_DRSR_RS_BIT)
/* 10 ~ 11 is reserved */
#define DMAC_DRSR_RS_UART4OUT	(12 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART4IN	(13 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART3OUT	(14 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART3IN	(15 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART2OUT	(16 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART2IN	(17 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART1OUT	(18 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART1IN	(19 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART0OUT	(20 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART0IN	(21 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_SSI0OUT	(22 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_SSI0IN	(23 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_SSI1OUT	(24 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_SSI1IN	(25 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_MSC0OUT	(26 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_MSC0IN	(27 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_MSC1OUT	(28 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_MSC1IN	(29 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_MSC2OUT	(30 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_MSC2IN	(31 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_PM0OUT	(32 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_PM0IN	(33 << DMAC_DRSR_RS_BIT)
/* 34 ~ 35 is reserved */
#define DMAC_DRSR_RS_I2C0OUT	(36 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_I2C0IN	(37 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_I2C1OUT	(38 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_I2C1IN	(39 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_I2C2OUT	(40 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_I2C2IN	(41 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_I2C3OUT	(42 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_I2C3IN	(43 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_I2C4OUT	(44 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_I2C4IN	(45 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_DESOUT	(46 << DMAC_DRSR_RS_PM0OUT)
#define DMAC_DRSR_RS_DESIN	(47 << DMAC_DRSR_RS_PM0OUT)
/* others are reserved */

// DMA channel control/status register
#define DMAC_DCCSR_NDES		(1 << 31) /* descriptor (0) or not (1) ? */
#define DMAC_DCCSR_DES8    	(1 << 30) /* Descriptor 8 Word */
#define DMAC_DCCSR_DES4    	(0 << 30) /* Descriptor 4 Word */
#define DMAC_DCCSR_TOC_BIT	16        /* Time out counter */
#define DMAC_DCCSR_TOC_MASK	(0x3fff << DMAC_DCCSR_TOC_BIT)
#define DMAC_DCCSR_CDOA_BIT	8         /* copy of DMA offset address */
#define DMAC_DCCSR_CDOA_MASK	(0xff << DMAC_DCCSR_CDOA_BIT)
/* [7:5] reserved */
#define DMAC_DCCSR_AR		(1 << 4)  /* Address Error */
#define DMAC_DCCSR_TT		(1 << 3)  /* Transfer Terminated */
#define DMAC_DCCSR_HLT		(1 << 2)  /* DMA Halted */
#define DMAC_DCCSR_CT		(1 << 1)  /* count terminated */
#define DMAC_DCCSR_EN		(1 << 0)  /* Channel Transfer Enable */

// DMA channel command register
#define DMAC_DCMD_EACKS_LOW  	(1 << 31) /* External DACK Output Level Select, active low */
#define DMAC_DCMD_EACKS_HIGH  	(0 << 31) /* External DACK Output Level Select, active high */
#define DMAC_DCMD_EACKM_WRITE 	(1 << 30) /* External DACK Output Mode Select, output in write cycle */
#define DMAC_DCMD_EACKM_READ 	(0 << 30) /* External DACK Output Mode Select, output in read cycle */
#define DMAC_DCMD_ERDM_BIT      28        /* External DREQ Detection Mode Select */
#define DMAC_DCMD_ERDM_MASK     (0x03 << DMAC_DCMD_ERDM_BIT)
#define DMAC_DCMD_ERDM_LOW    (0 << DMAC_DCMD_ERDM_BIT)
#define DMAC_DCMD_ERDM_FALL   (1 << DMAC_DCMD_ERDM_BIT)
#define DMAC_DCMD_ERDM_HIGH   (2 << DMAC_DCMD_ERDM_BIT)
#define DMAC_DCMD_ERDM_RISE   (3 << DMAC_DCMD_ERDM_BIT)
/* [31:28] reserved  the bits before is no use*/
#define DMAC_DCMD_SID_BIT	26        /* Source Identification */
#define DMAC_DCMD_SID_MASK	(0x3 << DMAC_DCMD_SID_BIT)
#define DMAC_DCMD_SID(n)	((n) << DMAC_DCMD_SID_BIT)
#define DMAC_DCMD_SID_TCSM	(0 << DMAC_DCMD_SID_BIT)
#define DMAC_DCMD_SID_SPECIAL	(1 << DMAC_DCMD_SID_BIT)
#define DMAC_DCMD_SID_DDR	(2 << DMAC_DCMD_SID_BIT)

#define DMAC_DCMD_DID_BIT	24       /* Destination Identification */
#define DMAC_DCMD_DID_MASK	(0x3 << DMAC_DCMD_DID_BIT)
#define DMAC_DCMD_DID(n)	((n) << DMAC_DCMD_DID_BIT)
#define DMAC_DCMD_DID_TCSM	(0 << DMAC_DCMD_DID_BIT)
#define DMAC_DCMD_DID_SPECIAL	(1 << DMAC_DCMD_DID_BIT)
#define DMAC_DCMD_DID_DDR	(2 << DMAC_DCMD_DID_BIT)

#define DMAC_DCMD_SAI		(1 << 23) /* source address increment */
#define DMAC_DCMD_DAI		(1 << 22) /* dest address increment */
#define DMAC_DCMD_RDIL_BIT	16        /* request detection interval length */
#define DMAC_DCMD_RDIL_MASK	(0x0f << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_IGN	(0 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_2	(1 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_4	(2 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_8	(3 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_12	(4 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_16	(5 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_20	(6 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_24	(7 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_28	(8 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_32	(9 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_48	(10 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_60	(11 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_64	(12 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_124	(13 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_128	(14 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_200	(15 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_SWDH_BIT	14  /* source port width */
#define DMAC_DCMD_SWDH_MASK	(0x03 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_SWDH_32	(0 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_SWDH_8	(1 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_SWDH_16	(2 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_DWDH_BIT	12  /* dest port width */
#define DMAC_DCMD_DWDH_MASK	(0x03 << DMAC_DCMD_DWDH_BIT)
#define DMAC_DCMD_DWDH_32	(0 << DMAC_DCMD_DWDH_BIT)
#define DMAC_DCMD_DWDH_8	(1 << DMAC_DCMD_DWDH_BIT)
#define DMAC_DCMD_DWDH_16	(2 << DMAC_DCMD_DWDH_BIT)
/* [11] reserved */
#define DMAC_DCMD_DS_BIT	8   /* Transfer Data Size of a data unit */
#define DMAC_DCMD_DS_MASK	(0x07 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_32BIT	(0 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_8BIT	(1 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_16BIT	(2 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_16BYTE	(3 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_32BYTE	(4 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_64BYTE	(5 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_128BYTE	(6 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_AUTO	(7 << DMAC_DCMD_DS_BIT)
/* [7:3] reserved */
#define DMAC_DCMD_STDE   	(1 << 2)  /* Stride Disable/Enable */
#define DMAC_DCMD_TIE		(1 << 1)  /* DMA transfer interrupt enable */
#define DMAC_DCMD_LINK		(1 << 0)  /* descriptor link enable */

// DMA descriptor address register
#define DMAC_DDA_BASE_BIT	12  /* descriptor base address */
#define DMAC_DDA_BASE_MASK	(0x0fffff << DMAC_DDA_BASE_BIT)
#define DMAC_DDA_OFFSET_BIT	4   /* descriptor offset address */
#define DMAC_DDA_OFFSET_MASK	(0x0ff << DMAC_DDA_OFFSET_BIT)
/* [3:0] reserved */

// DMA stride address register
#define DMAC_DSD_TSD_BIT        16  /* target stride address */
#define DMAC_DSD_TSD_MASK      	(0xffff << DMAC_DSD_TSD_BIT)
#define DMAC_DSD_SSD_BIT        0  /* source stride address */
#define DMAC_DSD_SSD_MASK      	(0xffff << DMAC_DSD_SSD_BIT)

// DMA control register
#define DMAC_DMACR_FMSC		(1 << 31)  /* MSC Fast DMA mode */
#define DMAC_DMACR_FSSI		(1 << 30)  /* SSI Fast DMA mode */
#define DMAC_DMACR_FTSSI	(1 << 29)  /* TSSI Fast DMA mode */
#define DMAC_DMACR_FUART	(1 << 28)  /* UART Fast DMA mode */
#define DMAC_DMACR_FAIC		(1 << 27)  /* AIC Fast DMA mode */
/* [26:22] reserved */
#define DMAC_DMACR_INTCC_BIT	17         /* Channel Interrupt Counter */
#define DMAC_DMACR_INTCC_MASK	(0x1f << DMAC_DMACR_INTCC_MASK)
#define DMAC_DMACR_INTCE	(1 << 16)  /*Channel Intertupt Enable */
/* [15:4] reserved */
#define DMAC_DMACR_PR_BIT	8  /* channel priority mode */
#define DMAC_DMACR_PR_MASK	(0x03 << DMAC_DMACR_PR_BIT)
#define DMAC_DMACR_PR_012345	(0 << DMAC_DMACR_PR_BIT)
#define DMAC_DMACR_PR_120345	(1 << DMAC_DMACR_PR_BIT)
#define DMAC_DMACR_PR_230145	(2 << DMAC_DMACR_PR_BIT)
#define DMAC_DMACR_PR_340125	(3 << DMAC_DMACR_PR_BIT)
/* [15:4] resered */
#define DMAC_DMACR_HLT		(1 << 3)  /* DMA halt flag */
#define DMAC_DMACR_AR		(1 << 2)  /* address error flag */
#define DMAC_DMACR_CH01		(1 << 1)  /* Special Channel 0 and Channel 1 Enable */
#define DMAC_DMACR_DMAE		(1 << 0)  /* DMA enable bit */

// DMA doorbell register
#define DMAC_DDB_DB(n)		(1 << (n)) /* Doorbell for Channel n */
#define DMAC_DMADBR_DB5		(1 << 5)  /* doorbell for channel 5 */
#define DMAC_DMADBR_DB4		(1 << 4)  /* doorbell for channel 4 */
#define DMAC_DMADBR_DB3		(1 << 3)  /* doorbell for channel 3 */
#define DMAC_DMADBR_DB2		(1 << 2)  /* doorbell for channel 2 */
#define DMAC_DMADBR_DB1		(1 << 1)  /* doorbell for channel 1 */
#define DMAC_DMADBR_DB0		(1 << 0)  /* doorbell for channel 0 */

// DMA doorbell set register
#define DMAC_DDBS_DBS(n)	(1 << (n)) /* Enable Doorbell for Channel n */
#define DMAC_DMADBSR_DBS5	(1 << 5)  /* enable doorbell for channel 5 */
#define DMAC_DMADBSR_DBS4	(1 << 4)  /* enable doorbell for channel 4 */
#define DMAC_DMADBSR_DBS3	(1 << 3)  /* enable doorbell for channel 3 */
#define DMAC_DMADBSR_DBS2	(1 << 2)  /* enable doorbell for channel 2 */
#define DMAC_DMADBSR_DBS1	(1 << 1)  /* enable doorbell for channel 1 */
#define DMAC_DMADBSR_DBS0	(1 << 0)  /* enable doorbell for channel 0 */

// DMA interrupt pending register
#define DMAC_DMAIPR_CIRQ(n)	(1 << (n)) /* irq pending status for channel 5 */
#define DMAC_DMAIPR_CIRQ5	(1 << 5)  /* irq pending status for channel 5 */
#define DMAC_DMAIPR_CIRQ4	(1 << 4)  /* irq pending status for channel 4 */
#define DMAC_DMAIPR_CIRQ3	(1 << 3)  /* irq pending status for channel 3 */
#define DMAC_DMAIPR_CIRQ2	(1 << 2)  /* irq pending status for channel 2 */
#define DMAC_DMAIPR_CIRQ1	(1 << 1)  /* irq pending status for channel 1 */
#define DMAC_DMAIPR_CIRQ0	(1 << 0)  /* irq pending status for channel 0 */

// DMA Channel Programmable
#define DMAC_DMACP_DCP(n)	(1 << (n)) /* Channel programmable enable */

/*************************************************************************
 * PDMA MCU
 *************************************************************************/

#define NUM_MCU		2  /* 0: Normal Mailbox IRQ. 1: Security Mailbox IRQ */

// MCU Control/Status
#define DMAC_DMCSR_SLEEP	(1 << 31) /* Sleep Status of MCU */
#define DMAC_DMCSR_SCMD		(1 << 30) /* Security Mode */
/* [29:24] reserved */
#define DMAC_DMCSR_SCOFF_BIT	8
#define DMAC_DMCSR_SCOFF_MASK	(0xffff << DMAC_DMCSR_SCOFF_BIT)
#define DMAC_DMCSR_BCH_DB	(1 << 7)  /* Block Syndrome of BCH Decoding */
#define DMAC_DMCSR_BCH_DF	(1 << 6)  /* BCH Decoding finished */ 
#define DMAC_DMCSR_BCH_EF	(1 << 5)  /* BCH Encoding finished */
#define DMAC_DMCSR_BTB_INV	(1 << 4)  /* Invalidates BTB in MCU */
#define DMAC_DMCSR_SC_CALL	(1 << 3)  /* SecurityCall */
#define DMAC_DMCSR_SW_RST	(1 << 0)  /* Software reset */
// MCU Interrupt
/* [31:18] reserved */
#define DMAC_DMINT_S_IP		(1 << 17) /* Security Mailbox IRQ pending */
#define DMAC_DMINT_N_IP		(1 << 16) /* Normal Mailbox IRQ pending */
/* [15:2] reserved */
#define DMAC_DMINT_S_IMSK	(1 << 1)  /* Security Mailbox IRQ mask */
#define DMAC_DMINT_N_IMSK	(1 << 0)  /* Normal Mailbox IRQ mask */

#ifndef __MIPS_ASSEMBLER


/***************************************************************************
 * DMAC
 ***************************************************************************/

#define __dmac_enable()		(REG_DMAC_DMACR |= DMAC_DMACR_DMAE)
#define __dmac_disable()	(REG_DMAC_DMACR &= ~DMAC_DMACR_DMAE)

/* m is the DMA controller index (0, 1), n is the DMA channel index (0 - 11) */

#define __dmac_enable_module(m)						\
	( REG_DMAC_DMACR |= DMAC_DMACR_DMAE | DMAC_DMACR_PR_012345 )
#define __dmac_disable_module(m)			\
	( REG_DMAC_DMACR &= ~DMAC_DMACR_DMAE )

/* p=0,1,2,3 */
#define __dmac_set_priority(m,p)					\
	do {								\
		REG_DMAC_DMACR &= ~DMAC_DMACR_PR_MASK;		\
		REG_DMAC_DMACR |= ((p) << DMAC_DMACR_PR_BIT);	\
	} while (0)

#define __dmac_test_halt_error(m) ( REG_DMAC_DMACR & DMAC_DMACR_HLT )
#define __dmac_test_addr_error(m) ( REG_DMAC_DMACR & DMAC_DMACR_AR )

#define __dmac_channel_enable_clk(n)			\
	REG_DMAC_DMACKE |= 1 << n;

#define __dmac_enable_descriptor(n)			\
	( REG_DMAC_DCCSR((n)) &= ~DMAC_DCCSR_NDES )
#define __dmac_disable_descriptor(n)			\
	( REG_DMAC_DCCSR((n)) |= DMAC_DCCSR_NDES )

#define __dmac_enable_channel(n)			\
	( REG_DMAC_DCCSR((n)) |= DMAC_DCCSR_EN )
#define __dmac_disable_channel(n)			\
	( REG_DMAC_DCCSR((n)) &= ~DMAC_DCCSR_EN )
#define __dmac_channel_enabled(n)			\
	( REG_DMAC_DCCSR((n)) & DMAC_DCCSR_EN )

#define __dmac_channel_irq_unmask()			\
	( REG_DMAC_DCIRQM = 0 )
#define __dmac_channel_enable_irq(n)			\
	( REG_DMAC_DCMD((n)) |= DMAC_DCMD_TIE )
#define __dmac_channel_disable_irq(n)			\
	( REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_TIE )

#define __dmac_channel_transmit_halt_detected(n)	\
	(  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_HLT )
#define __dmac_channel_transmit_end_detected(n)		\
	(  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_TT )
#define __dmac_channel_address_error_detected(n)	\
	(  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_AR )
#define __dmac_channel_count_terminated_detected(n)	\
	(  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_CT )
#define __dmac_channel_descriptor_invalid_detected(n)	\
	(  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_INV )

#define __dmac_channel_clear_transmit_halt(n)				\
	do {								\
		/* clear both channel halt error and globle halt error */ \
		REG_DMAC_DCCSR(n) &= ~DMAC_DCCSR_HLT;			\
		REG_DMAC_DMACR &= ~DMAC_DMACR_HLT;	\
	} while (0)
#define __dmac_channel_clear_transmit_end(n)		\
	(  REG_DMAC_DCCSR(n) &= ~DMAC_DCCSR_TT )
#define __dmac_channel_clear_address_error(n)				\
	do {								\
		REG_DMAC_DDA(n) = 0; /* clear descriptor address register */ \
		REG_DMAC_DSAR(n) = 0; /* clear source address register */ \
		REG_DMAC_DTAR(n) = 0; /* clear target address register */ \
		/* clear both channel addr error and globle address error */ \
		REG_DMAC_DCCSR(n) &= ~DMAC_DCCSR_AR;			\
		REG_DMAC_DMACR &= ~DMAC_DMACR_AR;	\
	} while (0)
#define __dmac_channel_clear_count_terminated(n)	\
	(  REG_DMAC_DCCSR((n)) &= ~DMAC_DCCSR_CT )
#define __dmac_channel_clear_descriptor_invalid(n)	\
	(  REG_DMAC_DCCSR((n)) &= ~DMAC_DCCSR_INV )

#define __dmac_channel_set_transfer_unit_32bit(n)		\
	do {							\
		REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
		REG_DMAC_DCMD((n)) |= DMAC_DCMD_DS_32BIT;	\
	} while (0)

#define __dmac_channel_set_transfer_unit_16bit(n)		\
	do {							\
		REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
		REG_DMAC_DCMD((n)) |= DMAC_DCMD_DS_16BIT;	\
	} while (0)

#define __dmac_channel_set_transfer_unit_8bit(n)		\
	do {							\
		REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
		REG_DMAC_DCMD((n)) |= DMAC_DCMD_DS_8BIT;	\
	} while (0)

#define __dmac_channel_set_transfer_unit_16byte(n)		\
	do {							\
		REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
		REG_DMAC_DCMD((n)) |= DMAC_DCMD_DS_16BYTE;	\
	} while (0)

#define __dmac_channel_set_transfer_unit_32byte(n)		\
	do {							\
		REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
		REG_DMAC_DCMD((n)) |= DMAC_DCMD_DS_32BYTE;	\
	} while (0)

/* w=8,16,32 */
#define __dmac_channel_set_dest_port_width(n,w)			\
	do {							\
		REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DWDH_MASK;	\
		REG_DMAC_DCMD((n)) |= DMAC_DCMD_DWDH_##w;	\
	} while (0)

/* w=8,16,32 */
#define __dmac_channel_set_src_port_width(n,w)			\
	do {							\
		REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_SWDH_MASK;	\
		REG_DMAC_DCMD((n)) |= DMAC_DCMD_SWDH_##w;	\
	} while (0)

/* v=0-15 */
#define __dmac_channel_set_rdil(n,v)				\
	do {							\
	REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_RDIL_MASK;		\
	REG_DMAC_DCMD((n)) |= ((v) << DMAC_DCMD_RDIL_BIT);	\
	} while (0)

#define __dmac_channel_dest_addr_fixed(n)		\
	(  REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DAI )
#define __dmac_channel_dest_addr_increment(n)		\
	(  REG_DMAC_DCMD((n)) |= DMAC_DCMD_DAI )

#define __dmac_channel_src_addr_fixed(n)		\
	(  REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_SAI )
#define __dmac_channel_src_addr_increment(n)		\
	(  REG_DMAC_DCMD((n)) |= DMAC_DCMD_SAI )

#define __dmac_channel_set_doorbell(n)					\
	(  REG_DMAC_DMADBSR = (1 << n) )

#define __dmac_channel_irq_detected(n)  ( REG_DMAC_DMAIPR & (1 << n) )
#define __dmac_channel_ack_irq(n)       ( REG_DMAC_DMAIPR &= ~(1 << n) )

/* Special Channel Contrl */
#define __dmac_channel_priv_irq_set(n)		(REG_DMAC_DCIRQM = ~(1 << (n)))
#define __dmac_channel_priv_irq_add(n)		(REG_DMAC_DCIRQM &= ~(1 << (n)))
#define __dmac_channel_priv_irq_clear()		(REG_DMAC_DCIRQM = 0)

#define __dmac_channel_mirq_check(n)		(REG_DMAC_DCIRQP & (1 << (n)))
#define __dmac_channel_mirq_clear(n) \
	(REG_DMAC_DCCSR(n) &= ~(DMAC_DCCSR_AR | DMAC_DCCSR_TT | DMAC_DCCSR_HLT | DMAC_DCCSR_EN))

#define __dmac_special_channel_enable()		(REG_DMAC_DMACR |= DMAC_DMACR_CH01)
#define __dmac_special_channel_disable()	(REG_DMAC_DMACR &= ~DMAC_DMACR_CH01)

#define __dmac_channel_programmable_set(n)	(REG_DMAC_DMACP |= 1 << (n))
#define __dmac_channel_programmable_clear(n)	(REG_DMAC_DMACP &= ~(1 << (n)))
#define __dmac_channel_programmable_default()	(REG_DMAC_DMACP = 0)

#define __dmac_channel_launch(n)		(REG_DMAC_DCCSR(n) |= DMAC_DCCSR_EN)
#define __dmac_special_channel_launch(n)	(REG_DMAC_DCCSR(n) |= DMAC_DCCSR_NDES | DMAC_DCCSR_EN)
#define __dmac_channel_halt(n)			(REG_DMAC_DCCSR(n) &= ~DMAC_DCCSR_EN)

/* MCU Control */
#define __mbch_encode_sync()		while (REG_DMAC_DMCSR & DMAC_DMCSR_BCH_EF)
#define __mbch_decode_sync()		while (REG_DMAC_DMCSR & DMAC_DMCSR_BCH_DF)
#define __mbch_decode_sdmf()		while (REG_DMAC_DMCSR & DMAC_DMCSR_BCH_DB)

#define __dmac_mcu_reset1()		(REG_DMAC_DMCSR |= DMAC_DMCSR_SW_RST)
#define __dmac_mcu_reset0()		(REG_DMAC_DMCSR &= ~DMAC_DMCSR_SW_RST)
#define __dmac_mcu_is_sleep()		(REG_DMAC_DMCSR & DMAC_DMCSR_SLEEP)

/* MCU MailBox */
#define __dmac_mmb_mask_irq(irq)	(REG_DMAC_DMINT |= (1 << (irq)))
#define __dmac_mmb_unmask_irq(irq)	(REG_DMAC_DMINT &= ~(1 << (irq)))
#define __dmac_mmb_ack_irq(irq)		(REG_DMAC_DMINT &= ~(1 << (irq + 16)))
#define __dmac_mmb_irq_clear()		(REG_DMAC_DMINT = ~(DMAC_DMINT_N_IP | DMAC_DMINT_S_IP))

#define __dmac_mnmb_mask()		(REG_DMAC_DMINT |= DMAC_DMINT_N_IMSK)
#define __dmac_mnmb_unmask()		(REG_DMAC_DMINT &= DMAC_DMINT_N_IMSK)
#define __dmac_mnmb_send(n)		(REG_DMAC_DMNMB = (n))
#define __dmac_mnmb_get(n)		((n) = REG_DMAC_DMNMB)
#define __dmac_mnmb_clear()		(REG_DMAC_DMNMB = 0)

#define __dmac_msmb_send(n)		(REG_DMAC_DMSMB = (n))
#define __dmac_msmb_clear()		(REG_DMAC_DMSMB = 0)
/*
static __inline__ int __dmac_get_irq(void)
{
	int i;
	for (i = 0; i < MAX_DMA_NUM; i++)
		if (__dmac_channel_irq_detected(i))
			return i;
	return -1;
}

static __inline__ int __mcu_get_irq(void)
{	
	int i;	
	for (i = 0; i < NUM_MCU; i++)	
		if (REG_DMAC_DMINT & (1 << (i + 16 )))
			return i;
	return -1;
}
*/
#endif /* __MIPS_ASSEMBLER */

#endif /* __JZ4780PDMAC_H__ */
