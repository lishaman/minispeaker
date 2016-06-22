#include <mach/camera.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>

#define CAMERA_RST			GPIO_PD(27)
#define CAMERA_PWDN_N		GPIO_PA(13) /* pin conflict with USB_ID */
#define CAMERA_MCLK			GPIO_PE(2) /* no use */



#if defined(CONFIG_VIDEO_OVISP)

int temp = 1;
/******************************** ov9724 start ************************************************/
#if defined(CONFIG_VIDEO_OV9724)
static int ov9724_power(int onoff)
{
	if(temp) {
	gpio_request(CAMERA_PWDN_N, "CAMERA_PWDN_N");
	gpio_request(CAMERA_RST, "CAMERA_RST");
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
/******************************** ov9724 end   ************************************************/
/******************************** ov5645 start ************************************************/
#if defined(CONFIG_VIDEO_OV5645)
static int ov5645_power(int onoff)
{
	if(temp) {
//		gpio_request(CAMERA_PWDN_N, "CAMERA_PWDN_N");
		gpio_request(CAMERA_RST, "CAMERA_RST");
		temp = 0;
	}
	if (onoff) {
		printk("[board camera]:%s, power on\n", __func__);
		mdelay(10);
		//gpio_direction_output(CAMERA_RST, 1);   /*PWM0 */
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
/******************************** ov5645 end   ************************************************/
/******************************** ov8858 start ************************************************/
#if defined(CONFIG_VIDEO_OV8858)
static int ov8858_power(int onoff)
{
	if(temp) {
	//gpio_request(CAMERA_PWDN_N, "CAMERA_PWDN_N");
	gpio_request(CAMERA_RST, "CAMERA_RST");
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
/******************************** ov8858 end   ************************************************/
/******************************** aw6120 start ************************************************/

#if defined(CONFIG_VIDEO_AW6120)
#define AW6120_RST		GPIO_PF(3)
#define AW6120_PWDN_EN		GPIO_PF(7)
static struct regulator *regulator_aw6120 = NULL;
static int aw6120_power(int onoff)
{
	if(temp) {
		gpio_request(AW6120_PWDN_EN, "AW6120_PWDN_EN");
		gpio_request(AW6120_RST, "AW6120_RST");
		regulator_aw6120 = regulator_get(NULL, "cam_avdd_2v8");
		if (IS_ERR(regulator_aw6120)) {
			printk("failed to get regulator aw6120\n");
			return -1;
		}
		temp = 0;
	}
	if (onoff) {
		if(!regulator_is_enabled(regulator_aw6120))
			regulator_enable(regulator_aw6120);
		gpio_direction_output(AW6120_PWDN_EN, 1);
		gpio_direction_output(AW6120_RST, 0);
		mdelay(10);
		gpio_direction_output(AW6120_PWDN_EN, 0);
		gpio_direction_output(AW6120_RST, 1);
		mdelay(10);
	} else {
		gpio_direction_output(AW6120_RST, 0);
		mdelay(10);
		if(regulator_is_enabled(regulator_aw6120))
			regulator_disable(regulator_aw6120);
	}
	return 0;
}

static int aw6120_reset(void)
{
	/*reset*/
	gpio_direction_output(AW6120_RST, 0);
	mdelay(10);
	gpio_direction_output(AW6120_RST, 1);
	mdelay(10);

	return 0;
}

static struct i2c_board_info aw6120_board_info = {
	.type = "aw6120",
	.addr = 0x1b,
};
#endif /* CONFIG_VIDEO_AW6120 */
/******************************** aw6120 end   ************************************************/

/******************************** ov9712 start ************************************************/
#if defined(CONFIG_DVP_OV9712)
/* OV9712 PIN */
#define OV9712_POWER	 	GPIO_PC(2) //the power of camera board
#define OV9712_RST		GPIO_PA(11)
#define OV9712_PWDN_EN		GPIO_PD(28)
static int ov9712_power(int onoff)
{
	if(temp) {
	gpio_request(OV9712_POWER, "OV9712_POWER");
	gpio_request(OV9712_PWDN_EN, "OV9712_PWDN_EN");
	gpio_request(OV9712_RST, "OV9712_RST");
	temp = 0;
	}
	if (onoff) { /* conflict with USB_ID pin */
		printk("##### power on######### %s\n", __func__);
		gpio_direction_output(OV9712_PWDN_EN, 1);
//		gpio_direction_output(OV9712_RST, 1);
		mdelay(1); /* this is necesary */
		gpio_direction_output(OV9712_POWER, 1);
		mdelay(100); /* this is necesary */
		gpio_direction_output(OV9712_PWDN_EN, 0);
		mdelay(50); /* this is necesary */
		gpio_direction_output(OV9712_RST, 0);
		mdelay(150); /* this is necesary */
		gpio_direction_output(OV9712_RST, 1);
		mdelay(150); /* this is necesary */
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
/******************************** ov9712 end   ************************************************/


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

#if defined(CONFIG_VIDEO_AW6120)
	{
		.board_info = &aw6120_board_info,
#if 1
		.flags = CAMERA_CLIENT_IF_MIPI
				| CAMERA_CLIENT_CLK_EXT
				| CAMERA_CLIENT_ISP_BYPASS,
#else
		.flags = CAMERA_CLIENT_IF_MIPI,
#endif
		.mclk_rate = 24000000,
		.power = aw6120_power,
		.reset = aw6120_reset,
	},
#endif /* CONFIG_VIDEO_AW6120 */

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
