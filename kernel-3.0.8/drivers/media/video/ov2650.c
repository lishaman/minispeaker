/*
 * V4L2 Driver for camera sensor ov2650
 *
 * Copyright (C) 2012, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-subdev.h>
#include <media/soc_camera.h>
#include <media/soc_mediabus.h>

#define REG_CHIP_ID		0x26
/* Private v4l2 controls */
#define V4L2_CID_PRIVATE_BALANCE  (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PRIVATE_EFFECT  (V4L2_CID_PRIVATE_BASE + 1)


#define REG29				0x29
#define REG29_HFLIP_IMG		0x01 /* Horizontal mirror image ON/OFF */
#define REG29_VFLIP_IMG     0x02 /* Vertical flip image ON/OFF */

#define SENSOR_WRITE_DELAY 0xffff

 /* whether sensor support high resolution (> vga) preview or not */
#define SUPPORT_HIGH_RESOLUTION_PRE		0

/*
 * Struct
 */
struct regval_list {
	u16 reg_num;
	u8 value;
};

struct mode_list {
	u8 index;
	const struct regval_list *mode_regs;
};

/* Supported resolutions */
enum ov2650_width {
	W_QCIF	= 176,
	W_CIF	= 352,
	W_VGA	= 640,
	W_XGA	= 1024,
	W_UXGA	= 1600,
};

enum ov2650_height {
	H_QCIF	= 144,
	H_CIF	= 288,
	H_VGA	= 480,
	H_XGA	= 768,
	H_UXGA	= 1200,
};

struct ov2650_win_size {
	char *name;
	enum ov2650_width width;
	enum ov2650_height height;
	const struct regval_list *regs;
};


struct ov2650_priv {
	struct v4l2_subdev subdev;
	struct ov2650_camera_info *info;
	enum v4l2_mbus_pixelcode cfmt_code;
	const struct ov2650_win_size *win;
	int	model;
	u8 balance_value;
	u8 effect_value;
	u16	flag_vflip:1;
	u16	flag_hflip:1;
};

/*
 * Registers settings
 */

#define ENDMARKER { 0xffff, 0xff }

static const struct regval_list ov2650_init_regs[] = {
	//Soft Reset
	{0x3012, 0x80},
	//Add some dealy or wait a few miliseconds after register reset
	{SENSOR_WRITE_DELAY, 0x0a},

	//IO & Clock & Analog Setup
	{0x308c, 0x80},
	{0x308d, 0x0e},
	{0x360b, 0x00},
	{0x30b0, 0xff},
	{0x30b1, 0xff},
	{0x30b2, 0x24},

	{0x300e, 0x34},
	{0x300f, 0xa6},
	{0x3010, 0x81},
	{0x3082, 0x01},
	{0x30f4, 0x01},
	{0x3090, 0x33},
	{0x3091, 0xc0},
	{0x30ac, 0x42},

	{0x30d1, 0x08},
	{0x30a8, 0x56},
	{0x3015, 0x03},
	{0x3093, 0x00},
	{0x307e, 0xe5},
	{0x3079, 0x00},
	{0x30aa, 0x42},
	{0x3017, 0x40},
	{0x30f3, 0x82},
	{0x306a, 0x0c},
	{0x306d, 0x00},
	{0x336a, 0x3c},
	{0x3076, 0x6a},
	{0x30d9, 0x8c},
	{0x3016, 0x82},
	{0x3601, 0x30},
	{0x304e, 0x88},
	{0x30f1, 0x82},
	{0x306f, 0x14},
	{0x302a, 0x02},
	{0x302b, 0x6a},

	{0x3012, 0x10},
	{0x3011, 0x01},

	//AEC/AGC
	{0x3013, 0xf7},
	{0x301c, 0x13},
	{0x301d, 0x17},
	{0x3070, 0x5d},
	{0x3072, 0x4d},

	//D5060
	{0x30af, 0x00},
	{0x3048, 0x1f},
	{0x3049, 0x4e},
	{0x304a, 0x20},
	{0x304f, 0x20},
	{0x304b, 0x02},
	{0x304c, 0x00},
	{0x304d, 0x02},
	{0x304f, 0x20},
	{0x30a3, 0x10},
	{0x3013, 0xf7},
	{0x3014, 0x44},
	{0x3071, 0x00},
	{0x3070, 0x5d},
	{0x3073, 0x00},
	{0x3072, 0x4d},
	{0x301c, 0x05},
	{0x301d, 0x06},
	{0x304d, 0x42},
	{0x304a, 0x40},
	{0x304f, 0x40},
	{0x3095, 0x07},
	{0x3096, 0x16},
	{0x3097, 0x1d},

	//Window Setup
	{0x300e, 0x38},
	{0x3020, 0x01},
	{0x3021, 0x18},
	{0x3022, 0x00},
	{0x3023, 0x06},
	{0x3024, 0x06},
	{0x3025, 0x58},
	{0x3026, 0x02},
	{0x3027, 0x5e},
	{0x3088, 0x03},
	{0x3089, 0x20},
	{0x308a, 0x02},
	{0x308b, 0x58},
	{0x3316, 0x64},
	{0x3317, 0x25},
	{0x3318, 0x80},
	{0x3319, 0x08},
	{0x331a, 0x64},
	{0x331b, 0x4b},
	{0x331c, 0x00},
	{0x331d, 0x38},
	{0x3100, 0x00},

	//AWB
	{0x3320, 0xfa},
	{0x3321, 0x11},
	{0x3322, 0x92},
	{0x3323, 0x01},
	{0x3324, 0x97},
	{0x3325, 0x02},
	{0x3326, 0xff},
	{0x3327, 0x0c},
	{0x3328, 0x10},
	{0x3329, 0x10},
	{0x332a, 0x58},
	{0x332b, 0x50},
	{0x332c, 0xbe},
	{0x332d, 0xe1},
	{0x332e, 0x43},
	{0x332f, 0x36},
	{0x3330, 0x4d},
	{0x3331, 0x44},
	{0x3332, 0xf8},
	{0x3333, 0x0a},
	{0x3334, 0xf0},
	{0x3335, 0xf0},
	{0x3336, 0xf0},
	{0x3337, 0x40},
	{0x3338, 0x40},
	{0x3339, 0x40},
	{0x333a, 0x00},
	{0x333b, 0x00},

	//Color Matrix
	{0x3380, 0x28},
	{0x3381, 0x48},
	{0x3382, 0x10},
	{0x3383, 0x23},
	{0x3384, 0xc0},
	{0x3385, 0xe5},
	{0x3386, 0xc2},
	{0x3387, 0xb3},
	{0x3388, 0x0e},
	{0x3389, 0x98},
	{0x338a, 0x01},

	//Gamma
	{0x3340, 0x0e},
	{0x3341, 0x1a},
	{0x3342, 0x31},
	{0x3343, 0x45},
	{0x3344, 0x5a},
	{0x3345, 0x69},
	{0x3346, 0x75},
	{0x3347, 0x7e},
	{0x3348, 0x88},
	{0x3349, 0x96},
	{0x334a, 0xa3},
	{0x334b, 0xaf},
	{0x334c, 0xc4},
	{0x334d, 0xd7},
	{0x334e, 0xe8},
	{0x334f, 0x20},

	//Lens correction
	{0x3350, 0x32},
	{0x3351, 0x25},
	{0x3352, 0x80},
	{0x3353, 0x1e},
	{0x3354, 0x00},
	{0x3355, 0x85},
	{0x3356, 0x32},
	{0x3357, 0x25},
	{0x3358, 0x80},
	{0x3359, 0x1b},
	{0x335a, 0x00},
	{0x335b, 0x85},
	{0x335c, 0x32},
	{0x335d, 0x25},
	{0x335e, 0x80},
	{0x335f, 0x1b},
	{0x3360, 0x00},
	{0x3361, 0x85},
	{0x3363, 0x70},
	{0x3364, 0x7f},
	{0x3365, 0x00},
	{0x3366, 0x00},

	//UVadjust
	{0x3301, 0xff},
	{0x338B, 0x11},
	{0x338c, 0x10},
	{0x338d, 0x40},

	//Sharpness/De-noise
	{0x3370, 0xd0},
	{0x3371, 0x00},
	{0x3372, 0x00},
	{0x3373, 0x40},
	{0x3374, 0x10},
	{0x3375, 0x10},
	{0x3376, 0x04},
	{0x3377, 0x00},
	{0x3378, 0x04},
	{0x3379, 0x80},

	//BLC
	{0x3069, 0x84},
	{0x307c, 0x10},
	{0x3087, 0x02},

	//Other functions
	{0x3300, 0xfc},
	{0x3302, 0x11},
	{0x3400, 0x02},
	{0x3606, 0x20},
	{0x3601, 0x30},
	{0x300e, 0x34},
	{0x30f3, 0x83},
	{0x304e, 0x88},

	{0x3086, 0x0f},
	{0x3086, 0x00},
};


static const struct regval_list ov2650_qcif_regs[] = {
	{0xfe,0x00}, {0x02,0x01},
	{0x2a,0xca}, {0x59,0x44},
	{0x5a,0x06}, {0x5b,0x00},
	{0x5c,0x00}, {0x5d,0x00},
	{0x5e,0x00}, {0x5f,0x00},
	{0x60,0x00}, {0x61,0x00},
	{0x62,0x00}, {0x50,0x01},
	{0x51,0x00}, {0x52,0x02},
	{0x53,0x00}, {0x54,0x0c},
	{0x55,0x00}, {0x56,0x90},
	{0x57,0x00}, {0x58,0xb0},
	ENDMARKER,
};

static const struct regval_list ov2650_cif_regs[] = {
	{0xfe,0x00}, {0x02,0x01},
	{0x2a,0xca}, {0x59,0x22},
	{0x5a,0x06}, {0x5b,0x00},
	{0x5c,0x00}, {0x5d,0x00},
	{0x5e,0x00}, {0x5f,0x00},
	{0x60,0x00}, {0x61,0x00},
	{0x62,0x00}, {0x50,0x01},
	{0x51,0x00}, {0x52,0x06},
	{0x53,0x00}, {0x54,0x18},
	{0x55,0x01}, {0x56,0x20},
	{0x57,0x01}, {0x58,0x60},
	ENDMARKER,
};

static const struct regval_list ov2650_vga_regs[] = {
	{0xfe,0x00}, {0x02,0x01},
	{0x2a,0xca}, {0x59,0x55},
	{0x5a,0x06}, {0x5b,0x00},
	{0x5c,0x00}, {0x5d,0x01},
	{0x5e,0x23}, {0x5f,0x00},
	{0x60,0x00}, {0x61,0x01},
	{0x62,0x23}, {0x50,0x01},
	{0x51,0x00}, {0x52,0x00},
	{0x53,0x00}, {0x54,0x00},
	{0x55,0x01}, {0x56,0xe0},
	{0x57,0x02}, {0x58,0x80},
	ENDMARKER,
};


static const struct regval_list ov2650_xga_regs[] = {
	{0xfe,0x00}, {0x02,0x00},
	{0x2a,0x0a}, {0x59,0x33},
	{0x5a,0x06}, {0x5b,0x00},
	{0x5c,0x00}, {0x5d,0x00},
	{0x5e,0x01}, {0x5f,0x00},
	{0x60,0x00}, {0x61,0x00},
	{0x62,0x01}, {0x50,0x01},
	{0x51,0x00}, {0x52,0x10},
	{0x53,0x00}, {0x54,0x14},
	{0x55,0x03}, {0x56,0x00},
	{0x57,0x04}, {0x58,0x00},
	ENDMARKER,
};

static const struct regval_list ov2650_uxga_regs[] = {
	{0xfe,0x00}, {0x02,0x00},
	{0x2a,0x0a}, {0x59,0x11},
	{0x5a,0x06}, {0x5b,0x00},
	{0x5c,0x00}, {0x5d,0x00},
	{0x5e,0x00}, {0x5f,0x00},
	{0x60,0x00}, {0x61,0x00},
	{0x62,0x00}, {0x50,0x01},
	{0x51,0x00}, {0x52,0x00},
	{0x53,0x00}, {0x54,0x00},
	{0x55,0x04}, {0x56,0xb0},
	{0x57,0x06}, {0x58,0x40},
	ENDMARKER,
};

static const struct regval_list ov2650_wb_auto_regs[] = {
	{0x3306,0x00},   // select Auto WB
	ENDMARKER,
};

static const struct regval_list ov2650_wb_incandescence_regs[] = {
	{0x3306, 0x02},  // Disable Auto WB, select Manual WB
	{0x3337, 0x52},
	{0x3338, 0x40},
	{0x3339, 0x58},
	ENDMARKER,
};

static const struct regval_list ov2650_wb_daylight_regs[] = {
	{0x3306, 0x02},  // Disable Auto WB, select Manual WB
	{0x3337, 0x5e},
	{0x3338, 0x40},
	{0x3339, 0x46},
	ENDMARKER,
};

static const struct regval_list ov2650_wb_fluorescent_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov2650_wb_cloud_regs[] = {
	{0x3306, 0x82},  // Disable Auto WB, select Manual WB
	{0x3337, 0x68}, //manual R G B
	{0x3338, 0x40},
	{0x3339, 0x4e},
	ENDMARKER,
};

static const struct mode_list ov2650_balance[] = {
	{0, ov2650_wb_auto_regs}, {1, ov2650_wb_incandescence_regs},
	{2, ov2650_wb_daylight_regs}, {3, ov2650_wb_fluorescent_regs},
	{4, ov2650_wb_cloud_regs},
};


static const struct regval_list ov2650_effect_normal_regs[] = {
	{ 0x3391, 0x00},
	ENDMARKER,
};

static const struct regval_list ov2650_effect_grayscale_regs[] = {
	{ 0x3391, 0x20},
	ENDMARKER,
};

static const struct regval_list ov2650_effect_sepia_regs[] = {
	{ 0x3391, 0x18},
    { 0x3396, 0x40},
    { 0x3397, 0xA6},
	ENDMARKER,
};

static const struct regval_list ov2650_effect_colorinv_regs[] = {
	{ 0x3391, 0x40},
	ENDMARKER,
};

static const struct regval_list ov2650_effect_sepiabluel_regs[] = {
	{ 0x3391, 0x18},
	{ 0x3396, 0xa0},
	{ 0x3397, 0x40},
	ENDMARKER,
};

static const struct mode_list ov2650_effect[] = {
	{0, ov2650_effect_normal_regs}, {1, ov2650_effect_grayscale_regs},
	{2, ov2650_effect_sepia_regs}, {3, ov2650_effect_colorinv_regs},
	{4, ov2650_effect_sepiabluel_regs},
};

#define OV2650_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r }

static struct ov2650_win_size ov2650_supported_win_sizes[] = {
	OV2650_SIZE("QCIF", W_QCIF, H_QCIF, ov2650_qcif_regs),
	OV2650_SIZE("CIF", W_CIF, H_CIF, ov2650_cif_regs),
	OV2650_SIZE("VGA", W_VGA, H_VGA, ov2650_vga_regs),

	OV2650_SIZE("XGA", W_XGA, H_XGA, ov2650_xga_regs),
	OV2650_SIZE("UXGA", W_UXGA, H_UXGA, ov2650_uxga_regs),
};

#define N_WIN_SIZES (ARRAY_SIZE(ov2650_supported_win_sizes))

static const struct regval_list ov2650_yuv422_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov2650_rgb565_regs[] = {
	ENDMARKER,
};

static enum v4l2_mbus_pixelcode ov2650_codes[] = {
	V4L2_MBUS_FMT_YUYV8_2X8,
	V4L2_MBUS_FMT_YUYV8_1_5X8,
	V4L2_MBUS_FMT_JZYUYV8_1_5X8,
};

/*
 * Supported controls
 */
static const struct v4l2_queryctrl ov2650_controls[] = {
	{
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.type		= V4L2_CTRL_TYPE_MENU,
		.name		= "whitebalance",
		.minimum	= 0,
		.maximum	= 4,
		.step		= 1,
		.default_value	= 0,
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.type		= V4L2_CTRL_TYPE_MENU,
		.name		= "effect",
		.minimum	= 0,
		.maximum	= 4,
		.step		= 1,
		.default_value	= 0,
	},
};


/*
 * Supported balance menus
 */
static const struct v4l2_querymenu ov2650_balance_menus[] = {
	{
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 0,
		.name		= "auto",
	}, {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 1,
		.name		= "incandescent",
	}, {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 2,
		.name		= "fluorescent",
	},  {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 3,
		.name		= "daylight",
	},  {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 4,
		.name		= "cloudy-daylight",
	},

};

/*
 * Supported effect menus
 */
static const struct v4l2_querymenu ov2650_effect_menus[] = {
	{
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 0,
		.name		= "none",
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 1,
		.name		= "mono",
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 2,
		.name		= "sepia",
	},  {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 3,
		.name		= "negative",
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 4,
		.name		= "aqua",
	},
};

/*
 * General functions
 */
static struct ov2650_priv *to_ov2650(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ov2650_priv,
			    subdev);
}

static inline int ov2659_i2c_master_send(struct i2c_client *client,
					const char *buf ,int count)
{
	int ret;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;
	ret = i2c_transfer(adap, &msg, 1);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

static inline int ov2659_i2c_master_recv(struct i2c_client *client,
					char *buf ,int count)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = buf;
	ret = i2c_transfer(adap, &msg, 1);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

static int ov2650_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	u8 msg[3];
	int ret;

    reg = cpu_to_be16(reg);

	memcpy(&msg[0], &reg, 2);
	memcpy(&msg[2], &val, 1);

	ret = ov2659_i2c_master_send(client, msg, 3);
	if (ret < 0)
		return ret;
	if (ret < 3)
		return -EIO;

	return 0;
}

static u8 ov2650_read_reg(struct i2c_client *client, u16 reg)
{
	int ret;
	u8 retval;
    u16 r = cpu_to_be16(reg);

	ret = ov2659_i2c_master_send(client, (u8 *)&r, 2);

	if (ret < 0)
		return ret;
	if (ret != 2)
		return -EIO;

	ret = ov2659_i2c_master_recv(client, &retval, 1);
	if (ret < 0)
		return ret;
	if (ret != 1)
		return -EIO;

	return retval;
}

static int ov2650_write_array(struct i2c_client *client,
			      const struct regval_list *vals)
{
	int ret;

	while ((vals->reg_num != 0xffff) || (vals->value != 0xff)) {
		ret = ov2650_write_reg(client, vals->reg_num, vals->value);
		dev_vdbg(&client->dev, "array: 0x%02x, 0x%02x",
			 vals->reg_num, vals->value);

		if (ret < 0)
			return ret;
		vals++;
	}
	return 0;
}

static int ov2650_mask_set(struct i2c_client *client,
			   u8  reg, u8  mask, u8  set)
{
	s32 val = i2c_smbus_read_byte_data(client, reg);
	if (val < 0)
		return val;

	val &= ~mask;
	val |= set & mask;

	dev_vdbg(&client->dev, "masks: 0x%02x, 0x%02x", reg, val);

	return i2c_smbus_write_byte_data(client, reg, val);
}

/*
 * soc_camera_ops functions
 */
static int ov2650_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int ov2650_set_bus_param(struct soc_camera_device *icd,
				unsigned long flags)
{
	struct soc_camera_link *icl = to_soc_camera_link(icd);
	unsigned long width_flag = flags & SOCAM_DATAWIDTH_MASK;

	/* Only one width bit may be set */
	if (!is_power_of_2(width_flag))
		return -EINVAL;

	if (icl->set_bus_param)
		return icl->set_bus_param(icl, width_flag);

	/*
	 * Without board specific bus width settings we support only the
	 * sensors native bus width which are tested working
	 */
	if (width_flag & SOCAM_DATAWIDTH_8)
		return 0;

	return 0;
}

static unsigned long ov2650_query_bus_param(struct soc_camera_device *icd)
{
	struct soc_camera_link *icl = to_soc_camera_link(icd);
	unsigned long flags = SOCAM_PCLK_SAMPLE_FALLING | SOCAM_MASTER |
		SOCAM_VSYNC_ACTIVE_HIGH | SOCAM_HSYNC_ACTIVE_HIGH |
		SOCAM_DATA_ACTIVE_HIGH;

	if (icl->query_bus_param)
		flags |= icl->query_bus_param(icl) & SOCAM_DATAWIDTH_MASK;
	else
		flags |= SOCAM_DATAWIDTH_8;

	return soc_camera_apply_sensor_flags(icl, flags);
}

static int ov2650_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov2650_priv *priv = to_ov2650(client);

	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		ctrl->value = priv->flag_vflip;
		break;
	case V4L2_CID_HFLIP:
		ctrl->value = priv->flag_hflip;
		break;
	case V4L2_CID_PRIVATE_BALANCE:
		ctrl->value = priv->balance_value;
		break;
	case V4L2_CID_PRIVATE_EFFECT:
		ctrl->value = priv->effect_value;
		break;
	default:
		break;
	}
	return 0;
}

static int ov2650_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov2650_priv *priv = to_ov2650(client);
	int ret = 0;
	int i = 0;
	u8 value;

	int balance_count = ARRAY_SIZE(ov2650_balance);
	int effect_count = ARRAY_SIZE(ov2650_effect);

	switch (ctrl->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		if(ctrl->value > balance_count)
			return -EINVAL;

		for(i = 0; i < balance_count; i++) {
			if(ctrl->value == ov2650_balance[i].index) {
				ret = ov2650_write_array(client,
						ov2650_balance[ctrl->value].mode_regs);
				priv->balance_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		if(ctrl->value > effect_count)
			return -EINVAL;

		for(i = 0; i < effect_count; i++) {
			if(ctrl->value == ov2650_effect[i].index) {
				ret = ov2650_write_array(client,
						ov2650_effect[ctrl->value].mode_regs);
				priv->effect_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_VFLIP:
		value = ctrl->value ? REG29_VFLIP_IMG : 0x00;
		priv->flag_vflip = ctrl->value ? 1 : 0;
		ret = ov2650_mask_set(client, REG29, REG29_VFLIP_IMG, value);
		break;

	case V4L2_CID_HFLIP:
		value = ctrl->value ? REG29_HFLIP_IMG : 0x00;
		priv->flag_hflip = ctrl->value ? 1 : 0;
		ret = ov2650_mask_set(client, REG29, REG29_HFLIP_IMG, value);
		break;

	default:
		dev_err(&client->dev, "no V4L2 CID: 0x%x ", ctrl->id);
		return -EINVAL;
	}

	return ret;
}

static int ov2650_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *id)
{
	id->ident    = SUPPORT_HIGH_RESOLUTION_PRE;
	id->revision = 0;

	return 0;
}


static int ov2650_querymenu(struct v4l2_subdev *sd,
					struct v4l2_querymenu *qm)
{
	switch (qm->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		memcpy(qm->name, ov2650_balance_menus[qm->index].name,
				sizeof(qm->name));
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		memcpy(qm->name, ov2650_effect_menus[qm->index].name,
				sizeof(qm->name));
		break;
	}

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov2650_g_register(struct v4l2_subdev *sd,
			     struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	reg->size = 1;
	if (reg->reg > 0xff)
		return -EINVAL;

	ret = i2c_smbus_read_byte_data(client, reg->reg);
	if (ret < 0)
		return ret;

	reg->val = ret;

	return 0;
}

static int ov2650_s_register(struct v4l2_subdev *sd,
			     struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (reg->reg > 0xff ||
	    reg->val > 0xff)
		return -EINVAL;

	return i2c_smbus_write_byte_data(client, reg->reg, reg->val);
}
#endif

/* Select the nearest higher resolution for capture */
static const struct ov2650_win_size *ov2650_select_win(u32 *width, u32 *height)
{
	int i, default_size = ARRAY_SIZE(ov2650_supported_win_sizes) - 1;

	for (i = 0; i < ARRAY_SIZE(ov2650_supported_win_sizes); i++) {
		if (ov2650_supported_win_sizes[i].width  >= *width &&
		    ov2650_supported_win_sizes[i].height >= *height) {
			*width = ov2650_supported_win_sizes[i].width;
			*height = ov2650_supported_win_sizes[i].height;
			return &ov2650_supported_win_sizes[i];
		}
	}

	*width = ov2650_supported_win_sizes[default_size].width;
	*height = ov2650_supported_win_sizes[default_size].height;
	return &ov2650_supported_win_sizes[default_size];
}

static int ov2650_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

    for (i = 0; i < ARRAY_SIZE(ov2650_init_regs); i++) {
        if (SENSOR_WRITE_DELAY == ov2650_init_regs[i][0]) {
            mdelay(ov2650_init_regs[i][1]);
        } else {
            ret = ov2650_write_reg(client, ov2650_init_regs[i][0], ov2650_init_regs[i][1]);
        }
    }

    if(ret) {
    	dev_err(&client->dev, "sensor init failed!\n");
    	return -1;
    }

    dev_info(&client->dev, "sensor init ok\n");
	return 0;
}

#if 0
static int ov2650_set_params(struct i2c_client *client, u32 *width, u32 *height,
			     enum v4l2_mbus_pixelcode code)
{
	struct ov2650_priv *priv = to_ov2650(client);
	const struct regval_list *selected_cfmt_regs;
	int ret;

	/* select win */
	priv->win = ov2650_select_win(width, height);

	/* select format */
	priv->cfmt_code = 0;
	switch (code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
		dev_dbg(&client->dev, "%s: Selected cfmt YUV422", __func__);
		selected_cfmt_regs = ov2650_rgb565_regs;
		break;
	default:
	case V4L2_MBUS_FMT_YUYV8_1_5X8:
		dev_dbg(&client->dev, "%s: Selected cfmt YUV422", __func__);
		selected_cfmt_regs = ov2650_yuv422_regs;
		break;
	}

	/* initialize the sensor with default data */
	dev_info(&client->dev, "%s: Init default", __func__);
	ret = ov2650_write_array(client, ov2650_init_regs);
	if (ret < 0)
		goto err;

	/* set size win */
	ret = ov2650_write_array(client, priv->win->regs);
	if (ret < 0)
		goto err;

	priv->cfmt_code = code;
	*width = priv->win->width;
	*height = priv->win->height;

	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	priv->win = NULL;

	return ret;
}
#endif

static int ov2650_g_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov2650_priv *priv = to_ov2650(client);

	mf->width = priv->win->width;
	mf->height = priv->win->height;
	mf->code = priv->cfmt_code;

	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field	= V4L2_FIELD_NONE;

	return 0;
}

static int ov2650_s_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	/* current do not support set format, use unify format yuv422i */
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov2650_priv *priv = to_ov2650(client);
	int ret;

	priv->win = ov2650_select_win(&mf->width, &mf->height);
	/* set size win */
	ret = ov2650_write_array(client, priv->win->regs);
	if (ret < 0) {
		dev_err(&client->dev, "%s: Error\n", __func__);
		return ret;
	}
	return 0;
}

static int ov2650_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *mf)
{
	const struct ov2650_win_size *win;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	/*
	 * select suitable win
	 */
	win = ov2650_select_win(&mf->width, &mf->height);

	if(mf->field == V4L2_FIELD_ANY) {
		mf->field = V4L2_FIELD_NONE;
	} else if (mf->field != V4L2_FIELD_NONE) {
		dev_err(&client->dev, "Field type invalid.\n");
		return -ENODEV;
	}

	switch (mf->code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
	case V4L2_MBUS_FMT_YUYV8_1_5X8:
	case V4L2_MBUS_FMT_JZYUYV8_1_5X8:
		mf->colorspace = V4L2_COLORSPACE_JPEG;
		break;

	default:
		mf->code = V4L2_MBUS_FMT_YUYV8_2X8;
		break;
	}

	return 0;
}


static int ov2650_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= ARRAY_SIZE(ov2650_codes))
		return -EINVAL;

	*code = ov2650_codes[index];
	return 0;
}

static int ov2650_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	return 0;
}

static int ov2650_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	return 0;
}


/*
 * Frame intervals.  Since frame rates are controlled with the clock
 * divider, we can only do 30/n for integer n values.  So no continuous
 * or stepwise options.  Here we just pick a handful of logical values.
 */

static int ov2650_frame_rates[] = { 30, 15, 10, 5, 1 };

static int ov2650_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	if (interval->index >= ARRAY_SIZE(ov2650_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = ov2650_frame_rates[interval->index];
	return 0;
}


/*
 * Frame size enumeration
 */
static int ov2650_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	for (i = 0; i < N_WIN_SIZES; i++) {
		struct ov2650_win_size *win = &ov2650_supported_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int ov2650_video_probe(struct soc_camera_device *icd,
			      struct i2c_client *client)
{
	unsigned short retval;
	/*
	 * we must have a parent by now. And it cannot be a wrong one.
	 * So this entire test is completely redundant.
	 */
	if (!icd->dev.parent ||
	    to_soc_camera_host(icd->dev.parent)->nr != icd->iface) {
		dev_err(&client->dev, "Parent missing or invalid!\n");
		return -ENODEV;
	}

	/*
	 * check and show product ID and manufacturer ID
	 */
	retval = ov2650_read_reg(client, 0x300a);
	if(retval == REG_CHIP_ID) {
		dev_info(&client->dev, "read sensor %s id ok\n",
				client->name);
		return 0;
	}

	dev_err(&client->dev, "read sensor %s id: 0x%x, failed!\n",
			client->name, retval);
	return -1;
}

static struct soc_camera_ops ov2650_ops = {
	.set_bus_param		= ov2650_set_bus_param,
	.query_bus_param	= ov2650_query_bus_param,
	.controls		= ov2650_controls,
	.num_controls		= ARRAY_SIZE(ov2650_controls),
};

static struct v4l2_subdev_core_ops ov2650_subdev_core_ops = {
	.init 		= ov2650_init,
	.g_ctrl		= ov2650_g_ctrl,
	.s_ctrl		= ov2650_s_ctrl,
	.g_chip_ident	= ov2650_g_chip_ident,
	.querymenu  = ov2650_querymenu,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register	= ov2650_g_register,
	.s_register	= ov2650_s_register,
#endif
};

static struct v4l2_subdev_video_ops ov2650_subdev_video_ops = {
	.s_stream	= ov2650_s_stream,
	.g_mbus_fmt	= ov2650_g_fmt,
	.s_mbus_fmt	= ov2650_s_fmt,
	.try_mbus_fmt	= ov2650_try_fmt,
	.cropcap	= ov2650_cropcap,
	.g_crop		= ov2650_g_crop,
	.enum_mbus_fmt	= ov2650_enum_fmt,
	.enum_framesizes = ov2650_enum_framesizes,
	.enum_frameintervals = ov2650_enum_frameintervals,
};

static struct v4l2_subdev_ops ov2650_subdev_ops = {
	.core	= &ov2650_subdev_core_ops,
	.video	= &ov2650_subdev_video_ops,
};

/*
 * i2c_driver functions
 */

static int ov2650_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct ov2650_priv *priv;
	struct soc_camera_device *icd = client->dev.platform_data;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct soc_camera_link *icl;
	int ret = 0;

	if(!strcmp(client->name, "ov2650-back")) {
		client->addr -= 1;
	}

	if (!icd) {
		dev_err(&adapter->dev, "OV2650: missing soc-camera data!\n");
		return -EINVAL;
	}

	icl = to_soc_camera_link(icd);
	if (!icl) {
		dev_err(&adapter->dev,
			"OV2650: Missing platform_data for driver\n");
		return -EINVAL;
	}

	priv = kzalloc(sizeof(struct ov2650_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&adapter->dev,
			"Failed to allocate memory for private data!\n");
		return -ENOMEM;
	}

	priv->info = icl->priv;

	v4l2_i2c_subdev_init(&priv->subdev, client, &ov2650_subdev_ops);

	icd->ops = &ov2650_ops;

	ret = ov2650_video_probe(icd, client);
	if (ret) {
		icd->ops = NULL;
		kfree(priv);
	}
	return ret;
}

static int ov2650_remove(struct i2c_client *client)
{
	struct ov2650_priv       *priv = to_ov2650(client);
	struct soc_camera_device *icd = client->dev.platform_data;

	icd->ops = NULL;
	kfree(priv);
	return 0;
}

static const struct i2c_device_id ov2650_id[] = {
	{ "ov2650-front",  0 },
	{ "ov2650-back",  0 },
	{ }
};


MODULE_DEVICE_TABLE(i2c, ov2650_id);

static struct i2c_driver ov2650_i2c_driver = {
	.driver = {
		.name = "ov2650",
	},
	.probe    = ov2650_probe,
	.remove   = ov2650_remove,
	.id_table = ov2650_id,
};

/*
 * Module functions
 */
static int __init ov2650_module_init(void)
{
	return i2c_add_driver(&ov2650_i2c_driver);
}

static void __exit ov2650_module_exit(void)
{
	i2c_del_driver(&ov2650_i2c_driver);
}

module_init(ov2650_module_init);
module_exit(ov2650_module_exit);

MODULE_DESCRIPTION("camera sensor ov2650 driver");
MODULE_LICENSE("GPL");
