/* 
 *
 * Description:
 * 		Bluetooth power driver with rfkill interface and bluetooth host wakeup support.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/gpio.h>
#include <linux/bcm4330-rfkill.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <mach/jzmmc.h>

#define DEV_NAME		"bcm4330_bt_power"

#if 1
#define	DBG_MSG(fmt, ...)	printk(fmt, ##__VA_ARGS__)
#else
#define DBG_MSG(fmt, ...)
#endif 

static int bt_power_state = 0;
static int bt_power_control(int enable);
static int bt_rst_n ;
static int bt_reg_on;
static int bt_wake;
static int bt_int;
static struct regulator *power;

//static DEFINE_SPINLOCK(bt_power_lock);
struct mutex bt_power_lock;

extern void wlan_pw_en_enable(void);
extern void wlan_pw_en_disable(void);
extern void clk_32k_on (void);
extern void clk_32k_off (void);

#ifdef CONFIG_BT_HOST_WAKEUP
static irqreturn_t bt_host_wake_handler(int irq, void* dev_id)
{
	printk("%s\n", __func__);

	
	gpio_direction_output(bt_int,0);
	

	return IRQ_HANDLED;
}
#endif

/* For compile only, remove later */
#define RFKILL_STATE_SOFT_BLOCKED	0
#define RFKILL_STATE_UNBLOCKED		1





static void bt_enable_power()
{
	wlan_pw_en_enable();
	gpio_direction_output(bt_reg_on,1);
}


static void bt_disable_power()
{
	wlan_pw_en_disable();
	gpio_direction_output(bt_reg_on,0);
}



static int bt_power_control(int enable)
{
	if (enable == bt_power_state)
		return 0;

printk("cljiang------bt_power_control,enable = %d\n",enable);
	switch (enable)	{
	case RFKILL_STATE_SOFT_BLOCKED:
		clk_32k_off();
		regulator_disable(power); 
		bt_disable_power();
		break;
	case RFKILL_STATE_UNBLOCKED:
		clk_32k_on();
		regulator_enable(power);  /*vddio must be set*/
		gpio_direction_output(bt_rst_n,0);
		bt_enable_power();
		mdelay(15);
		gpio_set_value(bt_rst_n,1);
		mdelay(15);
		break;
	default:
		break;
	}

	bt_power_state = enable;

	return 0;
}

static bool first_called = true;

static int bt_rfkill_set_block(void *data, bool blocked)
{
	int ret;

	if (!first_called) {
		//spin_lock(&bt_power_lock);
		if (mutex_lock_interruptible(&bt_power_lock) != 0) {
			printk("bt_rfkill_set_block---------->mutex_lock_interruptible : fail\n");
			ret = -EIO;
		}
		ret = bt_power_control(blocked ? 0 : 1);
		//spin_unlock(&bt_power_lock);
		mutex_unlock(&bt_power_lock);
	} else {
		first_called = false;
		return 0;
	}

	return ret;
}

static const struct rfkill_ops bt_rfkill_ops = {
	.set_block = bt_rfkill_set_block,
};

static int bt_power_rfkill_probe(struct platform_device *pdev)
{
	struct bcm4330_rfkill_platform_data *pdata = pdev->dev.platform_data;
	int ret = -ENOMEM;
	
	mutex_init(&bt_power_lock);

	pdata->rfkill = rfkill_alloc("bluetooth", &pdev->dev, RFKILL_TYPE_BLUETOOTH, 
                            &bt_rfkill_ops, NULL);

	if (!pdata->rfkill) {
		goto exit;
	}


	ret = rfkill_register(pdata->rfkill);
	if (ret) {
		rfkill_destroy(pdata->rfkill);
		return ret;
	} else {
		platform_set_drvdata(pdev, pdata->rfkill);
	}
exit:
	return ret;
}

static void bt_power_rfkill_remove(struct platform_device *pdev)
{
	struct bcm4330_rfkill_platform_data *pdata = pdev->dev.platform_data;

	
	pdata->rfkill = platform_get_drvdata(pdev);
	if (pdata->rfkill)
		rfkill_unregister(pdata->rfkill);

	platform_set_drvdata(pdev, NULL);
}

int bt_power_suspend(struct platform_device *pdev, pm_message_t state)
{	

	/* Configure BT_INT as wakeup source */
#ifdef CONFIG_BT_HOST_WAKEUP
	gpio_direction_input(bt_int,0);
#endif
	printk("cljiang---bt_power_suspend\n");
	gpio_set_value(bt_int,0);
	gpio_set_value(bt_wake,0);

	return 0;
}

int bt_power_resume(struct platform_device *pdev)
{	

	printk("cljiang---bt_power_resume\n");
	if (bt_power_state == 1) {

 	}
	gpio_set_value(bt_int,1);
	gpio_set_value(bt_wake,1);

	return 0;
}

static int __init_or_module bt_power_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct bcm4330_rfkill_platform_data *pdata;


	if (!ret && !bt_power_state && bt_power_rfkill_probe(pdev)) {
		ret = -ENOMEM;
	}

	pdata = pdev->dev.platform_data;

	bt_rst_n = pdata->gpio.bt_rst_n;
	bt_reg_on = pdata->gpio.bt_reg_on;
	bt_wake = pdata->gpio.bt_wake;
	bt_int = pdata->gpio.bt_int;

	ret = gpio_request(bt_rst_n,"bcm4330_bt_rst_n");
	if(unlikely(ret)){
		return ret;
	}

	ret = gpio_request(bt_reg_on,"bcm4330_bt_reg_on");
	if(unlikely(ret)){
		gpio_free(bt_rst_n);
		return ret;
	}
	

	ret = gpio_request(bt_int,"bcm4330_bt_int");
	if(unlikely(ret)){
		gpio_free(bt_rst_n);
		gpio_free(bt_reg_on);
		return ret;
	}

	ret = gpio_request(bt_wake,"bcm4330_bt_wake");

	if(unlikely(ret)){
		gpio_free(bt_rst_n);
		gpio_free(bt_reg_on);
		gpio_free(bt_int);
		return ret;
	}

	power = regulator_get(NULL, "vwifi");  /*vddio must be set*/
	if (IS_ERR(power)) {
		printk("bt regulator missing\n");
//		return -EINVAL;
	}

	/* Register wake up source as interrupt */
#ifdef CONFIG_BT_HOST_WAKEUP
	ret = request_irq(gpio_to_irq(bt_int), bt_host_wake_handler,
			IRQF_DISABLED | IRQF_TRIGGER_HIGH, DEV_NAME, NULL);
	if(ret){
		return ret;
	}
#endif
	gpio_direction_output(bt_int,1);

	gpio_direction_output(bt_wake,1);

	gpio_direction_output(bt_rst_n,1);

	return 0;
}

static int __devexit bt_power_remove(struct platform_device *pdev)
{
	int ret;


	bt_power_rfkill_remove(pdev);

	//spin_lock(&bt_power_lock);
	if (mutex_lock_interruptible(&bt_power_lock) != 0) {
		printk("bt_power_remove--->mutex_lock_interruptible : fail\n");
		ret = -EIO;
	}
	bt_power_state = 0;
	ret = bt_power_control(bt_power_state);
	//spin_unlock(&bt_power_lock);
	mutex_unlock(&bt_power_lock);

	return ret;
}


static struct platform_driver bcm4330_bt_power_driver = {
	.probe = bt_power_probe,
	.remove = __devexit_p(bt_power_remove),
	.driver = {
		.name = DEV_NAME,
		.owner = THIS_MODULE,
	},
	.suspend = bt_power_suspend,
	.resume = bt_power_resume,
};

static int __init bt_power_init(void)
{
	int ret;

	ret = platform_driver_register(&bcm4330_bt_power_driver);
	
	return ret;
}

static void __exit bt_power_exit(void)
{
	platform_driver_unregister(&bcm4330_bt_power_driver);
}

module_init(bt_power_init);
module_exit(bt_power_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Bluetooth power control driver");
MODULE_VERSION("1.0");
