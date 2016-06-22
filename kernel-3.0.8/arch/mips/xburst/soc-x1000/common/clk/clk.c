/*
 * JZSOC Clock and Power Manager
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/syscore_ops.h>
#include <linux/vmalloc.h>
#include <linux/suspend.h>
#include <linux/cpufreq.h>

#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <jz_proc.h>

#include "clk.h"

static int clk_suspend(void)
{
	printk("clk suspend!\n");
	return 0;
}

static void clk_resume(void)
{
	printk("clk resume!\n");
}
static int clk_sleep_pm_callback(struct notifier_block *nfb,unsigned long action,void *ignored)
{
	switch (action) {
	case PM_SUSPEND_PREPARE:
		printk("clk_sleep_pm_callback PM_SUSPEND_PREPARE\n");
		break;
	case PM_POST_SUSPEND:
		printk("clk_sleep_pm_callback PM_POST_SUSPEND\n");
		break;
	}
	return NOTIFY_OK;
}
static struct notifier_block clk_sleep_pm_notifier = {
	.notifier_call = clk_sleep_pm_callback,
	.priority = 0,
};
struct syscore_ops clk_pm_ops = {
	.suspend = clk_suspend,
	.resume = clk_resume,
};
static void init_clk_parent(struct clk *p) {
	int init = 0;
	if(!p)
		return;
	if(p->init_state) {
		atomic_set(&p->count,1);
		p->init_state = 0;
		init = 1;
	}
	if(atomic_read(&p->count) == 0) {
		printk("%s clk should be opened!\n",p->name);
		atomic_set(&p->count,1);
	}
	if(!init)
		atomic_inc(&p->count);
}

void __init init_all_clk(void)
{
	int i;
	struct clk *clk_srcs = get_clk_from_id(0);
	int clk_srcs_size = get_clk_sources_size();
	for(i = 0; i < clk_srcs_size; i++) {
		clk_srcs[i].CLK_ID = i;

		if(clk_srcs[i].flags & CLK_FLG_CPCCR) {
			init_cpccr_clk(&clk_srcs[i]);
		}
		if(clk_srcs[i].flags & CLK_FLG_CGU) {
			init_cgu_clk(&clk_srcs[i]);
		}

		if(clk_srcs[i].flags & CLK_FLG_CGU_AUDIO){
			init_cgu_audio_clk(&clk_srcs[i]);
		}

		if(clk_srcs[i].flags & CLK_FLG_PLL) {
			init_ext_pll(&clk_srcs[i]);
		}
		if(clk_srcs[i].flags & CLK_FLG_NOALLOC) {
			init_ext_pll(&clk_srcs[i]);
		}
		if(clk_srcs[i].flags & CLK_FLG_GATE) {
			init_gate_clk(&clk_srcs[i]);
		}
		if(clk_srcs[i].flags & CLK_FLG_ENABLE)
			clk_srcs[i].init_state = 1;
	}
	for(i = 0; i < clk_srcs_size; i++) {
		if(clk_srcs[i].parent && clk_srcs[i].init_state)
			init_clk_parent(clk_srcs[i].parent);
	}

	register_syscore_ops(&clk_pm_ops);
	register_pm_notifier(&clk_sleep_pm_notifier);
	printk("CCLK:%luMHz L2CLK:%luMhz H0CLK:%luMHz H2CLK:%luMhz PCLK:%luMhz\n",
			clk_srcs[CLK_ID_CCLK].rate/1000/1000,
			clk_srcs[CLK_ID_L2CLK].rate/1000/1000,
			clk_srcs[CLK_ID_H0CLK].rate/1000/1000,
			clk_srcs[CLK_ID_H2CLK].rate/1000/1000,
			clk_srcs[CLK_ID_PCLK].rate/1000/1000);
}
struct clk *clk_get(struct device *dev, const char *id)
{
	int i;
	struct clk *retval = NULL;
	struct clk *clk_srcs = get_clk_from_id(0);
	struct clk *parent_clk = NULL;
	for(i = 0; i < get_clk_sources_size(); i++) {
		if(id && clk_srcs[i].name && !strcmp(id,clk_srcs[i].name)) {
			if(clk_srcs[i].flags & CLK_FLG_NOALLOC)
				return &clk_srcs[i];
			retval = kzalloc(sizeof(struct clk),GFP_KERNEL | __GFP_FINER);
			if(!retval)
				return ERR_PTR(-ENODEV);
			memcpy(retval,&clk_srcs[i],sizeof(struct clk));
			retval->flags = 0;
			retval->source = &clk_srcs[i];
			if(CLK_FLG_RELATIVE & clk_srcs[i].flags)
			{
				parent_clk = get_clk_from_id(CLK_RELATIVE(clk_srcs[i].flags));
				parent_clk->child = NULL;
			}
			atomic_set(&retval->count,0);
			return retval;
		}
	}
	return ERR_PTR(-EINVAL);
}
EXPORT_SYMBOL(clk_get);

int clk_enable(struct clk *clk)
{
	int count;
	if(!clk)
		return -EINVAL;
	/**
	 * if it has parent clk,first it will control itself,then it will control parent.
	 * if it hasn't parent clk,it will control itself.
	 */
	if(clk->source) {
		count = atomic_inc_return(&clk->count);
		if(count != 1)
			return 0;
		clk->flags |= CLK_FLG_ENABLE;
		clk = clk->source;
		if(clk->init_state) {
			atomic_set(&clk->count,1);
			clk->init_state = 0;
			return 0;
		}
	}
	count = atomic_inc_return(&clk->count);
	if(count == 1) {
		if(clk->parent)
		{
			clk_enable(clk->parent);
		}
		if(clk->ops && clk->ops->enable) {
			clk->ops->enable(clk,1);
		}
		clk->flags |= CLK_FLG_ENABLE;
	}
	return 0;
}
EXPORT_SYMBOL(clk_enable);

int clk_is_enabled(struct clk *clk)
{
	/* if(clk->source) */
	/* 	clk = clk->source; */
	return !!(clk->flags & CLK_FLG_ENABLE);
}
EXPORT_SYMBOL(clk_is_enabled);
void clk_disable(struct clk *clk)
{
	int count;
	if(!clk)
		return;
        /**
	 * if it has parent clk,first it will control itself,then it will control parent.
	 * if it hasn't parent clk,it will control itself.
	 */
	if(clk->source)
	{

		count = atomic_dec_return(&clk->count);
		if(count != 0){
			if(count < 0)
			{
				atomic_set(&clk->count,0);
				printk("%s isn't enabled!\n",clk->name);
				dump_stack();
		return;
	}
		}
		clk->flags &= ~CLK_FLG_ENABLE;
		clk = clk->source;
	}
	count = atomic_dec_return(&clk->count);
	if(count < 0){
		atomic_inc(&clk->count);
		return;
	}
	if(count == 0) {
		if(clk->ops && clk->ops->enable)
			clk->ops->enable(clk,0);
		clk->flags &= ~CLK_FLG_ENABLE;
		if(clk->parent)
			clk_disable(clk->parent);
	}
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	if (!clk)
		return -EINVAL;
	if(clk->source)
		clk = clk->source;
	return clk? clk->rate: 0;
}
EXPORT_SYMBOL(clk_get_rate);

void clk_put(struct clk *clk)
{
	struct clk *parent_clk;
	if(clk && !(clk->flags & CLK_FLG_NOALLOC)) {
		if(clk->source && atomic_read(&clk->count) && atomic_read(&clk->source->count) > 0) {
			if(atomic_dec_return(&clk->source->count) == 0)
				clk->source->init_state = 1;

		}
		if(CLK_FLG_RELATIVE & clk->source->flags)
		{
			parent_clk = get_clk_from_id(CLK_RELATIVE(clk->source->flags));
			parent_clk->child = clk->source;
		}
		kfree(clk);
	}
}
EXPORT_SYMBOL(clk_put);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret = 0;
	if (!clk)
		return -EINVAL;
	if(clk->source)
		clk = clk->source;
	if (!clk->ops || !clk->ops->set_rate)
		return -EINVAL;
	if(clk->rate != rate)
		ret = clk->ops->set_rate(clk, rate);
	return ret;
}
EXPORT_SYMBOL(clk_set_rate);

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int err;

	if (!clk)
		return -EINVAL;
	if(clk->source)
		clk = clk->source;
	if (!clk->ops || !clk->ops->set_rate)
		return -EINVAL;

	err = clk->ops->set_parent(clk, parent);
	clk->rate = clk->ops->get_rate(clk);
	return err;
}
EXPORT_SYMBOL(clk_set_parent);

struct clk *clk_get_parent(struct clk *clk)
{
	if (!clk)
		return NULL;
	return clk->source->parent;
}
EXPORT_SYMBOL(clk_get_parent);

static int clk_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int len = 0;
	len += dump_out_clk(page,sprintf);
	return len;
}
static int clk_enable_proc_write(struct file *file, const char __user *buffer,unsigned long count, void *data)
{
	struct clk *clk = (struct clk *)data;
	if(clk) {
		if(count && (buffer[0] == '1'))
			clk_enable(clk);
		else if(count && (buffer[0] == '0'))
			clk_disable(clk);
		else
			printk("\"echo 1 > enable\" or \"echo 0 > enable \" ");
	}
	return count;
}
static int clk_enable_proc_read(char *page, char **start, off_t off,int count, int *eof, void *data)
{
	struct clk *clk = (struct clk *)data;
	int len;
	len = sprintf("%s\n",clk_is_enabled(clk) ? "enabled" : "disabled");
	return len;
}

static int clk_set_rate_proc_read(char *page, char **start, off_t off,int count, int *eof, void *data)
{
	struct clk *clk = (struct clk *)data;
	int len;
	len = sprintf(page,"rate: %ld\n",clk_get_rate(clk));
	return len;
}

static int clk_set_rate_proc_write(struct file *file, const char __user *buffer,unsigned long count, void *data)
{
	struct clk *clk = (struct clk *)data;
	long rate;
	if(clk) {
		if(kstrtol_from_user(buffer,count,0,&rate) >= 0) {
			clk_set_rate(clk,rate);
		}else
			printk("\"echo 100000000 > rate");
	}
	return count;
}

struct list_clk
{
	struct list_head head;
	struct clk *clk;
	struct proc_dir_entry *p_entry;
};
LIST_HEAD(ctrl_clk_top);

static struct list_clk* ctrl_clk_get_exist(char *name)
{
	struct list_clk *pos_clk;
	list_for_each_entry(pos_clk,&ctrl_clk_top,head) {
		if(strcmp(pos_clk->clk->name,name) == 0) {
			return pos_clk;
		}
	}
	return NULL;
}

static int clk_get_proc_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	struct list_clk *pos_clk;
	int len = 0;
	list_for_each_entry(pos_clk,&ctrl_clk_top,head) {
		len += sprintf(page + len,"%s \t",pos_clk->clk->name);
	}
	len += sprintf(page + len,"\n");
	return len;
}
static void get_str_from_user(unsigned char *str,int strlen,const char *buffer,unsigned long count)
{
	int len = count > strlen-1 ? strlen-1 : count;
	int i;
	if(len == 0) {
		str[0] = 0;
		return;
	}
	copy_from_user(str,buffer,len);
	str[len] = 0;
	for(i = len;i >= 0;i--) {
		if((str[i] == '\r') || (str[i] == '\n'))
			str[i] = 0;
	}
}

static int clk_get_proc_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	struct list_clk *l_clk;
	struct list_head *c_clk_top = &ctrl_clk_top;
	struct proc_dir_entry *res,*p = (struct proc_dir_entry *)data;
	char str[21];
	get_str_from_user(str,21,buffer,count);
	if(strlen(str) > 0 && !ctrl_clk_get_exist(str)) {
		l_clk = vmalloc(sizeof(struct list_clk));
		list_add_tail(&l_clk->head,c_clk_top);
		l_clk->clk = clk_get(NULL,str);
		if(IS_ERR(l_clk->clk)) {
			list_del(&l_clk->head);
			vfree(l_clk);
		}else
		{
			p = proc_mkdir(str,p);
			l_clk->p_entry = p;
			res = create_proc_entry("enable", 0666, p);
			if (res) {
				res->read_proc = clk_enable_proc_read;
				res->write_proc = clk_enable_proc_write;
				res->data = (void *)l_clk->clk;
			}
			res = create_proc_entry("rate", 0666, p);
			if (res) {
				res->read_proc = clk_set_rate_proc_read;
				res->write_proc = clk_set_rate_proc_write;
				res->data = (void *)l_clk->clk;
			}
		}
	}
	return count;
}
static int clk_put_proc_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	struct list_clk *l_clk;
	struct proc_dir_entry *p = (struct proc_dir_entry *)data;
	char str[21];
	get_str_from_user(str,21,buffer,count);
	l_clk = ctrl_clk_get_exist(str);
	if(strlen(str) > 0 && l_clk) {
		list_del(&l_clk->head);
		clk_put(l_clk->clk);
		remove_proc_entry("enable",l_clk->p_entry);
		remove_proc_entry("rate",l_clk->p_entry);
		vfree(l_clk);
		remove_proc_entry(str,p);
	}
	return count;
}

static int __init init_clk_proc(void)
{
	struct proc_dir_entry *res,*p;

	p = jz_proc_mkdir("clock");
	if (!p) {
		pr_warning("create_proc_entry for common clock failed.\n");
		return -ENODEV;
	}
	res = create_proc_entry("clocks", 0444, p);
	if (res) {
		res->read_proc = clk_read_proc;
		res->write_proc = NULL;
		res->data = NULL;
	}
	res = create_proc_entry("get_clk", 0600, p);
	if (res) {
		res->read_proc = clk_get_proc_read;
		res->write_proc = clk_get_proc_write;
		res->data = (void *)p;
	}

	res = create_proc_entry("put_clk", 0600, p);
	if (res) {
		res->read_proc = NULL;
		res->write_proc = clk_put_proc_write;
		res->data = (void *)p;
	}

	return 0;
}

module_init(init_clk_proc);
