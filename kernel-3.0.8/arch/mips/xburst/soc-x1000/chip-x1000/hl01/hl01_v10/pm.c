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
	/* GPIO Group - A */
        {32*0+26,       GSS_OUTPUT_LOW  },      /* SFC_CLK */
        {32*0+27,       GSS_OUTPUT_HIGH },      /* SFC_CE */
        {32*0+28,       GSS_INPUT_PULL  },      /* SFC_DR */
        {32*0+29,       GSS_INPUT_PULL  },      /* SFC_DT */
        {32*0+30,       GSS_INPUT_PULL  },      /* SFC_WP */
        {32*0+31,       GSS_INPUT_PULL  },      /* SFC_HOLD */

	/* GPIO Group - B */
	{32*1+26,       GSS_IGNORE	},      /* CLK32K */

	/* GPIO Group - C */
	{32*2+0,       	GSS_OUTPUT_LOW  },      /* MSC1_CLK */
	{32*2+1,       	GSS_OUTPUT_HIGH },      /* MSC1_CMD */
	{32*2+2,       	GSS_OUTPUT_HIGH },      /* MSC1_D0 */
	{32*2+3,       	GSS_OUTPUT_HIGH },      /* MSC1_D1 */
	{32*2+4,       	GSS_OUTPUT_HIGH },      /* MSC1_D2 */
	{32*2+5,       	GSS_OUTPUT_HIGH },      /* MSC1_D3 */
	{32*2+6,       	GSS_OUTPUT_HIGH },      /* BT_PCM_CLK */
	{32*2+7,       	GSS_OUTPUT_HIGH },      /* BT_PCM_DO */
	{32*2+8,       	GSS_OUTPUT_HIGH },      /* BT_PCM_DI */
	{32*2+9,       	GSS_OUTPUT_HIGH },      /* BT_PCM_SYN */
	{32*2+10,       GSS_OUTPUT_HIGH },      /* UART0_RXD */
	{32*2+11,       GSS_OUTPUT_HIGH },      /* UART0_TXD III*/
	{32*2+12,       GSS_OUTPUT_HIGH },      /* UART0_CTS_N */
	{32*2+13,       GSS_OUTPUT_HIGH },      /* UART0_RTS_N III*/
	{32*2+16,       GSS_OUTPUT_HIGH },      /* WL_WAKE_HOST */
	{32*2+17,       GSS_IGNORE      },      /* WL_REG_EN */
	{32*2+18,       GSS_OUTPUT_LOW  },      /* BT_REG_EN III*/
	{32*2+19,       GSS_OUTPUT_HIGH },      /* HOST_WAKE_BT III*/
	{32*2+20,       GSS_OUTPUT_HIGH },      /* BT_W AKE_HOST */
	{32*2+31,       GSS_IGNORE      },      /* JTAG/UART2 */
	/* GPIO Group Set End */
	{GSS_TABLET_END,GSS_TABLET_END	}
};
