#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/mtd/mtd.h>
#include <linux/bitops.h>
#include "nand_api.h"
#include "nandinterface.h"
#include "vnandinfo.h"
#include "nand2mtd.h"

extern NandInterface jz_nand_interface;
static struct mtd_info Mtd;
PPartition *ppt;
VNandManager vNand;

static PPartArray* get_parry(void)
{
	int ret;

	ret = jz_nand_interface.iInitNand(&vNand);
	if (ret != 0) {
		printk("get parry failed\n");
		return NULL;
	}
	
	return vNand.pt;
}

static PPartition* get_partition(void)
{
	int p_num = 0;
	int i;
	PPartition *ppt;

	PPartArray *p_arry = get_parry();
	if (NULL == p_arry)
		return NULL;
	
	p_num = p_arry->ptcount;
	ppt = p_arry->ppt;

	for(i = 0; i < p_num; i++) {
		printk("partiton name = %s\n",ppt[i].name);
		if(!strncmp(ppt[i].name, CONFIG_APANIC_PLABEL,sizeof(CONFIG_APANIC_PLABEL)))
			break;
	}

	if (p_num == i) {
		printk("can not found apanic partiton\n");
		return NULL;
	}

	return &ppt[i];
}

static int block_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	BlockList blocklist;
	int ret;

	blocklist.startBlock = instr->addr >> mtd->erasesize_shift;
	blocklist.BlockCount = (int)instr->len / mtd->erasesize;
	blocklist.head.next = 0;

	if (blocklist.BlockCount < 1)
	{
		printk("erase block count error\n");
		return -1;
	}

	ret = jz_nand_interface.iMultiBlockErase(ppt, &blocklist);
	if (ret != 0) {
		printk("ndisk erase err \n");
		dump_stack();
	}

	if (instr->callback)
		instr->callback(instr);

	return ret;

}

static int page_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	int ret;
	int page_id;
	int offset;

	page_id = (int)from / mtd->writesize;
	offset = (int)from % mtd->writesize;

	if (len > mtd->writesize) {
		printk("max len is pagesize");
	}

	ret = jz_nand_interface.iPageRead(ppt, page_id, offset, len, buf);
	if (ret < 0) {
		if (-6 == ret) {
			*retlen = mtd->writesize;
			return 0;
		}
		printk("read page error\n");
		return -1;
	}

	*retlen = ret;

	return 0;
}

static int panic_page_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	int ret;
	int page_id;
	int offset;

	page_id = (int)to / mtd->writesize;
	offset = (int)to % mtd->writesize;

	ret = jz_nand_interface.iPanicPageWrite(ppt, page_id, offset, len, (void*)buf);
	if (ret < 0) {
		printk("write page error\n");
		return -1;
	}

	*retlen = ret;
	return 0;
}

static int block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	ret = jz_nand_interface.iIsBadBlock(ppt, (int)ofs/mtd->erasesize);
	
	return ret;

}

static int block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	ret = jz_nand_interface.iMarkBadBlock(ppt, (int)ofs/mtd->erasesize);

	return ret;
}


static struct mtd_info *nand_mtd(void)
{

	ppt = get_partition();
	if (NULL == ppt)
		return NULL;

	memset(&Mtd, 0, sizeof(Mtd));

	Mtd.size = ppt->PageCount * ppt->byteperpage;
	Mtd.erasesize = ppt->pageperblock * ppt->byteperpage;
	Mtd.writesize =  ppt->byteperpage;
	Mtd.erasesize_shift = ffs(Mtd.erasesize) - 1;
	Mtd.writesize_shift = ffs(Mtd.writesize) - 1;
	Mtd.name = ppt->name;
	Mtd._erase = block_erase;
	Mtd._read = page_read;
	Mtd._write = panic_page_write;
	Mtd._panic_write = panic_page_write;
	Mtd._block_isbad = block_isbad;
	Mtd._block_markbad = block_markbad;

	return &Mtd;
}

void register_mtd_user (struct mtd_notifier *new)
{
	if (NULL == nand_mtd()) {
		printk("get mtd_info failed\n");
		return;
	}

	new->add(&Mtd);

	return;
}
