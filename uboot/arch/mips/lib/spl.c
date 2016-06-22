/*
 * MIPS SPL support
 *
 * Copyright (c) 2013 Imagination Technologies
 * Author: Paul Burton <paul.burton@imgtec.com>
 *
 * Based on arch/arm/lib/spl.c
 * (C) Copyright 2010-2012 Texas Instruments, <www.ti.com>
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
#include <common.h>
#include <config.h>
#include <spl.h>
#include <image.h>
#include <linux/compiler.h>
#include <boot_img.h>

extern void sfc_nor_load(unsigned int src_addr, unsigned int count,unsigned int dst_addr);
/* Pointer to as well as the global data structure for SPL */
DECLARE_GLOBAL_DATA_PTR;
__weak gd_t gdata __attribute__ ((section(".data")));

/*
 * In the context of SPL, board_init_f must ensure that any clocks/etc for
 * DDR are enabled, ensure that the stack pointer is valid, clear the BSS
 * and call board_init_r.  We provide this version by default but mark it
 * as __weak to allow for platforms to do this in their own way if needed.
 */
void __weak board_init_f(ulong dummy)
{
	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - (ulong)__bss_start);

	/* Set global data pointer. */
	gd = &gdata;

	board_init_r(NULL, 0);
}

/*
 * This function jumps to an image with argument. Normally an FDT or ATAGS
 * image.
 * arg: Pointer to paramter image in RAM
 */
#ifdef CONFIG_SPL_OS_BOOT
int __weak cleanup_before_linux (void)
{
}

void __noreturn jump_to_image_linux(void *arg)
{
	debug("Entering kernel arg pointer: 0x%p\n", arg);
#if defined (CONFIG_GET_WIFI_MAC)
	char *wifi_mac_str = NULL;
	unsigned int mac_addr[4] = {};
#endif
#ifdef CONFIG_GET_BAT_PARAM
	char *bat_param_str = NULL;
	unsigned char *bat_str = "4400";
	unsigned char buf[3];
#endif
	static u32 *param_addr = NULL;
	typedef void (*image_entry_arg_t)(int, char **, void *)
		__attribute__ ((noreturn));

	debug("Entering kernel arg pointer: 0x%p\n", arg);
	image_entry_arg_t image_entry =
		(image_entry_arg_t) spl_image.entry_point;

#if defined (CONFIG_GET_WIFI_MAC)
	memset(mac_addr, 0 , sizeof(mac_addr));
	sfc_nor_load(WIFI_MAC_READ_ADDR, WIFI_MAC_READ_COUNT, mac_addr);
	wifi_mac_str = strstr(arg, "wifi_mac");
	if (wifi_mac_str != NULL)
		memcpy(wifi_mac_str + 9, mac_addr, WIFI_MAC_READ_COUNT);
#endif
#ifdef CONFIG_GET_BAT_PARAM
	sfc_nor_load(BAT_PARAM_READ_ADDR, BAT_PARAM_READ_COUNT, buf);
	bat_param_str = strstr(arg, "bat");
	/* [0x69, 0xaa, 0x55] new battery's flag in nv */
	if((bat_param_str != NULL) && (buf[0] == 0x69) && (buf[1] == 0xaa)
			&& (buf[2] ==0x55))
		memcpy(bat_param_str + 4, bat_str, 4);
#endif
	cleanup_before_linux();
	param_addr = (u32 *)CONFIG_PARAM_BASE;
	param_addr[0] = 0;
	param_addr[1] = arg;
	flush_cache_all();
	image_entry(2, (char **)param_addr, NULL);
}
#endif
