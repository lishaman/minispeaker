/*
 * Ingenic JZ SPI driver
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
 * Author: Tiger <lbh@ingenic.cn>
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

#ifndef __JZ_SPI_H__
#define __JZ_SPI_H__

#ifdef CONFIG_JZ_SFC
#include <asm/arch/sfc.h>
#endif
#define SSI_BASE SSI0_BASE
#define COMMAND_MAX_LENGTH		8
#define SIZEOF_NAME			32
#define FIFI_THRESHOLD			64
#define SPI_WRITE_CHECK_TIMES		50

#define NOR_MAGIC       0x726f6e        //ascii "nor"
#define NOR_PART_NUM    10
#define NORFLASH_PART_RW        0
#define NORFLASH_PART_WO        1
#define NORFLASH_PART_RO        2
struct nor_partition {
	char name[32];
	uint32_t size;
	uint32_t offset;
	uint32_t mask_flags;//bit 0-1 mask the partiton RW mode, 0:RW  1:W  2:R
	uint32_t manager_mode;
};

struct norflash_partitions {
	struct nor_partition nor_partition[NOR_PART_NUM];
	uint32_t num_partition_info;
};


struct spi_nor_block_info {
	u32 blocksize;
	u8 cmd_blockerase;
	/* MAX Busytime for block erase, unit: ms */
	u32 be_maxbusy;
};

struct spi_quad_mode {
	u8 dummy_byte;
	u8 RDSR_CMD;
	u8 WRSR_CMD;
	unsigned int RDSR_DATE;//the data is write the spi status register for QE bit
	unsigned int RD_DATE_SIZE;//the data is write the spi status register for QE bit
	unsigned int WRSR_DATE;//this bit should be the flash QUAD mode enable
	unsigned int WD_DATE_SIZE;//the data is write the spi status register for QE bit
	u8 cmd_read;
	u8 sfc_mode;
};

#ifdef CONFIG_JZ_SFC
struct norflash_params {
	char name[SIZEOF_NAME];
	u32 pagesize;
	u32 sectorsize;
	u32 chipsize;
	u32 erasesize;
	int id;
	/* Flash Address size, unit: Bytes */
	int addrsize;

	/* MAX Busytime for page program, unit: ms */
	u32 pp_maxbusy;
	/* MAX Busytime for sector erase, unit: ms */
	u32 se_maxbusy;
	/* MAX Busytime for chip erase, unit: ms */
	u32 ce_maxbusy;

	/* Flash status register num, Max support 3 register */
	int st_regnum;
	/* Some NOR flash has different blocksize and block erase command,
	*          * One command with One blocksize. */
	struct spi_nor_block_info block_info;
	struct spi_quad_mode quad_mode;
};

struct nor_sharing_params {
	uint32_t magic;
	uint32_t version;
	struct norflash_params norflash_params;
	struct norflash_partitions norflash_partitions;
};
#endif


struct jz_spi_support {
	unsigned int id_manufactory;
	u8 id_device;
	char name[SIZEOF_NAME];
	int page_size;
	int oobsize;
	int sector_size;
	int block_size;
	int addr_size;
	int size;
	int page_num;
	unsigned int *page_list;
	unsigned short column_cmdaddr_bits;/* read from cache ,the bits of cmd + addr */
	struct spi_quad_mode quad_mode;
};

struct jz_spi_slave {
	struct spi_slave slave;
	unsigned int mode;
	unsigned int max_hz;
};

static struct jz_spi_support jz_spi_nand_support_table[] = {

	{
		.id_manufactory = 0xc8,
		.id_device = 0xf1,
		.name = "GD5F1GQ4U",
		.page_size = 2 * 1024,
		.block_size = 128 * 1024,
		.size = 128 * 1024 * 1024,
	},
	{
		.id_manufactory = 0xc8,
		.id_device = 0xf2,
		.name = "GD5F2GQ4U",
		.page_size = 2 * 1024,
		.block_size = 128 * 1024,
		.size = 256 * 1024 * 1024,
	},
	{
		.id_manufactory = 0xc8,
		.id_device = 0xf4,
		.name = "GD5F4GQ4U",
		.page_size = 2 * 1024,
		.block_size = 128 * 1024,
		.size = 512 * 1024 * 1024,
	},

	/****************supported spi nand *************************/
	{
		.id_manufactory = 0xc8,
		.id_device = 0xd1,
		.name = "GD5F1GQ4UBY1G",
		.page_size = 2 * 1024,
		.oobsize = 128,
		.block_size = 128 * 1024,
		.size = 128 * 1024 * 1024,
		.column_cmdaddr_bits = 24,
	},
	{
		.id_manufactory = 0xa1,
		.id_device = 0xe1,
		.name = "PN26G01AWSIUG-1Gbit",
		.page_size = 2 * 1024,
		.oobsize = 128,
		.block_size = 128 * 1024,
		.size = 128 * 1024 * 1024,
		.column_cmdaddr_bits = 24,
	},
	{
		.id_manufactory = 0xc9,
		.id_device = 0x51,
		.name = "QPSYG01AW0A-A1",
		.page_size = 2 * 1024,
		.oobsize = 128,
		.block_size = 128 * 1024,
		.size = 128 * 1024 * 1024,
		.column_cmdaddr_bits = 24,
	},
	{
		.id_manufactory = 0xb2,
		.id_device = 0x48,
		.name = "GD5F2GQ4UCY1G",
		.page_size = 2 * 1024,
		.oobsize = 128,
		.block_size = 128 * 1024,
		.size = 256 * 1024 * 1024,
		.column_cmdaddr_bits = 32,
	},
};

static struct jz_spi_support jz_spi_support_table[] = {
	{
		.id_manufactory = 0x1820c2,
		.name = "MX25L12835F",
		.page_size = 256,
		.sector_size = 4 * 1024,
		.addr_size = 3,
		.size = 16 * 1024 * 1024,
		.quad_mode = {
			.dummy_byte = 8,
			.RDSR_CMD = CMD_RDSR,
			.WRSR_CMD = CMD_WRSR,
			.RDSR_DATE = 0x40,//the data is write the spi status register for QE bit
			.RD_DATE_SIZE = 1,
			.WRSR_DATE = 0x40,//this bit should be the flash QUAD mode enable
			.WD_DATE_SIZE = 1,
			.cmd_read = CMD_QUAD_READ,
#ifdef CONFIG_JZ_SFC
			.sfc_mode = TRAN_SPI_QUAD,
#endif
		},
	},
	{
		.id_manufactory = 0x1840c8,
		.name = "GD25Q128C",
		.page_size = 256,
		.sector_size = 4 * 1024,
		.addr_size = 3,
		.size = 16 * 1024 * 1024,
		.quad_mode = {
			.dummy_byte = 8,
			.RDSR_CMD = CMD_RDSR_1,
			.WRSR_CMD = CMD_WRSR_1,
			.RDSR_DATE = 0x2,// the data is write the spi status register for QE bit
			.RD_DATE_SIZE = 1,
			.WRSR_DATE = 0x2,// this bit should be the flash QUAD mode enable
			.WD_DATE_SIZE = 1,
			.cmd_read = CMD_QUAD_READ,
#ifdef CONFIG_JZ_SFC
			.sfc_mode = TRAN_SPI_QUAD,
#endif
		},
	},
	{
		.id_manufactory = 0x1860c8,
		.name = "GD25LQ128C",
		.page_size = 256,
		.sector_size = 4 * 1024,
		.addr_size = 3,
		.size = 16 * 1024 * 1024,
		.quad_mode = {
			.dummy_byte = 8,
			.RDSR_CMD = CMD_RDSR_1,
			.WRSR_CMD = CMD_WRSR,
			.RDSR_DATE = 0x2,// the data is write the spi status register for QE bit
			.RD_DATE_SIZE = 1,
			.WRSR_DATE = 0x0200,// this bit should be the flash QUAD mode enable
			.WD_DATE_SIZE = 2,
			.cmd_read = CMD_QUAD_READ,
#ifdef CONFIG_JZ_SFC
			.sfc_mode = TRAN_SPI_QUAD,
#endif
		},
	},
	{
		.id_manufactory = 0x18609d,
		.name = "IS25LP128",
		.page_size = 256,
		.sector_size = 4 * 1024,
		.addr_size = 3,
		.size = 16 * 1024 * 1024,
		.quad_mode = {
			.dummy_byte = 6,
			.RDSR_CMD = CMD_RDSR,
			.WRSR_CMD = CMD_WRSR,
			.RDSR_DATE = 0x40,//the data is write the spi status register for QE bit
			.RD_DATE_SIZE = 1,
			.WRSR_DATE = 0x40,//his bit should be the flash QUAD mode enable
			.WD_DATE_SIZE = 1,
			.cmd_read = CMD_QUAD_IO_FAST_READ,
#ifdef CONFIG_JZ_SFC
			.sfc_mode = TRAN_SPI_IO_QUAD,
#endif
		},
	},
	{
		.id_manufactory = 0x1840ef,
		.name = "win25Q128fvq",
		.page_size = 256,
		.sector_size = 4 * 1024,
		.addr_size = 3,
		.size = 16 * 1024 * 1024,
		.quad_mode = {
			.dummy_byte = 6,
			.RDSR_CMD = CMD_RDSR_1,
			.WRSR_CMD = CMD_WRSR_1,
			.RDSR_DATE = 0x2,//the data is write the spi status register for QE bit
			.RD_DATE_SIZE = 1,
			.WRSR_DATE = 0x02,//his bit should be the flash QUAD mode enable
			.WD_DATE_SIZE = 1,
			.cmd_read = CMD_QUAD_IO_FAST_READ,
#ifdef CONFIG_JZ_SFC
			.sfc_mode = TRAN_SPI_IO_QUAD,
#endif
		},
	},
	{
		.id_manufactory = 0x1940ef,
		.name = "win25Q256fvq",
		.page_size = 256,
		.sector_size = 4 * 1024,
		.addr_size = 4,
		.size = 32 * 1024 * 1024,
		.quad_mode = {
			.dummy_byte = 8,
			.RDSR_CMD = CMD_RDSR_1,
			.WRSR_CMD = CMD_WRSR_1,
			.RDSR_DATE = 0x2,//the data is write the spi status register for QE bit
			.RD_DATE_SIZE = 1,
			.WRSR_DATE = 0x2,//his bit should be the flash QUAD mode enable
			.WD_DATE_SIZE = 1,
			.cmd_read = CMD_QUAD_READ,
#ifdef CONFIG_JZ_SFC
			.sfc_mode = TRAN_SPI_QUAD,
#endif
		},
	},

	{
		.id_manufactory = 0x1940c8,
		.name = "GD25Q256C",
		.page_size = 256,
		.sector_size = 4 * 1024,
		.addr_size = 4,
		.size = 32 * 1024 * 1024,
		.quad_mode = {
			.dummy_byte = 8,
			.RDSR_CMD = CMD_RDSR,
			.WRSR_CMD = CMD_WRSR,
			.RDSR_DATE = 0x40,//the data is write the spi status register for QE bit
			.RD_DATE_SIZE = 1,
			.WRSR_DATE = 0x40,//this bit should be the flash QUAD mode enable
			.WD_DATE_SIZE = 1,
			.cmd_read = CMD_QUAD_READ,
#ifdef CONFIG_JZ_SFC
			.sfc_mode = TRAN_SPI_QUAD,
#endif
		},
	},
	
		

};

#endif /* __JZ_SPI_H__ */
