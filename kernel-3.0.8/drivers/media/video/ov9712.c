#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <mach/ovisp-v4l2.h>
#include "ov9712.h"

#define OV9712_CHIP_ID_H	(0x97)
#define OV9712_CHIP_ID_L	(0x11)

#define VGA_WIDTH		640
#define VGA_HEIGHT		480
#define _720P_WIDTH		1280
#define _720P_HEIGHT		720

#define MAX_WIDTH		1280
#define MAX_HEIGHT		800

#define OV9712_REG_END		0xff
#define OV9712_REG_DELAY	0xfe

//#define OV9712_YUV

struct ov9712_format_struct;
struct ov9712_info {
	struct v4l2_subdev sd;
	struct ov9712_format_struct *fmt;
	struct ov9712_win_setting *win;
};

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

static struct regval_list ov9712_stream_on[] = {
	{0x09, 0x00},

	{OV9712_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov9712_stream_off[] = {
	/* Sensor enter LP11*/
	{0x09, 0x10},

	{OV9712_REG_END, 0x00},	/* END MARKER */
};
static struct regval_list ov9712_win_720p[] = {
	{OV9712_REG_END, 0x00},	/* END MARKER */
};
static struct regval_list ov9712_init_regs_1280_800[] = {
/*
	@@ DVP interface 1280*800 30fps
*/
       {0x12, 0x80}, // reset
       {0x09, 0x10},
       {0x1e, 0x07},
       {0x5f, 0x18},
       {0x69, 0x04},
       {0x65, 0x2a},
       {0x68, 0x0a},
       {0x39, 0x28},
       {0x4d, 0x90},
       {0xc1, 0x80},
       {0x0c, 0x30},
       {0x6d, 0x02},
       {0x96, 0x01}, //DSP options enable AWB Enable ok
       {0xbc, 0x68},
       {0x12, 0x00},
       {0x3b, 0x00}, //DSP Downsample
       {0x97, 0x80}, // bit3 colorbar
       {0x17, 0x25},
       {0x18, 0xa2},
       {0x19, 0x01},
       {0x1a, 0xca},
       {0x03, 0x0a},
       {0x32, 0x07},
       {0x98, 0x00},
       {0x99, 0x00},
       {0x9a, 0x00},
       {0x57, 0x00},
       {0x58, 0xc8},
       {0x59, 0xa0},
       {0x4c, 0x13},
       {0x4b, 0x36},
       {0x3d, 0xe3},
       {0x3e, 0x03},
       {0xbd, 0xa0},
       {0xbe, 0xc8},
       {0x41, 0x82}, //AVERAGE
       {0x4e, 0x55}, //AVERAGE
       {0x4f, 0x55},
       {0x50, 0x55},
       {0x51, 0x55},
       {0x24, 0x55}, //Exposure windows
       {0x25, 0x40},
       {0x26, 0xa1},
       {0x5c, 0x52},
       {0x5d, 0x00},
       {0x11, 0x01},
       {0x2a, 0x98},
       {0x2b, 0x06},
       {0x2d, 0x00},
       {0x2e, 0x00},
       {0x13, 0xa0},//manual exposure & gain ok
       {0x14, 0x40}, //gain ceiling 8X
       {0x4a, 0x00},/* banding step remove for isp calibration */
       {0x49, 0xce},
       {0x22, 0x03},
       {0x09, 0x00},
       {0x60, 0x9d},
       {OV9712_REG_END, 0x00},	/* END MARKER */
};

static struct ov9712_format_struct {
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
} ov9712_formats[] = {
	{
		/*RAW10 FORMAT, 10 bit per pixel*/
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
	},
	/*add other format supported*/
};
#define N_OV9712_FMTS ARRAY_SIZE(ov9712_formats)

static struct ov9712_win_setting {
	int	width;
	int	height;
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
	struct regval_list *regs; /* Regs to tweak */
} ov9712_win_sizes[] = {
	/* 1280*800 */
	{
		.width		= 1280,
		.height		= 800,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov9712_init_regs_1280_800,
	}
};
#define N_WIN_SIZES (ARRAY_SIZE(ov9712_win_sizes))

int ov9712_read(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[2] = {
		[0] = {
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= &reg,
		},
		[1] = {
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= value,
		}
	};
	int ret;
//	printk("client->addr = 0x%02x\n",client->addr);
	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;

	return ret;
}


 int ov9712_write(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[2] = {reg, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 2,
		.buf	= buf,
	};
	int ret;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}

static int ov9712_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != OV9712_REG_END) {
		if (vals->reg_num == OV9712_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = ov9712_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		printk("vals->reg_num:0x%02x, vals->value:0x%02x\n",vals->reg_num, val);
		//mdelay(200);
		vals++;
	}
	//printk("vals->reg_num:%x, vals->value:%x\n", vals->reg_num, vals->value);
	//ov9712_write(sd, vals->reg_num, vals->value);
	return 0;
}
static int ov9712_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != OV9712_REG_END) {
		if (vals->reg_num == OV9712_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = ov9712_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		//printk("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
		//mdelay(200);
		vals++;
	}
	//printk("vals->reg_num:%x, vals->value:%x\n", vals->reg_num, vals->value);
	//ov9712_write(sd, vals->reg_num, vals->value);
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
static int ov9712_update_awb_gain(struct v4l2_subdev *sd,
				unsigned int R_gain, unsigned int G_gain, unsigned int B_gain)
{
#if 0
	printk("[ov9712] problem function:%s, line:%d\n", __func__, __LINE__);
	printk("R_gain:%04x, G_gain:%04x, B_gain:%04x \n ", R_gain, G_gain, B_gain);
	if (R_gain > 0x400) {
		ov9712_write(sd, 0x5186, (unsigned char)(R_gain >> 8));
		ov9712_write(sd, 0x5187, (unsigned char)(R_gain & 0x00ff));
	}

	if (G_gain > 0x400) {
		ov9712_write(sd, 0x5188, (unsigned char)(G_gain >> 8));
		ov9712_write(sd, 0x5189, (unsigned char)(G_gain & 0x00ff));
	}

	if (B_gain > 0x400) {
		ov9712_write(sd, 0x518a, (unsigned char)(B_gain >> 8));
		ov9712_write(sd, 0x518b, (unsigned char)(B_gain & 0x00ff));
	}
#endif

	return 0;
}

static int ov9712_reset(struct v4l2_subdev *sd, u32 val)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov9712_detect(struct v4l2_subdev *sd);
static int ov9712_init(struct v4l2_subdev *sd, u32 val)
{
	struct ov9712_info *info = container_of(sd, struct ov9712_info, sd);
	int ret = 0;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);

	info->fmt = NULL;
	info->win = NULL;

//	ret = ov9712_write_array(sd, ov9712_init_regs_1280_800);
	if (ret < 0)
		return ret;
//	ov9712_read_array(sd, ov9712_init_regs_1280_800);

	return 0;
}
static int ov9712_get_sensor_vts(struct v4l2_subdev *sd, unsigned short *value)
{
	unsigned char h,l;
	int ret = 0;
	ret = ov9712_read(sd, 0x3e, &h);
	if (ret < 0)
		return ret;
	ret = ov9712_read(sd, 0x3d, &l);
	if (ret < 0)
		return ret;
	*value = h;
	*value = (*value << 8) | (l & 0xff);
	return ret;
}

static int ov9712_get_sensor_lans(struct v4l2_subdev *sd, unsigned char *value)
{
	return 0;
}
static int ov9712_detect(struct v4l2_subdev *sd)
{
	unsigned char v;
	int ret;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
#if 1
	/*test gpio e*/
	{
		printk("0x10010210:%x\n", *((volatile unsigned int *)0xb0010210));
		printk("0x10010220:%x\n", *((volatile unsigned int *)0xb0010220));
		printk("0x10010230:%x\n", *((volatile unsigned int *)0xb0010230));
		printk("0x10010240:%x\n", *((volatile unsigned int *)0xb0010240));
		printk("0x10010010:%x\n", *((volatile unsigned int *)0xb0010010));
		printk("0x10010020:%x\n", *((volatile unsigned int *)0xb0010020));
		printk("0x10010030:%x\n", *((volatile unsigned int *)0xb0010030));
		printk("0x10010040:%x\n", *((volatile unsigned int *)0xb0010040));

	}
#endif
	ret = ov9712_read(sd, 0x0a, &v);
	printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	printk("-----%s: %d v = %08X\n", __func__, __LINE__, v);
	if (v != OV9712_CHIP_ID_H)
		return -ENODEV;
	ret = ov9712_read(sd, 0x0b, &v);
	printk("-----%s: %d ret = %d\n", __func__, __LINE__, ret);
	if (ret < 0)
		return ret;
	printk("-----%s: %d v = %08X\n", __func__, __LINE__, v);
	if (v != OV9712_CHIP_ID_L)
		return -ENODEV;
	return 0;
}

static int ov9712_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned index,
					enum v4l2_mbus_pixelcode *code)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	if (index >= N_OV9712_FMTS)
		return -EINVAL;

	*code = ov9712_formats[index].mbus_code;
	return 0;
}

static int ov9712_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt,
		struct ov9712_win_setting **ret_wsize)
{
	struct ov9712_win_setting *wsize;

	if(fmt->width > MAX_WIDTH || fmt->height > MAX_HEIGHT)
		return -EINVAL;
	for (wsize = ov9712_win_sizes; wsize < ov9712_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;
	if (wsize >= ov9712_win_sizes + N_WIN_SIZES)
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

static int ov9712_try_mbus_fmt(struct v4l2_subdev *sd,
			    struct v4l2_mbus_framefmt *fmt)
{
	printk("%s:------->%d\n", __func__, __LINE__);
	return ov9712_try_fmt_internal(sd, fmt, NULL);
}

static int ov9712_s_mbus_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *fmt)
{

	struct ov9712_info *info = container_of(sd, struct ov9712_info, sd);
	struct v4l2_fmt_data *data = v4l2_get_fmt_data(fmt);
	struct ov9712_win_setting *wsize;
	int ret;


	printk("[ov9712], problem function:%s, line:%d\n", __func__, __LINE__);
	ret = ov9712_try_fmt_internal(sd, fmt, &wsize);
	if (ret)
		return ret;
	if ((info->win != wsize) && wsize->regs) {
		printk("pay attention : ov9712, %s:LINE:%d  size = %d\n", __func__, __LINE__, sizeof(*wsize->regs));
		ret = ov9712_write_array(sd, wsize->regs);
		if (ret)
			return ret;
	}
	data->i2cflags = 0;
	data->mipi_clk = 282;
	ret = ov9712_get_sensor_vts(sd, &(data->vts));
	if(ret < 0){
		printk("[ov9712], problem function:%s, line:%d\n", __func__, __LINE__);
		return ret;
	}
	ret = ov9712_get_sensor_lans(sd, &(data->lans));
	if(ret < 0){
		printk("[ov9712], problem function:%s, line:%d\n", __func__, __LINE__);
		return ret;
	}
	info->win = wsize;

	return 0;
}

static int ov9712_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	printk("--------%s: %d enable:%d\n", __func__, __LINE__, enable);
	if (enable) {
		ret = ov9712_write_array(sd, ov9712_stream_on);
		printk("ov9712 stream on\n");
	}
	else {
		ret = ov9712_write_array(sd, ov9712_stream_off);
		printk("ov9712 stream off\n");
	}
	return ret;
}

static int ov9712_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	a->c.left	= 0;
	a->c.top	= 0;
	a->c.width	= MAX_WIDTH;
	a->c.height	= MAX_HEIGHT;
	a->type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;

	return 0;
}

static int ov9712_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
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

static int ov9712_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov9712_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov9712_frame_rates[] = { 30, 15, 10, 5, 1 };

static int ov9712_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	if (interval->index >= ARRAY_SIZE(ov9712_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = ov9712_frame_rates[interval->index];
	return 0;
}

static int ov9712_enum_framesizes(struct v4l2_subdev *sd,
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
		struct ov9712_win_setting *win = &ov9712_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int ov9712_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov9712_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov9712_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov9712_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	printk("functiong:%s, line:%d\n", __func__, __LINE__);

//	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_OV9712, 0);
	return v4l2_chip_ident_i2c_client(client, chip, 123, 0);
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov9712_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ov9712_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int ov9712_s_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ov9712_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static int ov9712_s_power(struct v4l2_subdev *sd, int on)
{
	printk("--%s:%d\n", __func__, __LINE__);
	return 0;
}

static const struct v4l2_subdev_core_ops ov9712_core_ops = {
	.g_chip_ident = ov9712_g_chip_ident,
	.g_ctrl = ov9712_g_ctrl,
	.s_ctrl = ov9712_s_ctrl,
	.queryctrl = ov9712_queryctrl,
	.reset = ov9712_reset,
	.init = ov9712_init,
	.s_power = ov9712_s_power,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov9712_g_register,
	.s_register = ov9712_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ov9712_video_ops = {
	.enum_mbus_fmt = ov9712_enum_mbus_fmt,
	.try_mbus_fmt = ov9712_try_mbus_fmt,
	.s_mbus_fmt = ov9712_s_mbus_fmt,
	.s_stream = ov9712_s_stream,
	.cropcap = ov9712_cropcap,
	.g_crop	= ov9712_g_crop,
	.s_parm = ov9712_s_parm,
	.g_parm = ov9712_g_parm,
	.enum_frameintervals = ov9712_enum_frameintervals,
	.enum_framesizes = ov9712_enum_framesizes,
};

static const struct v4l2_subdev_ops ov9712_ops = {
	.core = &ov9712_core_ops,
	.video = &ov9712_video_ops,
};

static ssize_t ov9712_rg_ratio_typical_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return sprintf(buf, "%d", rg_ratio_typical);
}

static ssize_t ov9712_rg_ratio_typical_store(struct device *dev,
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

static ssize_t ov9712_bg_ratio_typical_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return sprintf(buf, "%d", bg_ratio_typical);
}

static ssize_t ov9712_bg_ratio_typical_store(struct device *dev,
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

static DEVICE_ATTR(ov9712_rg_ratio_typical, 0664, ov9712_rg_ratio_typical_show, ov9712_rg_ratio_typical_store);
static DEVICE_ATTR(ov9712_bg_ratio_typical, 0664, ov9712_bg_ratio_typical_show, ov9712_bg_ratio_typical_store);

static int ov9712_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct ov9712_info *info;
	int ret;

	info = kzalloc(sizeof(struct ov9712_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &ov9712_ops);

	/* Make sure it's an ov9712 */
//aaa:
	ret = ov9712_detect(sd);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an ov9712 chip.\n",
			client->addr, client->adapter->name);
//		goto aaa;
		kfree(info);
		return ret;
	}
	v4l_info(client, "ov9712 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);

	ret = device_create_file(&client->dev, &dev_attr_ov9712_rg_ratio_typical);
	if(ret){
		v4l_err(client, "create dev_attr_ov9712_rg_ratio_typical failed!\n");
		goto err_create_dev_attr_ov9712_rg_ratio_typical;
	}

	ret = device_create_file(&client->dev, &dev_attr_ov9712_bg_ratio_typical);
	if(ret){
		v4l_err(client, "create dev_attr_ov9712_bg_ratio_typical failed!\n");
		goto err_create_dev_attr_ov9712_bg_ratio_typical;
	}

	printk("probe ok ------->ov9712\n");
	return 0;

err_create_dev_attr_ov9712_bg_ratio_typical:
	device_remove_file(&client->dev, &dev_attr_ov9712_rg_ratio_typical);
err_create_dev_attr_ov9712_rg_ratio_typical:
	kfree(info);
	return ret;
}

static int ov9712_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov9712_info *info = container_of(sd, struct ov9712_info, sd);

	device_remove_file(&client->dev, &dev_attr_ov9712_rg_ratio_typical);
	device_remove_file(&client->dev, &dev_attr_ov9712_bg_ratio_typical);

	v4l2_device_unregister_subdev(sd);
	kfree(info);
	return 0;
}

static const struct i2c_device_id ov9712_id[] = {
	{ "ov9712", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov9712_id);

static struct i2c_driver ov9712_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ov9712",
	},
	.probe		= ov9712_probe,
	.remove		= ov9712_remove,
	.id_table	= ov9712_id,
};

static __init int init_ov9712(void)
{
	printk("init_ov9712 #########\n");
	return i2c_add_driver(&ov9712_driver);
}

static __exit void exit_ov9712(void)
{
	i2c_del_driver(&ov9712_driver);
}

module_init(init_ov9712);
module_exit(exit_ov9712);

MODULE_DESCRIPTION("A low-level driver for OmniVision ov9712 sensors");
MODULE_LICENSE("GPL");
