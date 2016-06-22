/*
 * drivers/regulator/ricoh618-regulator.c
 *
 * Regulator driver for RICOH618 power management chip.
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
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/ricoh618.h>
#include <linux/mfd/pmu-common.h>
#include <linux/regulator/ricoh618-regulator.h>

#define RICOH618_ONKEY_TRIGGER_LEVEL	0
#define RICOH618_ONKEY_OFF_IRQ		0

int test_value = 0;
struct ricoh618_reg {
	struct device		*dev;
	struct ricoh618		*iodev;
	struct regulator_dev	**rdev;
};

struct ricoh618_regulator {
	int		id;
	int		sleep_id;
	/* Regulator register address.*/
	u8		reg_en_reg;
	u8		en_bit;
	u8		reg_disc_reg;
	u8		disc_bit;
	u8		vout_reg;
	u8		vout_mask;
	u8		vout_reg_cache;
	u8		sleep_reg;

	/* chip constraints on regulator behavior */
	int			min_uV;
	int			max_uV;
	int			step_uV;
	int			nsteps;

	/* regulator specific turn-on delay */
	u16			delay;

	/* used by regulator core */
	struct regulator_desc	desc;

	/* Device */
	struct device		*dev;

};

struct ricoh618_pwrkey {
	struct device *dev;
	struct input_dev *pwr;
	#if RICOH618_ONKEY_TRIGGER_LEVEL
		struct timer_list timer;
	#endif
	struct workqueue_struct *workqueue;
	struct work_struct work;
	unsigned long delay;
	int key_irq;
	bool pressed_first;
	struct ricoh618_pwrkey_platform_data *pdata;
	spinlock_t lock;
};

struct ricoh618_pwrkey *g_pwrkey;

#if RICOH618_ONKEY_TRIGGER_LEVEL
void ricoh618_pwrkey_timer(unsigned long t)
{
	queue_work(g_pwrkey->workqueue, &g_pwrkey->work);
}
#endif

static void ricoh618_irq_work(struct work_struct *work)
{
	/* unsigned long flags; */
	uint8_t val;

	printk(KERN_INFO "PMU: %s:\n", __func__);
	/* spin_lock_irqsave(&g_pwrkey->lock, flags); */

	if (pwrkey_wakeup) {
		printk(KERN_INFO "PMU: %s: pwrkey_wakeup\n", __func__);
		pwrkey_wakeup = 0;
		input_event(g_pwrkey->pwr, EV_KEY, KEY_POWER, 1);
		input_event(g_pwrkey->pwr, EV_SYN, 0, 0);
		input_event(g_pwrkey->pwr, EV_KEY, KEY_POWER, 0);
		input_event(g_pwrkey->pwr, EV_SYN, 0, 0);

		return;
	}

	ricoh618_read(g_pwrkey->dev->parent, RICOH618_INT_MON_SYS, &val);
	dev_dbg(g_pwrkey->dev, "pwrkey is pressed?(0x%x): 0x%x\n",
						RICOH618_INT_MON_SYS, val);
	printk(KERN_INFO "PMU: %s: val=0x%x\n", __func__, val);
	val &= 0x1;
	if (val) {
		#if (RICOH618_ONKEY_TRIGGER_LEVEL)
		g_pwrkey->timer.expires = jiffies + g_pwrkey->delay;
		dd_timer(&g_pwrkey->timer);
		#endif
		if (!g_pwrkey->pressed_first) {
			g_pwrkey->pressed_first = true;
			printk(KERN_INFO "PMU1: %s: Power Key!!!\n", __func__);
			/* input_report_key(g_pwrkey->pwr, KEY_POWER, 1); */
			/* input_sync(g_pwrkey->pwr); */
			input_event(g_pwrkey->pwr, EV_KEY, KEY_POWER, 1);
			input_event(g_pwrkey->pwr, EV_SYN, 0, 0);
		}
	} else {
		if (g_pwrkey->pressed_first) {
			printk(KERN_INFO "PMU2: %s: Power Key!!!\n", __func__);
			/* input_report_key(g_pwrkey->pwr, KEY_POWER, 0); */
			/* input_sync(g_pwrkey->pwr); */
			input_event(g_pwrkey->pwr, EV_KEY, KEY_POWER, 0);
			input_event(g_pwrkey->pwr, EV_SYN, 0, 0);
		}
		g_pwrkey->pressed_first = false;
	}

	/* spin_unlock_irqrestore(&g_pwrkey->lock, flags); */
}

static irqreturn_t pwrkey_irq(int irq, void *_pwrkey)
{
	printk(KERN_INFO "PMU: %s:\n", __func__);

	#if (RICOH618_ONKEY_TRIGGER_LEVEL)
	g_pwrkey->timer.expires = jiffies + g_pwrkey->delay;
	add_timer(&g_pwrkey->timer);
	#else
	queue_work(g_pwrkey->workqueue, &g_pwrkey->work);
	#endif
	return IRQ_HANDLED;
}

#if RICOH618_ONKEY_OFF_IRQ
static irqreturn_t pwrkey_irq_off(int irq, void *_pwrkey)
{
	dev_warn(g_pwrkey->dev, "ONKEY is pressed long time!\n");
	return IRQ_HANDLED;
}
#endif

static inline struct device *to_ricoh618_dev(struct regulator_dev *rdev)
{
	return rdev_get_dev(rdev)->parent->parent;
}

static int ricoh618_regulator_enable_time(struct regulator_dev *rdev)
{
	struct ricoh618_regulator *ri = rdev_get_drvdata(rdev);

	return ri->delay;
}

static int ricoh618_reg_is_enabled(struct regulator_dev *rdev)
{
	struct ricoh618_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_ricoh618_dev(rdev);
	uint8_t control;
	int ret;

	ret = ricoh618_read(parent, ri->reg_en_reg, &control);
	if (ret < 0) {
		dev_err(&rdev->dev, "Error in reading the control register\n");
		return ret;
	}
	return (((control >> ri->en_bit) & 1) == 1);
}

static int ricoh618_reg_enable(struct regulator_dev *rdev)
{
	struct ricoh618_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_ricoh618_dev(rdev);
	int ret;

	ret = ricoh618_set_bits(parent, ri->reg_en_reg, (1 << ri->en_bit));
	if (ret < 0) {
		dev_err(&rdev->dev, "Error in updating the STATE register\n");
		return ret;
	}
	udelay(ri->delay);
	return ret;
}

static int ricoh618_reg_disable(struct regulator_dev *rdev)
{
	struct ricoh618_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_ricoh618_dev(rdev);
	int ret;

	ret = ricoh618_clr_bits(parent, ri->reg_en_reg, (1 << ri->en_bit));
	if (ret < 0)
		dev_err(&rdev->dev, "Error in updating the STATE register\n");

	return ret;
}

static int ricoh618_list_voltage(struct regulator_dev *rdev, unsigned index)
{
	struct ricoh618_regulator *ri = rdev_get_drvdata(rdev);

	return ri->min_uV + ((ri->step_uV ) * index);
}

#ifdef RICOH618_SLEEP_MODE
static int __ricoh618_set_s_voltage(struct device *parent,
		struct ricoh618_regulator *ri, int min_uV, int max_uV)
{
	int vsel;
	int ret;

	if ((min_uV < ri->min_uV) || (max_uV > ri->max_uV))
		return -EDOM;

	vsel = (min_uV - ri->min_uV + ri->step_uV - 1)/ri->step_uV;
	if (vsel > ri->nsteps)
		return -EDOM;

	ret = ricoh618_update(parent, ri->sleep_reg, vsel, ri->vout_mask);
	if (ret < 0)
		dev_err(ri->dev, "Error in writing the sleep register\n");
	return ret;
}
#endif

static int __ricoh618_set_voltage(struct device *parent,
		struct ricoh618_regulator *ri, int min_uV, int max_uV,
		unsigned *selector)
{
	int vsel;
	int ret;
	uint8_t vout_val;
	/*uint8_t vout_val1;
	uint8_t reg_en_test;*/

	if ((min_uV < ri->min_uV) || (max_uV > ri->max_uV))
		return -EDOM;

	vsel = (min_uV - ri->min_uV + ri->step_uV - 1)/ri->step_uV;
	if (vsel > ri->nsteps)
		return -EDOM;

	if (selector)
		*selector = vsel;

	vout_val = (ri->vout_reg_cache & ~ri->vout_mask) |
				(vsel & ri->vout_mask);
	ret = ricoh618_write(parent, ri->vout_reg, vout_val);
	if (ret < 0)
		dev_err(ri->dev, "Error in writing the Voltage register\n");
	else
		ri->vout_reg_cache = vout_val;
	/*
	ret = ricoh618_read(parent, ri->vout_reg, &vout_val1);
	if (ret < 0) {
		dev_err(&parent, "Error in reading the control register\n");
		return ret;
	}
	ret = ricoh618_set_bits(parent, ri->reg_en_reg, (1 << ri->en_bit));
	if (ret < 0) {
		dev_err(&ri->dev, "Error in updating the STATE register\n");
		return ret;
	}
	udelay(ri->delay);

	ret = ricoh618_read(parent, ri->reg_en_reg, &reg_en_test);
	printk("----> set_voltage vout_val = %d , vout_val1 = %d en_reg_bit = %x!!!! \n", vout_val, vout_val1, reg_en_test);
	if (vout_val != vout_val1)
		printk("\n\n");
	*/

	return ret;
}

static int ricoh618_set_voltage(struct regulator_dev *rdev,
		int min_uV, int max_uV, unsigned *selector)
{
	struct ricoh618_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_ricoh618_dev(rdev);

	return __ricoh618_set_voltage(parent, ri, min_uV, max_uV, selector);
}

static int ricoh618_get_voltage(struct regulator_dev *rdev)
{
	struct ricoh618_regulator *ri = rdev_get_drvdata(rdev);
	uint8_t vsel;

	vsel = ri->vout_reg_cache & ri->vout_mask;
	return ri->min_uV + vsel * ri->step_uV;
}

static struct regulator_ops ricoh618_ops = {
	.list_voltage	= ricoh618_list_voltage,
	.set_voltage	= ricoh618_set_voltage,
	.get_voltage	= ricoh618_get_voltage,
	.enable		= ricoh618_reg_enable,
	.disable	= ricoh618_reg_disable,
	.is_enabled	= ricoh618_reg_is_enabled,
	.enable_time	= ricoh618_regulator_enable_time,
};

#define RICOH618_REG(_id, _en_reg, _en_bit, _disc_reg, _disc_bit, _vout_reg, \
		_vout_mask, _ds_reg, _min_mv, _max_mv, _step_uV, _nsteps,    \
		_ops, _delay)		\
{								\
	.reg_en_reg	= _en_reg,				\
	.en_bit		= _en_bit,				\
	.reg_disc_reg	= _disc_reg,				\
	.disc_bit	= _disc_bit,				\
	.vout_reg	= _vout_reg,				\
	.vout_mask	= _vout_mask,				\
	.sleep_reg	= _ds_reg,				\
	.min_uV		= _min_mv * 1000,			\
	.max_uV		= _max_mv * 1000,			\
	.step_uV	= _step_uV,				\
	.nsteps		= _nsteps,				\
	.delay		= _delay,				\
	.id		= RICOH618_ID_##_id,			\
	.desc = {						\
		.name = ricoh618_rails(_id),			\
		.id = RICOH618_ID_##_id,			\
		.n_voltages = _nsteps,				\
		.ops = &_ops,					\
		.type = REGULATOR_VOLTAGE,			\
		.owner = THIS_MODULE,				\
	},							\
}

/*	.sleep_id	= RICOH618_DS_##_id,			\ */

static struct ricoh618_regulator ricoh618_regulators[] = {
	RICOH618_REG(DC1, 0x2C, 0, 0x2C, 1, 0x36, 0xFF, 0x3B,
			600, 3500, 12500, 0xE8, ricoh618_ops, 500),
	RICOH618_REG(DC3, 0x30, 0, 0x30, 1, 0x38, 0xFF, 0x3D,
			600, 3500, 12500, 0xE8, ricoh618_ops, 500),
	RICOH618_REG(LDO2, 0x44, 1, 0x46, 1, 0x4D, 0x7F, 0x59,
			900, 3500, 25000, 0x68, ricoh618_ops, 500),
	RICOH618_REG(LDO3, 0x44, 2, 0x46, 2, 0x4E, 0x7F, 0x5A,
			600, 3500, 25000, 0x74, ricoh618_ops, 500),
	RICOH618_REG(LDO4, 0x44, 3, 0x46, 3, 0x4F, 0x7F, 0x5B,
			900, 3500, 25000, 0x68, ricoh618_ops, 500),
	RICOH618_REG(VBUS, 0xb3, 1, 0xb3, 1, -1, -1, -1,
			-1, -1, -1, -1, ricoh618_ops, 500),
};

static inline struct ricoh618_regulator *find_regulator_info(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ricoh618_regulators); i++) {
		if (!strcmp(ricoh618_regulators[i].desc.name, name))
			return &ricoh618_regulators[i];
	}

	return NULL;
}

static int ricoh618_set_longpress(struct device *parent, int delay)
{
	int ret;
	uint8_t val;

	ricoh618_read(parent, RICOH618_PWR_ON_TIMSET, &val);
	val &= ~(0x7 << 4);
	ret = ricoh618_write(parent, RICOH618_PWR_ON_TIMSET, val | ricoh618_longpress_pwr[CONFIG_RICOH618_LONGPRESS_PWOFF]);
	if (ret < 0) {
		dev_err(parent, "Error in updating the STATE register\n");
		return ret;
	}

	/*ricoh618_read(parent, RICOH618_PWR_ON_TIMSET, &val);
	printk("----> set the longpress 0x%x  time = 0x%x \n\n", val,ricoh618_longpress_pwr[CONFIG_RICOH618_LONGPRESS_PWOFF]);*/

	udelay(delay);

	return ret;
}

static inline int ricoh618_cache_regulator_register(struct device *parent,
	struct ricoh618_regulator *ri)
{
	ri->vout_reg_cache = 0;
	return ricoh618_read(parent, ri->vout_reg, &ri->vout_reg_cache);
}

static int __devinit ricoh618_regulator_probe(struct platform_device *pdev)
{
	struct ricoh618 *iodev = dev_get_drvdata(pdev->dev.parent);
	struct ricoh618_reg *ricoh618_reg;
	struct pmu_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct regulator_dev **rdev;
	struct ricoh618_pwrkey *pwrkey;
	struct input_dev *pwr;
	int key_irq;
	int i, size;
	int err;
	uint8_t val;

	printk("-----> register ricoh618 regulator!!!\n");
	ricoh618_reg = kzalloc(sizeof(struct ricoh618_reg), GFP_KERNEL);
	if (!ricoh618_reg)
		return -ENOMEM;

	key_irq = (IRQ_RESERVED_BASE + RICOH618_IRQ_POWER_ON);
	printk(KERN_INFO "PMU1: %s: key_irq=%d\n", __func__, key_irq);
	pwrkey = kzalloc(sizeof(*pwrkey), GFP_KERNEL);
	if (!pwrkey)
		return -ENOMEM;

	pwrkey->dev = &pdev->dev;
	pwrkey->pressed_first = false;
	pwrkey->delay = HZ / 1000 ; 
	g_pwrkey = pwrkey;

	pwr = input_allocate_device();
	if (!pwr) {
		dev_dbg(&pdev->dev, "Can't allocate power button\n");
		err = -ENOMEM;
		goto free_pwrkey;
	}
	input_set_capability(pwr, EV_KEY, KEY_POWER);
	pwr->name = "ricoh618_pwrkey";
	pwr->phys = "ricoh618_pwrkey/input0";
	pwr->dev.parent = &pdev->dev;

	#if RICOH618_ONKEY_TRIGGER_LEVEL
	init_timer(&pwrkey->timer);
	pwrkey->timer.function = ricoh618_pwrkey_timer;
	#endif

	spin_lock_init(&pwrkey->lock);
	err = input_register_device(pwr);
	if (err) {
		dev_dbg(&pdev->dev, "Can't register power key: %d\n", err);
		goto free_input_dev;
	}
	pwrkey->key_irq = key_irq;
	pwrkey->pwr = pwr;

	size = sizeof(struct regulator_dev *) * pdata->num_regulators;

	ricoh618_reg->rdev = kzalloc(size, GFP_KERNEL);
	if (!ricoh618_reg->rdev) {
		kfree(ricoh618_reg);
		return -ENOMEM;
	}

	rdev = ricoh618_reg->rdev;
	ricoh618_reg->dev = &pdev->dev;
	ricoh618_reg->iodev = iodev;

	platform_set_drvdata(pdev, ricoh618_reg);

	for (i = 0; i < pdata->num_regulators; i++) {
		struct regulator_info *reg_info = &pdata->regulators[i];
		struct ricoh618_regulator *ri = find_regulator_info(reg_info->name);
		if (!ri) {
			dev_err(pdev->dev.parent, "WARNING: can't find regulator: %s\n", reg_info->name);
		} else {
			dev_dbg(pdev->dev.parent, "register regulator: %s\n",reg_info->name);
			err = ricoh618_cache_regulator_register(pdev->dev.parent, ri);
			if (err) {
				dev_err(&pdev->dev, "Fail in caching register\n");
			}
			if (reg_info->init_data)
				rdev[i] = regulator_register(&ri->desc, ricoh618_reg->dev,
					reg_info->init_data, ri);
			if (IS_ERR_OR_NULL(rdev[i])) {
				dev_err(&pdev->dev, "failed to register regulator %s\n",
				ri->desc.name);
				rdev[i] = NULL;
			}
			if (rdev[i] && ri->desc.ops->is_enabled && ri->desc.ops->is_enabled(rdev[i])) {
				rdev[i]->use_count++;
			}
		}
	}

#ifdef CONFIG_CHARGER_RICOH618
	struct ricoh618_regulator *ri = find_regulator_info("USB_CHARGER");
	if (ri) {
		err = ricoh618_cache_regulator_register(pdev->dev.parent, ri);
		if (err) {
			dev_err(&pdev->dev, "Fail in caching register\n");
		}

		rdev[i] = regulator_register(&ri->desc, ricoh618_reg->dev,
		reg_info->init_data, ricoh618_reg);
		if (IS_ERR_OR_NULL(rdev)) {
			err = PTR_ERR(rdev[i]);
			dev_err(&pdev->dev, "failed to register regulator %s\n",
			ri->desc.name);
			rdev[i] = NULL;
			goto error;
		}
		if (rdev[i] && ri->desc.ops->is_enabled && ri->desc.ops->is_enabled(rdev[i])) {
			rdev[i]->use_count++;
		}
	}

#endif
	err = ricoh618_set_longpress(pdev->dev.parent, 500);
	if (err) {
		dev_err(&pdev->dev, "Fail in caching register\n");
	}

	/* Check if power-key is pressed at boot up */
	err = ricoh618_read(pwrkey->dev->parent, RICOH618_INT_MON_SYS, &val);
	if (err < 0) {
		dev_err(&pdev->dev, "Key-press status at boot failed rc=%d\n",
									 err);
		goto unreg_input_dev;
	}
	val &= 0x1;
	if (val) {
		input_report_key(pwrkey->pwr, KEY_POWER, 1);
		printk(KERN_INFO "******KEY_POWER:1\n");
		input_sync(pwrkey->pwr);
		pwrkey->pressed_first = true;
	}

	#if !(RICOH618_ONKEY_TRIGGER_LEVEL)
		/* trigger both edge */
		ricoh618_set_bits(pwrkey->dev->parent, RICOH618_PWR_IRSEL, 0x1);
	#endif

	err = request_threaded_irq(key_irq, NULL, pwrkey_irq,
		IRQF_ONESHOT, "ricoh618_pwrkey", pwrkey);
	if (err < 0) {
		dev_err(&pdev->dev, "Can't get %d IRQ for pwrkey: %d\n",
								key_irq, err);
		goto unreg_input_dev;
	}

	#if RICOH618_ONKEY_OFF_IRQ
	err = request_threaded_irq(key_irq + RICOH618_IRQ_ONKEY_OFF, NULL,
						pwrkey_irq_off, IRQF_ONESHOT,
						"ricoh618_pwrkey_off", pwrkey);
	if (err < 0) {
		dev_err(&pdev->dev, "Can't get %d IRQ for pwrkey: %d\n",
			key_irq + RICOH618_IRQ_ONKEY_OFF, err);
		free_irq(key_irq, pwrkey);
		goto unreg_input_dev;
	}
	#endif

	pwrkey->workqueue = create_singlethread_workqueue("ricoh618_pwrkey");
	INIT_WORK(&pwrkey->work, ricoh618_irq_work);

	/* Enable power key IRQ */
	/* trigger both edge */
	ricoh618_set_bits(pwrkey->dev->parent, RICOH618_PWR_IRSEL, 0x1);
	/* Enable system interrupt */
	ricoh618_set_bits(pwrkey->dev->parent, RICOH618_INTC_INTEN, 0x1);
	/* Enable power-on interrupt */
	ricoh618_set_bits(pwrkey->dev->parent, RICOH618_INT_EN_SYS, 0x1);
	printk(KERN_INFO "PMU: %s is OK!\n", __func__);

	return 0;

unreg_input_dev:
	input_unregister_device(pwr);
	pwr = NULL;

free_input_dev:
	input_free_device(pwr);
free_pwrkey:
	kfree(pwrkey);

	return err;

#ifdef CONFIG_CHARGER_RICOH618
error:
	for (i = 0; i < pdata->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);

	kfree(ricoh618_reg->rdev);
	kfree(ricoh618_reg);

	return err;
#endif
}

#ifdef CONFIG_PM
static int ricoh618_regulator_suspend(struct device *dev)
{
	printk(KERN_INFO "PMU: %s\n", __func__);

	if (g_pwrkey->key_irq)
		disable_irq(g_pwrkey->key_irq);
	cancel_work_sync(&g_pwrkey->work);
	flush_workqueue(g_pwrkey->workqueue);

	return 0;
}

static int ricoh618_regulator_resume(struct device *dev)
{
	printk(KERN_INFO "PMU: %s\n", __func__);
	queue_work(g_pwrkey->workqueue, &g_pwrkey->work);
	if (g_pwrkey->key_irq)
		enable_irq(g_pwrkey->key_irq);

	return 0;
}

static const struct dev_pm_ops ricoh618_regulator_pm_ops = {
	.suspend	= ricoh618_regulator_suspend,
	.resume		= ricoh618_regulator_resume,
};
#endif

static int __devexit ricoh618_regulator_remove(struct platform_device *pdev)
{
	struct regulator_dev *rdev = platform_get_drvdata(pdev);

	flush_workqueue(g_pwrkey->workqueue);
	destroy_workqueue(g_pwrkey->workqueue);
	free_irq(g_pwrkey->key_irq, g_pwrkey);
	input_unregister_device(g_pwrkey->pwr);
	kfree(g_pwrkey);

	regulator_unregister(rdev);
	return 0;
}

static struct platform_driver ricoh618_regulator_driver = {
	.driver	= {
		.name	= "ricoh618-regulator",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &ricoh618_regulator_pm_ops,
#endif
	},
	.probe		= ricoh618_regulator_probe,
	.remove		= __devexit_p(ricoh618_regulator_remove),
};

static int __init ricoh618_regulator_init(void)
{
	return platform_driver_register(&ricoh618_regulator_driver);
}
subsys_initcall(ricoh618_regulator_init);

static void __exit ricoh618_regulator_exit(void)
{
	platform_driver_unregister(&ricoh618_regulator_driver);
}
module_exit(ricoh618_regulator_exit);

MODULE_DESCRIPTION("RICOH618 regulator driver");
MODULE_ALIAS("platform:ricoh618-regulator");
MODULE_LICENSE("GPL");
