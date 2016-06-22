/*
 *Ingenic mensa boot android system command
 *
 * Copyright (c) 2013 Imagination Technologies
 * Author: Martin <czhu@ingenic.cn>
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

#include <stdarg.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <errno.h>
#include <div64.h>
#include <common.h>
#include <command.h>
#include <config.h>
#include <mmc.h>
#include <boot_img.h>
#include <fs.h>
#include <fat.h>

#ifdef CONFIG_ASLMOM_BOARD
#include <asm/gpio.h>
#include <asm/arch/lcdc.h>
#include <asm/arch/rtc.h>
#include <asm/io.h>
#endif
extern void flush_cache_all(void);

/*boot.img has been in memory already. just call init_boot_linux() and jump to kernel.*/
static void bootx_jump_kernel(unsigned long mem_address)
{
	static u32 *param_addr = NULL;
	typedef void (*image_entry_arg_t)(int, char **, void *)
		__attribute__ ((noreturn));

#ifdef CONFIG_ASLMOM_BOARD
	unsigned int update_flag;
	update_flag = get_update_flag();
#ifdef CONFIG_GET_BAT_PARAM
        char *bat_param_str = NULL;
        unsigned char *bat_str = "4400";
        unsigned char buf[3];
#endif
#endif
	image_entry_arg_t image_entry =
		(image_entry_arg_t) mem_address;

	printf("Prepare kernel parameters ...\n");
	param_addr = (u32 *)CONFIG_PARAM_BASE;
	param_addr[0] = 0;
#ifdef CONFIG_ASLMOM_BOARD
	if((update_flag & 0x3) != 0x3) {
#ifdef CONFIG_GET_BAT_PARAM
		sfc_nor_read(BAT_PARAM_READ_ADDR, BAT_PARAM_READ_COUNT, buf);
		bat_param_str = strstr(CONFIG_SPL_BOOTARGS, "bat");
		/* [0x69, 0xaa, 0x55] new battery's flag in nv */
		if((bat_param_str != NULL) && (buf[0] == 0x69) && (buf[1] == 0xaa)
				&& (buf[2] ==0x55))
			memcpy(bat_param_str + 4, bat_str, 4);
#endif
		param_addr[1] = CONFIG_SPL_BOOTARGS;
	}
	else
		param_addr[1] = CONFIG_BOOTX_BOOTARGS;
#else
	param_addr[1] = CONFIG_BOOTX_BOOTARGS;
#endif
	printf("param_addr[1] is %x\n",param_addr[1]);
	flush_cache_all();
	image_entry(2, (char **)param_addr, NULL);
	printf("We should not come here ... \n");
}

/* boot the android system form the memory directly.*/
static int mem_bootx(unsigned int mem_address)
{
	printf("Enter mem_boot routine ...\n");
	bootx_jump_kernel(mem_address);
	return 0;
}

#ifdef CONFIG_JZ_SPI
static void spi_boot(unsigned int mem_address,unsigned int spi_addr)
{
	struct image_header *header;
	unsigned int header_size;
	unsigned int entry_point, load_addr, size;

	printf("Enter SPI_boot routine ...\n");
	header_size = sizeof(struct image_header);
	spi_load(spi_addr, header_size, CONFIG_SYS_TEXT_BASE);
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

	entry_point = image_get_load(header);
	/* Load including the header */
	load_addr = entry_point - header_size;
	size = image_get_data_size(header) + header_size;

	spi_load(spi_addr, size, load_addr);

	bootx_jump_kernel(mem_address);
}
#endif

#ifdef CONFIG_JZ_SFC
#ifdef CONFIG_ASLMOM_BOARD
int bat_cap = -1;
int first = 1;
int line;

int get_line_count()
{
	if(bat_cap <= 10)
		line = bat_cap;
	else if (bat_cap > 10 && bat_cap <= 90)
		line = (bat_cap - 10) / 5 + bat_cap;
	else if (bat_cap > 90 && bat_cap <= 99)
		line = 107 + (bat_cap - 90);
	return line;
}

void display_battery_capacity(int line)
{
	int i;
	for(i = 1;i <= line; i++){
		if(!gpio_get_value(63) ||(gpio_get_value(40)))
			return;
		lcd_display_bat_line(i,0xff00);
		lcd_sync();
		mdelay(55);
	}
}
#endif
static void sfc_boot(unsigned int mem_address,unsigned int sfc_addr)
{
	struct image_header *header;
	unsigned int header_size;
	unsigned int entry_point, load_addr, size;

#ifdef CONFIG_ASLMOM_BOARD
	unsigned int update_flag;
	int hspr = readl(RTC_BASE + RTC_HSPR);

	disable_ldo4();
	/* Low power does not boot */
	if ((gpio_get_value(40)) && (hspr != 0x50574f46) && (low_power_detect())) {
		lcd_enable();
		lcd_display_bat_cap_first(0);
		mdelay(2000);
		lcd_disable();
		jz_hibernate();
	}
	gpio_port_direction_input(1,31);
	gpio_port_direction_input(1,8);
	update_flag = get_update_flag();
	if((update_flag & 0x03) != 0x03){
		while (gpio_get_value(63) && (!(gpio_get_value(40)))) {
			if (bat_cap != get_battery_current_cpt()) {
				bat_cap = get_battery_current_cpt();
				line = get_line_count();
			}

			if (first && bat_cap == 100) {
				lcd_enable();
				lcd_display_bat_cap_first(100);
				mdelay(100);
				lcd_disable();
			} else {
				lcd_enable();
				lcd_display_zero_cap();
				mdelay(100);
				if (line < 5)
					line = 5;
				display_battery_capacity(line);
				mdelay(100);
				lcd_disable();
				if (bat_cap == 100)
					first = 1;
				else
					first = 0;
			}
		}
		if(gpio_get_value(40)){

			printf("usb have remove ,power off!!!\n");
			//call axp173 power off
			jz_hibernate();
		}
	}
#endif
	printf("Enter SFC_boot routine ...\n");
	header_size = sizeof(struct image_header);
	sfc_nor_read(sfc_addr, header_size, CONFIG_SYS_TEXT_BASE);
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

	entry_point = image_get_load(header);
	/* Load including the header */
	load_addr = entry_point - header_size;
	size = image_get_data_size(header) + header_size;

	sfc_nor_read(sfc_addr, size, load_addr);
#ifdef CONFIG_ASLMOM_BOARD
	panel_power_off();
#endif
	bootx_jump_kernel(mem_address);
}
#endif

static int bootx_fs_boot(
	ulong mem_address,
	char *load_file,
	char *if_name,
	char *dev_par_str,
	char *fs_type_str)
{
	char *dev_par_v[3] = {NULL, NULL, NULL};
	char par_zero[8];
	char *par_info;
	int fs_type;
	ulong load_addr;
	int load_offset;
	unsigned long time;
	int len_read;
	int i;

	dev_par_v[0] = dev_par_str;
	memset(par_zero, 0, sizeof(par_zero));

	par_info = strchr(dev_par_str, ':');
	if (par_info && !strcasecmp(par_info + 1, "auto")) {
		if (par_info - dev_par_str > 3) {
			printf("Bootx fs invalid par info: %s\n", dev_par_str);
			return -1;
		}

		strncpy(par_zero, dev_par_str, par_info - dev_par_str);
		strcat(par_zero, ":0");

		dev_par_v[1] = dev_par_v[0];	/* "0:auto" first valid partition */
		dev_par_v[0] = par_zero;	/* "0:0" None partition */
	}

	if (!strcmp(fs_type_str, "fat")) {
		fs_type		= FS_TYPE_FAT;
		load_addr	= mem_address;
		load_offset	= sizeof(struct image_header);
	} else if (!strcmp(fs_type_str, "ext4")) {
		fs_type		= FS_TYPE_EXT;
		load_addr	= mem_address - sizeof(struct image_header);
		load_offset	= 0;	/* Ext read not support offset */
	} else {
		fs_type		= FS_TYPE_ANY;
		load_addr	= mem_address - sizeof(struct image_header);
		load_offset	= 0;
	}

	for (i = 0, len_read = -1; dev_par_v[i]; i++) {
		/* Get fs block device */
		if (fs_set_blk_dev(if_name, dev_par_v[i], fs_type) < 0)
			continue;

		printf("%s read %s partition\n", fs_type_str, dev_par_v[i]);

		/* Read bootx image from filesystem */
		time = get_timer(0);
		len_read = fs_read(load_file, load_addr, load_offset, 0);
		time = get_timer(time);
		if (len_read >= 0)
			break;
	}

	if (len_read < 0) {
		printf("Bootx %s-%s read failed\n", if_name, fs_type_str);
		return -1;
	}

	printf("%d bytes read in %lu ms", len_read, time);
	if (time > 0) {
		puts(" (");
		print_size(len_read / time * 1000, "/s");
		puts(")");
	}
	puts("\n");

	setenv_hex("filesize", len_read);

	/* Start boot kernel */
	bootx_jump_kernel(mem_address);

	/* Can not get there */
	return 0;
}

static int do_bootx(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long mem_address, sfc_addr;
#ifdef CONFIG_ASLMOM_BOARD
	unsigned int update_flag;
#endif
	argc--; argv++;
#ifdef CONFIG_ASLMOM_BOARD
	update_flag = get_update_flag();
	if((update_flag & 0x3) != 0x3)	{
		strcpy(argv[0],"sfc");
		strcpy(argv[1],argv[3]);
	}

#endif
	printf("argv[0]: %s,argv[1]:%s\n",argv[0],argv[1]);
	/* Consume 'boota' */
	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp("mem",argv[0])) {
		mem_address=simple_strtoul(argv[1], NULL, 16);
		printf("mem boot start\n");
		mem_bootx(mem_address);
		printf("mem boot error\n");
	} else if (!strcmp("sfc",argv[0])) {
		mem_address = simple_strtoul(argv[1], NULL, 16);
#ifdef CONFIG_ASLMOM_BOARD
		if((update_flag & 0x3) != 0x3)
			sfc_addr = 0x100000;
		else
			sfc_addr = simple_strtoul(argv[2], NULL, 16);
#else
		sfc_addr = simple_strtoul(argv[2], NULL, 16);
#endif
		printf("SFC boot start\n");
#ifdef CONFIG_JZ_SFC
		sfc_boot(mem_address, sfc_addr);
#endif
		printf("SFC boot error\n");
		return 0;
	} else if (!strcmp("spi",argv[0])) {
		mem_address = simple_strtoul(argv[1], NULL, 16);
		sfc_addr = simple_strtoul(argv[2], NULL, 16);
		printf("SPI boot start\n");
#ifdef CONFIG_JZ_SPI
		spi_boot(mem_address, sfc_addr);
#endif
		printf("SPI boot error\n");
	} else if (!strcmp("fat", argv[0]) || !strcmp("ext4", argv[0])) {
		if (argc < 5)
			return CMD_RET_USAGE;

		mem_address = simple_strtoul(argv[3], NULL, 16);
		printf("%s boot start\n", argv[0]);
		if (bootx_fs_boot(mem_address, argv[4], argv[1], argv[2], argv[0]) < 0)
			return CMD_RET_FAILURE;

		printf("%s boot error\n", argv[0]);
	} else {
		printf("%s boot unsupport\n", argv[0]);
                return CMD_RET_USAGE;
	}
	return 0;
}

#ifdef CONFIG_SYS_LONGHELP
static char bootx_help_text[] =
        "[[way],[mem_address],[offset]]\n"
        "- boot Android system....\n"
        "\tThe argument [way] means the way of booting boot.img.[way]='mem'/'sfc'.\n"
        "\tThe argument [mem_address] means the start position of xImage in memory.\n"
        "\tThe argument [offset] means the position of xImage in sfc-nor.\n"
	"\t[fs_type],[fs_dev],[dev:par],[mem_offset],[load_path] for filesystem boot.\n"
        "";
#endif

#ifdef CONFIG_ASLMOM_BOARD
U_BOOT_CMD(
        bootx, 6, 1, do_bootx,
        "boot xImage ",bootx_help_text
);
#else
U_BOOT_CMD(
        bootx, 5, 1, do_bootx,
        "boot xImage ",bootx_help_text
);
#endif


