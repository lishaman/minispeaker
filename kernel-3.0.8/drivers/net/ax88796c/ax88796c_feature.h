#ifndef _AX88796C_FEATURE_H_
#define _AX88796C_FEATURE_H_

#ifndef TRUE
#define TRUE				1
#endif

#ifndef FALSE
#define FALSE				0
#endif

/*
 * Configuration options
 */
/* DMA mode only effected on SMDK2440 platform. */
#define TX_DMA_MODE			TRUE
#define RX_DMA_MODE			TRUE
#define AX88796B_PIN_COMPATIBLE		FALSE
#define AX88796C_8BIT_MODE		TRUE
#define DMA_BURST_LEN			DMA_BURST_LEN_8_WORD
#define REG_SHIFT			0x00

#endif /* _AX88796C_FEATURE_H_ */
