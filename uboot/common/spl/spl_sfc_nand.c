#include <common.h>
#include <config.h>
#include <spl.h>
#include <asm/io.h>
#include <nand.h>
#include <asm/arch/clk.h>
#include <asm/arch/base.h>
#include <asm/arch/sfc.h>
//#include <asm/arch/spi.h>
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

#define SSI_BASE SSI0_BASE
#define SPL_TYPE_FLAG_LEN	6


static uint32_t jz_sfc_readl(unsigned int offset)
{
	return readl(SFC_BASE + offset);
}

static void jz_sfc_writel(unsigned int value, unsigned int offset)
{
	writel(value, SFC_BASE + offset);
}


unsigned int sfc_fifo_num()
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_SR);
	tmp &= (0x7f << 16);
	tmp = tmp >> 16;
	return tmp;
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
void sfc_set_mode(int channel, int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~(TRAN_MODE_MSK);
	tmp |= (value << TRAN_MODE_OFFSET);
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
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
void sfc_send_cmd(unsigned char *cmd,unsigned int len,unsigned int addr ,unsigned addr_len,unsigned dummy_byte,unsigned int daten,unsigned char dir)
{
	struct jz_sfc sfc;
	unsigned int reg_tmp = 0;
	sfc.cmd = *cmd;
	sfc.addr_len = addr_len;
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
	if((daten == 0))
	{
		reg_tmp = jz_sfc_readl(SFC_SR);
		while (!(reg_tmp & END)){
			reg_tmp = jz_sfc_readl(SFC_SR);
		}
		if ((jz_sfc_readl(SFC_SR)) & END)
			jz_sfc_writel(CLR_END,SFC_SCR);
	}
}
static int sfc_nand_read_data(unsigned int *data, unsigned int length)
{
	unsigned int tmp_len = 0;
	unsigned int fifo_num = 0;
	unsigned int i;
	unsigned int reg_tmp = 0;
	unsigned int  len = (length + 3) / 4 ;
	unsigned int time_out = 1000;

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

static void get_sfcnand_base_param(int *pagesize)
{
	unsigned char *spl_flag = (unsigned char *)0xF4001000;
	int type_len = SPL_TYPE_FLAG_LEN;

	*pagesize = spl_flag[type_len + 5] * 1024;//pagesize off 5,blocksize off 4
}

static int sfc_read_page(unsigned int page,unsigned char *dst_addr,int pagesize)
{
	unsigned char cmd[5];
	unsigned int src_addr;
	volatile unsigned int read_buf=0;
	u_char dummy;
	int column = 0;
	int t_read = 120;
	unsigned int i=0;

	/* the paraterms is
	* cmd , datelen,
	* addr,addr_len
	* dummy_byte, daten
	* dir 0,read 1.write
	*
	* */
	cmd[0]=0x13;//
	sfc_send_cmd(&cmd[0],0,page,3,0,0,0);

	cmd[0]=0x0f;//get feature
	sfc_send_cmd(&cmd[0],1,0xc0,1,0,1,0);
	sfc_nand_read_data(&read_buf,1);

	while((read_buf & 0x1))
	{
		cmd[0]=0x0f;//get feature
		sfc_send_cmd(&cmd[0],1,0xc0,1,0,1,0);
		sfc_nand_read_data(&read_buf,1);
	}
	if((read_buf & 0x30) == 0x20)
	{
		printf("read error pageid\n");
		return -1;
	}
	cmd[0]=0x03;//get feature
	column=(column<<8)&0xffffff00;
	sfc_send_cmd(&cmd[0],pagesize,column,3,0,1,0);
	sfc_nand_read_data(dst_addr,pagesize);
//	printf("---------column=%d,dst_addr=%x,pagesize=%d\n",column,dst_addr,pagesize);
}

void sfc_nand_load(long offs,long size,void *dst)
{
	int pagesize,page;
	int pagecopy_cnt = 0;
	unsigned int i=0;
	get_sfcnand_base_param(&pagesize);
	unsigned char *buf=dst;
	page = offs / pagesize;
	while (pagecopy_cnt * pagesize < size) {
		sfc_read_page(page,(unsigned char *)dst,pagesize);

		dst += pagesize;
		page++;
		pagecopy_cnt++;
	}
	return ;
}

static void sfc_init(void)
{
	//clk_set_rate(SSI, 24000000);
	unsigned int i;
	volatile unsigned int tmp;

	sfc_rate = 70000000;
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
	}
	jz_sfc_writel((CLR_END | CLR_TREQ | CLR_RREQ | CLR_OVER | CLR_UNDER),SFC_INTC);
	jz_sfc_writel(0,SFC_CGE);
	tmp = jz_sfc_readl(SFC_GLB);
	tmp &= ~(THRESHOLD_MSK);
	tmp |= (THRESHOLD << THRESHOLD_OFFSET);
	jz_sfc_writel(tmp,SFC_GLB);


}
static void gpio_as_sfc(void)
{
	writel(0x3c << 26,GPIO_PXINTC(0));
	writel(0x3c << 26,GPIO_PXMSKC(0));
	writel(0x3c << 26,GPIO_PXPAT1S(0));
	writel(0x3c << 26,GPIO_PXPAT0C(0));
}
void spl_sfc_nand_load_image(void)
{
	struct image_header *header;
	char cmd[5] = {0};
	char idcode[5] = {0};
	unsigned int i=0;
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

	sfc_init();

	spl_parse_image_header(header);
	sfc_nand_load(CONFIG_UBOOT_OFFSET,CONFIG_SYS_MONITOR_LEN,(void *)CONFIG_SYS_TEXT_BASE);
	/*sfc_read_page(0x100000/2048,0x80100000,2048);
	for(i=0;i<2048;)
	{
		printf("%x",((unsigned char *)0x80100000)[i]);
		i++;
		if(i%12==0)printf("\n");
	}
	printf("\n");
	while(1);*/
}

