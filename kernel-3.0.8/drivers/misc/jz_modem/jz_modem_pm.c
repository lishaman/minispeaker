
/**
 * jz_modem.c - the driver for the modem which is connected to JZ SOC.
 *
 * Copyright (c) 2010  Ingenic Semiconductor Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/modem_pm.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#undef DEBUG
//#define DEBUG 
#ifdef DEBUG
#define dprintk(x...)	printk(x)
#else
#define dprintk(x...)
#endif

static int registered_ops_nums = 0;
static struct modem_ops *registered_ops_list[MAX_CARDS] = {NULL};
static struct modem_t registered_modems[MAX_CARDS];
struct jz_ringin {
        struct input_dev *dev;
        struct delayed_work wake_work;
        struct wake_lock wakelock;
} ringin;


/*
 * fops routines
 */
static int modem_open(struct inode *inode, struct file *filp);
static int modem_release(struct inode *inode, struct file *filp);
static ssize_t modem_read(struct file *filp, char *buf, size_t size, loff_t *l);
static ssize_t modem_write(struct file *filp, const char *buf, size_t size, loff_t *l);
static long modem_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);

static struct file_operations modem_fops = 
{
	open:		modem_open,
	release:	modem_release,
	read:		modem_read,
	write:		modem_write,
	unlocked_ioctl:		modem_ioctl,
};

static int modem_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int modem_release(struct inode *inode, struct file *filp)
{
    return 0;	
}

static ssize_t modem_read(struct file *filp, char *buf, size_t size, loff_t *l)
{
	printk("modem: read is not implemented\n");
	return -1;
}

static ssize_t modem_write(struct file *filp, const char *buf, size_t size, loff_t *l)
{
	printk("modem: write is not implemented\n");
	return -1;
}

#define CARDID(x)  ((x) & 0xff)
#define EXTFUNC(x) (((x) & 0xff00) >> 8)

static long modem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
        int cardid = CARDID(arg);
	int extfcun = EXTFUNC(arg);
	struct modem_data *bp_data;
	struct modem_ops *bp_ops;
        //printk("cardId=%d.\n", cardId);
	if (cardid > MAX_CARDS || registered_modems[cardid].bp_ops == NULL ||
		registered_modems[cardid].bp_data == NULL)
		return -EINVAL;
        
	bp_data = registered_modems[cardid].bp_data;
	bp_ops = registered_modems[cardid].bp_ops;

	switch (cmd) {
	case POWER_OFF:
		if (bp_ops->poweroff)
			bp_ops->poweroff(bp_data);
		break;
	 	
        case POWER_ON:
		if (bp_ops->poweron)
			bp_ops->poweron(bp_data);
		break;

        case RESET:
		if (bp_ops->reset)
			bp_ops->reset(bp_data);
		break;

	case WAKEUP:
		if (bp_ops->wakeup)
			bp_ops->wakeup(bp_data);
		break;

 	default:
		printk("The case 0x%x is not supported in modem ioctl!\n", cmd);
        }
        
	return ret;
}

static struct miscdevice modem_dev = {
	MODEM_MINOR,
	"jz_modem",
	&modem_fops
};

int modem_register_ops(struct modem_ops *ops) 
{
	if (registered_ops_nums >= MAX_CARDS || ops == NULL)
		return -EINVAL;
	registered_ops_list[registered_ops_nums++] = ops;
	return 0;
}

static void wake_up_screen_work(struct work_struct *work)
{
//	if (get_earlysuspend_state()) {
//		clear_earlysuspend_state();
		input_report_key(ringin.dev, KEY_MENU, 1);
		input_report_key(ringin.dev, KEY_MENU, 0);
		printk("modem wake up screen.\n");
//	}
}

static irqreturn_t ringin_irq(int irq, void *data)
{
	wake_lock_timeout(&ringin.wakelock, 1000);
	schedule_delayed_work(&ringin.wake_work, HZ / 2);

	return IRQ_HANDLED;
}

static int init_ringin(struct platform_device *pdev)
{
	int ret;

	ringin.dev = input_allocate_device();
	if (!ringin.dev) {
		dev_err(&pdev->dev, "Can't allocate input dev\n");
		ret = -ENOMEM;
		goto err;
	}

	ringin.dev->evbit[0] = BIT_MASK(EV_KEY);
	ringin.dev->name = "ringin";
	ringin.dev->phys = "ringin/input0";
	ringin.dev->dev.parent = &pdev->dev;
	set_bit(KEY_MENU, ringin.dev->keybit);

	INIT_DELAYED_WORK(&ringin.wake_work, wake_up_screen_work);
	wake_lock_init(&ringin.wakelock, WAKE_LOCK_SUSPEND, "ringin");

	ret = input_register_device(ringin.dev);
	if (ret) {
                dev_dbg(&pdev->dev, "Can't register input device: %d\n", ret);
		goto err;
	}
    
        return 0;

err:
	if (ringin.dev)
		input_free_device(ringin.dev);
	return ret;
}

static int init_modems(struct modem_platform_data *pdata)
{
	int i, j, id;
	int ret = 0;
	int irq, level;

	for (i=0; i<MAX_CARDS; i++) {
		registered_modems[i].bp_ops = NULL;
		registered_modems[i].bp_data = NULL;
	}

	for (i=0; i<pdata->bp_nums; i++) {
		id = pdata->bp[i].id;
		if (id > MAX_CARDS)
			continue;
		for (j=0; j<registered_ops_nums; j++) {
			if (0 == strcmp(pdata->bp[i].name, registered_ops_list[j]->name))
				break;
		}
		if (j >= registered_ops_nums) {
			printk("%s, wrong board modem data!", __func__);
			goto error;	
		}
		registered_modems[id].bp_ops = registered_ops_list[j];
		registered_modems[id].bp_data = &pdata->bp[i];
		if (registered_modems[id].bp_ops->init)
			registered_modems[id].bp_ops->init(&pdata->bp[i]);

		if (pdata->bp[i].bp_wake_ap.gpio <= 0) {
			printk("%s, bad bp wakeup pin!", __func__);
			goto error;
		}
		gpio_request(pdata->bp[i].bp_wake_ap.gpio, "bp_wake");
		irq = gpio_to_irq(pdata->bp[i].bp_wake_ap.gpio);
		level = pdata->bp[i].bp_wake_ap.active_level ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING;
		ret = request_irq(irq, ringin_irq, IRQF_DISABLED | IRQF_SHARED |  level, 
				"ringin", &pdata->bp[i]);
		if (ret < 0) {
			printk("%s, register irq failed!", __func__);
			goto error;
		}
		disable_irq(irq);
		irq_set_irq_wake(irq, 1);
	}

	return 0;

error:
	for (j=0; j<i; j++) {
		irq = gpio_to_irq(pdata->bp[i].bp_wake_ap.gpio);
		free_irq (irq, &pdata->bp[i]);
	}
	return -EINVAL;
}

#ifdef CONFIG_PM
static int modem_suspend(struct platform_device *pdev, pm_message_t state)
{
	int i, irq;
	for (i=0; i<MAX_CARDS; i++) {
		if (registered_modems[i].bp_ops && registered_modems[i].bp_data) {
			registered_modems[i].bp_ops->suspend(registered_modems[i].bp_data);
			irq = gpio_to_irq(registered_modems[i].bp_data->bp_wake_ap.gpio);
			enable_irq (irq);
		}
	}
        return 0;
}

static int modem_resume(struct platform_device *pdev)
{
	int i, irq;
	for (i=0; i<MAX_CARDS; i++) {
		if (registered_modems[i].bp_ops && registered_modems[i].bp_data) {
			registered_modems[i].bp_ops->resume(registered_modems[i].bp_data);
			irq = gpio_to_irq(registered_modems[i].bp_data->bp_wake_ap.gpio);
			disable_irq (irq);
		}
	}
	return 0;
}

#else
#define modem_suspend NULL
#define modem_resume NULL
#endif

static int modem_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i;
   	struct modem_platform_data *pdata = pdev->dev.platform_data;

	ret = init_ringin(pdev);
	if (ret < 0)
		goto err_init_ringin;

	ret = init_modems(pdata);	
	if (ret < 0)
		goto err_init_modems;
 
	ret = misc_register(&modem_dev);
	if (ret < 0)
		goto err_register_misc;

	printk("Driver of MODEM registered.\n");
	return 0;

err_register_misc:
	for (i=0; i<pdata->bp_nums; i++) {
		int irq = gpio_to_irq(pdata->bp[i].bp_wake_ap.gpio);
		free_irq (irq, &pdata->bp[i]);
	}	
err_init_modems:
	if (ringin.dev)
		input_free_device(ringin.dev);
err_init_ringin:
	return ret;
}

static int __devexit modem_remove(struct platform_device *pdev)
{
	int i, irq;
   	struct modem_platform_data *pdata = pdev->dev.platform_data;
	
	for (i=0; i<pdata->bp_nums; i++) {
		irq = gpio_to_irq(pdata->bp[i].bp_wake_ap.gpio);
		free_irq (irq, &pdata->bp[i]);
	}
	if (ringin.dev)
		input_free_device(ringin.dev);	
        misc_deregister(&modem_dev);
        return 0;
}


static struct platform_driver bp_driver = {
        .driver.name    = "jz-modem",
        .driver.owner   = THIS_MODULE,
	.suspend	= modem_suspend,
	.resume		= modem_resume,
        .probe          = modem_probe,
        .remove         = modem_remove,
};

static int __init modem_init(void)
{
        return platform_driver_register(&bp_driver);
}

static void __exit modem_exit(void)
{
        platform_driver_unregister(&bp_driver);
}

late_initcall(modem_init);
module_exit(modem_exit);

MODULE_ALIAS("platform:jz-modem");
MODULE_DESCRIPTION("JZ SOC modem");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("<jbyu@ingenic.cn>");
