

#ifdef CONFIG_JZ_SFC
extern unsigned int sfc_rate;
extern unsigned int sfc_quad_mode;
extern int sfc_is_init;

int sfc_erase(struct cloner *cloner)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	int err = 0;
	struct spi_args *spi_arg = &cloner->args->spi_args;
	sfc_quad_mode = spi_arg->sfc_quad_mode;
	spi.rate  = spi_arg->rate;
	sfc_rate = spi_arg->sfc_rate;

	if(sfc_is_init == 0){
		err = sfc_init();
		if(err < 0){
			printf("!!!!!!!!!!!!!!!!!%d,%s,the sfc init failed\n",__LINE__,__func__);
			return -1;
		}
	}
	jz_sfc_chip_erase();
	printf("sfc chip erase ok\n");

}
int sfc_program(struct cloner *cloner)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	u32 offset = cloner->cmd->write.partation + cloner->cmd->write.offset;
	u32 length = cloner->cmd->write.length;
	int blk_size = cloner->args->spi_erase_block_siz;
	void *addr = (void *)cloner->write_req->buf;
	struct spi_args *spi_arg = &cloner->args->spi_args;
	unsigned int ret;
	int len = 0,err = 0;
	struct spi_flash *flash;
	spi.enable = spi_arg->enable;
	spi.clk   = spi_arg->clk;
	spi.data_in  = spi_arg->data_in;
	spi.data_out  = spi_arg->data_out;
	sfc_quad_mode = spi_arg->sfc_quad_mode;
	spi.rate  = spi_arg->rate ;
	sfc_rate = spi_arg->sfc_rate;

	if(sfc_is_init == 0){
		printf("in sfc init\n");
		err = sfc_init();
		if(err < 0){
			printf("%d,%s,!!!!!!!!!!!!!!!!!!!!!the sfc init failed\n",__LINE__,__func__);
			return -1;
		}
	}

	printf("the offset = %x\n",offset);
	printf("the length = %x\n",length);

	if (length%blk_size == 0){
		len = length;
		printf("the length = %x\n",len);
	}
	else{
		printf("the length = %x, is no enough %x\n",length,blk_size);
		len = (length/blk_size)*blk_size + blk_size;
	}

	if (cloner->args->spi_erase == SPI_NO_ERASE) {
		ret = sfc_nor_erase(offset, len);
		printf("SF: %zu bytes @ %#x Erased: %s\n", (size_t)len, (u32)offset,
				ret ? "ERROR" : "OK");
	}

	ret = sfc_nor_write(offset, len, addr,0);
	printf("SF: %zu bytes @ %#x write: %s\n", (size_t)len, (u32)offset,
			ret ? "ERROR" : "OK");

	return 0;
}
#endif
