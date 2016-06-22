#include <linux/completion.h>
#include <linux/crc-ccitt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/jz_ledrgb.h>

#define DEVICE_NAME	"jz_led_RGB"

#define LED_R_1	gpio_direction_output(jz_led_RGB_pdata->gpio_RGB_R, 1)
#define LED_R_0	gpio_direction_output(jz_led_RGB_pdata->gpio_RGB_R, 0)

#define LED_G_1	gpio_direction_output(jz_led_RGB_pdata->gpio_RGB_G, 1)
#define LED_G_0	gpio_direction_output(jz_led_RGB_pdata->gpio_RGB_G, 0)

#define LED_B_1	gpio_direction_output(jz_led_RGB_pdata->gpio_RGB_B, 1)
#define LED_B_0	gpio_direction_output(jz_led_RGB_pdata->gpio_RGB_B, 0)

static struct miscdevice miscdev;
static struct jz_led_RGB_pdata *jz_led_RGB_pdata;

static int jz_led_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int jz_led_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long jz_led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd){
		case JZ_LED_RGB_OFF:
			if(arg & JZ_LED_RGB_RED_BIT)	 LED_R_0;
			if(arg & JZ_LED_RGB_GREEN_BIT)	 LED_G_0;
			if(arg & JZ_LED_RGB_BLUE_BIT)	 LED_B_0;
			break;
		case JZ_LED_RGB_ON:
			if(arg & JZ_LED_RGB_RED_BIT)	 LED_R_1;
			if(arg & JZ_LED_RGB_GREEN_BIT)	 LED_G_1;
			if(arg & JZ_LED_RGB_BLUE_BIT)	 LED_B_1;
			break;
		case JZ_LED_RGB_SET:
		if(arg>=8){
			LED_R_1; LED_G_0; LED_B_0;mdelay(1000);
			LED_R_0; LED_G_1; LED_B_0;mdelay(1000);
			LED_R_1; LED_G_1; LED_B_0;mdelay(1000);
			LED_R_0; LED_G_0; LED_B_1;mdelay(1000);
			LED_R_1; LED_G_0; LED_B_1;mdelay(1000);
			LED_R_0; LED_G_1; LED_B_1;mdelay(1000);
			LED_R_1; LED_G_1; LED_B_1;mdelay(1000);
			LED_R_0; LED_G_0; LED_B_0;mdelay(1000);
		}else{
			if(arg & JZ_LED_RGB_RED_BIT)	 LED_R_1;else LED_R_0;
			if(arg & JZ_LED_RGB_GREEN_BIT)	 LED_G_1;else LED_G_0;
			if(arg & JZ_LED_RGB_BLUE_BIT)	 LED_B_1;else LED_B_0;
		}
	}
	return 0;
}

static ssize_t jz_led_write(struct file *file, const char __user *buf, size_t size, loff_t *loff)
{
	return 0;
}

static struct file_operations jz_led_fops =
{
	.owner  =   THIS_MODULE,
	.open   =   jz_led_open,
	.release =  jz_led_release,
	.write = jz_led_write,
	.unlocked_ioctl	= jz_led_ioctl,
};

static int jz_led_probe(struct platform_device *pdev)
{
	int res;
	printk("jz_led_RGB driver init\n");
	jz_led_RGB_pdata = (struct jz_led_RGB_pdata*)(pdev->dev.platform_data);
	if(!jz_led_RGB_pdata){
		printk("jz_led_RGB driver has no pdata.\n");
		return -1;
	}
	res = gpio_request(jz_led_RGB_pdata->gpio_RGB_R, "jz_ledRGBR");
	if(res < 0){
		printk("request gpio_RGB_R:%X failed\n", jz_led_RGB_pdata->gpio_RGB_R);
	}
	res = gpio_request(jz_led_RGB_pdata->gpio_RGB_G, "jz_ledRGBG");
	if(res < 0){
		printk("request gpio_RGB_G:%X failed\n", jz_led_RGB_pdata->gpio_RGB_G);
	}
	res = gpio_request(jz_led_RGB_pdata->gpio_RGB_B, "jz_ledRGBB");
	if(res < 0){
		printk("request gpio_RGB_B:%X failed\n", jz_led_RGB_pdata->gpio_RGB_B);
	}

	miscdev.minor = MISC_DYNAMIC_MINOR;
	miscdev.name = DEVICE_NAME;
	miscdev.fops = &jz_led_fops ;
	res = misc_register(&miscdev);
	if (res < 0){
		printk("jz_led_RGB can't register major number\n");
		goto register_major_number_failed;
	}
	printk("register jz_led_RGB Driver OK.\n");
	return 0;
register_major_number_failed:
	gpio_free(jz_led_RGB_pdata->gpio_RGB_B);
	gpio_free(jz_led_RGB_pdata->gpio_RGB_G);
	gpio_free(jz_led_RGB_pdata->gpio_RGB_R);
	return -1;
}

static struct platform_driver jz_led_probe_driver = {
	.driver = {
		.name   = DEVICE_NAME,
		.owner  = THIS_MODULE,
	},
	.probe          = jz_led_probe,
};

static int __init jz_led_init(void)
{
	return platform_driver_register(&jz_led_probe_driver);
}

static void __exit jz_led_exit(void)
{
	printk("JZ LED DRIVER MODULE EXIT\n");
}

module_init(jz_led_init);
module_exit(jz_led_exit);

MODULE_DESCRIPTION("JZ LED Driver");
MODULE_LICENSE("GPL");

