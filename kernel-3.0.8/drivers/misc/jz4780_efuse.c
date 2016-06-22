#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#include <soc/base.h>
#include <mach/jz4780_efuse.h>


#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/spinlock.h>

#define BYTEMASK(n)		(((unsigned int)0xffffffff) >> (32 - n * 8))
#define STRTADDR		0x0
#define STRTSPACE		0x200

//#define COMMON_INGENIC_EFUSE

#ifndef COMMON_INGENIC_EFUSE
#define STRTADDR4		0x360
#define ENDADDR4		0x3df
#endif

struct jz_efuse *extra_efuse;

#define DUMP_JZ_EFUSE
#ifdef DUMP_JZ_EFUSE
void dump_jz_efuse(struct jz_efuse *efuse)
{
	printk("max_program_length = %x\n", efuse->max_program_length);
	printk("use_count = %x\n", efuse->use_count);
	printk("is_timer_on = %x\n", efuse->is_timer_on);
	printk("gpio_vddq_en_n = %x\n", efuse->gpio_vddq_en_n);

	/* struct jz_efucfg_info	efucfg_info; */
	printk("rd_adj = %x\n", efuse->efucfg_info.rd_adj);
	printk("rd_strobe = %x\n", efuse->efucfg_info.rd_strobe);
	printk("wr_adj = %x\n", efuse->efucfg_info.wr_adj);
	printk("wr_strobe = %x\n", efuse->efucfg_info.wr_strobe);

	/* struct jz_efuse_strict	strict; */
	printk("min_rd_adj = %x\n", efuse->efucfg_info.strict.min_rd_adj);
	printk("min_rd_adj_strobe = %x\n",
			efuse->efucfg_info.strict.min_rd_adj_strobe);
	printk("min_wr_adj = %x\n", efuse->efucfg_info.strict.min_wr_adj);
	printk("min_wr_adj_strobe = %x\n",
			efuse->efucfg_info.strict.min_wr_adj_strobe);
	printk("max_wr_adj_strobe = %x\n",
			efuse->efucfg_info.strict.max_wr_adj_strobe);
}
#endif

static void jz_efuse_vddq_set(unsigned long efuse_ptr)
{
	struct jz_efuse *efuse = (struct jz_efuse *)efuse_ptr;
	printk("JZ4780-EFUSE: vddq_set %d\n", (int)efuse->is_timer_on);
	if (efuse->is_timer_on) {
		mod_timer(&efuse->vddq_protect_timer, jiffies + HZ);
	}
	gpio_set_value(efuse->gpio_vddq_en_n, !efuse->is_timer_on);
}

static int jz_get_efuse_program_length(unsigned long clk_rate)
{
	if ((clk_rate < (185 * 1000000LL)) || (clk_rate > (512 * 1000000LL))) {
		printk("the clk rate %lu is smaller than 185M"
				" or biger than 512M\n", clk_rate);
		return -1;
	}

	/* for the max time to program or read a value is 11us and The maximum
	 * 2.5V supply time to VDDQ must be strictly controlled less than 1sec.
	 * and for 1sec / 11 us / 32 = 90909 / 32 = 2840 and EFUCTL_Length is
	 * 1 - 32 bytes. so in any condition, the EFUCTL_Length's any value is
	 * all ok if the clk_rate is ok
	 */

	return 31;
}

static inline int jz_efuse_get_skip(size_t size)
{
	if (size >= 32) {
		return 32;
	} else if ((size / 4) > 0) {
		return (size / 4) * 4;
	} else {
		return size % 4;
	}
}

static int calculate_efuse_strict(struct jz_efuse_strict *t, unsigned long rate)
{
	unsigned int tmp;

	if (t == NULL) {
		//printf("no space for efuse timing:%p\n", t);
		return -1;
	}

	tmp = (((6500 * (rate / 1000000)) / 1000000) + 1) - 1;
	if (tmp > 0xf) {
		//printf("clk rate is too small or too large to calculate\n");
		return -1;
	} else {
		t->min_rd_adj = tmp;
	}

	tmp = ((((35000 * (rate / 1000000)) / 1000000) + 1) - 5);
	if (tmp > (0xf + 0xf)) {
		//printf("clk rate is too small or too large to calculate\n");
		return -1;
	} else {
		t->min_rd_adj_strobe = tmp;
	}

	tmp = (((6500 * (rate / 1000000)) / 1000000) + 1) - 1;
	if (tmp > 0xf) {
		//printf("clk rate is too small or too large to calculate\n");
		return -1;
	} else {
		t->min_wr_adj = tmp;
	}

	tmp = ((((9000000 / 1000000) * (rate / 1000000))) + 1) - 1666;
	if (tmp > (0xfff + 0xf)) {
		//printf("clk rate is too small or too large to calculate\n");
		return -1;
	} else {
		t->min_wr_adj_strobe = tmp;
	}

	tmp = ((((11000000 / 1000000) * (rate / 1000000))) + 1) - 1666;
	if (tmp > (0xfff + 0xf)) {
		//printf("clk rate is too small or too large to calculate\n");
		return -1;
	} else {
		t->max_wr_adj_strobe = tmp;
	}

	return 0;
}

static int jz_init_efuse_cfginfo(struct jz_efuse *efuse, unsigned long clk_rate)
{
	struct jz_efucfg_info *info = &efuse->efucfg_info;
	struct jz_efuse_strict *s = &info->strict;
	unsigned int tmp = 0;

	if (calculate_efuse_strict(s, clk_rate) < 0) {
		printk("can't find a right pram to efucfg to h2clk rate:%x\n",
				(unsigned int)clk_rate);
		return -1;
	}

	info->rd_adj = (s->min_rd_adj + 0xf) / 2;
	tmp = s->min_rd_adj_strobe - info->rd_adj;
	info->rd_strobe = ((tmp + 0xf) / 2 < 7) ? 7 : (tmp + 0xf) / 2;
	if (info->rd_strobe > 0xf) {
		return -1;
	}

	tmp = (s->min_wr_adj_strobe + s->max_wr_adj_strobe) / 2;
	info->wr_adj = tmp < 0xf ? tmp : 0xf;
	info->wr_strobe = tmp - info->wr_adj;
	if (info->wr_strobe > 0xfff) {
		return -1;
	}

	return 0;
}

static int jz_efuse_open(struct inode *inode, struct file *filp)
{
	struct miscdevice *dev = filp->private_data;
	struct jz_efuse *efuse = container_of(dev, struct jz_efuse, mdev);

	spin_lock(&efuse->lock);
	if (efuse->use_count == 0) {
		if (!clk_is_enabled(efuse->clk)) {
			dev_err(efuse->dev, "h2clk is disable\n");
			return -1;
		}
	} else {
		dev_dbg(efuse->dev, "[%d:%d] already on\n",
			current->tgid, current->pid);
	}

	efuse->use_count++;
	spin_unlock(&efuse->lock);

	return 0;
}

static int jz_efuse_release(struct inode *inode, struct file *filp)
{
	struct miscdevice *dev = filp->private_data;
	struct jz_efuse *efuse = container_of(dev, struct jz_efuse, mdev);

	spin_lock(&efuse->lock);
	efuse->use_count--;
	spin_unlock(&efuse->lock);

	return 0;
}

int get_segment_num(size_t size, loff_t l)
{
	if ((l >= (0x500 - STRTSPACE)) && ((l + size) < (0x600 - STRTSPACE))) {
		return 7;
	} else if ((l > (0x3E1 - STRTSPACE)) && ((l + size) < (0x500 - STRTSPACE))) {
		return 6;
	} else if ((l > (0x3E0 - STRTSPACE)) && ((l + size) < (0x3E1 - STRTSPACE))) {
		return 5;
	} else if ((l > (0x228 - STRTSPACE)) && ((l + size) < (0x3E0 - STRTSPACE))) {
		return 4;
	} else if ((l > (0x218 - STRTSPACE)) && ((l + size) < (0x228 - STRTSPACE))) {
		return 3;
	} else if ((l > (0x208 - STRTSPACE)) && ((l + size) < (0x218 - STRTSPACE))) {
		return 2;
	} else if ((l > (0x200 - STRTSPACE)) && ((l + size) < (0x208 - STRTSPACE))) {
		return 1;
	} else {
		return -1;
	}
}

#ifdef COMMON_INGENIC_EFUSE
static ssize_t jz_security_random_efuse_read(struct jz_efuse *efuse, char *buf,
		size_t size, loff_t *l)
{
	return -1;
}
#endif

static ssize_t jz_nomal_efuse_read_bytes(struct jz_efuse *efuse, char *buf,
		unsigned int addr, int skip)
{
	unsigned int tmp = 0;
	int i = 0;
	unsigned long flag;


	/* 1. Set config register */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCFG);
	tmp &= ~((0xf << 20) | (0xf << 16));
	tmp |= (efuse->efucfg_info.rd_adj << 20)
		| (efuse->efucfg_info.rd_strobe << 16);
	writel(tmp, efuse->iomem + JZ_EFUCFG);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/*
	 * 2. Set control register to indicate what to read data address,
	 * read data numbers and read enable.
	 */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCTRL);
	tmp &= ~((0x1 << 30) | (0x1ff << 21) | (0x1 << 15) | (0x3 << 0));
	if (addr >= (STRTADDR + 0x200)) {
		tmp |= (1 << 30);
	}
	tmp |= (addr << 21) | ((skip - 1) << 16) | (0x1 << 0);
	writel(tmp, efuse->iomem + JZ_EFUCTRL);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/*
	 * 3. Wait status register RD_DONE set to 1 or EFUSE interrupted,
	 * software can read EFUSE data buffer 0 – 8 registers.
	 */
	do {
		tmp = readl(efuse->iomem + JZ_EFUSTATE);
	}while(!(tmp & RD_DONE));
	if ((skip % 4) == 0) {
		for (i = 0; i < (skip / 4); i++) {
			*((unsigned int *)(buf + i * 4))
				= readl(efuse->iomem + JZ_EFUDATA(i));
		}
	} else {
		*((unsigned int *)buf)
			= readl(efuse->iomem + JZ_EFUDATA(0)) & BYTEMASK(skip);
	}

	return 0;
}

static ssize_t jz_nomal_efuse_read(struct jz_efuse *efuse, char *buf,
		size_t size, loff_t *l)
{
	int bytes = 0;
	unsigned int addr = 0, start = *l + STRTADDR;
	int skip = jz_efuse_get_skip(size - bytes);
	int org_skip = 0;

	for (addr = start; addr < start + size; addr += org_skip) {
		if (jz_nomal_efuse_read_bytes(efuse, buf + bytes, addr,
					skip) < 0) {
			printk("error to read efuse byte at addr=%x\n", addr);
			return -1;
		}

		*l += skip;
		bytes += skip;

		org_skip = skip;
		skip = jz_efuse_get_skip(size - bytes);
	}

	return bytes;
}

#ifdef COMMON_INGENIC_EFUSE
/* this is the common interface to ingenic efuse */
static ssize_t jz_efuse_read(struct file *filp, char *buf, size_t size, loff_t *l)
{
	int seg = -1;
	int ret = -1;
	char *tmp_buf = NULL;
	struct miscdevice *dev = filp->private_data;
	struct jz_efuse *efuse = container_of(dev, struct jz_efuse, mdev);

	if ((seg = get_segment_num(size, *l)) < 0) {
		printk("overflow the efuse file\n");
		ret = -1;
		goto get_segment_num_err;
	}

	if ((tmp_buf = kzalloc(size, GFP_KERNEL)) < 0) {
		ret = -ENOMEM;
		goto kzalloc_tmp_buf_err;
	}

	if ((seg == 1) || (seg == 7)) {
		if ((ret = jz_security_random_efuse_read(efuse, tmp_buf,
						size, l)) < 0) {
			goto read_security_random_efuse_err;
		}
	} else {
		if ((ret = jz_nomal_efuse_read(efuse, tmp_buf, size, l)) < 0) {
			goto read_nomal_efuse_err;
		}
	}

	if (ret > 0) {
		copy_to_user(buf, tmp_buf, ret);
	}

read_nomal_efuse_err:
read_security_random_efuse_err:
	kfree(tmp_buf);

kzalloc_tmp_buf_err:
get_segment_num_err:
	return ret;
}
#else
/* this is only to 0x360 to 0x3df efuse space */
static ssize_t jz_efuse_read(struct file *filp, char *buf, size_t size,
		loff_t *l)
{
	struct miscdevice *dev = filp->private_data;
	struct jz_efuse *efuse = container_of(dev, struct jz_efuse, mdev);
	int ret = -1;
	char *tmp_buf = NULL;
	loff_t real_offset = *l + STRTADDR4 - STRTSPACE;

	if (size > (ENDADDR4 - (STRTADDR4 + *l) + 1)) {
		ret = -1;
		goto not_in_file_space_err;
	}

	if ((tmp_buf = kzalloc(size, GFP_KERNEL)) < 0) {
		ret = -ENOMEM;
		goto kzalloc_tmp_buf_err;
	}

	if ((ret = jz_nomal_efuse_read(efuse, tmp_buf, size,
					&real_offset)) < 0) {
		goto read_nomal_efuse_err;
		return ret;
	}

*l = real_offset - (STRTADDR4 - STRTSPACE);
	if (ret > 0) {
		copy_to_user(buf, tmp_buf, ret);
	}

read_nomal_efuse_err:
	kfree(tmp_buf);

kzalloc_tmp_buf_err:
not_in_file_space_err:

	return ret;
}
#endif

#ifdef COMMON_INGENIC_EFUSE
static ssize_t jz_security_random_efuse_write(struct jz_efuse *efuse,
		const char *buf, size_t size, loff_t *l)
{
	return -1;
}
#endif

static int is_space_written(const char *tmp, const char *buf, int skip)
{
	int i = 0;
	for (i = 0; i < skip; i++) {
		if ((tmp[i] & buf[i]) > 0) {
			return 1;
		}
	}
	return 0;
}

static ssize_t jz_nomal_efuse_write_bytes(struct jz_efuse *efuse,
		const char *buf, unsigned int addr, int skip)
{
	unsigned int tmp_buf[8];
	unsigned int tmp = 0;
	unsigned long flag = 0;
	int i = 0;

	if (jz_nomal_efuse_read_bytes(efuse, (char *)&tmp_buf, addr, skip) < 0) {
		printk("read efuse at addr = %x failed\n", addr);
		return -1;
	}

	if (is_space_written((char *)tmp_buf, (char *)buf, skip)) {
		printk("ERROR: the write spaced has been written\n");
		return -1;
	}

	/* 1. Set config register */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCFG);
	tmp &= ~((0xf << 12) | (0xfff << 0));
	tmp |= (efuse->efucfg_info.wr_adj << 12)
		| (efuse->efucfg_info.wr_strobe << 0);
	writel(tmp, efuse->iomem + JZ_EFUCFG);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 2. Write want program data to EFUSE data buffer 0-7 registers */
	if (skip % 4 == 0) {
		for (i = 0; i < skip / 4; i++) {
			writel(*((unsigned int *)(buf + i * 4)),
					efuse->iomem + JZ_EFUDATA(i));
		}
	} else {
		writel(*((unsigned int *)buf) & BYTEMASK(skip),
				efuse->iomem + JZ_EFUDATA(0));
	}

	/*
	 * 3. Set control register, indicate want to program address,
	 * data length.
	 */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCTRL);
	tmp &= ~((0x1 << 30) | (0x1ff << 21) | (0x1 << 15) | (0x3 << 0));
	if (addr >= (STRTADDR + 0x200)) {
		tmp |= (1 << 30);
	}
	tmp |= (addr << 21) | ((skip - 1) << 16);
	writel(tmp, efuse->iomem + JZ_EFUCTRL);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 4. Write control register PG_EN bit to 1 */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCTRL);
	tmp |= (1 << 15);
	writel(tmp, efuse->iomem + JZ_EFUCTRL);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 5. Connect VDDQ pin to 2.5V */
	spin_lock_irqsave(&efuse->lock, flag);
	efuse->is_timer_on = 1;
	jz_efuse_vddq_set((unsigned long)efuse);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 6. Write control register WR_EN bit */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCTRL);
	tmp |= (1 << 1);
	writel(tmp, efuse->iomem + JZ_EFUCTRL);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 7. Wait status register WR_DONE set to 1. */
	do {
		tmp = readl(efuse->iomem + JZ_EFUSTATE);
	}while(!(tmp & WR_DONE));

	/* 8. Disconnect VDDQ pin from 2.5V. */
	spin_lock_irqsave(&efuse->lock, flag);
	efuse->is_timer_on = 0;
	jz_efuse_vddq_set((unsigned long)efuse);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 9. Write control register PG_EN bit to 0. */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCTRL);
	tmp &= ~(1 << 15);
	writel(tmp, efuse->iomem + JZ_EFUCTRL);
	spin_unlock_irqrestore(&efuse->lock, flag);

	return 0;
}

static ssize_t jz_nomal_efuse_write(struct jz_efuse *efuse, const char *buf,
		size_t size, loff_t *l)
{
	int bytes = 0;
	unsigned int addr = 0, start = *l + STRTADDR;
	int skip = jz_efuse_get_skip(size - bytes);
	int org_skip = 0;

	for (addr = start; addr < (start + size); addr += org_skip) {
		if (jz_nomal_efuse_write_bytes(efuse, buf + bytes, addr,
					skip) < 0) {
			printk("error to write efuse byte at addr=%x\n", addr);
			return -1;
		}

		*l += skip;
		bytes += skip;

		org_skip = skip;
		skip = jz_efuse_get_skip(size - bytes);
	}

	return bytes;
}

#ifdef COMMON_INGENIC_EFUSE
static ssize_t jz_efuse_write(struct file *filp, const char *buf, size_t size,
		loff_t *l)
{
	int seg = -1;
	int ret = -1;
	char *tmp_buf = NULL;
	struct miscdevice *dev = filp->private_data;
	struct jz_efuse *efuse = container_of(dev, struct jz_efuse, mdev);

	if ((seg = get_segment_num(size, *l)) < 0) {
		printk("overflow the efuse file\n");
		ret = -1;
		goto get_segment_num_err;
	}

	if ((tmp_buf = kzalloc(size, GFP_KERNEL)) < 0) {
		ret = -ENOMEM;
		goto kzalloc_tmp_buf_err;
	}

	copy_from_user(tmp_buf, buf, size);

	if ((seg == 1) || (seg == 7)) {
		if ((ret = jz_security_random_efuse_write(efuse, tmp_buf, size,
						l)) < 0) {
			ret = -1;
			goto write_security_random_efuse_err;
		}
	} else {
		if ((ret = jz_nomal_efuse_write(efuse, tmp_buf, size, l)) < 0) {
			ret = -1;
			goto write_nomal_efuse_err;
		}
	}

write_nomal_efuse_err:
write_security_random_efuse_err:
	kfree(tmp_buf);

kzalloc_tmp_buf_err:
get_segment_num_err:
	return ret;
}
#else
static ssize_t jz_efuse_write(struct file *filp, const char *buf, size_t size,
		loff_t *l)
{
	int ret = -1;
	char *tmp_buf = NULL;
	struct miscdevice *dev = filp->private_data;
	struct jz_efuse *efuse = container_of(dev, struct jz_efuse, mdev);
	loff_t real_offset = *l + STRTADDR4 - STRTSPACE;

	if (size > (ENDADDR4 - (STRTADDR4 + *l) + 1)) {
		ret = -1;
		goto not_in_file_space_err;
	}

	if ((tmp_buf = kzalloc(size, GFP_KERNEL)) < 0) {
		ret = -ENOMEM;
		goto kzalloc_tmp_buf_err;
	}

	copy_from_user(tmp_buf, buf, size);

	if ((ret = jz_nomal_efuse_write(efuse, tmp_buf, size, &real_offset)) < 0) {
		ret = -1;
		goto write_nomal_efuse_err;
	}

	*l = real_offset - (STRTADDR4 - STRTSPACE);

write_nomal_efuse_err:
	kfree(tmp_buf);

kzalloc_tmp_buf_err:
not_in_file_space_err:
	return ret;
}
#endif

static long jz_efuse_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return -1;
}

static int jz_efuse_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return -1;
}

static struct file_operations efuse_misc_fops = {
	.open		= jz_efuse_open,
	.release	= jz_efuse_release,
	.llseek		= default_llseek,
	.read		= jz_efuse_read,
	.write		= jz_efuse_write,
	.unlocked_ioctl	= jz_efuse_ioctl,
	.mmap		= jz_efuse_mmap,
};

static ssize_t jz_efuse_id_show(struct device *dev,
		struct device_attribute *attr, char *buf, loff_t lpos)
{
	struct jz_efuse * efuse = dev_get_drvdata(dev);
	char *tmp_buf = NULL;
	unsigned int *data = NULL;
	int ret = -1;

	if ((tmp_buf = kzalloc(16, GFP_KERNEL)) < 0) {
		ret = -ENOMEM;
		goto kzalloc_tmp_buf_err;
	}
	data = (unsigned int *)tmp_buf;

	if ((ret = jz_nomal_efuse_read(efuse, tmp_buf, 16, &lpos)) < 0) {
		ret = -1;
		goto read_nomal_efuse_err;
	}

	if (ret > 0) {
		if ((ret = snprintf(buf, PAGE_SIZE, "%08x %08x %08x %08x\n",
				data[0], data[1], data[2], data[3])) < 0) {
			goto snprintf_err;
		}
	}

snprintf_err:
read_nomal_efuse_err:
	kfree(tmp_buf);

kzalloc_tmp_buf_err:
	return ret;
}

static ssize_t jz_efuse_id_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count,
		loff_t lpos)
{
	struct jz_efuse * efuse = dev_get_drvdata(dev);
	char *tmp_buf = NULL;
	int ret = -1;
	int buf_len = strlen(buf);

	if (buf_len > 16) {
		buf_len = 16;
	}

	if ((tmp_buf = kzalloc(buf_len, GFP_KERNEL)) < 0) {
		ret = -ENOMEM;
		goto kzalloc_tmp_buf_err;
	}

	strncpy(tmp_buf, buf, buf_len);

	if ((ret = jz_nomal_efuse_write(efuse, tmp_buf, buf_len, &lpos)) < 0) {
		goto write_nomal_efuse_err;
	}

write_nomal_efuse_err:
	kfree(tmp_buf);

kzalloc_tmp_buf_err:
	return ret;
}

static ssize_t jz_efuse_chip_id_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return jz_efuse_id_show(dev, attr, buf, 0x8);
}

static ssize_t jz_efuse_chip_id_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return jz_efuse_id_store(dev, attr, buf, count, 0x8);
}

static ssize_t jz_efuse_user_id_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return jz_efuse_id_show(dev, attr, buf, 0x18);
}

static void jz_efuse_id_real_write(struct jz_efuse *efuse, uint32_t *buf, unsigned int addr)
{
	unsigned int tmp_buf[8];
	unsigned int tmp = 0;
	unsigned long flag = 0;
	int i = 0;
	dump_jz_efuse(efuse);

#if 0
	if (jz_nomal_efuse_read_bytes(efuse, (char *)&tmp_buf, addr, skip) < 0) {
		printk("read efuse at addr = %x failed\n", addr);
		return -1;
	}

	if (is_space_written((char *)tmp_buf, (char *)buf, skip)) {
		printk("ERROR: the write spaced has been written\n");
		return -1;
	}
#endif

#if 1
	/* 1. Set config register */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCFG);
	tmp &= ~((0xf << 12) | (0xfff << 0));
	tmp |= (efuse->efucfg_info.wr_adj << 12)
		| (efuse->efucfg_info.wr_strobe << 0);
	writel(tmp, efuse->iomem + JZ_EFUCFG);
	spin_unlock_irqrestore(&efuse->lock, flag);

	printk("efuse efucfg is 0x%x\n", JZ_EFUCFG + efuse->iomem);
#endif
	/* 2. Write want program data to EFUSE data buffer 0-7 registers */
		for (i = 0; i < 4; i++) {
			printk("befor write: iomem is 0x%x, JZ_EFUDATA[%d] is =0x%x\n",
					efuse->iomem, i, readl(efuse->iomem + JZ_EFUDATA(i)));
			printk("now write (int)buf is 0x%x\n", *(buf+i));
			writel(*(buf + i), efuse->iomem + JZ_EFUDATA(i));
			printk("after write: iomem is 0x%x, JZ_EFUDATA[%d] is =0x%x\n",
					efuse->iomem, i, readl(efuse->iomem + JZ_EFUDATA(i)));
		}

#if 1
	/*
	 * 3. Set control register, indicate want to program address,
	 * data length.
	 */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCTRL);
	tmp &= ~((0x1 << 30) | (0x1ff << 21) | (0x1 << 15) | (0x3 << 0));
	if (addr >= (STRTADDR + 0x200)) {
		tmp |= (1 << 30);
	}
	tmp |= (addr << 21) | (4 << 16);
	writel(tmp, efuse->iomem + JZ_EFUCTRL);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 4. Write control register PG_EN bit to 1 */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCTRL);
	tmp |= (1 << 15);
	writel(tmp, efuse->iomem + JZ_EFUCTRL);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 5. Connect VDDQ pin to 2.5V */
	spin_lock_irqsave(&efuse->lock, flag);
	efuse->is_timer_on = 1;
	jz_efuse_vddq_set((unsigned long)efuse);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 6. Write control register WR_EN bit */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCTRL);
	tmp |= (1 << 1);
	writel(tmp, efuse->iomem + JZ_EFUCTRL);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 7. Wait status register WR_DONE set to 1. */
	do {
		tmp = readl(efuse->iomem + JZ_EFUSTATE);
	}while(!(tmp & WR_DONE));

	/* 8. Disconnect VDDQ pin from 2.5V. */
	spin_lock_irqsave(&efuse->lock, flag);
	efuse->is_timer_on = 0;
	jz_efuse_vddq_set((unsigned long)efuse);
	spin_unlock_irqrestore(&efuse->lock, flag);

	/* 9. Write control register PG_EN bit to 0. */
	spin_lock_irqsave(&efuse->lock, flag);
	tmp = readl(efuse->iomem + JZ_EFUCTRL);
	tmp &= ~(1 << 15);
	writel(tmp, efuse->iomem + JZ_EFUCTRL);
	spin_unlock_irqrestore(&efuse->lock, flag);
#endif

	return 0;

}

static void jz_efuse_id_write(struct jz_efuse *efuse, int is_chip_id, uint32_t *buf)
{
	if (is_chip_id) {
		jz_efuse_id_real_write(efuse, buf, 0x8);
	} else {
		jz_efuse_id_real_write(efuse, buf, 0x18);
	}
}

static ssize_t jz_efuse_user_id_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct jz_efuse * efuse = dev_get_drvdata(dev);
	uint32_t data[4];
	printk("\n1--jz_efuse_user_id_store\n");

	sscanf (buf, "%08x %08x %08x %08x", &data[0], &data[1], &data[2], &data[3]);
	dev_info(dev, "user id store: %08x %08x %08x %08x\n", data[0], data[1], data[2], data[3]);
	jz_efuse_id_write(efuse, 0, data);
	return strnlen(buf, PAGE_SIZE);
}

static ssize_t jz_efuse_protect_bit_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct jz_efuse * efuse = dev_get_drvdata(dev);
	char *tmp_buf = NULL;
	unsigned int *data = NULL;
	loff_t lpos = 0x1e0;
	int ret = -1;

	if ((tmp_buf = kzalloc(4, GFP_KERNEL)) < 0) {
		ret = -ENOMEM;
		goto kzalloc_tmp_buf_err;
	}
	data = (unsigned int *)tmp_buf;

	if ((ret = jz_nomal_efuse_read(efuse, tmp_buf, 1, &lpos)) < 0) {
		ret = -1;
		goto read_nomal_efuse_err;
	}

	if (ret > 0) {
		if ((ret = snprintf(buf, PAGE_SIZE, "%08x\n", data[0])) < 0) {
			goto snprintf_err;
		}
	}

snprintf_err:
read_nomal_efuse_err:
	kfree(tmp_buf);

kzalloc_tmp_buf_err:
	return ret;
}

static struct device_attribute jz_efuse_sysfs_attrs[] = {
    __ATTR(chip_id, S_IRUGO | S_IWUSR, jz_efuse_chip_id_show,
		    jz_efuse_chip_id_store),
    __ATTR(user_id, S_IRUGO | S_IWUSR, jz_efuse_user_id_show,
		    jz_efuse_user_id_store),
    __ATTR(protect_bit, S_IRUGO | S_IWUSR, jz_efuse_protect_bit_show,
		    NULL),
};

static int efuse_probe(struct platform_device *pdev)
{
	int		ret;
	struct jz4780_efuse_platform_data *pdata;
	struct resource	*regs;
	struct jz_efuse	*efuse = NULL;
	unsigned long	clk_rate;

	pdata = pdev->dev.platform_data;

	if (!pdata) {
		ret = -1;
		dev_err(&pdev->dev, "No platform data\n");
		goto err_get_platform_data;
	}

	efuse = kzalloc(sizeof(struct jz_efuse), GFP_KERNEL);
	if (!efuse) {
		ret = -ENOMEM;
		goto err_no_enomem;
	}

#if 0
	/* for we needn't to use the irq to do our work */
	efuse->irq = platform_get_irq(pdev, 0);
	if(efuse->irq < 0) {
		dev_err(&pdev->dev, "get irq failed\n");
		ret = efuse->irq;
		goto err_get_irq;
	}
#endif

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&pdev->dev, "No iomem resource\n");
		ret = -ENXIO;
		goto err_get_mem;
	}

	efuse->iomem = ioremap(regs->start, resource_size(regs));
	if (!efuse->iomem) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENXIO;
		goto err_get_mem;
	}

	efuse->clk = clk_get(&pdev->dev, "h2clk");
	if (IS_ERR(efuse->clk)) {
		ret = PTR_ERR(efuse->clk);
		goto err_get_clk;
	}

	if ((clk_rate = clk_get_rate(efuse->clk)) == 0) {
		ret = -1;
		goto err_get_clk_rate;
	}

	if ((efuse->max_program_length =
				jz_get_efuse_program_length(clk_rate)) < 0) {
		ret = -1;
		goto err_clk_check;
	}

	printk("h2clk clk_rate = %x\n", (unsigned int)clk_rate);
	jz_init_efuse_cfginfo(efuse, clk_rate);
	efuse->dev = &pdev->dev;
	efuse->gpio_vddq_en_n = pdata->gpio_vddq_en_n;
	efuse->mdev.minor = MISC_DYNAMIC_MINOR;
	efuse->mdev.name =  "jz4780-efuse";
	efuse->mdev.fops = &efuse_misc_fops;

	if (efuse->gpio_vddq_en_n != -ENODEV) {
		ret = gpio_request(efuse->gpio_vddq_en_n, dev_name(&pdev->dev));
		if (ret) {
			dev_err(&pdev->dev, "Failed to request gpio pin: %d\n", ret);
			goto err_gpio_request;
		}

		/* power off by default */
		ret = gpio_direction_output(efuse->gpio_vddq_en_n, 1);
		if (ret) {
			dev_err(&pdev->dev, "Failed to set gpio as output: %d\n", ret);
			goto err_gpio_direction_output;
		}

		efuse->is_timer_on = 0;
		setup_timer(&efuse->vddq_protect_timer, jz_efuse_vddq_set,
				(unsigned long)efuse);
		add_timer(&efuse->vddq_protect_timer);
	} else {
		dev_info(&pdev->dev,"can't find gpio vddq.\n");
		ret = -1;
		goto err_gpio_vddq_found;
	}

	spin_lock_init(&efuse->lock);
	{
	    int i;
	    for (i = 0; i < ARRAY_SIZE(jz_efuse_sysfs_attrs); i++) {
		ret = device_create_file(&pdev->dev, &jz_efuse_sysfs_attrs[i]);
		if (ret)
		    break;
	    }
	}

	ret = misc_register(&efuse->mdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "misc_register failed\n");
		goto err_registe_misc;
	}

	platform_set_drvdata(pdev, efuse);
	extra_efuse = efuse;

#if 0
	/* for we needn't to use this lok and irq to work now */
	wake_lock_init(&efuse->wake_lock, WAKE_LOCK_SUSPEND, "efuse");

	mutex_init(&efuse->mutex);

	init_completion(&efuse->done);
	ret = request_irq(efuse->irq, efuse_interrupt, IRQF_DISABLED,
			  "efuse",efuse);
	if (ret < 0) {
		dev_err(&pdev->dev, "request_irq failed\n");
		goto err_request_irq;
	}
	disable_irq_nosync(efuse->irq);
#endif

#ifdef DUMP_JZ_EFUSE
	dump_jz_efuse(efuse);
#endif

	return 0;

#if 0
err_request_irq:
	misc_deregister(&efuse->mdev);
#endif

err_registe_misc:
err_gpio_direction_output:
	gpio_free(efuse->gpio_vddq_en_n);

err_gpio_request:
err_gpio_vddq_found:
err_clk_check:
err_get_clk_rate:
	clk_put(efuse->clk);

err_get_clk:
	iounmap(efuse->iomem);

err_get_mem:

#if 0
err_get_irq:
#endif
	kfree(efuse);

err_no_enomem:
err_get_platform_data:

	return ret;
}

static int __devexit efuse_remove(struct platform_device *pdev)
{
	struct jz_efuse *efuse = platform_get_drvdata(pdev);

	misc_deregister(&efuse->mdev);
	gpio_free(efuse->gpio_vddq_en_n);
	del_timer(&efuse->vddq_protect_timer);
	clk_put(efuse->clk);
	iounmap(efuse->iomem);
	kfree(efuse);

	return 0;
}

static struct platform_driver efuse_driver = {
	.probe		= efuse_probe,
	.remove		= efuse_remove,
	.driver		= {
		.name	= "jz4780-efuse",
		.owner	= THIS_MODULE,
	}
};

static int __init efuse_init(void)
{
	return platform_driver_register(&efuse_driver);
}

static void __exit efuse_exit(void)
{
	platform_driver_unregister(&efuse_driver);
}

module_init(efuse_init);
module_exit(efuse_exit);
MODULE_LICENSE("GPL");

void jz_efuse_id_read(int is_chip_id, uint32_t *buf)
{
	loff_t lpos;

	if (extra_efuse == NULL) {
		int i = 0;
		for (i = 0; i < 4; i++) {
			buf[i] = 0;
		}
		return;
	}

	lpos = (is_chip_id) > 0 ? 0x8 : 0x18;
	if (jz_nomal_efuse_read(extra_efuse, (char *)buf, 16, &lpos) < 0) {
		printk("read %s failed\n", lpos == 0x8 ? "chip_id" : "user_id");
		return;
	}
}
EXPORT_SYMBOL_GPL(jz_efuse_id_read);
