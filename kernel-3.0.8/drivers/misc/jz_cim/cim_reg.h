#ifndef __REG_CIM_H__
#define __REG_CIM_H__

#define	CIM_CFG			(0x00)
#define	CIM_CTRL		(0x04)
#define	CIM_STATE		(0x08)
#define	CIM_IID			(0x0c)
#define	CIM_DA			(0x20)
#define	CIM_FA			(0x24)
#define	CIM_FID			(0x28)
#define	CIM_CMD			(0x2c)
#define	CIM_SIZE		(0x30)
#define	CIM_OFFSET		(0x34)
#define CIM_YFA			(0x38)
#define CIM_YCMD		(0x3c)
#define CIM_CBFA		(0x40)
#define CIM_CBCMD		(0x44)
#define CIM_CRFA		(0x48)
#define CIM_CRCMD		(0x4c)
#define CIM_CTRL2		(0x50)
#define CIM_FS			(0x54)
#define CIM_IMR			(0x58)
#define CIM_TC			(0x5c)
#define CIM_TINX		(0x60)
#define CIM_TCNT		(0x64)

/*CIM Configuration Register (CIMCFG)*/
#define CIM_CFG_SEP	(1<<20)
#define	CIM_CFG_ORDER		18

#define	CIM_CFG_ORDER_YUYV		(0 << 	CIM_CFG_ORDER)
#define	CIM_CFG_ORDER_YVYU		(1 << 	CIM_CFG_ORDER)
#define	CIM_CFG_ORDER_UYVY		(2 << 	CIM_CFG_ORDER)
#define	CIM_CFG_ORDER_VYUY		(3 << 	CIM_CFG_ORDER)

#define	CIM_CFG_DF_BIT		16
#define	CIM_CFG_DF_MASK		  (0x3 << CIM_CFG_DF_BIT)
  #define CIM_CFG_DF_YUV444	  (0x1 << CIM_CFG_DF_BIT) 	/* YCbCr444 */
  #define CIM_CFG_DF_YUV422	  (0x2 << CIM_CFG_DF_BIT)	/* YCbCr422 */
  #define CIM_CFG_DF_ITU656	  (0x3 << CIM_CFG_DF_BIT)	/* ITU656 YCbCr422 */
#define	CIM_CFG_VSP			(1 << 14) /* VSYNC Polarity:0-rising edge active,1-falling edge active */
#define	CIM_CFG_HSP			(1 << 13) /* HSYNC Polarity:0-rising edge active,1-falling edge active */
#define	CIM_CFG_PCP			(1 << 12) /* PCLK working edge: 0-rising, 1-falling */
#define	CIM_CFG_DMA_BURST_TYPE_BIT	10
#define	CIM_CFG_DMA_BURST_TYPE_MASK	(0x3 << CIM_CFG_DMA_BURST_TYPE_BIT)
  #define	CIM_CFG_DMA_BURST_INCR4		(0 << CIM_CFG_DMA_BURST_TYPE_BIT)
  #define	CIM_CFG_DMA_BURST_INCR8		(1 << CIM_CFG_DMA_BURST_TYPE_BIT)	/* Suggested */
  #define	CIM_CFG_DMA_BURST_INCR16	(2 << CIM_CFG_DMA_BURST_TYPE_BIT)	/* Suggested High speed AHB*/
  #define	CIM_CFG_DMA_BURST_INCR32	(3 << CIM_CFG_DMA_BURST_TYPE_BIT)	/* Suggested High speed AHB*/
#define	CIM_CFG_PACK_BIT	4
#define	CIM_CFG_PACK_MASK	(0x7 << CIM_CFG_PACK_BIT)
  #define CIM_CFG_PACK_Y0UY1V	  (0 << CIM_CFG_PACK_BIT) 
  #define CIM_CFG_PACK_UY1VY0	  (1 << CIM_CFG_PACK_BIT) 
  #define CIM_CFG_PACK_Y1VY0U	  (2 << CIM_CFG_PACK_BIT) 
  #define CIM_CFG_PACK_VY0UY1	  (3 << CIM_CFG_PACK_BIT) 
  #define CIM_CFG_PACK_VY1UY0	  (4 << CIM_CFG_PACK_BIT) 
  #define CIM_CFG_PACK_Y1UY0V	  (5 << CIM_CFG_PACK_BIT) 
  #define CIM_CFG_PACK_UY0VY1	  (6 << CIM_CFG_PACK_BIT) 
  #define CIM_CFG_PACK_Y0VY1U	  (7 << CIM_CFG_PACK_BIT) 
#define	CIM_CFG_DSM_BIT		0
#define	CIM_CFG_DSM_MASK	(0x3 << CIM_CFG_DSM_BIT)
  #define CIM_CFG_DSM_CPM	  (0 << CIM_CFG_DSM_BIT) /* CCIR656 Progressive Mode */
  #define CIM_CFG_DSM_CIM	  (1 << CIM_CFG_DSM_BIT) /* CCIR656 Interlace Mode */
  #define CIM_CFG_DSM_GCM	  (2 << CIM_CFG_DSM_BIT) /* Gated Clock Mode */

#define	CIM_STATE_CR_RXF_OF	(1 << 21) /* Y RXFIFO over flow irq */
#define	CIM_STATE_CB_RXF_OF	(1 << 19) /* Y RXFIFO over flow irq */
#define	CIM_STATE_Y_RXF_OF	(1 << 17) /* Y RXFIFO over flow irq */
/* CIM State Register  (CIM_STATE) */
#define	CIM_STATE_DMA_EEOF	(1 << 11) /* DMA Line EEOf irq */
#define	CIM_STATE_DMA_STOP	(1 << 10) /* DMA stop irq */
#define	CIM_STATE_DMA_SOF	(1 << 8) /* DMA start irq */
#define	CIM_STATE_DMA_EOF	(1 << 9) /* DMA end irq */
#define	CIM_STATE_TLB_ERR	(1 << 4) /* TLB error */
#define	CIM_STATE_SIZE_ERR	(1 << 4) /* Frame size check error */
#define	CIM_STATE_RXF_OF	(1 << 2) /* RXFIFO over flow irq */
#define	CIM_STATE_RXF_EMPTY	(1 << 1) /* RXFIFO empty irq */
#define	CIM_STATE_VDD		(1 << 0) /* CIM disabled irq */

/* CIM DMA Command Register (CIM_CMD) */
#define	CIM_CMD_SOFINT		(1 << 31) /* enable DMA start irq */
#define	CIM_CMD_EOFINT		(1 << 30) /* enable DMA end irq */
#define	CIM_CMD_EEOFINT		(1 << 29) /* enable DMA EEOF irq */
#define	CIM_CMD_STOP		(1 << 28) /* enable DMA stop irq */
#define CIM_CMD_OFRCV           (1 << 27) 

/*CIM Control Register (CIMCR)*/
#define	CIM_CTRL_FRC_BIT	16
#define	CIM_CTRL_FRC_MASK	(0xf << CIM_CTRL_FRC_BIT)
#define CIM_CTRL_FRC_1	        (0x0 << CIM_CTRL_FRC_BIT) /* Sample every frame */
#define CIM_CTRL_FRC_10         (9 << CIM_CTRL_FRC_BIT)

#define CIM_CTRL_MBEN           (1 << 6)    /* 16x16 yuv420  macro blocks */
#define	CIM_CTRL_DMA_SYNC	(1 << 7)	/*when change DA, do frame sync */

#define CIM_CTRL_CIM_RST	(1 << 3)
#define	CIM_CTRL_DMA_EN		(1 << 2) /* Enable DMA */
#define	CIM_CTRL_RXF_RST	(1 << 1) /* RxFIFO reset */
#define	CIM_CTRL_ENA		(1 << 0) /* Enable CIM */

/* cim control2 */
#define CIM_CTRL2_FRAGHE	(1 << 25)	/* horizontal size remainder ingore */
#define CIM_CTRL2_FRAGVE	(1 << 24)	/* vertical size remainder ingore */
#define CIM_CTRL2_FSC		(1 << 23)	/* enable frame size check */
#define CIM_CTRL2_ARIF		(1 << 22)	/* enable auto-recovery for incomplete frame */
#define CIM_CTRL2_HRS_BIT	20		/* horizontal ratio for down scale */
#define CIM_CTRL2_HRS_MASK	(0x3 << CIM_CTRL2_HRS_BIT)
#define CIM_CTRL2_HRS_0		(0x0 << CIM_CTRL2_HRS_BIT)	/* no scale */
#define CIM_CTRL2_HRS_1		(0x1 << CIM_CTRL2_HRS_BIT)	/* 1/2 down scale */
#define CIM_CTRL2_HRS_2		(0x2 << CIM_CTRL2_HRS_BIT)	/* 1/4 down scale */
#define CIM_CTRL2_HRS_3		(0x3 << CIM_CTRL2_HRS_BIT)	/* 1/8 down scale */
#define CIM_CTRL2_VRS_BIT	18		/* vertical ratio for down scale */
#define CIM_CTRL2_VRS_MASK	(0x3 << CIM_CTRL2_VRS_BIT)
#define CIM_CTRL2_VRS_0		(0x0 << CIM_CTRL2_VRS_BIT)	/* no scale */
#define CIM_CTRL2_VRS_1		(0x1 << CIM_CTRL2_VRS_BIT)	/* 1/2 down scale */
#define CIM_CTRL2_VRS_2		(0x2 << CIM_CTRL2_VRS_BIT)	/* 1/4 down scale */
#define CIM_CTRL2_VRS_3		(0x3 << CIM_CTRL2_VRS_BIT)	/* 1/8 down scale */
#define CIM_CTRL2_CSC_BIT	16		/* CSC Mode Select */
#define CIM_CTRL2_CSC_MASK	(0x3 << CIM_CTRL2_CSC_BIT)
#define CIM_CTRL2_CSC_BYPASS	(0x0 << CIM_CTRL2_CSC_BIT)	/* Bypass mode */
#define CIM_CTRL2_CSC_YUV422	(0x2 << CIM_CTRL2_CSC_BIT)	/* CSC to YCbCr422 */
#define CIM_CTRL2_CSC_YUV420	(0x3 << CIM_CTRL2_CSC_BIT)	/* CSC to YCbCr420 */
#define CIM_CTRL2_OPG_BIT	4		/* option priority configuration */
#define CIM_CTRL2_OPG_MASK	(0x3 << CIM_CTRL2_OPG_BIT)
#define CIM_CTRL2_OPE		(1 << 2)	/* optional priority mode enable */
#define CIM_CTRL2_EME		(1 << 1)	/* emergency priority enable */
#define CIM_CTRL2_APM		(1 << 0)	/* auto priority mode enable*/


/*CIM Interrupt Mask Register (CIMIMR)*/
#define CIM_IMR_EOFM		(1<<9)
#define CIM_IMR_SOFM		(1<<8)
#define CIM_IMR_TLBEM		(1<<4)
#define CIM_IMR_FSEM		(1<<3)
#define CIM_IMR_RFIFO_OFM		(1<<2)

/* CIM Frame Size Register (CIM_FS) */
#define CIM_FS_FVS_BIT		16	/* vertical size of the frame */
#define CIM_FS_FVS_MASK		(0x1fff << CIM_FS_FVS_BIT)
#define CIM_FS_BPP_BIT		14	/* bytes per pixel */
#define CIM_FS_BPP_MASK		(0x3 << CIM_FS_BPP_BIT)
#define CIM_FS_FHS_BIT		0	/* horizontal size of the frame */
#define CIM_FS_FHS_MASK		(0x1fff << CIM_FS_FHS_BIT)

/* CIM TLB Control Register (CIMTC) */
#define CIM_TC_RBA			(1 << 2)
#define CIM_TC_RST			(1 << 1)
#define CIM_TC_ENA			(1 << 0)

#endif
