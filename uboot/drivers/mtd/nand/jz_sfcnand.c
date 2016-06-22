/*
 * jz mtd spi nand driver probe interface
 *
 *  Copyright (C) 2013 Ingenic Semiconductor Co., LTD.
 *  Author: cli <chen.li@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later versio.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <nand.h>
#include <spi.h>
#include <spi_flash.h>
#include <linux/list.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <asm/arch/spi.h>
#include <asm/arch/sfc.h>
#include <ingenic_nand_mgr/nand_param.h>
#include "../spi/spi_flash_internal.h"
#include "../../spi/jz_spi.h"
#include "jz_spinand.h"

#include <asm/arch/cpm.h>
#include <asm/arch/clk.h>

#define IDCODE_CONT_LEN 0
#define IDCODE_PART_LEN 5
#ifdef MTDIDS_DEFAULT
static const char *const mtdids_default = MTDIDS_DEFAULT;
#else
static const char *const mtdids_default = "nand0:nand";
#endif

#define SIZE_UBOOT  0x100000    /* 1M */
#define SIZE_KERNEL 0x800000    /* 8M */
#define SIZE_ROOTFS (0x100000 * 40)        /* -1: all of left */

extern unsigned int sfc_rate ;
unsigned short column_cmdaddr_bits;/* read from cache ,the bits of cmd + addr */
static struct jz_spinand_partition jz_mtd_spinand_partition[] = {
	{
		.name =     "uboot",
		.offset =   0,
		.size =     SIZE_UBOOT,
		.manager_mode = MTD_MODE,
	},
	{
		.name =     "kernel",
		.offset =   SIZE_UBOOT,
		.size =     SIZE_KERNEL,
		.manager_mode = MTD_MODE,
	},
	{
		.name   =       "rootfs",
		.offset =   SIZE_UBOOT + SIZE_KERNEL,
		.size   =   SIZE_ROOTFS,
		.manager_mode = UBI_MANAGER,
		//.manager_mode = MTD_MODE,
	},
};



/* wait time before read status (us) for spi nand */
//static int t_reset = 500;
static int t_read  = 120;
static int t_write = 700;
static int t_erase = 5000;


static struct nand_ecclayout gd5f_ecc_layout_128 = {
	.oobavail = 0,
};
int sfc_nand_erase(struct mtd_info *mtd,int addr)
{
	unsigned char cmd[COMMAND_MAX_LENGTH];
	int erase_cmd;
	int page = addr / mtd->writesize;
	int block_size = mtd->erasesize;

	switch(block_size){
		case 4 * 1024:
			erase_cmd = CMD_ERASE_4K;
			break;
		case 32 * 1024:
			erase_cmd = CMD_ERASE_32K;
			break;
		case 64 * 1024:
			erase_cmd = CMD_ERASE_64K;
			break;
		case 128 * 1024:
			erase_cmd = CMD_ERASE_128K;
			break;
		default:
			printf("WARNING: don't support the erase size !\n");
			break;
	}

	/* the paraterms is
	* cmd , len, addr,addr_len
	* dummy_byte, daten
	* dir 0,read 1.write
	*
	* */
	volatile unsigned int x;
	cmd[0] = CMD_WREN;//write en
	sfc_send_cmd(&cmd[0],0,0,0,0,0,0);
	memset(cmd,COMMAND_MAX_LENGTH,0);
	cmd[0]=erase_cmd;//erase
	sfc_send_cmd(&cmd[0],0,page,3,0,0,0);
	memset(cmd,COMMAND_MAX_LENGTH,0);

	cmd[0]=CMD_GET_FEATURE;//get feature
	sfc_send_cmd(&cmd[0],1,0xc0,1,0,1,0);
	sfc_nand_read_data(&x,1);
	x=x&0x000000ff;
	while(x & 0x1)
	{
		x=0;
		sfc_send_cmd(&cmd[0],1,0xc0,1,0,1,0);
		sfc_nand_read_data(&x,1);
	}
	if(x & E_FAIL)
		return -1;

	return 0;
}
static int sfc_nand_write(struct mtd_info *mtd,loff_t addr,int column,size_t len,u_char *buf)
{
	unsigned char state, cmd[COMMAND_MAX_LENGTH];
	int page_size = mtd->writesize;
	int page;// = addr / page_size;
	int wlen,i,write_num;
	int ops_len;
	u_char *buffer = buf;
	page = addr/page_size;

	write_num = (len + page_size - 1) / page_size;
	for(i = 0; i < write_num; i++)
	{
		if(len >= page_size)
			wlen = page_size;
		else
			wlen = len;

		/* the paraterms is
		* cmd , datelen,
		*addr,addr_len
		* dummy_byte, daten
		* dir 0,read 1.write
		*
		* */

		cmd[0]=CMD_PRO_LOAD;//get feature
		sfc_send_cmd(&cmd[0],wlen,column,2,0,1,1);

		ops_len = wlen;
	/*	while(ops_len) {
			if(ops_len > FIFI_THRESHOLD) {
				sfc_nand_write_data(buffer,FIFI_THRESHOLD);
				buffer += FIFI_THRESHOLD;
				ops_len -= FIFI_THRESHOLD;
			} else {
				sfc_nand_write_data(buffer,ops_len);
				buffer += ops_len;
				ops_len = 0;
			}
		}*/
		sfc_nand_write_data(buffer,ops_len);
		buffer += ops_len;
		cmd[0] = CMD_WREN;
		sfc_send_cmd(&cmd[0],0,0,0,0,0,0);

		cmd[0] = CMD_PE;
		sfc_send_cmd(&cmd[0],0,page,3,0,0,0);
		udelay(t_write);

		state=0;
		cmd[0] = CMD_GET_FEATURE;
		sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
		sfc_nand_read_data(&state,1);
		while(state & 0x1) ///////////////////////////////////////////
		{
			cmd[0] = CMD_GET_FEATURE;
			sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
			sfc_nand_read_data(&state,1);
		}

		if(state & P_FAIL){
			printf("WARNING: write fail !\n");
			return (len - i * wlen);
		}

		len -= wlen;
		page++;
	}
	return 0;
}

static int sfc_nand_read(struct mtd_info *mtd,loff_t addr,int column,size_t len,u_char *buf)
{
	unsigned char cmd[COMMAND_MAX_LENGTH];
	volatile unsigned int read_buf;
	int page_size = mtd->writesize;
	int page = addr / page_size;
	int i,read_num,rlen;
	u_char *buffer = buf;

	read_num = (len + page_size - 1) / page_size;
	for(i = 0; i < read_num; i++){
		if(len >= page_size)
			rlen = page_size;
		else
			rlen = len;
		/* the paraterms is
		* cmd , datelen,
		* addr,addr_len
		* dummy_byte, daten
		* dir 0,read 1.write
		*
		* */
		cmd[0]=CMD_PARD;//
		sfc_send_cmd(&cmd[0],0,page,3,0,0,0);


		cmd[0]=CMD_GET_FEATURE;//get feature
		sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
		sfc_nand_read_data(&read_buf,1);
	//	printf("read_buf=%08x\n",read_buf);
		while(read_buf & 0x1)
		{
			cmd[0]=CMD_GET_FEATURE;//get feature
			sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
			sfc_nand_read_data(&read_buf,1);
		}
		if((read_buf & 0x30) == 0x20) {
			printf("%s %d read error pageid = %d!!!\n",__func__,__LINE__,page);
			return -1;
		}
		switch(column_cmdaddr_bits){
				case 24:
					cmd[0]=CMD_R_CACHE;//get feature
					column=(column<<8)&0xffffff00;
					sfc_send_cmd(&cmd[0],rlen,column,3,0,1,0);
					sfc_nand_read_data(buffer,rlen);
				break;
			case 32:
				cmd[0]=CMD_FR_CACHE;//get feature
				column=(column<<8)&0xffffff00;
				sfc_send_cmd(&cmd[0],rlen,column,4,0,1,0);
				sfc_nand_read_data(buffer,rlen);
				break;
			default:
				printk("can't support the column addr format !!!\n");
				break;
		}
		buffer += rlen;
		len -= rlen;
		page++;
	}

	return 0;
}
static int sfcnand_write_oob(struct mtd_info *mtd,loff_t addr,struct mtd_oob_ops *ops)
{
	unsigned char cmd[COMMAND_MAX_LENGTH];
	int page = addr / mtd->writesize;
	int ret;

	cmd[0]=CMD_PARD;//get feature
	sfc_send_cmd(&cmd[0],0,page,3,0,0,0);
	udelay(t_read);

	ret = sfc_nand_write(mtd,addr,mtd->writesize,ops->ooblen,ops->oobbuf);
	if(ret){
		printf("WARNING: Mark bad block fail !\n");
		return -1;
	}

	return 0;
}
static int sfcnand_block_isbad(struct mtd_info *mtd,loff_t ofs) ;
static int sfcnand_block_markbad(struct mtd_info *mtd,loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	int ret;

	ret = sfcnand_block_isbad(mtd, ofs);
	if (ret) {
		/* If it was bad already, return success and do nothing */
		if (ret > 0)
			return 0;
		return ret;
	}

	return chip->block_markbad(mtd, ofs);
}
static int spinand_read_oob(struct mtd_info *mtd,loff_t addr,struct mtd_oob_ops *ops);
int sfcnand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int block_size = mtd->erasesize;
	int block,addr,ret;
	addr = instr->addr;

	ret = sfc_nand_erase(mtd,addr);
	if(ret){
		printf("WARNING: block %d erase fail !\n",addr / mtd->erasesize);

		ret = sfcnand_block_markbad(mtd,addr);
		if(ret){
			printf("mark bad block error, there will occur error,so exit !\n");
			return -1;
		}
	}
	instr->state = MTD_ERASE_DONE;
	return ret;
}
static int sfcnand_read_oob(struct mtd_info *mtd,loff_t addr,struct mtd_oob_ops *ops)
{
	int len = ops->ooblen;
	int column = mtd->writesize;
	unsigned char cmd[COMMAND_MAX_LENGTH];
	volatile unsigned int read_buf;
	int page_size = mtd->writesize;
	int page = addr / page_size;
	u_char *buffer = ops->oobbuf;
	/* the paraterms is
	* cmd , len, addr,addr_len
	* dummy_byte, daten
	* dir 0,read 1.write
	*
	* */
	cmd[0] = CMD_PARD;//write en
	sfc_send_cmd(&cmd[0],0,page,3,0,0,0);
	udelay(t_read);

	cmd[0]=CMD_GET_FEATURE;//get feature
	sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
	sfc_nand_read_data(&read_buf,1);



	while(read_buf & 0x1)
	{
		cmd[0]=CMD_GET_FEATURE;//get feature
		sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
		sfc_nand_read_data(&read_buf,1);
	}
	if((read_buf & 0x30) == 0x20) {
		printf("%s %d read error pageid = %d!!!\n",__func__,__LINE__,page);
		return 1;
	}
        switch(column_cmdaddr_bits){
        	case 24:
			cmd[0]=CMD_R_CACHE;//get feature
			column=(column<<8)&0xffffff00;
			sfc_send_cmd(&cmd[0],len,column,3,0,1,0);
			sfc_nand_read_data(buffer,len);
			break;
		case 32:
			cmd[0]=CMD_FR_CACHE;//get feature
			column=(column<<8)&0xffffff00;
			sfc_send_cmd(&cmd[0],len,column,4,0,1,0);
			sfc_nand_read_data(buffer,len);
			break;
		default:
			printk("can't support the column addr format !!!\n");
			break;
	}

	return 0;
}
static int badblk_check(int len,unsigned char *buf)
{
	int i,bit0_cnt = 0;
	unsigned short *check_buf = (unsigned short *)buf;

	if(check_buf[0] != 0xff){
		for(i = 0; i < len * 8; i++){
			if(!((check_buf[0] >> i) & 0x1))
				bit0_cnt++;
		}
	}
	if(bit0_cnt > 6 * len)
		return 1; // is bad blk

	return 0;
}
static int sfcnand_block_checkbad(struct mtd_info *mtd, loff_t ofs,int getchip,int allowbbt)
{
	struct nand_chip *chip = mtd->priv;

	if (!(chip->options & NAND_BBT_SCANNED)) {
		chip->options |= NAND_BBT_SCANNED;
		chip->scan_bbt(mtd);
	}
	if (!chip->bbt)
		return chip->block_bad(mtd, ofs,getchip);

	/* Return info from the table */
	return nand_isbad_bbt(mtd, ofs, allowbbt);
}

static int sfcnand_block_isbad(struct mtd_info *mtd,loff_t ofs)
{
	int ret;
	ret = sfcnand_block_checkbad(mtd, ofs,1, 0);
	return ret;
}
static int jz_sfcnand_block_bad_check(struct mtd_info *mtd, loff_t ofs,int getchip)
{
	int column = mtd->writesize;
	int check_len = 2;
	unsigned char check_buf[] = {0xaa,0xaa};
	unsigned char  cmd[COMMAND_MAX_LENGTH];
	struct mtd_oob_ops ops;

	ops.oobbuf = check_buf;
	ops.ooblen = check_len;
	sfcnand_read_oob(mtd,ofs,&ops);

	if(badblk_check(check_len,check_buf))
		return 1;
	return 0;
}
static int sfcnand_read(struct mtd_info *mtd,loff_t addr,size_t len,size_t *retlen,u_char *buf)
{
	int ret;

	ret = sfc_nand_read(mtd,addr,0,len,buf);
	if(ret)
		*retlen += ret;
	else
		*retlen += len;

	return 0;
}
static int sfcnand_write(struct mtd_info *mtd,loff_t addr,size_t len,size_t *retlen, u_char *buf)
{
	int ret,i,n=0;
	int readv;
	ret = sfc_nand_write(mtd,addr,0,len,buf);
	*retlen = ret;

	return ret;
}
static void mtd_sfcnand_init(struct mtd_info *mtd)
{
	mtd->_erase = sfcnand_erase;
	mtd->_point = NULL;
	mtd->_unpoint = NULL;
	mtd->_read = sfcnand_read;
	mtd->_write = sfcnand_write;
	mtd->_read_oob = sfcnand_read_oob;
	mtd->_write_oob = sfcnand_write_oob;
	mtd->_lock = NULL;
	mtd->_unlock = NULL;
	mtd->_block_isbad = sfcnand_block_isbad;
	mtd->_block_markbad = sfcnand_block_markbad;
}

static void jz_sfcnand_ext_init(void )
{
	unsigned char cmd[COMMAND_MAX_LENGTH];
	unsigned int x=0;
	/* disable write protect */
	unsigned int add;
	cmd[0]=0x1f;//get feature
	add=0xa0;

	sfc_send_cmd(&cmd[0],1,add,1,0,1,1);
	sfc_nand_write_data(&x,1);

	cmd[0]=0x1f;//get feature
	add=0xb0;
	x=0x10;
	sfc_send_cmd(&cmd[0],1,add,1,0,1,1);
	sfc_nand_write_data(&x,1);

	x=0;
	cmd[0]=CMD_GET_FEATURE;//get feature
	add=0xa0;
	sfc_send_cmd(&cmd[0],1,add,1,0,1,0);
	sfc_nand_read_data(&x,1);
	x=x&0x000000ff;
	printf("read status 0xa0 : %x\n", x);

	x=0;
	cmd[0]=CMD_GET_FEATURE;//get feature
	add=0xb0;
	sfc_send_cmd(&cmd[0],1,add,1,0,1,0);
	sfc_nand_read_data(&x,1);
	x=x&0x000000ff;
	printf("read status 0xb0 : %x\n", x);

}
static int jz_sfcnand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	uint8_t buf[2] = { 0, 0 };
	int block, res, ret = 0, i = 0;
	int write_oob = !(chip->bbt_options & NAND_BBT_NO_OOB_BBM);

	/* Write bad block marker to OOB */
	if (write_oob) {
		struct mtd_oob_ops ops;
		loff_t wr_ofs = ofs;

		ops.datbuf = NULL;
		ops.oobbuf = buf;
		ops.ooboffs = chip->badblockpos;
		if (chip->options & NAND_BUSWIDTH_16) {
			ops.ooboffs &= ~0x01;
			ops.len = ops.ooblen = 2;
		} else {
			ops.len = ops.ooblen = 1;
		}
		ops.mode = MTD_OPS_PLACE_OOB;

		/* Write to first/last page(s) if necessary */
		if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
			wr_ofs += mtd->erasesize - mtd->writesize;
		do {
			res = sfcnand_write_oob(mtd, wr_ofs, &ops);
			if (!ret)
				ret = res;

			i++;
			wr_ofs += mtd->writesize;
		} while ((chip->bbt_options & NAND_BBT_SCAN2NDPAGE) && i < 2);
	}

	/* Update flash-based bad block table */
	if (chip->bbt_options & NAND_BBT_USE_FLASH) {
		res = nand_update_bbt(mtd, ofs);
		if (!ret)
			ret = res;
	}

	if (!ret)
		mtd->ecc_stats.badblocks++;

	return ret;

}

static struct jz_spi_support *sfc_nandflash_probe(u8 *idcode)
{
	int i;
	struct jz_spi_support *params;

	for (i = 0; i < ARRAY_SIZE(jz_spi_nand_support_table); i++) {
		params = &jz_spi_nand_support_table[i];
		if ( (params->id_manufactory == idcode[0]) && (params->id_device == idcode[1]) )
			break;
	}

	if (i == ARRAY_SIZE(jz_spi_nand_support_table)) {
		printf("ingenic: Unsupported ID %02x\n", idcode[0]);
		return NULL;
	}

	return params;
}

#define IDCODE_LEN (IDCODE_CONT_LEN + IDCODE_PART_LEN)
int jz_sfc_nand_init(int sfc_quad_mode)
{
	u8 idcode[IDCODE_LEN + 1];
	struct nand_chip *chip;
	struct mtd_info *mtd;
	struct jz_spi_support *spi_flash;
	mtd = &nand_info[0];
	sfc_for_nand_init(sfc_quad_mode);
	read_sfcnand_id(idcode,2);

	spi_flash = sfc_nandflash_probe(idcode);

	chip = malloc(sizeof(struct nand_chip));
	if (!chip)
		return -ENOMEM;
	memset(chip,0,sizeof(struct nand_chip));

	mtd->size = spi_flash->size;
	mtd->flags |= MTD_CAP_NANDFLASH;
	mtd->erasesize = spi_flash->block_size;
	mtd->writesize = spi_flash->page_size;
	mtd->oobsize = spi_flash->oobsize;

	column_cmdaddr_bits = spi_flash->column_cmdaddr_bits;

	chip->select_chip = NULL;
	chip->badblockbits = 8;
	chip->scan_bbt = nand_default_bbt;
	chip->block_bad = jz_sfcnand_block_bad_check;
	chip->block_markbad = jz_sfcnand_block_markbad;
	chip->ecc.layout= &gd5f_ecc_layout_128; // for erase ops
	chip->bbt_erase_shift = chip->phys_erase_shift = ffs(mtd->erasesize) - 1;

	if (!(chip->options & NAND_OWN_BUFFERS))
		chip->buffers = memalign(ARCH_DMA_MINALIGN,sizeof(*chip->buffers));

	/* Set the bad block position */
	if (mtd->writesize > 512 || (chip->options & NAND_BUSWIDTH_16))
		    chip->badblockpos = NAND_LARGE_BADBLOCK_POS;
	else
		    chip->badblockpos = NAND_SMALL_BADBLOCK_POS;


	mtd->priv = chip;

	jz_sfcnand_ext_init();
	mtd_sfcnand_init(mtd);
	nand_register(0);

	return 0;
}
static int mtd_sfcnand_partition_analysis(/*MTDPartitionInfo *pinfo*/)
{
	char mtdparts_env[X_ENV_LENGTH];
	char command[X_COMMAND_LENGTH];
	int ptcount = ARRAY_SIZE(jz_mtd_spinand_partition);
	int part, ret;

	memset(mtdparts_env, 0, X_ENV_LENGTH);
	memset(command, 0, X_COMMAND_LENGTH);

	/*MTD part*/
	sprintf(mtdparts_env, "mtdparts=nand:");
	for (part = 0; part < ptcount; part++) {
		if (jz_mtd_spinand_partition[part].size == -1) {
			sprintf(mtdparts_env,"%s-(%s)", mtdparts_env,
					jz_mtd_spinand_partition[part].name);
			break;
		} else if (jz_mtd_spinand_partition[part].size != 0) {
			sprintf(mtdparts_env,"%s%dM(%s),", mtdparts_env,
					jz_mtd_spinand_partition[part].size / 0x100000,
					jz_mtd_spinand_partition[part].name);
		} else
			break;
	}
	debug("env:mtdparts=%s\n", mtdparts_env);
	setenv("mtdids", mtdids_default);
	setenv("mtdparts", mtdparts_env);
	setenv("partition", NULL);
}
extern struct jz_spinand_partition *get_partion_index(u32 startaddr,int *pt_index);

int mtd_sfcnand_probe_burner(/*MTDPartitionInfo *pinfo,*/int erase_mode,int sfc_quad_mode)
{
	int ret;
	struct mtd_info *mtd;
	mtd = &nand_info[0];
	struct nand_chip *chip;
	unsigned int i=0;
	//int wppin = pinfo->gpio_wp;
	printf("========>>>>>>>> erase_mode = %d \n",erase_mode);
	ret = jz_sfc_nand_init(sfc_quad_mode);


	/*0: none 1, force-erase*/
	if (erase_mode == 1)
		ret = run_command("nand scrub.chip -y", 0);

	chip = mtd->priv;
	chip->scan_bbt(mtd);
	chip->options |= NAND_BBT_SCANNED;
/*	sfc_nand_read(mtd,0x100000,0,2048,(u_char *)0x80100000);
	printf("\n");
	for(i=0;i<2048;)
	{
		printf("%02x,",((unsigned char *)0x80100000)[i]);
		i++;
		if(i%12==0)printf("\n");
	}
	printf("\n");*/
	mtd_sfcnand_partition_analysis(/*pinfo*/);
	return 0;
}
