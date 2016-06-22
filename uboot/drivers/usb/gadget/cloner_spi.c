
#ifdef CONFIG_JZ_SPI
extern unsigned int ssi_rate;


int spi_erase(struct cloner *cloner)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	struct spi_flash *flash;
	struct spi_args *spi_arg = &cloner->args->spi_args;
	spi.rate  = spi_arg->rate ;
	ssi_rate  = spi.rate;

#ifdef CONFIG_JZ_SPI
	spi_init();
#endif

	if(flash == NULL){
		flash = spi_flash_probe(bus, cs, spi.rate, mode);
		if (!flash) {
			printf("Failed to initialize SPI flash at %u:%u\n", bus, cs);
			return 1;
		}
	}
//	printf("######################\n");
	jz_erase_all(flash);
	printf("spi erase ok\n");
	return 0;
}



int spi_program(struct cloner *cloner)
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
	int len = 0;
	struct spi_flash *flash;
	spi.enable = spi_arg->enable;
	spi.clk   = spi_arg->clk;
	spi.data_in  = spi_arg->data_in;
	spi.data_out  = spi_arg->data_out;
	spi.rate  = spi_arg->rate ;
	ssi_rate = spi.rate;

#ifdef CONFIG_JZ_SPI
	spi_init();
#endif

#ifdef CONFIG_INGENIC_SOFT_SPI
	spi_init_jz(&spi);
#endif

	if(flash == NULL){
		flash = spi_flash_probe(bus, cs, spi.rate, mode);
		if (!flash) {
			printf("Failed to initialize SPI flash at %u:%u\n", bus, cs);
			return 1;
		}
	}

	debug("the offset = %x\n",offset);
	debug("the length = %x\n",length);


	if (length%blk_size == 0){
		len = length;
		printf("the length = %x,blk_size = %x\n",length,blk_size);
	}
	else{
		printf("the length = %x, is no enough %x\n",length,blk_size);
		len = (length/blk_size)*blk_size + blk_size;
	}

	if (cloner->args->spi_erase == SPI_NO_ERASE) {
		ret = spi_flash_erase(flash, offset, len);
		printf("SF: %zu bytes @ %#x Erased: %s\n", (size_t)len, (u32)offset,
				ret ? "ERROR" : "OK");
	}

	ret = spi_flash_write(flash, offset, len, addr);
	printf("SF: %zu bytes @ %#x write: %s\n", (size_t)len, (u32)offset,
			ret ? "ERROR" : "OK");


	if (cloner->args->write_back_chk) {
		spi_flash_read(flash, offset,len, addr);

		uint32_t tmp_crc = local_crc32(0xffffffff,addr,cloner->cmd->write.length);
		debug_cond(BURNNER_DEBUG,"%d blocks check: %s\n",len,(cloner->cmd->write.crc == tmp_crc) ? "OK" : "ERROR");
		if (cloner->cmd->write.crc != tmp_crc) {
			printf("src_crc32 = %08x , dst_crc32 = %08x\n",cloner->cmd->write.crc,tmp_crc);
			return -EIO;
		}
	}

#if debug
	int buf_debug[8*1024*1024];
	if (spi_flash_read(flash, 1024, /*len*/2048, buf_debug)) {
		printf("read failed\n");
		return -1;
	}
	int i = 0;
	for(i=0;i<4096;i++){
		printf("the debug[%d] = %x\n",i,buf_debug[i]);
	}

#endif

	return 0;
}
#endif
