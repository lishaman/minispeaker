/*
 * include/linux/mfd/ricoh619.h
 *
 * Core driver interface to access RICOH R5T619 power management chip.
 *
 * Copyright (C) 2012-2014 RICOH COMPANY,LTD
 *
 * Based on code
 *	Copyright (C) 2011 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef __AXP173_H
#define __AXP173_H

#define   POWER_STATUS			(0x00)
#define   POWER_MODE_CHGSTATUS		(0x01)
#define   POWER_ON_OFF_REG		(0x12)
#define	  AXP173_POWER_OFF		(0x32)
#define   POWER_CHARGE1			(0x33)
/* adc data register */
#define   POWER_ACIN_VOL_H8		(0x56)
#define   POWER_ACIN_VOL_L4		(0x57)
#define   POWER_ACIN_CUR_H8		(0x58)
#define   POWER_ACIN_CUR_L4		(0x59)
#define   POWER_VBUS_VOL_H8		(0x5A)
#define   POWER_VBUS_VOL_L4		(0x5B)
#define   POWER_VBUS_CUR_H8		(0x5C)
#define   POWER_VBUS_CUR_L4		(0x5D)
#define   POWER_INT_TEMP_H8		(0x5E)
#define   POWER_INT_TEMP_L4		(0x5F)
#define   POWER_TS_VOL_H8		(0x62)
#define   POWER_TS_VOL_L4		(0x63)
#define   POWER_BAT_AVERVOL_H8		(0x78)
#define   POWER_BAT_AVERVOL_L4		(0x79)
#define   POWER_BAT_AVERCHGCUR_H8	(0x7A)
#define   POWER_BAT_AVERCHGCUR_L5	(0x7B)
#define   POWER_BAT_AVERDISCHGCUR_H8	(0x7C)
#define   POWER_BAT_AVERDISCHGCUR_L5	(0x7D)

#define CHARGED_STATUS			(1<<6)
#define AC_AVAILABLE			(1<<6)
#define USB_AVAILABLE			(1<<4)

struct ocv2soc {
        int vol;
        int cpt;
};
#endif
