
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/syscore_ops.h>
#include <linux/suspend.h>

#include <jz_notifier.h>
#include "./clk/clk.h"
static struct cpu_core_voltage{
	struct workqueue_struct *down_wq;
	struct delayed_work vol_down_work;
	struct jz_notifier clk_prechange;
	struct jz_notifier clkgate_change;
	struct regulator*  core_vcc;
	unsigned int current_vol;
	unsigned int target_vol;
	unsigned int current_rate;
	unsigned int gpu_adj;
	unsigned int vpu_adj;
	unsigned int msc_adj;
	int is_suspend;
	struct mutex mutex;
}cpu_core_vol;

struct vol_freq
{
	unsigned int u_vol;
	unsigned int k_freq;
}vol_freq[] = {
	{1025000,   25000},
	{1025000,  100000},
	{1050000,  300000},
	{1075000,  600000},
	{1100000,  800000},
	{1125000,  1200000},
};
static unsigned int get_vol_from_freq(struct cpu_core_voltage *pcore,unsigned int k_freq)
{
	int i;
	int min,max;
	int u_vol;
	min = vol_freq[0].u_vol;
	max = vol_freq[ARRAY_SIZE(vol_freq) - 1].u_vol;
	u_vol = max;
	if(k_freq < vol_freq[0].k_freq)
		u_vol = vol_freq[0].u_vol;
	else {
		for(i = 0; i < ARRAY_SIZE(vol_freq) - 1;i++) {
			if((k_freq < vol_freq[i + 1].k_freq) && (k_freq >= vol_freq[i].k_freq))
				u_vol = vol_freq[i + 1].u_vol;
		}
	}
	u_vol += pcore->gpu_adj + pcore->vpu_adj + pcore->msc_adj;
	if(u_vol > max)
		u_vol = max;
	if(u_vol < min)
		u_vol = min;
	return u_vol;
}

static void cpu_core_vol_down(struct work_struct *work)
{
	struct cpu_core_voltage *pcore = container_of(to_delayed_work(work),struct cpu_core_voltage,vol_down_work);
	unsigned int vol;
	//printk(KERN_DEBUG"d rate = %d set vol %d adj %d",pcore->current_rate,pcore->target_vol,pcore->gpu_adj);
	while(pcore->current_vol > pcore->target_vol) {
		vol = pcore->current_vol - 25000;
		if(vol > pcore->target_vol) {
			regulator_set_voltage(pcore->core_vcc,vol,vol);
		}
		pcore->current_vol = vol;

	}

	if(pcore->current_vol != pcore->target_vol) {
		vol = pcore->target_vol;
		regulator_set_voltage(pcore->core_vcc,vol,vol);
	}
	pcore->current_vol = pcore->target_vol;
	//printk(KERN_DEBUG" c:%d\n",pcore->current_vol);
}

static void cpu_core_change(struct cpu_core_voltage *pcore,unsigned int target_vol)
{
	unsigned int current_vol = pcore->current_vol;
	if(!pcore->is_suspend) {
		if(target_vol > current_vol)
		{
			cancel_delayed_work_sync(&pcore->vol_down_work);
			pcore->target_vol = target_vol;
			//printk(KERN_DEBUG"u rate = %d set vol %d adj %d",pcore->current_rate,target_vol,pcore->gpu_adj);
			while(target_vol > current_vol) {
				current_vol += 25000;
				if(target_vol >= current_vol)
					regulator_set_voltage(pcore->core_vcc,current_vol,current_vol);
			}
			if(target_vol != current_vol)
				regulator_set_voltage(pcore->core_vcc,target_vol,target_vol);
			pcore->current_vol = pcore->target_vol;
			//printk(KERN_DEBUG" c:%d\n",pcore->current_vol);

		} else if((target_vol < current_vol) && (target_vol != pcore->target_vol)) {
			cancel_delayed_work_sync(&pcore->vol_down_work);
			pcore->target_vol = target_vol;
			queue_delayed_work(pcore->down_wq,&pcore->vol_down_work,msecs_to_jiffies(1000));

		}
	}

}
static int clkgate_change_notify(struct jz_notifier *notify,void *v)
{
	struct cpu_core_voltage *pcore = container_of(notify,struct cpu_core_voltage,clkgate_change);
	unsigned int val = (unsigned int)v;
	unsigned int on = val & 0x80000000;
	unsigned int clk_id = val & (~0x80000000);
	unsigned int target_vol;

	switch(clk_id) {
	case CLK_ID_GPU:
		mutex_lock(&pcore->mutex);
		pcore->gpu_adj = on ? 100000:0;
		target_vol = get_vol_from_freq(pcore,pcore->current_rate / 1000);
		/* printk(KERN_DEBUG"u target_vol %d current_vol %d clk_data->target_rate = %d \n", */
		/*        target_vol,pcore->current_vol,pcore->current_rate); */
		if(target_vol != pcore->target_vol) {
			//printk("dddddddddddddddddddddddddddddddddddddddd cpu dvfs init\n");
			cpu_core_change(pcore,target_vol);
		}
		mutex_unlock(&pcore->mutex);
		break;
	case CLK_ID_VPU:
		mutex_lock(&pcore->mutex);
		pcore->vpu_adj = on ? 50000:0;
		target_vol = get_vol_from_freq(pcore,pcore->current_rate / 1000);
		/* printk(KERN_DEBUG"u target_vol %d current_vol %d clk_data->target_rate = %d \n", */
		/*        target_vol,pcore->current_vol,pcore->current_rate); */
		if(target_vol != pcore->target_vol) {
			//printk("dddddddddddddddddddddddddddddddddddddddd cpu dvfs init\n");
			cpu_core_change(pcore,target_vol);
		}
		mutex_unlock(&pcore->mutex);
		break;
	case CLK_ID_MSC:
		mutex_lock(&pcore->mutex);
		pcore->msc_adj = on ? 100000:0;
		target_vol = get_vol_from_freq(pcore,pcore->current_rate / 1000);
		/* printk(KERN_DEBUG"u target_vol %d current_vol %d clk_data->target_rate = %d \n", */
		/*        target_vol,pcore->current_vol,pcore->current_rate); */
		if(target_vol != pcore->target_vol) {
			//printk("dddddddddddddddddddddddddddddddddddddddd cpu dvfs init\n");
			cpu_core_change(pcore,target_vol);
		}
		mutex_unlock(&pcore->mutex);
		break;
	}
	return NOTIFY_OK;
}
static int clk_prechange_notify(struct jz_notifier *notify,void *v)
{
	struct cpu_core_voltage *pcore = container_of(notify,struct cpu_core_voltage,clk_prechange);
	struct clk_notify_data *clk_data = (struct clk_notify_data *)v;
	unsigned int target_vol;
	mutex_lock(&pcore->mutex);
	target_vol = get_vol_from_freq(pcore,clk_data->target_rate / 1000);
	/* printk(KERN_DEBUG"u target_vol %d current_vol %d clk_data->target_rate = %ld \n", */
	/*        target_vol,current_vol,clk_data->target_rate); */

	pcore->current_rate = clk_data->target_rate;
	cpu_core_change(pcore,target_vol);
	mutex_unlock(&pcore->mutex);
	return NOTIFY_OK;
}
static int cpu_core_sleep_pm_callback(struct notifier_block *nfb,unsigned long action,void *ignored)
{
	struct cpu_core_voltage *pcore = &cpu_core_vol;
	unsigned int max_vol = vol_freq[ARRAY_SIZE(vol_freq) - 1].u_vol;
	switch (action) {
	case PM_SUSPEND_PREPARE:
		pcore->is_suspend = 1;
		//printk("++vol PM_SUSPEND_PREPARE\n");
		cancel_delayed_work_sync(&pcore->vol_down_work);
		//printk("--vol PM_SUSPEND_PREPARE\n");
		mutex_lock(&pcore->mutex);
		regulator_set_voltage(pcore->core_vcc,max_vol,max_vol);
		regulator_put(pcore->core_vcc);
		mutex_unlock(&pcore->mutex);
		break;
	case PM_POST_SUSPEND:
		//printk("vol PM_POST_SUSPEND\n");
		mutex_lock(&pcore->mutex);
		pcore->is_suspend = 0;
		pcore->core_vcc = regulator_get(NULL,"cpu_core");
		mutex_unlock(&pcore->mutex);
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block cpu_core_sleep_pm_notifier = {
	.notifier_call = cpu_core_sleep_pm_callback,
	.priority = 0,
};

static int __init cpu_core_voltage_init(void)
{
	struct cpu_core_voltage *pcore = &cpu_core_vol;
	pcore->down_wq = alloc_workqueue("vol_down", 0, 1);
	if(!pcore->down_wq)
		return -ENOMEM;
	INIT_DELAYED_WORK(&pcore->vol_down_work,
			  cpu_core_vol_down);
	pcore->core_vcc = regulator_get(NULL,"cpu_core");
	if(IS_ERR(pcore->core_vcc))
		return -1;

	pcore->current_rate = vol_freq[ARRAY_SIZE(vol_freq) - 1].k_freq * 1000;

	mutex_init(&pcore->mutex);
	pcore->current_vol = regulator_get_voltage(pcore->core_vcc);
	pcore->target_vol = pcore->current_vol;
	pcore->clk_prechange.jz_notify = clk_prechange_notify;
	pcore->clk_prechange.level = NOTEFY_PROI_HIGH;
	pcore->clk_prechange.msg = JZ_CLK_PRECHANGE;
	jz_notifier_register(&pcore->clk_prechange, NOTEFY_PROI_HIGH);

	pcore->clkgate_change.jz_notify = clkgate_change_notify;
	pcore->clkgate_change.level = NOTEFY_PROI_HIGH;
	pcore->clkgate_change.msg = JZ_CLKGATE_CHANGE;
	jz_notifier_register(&pcore->clkgate_change, NOTEFY_PROI_HIGH);

	register_pm_notifier(&cpu_core_sleep_pm_notifier);

	//printk("ok!\n");
	return 0;
}
static void __exit cpu_core_voltage__exit(void)
{
	struct cpu_core_voltage *pcore = &cpu_core_vol;
	destroy_workqueue(pcore->down_wq);
	jz_notifier_unregister(&pcore->clk_prechange, NOTEFY_PROI_HIGH);
}

module_init(cpu_core_voltage_init);
module_exit(cpu_core_voltage__exit);
