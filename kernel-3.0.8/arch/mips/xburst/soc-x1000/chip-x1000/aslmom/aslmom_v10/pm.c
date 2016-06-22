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
	{32*0+20,       GSS_OUTPUT_LOW  },      /* MSC0_D3 */
        {32*0+21,       GSS_OUTPUT_LOW  },      /* MSC0_D2 */
        {32*0+22,       GSS_OUTPUT_LOW  },      /* MSC0_D1 */
        {32*0+23,       GSS_OUTPUT_LOW  },      /* MSC0_D0 */
        {32*0+24,       GSS_OUTPUT_LOW  },      /* MSC0_CLK */
        {32*0+25,       GSS_OUTPUT_LOW  },      /* MSC0_CMD */

	{32*2+17,       GSS_IGNORE      },      /* WL_REG_EN */
	{32*2+24,       GSS_OUTPUT_LOW  },      /* WL_REG_EN */
	{32*2+31,       GSS_IGNORE      },      /* JTAG/UART2 */
	/* GPIO Group Set End */
	{GSS_TABLET_END,GSS_TABLET_END	}
};
