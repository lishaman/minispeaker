/*
 * Root hub test FIXME we now just support TEST_PACKET
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/usb/hcd.h>

static char *get_usb_test_mode(int test_mode)
{
	switch (test_mode) {
	case TEST_SE0_NAK:	return "TEST_SE0_NAK";
	case TEST_J:		return "TEST_J";
	case TEST_K:		return "TEST_K";
	case TEST_PACKET:	return "TEST_PACKET";
	case TEST_FORCE_EN:	return "TEST_FORCE_ENABLE";
	default:		break;
	}
	return "Unkown test mode";
}

static int usage_print(void)
{
	int i;
	pr_info("Usage:\t\n");
	pr_info("\techo  x > usb_mode (x=0~8)\n");
	for (i = 0; i < 9; i++)
		pr_info("\t\tx = %d (%s)\n", i , get_usb_test_mode(i));
	return 0;
}

int usb_start_rh_test_mode(struct usb_device *dev, int test_mode) {
	struct usb_device *hdev = NULL;
	int test_timeout = HZ;

	if (!dev || !dev->bus || !dev->bus->root_hub) {
		dev_warn(&dev->dev, "Need connect device to hub\n");
		return -ENODEV;
	} else {
		hdev = dev->bus->root_hub;
	}

	if (!dev->parent && dev->parent != hdev) {
		dev_warn(&dev->dev, "Now we just support 'Root Hub' usb test \n");
		return -EPERM;
	}

	/* HSOTG Electrical Test */
	if (test_mode > 5 || test_mode < 0)
		test_mode = TEST_PACKET;

	if (test_mode) {
		pr_info("dev->portnum = %d\n, test_mode = %d\n",dev->portnum, test_mode);
		dev_warn(&dev->dev, "%s\n", get_usb_test_mode(test_mode));

		if (test_mode == TEST_PACKET) {
			/*when we test rh we found dwc otg test is ok
			 * but echi test, the device must be in test mode too
			 */
			pr_info("Make device in test mode\n");
			usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
					USB_REQ_SET_FEATURE, 0,
					USB_DEVICE_TEST_MODE,(test_mode << 8),
					NULL, 0, USB_CTRL_SET_TIMEOUT);
			pr_info("open the port %d power\n", dev->portnum);
			usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
					USB_REQ_SET_FEATURE, USB_RT_PORT,
					USB_PORT_FEAT_POWER, dev->portnum, NULL, 0, HZ);
			pr_info("Make device in suspend mode\n");
			usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
					USB_REQ_SET_FEATURE, USB_RT_PORT,
					USB_PORT_FEAT_SUSPEND, dev->portnum , NULL, 0, HZ);
		}

		pr_info("Make host in test mode\n");
		usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
				USB_REQ_SET_FEATURE, USB_RT_PORT,
				USB_PORT_FEAT_TEST, ((test_mode << 8) | dev->portnum),
				NULL, 0, test_timeout);
	}

	return 0;
}

static ssize_t usb_host_test_mode_store(struct device *dev,
						 struct device_attribute *attr,
						 const char *buf, size_t size)
{
	struct usb_device *rh_usb_dev = to_usb_device(dev);
	struct usb_device *childdev = NULL;
	int chix = 0;
	int test_mode;
	int ret = size;

	if (sscanf(buf, "%d\n", &test_mode) != 1)
		goto error_arg;
	for (chix = 0; chix < rh_usb_dev->maxchild; chix++) {
		childdev = rh_usb_dev->children[chix];
		if (childdev) {
			ret = usb_start_rh_test_mode(childdev, test_mode);
			if (ret) return ret;
			break;
		}
	}
	return size;
error_arg:
	usage_print();
	return ret;
}

static DEVICE_ATTR(test_mode , S_IWUSR ,
		NULL, usb_host_test_mode_store);
