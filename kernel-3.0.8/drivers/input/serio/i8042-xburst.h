
//#include"/home/chaoli/mouse/linux-2.6.31.3/arch/mips/include/asm/mach-jz4770/jz4770gpio.h"
#include<soc/base.h>
#include<soc/irq.h>
#include<linux/clk.h>

#ifndef _JZ_KBC_H
#define _JZ_KBC_H

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

/*
 * variable.
 */

struct clk	*clk;
void __iomem	*iomem;

/*
 * Standard commands.
 */

#define I8042_CMD_CTL_RCTR	0x0120
#define I8042_CMD_CTL_WCTR	0x1060
#define I8042_CMD_CTL_TEST	0x01aa
#define I8042_CMD_CTL_REPORT	0x01e0
#define I8042_CMD_CTL_RINPUT	0x01c0

#define I8042_CMD_KBD_DISABLE	0x00ad
#define I8042_CMD_KBD_ENABLE	0x00ae
#define I8042_CMD_KBD_TEST	0x01ab
#define I8042_CMD_KBD_LOOP	0x11d2

#define I8042_CMD_AUX_DISABLE	0x00a7
#define I8042_CMD_AUX_ENABLE	0x00a8
#define I8042_CMD_AUX_TEST	0x01a9
#define I8042_CMD_AUX_SEND	0x10d4    //0x10d4
#define I8042_CMD_AUX_LOOP	0x11d3

int i8042_command(unsigned char *param, int command);

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

/*
 * Names.
 */

#define I8042_KBD_PHYS_DESC "isa0060/serio0"
#define I8042_AUX_PHYS_DESC "isa0060/serio1"
#define I8042_MUX_PHYS_DESC "isa0060/serio%d"

/*
 * IRQ.
 */

#define I8042_KBD_IRQ	IRQ_KBC
#define I8042_AUX_IRQ	IRQ_KBC

/*
 * Register numbers.
 */

#define I8042_DATA_REG	     0x60
#define I8042_COMMAND_REG    0x64

/*!!!
#define __gpio_as_bkc()                         \
do {                                            \
        SETREG32(GPIO_PXIMS(3), 0xf << 4);    \
        SETREG32(GPIO_PXFUNS(3), 0xf << 4);    \
        SETREG32(GPIO_PXSELC(3), 0xf << 4);     \
}while (0)

#define KBC_IRQ_ENABLE()			\
	SETREG32(INTC_ICMSR(IRQ_KBC / 32), 1 << (IRQ_KBC % 32));

#define KBC_CLK_ENABLE()			\
	CLRREG32(CPM_CLKGR0, CLKGR0_KBC);
!!!*/


static inline int i8042_read_data(void)
{
	return readb(iomem + I8042_DATA_REG);
}

static inline int i8042_read_status(void)
{
	return readb(iomem + I8042_COMMAND_REG);
}

static inline void i8042_write_data(int val)
{
	writeb(val, iomem + I8042_DATA_REG);
}

static inline void i8042_write_command(int val)
{
	writeb(val, iomem + I8042_COMMAND_REG);
}

static int i8042_platform_init(void)
{

/*
 *	__gpio_as_bkc();
 *	KBC_IRQ_ENABLE();
 *	KBC_CLK_ENABLE();
*/
	char clkname[] = "kbc";
	iomem = ioremap(KBC_IOBASE, 0x08);
	if(!iomem)
		goto err_ioremap;

	clk = clk_get(NULL, clkname);
	clk_enable(clk);

	return 0;
err_ioremap:
	return 1;
}

static int i8042_platform_exit(void)
{
	return 0;
}


#endif /* _JZ_KBC_H */

