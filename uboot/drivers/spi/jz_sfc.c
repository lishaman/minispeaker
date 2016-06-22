/*
 * Ingenic JZ SFC driver
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
 * Author: Tiger <xyfu@ingenic.cn>
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

#include <config.h>
#include <common.h>
#include <spi.h>
#include <spi_flash.h>
#include <malloc.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>
#include <asm/arch/spi.h>
#include <asm/arch/sfc.h>
#include <asm/arch/clk.h>
#include <asm/arch/base.h>
#include <malloc.h>

#include "jz_spi.h"

static struct jz_spi_support gparams;
static struct nor_sharing_params pdata;

struct spi_quad_mode *quad_mode = NULL;
/* wait time before read status (us) for spi nand */
//static int t_reset = 500;
int mode = 0;
int flag = 0;
int sfc_is_init = 0;
unsigned int sfc_rate = 0;
unsigned int sfc_quad_mode = 0;
unsigned int quad_mode_is_set = 0;

struct jz_sfc {
	unsigned int  addr;
	unsigned int  len;
	unsigned int  cmd;
	unsigned int  addr_plus;
	unsigned int  sfc_mode;
	unsigned char daten;
	unsigned char addr_len;
	unsigned char pollen;
	unsigned char phase;
	unsigned char dummy_byte;
};

static uint32_t jz_sfc_readl(unsigned int offset)
{
	return readl(SFC_BASE + offset);
}

static void jz_sfc_writel(unsigned int value, unsigned int offset)
{
	writel(value, SFC_BASE + offset);
}

void dump_sfc_reg(void)
{
	int i = 0;
	printf("SFC_GLB			:%x\n", jz_sfc_readl(SFC_GLB ));
	printf("SFC_DEV_CONF	:%x\n", jz_sfc_readl(SFC_DEV_CONF ));
	printf("SFC_DEV_STA_RT	:%x\n", jz_sfc_readl(SFC_DEV_STA_RT ));
	printf("SFC_DEV_STA_MSK	:%x\n", jz_sfc_readl(SFC_DEV_STA_MSK ));
	printf("SFC_TRAN_LEN		:%x\n", jz_sfc_readl(SFC_TRAN_LEN ));

	for(i = 0; i < 6; i++)
		printf("SFC_TRAN_CONF(%d)	:%x\n", i,jz_sfc_readl(SFC_TRAN_CONF(i)));

	for(i = 0; i < 6; i++)
		printf("SFC_DEV_ADDR(%d)	:%x\n", i,jz_sfc_readl(SFC_DEV_ADDR(i)));

	printf("SFC_MEM_ADDR :%x\n", jz_sfc_readl(SFC_MEM_ADDR));
	printf("SFC_TRIG	 :%x\n", jz_sfc_readl(SFC_TRIG));
	printf("SFC_SR		 :%x\n", jz_sfc_readl(SFC_SR));
	printf("SFC_SCR		 :%x\n", jz_sfc_readl(SFC_SCR));
	printf("SFC_INTC	 :%x\n", jz_sfc_readl(SFC_INTC));
	printf("SFC_FSM		 :%x\n", jz_sfc_readl(SFC_FSM ));
	printf("SFC_CGE		 :%x\n", jz_sfc_readl(SFC_CGE ));

}

unsigned int sfc_fifo_num()
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_SR);
	tmp &= (0x7f << 16);
	tmp = tmp >> 16;
	return tmp;
}


void sfc_set_mode(int channel, int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~(TRAN_MODE_MSK);
	tmp |= (value << TRAN_MODE_OFFSET);
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
}


void sfc_dev_addr_dummy_bytes(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~TRAN_CONF_DMYBITS_MSK;
	tmp |= (value << TRAN_CONF_DMYBITS_OFFSET);
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
}

void sfc_transfer_direction(int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_GLB);
		tmp &= ~TRAN_DIR;
		jz_sfc_writel(tmp,SFC_GLB);
	} else {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_GLB);
		tmp |= TRAN_DIR;
		jz_sfc_writel(tmp,SFC_GLB);
	}
}

void sfc_set_length(int value)
{
	jz_sfc_writel(value,SFC_TRAN_LEN);
}

void sfc_set_addr_length(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~(ADDR_WIDTH_MSK);
	tmp |= (value << ADDR_WIDTH_OFFSET);
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
}

void sfc_cmd_en(int channel, unsigned int value)
{
	if(value == 1) {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
		tmp |= CMDEN;
		jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
	} else {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
		tmp &= ~CMDEN;
		jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
	}
}

void sfc_data_en(int channel, unsigned int value)
{
	if(value == 1) {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
		tmp |= DATEEN;
		jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
	} else {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
		tmp &= ~DATEEN;
		jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
	}
}

void sfc_write_cmd(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~CMD_MSK;
	tmp |= value;
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
}

void sfc_dev_addr(int channel, unsigned int value)
{
	jz_sfc_writel(value, SFC_DEV_ADDR(channel));
}

void sfc_dev_addr_plus(int channel, unsigned int value)
{
	jz_sfc_writel(value,SFC_DEV_ADDR_PLUS(channel));
}

void sfc_set_transfer(struct jz_sfc *hw,int dir)
{
	if(dir == 1)
		sfc_transfer_direction(GLB_TRAN_DIR_WRITE);
	else
		sfc_transfer_direction(GLB_TRAN_DIR_READ);
	sfc_set_length(hw->len);
	sfc_set_addr_length(0, hw->addr_len);
	sfc_cmd_en(0, 0x1);
	sfc_data_en(0, hw->daten);
	sfc_write_cmd(0, hw->cmd);
	sfc_dev_addr(0, hw->addr);
	sfc_dev_addr_plus(0, hw->addr_plus);
	sfc_dev_addr_dummy_bytes(0,hw->dummy_byte);
	sfc_set_mode(0,hw->sfc_mode);

}

static int sfc_read_data(unsigned int *data, unsigned int length)
{
	unsigned int tmp_len = 0;
	unsigned int fifo_num = 0;
	unsigned int i;
	unsigned int reg_tmp = 0;
	unsigned int  len = (length + 3) / 4 ;

	while(1){
		reg_tmp = jz_sfc_readl(SFC_SR);
		if (reg_tmp & RECE_REQ) {
			jz_sfc_writel(CLR_RREQ,SFC_SCR);
			if ((len - tmp_len) > THRESHOLD)
				fifo_num = THRESHOLD;
			else {
				fifo_num = len - tmp_len;
			}

			for (i = 0; i < fifo_num; i++) {
				*data = jz_sfc_readl(SFC_DR);
				data++;
				tmp_len++;
			}
		}
		if (tmp_len == len)
			break;
	}

	reg_tmp = jz_sfc_readl(SFC_SR);
	while (!(reg_tmp & END)){
		reg_tmp = jz_sfc_readl(SFC_SR);
	}

	if ((jz_sfc_readl(SFC_SR)) & END)
		jz_sfc_writel(CLR_END,SFC_SCR);


	return 0;
}


static int sfc_write_data(unsigned int *data, unsigned int length)
{
	unsigned int tmp_len = 0;
	unsigned int fifo_num = 0;
	unsigned int i;
	unsigned int reg_tmp = 0;
	unsigned int  len = (length + 3) / 4 ;

	while(1){
		reg_tmp = jz_sfc_readl(SFC_SR);
		if (reg_tmp & TRAN_REQ) {
			jz_sfc_writel(CLR_TREQ,SFC_SCR);
			if ((len - tmp_len) > THRESHOLD)
				fifo_num = THRESHOLD;
			else {
				fifo_num = len - tmp_len;
			}

			for (i = 0; i < fifo_num; i++) {
				jz_sfc_writel(*data,SFC_DR);
				data++;
				tmp_len++;
			}
		}
		if (tmp_len == len)
			break;
	}

	reg_tmp = jz_sfc_readl(SFC_SR);
	while (!(reg_tmp & END)){
		reg_tmp = jz_sfc_readl(SFC_SR);
	}

	if ((jz_sfc_readl(SFC_SR)) & END)
		jz_sfc_writel(CLR_END,SFC_SCR);


	return 0;
}

int sfc_init(void )
{
	unsigned int i;
	volatile unsigned int tmp;
	int err = 0;
#ifndef CONFIG_BURNER
	sfc_rate = 100000000;
	clk_set_rate(SSI, sfc_rate);
#else
	if(sfc_rate !=0 )
		clk_set_rate(SSI, sfc_rate);
	else{
		printf("this will be an error that the sfc rate is 0\n");
		sfc_rate = 70000000;
		clk_set_rate(SSI, sfc_rate);
	}
#endif

#ifdef CONFIG_SPI_QUAD
	sfc_quad_mode = 1;
#endif

	tmp = jz_sfc_readl(SFC_GLB);
	tmp &= ~(TRAN_DIR | OP_MODE );
	tmp |= WP_EN;
	jz_sfc_writel(tmp,SFC_GLB);

	tmp = jz_sfc_readl(SFC_DEV_CONF);
	tmp &= ~(CMD_TYPE | CPHA | CPOL | SMP_DELAY_MSK |
			THOLD_MSK | TSETUP_MSK | TSH_MSK);
	tmp |= (CEDL | HOLDDL | WPDL | 1 << SMP_DELAY_OFFSET);
	jz_sfc_writel(tmp,SFC_DEV_CONF);

	for (i = 0; i < 6; i++) {
		jz_sfc_writel((jz_sfc_readl(SFC_TRAN_CONF(i))& (~(TRAN_MODE_MSK | FMAT))),SFC_TRAN_CONF(i));
	}

	jz_sfc_writel((CLR_END | CLR_TREQ | CLR_RREQ | CLR_OVER | CLR_UNDER),SFC_INTC);

	jz_sfc_writel(0,SFC_CGE);

	tmp = jz_sfc_readl(SFC_GLB);
	tmp &= ~(THRESHOLD_MSK);
	tmp |= (THRESHOLD << THRESHOLD_OFFSET);
	jz_sfc_writel(tmp,SFC_GLB);

#ifdef CONFIG_JZ_SFC_NOR
	err = sfc_nor_init();
	if(err < 0){
		printf("the sfc quad mode err,check your soft code\n");
		return -1;
	}
	sfc_is_init = 1;
#endif
	return 0;
}

void sfc_send_cmd(unsigned char *cmd,unsigned int len,unsigned int addr ,unsigned addr_len,unsigned dummy_byte,unsigned int daten,unsigned char dir)
{
	struct jz_sfc sfc;
	unsigned int reg_tmp = 0;
	sfc.cmd = *cmd;
	sfc.addr_len = addr_len;
	//sfc.addr = ((*addr << 16) &0x00ff0000) | ((*(addr + 1) << 8)&0x0000ff00) |((*(addr + 2))&0xff);
	sfc.addr = addr;
	sfc.addr_plus = 0;
	sfc.dummy_byte = dummy_byte;
	sfc.daten = daten;
	sfc.len = len;

	if((daten == 1)&&(addr_len != 0)){
		sfc.sfc_mode = mode;
	}else{
		sfc.sfc_mode = 0;
	}
	sfc_set_transfer(&sfc,dir);
	jz_sfc_writel(1 << 2,SFC_TRIG);
	jz_sfc_writel(START,SFC_TRIG);

	/*this must judge the end status*/
	if((daten == 0)){
		reg_tmp = jz_sfc_readl(SFC_SR);
		while (!(reg_tmp & END)){
			reg_tmp = jz_sfc_readl(SFC_SR);
		}

		if ((jz_sfc_readl(SFC_SR)) & END)
			jz_sfc_writel(CLR_END,SFC_SCR);
	}

}
int sfc_nand_write_data(unsigned int *data,unsigned int length)
{
	return sfc_write_data(data,length);
}
int sfc_nand_read_data(unsigned int *data, unsigned int length)
{
	 return sfc_read_data(data,length);
}

#ifdef CONFIG_JZ_SFC_NOR
int jz_sfc_set_address_mode(struct spi_flash *flash, int on)
{
	unsigned char cmd[3];
	unsigned int  buf = 0;

	if(flash->addr_size == 4){
		cmd[0] = CMD_WREN;
		if(on == 1){
			cmd[1] = CMD_EN4B;
		}else{
			cmd[1] = CMD_EX4B;
		}
		cmd[2] = CMD_RDSR;

		sfc_send_cmd(&cmd[0],0,0,0,0,0,1);

		sfc_send_cmd(&cmd[1],0,0,0,0,0,1);

		sfc_send_cmd(&cmd[2], 1,0,0,0,1,0);

		sfc_read_data(&buf, 1);
		while(buf & CMD_SR_WIP) {
			sfc_send_cmd(&cmd[2], 1,0,0,0,1,0);
			sfc_read_data(&buf, 1);
		}
	}
	return 0;
}

int jz_sfc_read(struct spi_flash *flash, u32 offset, size_t len, void *data)
{
	unsigned char cmd[5];
	unsigned long read_len;
	unsigned int i;

	jz_sfc_set_address_mode(flash,1);

	if(sfc_quad_mode == 1){
		cmd[0]  = quad_mode->cmd_read;
		mode = quad_mode->sfc_mode;
	}else{
		cmd[0]  = CMD_READ;
		mode = TRAN_SPI_STANDARD;
	}

	for(i = 0; i < flash->addr_size; i++){
		cmd[i + 1] = offset >> (flash->addr_size - i - 1) * 8;
	}

	read_len = flash->size - offset;

	if(len < read_len)
		read_len = len;
	/* the paraterms is
		 * cmd , len, addr,addr_len
		 * dummy_byte, daten
		 * dir
		 *
		 * */

	if(sfc_quad_mode == 1){
		sfc_send_cmd(&cmd[0],read_len,offset,flash->addr_size,quad_mode->dummy_byte,1,0);
	}else{
		sfc_send_cmd(&cmd[0],read_len,offset,flash->addr_size,0,1,0);
	}
	//	dump_sfc_reg();
	sfc_read_data(data, len);

	jz_sfc_set_address_mode(flash,0);

	return 0;
}

static int jz_sfc_read_norflash_params(struct spi_flash *flash, u32 offset, size_t len, void *data)
{
	unsigned char cmd[5];
	unsigned long read_len;
	unsigned int i;

	cmd[0]  = CMD_READ;
	mode = TRAN_SPI_STANDARD;

	for(i = 0; i < flash->addr_size; i++){
		cmd[i + 1] = offset >> (flash->addr_size - i - 1) * 8;
	}

	read_len = flash->size - offset;

	if(len < read_len)
		read_len = len;

	sfc_send_cmd(&cmd[0],read_len,offset,flash->addr_size,0,1,0);
	sfc_read_data(data, len);

	return 0;
}

int jz_sfc_write(struct spi_flash *flash, u32 offset, size_t length, const void *buf)
{
	unsigned char cmd[7];
	unsigned tmp = 0;
	int chunk_len, actual, i;
	unsigned long byte_addr;
	unsigned char *send_buf = (unsigned char *)buf;
	unsigned int pagelen = 0,len = 0,retlen = 0;

	jz_sfc_set_address_mode(flash,1);

	if (offset + length > flash->size) {
		printf("Data write overflow this chip\n");
		return -1;
	}

	cmd[0] = CMD_WREN;

	if(sfc_quad_mode == 1){
		cmd[1]  = CMD_QPP;
		mode = TRAN_SPI_QUAD;
	}else{
		cmd[1]  = CMD_PP;
		mode = TRAN_SPI_STANDARD;
	}

	cmd[flash->addr_size + 2] = CMD_RDSR;

	while (length) {
		if (length >= flash->page_size)
			pagelen = 0;
		else
			pagelen = length % flash->page_size;

		/* the paraterms is
		 * cmd , len, addr,addr_len
		 * dummy_byte, daten
		 * dir
		 *
		 * */
		sfc_send_cmd(&cmd[0],0,0,0,0,0,1);

		if (!pagelen || pagelen > flash->page_size)
			len = flash->page_size;
		else
			len = pagelen;

		if (offset % flash->page_size + len > flash->page_size)
			len -= offset % flash->page_size + len - flash->page_size;

		for(i = 0; i < flash->addr_size; i++){
			cmd[i+2] = offset >> (flash->addr_size - i - 1) * 8;
		}

		sfc_send_cmd(&cmd[1], len,offset,flash->addr_size,0,1,1);

	//	dump_sfc_reg();

		sfc_write_data(send_buf ,len);

		retlen = len;

		/*polling*/
		sfc_send_cmd(&cmd[flash->addr_size + 2],1,0,0,0,1,0);
		sfc_read_data(&tmp, 1);
		while(tmp & CMD_SR_WIP) {
			sfc_send_cmd(&cmd[flash->addr_size + 2],1,0,0,0,1,0);
			sfc_read_data(&tmp, 1);
		}

		if (!retlen) {
			printf("spi nor write failed\n");
			return -1;
		}

		offset += retlen;
		send_buf += retlen;
		length -= retlen;
	}

	jz_sfc_set_address_mode(flash,0);
	return 0;
}


int jz_sfc_chip_erase()
{

	/* the paraterms is
	 * cmd , len, addr,addr_len
	 * dummy_byte, daten
	 * dir
	 *
	 * */

	unsigned char cmd[6];
	cmd[0] = CMD_WREN;
	cmd[1] = CMD_ERASE_CE;
	cmd[5] = CMD_RDSR;
	unsigned int  buf = 0;
	int err = 0;

	if(sfc_is_init == 0){
		err = sfc_init();
		if(err < 0){
			printf("the quad mode is not support\n");
			return -1;
		}
	}

	if(sfc_quad_mode == 1){
		if(quad_mode_is_set == 0){
			sfc_set_quad_mode();
		}
	}

	jz_sfc_writel(1 << 2,SFC_TRIG);
	sfc_send_cmd(&cmd[0],0,0,0,0,0,1);

	sfc_send_cmd(&cmd[1],0,0,0,0,0,1);

	sfc_send_cmd(&cmd[5], 1,0,0,0,1,0);
	sfc_read_data(&buf, 1);
	printf("sfc start chip erase\n");
	while(buf & CMD_SR_WIP) {
		sfc_send_cmd(&cmd[5], 1,0,0,0,1,0);
		sfc_read_data(&buf, 1);
	}
	printf("########## chip erase ok ######### \n");
	return 0;
}

int jz_sfc_erase(struct spi_flash *flash, u32 offset, size_t len)
{
	unsigned long erase_size;
	unsigned char cmd[7];
	unsigned int  buf = 0, i;


	jz_sfc_set_address_mode(flash,1);

	if((len >= 0x10000)&&((offset % 0x10000) == 0)){
		erase_size = 0x10000;
	}else if((len >= 0x8000)&&((offset % 0x8000) == 0)){
		erase_size = 0x8000;
	}else{
		erase_size = 0x1000;
	}

	if(len % erase_size != 0){
		len = len - (len % erase_size) + erase_size;
	}

	cmd[0] = CMD_WREN;

	switch(erase_size) {
	case 0x1000 :
		cmd[1] = CMD_ERASE_4K;
		break;
	case 0x8000 :
		cmd[1] = CMD_ERASE_32K;
		break;
	case 0x10000 :
		cmd[1] = CMD_ERASE_64K;
		break;
	default:
		printf("unknown erase size !\n");
		return -1;
	}

	cmd[flash->addr_size + 2] = CMD_RDSR;

	while(len > 0) {
		for(i = 0; i < flash->addr_size; i++){
			cmd[i+2] = offset >> (flash->addr_size - i - 1) * 8;
		}

		//	printf("erase %x %x %x %x %x %x %x \n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], offset);

		/* the paraterms is
		 * cmd , len, addr,addr_len
		 * dummy_byte, daten
		 * dir
		 *
		 * */
		sfc_send_cmd(&cmd[0],0,0,0,0,0,1);

		sfc_send_cmd(&cmd[1],0,offset,flash->addr_size,0,0,1);

		sfc_send_cmd(&cmd[flash->addr_size + 2], 1,0,0,0,1,0);
		sfc_read_data(&buf, 1);
		while(buf & CMD_SR_WIP) {
			sfc_send_cmd(&cmd[flash->addr_size + 2], 1,0,0,0,1,0);
			sfc_read_data(&buf, 1);
		}

		offset += erase_size;
		len -= erase_size;
	}

	jz_sfc_set_address_mode(flash,0);
	return 0;
}

void sfc_set_quad_mode()
{
	/* the paraterms is
	 * cmd , len, addr,addr_len
	 * dummy_byte, daten
	 * dir
	 *
	 * */
	unsigned char cmd[5];
	unsigned int buf = 0;
	unsigned int tmp = 0;
	int i = 10;

	if(quad_mode != NULL){
		cmd[0] = CMD_WREN;
		cmd[1] = quad_mode->WRSR_CMD;
		cmd[2] = quad_mode->RDSR_CMD;
		cmd[3] = CMD_RDSR;

		sfc_send_cmd(&cmd[0],0,0,0,0,0,1);

		sfc_send_cmd(&cmd[1],quad_mode->WD_DATE_SIZE,0,0,0,1,1);
		sfc_write_data(&quad_mode->WRSR_DATE,1);

		sfc_send_cmd(&cmd[3],1,0,0,0,1,0);
		sfc_read_data(&tmp, 1);

		while(tmp & CMD_SR_WIP) {
			sfc_send_cmd(&cmd[3],1,0,0,0,1,0);
			sfc_read_data(&tmp, 1);
		}

		sfc_send_cmd(&cmd[2], quad_mode->RD_DATE_SIZE,0,0,0,1,0);
		sfc_read_data(&buf, 1);
		while(!(buf & quad_mode->RDSR_DATE)&&((i--) > 0)) {
			sfc_send_cmd(&cmd[2], quad_mode->RD_DATE_SIZE,0,0,0,1,0);
			sfc_read_data(&buf, 1);
		}

		quad_mode_is_set = 1;
		printf("set quad mode is enable.the buf = %x\n",buf);
	}else{

		printf("the quad_mode is NULL,the nor flash id we not support\n");
	}
}

void sfc_nor_RDID(unsigned int *idcode)
{
	/* the paraterms is
	 * cmd , len, addr,addr_len
	 * dummy_byte, daten
	 * dir
	 *
	 * */
	unsigned char cmd[1];
//	unsigned char chip_id[4];
	unsigned int chip_id = 0;
	cmd[0] = CMD_RDID;
	sfc_send_cmd(&cmd[0],3,0,0,0,1,0);
	sfc_read_data(&chip_id, 1);
//	*idcode = chip_id[0];
	*idcode = chip_id & 0x00ffffff;
}

static void dump_norflash_params(void)
{
	int i;
	printf(" =================================================\n");
	printf(" ======   gparams->name = %s\n",gparams.name);
	printf(" ======   gparams->id_manufactory = %x\n",gparams.id_manufactory);
	printf(" ======   gparams->page_size = %d\n",gparams.page_size);
	printf(" ======   gparams->sector_size = %d\n",gparams.sector_size);
	printf(" ======   gparams->addr_size = %d\n",gparams.addr_size);
	printf(" ======   gparams->size = %d\n",gparams.size);

	printf(" ======   gparams->quad.dummy_byte = %d\n",gparams.quad_mode.dummy_byte);
	printf(" ======   gparams->quad.RDSR_CMD = %x\n",gparams.quad_mode.RDSR_CMD);
	printf(" ======   gparams->quad.WRSR_CMD = %x\n",gparams.quad_mode.WRSR_CMD);
	printf(" ======   gparams->quad.RDSR_DATE = %x\n",gparams.quad_mode.RDSR_DATE);
	printf(" ======   gparams->quad.WRSR_DATE = %x\n",gparams.quad_mode.WRSR_DATE);
	printf(" ======   gparams->quad.RD_DATE_SIZE = %x\n",gparams.quad_mode.RD_DATE_SIZE);
	printf(" ======   gparams->quad.WD_DATE_SIZE = %x\n",gparams.quad_mode.WD_DATE_SIZE);
	printf(" ======   gparams->quad.cmd_read = %x\n",gparams.quad_mode.cmd_read);
	printf(" ======   gparams->quad.sfc_mode = %x\n",gparams.quad_mode.sfc_mode);

	for(i = 0; i < pdata.norflash_partitions.num_partition_info; i++){
		printf(" ======   mtd_partition[%d] :name = %s ,size = %x ,offset = %x\n"\
			,i\
			,pdata.norflash_partitions.nor_partition[i].name\
			,pdata.norflash_partitions.nor_partition[i].size\
			,pdata.norflash_partitions.nor_partition[i].offset);
	}
	printf(" ======   mtd_partition_num = %d\n",pdata.norflash_partitions.num_partition_info);
	printf(" =================================================\n");
}

#ifdef CONFIG_BURNER
int get_norflash_params_from_burner(unsigned char *addr)
{
	unsigned int idcode,chipnum,i;
	struct norflash_params *tmp;

	unsigned int flash_type = *(unsigned int *)(addr + 4);	//0:nor 1:nand
	if(flash_type == 0){

		sfc_nor_RDID(&idcode);
		printf("the norflash chip_id is %x\n",idcode);

		pdata.magic = NOR_MAGIC;
		pdata.version = CONFIG_NOR_VERSION;

		chipnum = *(unsigned int *)(addr + 8);
		for(i = 0; i < chipnum; i++){
			tmp = (struct norflash_params *)(addr + 12 +sizeof(struct norflash_params) * i);
			if(tmp->id == idcode) {
				memcpy(&pdata.norflash_params,tmp,sizeof(struct norflash_params));
				printf("-----break,break,break,break %x\n",idcode);
				break;
			}
		}

		if(i == chipnum){
			printf("none norflash support for the table ,please check the burner norflash supprot table\n");
			return -1;
		}

		pdata.norflash_partitions.num_partition_info = *(unsigned int *)(addr + 12 + sizeof(struct norflash_params) * chipnum);
		memcpy(&pdata.norflash_partitions.nor_partition[0] ,addr + 12 +sizeof(struct norflash_params) * chipnum + 4, \
				sizeof(struct nor_partition) * pdata.norflash_partitions.num_partition_info);
	}else{
		printf("the params recive from cloner not't the norflash\n");
		return -1;
	}
	return 0;
}

static void write_norflash_params_to_spl(unsigned int addr)
{
	memcpy(addr+CONFIG_SPIFLASH_PART_OFFSET,&pdata,sizeof(struct nor_sharing_params));
}

unsigned int get_partition_index(u32 offset, int *pt_offset, int *pt_size)
{
	int i;
	for(i = 0; i < pdata.norflash_partitions.num_partition_info; i++){
		if(offset >= pdata.norflash_partitions.nor_partition[i].offset && \
				offset < (pdata.norflash_partitions.nor_partition[i].offset + \
				pdata.norflash_partitions.nor_partition[i].size)){
			*pt_offset = pdata.norflash_partitions.nor_partition[i].offset;
			*pt_size = pdata.norflash_partitions.nor_partition[i].size;
			break;
		}
	}
	return i;
}
#endif

static int sfc_nor_read_params(void);
int sfc_nor_init()
{
#ifndef CONFIG_BURNER
	sfc_nor_read_params();
#endif

	if(pdata.magic == NOR_MAGIC) {
		if(pdata.version == CONFIG_NOR_VERSION){
			memcpy(gparams.name,pdata.norflash_params.name,32);
			gparams.id_manufactory = pdata.norflash_params.id;
			gparams.page_size = pdata.norflash_params.pagesize;
			gparams.sector_size = pdata.norflash_params.sectorsize;
			gparams.addr_size = pdata.norflash_params.addrsize;
			gparams.size = pdata.norflash_params.chipsize;
			gparams.quad_mode = pdata.norflash_params.quad_mode;

			quad_mode = &gparams.quad_mode;
			//dump_norflash_params();
		}else{
			printf("EEEOR:norflash version mismatch,current version is %d.%d.%d,but the burner version is %d.%d.%d\n"\
					,CONFIG_NOR_MAJOR_VERSION_NUMBER,CONFIG_NOR_MINOR_VERSION_NUMBER,CONFIG_NOR_REVERSION_NUMBER\
					,pdata.version & 0xff,(pdata.version & 0xff00) >> 8,(pdata.version & 0xff0000) >> 16);
			return -1;
		}
	} else {
		int i = 0;
		unsigned int idcode;
		sfc_nor_RDID(&idcode);

		for (i = 0; i < ARRAY_SIZE(jz_spi_support_table); i++) {
			gparams = jz_spi_support_table[i];
			if (gparams.id_manufactory == idcode){
				printf("the id code = %x, the flash name is %s\n",idcode,gparams.name);
				if(sfc_quad_mode == 1){
					quad_mode = &jz_spi_support_table[i].quad_mode;
				}
				break;
			}
		}

		if (i == ARRAY_SIZE(jz_spi_support_table)) {
			if ((idcode != 0)&&(idcode != 0xff)&&(sfc_quad_mode == 0)){
				printf("the id code = %x\n",idcode);
				printf("unsupport ID is if the id not be 0x00,the flash is ok for burner\n");
			}else{
				printf("ingenic: sfc quad mode Unsupported ID %x\n", idcode);
				return -1;
			}
		}
	}
	return 0;
}

static int sfc_nor_read_params(void)
{
	int ret;
	struct spi_flash flash;

	flash.addr_size = 3;//default for read params

	jz_sfc_writel(1 << 2,SFC_TRIG);

	ret = jz_sfc_read_norflash_params(&flash,CONFIG_SPIFLASH_PART_OFFSET,sizeof(struct nor_sharing_params),&pdata);
	if (ret) {
		printf("sfc read error\n");
		return -1;
	}

	return 0;
}

int sfc_nor_read(unsigned int src_addr, unsigned int count,unsigned int dst_addr)
{

	int i,j;
	unsigned char *data;
	unsigned int temp;
	int ret = 0,err = 0;
	unsigned int spl_len = 0,words_of_spl;
	int addr_len;
	struct spi_flash flash;

	flag = 0;

#ifndef CONFIG_BURNER
	if(pdata.norflash_partitions.num_partition_info && (pdata.norflash_partitions.num_partition_info != 0xffffffff)) {
		for(i = 0; i < pdata.norflash_partitions.num_partition_info; i++){
			if(src_addr >= pdata.norflash_partitions.nor_partition[i].offset && \
					src_addr < (pdata.norflash_partitions.nor_partition[i].offset + \
						pdata.norflash_partitions.nor_partition[i].size) && \
					(pdata.norflash_partitions.nor_partition[i].mask_flags & NORFLASH_PART_WO)){
				printf("the partiton can't read,please check the partition RW mode\n");
				return 0;
			}
		}
	}
#endif

#ifdef CONFIG_SPI_QUAD
	sfc_quad_mode = 1;
#endif

	if(sfc_is_init == 0){
    	err = sfc_init();
		if(err < 0){
			printf("the quad mode is not support\n");
			return -1;
		}
	}

	flash.page_size = gparams.page_size;
	flash.sector_size = gparams.sector_size;
	flash.size = gparams.size;
	flash.addr_size = gparams.addr_size;

	if(sfc_quad_mode == 1){
		if(quad_mode_is_set == 0){
			sfc_set_quad_mode();
		}
	}

	jz_sfc_writel(1 << 2,SFC_TRIG);

	ret = jz_sfc_read(&flash,src_addr,count,dst_addr);
	if (ret) {
		printf("sfc read error\n");
		return -1;
	}

	return 0;
}

#define NV_AREA_START_ADDR (288 * 1024)
static void nv_map_area_addr(unsigned int *base_addr)
{
	unsigned int buf[3][2];
	unsigned int tmp_buf[4];
	unsigned int nv_num = 0, nv_count = 0;
	unsigned int addr, i;

	for(i = 0; i < 3; i++) {
		addr = NV_AREA_START_ADDR + i * 32 * 1024;
		sfc_nor_read(addr, 4, buf[i]);
		if(buf[i][0] == 0x5a5a5a5a) {
			sfc_nor_read(addr + 1 *1024,  16, tmp_buf);
			addr += 32 * 1024 - 8;
			sfc_nor_read(addr, 8, buf[i]);
			if(buf[i][1] == 0xa5a5a5a5) {
				if(nv_count < buf[i][0]) {
					nv_count = buf[i][0];
					nv_num = i;
				}
			}
		}
	}
	*base_addr = NV_AREA_START_ADDR + nv_num * 32 * 1024 + 1024;
}

unsigned int get_update_flag()
{
	unsigned nv_buf[4];
	int count = 16;
	unsigned int src_addr, update_flag;
	nv_map_area_addr((unsigned int)&src_addr);
	sfc_nor_read(src_addr, count, nv_buf);
	update_flag = nv_buf[3];

	return update_flag;
}

int get_battery_flag(void)
{
	unsigned char buf [64];
	unsigned char battery_flag[3];
	unsigned int src_addr;

	nv_map_area_addr((unsigned int)&src_addr);
	sfc_nor_read(src_addr, 64, buf);
	battery_flag[0] = buf[32];
	battery_flag[1] = buf[33];
	battery_flag[2] = buf[34];

	if ((battery_flag[0] == 0x69) && (battery_flag[1] == 0xaa)
			&& (battery_flag[2] == 0x55))
		return 1;
	else
		return 0;
}

int sfc_nor_write(unsigned int src_addr, unsigned int count,unsigned int dst_addr,unsigned int erase_en)
{

	int i,j;
	unsigned char *data;
	int ret = 0,err = 0;
	struct spi_flash flash;

#ifndef CONFIG_BURNER
	if(pdata.norflash_partitions.num_partition_info && (pdata.norflash_partitions.num_partition_info != 0xffffffff)) {
		for(i = 0; i < pdata.norflash_partitions.num_partition_info; i++){
			if(src_addr >= pdata.norflash_partitions.nor_partition[i].offset && \
					src_addr < (pdata.norflash_partitions.nor_partition[i].offset + \
						pdata.norflash_partitions.nor_partition[i].size) && \
					(pdata.norflash_partitions.nor_partition[i].mask_flags & NORFLASH_PART_RO)){
				printf("the partiton can't write,please check the partition RW mode\n");
				return 0;
			}
		}
	}
#endif

#ifdef CONFIG_SPI_QUAD
	sfc_quad_mode = 1;
#endif

	if(sfc_is_init == 0){
		err = sfc_init();
		if(err < 0){
			printf("the quad mode is not support\n");
			return -1;
		}
	}

	flash.page_size = gparams.page_size;
	flash.sector_size = gparams.sector_size;
	flash.size = gparams.size;
	flash.addr_size = gparams.addr_size;

	if(sfc_quad_mode == 1){
		if(quad_mode_is_set == 0){
			sfc_set_quad_mode();
		}
	}

#ifdef CONFIG_BURNER
	if(src_addr == 0){
		write_norflash_params_to_spl(dst_addr);
	}
#endif

	jz_sfc_writel(1 << 2,SFC_TRIG);

	if(erase_en == 1){
		jz_sfc_erase(&flash,src_addr,count);
		printf("sfc erase ok\n");
	}

	ret = jz_sfc_write(&flash,src_addr,count,dst_addr);
	if (ret) {
		printf("sfc write error\n");
		return -1;
	}

	return 0;
}

int sfc_nor_erase(unsigned int src_addr, unsigned int count)
{

	int i,j;
	unsigned char *data;
	unsigned int temp;
	int sfc_mode = 0;
	int ret = 0;
	int err = 0;
	unsigned int spl_len = 0,words_of_spl;
	struct spi_flash flash;

#ifndef CONFIG_BURNER
	if(pdata.norflash_partitions.num_partition_info && (pdata.norflash_partitions.num_partition_info != 0xffffffff)) {
		for(i = 0; i < pdata.norflash_partitions.num_partition_info; i++){
			if(src_addr >= pdata.norflash_partitions.nor_partition[i].offset && \
					src_addr < (pdata.norflash_partitions.nor_partition[i].offset + \
						pdata.norflash_partitions.nor_partition[i].size) && \
					(pdata.norflash_partitions.nor_partition[i].mask_flags & NORFLASH_PART_RO)){
				printf("the partiton can't erase,please check the partition RW mode\n");
				return 0;
			}
		}
	}
#endif

#ifdef CONFIG_SPI_QUAD
	sfc_quad_mode = 1;
#endif

	if(sfc_is_init == 0){
		sfc_init();
		if(err < 0){
			printf("the quad mode is not support\n");
			return -1;
		}
	}


	flash.page_size = gparams.page_size;
	flash.sector_size = gparams.sector_size;
	flash.size = gparams.size;
	flash.addr_size = gparams.addr_size;

	if(sfc_quad_mode == 1){
		if(quad_mode_is_set == 0){
			sfc_set_quad_mode();
		}
	}

	jz_sfc_writel(1 << 2,SFC_TRIG);
	ret = jz_sfc_erase(&flash,src_addr,count);
	if (ret) {
		printf("sfc erase error\n");
		return -1;
	}

	return 0;
}
#endif

void sfc_for_nand_init(int sfc_quad_mode)
{
	unsigned int i;
	volatile unsigned int tmp;
	sfc_rate = 100000000;
	clk_set_rate(SSI, sfc_rate);

	tmp = jz_sfc_readl(SFC_GLB);
	tmp &= ~(TRAN_DIR | OP_MODE );
	tmp |= WP_EN;
	jz_sfc_writel(tmp,SFC_GLB);
	tmp = jz_sfc_readl(SFC_DEV_CONF);
	tmp &= ~(CMD_TYPE | CPHA | CPOL | SMP_DELAY_MSK |
				THOLD_MSK | TSETUP_MSK | TSH_MSK);
	tmp |= (CEDL | HOLDDL | WPDL | 1 << SMP_DELAY_OFFSET);
	jz_sfc_writel(tmp,SFC_DEV_CONF);
	for (i = 0; i < 6; i++) {
		jz_sfc_writel((jz_sfc_readl(SFC_TRAN_CONF(i))& (~(TRAN_MODE_MSK | FMAT))),SFC_TRAN_CONF(i));
	     if(sfc_quad_mode==1)
	     {
		unsigned int temp=0;
		temp=jz_sfc_readl(SFC_TRAN_CONF(i));
		temp&=~(7<<29);
		temp|=(5<<29);
		jz_sfc_writel(temp,SFC_TRAN_CONF(i));
	     }
	}
	jz_sfc_writel((CLR_END | CLR_TREQ | CLR_RREQ | CLR_OVER | CLR_UNDER),SFC_INTC);
	jz_sfc_writel(0,SFC_CGE);
	tmp = jz_sfc_readl(SFC_GLB);
	tmp &= ~(THRESHOLD_MSK);
	tmp |= (THRESHOLD << THRESHOLD_OFFSET);
	jz_sfc_writel(tmp,SFC_GLB);

}
int read_sfcnand_id(u8 *response,size_t len)
{
	/* the paraterms is
	* cmd , len, addr,addr_len
	* dummy_byte, daten
	* dir
	*
	* */
	unsigned char cmd[1];
	//  unsigned char chip_id[4];
	unsigned int chip_id = 0;
	cmd[0] = CMD_RDID;
	sfc_send_cmd(&cmd[0],len,0,1,0,1,0);
	sfc_read_data(response,len);
	printf("id0=%02x\n",response[0]);
	printf("id1=%02x\n",response[1]);
	printf("SFC_DEV_STA_RT=0x%08x,\n",jz_sfc_readl(SFC_DEV_STA_RT));
	//  *idcode = chip_id[0];
}

