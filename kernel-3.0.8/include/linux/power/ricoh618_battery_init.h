/*
 * include/linux/power/ricoh61x_battery_init.h
 *
 * Battery initial parameter for RICOH RN5T618/619 power management chip.
 *
 * Copyright (C) 2013 RICOH COMPANY,LTD
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
#ifndef __LINUX_POWER_RICOH61X_BATTERY_INIT_H
#define __LINUX_POWER_RICOH61X_BATTERY_INIT_H


uint8_t __attribute__((weak)) battery_init_para[32] = {
        0x0B, 0xD5, 0x0B, 0xF7, 0x0C, 0x09, 0x0C, 0x1A, 0x0C, 0x2F, 0x0C, 0x4B, 0x0C, 0x76, 0x0C, 0xA2,
        0x0C, 0xD1, 0x0D, 0x0A, 0x0D, 0x4D, 0x09, 0xC7, 0x00, 0x42, 0x0F, 0xC8, 0x05, 0x2A, 0x22, 0x56
};

#endif

/*
<Other Parameter>
nominal_capacity=2900
cut-off_v=3600
thermistor_b=3445
board_impe=0
bat_impe=0.1586
load_c=617
available_c=2504
battery_v=3697
MakingMode=Normal
ChargeV=4.20V
LoadMode=Resistor
*/
