/*
 *  CPU frequency scaling for JZ47XX SOCS
 *
 *  Copyright (C) 2012 Ingenic Corporation
 *  Written by ztyan<ztyan@ingenic.cn>
 *
 *  Based on cpu-sa1110.c, Copyright (C) 2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/opp.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <jz_notifier.h>


static struct jz_cpufreq {
	unsigned int second_rate;
	unsigned int set_rate;
	struct mutex mutex;
	struct clk *cpu_clk;
	struct cpufreq_frequency_table *freq_table;
	struct jz_notifier freq_up_change;
	struct delayed_work freq_up_work;
} *jz_cpufreq;

static int m200_verify_speed(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, jz_cpufreq->freq_table);
}

static unsigned int m200_getspeed(unsigned int cpu)
{
	if (cpu >= NR_CPUS)
		return 0;
	return clk_get_rate(jz_cpufreq->cpu_clk) / 1000;
}
static unsigned int m200_getavg(struct cpufreq_policy *policy,
			   unsigned int cpu)
{
	if (cpu >= NR_CPUS)
		return 0;
	if(jz_cpufreq->set_rate)
		return policy->max;
	else
		return policy->cur;
}
static int m200_target(struct cpufreq_policy *policy,
			 unsigned int target_freq,
			 unsigned int relation)
{
	int index;
	int ret = 0;
	struct cpufreq_freqs freqs;
	ret = cpufreq_frequency_table_target(policy, jz_cpufreq->freq_table, target_freq, relation, &index);
	if (ret) {
		printk("%s: cpu%d: no freq match for %d(ret=%d)\n",
		       __func__, policy->cpu, target_freq, ret);
		return ret;
	}
	freqs.new = jz_cpufreq->freq_table[index].frequency;
	freqs.old = m200_getspeed(policy->cpu);
	freqs.cpu = policy->cpu;

	if (freqs.old == freqs.new && policy->cur == freqs.new) {
		return ret;
	}
	mutex_lock(&jz_cpufreq->mutex);
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	if (!freqs.new) {
		printk("%s: cpu%d: no match for freq %d\n", __func__,
		       policy->cpu, target_freq);
		while(1);
	}
	ret = clk_set_rate(jz_cpufreq->cpu_clk, freqs.new * 1000);
	/* notifiers */
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	mutex_unlock(&jz_cpufreq->mutex);
	return ret;
}
#define CPUFRQ_MIN  12000
extern struct cpufreq_frequency_table *init_freq_table(unsigned int max_freq,
						       unsigned int min_freq);
static void freq_up_irq_work(struct work_struct *work)
{
	jz_cpufreq->set_rate = 0;
}
static int freq_up_change_notify(struct jz_notifier *notify,void *v)
{
	unsigned int current_rate;
	current_rate = clk_get_rate(jz_cpufreq->cpu_clk) / 1000;
	if(current_rate >= jz_cpufreq->second_rate)
		return NOTIFY_OK;
	cancel_delayed_work(&jz_cpufreq->freq_up_work);
	jz_cpufreq->set_rate = 1;
	schedule_delayed_work(&jz_cpufreq->freq_up_work, msecs_to_jiffies(2000));
	return NOTIFY_OK;
}
static int __cpuinit m200_cpu_init(struct cpufreq_policy *policy)
{
	unsigned int i, max_freq;

	jz_cpufreq = (struct jz_cpufreq *)kzalloc(sizeof(struct jz_cpufreq), GFP_KERNEL);
	if(!jz_cpufreq) {
		pr_err("kzalloc fail!!!\n");
		return -1;
	}
	jz_cpufreq->cpu_clk = clk_get(NULL, "cclk");
	if (IS_ERR(jz_cpufreq->cpu_clk))
		goto cpu_clk_err;

	max_freq = clk_get_rate(jz_cpufreq->cpu_clk) / 1000;
	if(max_freq <= 0) {
		printk("get cclk max freq fail %d\n", max_freq);
		goto freq_table_err;
	}
	jz_cpufreq->freq_table = init_freq_table(max_freq, CPUFRQ_MIN);
	if(!jz_cpufreq->freq_table) {
		printk("get freq table error!!\n");
		goto freq_table_err;
	}
	printk("freq table is:");
	for(i = 0; jz_cpufreq->freq_table[i].frequency != CPUFREQ_TABLE_END; i++)
		printk(" %d", jz_cpufreq->freq_table[i].frequency);
	printk("\n");
	jz_cpufreq->second_rate = jz_cpufreq->freq_table[i-2].frequency;
	jz_cpufreq->set_rate = 0;
	printk("jz_cpufreq->second_rate = %d\n", jz_cpufreq->second_rate);
	if(cpufreq_frequency_table_cpuinfo(policy, jz_cpufreq->freq_table))
		goto freq_table_err;
	cpufreq_frequency_table_get_attr(jz_cpufreq->freq_table, policy->cpu);
	policy->min = policy->cpuinfo.min_freq;
	policy->max = policy->cpuinfo.max_freq;
	printk("policy min %d max %d\n",policy->min, policy->max);
	policy->cur = m200_getspeed(policy->cpu);
	/*
	 * On JZ47XX SMP configuartion, both processors share the voltage
	 * and clock. So both CPUs needs to be scaled together and hence
	 * needs software co-ordination. Use cpufreq affected_cpus
	 * interface to handle this scenario.
	 */
	policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
	cpumask_setall(policy->cpus);
	/* 300us for latency. FIXME: what's the actual transition time? */
	policy->cpuinfo.transition_latency = 100 * 1000;

	mutex_init(&jz_cpufreq->mutex);
	INIT_DELAYED_WORK(&jz_cpufreq->freq_up_work, freq_up_irq_work);

	jz_cpufreq->freq_up_change.jz_notify = freq_up_change_notify;
	jz_cpufreq->freq_up_change.level = NOTEFY_PROI_NORMAL;
	jz_cpufreq->freq_up_change.msg = JZ_CLK_CHANGING;
	jz_notifier_register(&jz_cpufreq->freq_up_change, NOTEFY_PROI_NORMAL);

	printk("cpu freq init ok!\n");
	return 0;
freq_table_err:
	printk("init freq_table_err fail!\n");
	clk_put(jz_cpufreq->cpu_clk);
cpu_clk_err:
	printk("init cpu_clk_err fail!\n");
	kfree(jz_cpufreq);
	return -1;
}

static int m200_cpu_suspend(struct cpufreq_policy *policy)
{
	return 0;
}

static int m200_cpu_resume(struct cpufreq_policy *policy)
{
	printk("cpufreq not resume to adj freq!\n");
	return -1;
}

static struct freq_attr *m200_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};
static struct cpufreq_driver m200_driver = {
	.name		= "m200",
	.flags		= CPUFREQ_STICKY,
	.verify		= m200_verify_speed,
	.target		= m200_target,
	.getavg         = m200_getavg,
	.get		= m200_getspeed,
	.init		= m200_cpu_init,
	.suspend	= m200_cpu_suspend,
	.resume		= m200_cpu_resume,
	.attr		= m200_cpufreq_attr,
};

static int __init m200_cpufreq_init(void)
{
	return cpufreq_register_driver(&m200_driver);
}

MODULE_AUTHOR("ztyan<ztyan@ingenic.cn>");
MODULE_DESCRIPTION("cpufreq driver for JZ47XX SoCs");
MODULE_LICENSE("GPL");
module_init(m200_cpufreq_init);
