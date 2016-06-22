/* drivers/misc/apanic.c
 *
 * Copyright (C) 2009 Google, Inc.
 * Author: San Mehat <san@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

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
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/mtd/mtd.h>
#include <linux/notifier.h>
#include <linux/mtd/mtd.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/preempt.h>
#include <linux/kthread.h>
#include "nand2mtd.h"


#define PANIC_BLOCK_PATH	"/dev/block/ndapanic"
#define PANIC_CHAR_PATH		"/dev/nand_char"
static mm_segment_t old_fs_record;

extern void ram_console_enable_console(int);

struct panic_header {
	u32 magic;
#define PANIC_MAGIC 0xdeadf00d

	u32 version;
#define PHDR_VERSION   0x01

	u32 console_offset;
	u32 console_length;

	u32 threads_offset;
	u32 threads_length;
};

struct apanic_data {
	struct mtd_info		*mtd;
	struct panic_header	curr;
	void			*bounce;
	struct proc_dir_entry	*apanic_console;
	struct proc_dir_entry	*apanic_threads;
};

static struct apanic_data drv_ctx;
static struct work_struct proc_removal_work;
static DEFINE_MUTEX(drv_mutex);

static unsigned int *apanic_bbt;
static unsigned int apanic_erase_blocks;
static unsigned int apanic_good_blocks;

static void set_bb(unsigned int block, unsigned int *bbt)
{
	unsigned int flag = 1;

	BUG_ON(block >= apanic_erase_blocks);

	flag = flag << (block%32);
	apanic_bbt[block/32] |= flag;
	apanic_good_blocks--;
}

static unsigned int get_bb(unsigned int block, unsigned int *bbt)
{
	unsigned int flag;

	BUG_ON(block >= apanic_erase_blocks);

	flag = 1 << (block%32);
	return apanic_bbt[block/32] & flag;
}

static void alloc_bbt(struct mtd_info *mtd, unsigned int *bbt)
{
	int bbt_size;
	apanic_erase_blocks = (mtd->size)>>(mtd->erasesize_shift);
	bbt_size = (apanic_erase_blocks+32)/32;

	apanic_bbt = kmalloc(bbt_size*4, GFP_KERNEL);
	memset(apanic_bbt, 0, bbt_size*4);
	apanic_good_blocks = apanic_erase_blocks;
}
static void scan_bbt(struct mtd_info *mtd, unsigned int *bbt)
{
	int i;

	for (i = 0; i < apanic_erase_blocks; i++) {
		if (mtd->_block_isbad(mtd, i*mtd->erasesize))
			set_bb(i, apanic_bbt);
	}
}

#define APANIC_INVALID_OFFSET 0xFFFFFFFF

static unsigned int phy_offset(struct mtd_info *mtd, unsigned int offset)
{
	unsigned int logic_block = offset>>(mtd->erasesize_shift);
	unsigned int phy_block;
	unsigned good_block = 0;

	for (phy_block = 0; phy_block < apanic_erase_blocks; phy_block++) {
		if (!get_bb(phy_block, apanic_bbt))
			good_block++;
		if (good_block == (logic_block + 1))
			break;
	}

	if (good_block != (logic_block + 1))
		return APANIC_INVALID_OFFSET;

	return offset + ((phy_block-logic_block)<<mtd->erasesize_shift);
}
#if 0
static void apanic_erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *) done->priv;
	wake_up(wait_q);
}
#endif
static int apanic_proc_read(char *buffer, char **start, off_t offset,
			       int count, int *peof, void *dat)
{
	struct apanic_data *ctx = &drv_ctx;
	size_t file_length;
	off_t file_offset;
	//unsigned int page_no;
	//off_t page_offset;
	int rc;
	//size_t len;
	struct file *file;
	loff_t pos;

	if (!count)
		return 0;

	mutex_lock(&drv_mutex);

	switch ((int) dat) {
	case 1:	/* apanic_console */
		file_length = ctx->curr.console_length;
		file_offset = ctx->curr.console_offset;
		break;
	case 2:	/* apanic_threads */
		file_length = ctx->curr.threads_length;
		file_offset = ctx->curr.threads_offset;
		break;
	default:
		pr_err("Bad dat (%d)\n", (int) dat);
		mutex_unlock(&drv_mutex);
		return -EINVAL;
	}

	if ((offset + count) > file_length) {
		mutex_unlock(&drv_mutex);
		return 0;
	}
#if 0
	/* We only support reading a maximum of a flash page */
	if (count > ctx->mtd->writesize)
		count = ctx->mtd->writesize;
	page_no = (file_offset + offset) / ctx->mtd->writesize;
	page_offset = (file_offset + offset) % ctx->mtd->writesize;


	if (phy_offset(ctx->mtd, (page_no * ctx->mtd->writesize))
		== APANIC_INVALID_OFFSET) {
		pr_err("apanic: reading an invalid address\n");
		mutex_unlock(&drv_mutex);
		return -EINVAL;
	}

#endif
	file = filp_open(PANIC_BLOCK_PATH, O_RDONLY, S_IRWXU);
	if (NULL == file) {
		printk("open /dev/block/ndapanic filed in %s\n",__func__);
		return -EINVAL;
	}

	old_fs_record = get_fs();
	set_fs(KERNEL_DS);

	pos = file_offset + offset;
	rc = vfs_read(file, ctx->bounce, count, &pos);
	if (rc < 0) {
		printk("read /dev/block/ndapanic failed in %s\n",__func__);
		filp_close(file, NULL);
		return -1;
	}

	set_fs(old_fs_record);
	filp_close(file, NULL);
#if 0
	rc = ctx->mtd->read(ctx->mtd,
		phy_offset(ctx->mtd, (page_no * ctx->mtd->writesize)),
		ctx->mtd->writesize,
		&len, ctx->bounce);

	if (page_offset)
		count = count < (ctx->mtd->writesize - page_offset) ?
			count : (ctx->mtd->writesize - page_offset);
	memcpy(buffer, ctx->bounce + page_offset, count);
#endif
	memcpy(buffer, ctx->bounce, rc);

	*start = (char*)rc;

	if ((offset + rc) == file_length)
		*peof = 1;

	mutex_unlock(&drv_mutex);

	return count;
}
#if 1
static void mtd_panic_erase(void)
{
	struct file *file;
	int error = -ENOTTY;
	char *panic = "ndapanic";

#if 0
	file = filp_open(PANIC_CHAR_PATH, O_RDWR, S_IRWXU);
	if (IS_ERR(file)) {
		printk("open /dev/char/ndapanic filed\n");
		return ;
	}
#endif

	int erase_count = 100 , erase_i, erase_flag = 0;

	for(erase_i = 0; erase_i < erase_count; erase_i++) {
		file = filp_open(PANIC_CHAR_PATH, O_RDWR, 0);
		if (IS_ERR(file)) {
			erase_flag++;
			msleep(100);
			continue;
		}
		break;
	}

	if(erase_flag == erase_count) {
		printk("open /dev/char/ndapanic filed in %s\n", __func__);
		return;
	}

	printk("filp_open %s is %d times in %s sucessfully\n", PANIC_CHAR_PATH, erase_i, __func__);

	if (!file->f_op || !file->f_op->unlocked_ioctl) {
		printk("==================no ioctl===================\n");
		goto out;
	}

	error = file->f_op->unlocked_ioctl(file, 98, (unsigned int long)panic);
	if (error < 0){
		error = -EINVAL;
		printk("erase /dev/char/ndapanic filed\n");
	}

 out:
	filp_close(file, NULL);
	return ;
}
#else
static void mtd_panic_erase(void)
{
	struct apanic_data *ctx = &drv_ctx;
	struct erase_info erase;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	int rc, i;

	init_waitqueue_head(&wait_q);
	erase.mtd = ctx->mtd;
	erase.callback = apanic_erase_callback;
	erase.len = ctx->mtd->erasesize;
	erase.priv = (u_long)&wait_q;
	for (i = 0; i < ctx->mtd->size; i += ctx->mtd->erasesize) {
		erase.addr = i;
		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&wait_q, &wait);

		if (get_bb(erase.addr>>ctx->mtd->erasesize_shift, apanic_bbt)) {
			printk(KERN_WARNING
			       "apanic: Skipping erase of bad "
			       "block @%llx\n", erase.addr);
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			continue;
		}

		rc = ctx->mtd->erase(ctx->mtd, &erase);
		if (rc) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			printk(KERN_ERR
			       "apanic: Erase of 0x%llx, 0x%llx failed\n",
			       (unsigned long long) erase.addr,
			       (unsigned long long) erase.len);
			if (rc == -EIO) {
				if (ctx->mtd->block_markbad(ctx->mtd,
							    erase.addr)) {
					printk(KERN_ERR
					       "apanic: Err marking blk bad\n");
					goto out;
				}
				printk(KERN_INFO
				       "apanic: Marked a bad block"
				       " @%llx\n", erase.addr);
				set_bb(erase.addr>>ctx->mtd->erasesize_shift,
					apanic_bbt);
				continue;
			}
			goto out;
		}
		schedule();
		remove_wait_queue(&wait_q, &wait);
	}
	printk(KERN_DEBUG "apanic: %s partition erased\n",
	       CONFIG_APANIC_PLABEL);
out:
	return;
}

#endif

static void apanic_remove_proc_work(struct work_struct *work)
{
	struct apanic_data *ctx = &drv_ctx;

	mutex_lock(&drv_mutex);
	mtd_panic_erase();
	memset(&ctx->curr, 0, sizeof(struct panic_header));
	if (ctx->apanic_console) {
		remove_proc_entry("apanic_console", NULL);
		ctx->apanic_console = NULL;
	}
	if (ctx->apanic_threads) {
		remove_proc_entry("apanic_threads", NULL);
		ctx->apanic_threads = NULL;
	}
	mutex_unlock(&drv_mutex);
}

static int apanic_proc_write(struct file *file, const char __user *buffer,
				unsigned long count, void *data)
{
	schedule_work(&proc_removal_work);
	return count;
}

static int mtd_panic_notify(void *data);
static void mtd_panic_notify_add(struct mtd_info *mtd)
{
	struct task_struct      *tsk;

	tsk = kthread_run(mtd_panic_notify, mtd, "mtd_panic");
	if(IS_ERR(tsk)) {
		printk("mtd_panic_notify_add error\n");
	}

}
static int mtd_panic_notify(void *data)
{
	struct mtd_info *mtd = (struct mtd_info*) data;
	struct apanic_data *ctx = &drv_ctx;
	struct panic_header *hdr = ctx->bounce;
	//size_t len;
	int rc;
	int    proc_entry_created = 0;
	struct file *file = NULL;
	loff_t pos;
	unsigned int timeout = 0;

	if (strcmp(mtd->name, CONFIG_APANIC_PLABEL))
		return -1;

	ctx->mtd = mtd;

	alloc_bbt(mtd, apanic_bbt);
	scan_bbt(mtd, apanic_bbt);

	if (apanic_good_blocks == 0) {
		printk(KERN_ERR "apanic: no any good blocks?!\n");
		goto out1_err;
	}

	timeout = 0;

	do{
		file = filp_open(PANIC_BLOCK_PATH, O_RDWR, S_IRUSR | S_IWUSR);
		if (IS_ERR(file)) {
			msleep(1000);
		}

	}while(IS_ERR(file) && timeout++ < 1000);


	if (IS_ERR(file)) {
		printk("open /dev/block/ndapanic filed\n");
		goto out1_err;
	}

	old_fs_record = get_fs();
	set_fs(KERNEL_DS);

	pos = 0;
	rc = vfs_read(file, (char __user *)ctx->bounce, mtd->writesize,  &pos);
	if (rc < 0) {
		printk(KERN_ERR "apanic: Error reading block 0 (%d)\n", rc);
		goto out_err;
	}
	set_fs(old_fs_record);

	if (rc != mtd->writesize) {
		printk(KERN_ERR "apanic: Bad read size (%d)\n", rc);
		goto out_err;
	}

	printk(KERN_INFO "apanic: Bound to mtd partition '%s'\n", mtd->name);

	if (hdr->magic != PANIC_MAGIC) {
		printk(KERN_INFO "apanic: No panic data available\n");
		mtd_panic_erase();
		return -1;
	}

	if (hdr->version != PHDR_VERSION) {
		printk(KERN_INFO "apanic: Version mismatch (%d != %d)\n",
		       hdr->version, PHDR_VERSION);
		mtd_panic_erase();
		return -1;
	}

	memcpy(&ctx->curr, hdr, sizeof(struct panic_header));

	printk(KERN_INFO "apanic: c(%u, %u) t(%u, %u)\n",
	       hdr->console_offset, hdr->console_length,
	       hdr->threads_offset, hdr->threads_length);

	if (hdr->console_length) {
		ctx->apanic_console = create_proc_entry("apanic_console",
						      S_IFREG | S_IRUGO, NULL);
		if (!ctx->apanic_console)
			printk(KERN_ERR "%s: failed creating procfile\n",
			       __func__);
		else {
			ctx->apanic_console->read_proc = apanic_proc_read;
			ctx->apanic_console->write_proc = apanic_proc_write;
			ctx->apanic_console->size = hdr->console_length;
			ctx->apanic_console->data = (void *) 1;
			proc_entry_created = 1;
		}
	}

	if (hdr->threads_length) {
		ctx->apanic_threads = create_proc_entry("apanic_threads",
						       S_IFREG | S_IRUGO, NULL);
		if (!ctx->apanic_threads)
			printk(KERN_ERR "%s: failed creating procfile\n",
			       __func__);
		else {
			ctx->apanic_threads->read_proc = apanic_proc_read;
			ctx->apanic_threads->write_proc = apanic_proc_write;
			ctx->apanic_threads->size = hdr->threads_length;
			ctx->apanic_threads->data = (void *) 2;
			proc_entry_created = 1;
		}
	}

	if (!proc_entry_created)
		mtd_panic_erase();

	filp_close(file, NULL);
	return 0;
out_err:
	filp_close(file, NULL);
out1_err:
	ctx->mtd = NULL;
	return -1;
}

static void mtd_panic_notify_remove(struct mtd_info *mtd)
{
	struct apanic_data *ctx = &drv_ctx;
	if (mtd == ctx->mtd) {
		ctx->mtd = NULL;
		printk(KERN_INFO "apanic: Unbound from %s\n", mtd->name);
	}
}

static struct mtd_notifier mtd_panic_notifier = {
	.add	= mtd_panic_notify_add,
	.remove	= mtd_panic_notify_remove,
};

static int in_panic = 0;

static int apanic_writeflashpage(struct mtd_info *mtd, loff_t to,
				 const u_char *buf)
{
	int rc;
	size_t wlen;
	int panic = in_interrupt() | in_atomic();

	if (panic && !mtd->_panic_write) {
		printk(KERN_EMERG "%s: No panic_write available\n", __func__);
		return 0;
	} else if (!panic && !mtd->_write) {
		printk(KERN_EMERG "%s: No write available\n", __func__);
		return 0;
	}

	to = phy_offset(mtd, to);
	if (to == APANIC_INVALID_OFFSET) {
		printk(KERN_EMERG "apanic: write to invalid address\n");
		return 0;
	}

	if (panic)
		rc = mtd->_panic_write(mtd, to, mtd->writesize, &wlen, buf);
	else
		rc = mtd->_write(mtd, to, mtd->writesize, &wlen, buf);

	if (rc) {
		printk(KERN_EMERG
		       "%s: Error writing data to flash (%d)\n",
		       __func__, rc);
		return rc;
	}

	return wlen;
}

extern int log_buf_copy(char *dest, int idx, int len);
extern void log_buf_clear(void);

/*
 * Writes the contents of the console to the specified offset in flash.
 * Returns number of bytes written
 */
static int apanic_write_console(struct mtd_info *mtd, unsigned int off)
{
	struct apanic_data *ctx = &drv_ctx;
	int saved_oip;
	int idx = 0;
	int rc, rc2;
	unsigned int last_chunk = 0;

	while (!last_chunk) {
		saved_oip = oops_in_progress;
		oops_in_progress = 1;
		rc = log_buf_copy(ctx->bounce, idx, mtd->writesize);
		if (rc < 0)
			break;

		if (rc != mtd->writesize)
			last_chunk = rc;

		oops_in_progress = saved_oip;
		if (rc <= 0)
			break;
		if (rc != mtd->writesize)
			memset(ctx->bounce + rc, 0, mtd->writesize - rc);

		rc2 = apanic_writeflashpage(mtd, off, ctx->bounce);
		if (rc2 <= 0) {
			printk(KERN_EMERG
			       "apanic: Flash write failed (%d)\n", rc2);
			return idx;
		}
		if (!last_chunk)
			idx += rc2;
		else
			idx += last_chunk;
		off += rc2;
	}
	return idx;
}

static int apanic(struct notifier_block *this, unsigned long event,
			void *ptr)
{
	struct apanic_data *ctx = &drv_ctx;
	struct panic_header *hdr = (struct panic_header *) ctx->bounce;
	int console_offset = 0;
	int console_len = 0;
	int threads_offset = 0;
	int threads_len = 0;
	int rc;

	if (in_panic)
		return NOTIFY_DONE;
	in_panic = 1;
#ifdef CONFIG_PREEMPT
	/* Ensure that cond_resched() won't try to preempt anybody */
	add_preempt_count(PREEMPT_ACTIVE);
#endif
	touch_softlockup_watchdog();

	if (!ctx->mtd)
		goto out;

	if (ctx->curr.magic) {
		printk(KERN_EMERG "Crash partition in use!\n");
		goto out;
	}
	console_offset = ctx->mtd->erasesize;

	/*
	 * Write out the console
	 */
	console_len = apanic_write_console(ctx->mtd, console_offset);
	if (console_len < 0) {
		printk(KERN_EMERG "Error writing console to panic log! (%d)\n",
		       console_len);
		console_len = 0;
	}

	/*
	 * Write out all threads
	 */
	threads_offset = ALIGN(console_offset + console_len,
			       ctx->mtd->writesize);
	if (!threads_offset)
		threads_offset = ctx->mtd->writesize;

	ram_console_enable_console(0);

	log_buf_clear();
	show_state_filter(0);

	threads_len = apanic_write_console(ctx->mtd, threads_offset);
	if (threads_len < 0) {
		printk(KERN_EMERG "Error writing threads to panic log! (%d)\n",
		       threads_len);
		threads_len = 0;
	}

	/*
	 * Finally write the panic header
	 */
	memset(ctx->bounce, 0, PAGE_SIZE);
	hdr->magic = PANIC_MAGIC;
	hdr->version = PHDR_VERSION;

	hdr->console_offset = console_offset;
	hdr->console_length = console_len;

	hdr->threads_offset = threads_offset;
	hdr->threads_length = threads_len;

	rc = apanic_writeflashpage(ctx->mtd, 0, ctx->bounce);
	if (rc <= 0) {
		printk(KERN_EMERG "apanic: Header write failed (%d)\n",
		       rc);
		goto out;
	}

	printk(KERN_EMERG "apanic: Panic dump sucessfully written to flash\n");

 out:
#ifdef CONFIG_PREEMPT
	sub_preempt_count(PREEMPT_ACTIVE);
#endif
	in_panic = 0;
	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call	= apanic,
};

static int panic_dbg_get(void *data, u64 *val)
{
	apanic(NULL, 0, NULL);
	return 0;
}

static int panic_dbg_set(void *data, u64 val)
{
	BUG();
	return -1;
}

DEFINE_SIMPLE_ATTRIBUTE(panic_dbg_fops, panic_dbg_get, panic_dbg_set, "%llu\n");

int __init apanic_init(void)
{
	memset(&drv_ctx, 0, sizeof(drv_ctx));
	drv_ctx.bounce = (void *) __get_free_page(GFP_KERNEL);
	register_mtd_user(&mtd_panic_notifier);
	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
	debugfs_create_file("apanic", 0644, NULL, NULL, &panic_dbg_fops);
	INIT_WORK(&proc_removal_work, apanic_remove_proc_work);
	printk(KERN_INFO "Android kernel panic handler initialized (bind=%s)\n",
	       CONFIG_APANIC_PLABEL);
	return 0;
}

late_initcall(apanic_init);
