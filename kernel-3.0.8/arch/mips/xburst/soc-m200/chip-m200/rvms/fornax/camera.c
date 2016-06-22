#include <mach/camera.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#if 0
#define CAMERA_RST			GPIO_PD(27)
#define CAMERA_PWDN_N		GPIO_PA(13) /* pin conflict with USB_ID */
#define CAMERA_MCLK			GPIO_PE(2) /* no use */
#else
#define CAMERA_RST			GPIO_PA(1)
#define CAMERA_PWDN_N		GPIO_PA(13) /* pin conflict with USB_ID */
#endif


#if defined(CONFIG_VIDEO_OVISP)

int temp = 1;
#if defined(CONFIG_VIDEO_OV9724)
static int ov9724_power(int onoff)
{
	int ret = 0;
	if(temp) {
		ret = gpio_request(CAMERA_PWDN_N, "CAMERA_PWDN_N");
		if(ret){
			printk("%s[%d] gpio_request failed, gpio=%d\n",__func__,__LINE__, CAMERA_PWDN_N);
			return ret;
		}
		ret = gpio_request(CAMERA_RST, "CAMERA_RST");
		if(ret){
			printk("%s[%d] gpio_request failed, gpio=%d\n",__func__,__LINE__, CAMERA_RST);
			return ret;
		}
		temp = 0;
	}
	if (onoff) { /* conflict with USB_ID pin */
		printk("##### power on######### %s\n", __func__);
		//gpio_direction_output(CAMERA_PWDN_N, 0);
		mdelay(10); /* this is necesary */
		//gpio_direction_output(CAMERA_PWDN_N, 1);
		;
	} else {
		//printk("##### power off ######### %s\n", __func__);
		//gpio_direction_output(CAMERA_PWDN_N, 0);
		gpio_direction_output(CAMERA_RST, 0);   /*PWM0 */
		;
	}

	return 0;
}

static int ov9724_reset(void)
{
	printk("############## %s\n", __func__);

#if 1
	/*reset*/
	gpio_direction_output(CAMERA_RST, 1);   /*PWM0 */
	mdelay(10);
	gpio_direction_output(CAMERA_RST, 0);   /*PWM0 */
	mdelay(10);
	gpio_direction_output(CAMERA_RST, 1);   /*PWM0 */
#endif
	return 0;
}

static struct i2c_board_info ov9724_board_info = {
	.type = "ov9724",
	.addr = 0x10,
};
#endif /* CONFIG_VIDEO_OV9724 */
#if defined(CONFIG_VIDEO_OV5645)
static int ov5645_power(int onoff)
{
	int ret = 0;
	if(temp) {
		ret = gpio_request(CAMERA_PWDN_N, "CAMERA_PWDN_N");
		if(ret){
			printk("%s[%d] gpio_request failed, gpio=%d\n",__func__,__LINE__, CAMERA_PWDN_N);
			return ret;
		}
		ret = gpio_request(CAMERA_RST, "CAMERA_RST");
		if(ret){
			printk("%s[%d] gpio_request failed, gpio=%d\n",__func__,__LINE__, CAMERA_RST);
			return ret;
		}
		temp = 0;
	}
	if (onoff) {
		printk("[board camera]:%s, (%d)power on\n", __func__, CAMERA_PWDN_N);
		gpio_direction_output(CAMERA_PWDN_N, 0);   /*PWM0 */
		mdelay(10);
//		gpio_direction_output(CAMERA_RST, 1);   /*PWM0 */
		gpio_direction_output(CAMERA_PWDN_N, 1);   /*PWM0 */
		;
	} else {
		printk("[board camera]:%s, power off\n", __func__);
		//gpio_direction_output(CAMERA_PWDN_N, 0);
		gpio_direction_output(CAMERA_RST, 0);   /*PWM0 */
		;
	}

	return 0;
}

static int ov5645_reset(void)
{
	printk("[board camera]:%s\n", __func__);

#if 1
	/*reset*/
	gpio_direction_output(CAMERA_RST, 1);   /*PWM0 */
	mdelay(10);
	gpio_direction_output(CAMERA_RST, 0);   /*PWM0 */
	mdelay(10);
	gpio_direction_output(CAMERA_RST, 1);   /*PWM0 */
#endif
	return 0;
}

static struct i2c_board_info ov5645_board_info = {
	.type = "ov5645",
	.addr = 0x3c,
};
#endif /* CONFIG_VIDEO_OV5645 */

#if defined(CONFIG_VIDEO_OV8858)
static int ov8858_power(int onoff)
{
	int ret = 0;
	if(temp) {
	//gpio_request(CAMERA_PWDN_N, "CAMERA_PWDN_N");
	ret = gpio_request(CAMERA_RST, "CAMERA_RST");
		if(ret){
			printk("%s[%d] gpio_request failed, gpio=%d\n",__func__,__LINE__, CAMERA_RST);
			return ret;
		}
		temp = 0;
	}
	if (onoff) {
	        printk("##### power on######### %s\n", __func__);
	//	gpio_direction_output(CAMERA_PWDN_N, 0);
		mdelay(10);
	//	gpio_direction_output(CAMERA_PWDN_N, 1);
		;
	} else {
		printk("##### power off ######### %s\n", __func__);
	//	gpio_direction_output(CAMERA_PWDN_N, 0);
		gpio_direction_output(CAMERA_RST, 0);   /*PWM0 */
		;
	}
	return 0;
}

static int ov8858_reset(void)
{
	printk("#######  %s  #######\n", __func__);

	/*reset*/
	gpio_direction_output(CAMERA_RST, 1);   /*PWM0 */
	mdelay(10);
	gpio_direction_output(CAMERA_RST, 0);   /*PWM0 */
	mdelay(10);
	gpio_direction_output(CAMERA_RST, 1);   /*PWM0 */
	mdelay(1000);

	return 0;
}

static struct i2c_board_info ov8858_board_info = {
	.type = "ov8858",
	.addr = 0x10,
	/* .addr = 0x36, */
};
#endif /* CONFIG_VIDEO_OV8858 */

#if defined(CONFIG_DVP_OV9712)
/* OV9712 PIN */
//#define OV9712_POWER	 	GPIO_PC(2) //the power of camera board
#define OV9712_RST		GPIO_PA(1)
#define OV9712_PWDN_EN		GPIO_PA(13)
static int ov9712_power(int onoff)
{
	int ret = 0;
	if(temp) {
		//	gpio_request(OV9712_POWER, "OV9712_POWER");
		ret = gpio_request(OV9712_PWDN_EN, "OV9712_PWDN_EN");
		if(ret){
			printk("%s[%d] gpio_request failed, gpio=%d\n",__func__,__LINE__, OV9712_PWDN_EN);
			return ret;
		}
		ret = gpio_request(OV9712_RST, "OV9712_RST");
		if(ret){
			printk("%s[%d] gpio_request failed, gpio=%d\n",__func__,__LINE__, OV9712_RST);
			return ret;
		}
		temp = 0;
	}
	if (onoff) { /* conflict with USB_ID pin */
		printk("##### power on######### %s\n", __func__);
		msleep(5); /* this is necesary */
		gpio_direction_output(OV9712_PWDN_EN, 1);
//		gpio_direction_output(OV9712_RST, 1);
//		mdelay(1); /* this is necesary */
//		gpio_direction_output(OV9712_POWER, 1);
		msleep(5); /* this is necesary */
		gpio_direction_output(OV9712_PWDN_EN, 0);
		msleep(5); /* this is necesary */
		gpio_direction_output(OV9712_RST, 0);
		msleep(1); /* this is necesary */
		gpio_direction_output(OV9712_RST, 1);
		msleep(20); /* this is necesary */
	} else {
		//printk("##### power off ######### %s\n", __func__);
		//gpio_direction_output(OV9712_PWDN_EN, 1);
		//gpio_direction_output(OV9712_POWER, 0);
	}

	return 0;
}

static int ov9712_reset(void)
{
	printk("############## %s\n", __func__);

#if 1
	/*reset*/
	gpio_direction_output(OV9712_RST, 0);	/*PWM0*/
	mdelay(150);
	gpio_direction_output(OV9712_RST, 1);	/*PWM0*/
	mdelay(150);
#endif
	return 0;
}

static struct i2c_board_info ov9712_board_info = {
	.type = "ov9712",
	.addr = 0x30,
};
#endif /* CONFIG_DVP_OV9712 */

#ifdef CONFIG_DVP_BF3116
/* BF3116 PIN */
#define BF3116_RST		GPIO_PA(1)
#define BF3116_PWDN_EN		GPIO_PA(13)
static int bf3116_power(int onoff)
{
#if 1
	int ret = 0;
	if(temp) {
		//	gpio_request(BF3116_POWER, "BF3116_POWER");
		ret = gpio_request(BF3116_PWDN_EN, "BF3116_PWDN_EN");
		if(ret){
			printk("%s[%d] gpio_request failed, gpio=%d\n",__func__,__LINE__, BF3116_PWDN_EN);
			return ret;
		}
		ret = gpio_request(BF3116_RST, "BF3116_RST");
		if(ret){
			printk("%s[%d] gpio_request failed, gpio=%d\n",__func__,__LINE__, BF3116_RST);
			return ret;
		}
		temp = 0;
	}
	if (onoff) { /* conflict with USB_ID pin */
		printk("##### power on######### %s\n", __func__);
		msleep(5); /* this is necesary */
		gpio_direction_output(BF3116_PWDN_EN, 1);
		msleep(5); /* this is necesary */
		gpio_direction_output(BF3116_PWDN_EN, 0);
		msleep(5); /* this is necesary */
		gpio_direction_output(BF3116_RST, 0);
		msleep(1); /* this is necesary */
		gpio_direction_output(BF3116_RST, 1);
		msleep(20); /* this is necesary */
	} else {
		//printk("##### power off ######### %s\n", __func__);
		//gpio_direction_output(BF3116_PWDN_EN, 1);
		//gpio_direction_output(BF3116_POWER, 0);
	}
#endif
	return 0;
}

static int bf3116_reset(void)
{
	printk("############## %s\n", __func__);

#if 1
	/*reset*/
	gpio_direction_output(BF3116_RST, 0);	/*PWM0*/
	mdelay(150);
	gpio_direction_output(BF3116_RST, 1);	/*PWM0*/
	mdelay(150);
#endif
	return 0;
}

static struct i2c_board_info bf3116_board_info = {
	.type = "bf3116",
	.addr = 0x6e,
};
#endif /* CONFIG_DVP_BF3116 */

static struct ovisp_camera_client ovisp_camera_clients[] = {
#if defined(CONFIG_VIDEO_OV5645)
	{
		.board_info = &ov5645_board_info,
		.flags = CAMERA_CLIENT_IF_MIPI,
		.mclk_rate = 24000000,
		.max_video_width = 1280,
		.max_video_height = 720,
		.power = ov5645_power,
		.reset = ov5645_reset,
	},
#endif /* CONFIG_VIDEO_OV5645 */

#if defined(CONFIG_VIDEO_OV9724)
	{
		.board_info = &ov9724_board_info,
#if 0
		.flags = CAMERA_CLIENT_IF_MIPI
				| CAMERA_CLIENT_CLK_EXT
				| CAMERA_CLIENT_ISP_BYPASS,
#else
		.flags = CAMERA_CLIENT_IF_MIPI,
#endif
		.mclk_rate = 26000000,
		//.max_video_width = 1280,
		//.max_video_height = 960,
		.max_video_width = 1280,
		.max_video_height = 720,
		.power = ov9724_power,
		.reset = ov9724_reset,
	},
#endif /* CONFIG_VIDEO_OV9724 */

#if defined(CONFIG_VIDEO_OV8858)
	{
		.board_info = &ov8858_board_info,
#if 0
		.flags = CAMERA_CLIENT_IF_MIPI
				| CAMERA_CLIENT_CLK_EXT
				| CAMERA_CLIENT_ISP_BYPASS,
#else
		.flags = CAMERA_CLIENT_IF_MIPI,
#endif
		.mclk_rate = 26000000,
		.max_video_width = 1632,
		.max_video_height = 1224,
		.power = ov8858_power,
		.reset = ov8858_reset,
	},
#endif /* CONFIG_VIDEO_OV8858 */

#if defined(CONFIG_DVP_BF3116)
	{
		.board_info = &bf3116_board_info,
#if 0
		.flags = CAMERA_CLIENT_IF_DVP
		| CAMERA_CLIENT_CLK_EXT
		| CAMERA_CLIENT_ISP_BYPASS,
#else
		.flags = CAMERA_CLIENT_IF_DVP | CAMERA_CLIENT_CLK_EXT,
//		.flags = CAMERA_CLIENT_IF_DVP,
#endif
		.mclk_name = "cgu_cim",
		.mclk_rate = 27000000,
		.max_video_width = 1280,
		.max_video_height = 720,
		.power = bf3116_power,
		.reset = bf3116_reset,
	},
#endif /* CONFIG_DVP_BF3116 */

#if defined(CONFIG_DVP_OV9712)
	{
		.board_info = &ov9712_board_info,
#if 0
		.flags = CAMERA_CLIENT_IF_DVP
				| CAMERA_CLIENT_CLK_EXT
				| CAMERA_CLIENT_ISP_BYPASS,
#else
		.flags = CAMERA_CLIENT_IF_DVP | CAMERA_CLIENT_CLK_EXT,
//		.flags = CAMERA_CLIENT_IF_DVP,
#endif
		.mclk_name = "cgu_cim",
		.mclk_rate = 24000000,
		.max_video_width = 1280,
		.max_video_height = 800,
		.power = ov9712_power,
		.reset = ov9712_reset,
	},
#endif /* CONFIG_DVP_OV9712 */
};

struct ovisp_camera_platform_data ovisp_camera_info = {
#ifdef CONFIG_OVISP_I2C
	.i2c_adapter_id = 4, /* larger than host i2c nums */
	.flags = CAMERA_USE_ISP_I2C | CAMERA_USE_HIGH_BYTE
			| CAMERA_I2C_PIO_MODE | CAMERA_I2C_STANDARD_SPEED,
#else
	.i2c_adapter_id = 3, /* use cpu's i2c adapter */
	.flags = CAMERA_USE_HIGH_BYTE
			| CAMERA_I2C_PIO_MODE | CAMERA_I2C_STANDARD_SPEED,
#endif
	.client = ovisp_camera_clients,
	.client_num = ARRAY_SIZE(ovisp_camera_clients),
};
#endif /* CONFIG_VIDEO_OVISP */
