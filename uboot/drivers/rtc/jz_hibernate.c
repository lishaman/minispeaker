/*
 * (C) Copyright 2003
 * David Müller ELSOFT AG Switzerland. d.mueller@elsoft.ch
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Date & Time support for the built-in Samsung S3C24X0 RTC
 */
#include <common.h>
#include <command.h>

#include <rtc.h>
#include <asm/io.h>
#include <linux/compiler.h>
#include "jz47xx_rtc.h"

extern int axp173_power_off(void);

static unsigned int jzrtc_readl(int offset)
{
	unsigned int data, timeout = 0x100000;
	do {
		data = readl(RTC_BASE + offset);
	} while (readl(RTC_BASE + offset) != data && timeout--);
	if (timeout <= 0)
		printf("RTC rtc_read_reg timeout!\n");
	return data;
}

static inline void wait_write_ready(void)
{
	int timeout = 0x100000;
	while (!(jzrtc_readl(RTC_RTCCR) & RTCCR_WRDY) && timeout--);
	if (timeout <= 0)
		printf("RTC __wait_write_ready timeout!\n");
}

void jzrtc_writel(int offset, unsigned int value)
{
	int timeout = 0x100000;

	wait_write_ready();
	writel(WENR_WENPAT_WRITABLE, RTC_BASE + RTC_WENR);
	wait_write_ready();
	while (!(jzrtc_readl(RTC_WENR) & WENR_WEN) && timeout--);
	if (timeout <= 0) {
		while(1) {
			timeout = 0x100000;
			writel(WENR_WENPAT_WRITABLE, RTC_BASE + RTC_WENR);
			wait_write_ready();
			while (!(jzrtc_readl(RTC_WENR) & WENR_WEN) && timeout--);
			if (timeout > 0)
				break;
			else
				printf("RTC __wait_writable 0x%x timeout!\n", (jzrtc_readl(RTC_WENR)));
		}
	}

	writel(value,RTC_BASE + offset);
	wait_write_ready();
}

#define HWFCR_WAIT_TIME(x) ((x > 0x7fff ? 0x7fff: (0x7ff*(x)) / 2000) << 5)
#define HRCR_WAIT_TIME(x) ((((x) > 1875 ? 1875: (x)) / 125) << 11)

void jz_hibernate(void)
{
	/* Set minimum wakeup_n pin low-level assertion time for wakeup: 1000ms */
	jzrtc_writel(RTC_HWFCR, HWFCR_WAIT_TIME(1000));

	/* Set reset pin low-level assertion time after wakeup: must  > 60ms */
	jzrtc_writel(RTC_HRCR, HRCR_WAIT_TIME(125));

	/* clear wakeup status register */
	jzrtc_writel(RTC_HWRSR, 0x0);

	jzrtc_writel(RTC_HWCR, 0x9);

	/* Put CPU to hibernate mode */
	jzrtc_writel(RTC_HCR, 0x1);

	axp173_power_off();

        while(1)
		printf("%s:We should NOT come here.%08x\n",__func__, jzrtc_readl(RTC_HCR));
}
#if 0
void rtc_alarm_test(unsigned long alarm_seconds)
{
	unsigned int temp;	

	jzrtc_writel(RTC_RTCSAR, jzrtc_readl(RTC_RTCSR) + alarm_seconds);
        temp = jzrtc_readl(RTC_RTCCR);
        temp &= ~RTCCR_AF;
        temp |= RTCCR_AIE | RTCCR_AE;
        jzrtc_writel(RTC_RTCCR, temp);
}
#endif
