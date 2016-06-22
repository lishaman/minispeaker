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
#include "bf3116.h"

#define BF3116_CHIP_ID_H	(0x31)
#define BF3116_CHIP_ID_L	(0x16)

#define VGA_WIDTH		640
#define VGA_HEIGHT		480
#define _720P_WIDTH		1280
#define _720P_HEIGHT		720

#define MAX_WIDTH		1280
#define MAX_HEIGHT		720

#define BF3116_FLAG_END       0x00
#define BF3116_FLAG_VALID    0x01
#define BF3116_FLAG_DELAY    0x02
#define BF3116_PAGE_REG       0xfe

struct bf3116_format_struct;
struct bf3116_info {
	struct v4l2_subdev sd;
	struct bf3116_format_struct *fmt;
	struct bf3116_win_setting *win;
};

struct regval_list {
	unsigned char flag;//if flag eq 0,end;flag eq 1 write;flag eq 2 delay
	unsigned char reg_num;
	unsigned char value;
};

static struct regval_list bf3116_stream_on[] = {
	{0x01, 0xfe, 0x00},
	{0x01, 0xe8, 0x00},
	{BF3116_FLAG_END, 0x00, 0x00},	/* END MARKER */
};

static struct regval_list bf3116_stream_off[] = {
	{0x01, 0xfe, 0x00},
	{0x01, 0xe8, 0x10},
	{BF3116_FLAG_END, 0x00, 0x00},	/* END MARKER */
};

static struct regval_list bf3116_init_regs_1280_720[] = {
/*
	@@ DVP interface 1280*720 30fps
*/
	{0x01, 0xfe, 0x00},
	{0x01, 0xe8, 0x10},
	{0x01, 0x01, 0x10},
	{0x01, 0xfe, 0x01},
	{0x01, 0x00, 0x38},
	{0x01, 0x04, 0x1f},
	{0x01, 0x09, 0x02},
	{0x01, 0x42, 0x50},
	{0x01, 0x43, 0x20},
	{0x01, 0x44, 0x00},
	{0x01, 0x45, 0x00},
	{0x01, 0x46, 0x00},
	{0x01, 0x47, 0xd0},
	{0x01, 0xfe, 0x00},
	{0x01, 0x09, 0x00},
	{0x01, 0x0a, 0x86},
	{0x01, 0xe1, 0x03},  //27MHz input.
	{0x01, 0xe2, 0x2b},
	{0x01, 0xe3, 0x15},
	{0x01, 0xe4, 0x5d},
	{0x01, 0xe5, 0x41},
	{0x01, 0xe6, 0x44},
	{0x01, 0xe7, 0x20},
	{0x01, 0xe8, 0x00},
	{0x01, 0xe9, 0x00},
	{0x01, 0xea, 0x10},
	{0x01, 0xeb, 0x05},
	{0x01, 0xec, 0x72},
	{0x01, 0xed, 0x06},
	{0x01, 0xf1, 0x02},
	{0x01, 0x81, 0xad},
	{0x01, 0x82, 0x0f},
	{0x01, 0x85, 0x00},
	{0x01, 0x86, 0x85},
	{0x01, 0xfe, 0x00},
	{0x01, 0xb2, 0x91},
	{0x01, 0xb0, 0x16},
	{0x01, 0xb1, 0x16},
	{0x01, 0xb3, 0x55},
	{0x01, 0x3d, 0xff},
	{0x01, 0x30, 0x54},
	{0x01, 0x31, 0x56},
	{0x01, 0x32, 0x54},
	{0x01, 0x33, 0x56},
	{0x01, 0x34, 0x02},
	{0x01, 0x35, 0x04},
	{0x01, 0x36, 0x06},
	{0x01, 0x37, 0x04},
	{0x01, 0x4c, 0x80}, //color bar
	{0x01, 0xfe, 0x01},
	{0x01, 0x40, 0x00}, //vsync pol
	{BF3116_FLAG_END, 0x00, 0x00},
};

static struct bf3116_format_struct {
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
} bf3116_formats[] = {
	{
		/*RAW10 FORMAT, 10 bit per pixel*/
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
	},
	/*add other format supported*/
};
#define N_BF3116_FMTS ARRAY_SIZE(bf3116_formats)

static struct bf3116_win_setting {
	int	width;
	int	height;
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
	struct regval_list *regs; /* Regs to tweak */
} bf3116_win_sizes[] = {
	/* 1280*720 */
	{
		.width		= 1280,
		.height		= 720,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= bf3116_init_regs_1280_720,
	}
};
#define N_WIN_SIZES (ARRAY_SIZE(bf3116_win_sizes))

int bf3116_read(struct v4l2_subdev *sd, unsigned char reg,
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
	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;

	return ret;
}


 int bf3116_write(struct v4l2_subdev *sd, unsigned char reg,
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

static int bf3116_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->flag != BF3116_FLAG_END) {
		if (vals->flag == BF3116_FLAG_DELAY){
			vals++;
			continue;
		}
		if (vals->reg_num == BF3116_PAGE_REG){
			ret = bf3116_read(sd, vals->reg_num, &val);
			val &= 0xfe;
			val |= (vals->value & 0x01);
			ret = bf3116_write(sd, vals->reg_num, val);
			ret = bf3116_read(sd, vals->reg_num, &val);
			printk("ret = %d, vals ->flag:0x%02x,vals->reg_num:0x%02x, vals->value:0x%02x\n", ret, vals->flag, vals->reg_num, val);
		} else {
			ret = bf3116_read(sd, vals->reg_num, &val);
			printk("ret = %d, vals ->flag:0x%02x,vals->reg_num:0x%02x, vals->value:0x%02x\n", ret, vals->flag, vals->reg_num, val);
		}
		vals++;
	}
	return 0;
}

static int bf3116_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->flag != BF3116_FLAG_END){
		if(vals->flag == BF3116_FLAG_DELAY){
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else if (vals->flag == BF3116_FLAG_VALID) {
			ret = bf3116_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		} else {
			printk("Other Flag that not require!\n");
		}
		vals++;
	}
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
static int bf3116_update_awb_gain(struct v4l2_subdev *sd,
				unsigned int R_gain, unsigned int G_gain, unsigned int B_gain)
{
#if 0
	printk("[bf3116] problem function:%s, line:%d\n", __func__, __LINE__);
	printk("R_gain:%04x, G_gain:%04x, B_gain:%04x \n ", R_gain, G_gain, B_gain);
	if (R_gain > 0x400) {
		bf3116_write(sd, 0x5186, (unsigned char)(R_gain >> 8));
		bf3116_write(sd, 0x5187, (unsigned char)(R_gain & 0x00ff));
	}

	if (G_gain > 0x400) {
		bf3116_write(sd, 0x5188, (unsigned char)(G_gain >> 8));
		bf3116_write(sd, 0x5189, (unsigned char)(G_gain & 0x00ff));
	}

	if (B_gain > 0x400) {
		bf3116_write(sd, 0x518a, (unsigned char)(B_gain >> 8));
		bf3116_write(sd, 0x518b, (unsigned char)(B_gain & 0x00ff));
	}
#endif

	return 0;
}

static int bf3116_reset(struct v4l2_subdev *sd, u32 val)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int bf3116_detect(struct v4l2_subdev *sd);
static int bf3116_init(struct v4l2_subdev *sd, u32 val)
{
	struct bf3116_info *info = container_of(sd, struct bf3116_info, sd);
	int ret = 0;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);

	info->fmt = NULL;
	info->win = NULL;

//	ret = bf3116_write_array(sd, bf3116_init_regs_1280_720);
	if (ret < 0)
		return ret;
//	bf3116_read_array(sd, bf3116_init_regs_1280_720);

	return 0;
}

static int bf3116_get_sensor_vts(struct v4l2_subdev *sd, unsigned short *value)
{
	return 0;
}

static int bf3116_get_sensor_lans(struct v4l2_subdev *sd, unsigned char *value)
{
	return 0;
}

static int bf3116_detect(struct v4l2_subdev *sd)
{
	unsigned char v;
	int ret;

	ret = bf3116_read(sd, 0xfc, &v);
	printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	printk("-----%s: %d v = %08X\n", __func__, __LINE__, v);
	if (v != BF3116_CHIP_ID_H)
		return -ENODEV;
	ret = bf3116_read(sd, 0xfd, &v);
	printk("-----%s: %d ret = %d\n", __func__, __LINE__, ret);
	if (ret < 0)
		return ret;
	printk("-----%s: %d v = %08X\n", __func__, __LINE__, v);
	if (v != BF3116_CHIP_ID_L)
		return -ENODEV;
	return 0;
}

static int bf3116_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned index,
					enum v4l2_mbus_pixelcode *code)
{
	if (index >= N_BF3116_FMTS)
		return -EINVAL;

	*code = bf3116_formats[index].mbus_code;
	return 0;
}

static int bf3116_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt,
		struct bf3116_win_setting **ret_wsize)
{
	struct bf3116_win_setting *wsize;

	if(fmt->width > MAX_WIDTH || fmt->height > MAX_HEIGHT)
		return -EINVAL;
	for (wsize = bf3116_win_sizes; wsize < bf3116_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;
	if (wsize >= bf3116_win_sizes + N_WIN_SIZES)
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

static int bf3116_try_mbus_fmt(struct v4l2_subdev *sd,
			    struct v4l2_mbus_framefmt *fmt)
{
	printk("%s:------->%d\n", __func__, __LINE__);
	return bf3116_try_fmt_internal(sd, fmt, NULL);
}

static int bf3116_s_mbus_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *fmt)
{

	struct bf3116_info *info = container_of(sd, struct bf3116_info, sd);
	struct v4l2_fmt_data *data = v4l2_get_fmt_data(fmt);
	struct bf3116_win_setting *wsize;
	int ret;


	printk("[bf3116], problem function:%s, line:%d\n", __func__, __LINE__);
	ret = bf3116_try_fmt_internal(sd, fmt, &wsize);
	if (ret)
		return ret;
	if ((info->win != wsize) && wsize->regs) {
		printk("pay attention : bf3116, %s:LINE:%d  size = %d\n", __func__, __LINE__, sizeof(*wsize->regs));
		ret = bf3116_write_array(sd, wsize->regs);
		if (ret)
			return ret;
	}

	data->i2cflags = 0;
	data->mipi_clk = 282;
	ret = bf3116_get_sensor_vts(sd, &(data->vts));
	if(ret < 0){
		printk("[bf3116], problem function:%s, line:%d\n", __func__, __LINE__);
		return ret;
	}
	ret = bf3116_get_sensor_lans(sd, &(data->lans));
	if(ret < 0){
		printk("[bf3116], problem function:%s, line:%d\n", __func__, __LINE__);
		return ret;
	}
	info->win = wsize;

	return 0;
}

static int bf3116_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	printk("--------%s: %d enable:%d\n", __func__, __LINE__, enable);
	if (enable) {
		ret = bf3116_write_array(sd, bf3116_stream_on);
		printk("bf3116 stream on\n");
	}
	else {
		ret = bf3116_write_array(sd, bf3116_stream_off);
		printk("bf3116 stream off\n");
	}
	return ret;
}

static int bf3116_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	a->c.left	= 0;
	a->c.top	= 0;
	a->c.width	= MAX_WIDTH;
	a->c.height	= MAX_HEIGHT;
	a->type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;

	return 0;
}

static int bf3116_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
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

static int bf3116_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int bf3116_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int bf3116_frame_rates[] = { 30, 15, 10, 5, 1 };

static int bf3116_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	if (interval->index >= ARRAY_SIZE(bf3116_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = bf3116_frame_rates[interval->index];
	return 0;
}

static int bf3116_enum_framesizes(struct v4l2_subdev *sd,
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
		struct bf3116_win_setting *win = &bf3116_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int bf3116_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int bf3116_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int bf3116_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int bf3116_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	printk("functiong:%s, line:%d\n", __func__, __LINE__);

//	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_bf3116, 0);
	return v4l2_chip_ident_i2c_client(client, chip, 123, 0);
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int bf3116_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = bf3116_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int bf3116_s_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	bf3116_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static int bf3116_s_power(struct v4l2_subdev *sd, int on)
{
	printk("--%s:%d\n", __func__, __LINE__);
	return 0;
}

static const struct v4l2_subdev_core_ops bf3116_core_ops = {
	.g_chip_ident = bf3116_g_chip_ident,
	.g_ctrl = bf3116_g_ctrl,
	.s_ctrl = bf3116_s_ctrl,
	.queryctrl = bf3116_queryctrl,
	.reset = bf3116_reset,
	.init = bf3116_init,
	.s_power = bf3116_s_power,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = bf3116_g_register,
	.s_register = bf3116_s_register,
#endif
};

static const struct v4l2_subdev_video_ops bf3116_video_ops = {
	.enum_mbus_fmt = bf3116_enum_mbus_fmt,
	.try_mbus_fmt = bf3116_try_mbus_fmt,
	.s_mbus_fmt = bf3116_s_mbus_fmt,
	.s_stream = bf3116_s_stream,
	.cropcap = bf3116_cropcap,
	.g_crop	= bf3116_g_crop,
	.s_parm = bf3116_s_parm,
	.g_parm = bf3116_g_parm,
	.enum_frameintervals = bf3116_enum_frameintervals,
	.enum_framesizes = bf3116_enum_framesizes,
};

static const struct v4l2_subdev_ops bf3116_ops = {
	.core = &bf3116_core_ops,
	.video = &bf3116_video_ops,
};

static ssize_t bf3116_rg_ratio_typical_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return sprintf(buf, "%d", rg_ratio_typical);
}

static ssize_t bf3116_rg_ratio_typical_store(struct device *dev,
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

static ssize_t bf3116_bg_ratio_typical_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return sprintf(buf, "%d", bg_ratio_typical);
}

static ssize_t bf3116_bg_ratio_typical_store(struct device *dev,
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

static DEVICE_ATTR(bf3116_rg_ratio_typical, 0664, bf3116_rg_ratio_typical_show, bf3116_rg_ratio_typical_store);
static DEVICE_ATTR(bf3116_bg_ratio_typical, 0664, bf3116_bg_ratio_typical_show, bf3116_bg_ratio_typical_store);

static int bf3116_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct bf3116_info *info;
	int ret;

	info = kzalloc(sizeof(struct bf3116_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &bf3116_ops);

	/* Make sure it's an bf3116 */
//aaa:
	ret = bf3116_detect(sd);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an bf3116 chip.\n",
			client->addr, client->adapter->name);
//		goto aaa;
		kfree(info);
		return ret;
	}
	v4l_info(client, "bf3116 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);

	ret = device_create_file(&client->dev, &dev_attr_bf3116_rg_ratio_typical);
	if(ret){
		v4l_err(client, "create dev_attr_bf3116_rg_ratio_typical failed!\n");
		goto err_create_dev_attr_bf3116_rg_ratio_typical;
	}

	ret = device_create_file(&client->dev, &dev_attr_bf3116_bg_ratio_typical);
	if(ret){
		v4l_err(client, "create dev_attr_bf3116_bg_ratio_typical failed!\n");
		goto err_create_dev_attr_bf3116_bg_ratio_typical;
	}

	printk("probe ok ------->bf3116\n");
	return 0;

err_create_dev_attr_bf3116_bg_ratio_typical:
	device_remove_file(&client->dev, &dev_attr_bf3116_rg_ratio_typical);
err_create_dev_attr_bf3116_rg_ratio_typical:
	kfree(info);
	return ret;
}

static int bf3116_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct bf3116_info *info = container_of(sd, struct bf3116_info, sd);

	device_remove_file(&client->dev, &dev_attr_bf3116_rg_ratio_typical);
	device_remove_file(&client->dev, &dev_attr_bf3116_bg_ratio_typical);

	v4l2_device_unregister_subdev(sd);
	kfree(info);
	return 0;
}

static const struct i2c_device_id bf3116_id[] = {
	{ "bf3116", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bf3116_id);

static struct i2c_driver bf3116_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "bf3116",
	},
	.probe		= bf3116_probe,
	.remove		= bf3116_remove,
	.id_table	= bf3116_id,
};

static __init int init_bf3116(void)
{
	printk("init_bf3116 #########\n");
	return i2c_add_driver(&bf3116_driver);
}

static __exit void exit_bf3116(void)
{
	i2c_del_driver(&bf3116_driver);
}

module_init(init_bf3116);
module_exit(exit_bf3116);

MODULE_DESCRIPTION("A low-level driver for BYD bf3116 sensors");
MODULE_LICENSE("GPL");
