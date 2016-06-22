/* include/linux/sc8800s.h
 *
 *  Copyright (C) 2010 Ingenic Semiconductor, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _LINUX_SC8800S_H
#define _LINUX_SC8800S_H

/* message display level. */

#define	LEVEL_NOTHING	0 /* show noting */
#define	LEVEL_DEMAND	0 /* show needed but not error. */
#define	LEVEL_ERROR		1 /* error messgae */
#define	LEVEL_NOTICE	2 /* notice message and above */
#define	LEVEL_MSG		3 /* info and above */
#define	LEVEL_DEBUG		4 /* all */


#define	DISPLAY_LEVEL	LEVEL_ERROR

#define	spi_printk(level, format, arg...) \
	printk(level format, ##arg)

#define	spi_null(level, format, arg...) \
     do { } while(0)


/* spi_debug */
#if (DISPLAY_LEVEL >= LEVEL_DEBUG)
#define	spi_debug(format, arg...) \
	spi_printk("", format, ##arg)
#else
#define	spi_debug(format, arg...) \
	spi_null(KERN_INFO, format, ##arg)
#endif

/* spi_msg */
#if (DISPLAY_LEVEL >= LEVEL_MSG)
#define	spi_msg(format, arg...) \
	spi_printk(KERN_INFO, format, ##arg)
#else
#define	spi_msg(format, arg...) \
	spi_null(KERN_INFO, format, ##arg)
#endif

/* spi_notice */
#if (DISPLAY_LEVEL >= LEVEL_NOTICE) 
#define	spi_notice(format, arg...) \
	spi_printk(KERN_NOTICE, format, ##arg)
#else
#define	spi_notice(format, arg...) \
	spi_null(KERN_NOTICE, format, ##arg)
#endif

/* spi_error */
#if (DISPLAY_LEVEL >= LEVEL_ERROR)
#define	spi_error(format, arg...) \
do { \
	printk(KERN_ERR "%s:" format, "SPI_ERROR", ##arg); \
} while (0)
#else
#define	spi_error(format, arg...) \
	spi_null(KERN_ERR, format, ##arg)
#endif

/* spi_demand */
#if (DISPLAY_LEVEL >= LEVEL_DEMAND) 
#define	spi_demand(format, arg...) \
	printk(KERN_CRIT "%s:" format, "SPI_DEMAND", ##arg) 
#else
#define	spi_demand(format, arg...) \
	spi_null(KERN_CRIT, format, ##arg)
#endif

#if 0
/* spi_error */
#if (DISPLAY_LEVEL >= LEVEL_ERROR) 
#define	spi_error(format, arg...) \
	spi_printk(KERN_ERR, format, ##arg) 
#else
#define	spi_error(format, arg...) \
	spi_null(KERN_ERR, format, ##arg)
#endif

/* spi_demand */
#if (DISPLAY_LEVEL >= LEVEL_DEMAND) 
#define	spi_demand(format, arg...) \
	spi_printk(KERN_CRIT, format, ##arg) 
#else
#define	spi_demand(format, arg...) \
	spi_null(KERN_CRIT, format, ##arg)
#endif
#endif 

#if 0
#define	spi_error(format, arg...) \
if (DISPLAY_LEVEL >= LEVEL_ERROR) { \
	printk(KERN_ERR format, ##arg); \
} 

#define	spi_demand(format, arg...) \
if (DISPLAY_LEVEL >= LEVEL_DEMAND) { \
	printk(KERN_ERR format, ##arg); \
} 
#endif

struct sc8800s_platform_data {
	int td_rts_irq;
	int td_rts_pin;
	int td_rdy_irq;
	int td_rdy_pin;

	int ap_bb_spi_rts;
	int ap_bb_spi_rdy;
	int power_on;

};

#endif /* _LINUX_SC8800S_H */

