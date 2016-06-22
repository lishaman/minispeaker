#include <linux/mtd/mtd.h>
#include <ingenic_nand_mgr/nand_param.h>
#include "cloner.h"
#include "../../mtd/nand/jz_spinand.h"

extern struct jz_spinand_partition *get_partion_index(u32 startaddr,int *pt_index);
int spinand_program(struct cloner *cloner)
{
	u32 length = cloner->cmd->write.length;
	u32 full_size = cloner->full_size;
	void *databuf = (void *)cloner->write_req->buf;
	u32 startaddr = cloner->cmd->write.partation + (cloner->cmd->write.offset);
	char command[128];
	volatile int pt_index;
	struct jz_spinand_partition *partation;
	int ret;

	static int pt_index_bak = -1;
	static char *part_name = NULL;

	partation = get_partion_index(startaddr,&pt_index);

	if((!cloner->args->spi_erase) && (partation->manager_mode != UBI_MANAGER)){
		if(pt_index != pt_index_bak){/* erase part partation */
			pt_index_bak = pt_index;
			memset(command, 0 , 128);
			sprintf(command, "nand erase 0x%x 0x%x", partation->offset,partation->size);
			printf("%s\n", command);
			ret = run_command(command, 0);
			if (ret) goto out;
		}
	}

	memset(command, 0 , 128);

	if(partation->manager_mode == MTD_MODE){
		sprintf(command, "nand write.jffs2 0x%x 0x%x 0x%x", (unsigned)databuf, startaddr, length);
		printf("%s\n", command);
		ret = run_command(command, 0);
		if (ret) goto out;
		printf("...ok\n");
	}else if(partation->manager_mode == UBI_MANAGER){
		if(!(part_name == partation->name) && cloner->args->spi_erase){/* need change part */
			memset(command, 0, 128);
			sprintf(command, "ubi part %s", partation->name);
			printf("%s\n", command);
			ret = run_command(command, 0);
			memset(command, 0, X_COMMAND_LENGTH);
			sprintf(command, "ubi create %s",partation->name,partation->size);
			ret = run_command(command, 0);

			if (ret) {
				printf("error...\n");
				return ret;
			}
			part_name = partation->name;
		}

		if(cloner->full_size && !(cloner->args->spi_erase)){
			memset(command, 0, 128);
			sprintf(command, "ubi part %s", partation->name);
			printf("%s\n", command);
			ret = run_command(command, 0);
		}

		memset(command, 0, 128);
		static wlen = 0;
		wlen += length;
		if (full_size && (full_size <= length)) {
			length = full_size;
			sprintf(command, "ubi write 0x%x %s 0x%x", (unsigned)databuf, partation->name, length);
		} else if (full_size) {
			sprintf(command, "ubi write.part 0x%x %s 0x%x 0x%x",(unsigned)databuf, partation->name, length, full_size);
		} else {
			sprintf(command, "ubi write.part 0x%x %s 0x%x",(unsigned)databuf, partation->name, length);
		}


		ret = run_command(command, 0);
		if (ret) {
			printf("...error\n");
			return ret;
		}
	}
	if(cloner->full_size)
		cloner->full_size = 0;
	return 0;
out:
	printf("...error\n");
	return ret;

}
