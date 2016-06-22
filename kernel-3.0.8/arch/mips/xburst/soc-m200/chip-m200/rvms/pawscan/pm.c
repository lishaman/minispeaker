/*
* Copyright (c) 2006-2010  Ingenic Semiconductor Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/
#include <gpio.h>
// default gpio state is input pull;
__initdata int gpio_ss_table[][2] = {
{32 * 0 +  0,	GSS_INPUT_PULL }, /* NC */
{32 * 0 +  1,	GSS_INPUT_PULL }, /* NC */
{32 * 0 +  2,	GSS_INPUT_PULL }, /* NC */
{32 * 0 +  3,	GSS_IGNORE },	  /* PMU_IRQ_N */
{32 * 0 +  4,	GSS_INPUT_PULL}, /* -MSC0 D4 */
{32 * 0 +  5,	GSS_INPUT_PULL}, /* -MSC0 D5 */
{32 * 0 +  6,	GSS_INPUT_PULL}, /* -MSC0 D6 */
{32 * 0 +  7,	GSS_INPUT_PULL}, /* -MSC0 D7 */
{32 * 0 +  8,	GSS_IGNORE },	  /* WL REG EN */
{32 * 0 +  9,	GSS_IGNORE },	  /* WL WAKE HOST */
{32 * 0 +  10,	GSS_INPUT_PULL },	  /* HOST WAKE WL */
{32 * 0 +  11,	GSS_INPUT_PULL },	  /* SSI CE N */
{32 * 0 +  12,	GSS_INPUT_PULL },	  /* -SMB1 SDA */
{32 * 0 +  13,	GSS_INPUT_PULL },	  /* -SMB1 SCK */
{32 * 0 +  14,	GSS_INPUT_PULL },	  /* -USB ID */
{32 * 0 +  15,	GSS_IGNORE },	  /* SENSOR INT */
{32 * 0 + 16,	GSS_INPUT_PULL	}, /* NC */
{32 * 0 + 17,	GSS_INPUT_PULL}, /* NC */
{32 * 0 + 18,	GSS_INPUT_PULL	}, /* MSC0 CLK */
{32 * 0 + 19,	GSS_INPUT_NOPULL	}, /* MSC0 CMD */
{32 * 0 + 20,	GSS_INPUT_PULL	}, /* -MSC0 D0 */
{32 * 0 + 21,	GSS_INPUT_PULL	}, /* -MSC0 D1 */
{32 * 0 + 22,	GSS_INPUT_PULL	}, /* -MSC0 D2 */
{32 * 0 + 23,	GSS_INPUT_PULL	}, /* -MSC0 D3 */
{32 * 0 + 24,	GSS_INPUT_PULL}, /* NC */
{32 * 0 + 27,	GSS_INPUT_PULL}, /* NC */
{32 * 0 + 29,	GSS_IGNORE}, /* MSC RST N */
{32 * 0 + 30,	GSS_IGNORE	}, /* WAKE_UP*/
{32 * 0 + 31,	GSS_INPUT_PULL	}, /* NC */
/* GPIO Group - B */
{32 * 1 +  0,	GSS_INPUT_NOPULL}, /* LCD DISP N */
{32 * 1 +  1,	GSS_IGNORE},	 /* SLEEP */
{32 * 1 +  2,	GSS_INPUT_PULL}, /* NC */
{32 * 1 +  3,	GSS_INPUT_PULL}, /* NC */
{32 * 1 +  4,	GSS_INPUT_PULL}, /* NC */
{32 * 1 +  5,	GSS_INPUT_PULL}, /* NC */
{32 * 1 +  7,	GSS_INPUT_NOPULL}, /* ISP SDA */
{32 * 1 +  8,	GSS_INPUT_NOPULL}, /* ISP SCK */
{32 * 1 + 20,	GSS_INPUT_PULL}, /* NC */
{32 * 1 + 21,	GSS_INPUT_PULL}, /* NC */
{32 * 1 + 28,	GSS_INPUT_PULL}, /* NC */
{32 * 1 + 29,	GSS_INPUT_PULL}, /* NC */
{32 * 1 + 30,	GSS_INPUT_PULL}, /* NC */
{32 * 1 + 31,	GSS_INPUT_PULL}, /* NC */
/* GPIO Group - C */
{32 * 2 +  0,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 +  1,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 +  2,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 +  3,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 +  4,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 +  5,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 +  6,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 +  7,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 +  8,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 +  9,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 10,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 11,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 12,	GSS_INPUT_PULL	}, /* RED LED */
{32 * 2 + 13,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 14,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 15,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 16,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 17,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 18,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 19,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 20,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 21,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 22,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 23,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 24,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 25,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 26,	GSS_INPUT_PULL	}, /* NC */
{32 * 2 + 27,	GSS_INPUT_PULL	}, /* NC */
/* GPIO Group - D */
{32 * 3 + 0,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 1,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 2,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 3,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 4,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 5,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 6,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 7,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 8,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 9,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 10,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 11,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 12,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 13,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 14,	GSS_INPUT_NOPULL}, /* CLK32 */
{32 * 3 + 15,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 17,	GSS_INPUT_PULL}, /* BOOT_SEL0/VOL- ???? */
{32 * 3 + 18,	GSS_INPUT_NOPULL}, /* BOOT_SEL1/VOL+ ???? */
{32 * 3 + 19,	GSS_INPUT_PULL}, /* BOOT_SEL2/RETURN ???? */
{32 * 3 + 20,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 21,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 22,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 23,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 24,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 25,	GSS_INPUT_PULL}, /* NC */
{32 * 3 + 26,	GSS_IGNORE}, /* UART1 RXD */
{32 * 3 + 27,	GSS_INPUT_PULL}, /* GREEN LED */
{32 * 3 + 28,	GSS_INPUT_PULL}, /* SSI CLK NC */
{32 * 3 + 29,	GSS_IGNORE}, /*UART1 TXD */
{32 * 3 + 30,	GSS_INPUT_NOPULL}, /* I2C0_SDA */
{32 * 3 + 31,	GSS_INPUT_NOPULL}, /* I2C0_SCK */
/* GPIO Group - E */
{32 * 4 +  0,	GSS_INPUT_PULL}, /* NC */
{32 * 4 +  1,	GSS_INPUT_PULL}, /* NC */
{32 * 4 +  2,	GSS_INPUT_NOPULL}, /* CIM MCLK */
{32 * 4 +  3,	GSS_INPUT_PULL}, /* NC */
{32 * 4 + 10,	GSS_OUTPUT_LOW}, /* DRVVBUS/SD0_WP_N*/
{32 * 4 + 20,	GSS_OUTPUT_LOW}, /* MSC0_D0 */
{32 * 4 + 21,	GSS_OUTPUT_LOW}, /* MSC0_D1_ */
{32 * 4 + 22,	GSS_OUTPUT_LOW}, /* MSC0_D2 */
{32 * 4 + 23,	GSS_OUTPUT_LOW}, /* MSC0_D3 */
{32 * 4 + 28,	GSS_OUTPUT_LOW}, /* MSC0_CLK */
{32 * 4 + 29,	GSS_OUTPUT_LOW}, /* MSC0_CMD */
{32 * 4 + 30,	GSS_INPUT_PULL}, /* NC */
{32 * 4 + 31,	GSS_INPUT_NOPULL}, /* USB DETE */
/* GPIO Group - F */
{32 * 5 +  0,	GSS_INPUT_PULL}, /* NC */
{32 * 5 +  1,	GSS_INPUT_PULL}, /* NC */
{32 * 5 +  2,	GSS_INPUT_PULL}, /* NC */
{32 * 5 +  3,	GSS_OUTPUT_LOW}, /* UART0_TXD */
{32 * 5 +  4,	GSS_INPUT_PULL}, /* NC */
{32 * 5 +  5,	GSS_INPUT_PULL}, /* NC */
{32 * 5 +  6,	GSS_INPUT_PULL}, /* NC */
{32 * 5 +  7,	GSS_OUTPUT_LOW}, /* CIM PWDN N */
{32 * 5 +  8,	GSS_INPUT_PULL}, /* NC */
{32 * 5 +  9,	GSS_INPUT_PULL}, /* NC */
{32 * 5 + 10,	GSS_INPUT_PULL}, /* NC */
{32 * 5 + 11,	GSS_INPUT_PULL}, /* NC */
{32 * 5 + 12,	GSS_INPUT_PULL}, /* NC */
{32 * 5 + 13,	GSS_INPUT_PULL}, /* NC */
{32 * 5 + 14,	GSS_INPUT_PULL}, /* NC */
{32 * 5 + 15,	GSS_INPUT_PULL}, /* NC */
/* GPIO Group Set End */
{GSS_TABLET_END,GSS_TABLET_END	}
};
