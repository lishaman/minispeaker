
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio.h>

//#define DEBUG

#ifdef DEBUG
#define wifi_printk(fmt, args...) printk(fmt, ## args)
#else
#define wifi_printk(fmt, args...)
#endif

#define WIFI_POWER_DEV_NAME "wifi_power"

#define GPIO_WIFI_POWER GPIO_PE(5)

#define WIFI_ON  1
#define WIFI_OFF 0

extern void RTL8188_wlan_power_on(void);
extern void RTL8188_wlan_power_off(void);

static dev_t dev;
static struct cdev wifi_cdev;
static struct class *wifi_class;
static int wifi_major_num;

static void wifi_power_on(void)
{
	RTL8188_wlan_power_on(); //usb wifi use this api to control wifi power on
}

static void wifi_power_off(void)
{
	RTL8188_wlan_power_off(); //usb wifi use this api to control wifi power off
}

static int wifi_power_open (struct inode *inode, struct file *file) 
{
	wifi_printk("=== enter %s ===\n", __FUNCTION__);
	return 0;
}

static long wifi_power_ioctl (struct file *file, unsigned int cmd, unsigned long arg) 
{
	wifi_printk("=== enter %s === cmd %d\n", __FUNCTION__, cmd);
	switch (cmd) {
		case WIFI_ON:
			wifi_power_on();
			break;

		case WIFI_OFF:
			wifi_power_off();
			break;
		default:
			printk("ioctl error cmd %d\n", cmd);
			break;
	}
	return 0;
}

static int wifi_power_release (struct inode *inode, struct file *file) 
{
	wifi_printk("=== enter %s ===\n", __FUNCTION__);
	return 0;
}

static const struct file_operations wifi_power_fops = {
	.owner           =    THIS_MODULE,
	.open            =    wifi_power_open,
	.unlocked_ioctl  =    wifi_power_ioctl,
	.release         =    wifi_power_release,
};

static int __init wifi_power_init(void)
{
	int ret;
	wifi_printk("=== enter %s ===\n", __FUNCTION__);

	ret = alloc_chrdev_region(&dev, 0, 1, WIFI_POWER_DEV_NAME);
	if (ret) { 
		printk("wifi_power alloc_chrdev_region failed %d\n", ret); 
		return ret;
	} else {
		wifi_major_num = MAJOR(dev);
	}

	memset(&wifi_cdev, 0, sizeof(wifi_cdev));

	cdev_init(&wifi_cdev, &wifi_power_fops);

	ret = cdev_add(&wifi_cdev, dev, 1);
	if (ret) {
		unregister_chrdev_region(dev, 1);
		printk("wifi_power cdev_add failed %d\n", ret);
		return ret;
	}

	wifi_class = class_create(THIS_MODULE, WIFI_POWER_DEV_NAME);
	device_create(wifi_class, NULL, MKDEV(wifi_major_num, MINOR(dev)), NULL, WIFI_POWER_DEV_NAME);

	wifi_printk("=== %s finish ===\n", __FUNCTION__);
	return 0;
}

static void __exit wifi_power_exit(void)
{
	wifi_printk("=== enter %s ===\n", __FUNCTION__);

	device_destroy(wifi_class, MKDEV(wifi_major_num, 0));
	class_destroy(wifi_class);

	cdev_del(&wifi_cdev);
	unregister_chrdev_region(dev, 1);

	wifi_printk("=== %s finish ===\n", __FUNCTION__);
}

module_init(wifi_power_init);
module_exit(wifi_power_exit);

MODULE_DESCRIPTION("wifi power control driver");
MODULE_LICENSE("GPL v2");

