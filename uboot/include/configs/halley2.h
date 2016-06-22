/*
 * Ingenic mensa configuration
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <ykli@ingenic.cn>
 * Based on: include/configs/urboard.h
 *           Written by Paul Burton <paul.burton@imgtec.com>
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

#ifndef __CONFIG_HALLEY2_H__
#define __CONFIG_HALLEY2_H__

/**
 * Basic configuration(SOC, Cache, UART, DDR).
 */
#define CONFIG_MIPS32		/* MIPS32 CPU core */
#define CONFIG_CPU_XBURST
#define CONFIG_SYS_LITTLE_ENDIAN
#define CONFIG_X1000

#define CONFIG_SYS_APLL_FREQ		1008000000	/*If APLL not use mast be set 0*/
#define CONFIG_SYS_MPLL_FREQ		600000000	/*If MPLL not use mast be set 0*/
#define CONFIG_CPU_SEL_PLL		APLL
#define CONFIG_DDR_SEL_PLL		MPLL
#define CONFIG_SYS_CPU_FREQ		1008000000
#define CONFIG_SYS_MEM_FREQ		200000000

#define CONFIG_SYS_EXTAL		24000000	/* EXTAL freq: 24 MHz */
#define CONFIG_SYS_HZ			1000 /* incrementer freq */


#define CONFIG_SYS_DCACHE_SIZE		16384
#define CONFIG_SYS_ICACHE_SIZE		16384
#define CONFIG_SYS_CACHELINE_SIZE	32

#ifdef CONFIG_FPGA
#define CONFIG_SYS_UART_INDEX		0
#define CONFIG_BAUDRATE			57600
#else
#define CONFIG_SYS_UART_INDEX		2
/*#define CONFIG_SYS_UART2_PD*/
#define CONFIG_SYS_UART2_PC
#define CONFIG_BAUDRATE			57600
#endif

#define CONFIG_UART2_PC

/*#define CONFIG_DDR_TEST*/
#define CONFIG_DDR_PARAMS_CREATOR
#define CONFIG_DDR_HOST_CC
#define CONFIG_DDR_TYPE_LPDDR
#define CONFIG_DDR_CS0			1	/* 1-connected, 0-disconnected */
#define CONFIG_DDR_CS1			0	/* 1-connected, 0-disconnected */
#define CONFIG_DDR_DW32			0	/* 1-32bit-width, 0-16bit-width */
#define CONFIG_MDDR_H5MS5122DFR_J3M
/*#define CONFIG_DDR3_TSD34096M1333C9_E*/

#define CONFIG_AUDIO_CAL_DIV
#define CONFIG_AUDIO_APLL CONFIG_SYS_APLL_FREQ
#define CONFIG_AUDIO_MPLL CONFIG_SYS_MPLL_FREQ


/*pmu slp pin*/
/* #define CONFIG_REGULATOR */
#ifdef  CONFIG_REGULATOR
#define CONFIG_JZ_PMU_SLP_OUTPUT1
#define CONFIG_INGENIC_SOFT_I2C
#define CONFIG_PMU_RICOH6x
#define CONFIG_RICOH61X_I2C_SCL	GPIO_PC(26)
#define CONFIG_RICOH61X_I2C_SDA	GPIO_PC(27)
#define CONFIG_SOFT_I2C_READ_REPEATED_START
#endif

/* #define CONFIG_DDR_DLL_OFF */
/*
 * #define CONFIG_DDR_CHIP_ODT
 * #define CONFIG_DDR_PHY_ODT
 * #define CONFIG_DDR_PHY_DQ_ODT
 * #define CONFIG_DDR_PHY_DQS_ODT
 * #define CONFIG_DDR_PHY_IMPED_PULLUP		0xe
 * #define CONFIG_DDR_PHY_IMPED_PULLDOWN	0xe
 */

#if defined(CONFIG_SPL_SFC_NOR) || defined(CONFIG_SPL_SFC_NAND)
#define CONFIG_SPL_SFC_SUPPORT
#endif

/**
 * Boot arguments definitions.
 */
#define BOOTARGS_COMMON "console=ttyS2,57600n8 mem=31M@0x0 "
#if defined(CONFIG_SPL_NOR_SUPPORT) || defined(CONFIG_SPL_SFC_SUPPORT)
	#if defined(CONFIG_SPL_SFC_SUPPORT)
		#if defined(CONFIG_SPL_SFC_NOR)
			/*#define	 CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off root=/dev/ram0 rw rdinit=/linuxrc"*/
			/* #define	 CONFIG_BOOTARGS BOOTARGS_COMMON "ip=192.168.4.254:192.168.4.1:192.168.4.1:255.255.255.0 rootdelay=2 nfsroot=192.168.4.13:/home/fpga/kyhe/rootfs rw" */
			#define	 CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=cramfs root=/dev/mtdblock2 rw"
			#define CONFIG_BOOTCOMMAND "sfcnor read 0x40000 0x800000 0x80800000 ;bootm 0x80800000"
		#else  /* CONFIG_SPL_SFC_NAND */
			#define	 CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off root=/dev/ram0 rw rdinit=/linuxrc"
			#define CONFIG_BOOTCOMMAND "sfcnand read 0x80600000 0x800000 0x500000 ;bootm 0x80600000"
		#endif
	#else
/*#define	 CONFIG_BOOTARGS BOOTARGS_COMMON " ip=192.168.10.205:192.168.10.1:192.168.10.1:255.255.255.0 nfsroot=192.168.4.3:/home/rootdir rw"*/
/*#define CONFIG_BOOTCOMMAND "tftpboot xxx/uImage; bootm"*/

	#define CONFIG_BOOTARGS BOOTARGS_COMMON " ip=192.168.10.205:192.168.10.1:192.168.10.1:255.255.255.0  nfsroot=192.168.4.13:/home/nfsroot/fpga/rootfs rw"
	#define CONFIG_BOOTCOMMAND "tftpboot 0x80600000 fpga/user/your_dir/your_uImage;bootm"

	/*#define CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off root=/dev/ram0 rw rdinit=/linuxrc"
	  #define CONFIG_BOOTCOMMAND "mmc read 0x80f00000 0x1800 0x1000; bootm 0x80f00000"*/

	#endif
#elif defined(CONFIG_SPL_SPI_NAND)
		#define	 CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off init=/linuxrc ubi.mtd=2 root=ubi0:rootfs ubi.mtd=3 rootfstype=ubifs rw"
/*		#define	 CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off init=/linuxrc root=/dev/mtdblock2 rw"*/
	/*	#define	 CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=jffs2 root=/dev/mtdblock2 rw"*/
/*		#define	 CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off ip=off root=/dev/ram0 rw rdinit=/linuxrc"*/
		#define CONFIG_BOOTCOMMAND "spinand read 0x100000 0x400000 0x80600000 ;bootm 0x80600000"
#elif defined(CONFIG_SPL_JZMMC_SUPPORT)
	#ifdef CONFIG_JZ_MMC_MSC0
		#define CONFIG_BOOTARGS BOOTARGS_COMMON " root=/dev/mmcblk0p1"
		#define CONFIG_BOOTCOMMAND "mmc dev 0;mmc read 0x80600000 0x1800 0x3000; bootm 0x80600000"
	#else
		#define CONFIG_BOOTARGS BOOTARGS_COMMON " root=/dev/mmcblk1p1"
		#define CONFIG_BOOTCOMMAND "mmc dev 1;mmc read 0x80600000 0x1800 0x3000; bootm 0x80600000"
	#endif
#endif

#ifdef CONFIG_SPL_OS_BOOT
#define CONFIG_SPL_OS_OFFSET        (0x100000) /* spi offset of xImage being loaded */
#define CONFIG_SPL_BOOTARGS         BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=cramfs root=/dev/mtdblock3 rw"
#define CONFIG_SYS_SPL_ARGS_ADDR    CONFIG_SPL_BOOTARGS
#define CONFIG_BOOTX_BOOTARGS       BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=cramfs root=/dev/mtdblock4 rw"
#undef  CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND    "bootx sfc 0x80f00000 0xd00000"
#endif

#define PARTITION_NUM 10

/**
 * Boot command definitions.
 */
#define CONFIG_BOOTDELAY 1


/* GPIO */
#define CONFIG_JZ_GPIO

/**
 * Command configuration.
 */
#define CONFIG_CMD_BOOTD	/* bootd			*/
#define CONFIG_CMD_CONSOLE	/* coninfo			*/
#define CONFIG_CMD_ECHO		/* echo arguments		*/
#define CONFIG_CMD_EXT4 	/* ext4 support			*/
#define CONFIG_CMD_FAT		/* FAT support			*/
#define CONFIG_CMD_MEMORY	/* md mm nm mw cp cmp crc base loop mtest */
#define CONFIG_CMD_MISC		/* Misc functions like sleep etc*/
#define CONFIG_CMD_RUN		/* run command in env variable	*/
#define CONFIG_CMD_SAVEENV	/* saveenv			*/
#define CONFIG_CMD_SETGETDCR	/* DCR support on 4xx		*/
#define CONFIG_CMD_SOURCE	/* "source" command support	*/
#define CONFIG_CMD_GETTIME
#define CONFIG_CMD_UNZIP        /* unzip from memory to memory  */
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_NET     /* networking support*/
#define CONFIG_CMD_PING

#ifndef CONFIG_SPL_BUILD
#define CONFIG_USE_ARCH_MEMSET
#define CONFIG_USE_ARCH_MEMCPY
#endif
/* DEBUG ETHERNET */

#define CONFIG_SERVERIP     192.168.4.13
#define CONFIG_IPADDR       192.168.4.90
#define CONFIG_GATEWAYIP    192.168.4.1
#define CONFIG_NETMASK      255.255.255.0
#define CONFIG_ETHADDR      00:11:22:33:44:55

#define GMAC_PHY_MII	1
#define GMAC_PHY_RMII	2
#define GMAC_PHY_GMII	3
#define GMAC_PHY_RGMII	4
#define CONFIG_NET_GMAC_PHY_MODE GMAC_PHY_RMII

#define PHY_TYPE_DM9161      1
#define PHY_TYPE_88E1111     2
#define CONFIG_NET_PHY_TYPE   PHY_TYPE_DM9161

#define CONFIG_NET_GMAC
#define CONFIG_GPIO_DM9161_RESET   GPIO_PB(3)
#define CONFIG_GPIO_DM9161_RESET_ENLEVEL   0
#define CONFIG_GMAC_CRLT_PORT GPIO_PORT_B
#define CONFIG_GMAC_CRLT_PORT_PINS (0x7 << 7)
#define CONFIG_GMAC_CRTL_PORT_INIT_FUNC GPIO_FUNC_1
#define CONFIG_GMAC_CRTL_PORT_SET_FUNC GPIO_OUTPUT0

/* LCD */
#define CONFIG_LCD

#define CONFIG_GPIO_PWR_WAKE		GPIO_PB(31)
#define CONFIG_GPIO_PWR_WAKE_ENLEVEL	0

#ifdef CONFIG_LCD
/*#define CONFIG_LCD_FORMAT_X8B8G8R8*/
#define LCD_BPP             5
#define CONFIG_GPIO_LCD_PWM     GPIO_PC(25)

#define CONFIG_LCD_LOGO
/*#define CONFIG_RLE_LCD_LOGO*/
/*#define CONFIG_LCD_INFO_BELOW_LOGO      //display the console info on lcd panel for debugg*/
#define CONFIG_SYS_WHITE_ON_BLACK
#define CONFIG_SYS_PWM_PERIOD       10000 /* Pwm period in ns */
#define CONFIG_SYS_PWM_CHN      0  /* Pwm channel ok*/
#define CONFIG_SYS_PWM_FULL     256
#define CONFIG_SYS_BACKLIGHT_LEVEL  80 /* Backlight brightness is (80 / 256) */
#define CONFIG_JZ_LCD_V13
#define SOC_X1000
#define CONFIG_JZ_PWM
#define CONFIG_SYS_CONSOLE_INFO_QUIET
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
/*#define CONFIG_SLCDC_CONTINUA*/
/*#define CONFIG_LCD_GPIO_FUNC0_24BIT*/
#define CONFIG_LCD_GPIO_FUNC1_SLCD
/*#define CONFIG_VIDEO_BM347WV_F_8991FTGF*/
#define CONFIG_VIDEO_TRULY_TFT240240_2_E

#ifdef CONFIG_VIDEO_TRULY_TFT240240_2_E
#define CONFIG_GPIO_LCD_RD GPIO_PB(16)
#define CONFIG_GPIO_LCD_RST GPIO_PD(0)
#define CONFIG_GPIO_LCD_CS GPIO_PB(18)
#define CONFIG_GPIO_LCD_BL GPIO_PD(1)
#endif	/* CONFIG_VIDEO_TRULY_TFT240240_2_E */

#ifdef CONFIG_RLE_LCD_LOGO
#define CONFIG_CMD_BATTERYDET       /* detect battery and show charge logo */
#define CONFIG_CMD_LOGO_RLE /*display the logo using rle command*/
#endif

#endif /* CONFIG_LCD */

/* MMC */
#define CONFIG_CMD_MMC
/*#define CONFIG_MMC_TRACE*/


#ifdef CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC		1
#define CONFIG_MMC			1
#define CONFIG_JZ_MMC			1

#ifdef CONFIG_JZ_MMC_MSC0
#define CONFIG_JZ_MMC_SPLMSC 0
#define CONFIG_JZ_MMC_MSC0_PA_4BIT 1
/* #define CONFIG_JZ_MMC_MSC0_PA_8BIT 1 */
/* #define CONFIG_MSC_DATA_WIDTH_8BIT */
#define CONFIG_MSC_DATA_WIDTH_4BIT
/* #define CONFIG_MSC_DATA_WIDTH_1BIT */
#endif

#ifdef CONFIG_JZ_MMC_MSC1
/*#define CONFIG_JZ_MMC_SPLMSC 1*/
#define CONFIG_JZ_MMC_MSC1_PC 1
/* #define CONFIG_MSC_DATA_WIDTH_8BIT */
#define CONFIG_MSC_DATA_WIDTH_4BIT
/* #define CONFIG_MSC_DATA_WIDTH_1BIT */
#endif
#endif

/**
 * Serial download configuration
 */
#define CONFIG_LOADS_ECHO	1	/* echo on for serial download */

/**
 * Miscellaneous configurable options
 */
#define CONFIG_DOS_PARTITION

#define CONFIG_LZO
#define CONFIG_RBTREE

#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_SYS_NO_FLASH
#define CONFIG_SYS_FLASH_BASE	0 /* init flash_base as 0 */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_MISC_INIT_R 1

#define CONFIG_BOOTP_MASK	(CONFIG_BOOTP_DEFAUL)

#define CONFIG_SYS_MAXARGS 16
#define CONFIG_SYS_LONGHELP

#if defined(CONFIG_SPL_JZMMC_SUPPORT)
	#if	defined(CONFIG_JZ_MMC_MSC0)
	#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-msc0# "
	#else
	#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-msc1# "
	#endif
#elif defined(CONFIG_SPL_NOR_SUPPORT)
#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-nor# "
#elif defined(CONFIG_SPL_SFC_SUPPORT)
	#if defined(CONFIG_SPL_SFC_NOR)
		#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-sfcnor# "
	#else  /* CONFIG_SPL_SFC_NAND */
		#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-sfcnand# "
	#endif
#elif defined(CONFIG_SPL_SPI_NAND)
		#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-spinand# "
#endif

#define CONFIG_SYS_CBSIZE 1024 /* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

#if defined(CONFIG_SUPPORT_EMMC_BOOT)
#define CONFIG_SYS_MONITOR_LEN		(384 * 1024)
#else
#define CONFIG_SYS_MONITOR_LEN		(512 << 10)
#endif

#define CONFIG_SYS_MALLOC_LEN		(8 * 1024 * 1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128 * 1024)

#define CONFIG_SYS_SDRAM_BASE		0x80000000 /* cached (KSEG0) address */
#define CONFIG_SYS_SDRAM_MAX_TOP	0x90000000 /* don't run into IO space */
#define CONFIG_SYS_INIT_SP_OFFSET	0x400000
#define CONFIG_SYS_LOAD_ADDR		0x88000000
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		0x88000000
#define CONFIG_SYS_TEXT_BASE		0x80100000
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE

/**
 * Environment
 */
#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_SIZE			(32 << 10)
#define CONFIG_ENV_OFFSET		(CONFIG_SYS_MONITOR_LEN + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)
#elif defined(CONFIG_ENV_IS_IN_SFC)
#define CONFIG_ENV_SIZE			(4 << 10)
#define CONFIG_ENV_OFFSET		0x3e800 /*write nor flash 250k address*/
#define CONFIG_CMD_SAVEENV
#endif

/**
 * SPL configuration
 */
#define CONFIG_SPL
#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_NO_CPU_SUPPORT_CODE
#define CONFIG_SPL_START_S_PATH		"$(CPUDIR)/$(SOC)"
#ifdef CONFIG_SPL_NOR_SUPPORT
#define CONFIG_SPL_LDSCRIPT             "$(CPUDIR)/$(SOC)/u-boot-nor-spl.lds"
#else
#define CONFIG_SPL_LDSCRIPT		"$(CPUDIR)/$(SOC)/u-boot-spl.lds"
#endif
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	0x3A /* 12KB+17K offset */
#define CONFIG_SYS_U_BOOT_MAX_SIZE_SECTORS	0x200 /* 256 KB */
#define CONFIG_SPL_SERIAL_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#if defined(CONFIG_SPL_NOR_SUPPORT)
#define CONFIG_SPL_TEXT_BASE		0xba000000
#define CONFIG_SYS_UBOOT_BASE		(CONFIG_SPL_TEXT_BASE + CONFIG_SPL_PAD_TO - 0x40)
					/* 0x40 = sizeof (image_header)*/
#define CONFIG_SYS_OS_BASE		0
#define CONFIG_SYS_SPL_ARGS_ADDR	0
#define CONFIG_SYS_FDT_BASE		0
#define CONFIG_SPL_PAD_TO		32768
#define CONFIG_SPL_MAX_SIZE		(32 * 1024)
#elif defined(CONFIG_SPL_JZMMC_SUPPORT)
#define CONFIG_SPL_PAD_TO		12288  /* spl size */
#define CONFIG_SPL_TEXT_BASE		0xf4001000
#define CONFIG_SPL_MAX_SIZE		(12 * 1024)
#elif defined(CONFIG_SPL_SFC_SUPPORT)
#define CONFIG_UBOOT_OFFSET             (4<<12)
#define CONFIG_JZ_SFC_PA_6BIT
#ifdef	CONFIG_SPL_SFC_NAND
#define CONFIG_SPI_NAND_BPP			(2048 +64)		/*Bytes Per Page*/
#define CONFIG_SPI_NAND_PPB			(64)		/*Page Per Block*/
#define CONFIG_SPL_TEXT_BASE		0xf4001000
#define CONFIG_SPL_MAX_SIZE		((16 * 1024) - 0x800)
#define CONFIG_SPL_PAD_TO		16384
#define CONFIG_SPL_SFC_NAND
#define CONFIG_CMD_SFC_NAND
#define CONFIG_CMD_NAND
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE    0xb3441000
#define CONFIG_SYS_MAXARGS	6
#else
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_SPL_TEXT_BASE		0xf4001000
#define CONFIG_SPL_MAX_SIZE		((16 * 1024) - 0x800)
#define CONFIG_SPL_PAD_TO		16384
#define CONFIG_CMD_SFC_NOR
#endif
#endif

#ifdef CONFIG_SPL_SPI_NAND
#define CONFIG_UBOOT_OFFSET             (4<<12)
#define CONFIG_SPL_TEXT_BASE		0xf4001000
#define CONFIG_SPL_MAX_SIZE		((16 * 1024) - 0x800)
#define CONFIG_SPL_PAD_TO		16384
#define CONFIG_SPI_NAND_BPP			(2048 +128)		/*Bytes Per Page*/
#define CONFIG_SPI_NAND_PPB			(64)		/*Page Per Block*/
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_JZ_SPI
#define CONFIG_JZ_SSI0_PA
#define CONFIG_MTD_SPINAND
#define CONFIG_CMD_SPINAND
#define CONFIG_SPI_FLASH
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_MTD_PARTITIONS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_ENV_IS_IN_SPI_NAND

/* spi nand environment */
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_ENV_SECT_SIZE 0x20000 /* 128K*/
#define SPI_NAND_BLK		0x20000 /*the spi nand block size */
#define CONFIG_ENV_SIZE		SPI_NAND_BLK /* uboot is 1M but the last block size is the env*/
#define CONFIG_ENV_OFFSET	0xc0000 /* offset is 768k */
#define CONFIG_ENV_OFFSET_REDUND (CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)


#define CONFIG_CMD_NAND /*use the mtd and the function do_nand() */
#define CONFIG_SYS_MAX_NAND_DEVICE  1

#endif /*CONFIG_SPL_SPI_NAND*/

#ifdef CONFIG_CMD_SPI
#define CONFIG_SSI_BASE SSI0_BASE
#define CONFIG_SPI_BUILD
#ifdef CONFIG_SOFT_SPI
#undef SPI_INIT
#define SPI_DELAY
#define	SPI_SDA(val)    gpio_direction_output(GPIO_PA(21), val)
#define	SPI_SCL(val)	gpio_direction_output(GPIO_PA(18), val)
#define	SPI_READ	gpio_get_value(GPIO_PA(20))
#else
#define CONFIG_JZ_SPI
#endif
/*#define CONFIG_JZ_SPI_FLASH*/
#define CONFIG_CMD_SF
#define CONFIG_SPI_FLASH_INGENIC
#define CONFIG_SPI_FLASH
#define CONFIG_UBOOT_OFFSET             (4<<12)
#endif

#ifdef CONFIG_CMD_SFC_NOR
#define CONFIG_JZ_SFC
/*#define CONFIG_SPI_DUAL*/
#define CONFIG_SPI_QUAD
#endif
/**
 * MBR configuration
 */
#ifdef CONFIG_MBR_CREATOR
#define CONFIG_MBR_P0_OFF	64mb
#define CONFIG_MBR_P0_END	556mb
#define CONFIG_MBR_P0_TYPE 	linux

#define CONFIG_MBR_P1_OFF	580mb
#define CONFIG_MBR_P1_END 	1604mb
#define CONFIG_MBR_P1_TYPE 	linux

#define CONFIG_MBR_P2_OFF	28mb
#define CONFIG_MBR_P2_END	58mb
#define CONFIG_MBR_P2_TYPE 	linux

#define CONFIG_MBR_P3_OFF	1609mb
#define CONFIG_MBR_P3_END	7800mb
#define CONFIG_MBR_P3_TYPE 	fat
#else
#define CONFIG_GPT_TABLE_PATH	"$(TOPDIR)/board/$(BOARDDIR)"
#endif


/*
* MTD support
*/
#define CONFIG_SYS_NAND_SELF_INIT

#endif /* __CONFIG_HALLEY2_H__ */
