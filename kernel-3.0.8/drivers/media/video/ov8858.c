#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
//#include <media/ov8858.h>
#include <mach/ovisp-v4l2.h>
#include "ov8858.h"


#define OV8858_CHIP_ID_H	(0x88)
#define OV8858_CHIP_ID_L	(0x58)

/* #define SXGA_WIDTH		1280 */
/* #define SXGA_HEIGHT		960 */
#define SXGA_WIDTH		1632
#define SXGA_HEIGHT		1224
#define VGA_WIDTH		640
#define VGA_HEIGHT		480
#define MAX_WIDTH		3264
#define MAX_HEIGHT		2448
//#define MAX_WIDTH		2592
//#define MAX_HEIGHT		1944

#define OV8858_REG_END		0xffff
#define OV8858_REG_DELAY	0xfffe

//#define OV8858_YUV

struct ov8858_format_struct;
struct ov8858_info {
	struct v4l2_subdev sd;
	struct ov8858_format_struct *fmt;
	struct ov8858_win_setting *win;
};

struct regval_list {
	unsigned short reg_num;
	unsigned char value;
};

static struct regval_list ov8858_init_3264_2448_raw8_regs[] = {


	/***************** init reg set **************************/
    {0x0103, 0x01},

    /* {0x303f, 0x01}, *///i2c address
    /* {0x3012, 0x6c}, */

    {0x0100, 0x00},
    {0x0100, 0x00},
    {0x0100, 0x00},
    {0x0100, 0x00},

    {0x0302, 0x1e},
    {0x0303, 0x00},
    {0x0304, 0x03},
    {0x030e, 0x02},
    {0x030f, 0x04},
    {0x0312, 0x03},
    {0x031e, 0x0c},

    {0x3600, 0x00},
    {0x3601, 0x00},
    {0x3602, 0x00},
    {0x3603, 0x00},
    {0x3604, 0x22},
    {0x3605, 0x30},
    {0x3606, 0x00},
    {0x3607, 0x20},
    {0x3608, 0x11},
    {0x3609, 0x28},
    {0x360a, 0x00},
    {0x360b, 0x06},
    {0x360c, 0xdc},
    {0x360d, 0x40},
    {0x360e, 0x0c},
    {0x360f, 0x20},
    {0x3610, 0x07},
    {0x3611, 0x20},
    {0x3612, 0x88},
    {0x3613, 0x80},
    {0x3614, 0x58},
    {0x3615, 0x00},
    {0x3616, 0x4a},
    {0x3617, 0x90},
    {0x3618, 0x56},
    {0x3619, 0x70},
    {0x361a, 0x99},
    {0x361b, 0x00},
    {0x361c, 0x07},
    {0x361d, 0x00},
    {0x361e, 0x00},
    {0x361f, 0x00},
    {0x3638, 0xff},
    {0x3633, 0x0c},
    {0x3634, 0x0c},
    {0x3635, 0x0c},
    {0x3636, 0x0c},
    {0x3645, 0x13},
    {0x3646, 0x83},
    {0x364a, 0x07},

    {0x3015, 0x00},
    {0x3018, 0x32},//lane
    {0x3020, 0x93},
    {0x3022, 0x01},
    {0x3031, 0x08},
    {0x3034, 0x00},
    {0x3106, 0x01},
    {0x3305, 0xf1},
    {0x3308, 0x00},
    {0x3309, 0x28},
    {0x330a, 0x00},
    {0x330b, 0x20},
    {0x330c, 0x00},
    {0x330d, 0x00},
    {0x330e, 0x00},
    {0x330f, 0x40},
    {0x3307, 0x04},
    {0x3500, 0x03},
    {0x3501, 0x9a},
    {0x3502, 0x20},
    {0x3503, 0x00},
    {0x3505, 0x80},
    {0x3507, 0x03},//add gain shift
    {0x3508, 0x02},
    {0x3509, 0x00},
    {0x350c, 0x00},
    {0x350d, 0x80},
    {0x3510, 0x00},
    {0x3511, 0x02},
    {0x3512, 0x00},
    {0x3700, 0x18},
    {0x3701, 0x0c},
    {0x3702, 0x28},
    {0x3703, 0x19},
    {0x3704, 0x14},
    {0x3705, 0x00},
    {0x3706, 0x6a},
    {0x3707, 0x04},
    {0x3708, 0x24},
    {0x3709, 0x33},
    {0x370a, 0x01},
    {0x370b, 0x6a},
    {0x370c, 0x04},
    {0x3718, 0x12},
    {0x3719, 0x31},
    {0x3712, 0x42},
    {0x3714, 0x24},
    {0x371e, 0x19},
    {0x371f, 0x40},
    {0x3720, 0x05},
    {0x3721, 0x05},
    {0x3724, 0x06},
    {0x3725, 0x01},
    {0x3726, 0x06},
    {0x3728, 0x05},
    {0x3729, 0x02},
    {0x372a, 0x03},
    {0x372b, 0x53},
    {0x372c, 0xa3},
    {0x372d, 0x53},
    {0x372e, 0x06},
    {0x372f, 0x10},
    {0x3730, 0x01},
    {0x3731, 0x06},
    {0x3732, 0x14},
    {0x3733, 0x10},
    {0x3734, 0x40},
    {0x3736, 0x20},
    {0x373a, 0x05},
    {0x373b, 0x06},
    {0x373c, 0x0a},
    {0x373e, 0x03},
    {0x3755, 0x10},
    {0x3758, 0x00},
    {0x3759, 0x4c},
    {0x375a, 0x06},
    {0x375b, 0x13},
    {0x375c, 0x20},
    {0x375d, 0x02},
    {0x375e, 0x00},
    {0x375f, 0x14},
    {0x3768, 0x22},
    {0x3769, 0x44},
    {0x376a, 0x44},
    {0x3761, 0x00},
    {0x3762, 0x00},
    {0x3763, 0x00},
    {0x3766, 0xff},
    {0x376b, 0x00},
    {0x3772, 0x23},
    {0x3773, 0x02},
    {0x3774, 0x16},
    {0x3775, 0x12},
    {0x3776, 0x04},
    {0x3777, 0x00},
    {0x3778, 0x32},
    {0x37a0, 0x44},
    {0x37a1, 0x3d},
    {0x37a2, 0x3d},
    {0x37a3, 0x00},
    {0x37a4, 0x00},
    {0x37a5, 0x00},
    {0x37a6, 0x00},
    {0x37a7, 0x44},
    {0x37a8, 0x4c},
    {0x37a9, 0x4c},
    {0x3760, 0x00},
    {0x376f, 0x01},
    {0x37aa, 0x44},
    {0x37ab, 0x2e},
    {0x37ac, 0x2e},
    {0x37ad, 0x33},
    {0x37ae, 0x0d},
    {0x37af, 0x0d},
    {0x37b0, 0x00},
    {0x37b1, 0x00},
    {0x37b2, 0x00},
    {0x37b3, 0x42},
    {0x37b4, 0x42},
    {0x37b5, 0x33},
    {0x37b6, 0x00},
    {0x37b7, 0x00},
    {0x37b8, 0x00},
    {0x37b9, 0xff},
    {0x3800, 0x00},
    {0x3801, 0x0c},
    {0x3802, 0x00},
    {0x3803, 0x0c},
    {0x3804, 0x0c},
    {0x3805, 0xd3},
    {0x3806, 0x09},
    {0x3807, 0xa3},
    {0x3808, 0x0c},//output width
    {0x3809, 0xc0},
    {0x380a, 0x09},//output height
    {0x380b, 0x90},
    {0x380c, 0x07},
    {0x380d, 0x94},
    {0x380e, 0x0a},
    {0x380f, 0xba},
    {0x3810, 0x00},
    {0x3811, 0x04},
    {0x3813, 0x02},
    {0x3814, 0x01},
    {0x3815, 0x01},
    {0x3820, 0x00},
    {0x3821, 0x46},
    {0x382a, 0x01},
    {0x382b, 0x01},
    {0x3830, 0x06},
    {0x3836, 0x01},
    {0x3837, 0x18},
    {0x3841, 0xff},
    {0x3846, 0x48},
    {0x3d85, 0x14},
    {0x3f08, 0x08},
    /* {0x3f0a, 0x80}, */
    {0x4000, 0xf1},
    {0x4001, 0x00},
    {0x4005, 0x10},
    {0x4002, 0x27},
    /* {0x4006, 0x04}, */
    /* {0x4007, 0x04}, */
    {0x4009, 0x81},
    {0x400b, 0x0c},
    {0x401b, 0x00},
    {0x401d, 0x00},
    {0x4020, 0x00},
    {0x4021, 0x04},
    {0x4022, 0x0b},
    {0x4023, 0xc3},
    {0x4024, 0x0c},//BLC control 背光补偿
    {0x4025, 0x36},
    {0x4026, 0x0c},
    {0x4027, 0x37},
    {0x4028, 0x00},
    {0x4029, 0x02},
    {0x402a, 0x04},
    {0x402b, 0x08},
    {0x402c, 0x02},
    {0x402d, 0x02},
    {0x402e, 0x0c},
    {0x402f, 0x02},
    {0x401f, 0x00},
    {0x4034, 0x3f},
    {0x403d, 0x04},
    {0x4300, 0xff},
    {0x4301, 0x00},
    {0x4302, 0x0f},
    {0x4316, 0x00},
    /* {0x4320, 0x90},//pixel order */

    /* {0x4500, 0x38}, */
    {0x4503, 0x18},
    {0x4600, 0x01},
    {0x4601, 0x97},
    {0x481f, 0x32},
    {0x4837, 0x16},//PCLK PERIOD
    {0x4850, 0x10},//lane sel
    {0x4851, 0x32},
    {0x4b00, 0x2a},
    {0x4b0d, 0x00},
    {0x4d00, 0x04},
    {0x4d01, 0x18},
    {0x4d02, 0xc3},
    {0x4d03, 0xff},
    {0x4d04, 0xff},
    {0x4d05, 0xff},
    {0x5000, 0x7e},//isp control image sensor prossesor
    {0x5001, 0x01},
    {0x5002, 0x08},
    {0x5003, 0x20},
    {0x5046, 0x12},
    /* {0x5780, 0xfc}, */
    /* {0x5784, 0x0c}, */
    /* {0x5787, 0x40}, */
    /* {0x5788, 0x08}, */
    /* {0x578a, 0x02}, */
    /* {0x578b, 0x01}, */
    /* {0x578c, 0x01}, */
    /* {0x578e, 0x02}, */
    /* {0x578f, 0x01}, */
    /* {0x5790, 0x01}, */
    {0x5901, 0x00},
    /* {0x5b00, 0x02}, */
    /* {0x5b01, 0x10}, */
    /* {0x5b02, 0x03}, */
    /* {0x5b03, 0xcf}, */
    /* {0x5b05, 0x6c}, */
    {0x5e00, 0x00},//test mode
    {0x5e01, 0x41},
    /* {0x382d, 0x7f}, */
    {0x4825, 0x3a},
    {0x4826, 0x40},
    {0x4808, 0x25},
    /* {0x0100, 0x01}, */

	{OV8858_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov8858_stream_on[] = {
	/* {0x4201, 0x00}, */
	{0x4202, 0x00},
	{0x0100, 0x01},

	{OV8858_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov8858_stream_off[] = {
	/* Sensor enter LP11*/
	/* {0x4201, 0x00}, */
	{0x4202, 0x0f},
	{0x0100, 0x00},

	{OV8858_REG_END, 0x00},	/* END MARKER */
};

int ov8858_read(struct v4l2_subdev *sd, unsigned short reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[2] = {reg >> 8, reg & 0xff};
	struct i2c_msg msg[2] = {
		[0] = {
			.addr	= client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= buf,
		},
		[1] = {
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= value,
		}
	};
	int ret;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;

	return ret;
}


static int ov8858_write(struct v4l2_subdev *sd, unsigned short reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 3,
		.buf	= buf,
	};
	int ret;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}


static int ov8858_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != OV8858_REG_END) {
		if (vals->reg_num == OV8858_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = ov8858_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		printk("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, val);
		//mdelay(200);
		vals++;
	}
	//printk("vals->reg_num:%x, vals->value:%x\n", vals->reg_num, vals->value);
	ov8858_write(sd, vals->reg_num, vals->value);
	return 0;
}
static int ov8858_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != OV8858_REG_END) {
		if (vals->reg_num == OV8858_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = ov8858_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		//printk("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
		//mdelay(200);
		vals++;
	}
	//printk("vals->reg_num:%x, vals->value:%x\n", vals->reg_num, vals->value);
	//ov8858_write(sd, vals->reg_num, vals->value);
	return 0;
}

/* R/G and B/G of typical camera module is defined here. */
static unsigned int rg_ratio_typical = 0x58;
static unsigned int bg_ratio_typical = 0x5a;

/*
	R_gain, sensor red gain of AWB, 0x400 =1
	G_gain, sensor green gain of AWB, 0x400 =1
	B_gain, sensor blue gain of AWB, 0x400 =1
	return 0;
*/
static int ov8858_update_awb_gain(struct v4l2_subdev *sd,
				unsigned int R_gain, unsigned int G_gain, unsigned int B_gain)
{
#if 0
	printk("[ov8858] problem function:%s, line:%d\n", __func__, __LINE__);
	printk("R_gain:%04x, G_gain:%04x, B_gain:%04x \n ", R_gain, G_gain, B_gain);
	if (R_gain > 0x400) {
		ov8858_write(sd, 0x5186, (unsigned char)(R_gain >> 8));
		ov8858_write(sd, 0x5187, (unsigned char)(R_gain & 0x00ff));
	}

	if (G_gain > 0x400) {
		ov8858_write(sd, 0x5188, (unsigned char)(G_gain >> 8));
		ov8858_write(sd, 0x5189, (unsigned char)(G_gain & 0x00ff));
	}

	if (B_gain > 0x400) {
		ov8858_write(sd, 0x518a, (unsigned char)(B_gain >> 8));
		ov8858_write(sd, 0x518b, (unsigned char)(B_gain & 0x00ff));
	}
#endif

	return 0;
}

static int ov8858_reset(struct v4l2_subdev *sd, u32 val)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov8858_detect(struct v4l2_subdev *sd);
static int ov8858_init(struct v4l2_subdev *sd, u32 val)
{
	struct ov8858_info *info = container_of(sd, struct ov8858_info, sd);
	printk("functiong:%s, line:%d\n", __func__, __LINE__);

	info->win = NULL;
	return 0;
}

static int ov8858_get_sensor_vts(struct v4l2_subdev *sd, unsigned short *value)
{
	unsigned char h,l;
	int ret = 0;
	ret = ov8858_read(sd, 0x380e, &h);
	if (ret < 0)
		return ret;
	ret = ov8858_read(sd, 0x380f, &l);
	if (ret < 0)
		return ret;
	*value = h;
	*value = (*value << 8) | l;
	return ret;
}

static int ov8858_get_sensor_lans(struct v4l2_subdev *sd, unsigned char *value)
{
	int ret = 0;
	unsigned char v = 0;
	ret = ov8858_read(sd, 0x3018, &v);
	if (ret < 0)
		return ret;
	*value = (v >> 5)+1 ;
	if(*value > 2 || *value < 1)
		ret = -EINVAL;
	return ret;
}

static int ov8858_detect(struct v4l2_subdev *sd)
{
	unsigned char v;
	int ret;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	ret = ov8858_read(sd, 0x300b, &v);
	printk("-----%s: %d ret = %d\n", __func__, __LINE__, ret);
	if (ret < 0)
		return ret;
	printk("-----%s: %d v = %08X\n", __func__, __LINE__, v);
	if (v != OV8858_CHIP_ID_H)
		return -ENODEV;
	ret = ov8858_read(sd, 0x300c, &v);
	printk("-----%s: %d ret = %d\n", __func__, __LINE__, ret);
	if (ret < 0)
		return ret;
	printk("-----%s: %d v = %08X\n", __func__, __LINE__, v);
	if (v != OV8858_CHIP_ID_L)
		return -ENODEV;
	ret = ov8858_read(sd, 0x302a, &v);
	printk("-----%s: %d ret = %d\n", __func__, __LINE__, ret);
	if (ret < 0)
		return ret;
	printk("-----%s: %d reg 302a = %08X\n", __func__, __LINE__, v);

	return 0;
}

static struct ov8858_format_struct {
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
} ov8858_formats[] = {
	{
		/*RAW8 FORMAT, 8 bit per pixel*/
		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,
		.colorspace	= V4L2_COLORSPACE_SRGB,
	},
	{
		.mbus_code = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace	 = V4L2_COLORSPACE_BT878,/*don't know*/

	}
	/*add other format supported*/
};
#define N_OV8858_FMTS ARRAY_SIZE(ov8858_formats)

static struct ov8858_win_setting {
	int	width;
	int	height;
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
	struct regval_list *regs; /* Regs to tweak */
} ov8858_win_sizes[] = {
	/* 3264*2448 */
	{
		.width		= 3264,
		.height		= 2448,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov8858_init_3264_2448_raw8_regs,
	}
};
#define N_WIN_SIZES (ARRAY_SIZE(ov8858_win_sizes))

static int ov8858_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned index,
					enum v4l2_mbus_pixelcode *code)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	if (index >= N_OV8858_FMTS)
		return -EINVAL;

	*code = ov8858_formats[index].mbus_code;
	return 0;
}

static int ov8858_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt,
		struct ov8858_win_setting **ret_wsize)
{
	struct ov8858_win_setting *wsize;

	printk("[ov8858], %s,%d :fmt->code:%0x\n", __func__, __LINE__, fmt->code);

	if(fmt->width > MAX_WIDTH || fmt->height > MAX_HEIGHT)
		return -EINVAL;
	for (wsize = ov8858_win_sizes; wsize < ov8858_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;
	if (wsize >= ov8858_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	fmt->width = wsize->width;
	fmt->height = wsize->height;
	fmt->code = wsize->mbus_code;
	fmt->field = V4L2_FIELD_NONE;
	fmt->colorspace = wsize->colorspace;
	printk("%s:------->%d fmt->code,%08X , fmt->width%d fmt->height%d\n", __func__, __LINE__, fmt->code, fmt->width, fmt->height);
	return 0;
}

static int ov8858_try_mbus_fmt(struct v4l2_subdev *sd,
			    struct v4l2_mbus_framefmt *fmt)
{
	printk("%s:------->%d\n", __func__, __LINE__);
	return ov8858_try_fmt_internal(sd, fmt, NULL);
}

static int ov8858_s_mbus_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *fmt)
{

	struct ov8858_info *info = container_of(sd, struct ov8858_info, sd);
	struct v4l2_fmt_data *data = v4l2_get_fmt_data(fmt);
	struct ov8858_win_setting *wsize;
	int ret;

	printk("[ov5645], problem function:%s, line:%d\n", __func__, __LINE__);
	ret = ov8858_try_fmt_internal(sd, fmt, &wsize);
	if (ret)
		return ret;
	if ((info->win != wsize) && wsize->regs) {
		printk("pay attention : ov5645, %s:LINE:%d  size = %d\n", __func__, __LINE__, sizeof(*wsize->regs));
		ret = ov8858_write_array(sd, wsize->regs);
		if (ret)
			return ret;
	}
	data->i2cflags = V4L2_I2C_ADDR_16BIT;
	data->mipi_clk = 282;
	ret = ov8858_get_sensor_vts(sd, &(data->vts));
	if(ret < 0){
		printk("[ov5645], problem function:%s, line:%d\n", __func__, __LINE__);
		return ret;
	}
	ret = ov8858_get_sensor_lans(sd, &(data->lans));
	if(ret < 0){
		printk("[ov5645], problem function:%s, line:%d\n", __func__, __LINE__);
		return ret;
	}
	info->win = wsize;


	return 0;
}

static int ov8858_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
#if 1
	printk("--------%s: %d enable:%d\n", __func__, __LINE__, enable);
	if (enable) {
		ret = ov8858_write_array(sd, ov8858_stream_on);
		printk("ov8858 stream on\n");
	}
	else {
		ret = ov8858_write_array(sd, ov8858_stream_off);
		printk("ov8858 stream off\n");
	}
#endif
	return ret;
}

static int ov8858_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	a->c.left	= 0;
	a->c.top	= 0;
	a->c.width	= MAX_WIDTH;
	a->c.height	= MAX_HEIGHT;
	a->type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;

	return 0;
}

static int ov8858_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	a->bounds.left			= 0;
	a->bounds.top			= 0;
	a->bounds.width			= MAX_WIDTH;
	a->bounds.height		= MAX_HEIGHT;
	a->defrect			= a->bounds;
	a->type				= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	a->pixelaspect.numerator	= 1;
	a->pixelaspect.denominator	= 1;

	return 0;
}

static int ov8858_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov8858_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov8858_frame_rates[] = { 30, 15, 10, 5, 1 };

static int ov8858_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	if (interval->index >= ARRAY_SIZE(ov8858_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = ov8858_frame_rates[interval->index];
	return 0;
}

static int ov8858_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	/*
	 * If a minimum width/height was requested, filter out the capture
	 * windows that fall outside that.
	 */
	for (i = 0; i < N_WIN_SIZES; i++) {
		struct ov8858_win_setting *win = &ov8858_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int ov8858_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov8858_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov8858_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov8858_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	printk("functiong:%s, line:%d\n", __func__, __LINE__);

//	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_OV8858, 0);
	return v4l2_chip_ident_i2c_client(client, chip, 123, 0);
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov8858_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ov8858_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int ov8858_s_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ov8858_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static int ov8858_s_power(struct v4l2_subdev *sd, int on)
{
		printk("--%s:%d\n", __func__, __LINE__);
	return 0;
}

static const struct v4l2_subdev_core_ops ov8858_core_ops = {
	.g_chip_ident = ov8858_g_chip_ident,
	.g_ctrl = ov8858_g_ctrl,
	.s_ctrl = ov8858_s_ctrl,
	.queryctrl = ov8858_queryctrl,
	.reset = ov8858_reset,
	.init = ov8858_init,
	.s_power = ov8858_s_power,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov8858_g_register,
	.s_register = ov8858_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ov8858_video_ops = {
	.enum_mbus_fmt = ov8858_enum_mbus_fmt,
	.try_mbus_fmt = ov8858_try_mbus_fmt,
	.s_mbus_fmt = ov8858_s_mbus_fmt,
	.s_stream = ov8858_s_stream,
	.cropcap = ov8858_cropcap,
	.g_crop	= ov8858_g_crop,
	.s_parm = ov8858_s_parm,
	.g_parm = ov8858_g_parm,
	.enum_frameintervals = ov8858_enum_frameintervals,
	.enum_framesizes = ov8858_enum_framesizes,
};

static const struct v4l2_subdev_ops ov8858_ops = {
	.core = &ov8858_core_ops,
	.video = &ov8858_video_ops,
};

static ssize_t ov8858_rg_ratio_typical_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return sprintf(buf, "%d", rg_ratio_typical);
}

static ssize_t ov8858_rg_ratio_typical_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	char *endp;
	int value;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);

	value = simple_strtoul(buf, &endp, 0);
	if (buf == endp)
		return -EINVAL;

	rg_ratio_typical = (unsigned int)value;

	return size;
}

static ssize_t ov8858_bg_ratio_typical_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return sprintf(buf, "%d", bg_ratio_typical);
}

static ssize_t ov8858_bg_ratio_typical_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	char *endp;
	int value;

	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	value = simple_strtoul(buf, &endp, 0);
	if (buf == endp)
		return -EINVAL;

	bg_ratio_typical = (unsigned int)value;

	return size;
}

static DEVICE_ATTR(ov8858_rg_ratio_typical, 0664, ov8858_rg_ratio_typical_show, ov8858_rg_ratio_typical_store);
static DEVICE_ATTR(ov8858_bg_ratio_typical, 0664, ov8858_bg_ratio_typical_show, ov8858_bg_ratio_typical_store);

static int ov8858_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct ov8858_info *info;
	int ret;

	info = kzalloc(sizeof(struct ov8858_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &ov8858_ops);

	/* Make sure it's an ov8858 */
//aaa:
	ret = ov8858_detect(sd);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an ov8858 chip.\n",
			client->addr, client->adapter->name);
//		goto aaa;
		kfree(info);
		return ret;
	}
	v4l_info(client, "ov8858 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);

	ret = device_create_file(&client->dev, &dev_attr_ov8858_rg_ratio_typical);
	if(ret){
		v4l_err(client, "create dev_attr_ov8858_rg_ratio_typical failed!\n");
		goto err_create_dev_attr_ov8858_rg_ratio_typical;
	}

	ret = device_create_file(&client->dev, &dev_attr_ov8858_bg_ratio_typical);
	if(ret){
		v4l_err(client, "create dev_attr_ov8858_bg_ratio_typical failed!\n");
		goto err_create_dev_attr_ov8858_bg_ratio_typical;
	}

	printk("probe ok ------->ov8858\n");
	return 0;

err_create_dev_attr_ov8858_bg_ratio_typical:
	device_remove_file(&client->dev, &dev_attr_ov8858_rg_ratio_typical);
err_create_dev_attr_ov8858_rg_ratio_typical:
	kfree(info);
	return ret;
}

static int ov8858_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov8858_info *info = container_of(sd, struct ov8858_info, sd);

	device_remove_file(&client->dev, &dev_attr_ov8858_rg_ratio_typical);
	device_remove_file(&client->dev, &dev_attr_ov8858_bg_ratio_typical);

	v4l2_device_unregister_subdev(sd);
	kfree(info);
	return 0;
}

static const struct i2c_device_id ov8858_id[] = {
	{ "ov8858", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov8858_id);

static struct i2c_driver ov8858_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ov8858",
	},
	.probe		= ov8858_probe,
	.remove		= ov8858_remove,
	.id_table	= ov8858_id,
};

static __init int init_ov8858(void)
{
	printk("init_ov8858 #########\n");
	return i2c_add_driver(&ov8858_driver);
}

static __exit void exit_ov8858(void)
{
	i2c_del_driver(&ov8858_driver);
}

module_init(init_ov8858);
module_exit(exit_ov8858);

MODULE_DESCRIPTION("A low-level driver for OmniVision ov8858 sensors");
MODULE_LICENSE("GPL");
