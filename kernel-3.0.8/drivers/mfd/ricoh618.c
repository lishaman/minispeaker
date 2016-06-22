/*
 * driver/mfd/ricoh618.c
 *
 * Core driver implementation to access RICOH RN5T618 power management chip.
 *
 * Copyright (C) 2012-2013 RICOH COMPANY,LTD
 *
 * Based on code
 *	Copyright (C) 2011 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*#define DEBUG			1*/
/*#define VERBOSE_DEBUG		1*/
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/mfd/core.h>
#include <linux/mfd/ricoh618.h>
#include <linux/mfd/pmu-common.h>
#include <linux/delay.h>
#include <jz_notifier.h>

#define DEBUG
struct sleep_control_data {
	u8 reg_add;
};

#define SLEEP_INIT(_id, _reg)		\
	[RICOH618_DS_##_id] = {.reg_add = _reg}

static struct sleep_control_data sleep_data[] = {
	SLEEP_INIT(DC1, 0x16),
	SLEEP_INIT(DC2, 0x17),
	SLEEP_INIT(DC3, 0x18),
	SLEEP_INIT(LDO1, 0x1B),
	SLEEP_INIT(LDO2, 0x1C),
	SLEEP_INIT(LDO3, 0x1D),
	SLEEP_INIT(LDO4, 0x1E),
	SLEEP_INIT(LDO5, 0x1F),
	SLEEP_INIT(PSO0, 0x25),
	SLEEP_INIT(PSO1, 0x26),
	SLEEP_INIT(PSO2, 0x27),
	SLEEP_INIT(PSO3, 0x28),
	SLEEP_INIT(LDORTC1, 0x2A),
};

static inline int __ricoh618_read(struct i2c_client *client,
				  u8 reg, uint8_t *val)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
		return ret;
	}

	*val = (uint8_t)ret;
	dev_dbg(&client->dev, "ricoh618: reg read  reg=%x, val=%x\n",
				reg, *val);
	return 0;
}

static inline int __ricoh618_bulk_reads(struct i2c_client *client, u8 reg,
				int len, uint8_t *val)
{
	int ret;
	int i;

	ret = i2c_smbus_read_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading from 0x%02x\n", reg);
		return ret;
	}
	for (i = 0; i < len; ++i) {
		dev_dbg(&client->dev, "ricoh618: reg read  reg=%x, val=%x\n",
				reg + i, *(val + i));
	}
	return 0;
}

static inline int __ricoh618_write(struct i2c_client *client,
				 u8 reg, uint8_t val)
{
	int ret;

	dev_dbg(&client->dev, "ricoh618: reg write  reg=%x, val=%x\n",
				reg, val);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writing 0x%02x to 0x%02x\n",
				val, reg);
		return ret;
	}

	return 0;
}

static inline int __ricoh618_bulk_writes(struct i2c_client *client, u8 reg,
				  int len, uint8_t *val)
{
	int ret;
	int i;

	for (i = 0; i < len; ++i) {
		dev_dbg(&client->dev, "ricoh618: reg write  reg=%x, val=%x\n",
				reg + i, *(val + i));
	}

	ret = i2c_smbus_write_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writings to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static inline int set_bank_ricoh618(struct device *dev, int bank)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	int ret;

    if(bank != (bank & 1))      return -EINVAL;
    if(bank == ricoh618->bank_num)   return 0;
    ret = __ricoh618_write(to_i2c_client(dev), RICOH618_REG_BANKSEL, bank);
    if( !ret ) {
        ricoh618->bank_num = bank;
    }
    return ret;
}

int ricoh618_write(struct device *dev, u8 reg, uint8_t val)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	int ret = 0;

	if (!ricoh618) {
		printk("%s get drvdata failed\n", __func__);
		return -1;
	}
	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 0);
	if( !ret )
		ret = __ricoh618_write(to_i2c_client(dev), reg, val);
	mutex_unlock(&ricoh618->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(ricoh618_write);

int ricoh618_write_bank1(struct device *dev, u8 reg, uint8_t val)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 1);
	if( !ret )
		ret = __ricoh618_write(to_i2c_client(dev), reg, val);
	mutex_unlock(&ricoh618->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(ricoh618_write_bank1);

int ricoh618_bulk_writes(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 0);
	if( !ret )
		ret = __ricoh618_bulk_writes(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&ricoh618->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(ricoh618_bulk_writes);

int ricoh618_bulk_writes_bank1(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 1);
	if( !ret )
		ret = __ricoh618_bulk_writes(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&ricoh618->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(ricoh618_bulk_writes_bank1);

static void ricoh618_read_pwroff_on(struct device *dev)
{
	int ret = 0;
	uint8_t val;

	ret = __ricoh618_read(to_i2c_client(dev), RICOH618_PWR_ON_HIS, &val);
	if( !ret )
		printk("RICOH618: power on squence: 0x%x\n", val);
	else
		printk("%s failed\n", __func__);

	ret = __ricoh618_read(to_i2c_client(dev), RICOH618_PWR_OFF_HIS, &val);
	if( !ret )
		printk("RICOH618: power off squence: 0x%x\n", val);
	else
		printk("%s failed\n", __func__);
}

int ricoh618_read(struct device *dev, u8 reg, uint8_t *val)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 0);
	if( !ret )
		ret = __ricoh618_read(to_i2c_client(dev), reg, val);
	mutex_unlock(&ricoh618->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(ricoh618_read);

int ricoh618_read_bank1(struct device *dev, u8 reg, uint8_t *val)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 1);
	if( !ret )
		ret =  __ricoh618_read(to_i2c_client(dev), reg, val);
	mutex_unlock(&ricoh618->io_lock);

	return ret;
}

EXPORT_SYMBOL_GPL(ricoh618_read_bank1);

int ricoh618_bulk_reads(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 0);
	if( !ret )
		ret = __ricoh618_bulk_reads(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&ricoh618->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(ricoh618_bulk_reads);

int ricoh618_bulk_reads_bank1(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 1);
	if( !ret )
		ret = __ricoh618_bulk_reads(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&ricoh618->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(ricoh618_bulk_reads_bank1);

int ricoh618_set_bits(struct device *dev, u8 reg, uint8_t bit_mask)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 0);
	if( !ret ){
		ret = __ricoh618_read(to_i2c_client(dev), reg, &reg_val);
		if (ret)
			goto out;

		if ((reg_val & bit_mask) != bit_mask) {
			reg_val |= bit_mask;
			ret = __ricoh618_write(to_i2c_client(dev), reg, reg_val);
		}
	}
out:
	mutex_unlock(&ricoh618->io_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(ricoh618_set_bits);

int ricoh618_clr_bits(struct device *dev, u8 reg, uint8_t bit_mask)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 0);
	if( !ret ){
		ret = __ricoh618_read(to_i2c_client(dev), reg, &reg_val);
		if (ret)
			goto out;

		if (reg_val & bit_mask) {
			reg_val &= ~bit_mask;
			ret = __ricoh618_write(to_i2c_client(dev), reg, reg_val);
		}
	}
out:
	mutex_unlock(&ricoh618->io_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(ricoh618_clr_bits);

int ricoh618_update(struct device *dev, u8 reg, uint8_t val, uint8_t mask)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 0);
	if( !ret ){
		ret = __ricoh618_read(ricoh618->client, reg, &reg_val);
		if (ret)
			goto out;

		if ((reg_val & mask) != val) {
			reg_val = (reg_val & ~mask) | (val & mask);
			ret = __ricoh618_write(ricoh618->client, reg, reg_val);
		}
	}
out:
	mutex_unlock(&ricoh618->io_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(ricoh618_update);

int ricoh618_update_bank1(struct device *dev, u8 reg, uint8_t val, uint8_t mask)
{
	struct ricoh618 *ricoh618 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&ricoh618->io_lock);
	ret = set_bank_ricoh618(dev, 1);
	if( !ret ){
		ret = __ricoh618_read(ricoh618->client, reg, &reg_val);
		if (ret)
			goto out;

		if ((reg_val & mask) != val) {
			reg_val = (reg_val & ~mask) | (val & mask);
			ret = __ricoh618_write(ricoh618->client, reg, reg_val);
		}
	}
out:
	mutex_unlock(&ricoh618->io_lock);
	return ret;
}

static struct i2c_client *ricoh618_i2c_client;
static int ricoh618_power_off(void)
{
	int ret;
	uint8_t val = 0;
	if (!ricoh618_i2c_client)
		return -EINVAL;

	val = g_soc;
	val &= 0x7f;
	ret = __ricoh618_write(ricoh618_i2c_client, RICOH618_PSWR, val);
	if (ret < 0)
		dev_err(&(ricoh618_i2c_client->dev), "Error in writing PSWR_REG\n");

	if (g_fg_on_mode == 0) {
		ret = ricoh618_clr_bits(ricoh618_i2c_client,
					 RICOH618_FG_CTRL, 0x01);
		if (ret < 0)
			dev_err(&(ricoh618_i2c_client->dev), "Error in writing FG_CTRL\n");
	}

	/* Not repeat power ON after power off(Power Off/N_OE) */
	__ricoh618_write(ricoh618_i2c_client, RICOH618_PWR_REP_CNT, 0x0);

	/* Power OFF */
	__ricoh618_write(ricoh618_i2c_client, RICOH618_PWR_SLP_CNT, 0x1);

	printk("ricoh618_power_off successed\n\n");
	return 0;
}

static int ricoh618_register_reset_notifier(struct jz_notifier *nb)
{
	return jz_notifier_register(nb);
}

static int ricoh618_unregister_reset_notifier(struct jz_notifier *nb)
{
	return jz_notifier_unregister(nb);
}
static int ricoh618_reset_notifier_handler(struct jz_notifier *nb,void *d)
{
	int ret;
	printk("WARNNING:system will power!\n");
	ret = ricoh618_power_off();
	if (ret < 0)
		printk("ricoh618_power_off failed \n");
	return ret;
}


static int ricoh618_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct ricoh618 *ricoh618 = container_of(gc, struct ricoh618, gpio_chip);
	uint8_t val;
	int ret;

	ret = ricoh618_read(ricoh618->dev, RICOH618_GPIO_MON_IOIN, &val);
	if (ret < 0)
		return ret;

	return ((val & (0x1 << offset)) != 0);
}

static void ricoh618_gpio_set(struct gpio_chip *gc, unsigned offset,
			int value)
{
	struct ricoh618 *ricoh618 = container_of(gc, struct ricoh618, gpio_chip);
	if (value)
		ricoh618_set_bits(ricoh618->dev, RICOH618_GPIO_IOOUT,
						1 << offset);
	else
		ricoh618_clr_bits(ricoh618->dev, RICOH618_GPIO_IOOUT,
						1 << offset);
}

static int ricoh618_gpio_input(struct gpio_chip *gc, unsigned offset)
{
	struct ricoh618 *ricoh618 = container_of(gc, struct ricoh618, gpio_chip);

	return ricoh618_clr_bits(ricoh618->dev, RICOH618_GPIO_IOSEL,
						1 << offset);
}

static int ricoh618_gpio_output(struct gpio_chip *gc, unsigned offset,
				int value)
{
	struct ricoh618 *ricoh618 = container_of(gc, struct ricoh618, gpio_chip);

	ricoh618_gpio_set(gc, offset, value);
	return ricoh618_set_bits(ricoh618->dev, RICOH618_GPIO_IOSEL,
						1 << offset);
}

static int ricoh618_gpio_to_irq(struct gpio_chip *gc, unsigned off)
{
	struct ricoh618 *ricoh618 = container_of(gc, struct ricoh618, gpio_chip);

	if ((off >= 0) && (off < 8))
		return ricoh618->irq_base + RICOH618_IRQ_GPIO0 + off;

	return -EIO;
}


static void __devinit ricoh618_gpio_init(struct ricoh618 *ricoh618,
	struct ricoh618_platform_data *pdata)
{
	int ret;
	int i;
	struct ricoh618_gpio_init_data *ginit;

	if (pdata->gpio_base  <= 0)
		return;

	for (i = 0; i < pdata->num_gpioinit_data; ++i) {
		ginit = &pdata->gpio_init_data[i];

		if (!ginit->init_apply)
			continue;

		if (ginit->output_mode_en) {
			/* GPIO output mode */
			if (ginit->output_val)
				/* output H */
				ret = ricoh618_set_bits(ricoh618->dev,
					RICOH618_GPIO_IOOUT, 1 << i);
			else
				/* output L */
				ret = ricoh618_clr_bits(ricoh618->dev,
					RICOH618_GPIO_IOOUT, 1 << i);
			if (!ret)
				ret = ricoh618_set_bits(ricoh618->dev,
					RICOH618_GPIO_IOSEL, 1 << i);
		} else
			/* GPIO input mode */
			ret = ricoh618_clr_bits(ricoh618->dev,
					RICOH618_GPIO_IOSEL, 1 << i);

		/* if LED function enabled in OTP */
		if (ginit->led_mode) {
			/* LED Mode 1 */
			if (i==0)	/* GP0 */
				ret = ricoh618_set_bits(ricoh618->dev,
						RICOH618_GPIO_LED_FUNC, 0x04 | (ginit->led_func & 0x03));
			if (i==1)	/* GP1 */
				ret = ricoh618_set_bits(ricoh618->dev,
						RICOH618_GPIO_LED_FUNC, 0x40 | (ginit->led_func & 0x03) << 4);

		}


		if (ret < 0)
			dev_err(ricoh618->dev, "Gpio %d init "
				"dir configuration failed: %d\n", i, ret);

	}

	ricoh618->gpio_chip.owner		= THIS_MODULE;
	ricoh618->gpio_chip.label		= ricoh618->client->name;
	ricoh618->gpio_chip.dev			= ricoh618->dev;
	ricoh618->gpio_chip.base		= pdata->gpio_base;
	ricoh618->gpio_chip.ngpio		= RICOH618_NR_GPIO;
	ricoh618->gpio_chip.can_sleep	= 1;

	ricoh618->gpio_chip.direction_input	= ricoh618_gpio_input;
	ricoh618->gpio_chip.direction_output	= ricoh618_gpio_output;
	ricoh618->gpio_chip.set			= ricoh618_gpio_set;
	ricoh618->gpio_chip.get			= ricoh618_gpio_get;
	ricoh618->gpio_chip.to_irq	  	= ricoh618_gpio_to_irq;

	ret = gpiochip_add(&ricoh618->gpio_chip);
	if (ret)
		dev_warn(ricoh618->dev, "GPIO registration failed: %d\n", ret);
}

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
static void print_regs(const char *header, struct i2c_client *client,
		int start_offset, int end_offset)
{
	uint8_t reg_val;
	int i;
	int ret;

	/*seq_printf("%s\n", header);*/
	printk("%s\n", header);
	for (i = start_offset; i <= end_offset; ++i) {
		ret = __ricoh618_read(client, i, &reg_val);
		if (ret >= 0)
			/*seq_printf(s, "Reg 0x%02x Value 0x%02x\n", i, reg_val);*/
			printk("Reg 0x%02x Value 0x%02x\n", i, reg_val);
	}
	printk("------------------\n");
}

/*static int dbg_ricoh_show(struct seq_file *s, void *unused)*/
static int dbg_ricoh_show(struct ricoh618 *ricoh)
{
	/*struct ricoh618 *ricoh = s->private;*/
	struct i2c_client *client = ricoh->client;

	printk("RICOH618 Registers\n");
	printk("------------------\n");

	print_regs("System Regs",		client, 0x0, 0x05);
	print_regs("Power Control Regs", client, 0x07, 0x2B);
	print_regs("DCDC  Regs",		client, 0x2C, 0x43);
	print_regs("LDO   Regs",		client, 0x44, 0x5C);
	print_regs("ADC   Regs",		client, 0x64, 0x8F);
	print_regs("GPIO  Regs",		client, 0x90, 0x9B);
	print_regs("INTC  Regs",		client, 0x9C, 0x9E);
	print_regs("OPT   Regs",		client, 0xB0, 0xB1);
	print_regs("CHG   Regs",		client, 0xB2, 0xDF);
	print_regs("FUEL  Regs",		client, 0xE0, 0xFC);
	return 0;
}

static int dbg_ricoh_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	printk("   come %s \n",__func__);
	return 0;
	/*return single_open(file, dbg_ricoh_show, inode->i_private);*/
}

static int dbg_ricoh_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[32];
	ssize_t buf_size;
	char *start = buf;
	unsigned long reg, value;
	int ret = 0;
	struct ricoh618 *ricoh = file->private_data;

	buf_size = min(count, (sizeof(buf) - 1));
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;
	buf[buf_size] = 0;

	while(*start == ' ')
		start++;
	reg = simple_strtoul(start, &start, 16);
	while(*start == ' ')
		start++;
	if (strict_strtoul(start, 16, &value))
		return -EINVAL;

	add_taint(TAINT_USER);

	printk("   %s  reg: 0x%lx  value: 0x%lx\n",__func__,reg,value);
	ret = __ricoh618_write(ricoh->client,(u8)reg, (uint8_t)value);
	if (ret)
		printk("       write 0x%lx register failed!\n", reg);
	else
		printk("       write successed\n");

	return buf_size;
}

static int dbg_ricoh_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	struct ricoh618 *ricoh = file->private_data;
	/*struct i2c_client *client = ricoh->client;*/
	char *buf;
	ssize_t ret;

	if (*ppos < 0 || !count)
		return -EINVAL;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = dbg_ricoh_show(ricoh);
	if (ret >= 0) {
        if (copy_to_user(user_buf, buf, ret)) {
			kfree(buf);
			return -EFAULT;
		}
		*ppos += ret;
	}

	kfree(buf);
	return ret;
}

static const struct file_operations debug_fops = {
	.open		= dbg_ricoh_open,
	/*.read		= seq_read,
	.llseek		= seq_lseek,*/
	.read		= dbg_ricoh_read,
	.write		= dbg_ricoh_write,
	.llseek		= seq_lseek,

	.release	= single_release,
};
static void __init ricoh618_debuginit(struct ricoh618 *ricoh)
{
	(void)debugfs_create_file("ricoh618", S_IRWXUGO, NULL,
			ricoh, &debug_fops);
}
#else
static void __init ricoh618_debuginit(struct ricoh618 *ricoh)
{
	return;
}
#endif

static struct mfd_cell ricoh618_cells[] = {
	{
		.id = 0,
		.name = "ricoh618-regulator",
	},
	{
		.id = 1,
		.name = "ricoh618-battery",
	},
};

static int ricoh618_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct ricoh618 *ricoh618;
	struct pmu_platform_data *pmu_pdata = dev_get_platdata(&i2c->dev);
	struct ricoh618_platform_data *pdata =
		(struct ricoh618_platform_data *)pmu_pdata->private;
	int ret;

	printk("hagahaga %s: \n",__func__);

	ricoh618 = kzalloc(sizeof(struct ricoh618), GFP_KERNEL);
	if (ricoh618 == NULL)
		return -ENOMEM;

	ricoh618->client = i2c;
	ricoh618->dev = &i2c->dev;
	i2c_set_clientdata(i2c, ricoh618);

	mutex_init(&ricoh618->io_lock);

	ricoh618->bank_num = 0;
	ricoh618->ricoh618_notifier.jz_notify = ricoh618_reset_notifier_handler;
	ricoh618->ricoh618_notifier.level = NOTEFY_PROI_NORMAL;
	ricoh618->ricoh618_notifier.msg = JZ_POST_HIBERNATION;

	ricoh618_read_pwroff_on(ricoh618->dev);

	if (gpio_request_one(pmu_pdata->gpio,
			     GPIOF_DIR_IN, "ricoh618_irq")) {
		dev_err(ricoh618->dev, "no irq pin available\n");
		pmu_pdata->gpio = -EBUSY;
	}

	i2c->irq = gpio_to_irq(pmu_pdata->gpio);
	if (i2c->irq) {
		ret = ricoh618_irq_init(ricoh618, i2c->irq, pdata->irq_base);
		if (ret) {
			dev_err(&i2c->dev, "IRQ init failed: %d\n", ret);
			goto err_irq_init;
		}
	}

	ret = mfd_add_devices(ricoh618->dev, -1,
			      ricoh618_cells, ARRAY_SIZE(ricoh618_cells),
			      NULL, 0);

	if (ret) {
		dev_err(&i2c->dev, "add mfd devices failed: %d\n", ret);
		goto err_add_devs;
	}

	ret = ricoh618_register_reset_notifier(&(ricoh618->ricoh618_notifier));
	if (ret) {
		printk("ricoh618_register_reset_notifier failed\n");
		goto err_add_notifier;
	}

	ricoh618_gpio_init(ricoh618, pdata);

	ricoh618_debuginit(ricoh618);

	ricoh618_i2c_client = i2c;
	return 0;

err_add_notifier:
err_add_devs:
	if (i2c->irq)
		ricoh618_irq_exit(ricoh618);
err_irq_init:
	kfree(ricoh618);
	return ret;
}

static int  __devexit ricoh618_i2c_remove(struct i2c_client *i2c)
{
	int ret = 0;
	struct ricoh618 *ricoh618 = i2c_get_clientdata(i2c);

	if (i2c->irq)
		ricoh618_irq_exit(ricoh618);

	ret = ricoh618_unregister_reset_notifier(&(ricoh618->ricoh618_notifier));
	if (ret) {
		printk("ricoh618_register_reset_notifier failed\n");
	}

	mfd_remove_devices(ricoh618->dev);
	kfree(ricoh618);
	return ret;
}

#ifdef CONFIG_PM
/*
 * IRQ should be enabled when suspend, debug it later.
 */
/*
*/
static int ricoh618_i2c_suspend(struct i2c_client *i2c, pm_message_t state)
{
	printk("hagahaga %s: \n",__func__);

	if (i2c->irq)
		disable_irq(i2c->irq);
	return 0;
}


int pwrkey_wakeup;
static int ricoh618_i2c_resume(struct i2c_client *i2c)
{
	uint8_t reg_val;
	int ret;

	printk(KERN_INFO "PMU: %s:\n", __func__);

	ret = __ricoh618_read(i2c, RICOH618_INT_IR_SYS, &reg_val);
	if (reg_val & 0x01) { /* If PWR_KEY wakeup */
		printk(KERN_INFO "PMU: %s: PWR_KEY Wakeup\n", __func__);
		pwrkey_wakeup = 1;
		/* Clear PWR_KEY IRQ */
		__ricoh618_write(i2c, RICOH618_INT_IR_SYS, 0x0);
	}

	if (i2c->irq)
		enable_irq(i2c->irq);
	return 0;
}
#endif

static const struct i2c_device_id ricoh618_i2c_id[] = {
	{"ricoh618", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, ricoh618_i2c_id);

static struct i2c_driver ricoh618_i2c_driver = {
	.driver = {
		   .name = "ricoh618",
		   .owner = THIS_MODULE,
		   },
	.probe = ricoh618_i2c_probe,
	.remove = __devexit_p(ricoh618_i2c_remove),
#ifdef CONFIG_PM
/*
	.suspend = ricoh618_i2c_suspend,
	.resume = ricoh618_i2c_resume,
*/
#endif
	.id_table = ricoh618_i2c_id,
};


static int __init ricoh618_i2c_init(void)
{
	int ret = -ENODEV;

	printk("hagahaga %s: \n",__func__);

	ret = i2c_add_driver(&ricoh618_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register I2C driver: %d\n", ret);

	return ret;
}

subsys_initcall(ricoh618_i2c_init);

static void __exit ricoh618_i2c_exit(void)
{
	i2c_del_driver(&ricoh618_i2c_driver);
}

module_exit(ricoh618_i2c_exit);

MODULE_DESCRIPTION("RICOH RN5T618 PMU multi-function core driver");
MODULE_LICENSE("GPL");
