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

#include <soc/ddr.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <jz_proc.h>

/*
 *  unit: kHz
 */
static struct timer_list timer;
static void timercount1(unsigned long data);
static void timercount2(unsigned long data);
static void timercount3(unsigned long data);
static int  timer_flag = 0;

typedef int (*DUMP_CALLBACK)(char *, const char *, ...);

int dump_out_ddr(char *str, DUMP_CALLBACK dump_callback)
{
	int len = 0;
	int i = 0;
	len += dump_callback(str + len,"--------------------dump the DDRC1---------------\n");

	len += dump_callback(str + len,"DDRC_STATUS\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_STATUS),DDRC_BASE + DDRC_STATUS);
	len += dump_callback(str + len,"DDRC_CFG\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_CFG),DDRC_BASE + DDRC_CFG);
	len += dump_callback(str + len,"DDRC_CTRL\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_CTRL),DDRC_BASE + DDRC_CTRL);
	len += dump_callback(str + len,"DDRC_LMR\t:0x%08x\taddress\t:0x%08x\n",ddr_readl(DDRC_LMR),DDRC_BASE + DDRC_LMR);
	len += dump_callback(str + len,"DDRC_TIMING1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(1)),DDRC_BASE + DDRC_TIMING(1));
	len += dump_callback(str + len,"DDRC_TIMING2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(2)),DDRC_BASE + DDRC_TIMING(2));
	len += dump_callback(str + len,"DDRC_TIMING3\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(3)),DDRC_BASE + DDRC_TIMING(3));
	len += dump_callback(str + len,"DDRC_TIMING4\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(4)),DDRC_BASE + DDRC_TIMING(4));
	len += dump_callback(str + len,"DDRC_TIMING5\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(5)),DDRC_BASE + DDRC_TIMING(5));
	len += dump_callback(str + len,"DDRC_TIMING6\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(6)),DDRC_BASE + DDRC_TIMING(6));
	len += dump_callback(str + len,"DDRC_REFCNT\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REFCNT),DDRC_BASE + DDRC_REFCNT);
	len += dump_callback(str + len,"DDRC_MMAP0\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_MMAP0),DDRC_BASE + DDRC_MMAP0);
	len += dump_callback(str + len,"DDRC_MMAP1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_MMAP1),DDRC_BASE + DDRC_MMAP1);
	len += dump_callback(str + len,"DDRC_REMAP1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(1)),DDRC_BASE + DDRC_REMAP(1));
	len += dump_callback(str + len,"DDRC_REMAP2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(2)),DDRC_BASE + DDRC_REMAP(2));
	len += dump_callback(str + len,"DDRC_REMAP3\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(3)),DDRC_BASE + DDRC_REMAP(3));
	len += dump_callback(str + len,"DDRC_REMAP4\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(4)),DDRC_BASE + DDRC_REMAP(4));
	len += dump_callback(str + len,"DDRC_REMAP5\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(5)),DDRC_BASE + DDRC_REMAP(5));
	len += dump_callback(str + len,"DDRC_AUTOSR_EN\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_AUTOSR_EN),DDRC_BASE + DDRC_AUTOSR_EN);

	len += dump_callback(str + len,"--------------------dump the DDRP---------------\n");

	len += dump_callback(str + len,"DDRP_PIR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PIR),DDRC_BASE + DDRP_PIR);
	len += dump_callback(str + len,"DDRP_PGCR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PGCR),DDRC_BASE + DDRP_PGCR);
	len += dump_callback(str + len,"DDRP_PGSR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PGSR),DDRC_BASE + DDRP_PGSR);
	len += dump_callback(str + len,"DDRP_PTR0\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PTR0),DDRC_BASE + DDRP_PTR0);
	len += dump_callback(str + len,"DDRP_PTR1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PTR1),DDRC_BASE + DDRP_PTR1);
	len += dump_callback(str + len,"DDRP_PTR2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PTR2),DDRC_BASE + DDRP_PTR2);
	len += dump_callback(str + len,"DDRP_DSGCR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_DSGCR),DDRC_BASE + DDRP_DSGCR);
	len += dump_callback(str + len,"DDRP_DCR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_DCR),DDRC_BASE + DDRP_DCR);
	len += dump_callback(str + len,"DDRP_DTPR0\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_DTPR0),DDRC_BASE + DDRP_DTPR0);
	len += dump_callback(str + len,"DDRP_DTPR1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_DTPR1),DDRC_BASE + DDRP_DTPR1);
	len += dump_callback(str + len,"DDRP_DTPR2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_DTPR2),DDRC_BASE + DDRP_DTPR2);
	len += dump_callback(str + len,"DDRP_MR0\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_MR0),DDRC_BASE + DDRP_MR0);
	len += dump_callback(str + len,"DDRP_MR1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_MR1),DDRC_BASE + DDRP_MR1);
	len += dump_callback(str + len,"DDRP_MR2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_MR2),DDRC_BASE + DDRP_MR2);
	len += dump_callback(str + len,"DDRP_MR3\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_MR3),DDRC_BASE + DDRP_MR3);
	len += dump_callback(str + len,"DDRP_ODTCR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_ODTCR),DDRC_BASE + DDRP_ODTCR);
	for(i=0;i<4;i++) {
		len += dump_callback(str + len,"DX%dGSR0 \t:0x%08x\taddress\t:0x%08x\n", i, ddr_readl(DDRP_DXGSR0(i)),DDRC_BASE + DDRP_DXGSR0(i));
		len += dump_callback(str + len,"@pas:DXDQSTR(%d)\t:0x%08x\taddress\t:0x%08x\n", i,ddr_readl(DDRP_DXDQSTR(i)),DDRC_BASE + DDRP_DXDQSTR(i));
	}
	return len;
}
static int ddr_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int len = 0;
	len += dump_out_ddr(page,sprintf);
	return len;
}

static int ddr_mon1_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	printk("\nSimple MODE to collect DDR rate \n\n");
	printk("Usage: For Example\n");
	printk("********************************************\n");
	printk("Start: echo 1 > /proc/jz/ddr/ddr_monitor1\n");
	printk("Stop : echo 0 > /proc/jz/ddr/ddr_monitor1\n");
	printk("********************************************\n");
	return 0;
}

static int ddr_mon1_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int i;
	i = simple_strtoul(buffer,NULL,0);

	if(i == 0){
		printk("***********DDR Monitor1 CLOSED *************\n");
		del_timer_sync(&timer);
	}
	if(i == 1){
		printk("***********DDR Monitor1 START **************\n");
		*((volatile unsigned int *)0xb34f030c) = (0x10000000 | 0x3ff);
		msleep(1000);

		setup_timer(&timer, timercount1, (unsigned long)NULL);
		mod_timer(&timer, jiffies+10);
	}
	return count;
}

static void timercount1(unsigned long data)
{
	unsigned int i = 0;

	i = *((volatile unsigned int *)0xb34f0310);
	printk(KERN_DEBUG "total_cycles = %d,valid_cycles = %d\n", 1023, i);
	printk(KERN_DEBUG "rate = %%%d\n",i * 100 / 1023);
	mod_timer(&timer,jiffies + msecs_to_jiffies(1000));
}

static int ddr_mon2_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	printk("\n\nMODE2 to collect DDR's  [ Using && IDLE ] rate \n\n");
	printk("Usage: For Example\n");
	printk("********************************************\n");
	printk("Start: echo 1 > /proc/jz/ddr/ddr_monitor2\n");
	printk("Stop : echo 0 > /proc/jz/ddr/ddr_monitor2\n");
	printk("********************************************\n");

	return 0;
}

static int ddr_mon2_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int i;

	i = *((volatile unsigned int *)0xb00000d0);
	i |= (1<<6);
	*((volatile unsigned int *)0xb00000d0) = i;
	printk("ddr_drcg = 0x%x\n",*((volatile unsigned int *)0xb00000d0));

	i = simple_strtoul(buffer,NULL,0);

	if(i == 0){
		printk("***********DDR Monitor2 CLOSED *************\n");
		del_timer_sync(&timer);
	}
	if(i == 1){
		printk("***********DDR Monitor2 START **************\n");
		setup_timer(&timer, timercount2, (unsigned long)NULL);
		mod_timer(&timer, jiffies+10);
	}

	return count;
}

static void timercount2(unsigned long data)
{
	unsigned int i,j,k;
	i = 0;
	j = 0;
	k = 0;
	if(!timer_flag) {
		*((volatile unsigned int *)0xb34f00d4) = 0;
		*((volatile unsigned int *)0xb34f00d8) = 0;
		*((volatile unsigned int *)0xb34f00dc) = 0;
		*((volatile unsigned int *)0xb34f00e4) = 3;
		mod_timer(&timer,jiffies + msecs_to_jiffies(1));
		timer_flag = 1;
	} else {
		*((volatile unsigned int *)0xb34f00e4) = 2;
		i = *((volatile unsigned int *)0xb34f00d4);
		j = *((volatile unsigned int *)0xb34f00d8);
		k = *((volatile unsigned int *)0xb34f00dc);

		printk(KERN_DEBUG "total_cycle = %d,valid_cycle = %d\n",i,j);
		printk(KERN_DEBUG "rate      = %%%d\n",j * 100 / i);
		printk(KERN_DEBUG "idle_rate = %%%d\n\n",k * 100 / i);
		mod_timer(&timer,jiffies + msecs_to_jiffies(999));
		timer_flag = 0;
	}
}

static int ddr_mon3_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	printk("\n\nMODE3 to collect DDR's [ Change_bank && Change_rw && Miss_page ] rate \n\n");
	printk("Usage: For Example\n");
	printk("********************************************\n");
	printk("Start: echo 1 > /proc/jz/ddr/ddr_monitor3\n");
	printk("Stop : echo 0 > /proc/jz/ddr/ddr_monitor3\n");
	printk("********************************************\n");

	return 0;
}

static int ddr_mon3_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int i;

	i = *((volatile unsigned int *)0xb00000d0);
	i |= (1<<6);
	*((volatile unsigned int *)0xb00000d0) = i;
	printk("ddr_drcg = 0x%x\n",*((volatile unsigned int *)0xb00000d0));

	i = simple_strtoul(buffer,NULL,0);

	if(i == 0){
		printk("***********DDR Monitor3 CLOSED *************\n");
		del_timer_sync(&timer);
	}
	if(i == 1){
		printk("***********DDR Monitor3 START **************\n");
		setup_timer(&timer, timercount3, (unsigned long)NULL);
		mod_timer(&timer, jiffies+10);
	}

	return count;
}

static void timercount3(unsigned long data)
{
	unsigned int i,j,k,l;
	i = 0;
	j = 0;
	k = 0;
	l = 0;
	if(!timer_flag) {
		*((volatile unsigned int *)0xb34f00d4) = 0;
		*((volatile unsigned int *)0xb34f00d8) = 0;
		*((volatile unsigned int *)0xb34f00dc) = 0;
		*((volatile unsigned int *)0xb34f00e0) = 0;
		*((volatile unsigned int *)0xb34f00e4) = 1;
		mod_timer(&timer,jiffies + msecs_to_jiffies(1));
		timer_flag = 1;
	} else {
		*((volatile unsigned int *)0xb34f00e4) = 0;
		i = *((volatile unsigned int *)0xb34f00d4);
		j = *((volatile unsigned int *)0xb34f00d8);
		k = *((volatile unsigned int *)0xb34f00dc);
		l = *((volatile unsigned int *)0xb34f00e0);
		printk(KERN_DEBUG "change_bank = %%%d\n",j * 100 / i);
		printk(KERN_DEBUG "change_rw   = %%%d\n",k * 100 / i);
		printk(KERN_DEBUG "miss page   = %%%d\n\n",l * 100 / i);

		mod_timer(&timer,jiffies + msecs_to_jiffies(999));
		timer_flag = 0;
	}
}

static int __init init_ddr_proc(void)
{
	struct proc_dir_entry *res,*p;

#ifdef CONFIG_DDR_DEBUG
	register int bypassmode = 0;
	register int AutoSR_en = 0;

	bypassmode = ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP;
	if(bypassmode) {
		printk("the ddr is in bypass mode\n");
	}else{
		printk("the ddr it not in bypass mode\n");
	}

	AutoSR_en = ddr_readl(DDRC_AUTOSR_EN) & DDRC_AUTOSR_ENABLE;
	if(AutoSR_en) {
		printk("the ddr self_refresh is enable\n");
	}else{
		printk("the ddr self_refrese is not enable\n");
	}

	printk("the ddr remap register is\n");
	printk("DDRC_REMAP1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(1)),DDRC_BASE + DDRC_REMAP(1));
	printk("DDRC_REMAP2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(2)),DDRC_BASE + DDRC_REMAP(2));
	printk("DDRC_REMAP3\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(3)),DDRC_BASE + DDRC_REMAP(3));
	printk("DDRC_REMAP4\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(4)),DDRC_BASE + DDRC_REMAP(4));
	printk("DDRC_REMAP5\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(5)),DDRC_BASE + DDRC_REMAP(5));
#endif
	p = jz_proc_mkdir("ddr");
	if (!p) {
		pr_warning("create_proc_entry for common ddr failed.\n");
		return -ENODEV;
	}
	res = create_proc_entry("ddr_Register", 0444, p);
	if (res) {
		res->read_proc = ddr_read_proc;
		res->write_proc = NULL;
		res->data = NULL;
	}

	res = create_proc_entry("ddr_monitor1", 0444, p);
	if (res) {
		res->read_proc = ddr_mon1_read;
		res->write_proc = ddr_mon1_write;
		res->data = (void *) p;
	}

	res = create_proc_entry("ddr_monitor2", 0444, p);
	if (res) {
		res->read_proc = ddr_mon2_read;
		res->write_proc = ddr_mon2_write;
		res->data = (void *) p;
	}

	res = create_proc_entry("ddr_monitor3", 0444, p);
	if (res) {
		res->read_proc = ddr_mon3_read;
		res->write_proc = ddr_mon3_write;
		res->data = (void *) p;
	}
	return 0;
}

module_init(init_ddr_proc);
