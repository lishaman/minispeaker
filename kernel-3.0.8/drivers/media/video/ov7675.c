/*
 * V4L2 Driver for camera sensor ov7675
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


#define REG_CHIP_ID		0x0a
#define PID_OV7675		0x76

/* Private v4l2 controls */
#define V4L2_CID_PRIVATE_BALANCE  (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PRIVATE_EFFECT  (V4L2_CID_PRIVATE_BASE + 1)

#define REG14				0x14
#define REG14_HFLIP_IMG		0x01 /* Horizontal mirror image ON/OFF */
#define REG14_VFLIP_IMG     0x02 /* Vertical flip image ON/OFF */

 /* whether sensor support high resolution (> vga) preview or not */
#define SUPPORT_HIGH_RESOLUTION_PRE		0

/*
 * Struct
 */
struct regval_list {
	u8 reg_num;
	u8 value;
};

struct mode_list {
	u8 index;
	const struct regval_list *mode_regs;
};

/* Supported resolutions */
enum ov7675_width {
	W_QCIF	= 176,
	W_QVGA	= 320,
	W_CIF	= 352,
	W_VGA	= 640,
};

enum ov7675_height {
	H_QCIF	= 144,
	H_QVGA	= 240,
	H_CIF	= 288,
	H_VGA	= 480,
};

struct ov7675_win_size {
	char *name;
	enum ov7675_width width;
	enum ov7675_height height;
	const struct regval_list *regs;
};


struct ov7675_priv {
	struct v4l2_subdev subdev;
	struct ov7675_camera_info *info;
	enum v4l2_mbus_pixelcode cfmt_code;
	const struct ov7675_win_size *win;
	int	model;
	u8 balance_value;
	u8 effect_value;
	u16	flag_vflip:1;
	u16	flag_hflip:1;
};

/*
 * Registers settings
 */

#define ENDMARKER { 0xff, 0xff }

static const struct regval_list ov7675_init_regs[] = {
	{0x12, 0x80},
//	{0x09, 0x10},
	{0xc1, 0x7f},
	{0x11, 0x00},
	{0x3a, 0x0c},
	{0x3d, 0xc0},
	{0x12, 0x00},
	{0x15, 0x02},//42
	{0x17, 0x13},//13
	{0x18, 0x01},//01
	{0x32, 0xbf},
	{0x19, 0x02},
	{0x1a, 0x7a},
	{0x03, 0x0a},
	{0x0c, 0x00},
	{0x3e, 0x00},
	{0x70, 0x3a},
	{0x71, 0x35},
	{0x72, 0x11},
	{0x73, 0xf0},
	{0xa2, 0x02},

//	{0x1b, 0x40},
	//gamma
	{0x7a, 0x20},
	{0x7b, 0x03},
	{0x7c, 0x0a},
	{0x7d, 0x1a},
	{0x7e, 0x3f},
	{0x7f, 0x4e},
	{0x80, 0x5b},
	{0x81, 0x68},
	{0x82, 0x75},
	{0x83, 0x7f},
	{0x84, 0x89},
	{0x85, 0x9a},
	{0x86, 0xa6},
	{0x87, 0xbd},
	{0x88, 0xd3},
	{0x89, 0xe8},
	{0x13, 0xe0},
	{0x00, 0x00},
	{0x10, 0x00},
	{0x0d, 0x40},
	{0x14, 0x28},
	{0xa5, 0x02},
	{0xab, 0x02},
	//bright
	{0x24, 0x68},
	{0x25, 0x58},
	{0x26, 0xc2},
	{0x9f, 0x78},
	{0xa0, 0x68},
	{0xa1, 0x03},
	{0xa6, 0xd8},
	{0xa7, 0xd8},
	{0xa8, 0xf0},
	{0xa9, 0x90},
	{0xaa, 0x14},
	{0x13, 0xe5},
	{0x0e, 0x61},
	{0x0f, 0x4b},
	{0x16, 0x02},
	{0x1e, 0x07},
	{0x21, 0x02},
	{0x22, 0x91},
	{0x29, 0x07},
	{0x33, 0x0b},
	{0x35, 0x0b},
	{0x37, 0x1d},
	{0x38, 0x71},
	{0x39, 0x2a},
	{0x3c, 0x78},
	{0x4d, 0x40},
	{0x4e, 0x20},
	{0x69, 0x00},
	{0x6b, 0x0a},
	{0x74, 0x10},
	{0x8d, 0x4f},
	{0x8e, 0x00},
	{0x8f, 0x00},
	{0x90, 0x00},
	{0x91, 0x00},
	{0x96, 0x00},
	{0x9a, 0x80},
	{0xb0, 0x84},
	{0xb1, 0x0c},
	{0xb2, 0x0e},
	{0xb3, 0x82},
	{0xb8, 0x0a},
	{0x43, 0x0a},
	{0x44, 0xf2},
	{0x45, 0x39},
	{0x46, 0x62},
	{0x47, 0x3d},
	{0x48, 0x55},
	{0x59, 0x83},
	{0x5a, 0x0d},
	{0x5b, 0xcd},
	{0x5c, 0x8c},
	{0x5d, 0x77},
	{0x5e, 0x16},
	{0x6c, 0x0a},
	{0x6d, 0x65},
	{0x6e, 0x11},
	{0x6a, 0x40},
	{0x01, 0x56},
	{0x02, 0x44},
	{0x13, 0xe7}, //8f
	//	dprintk("-------------------------------ov7675 init 2"},
	//cmx
	{0x4f, 0x88},
	{0x50, 0x8b},
	{0x51, 0x04},
	{0x52, 0x11},
	{0x53, 0x8c},
	{0x54, 0x9d},
	{0x55, 0x98},
	{0x56, 0x40},
	{0x57, 0x80},
	{0x58, 0x9a},
	//sharpness
	{0x41, 0x08},
	{0x3f, 0x00},
	{0x75, 0x23},
	{0x76, 0xe1},
	{0x4c, 0x00},
	{0x77, 0x01},
	{0x3d, 0xc2},
	{0x4b, 0x09},
	{0xc9, 0x30},
	{0x41, 0x38},

	{0x56, 0x40},
	{0x34, 0x11},
	{0x3b, 0x00},
	{0xa4, 0x88},
	{0x96, 0x00},
	{0x97, 0x30},
	{0x98, 0x20},
	{0x99, 0x30},
	{0x9a, 0x84},
	{0x9b, 0x29},
	{0x9c, 0x03},
	{0x9d, 0x99},
	{0x9e, 0x7f},
	{0x78, 0x04},
	{0x79, 0x01},
	{0xc8, 0xf0},
	{0x79, 0x0f},
	{0xc8, 0x00},
	{0x79, 0x10},
	{0xc8, 0x7e},
	{0x79, 0x0a},
	{0xc8, 0x80},
	{0x79, 0x0b},
	{0xc8, 0x01},
	{0x79, 0x0c},
	{0xc8, 0x0f},
	{0x79, 0x0d},
	{0xc8, 0x20},
	{0x79, 0x09},
	{0xc8, 0x80},
	{0x79, 0x02},
	{0xc8, 0xc0},
	{0x79, 0x03},
	{0xc8, 0x40},
	{0x79, 0x05},
	{0xc8, 0x30},
	{0x79, 0x26},
	{0x94, 0x05},
	{0x95, 0x09},
//	{0x3a, 0x0d},
//	{0x3d, 0xc3},
//	{0x19, 0x03},
//	{0x1a, 0x7b},
	{0x2a, 0x00},
	{0x2b, 0x00},
//	{0x18, 0x01},
	//lens
	{0x66, 0x05},
	{0x62, 0x10},
	{0x63, 0x0b},
	{0x65, 0x07},
	{0x64, 0x0f},
	{0x94, 0x0e},
	{0x95, 0x10},
	{0x09, 0x00},


	ENDMARKER,
};


static const struct regval_list ov7675_qcif_regs[] = {
	{0x92,0x88}, {0x93,0x00},
	{0xb9,0x30}, {0x19,0x02},
	{0x1a,0x3e}, {0x17,0x13},
	{0x18,0x3b}, {0x03,0x0a},
	{0xe6,0x05}, {0xd2,0x1c},

	{0x19,0x17}, {0x1a,0x3b},
	{0x17,0x1d}, {0x18,0x33},
	{0x03,0x0a}, {0x32,0xbf},
	ENDMARKER,
};

static const struct regval_list ov7675_qvga_regs[] = {
	{0x92,0x88}, {0x93,0x00},
	{0xb9,0x30}, {0x19,0x02},
	{0x1a,0x3e}, {0x17,0x13},
	{0x18,0x3b}, {0x03,0x0a},
	{0xe6,0x05}, {0xd2,0x1c},
	ENDMARKER,
};

static const struct regval_list ov7675_cif_regs[] = {
	{0x92,0x00}, {0x93,0x00},
	{0xb9,0x00}, {0x19,0x02},
	{0x1a,0x7a}, {0x17,0x13},
	{0x18,0x01}, {0x03,0x0a},
	{0xe6,0x00}, {0xd2,0x1c},

	{0x19,0x17}, {0x1a,0x5f},
	{0x17,0x1d}, {0x18,0x49},
	{0x03,0x0a}, {0x32,0xbf},
	ENDMARKER,
};

static const struct regval_list ov7675_vga_regs[] = {
	{0x92,0x00}, {0x93,0x00},
	{0xb9,0x00}, {0x19,0x03},
	{0x1a,0x7b}, {0x17,0x13},
	{0x18,0x01}, {0x03,0x0a},
	{0xe6,0x00}, {0xd2,0x1c},
	ENDMARKER,
};

static const struct regval_list ov7675_wb_auto_regs[] = {
	{0x01,0x56}, {0x02,0x44},
	ENDMARKER,
};

static const struct regval_list ov7675_wb_incandescence_regs[] = {
	{0x01,0x9a}, {0x02,0x40},
	{0x6a,0x48},
	ENDMARKER,
};

static const struct regval_list ov7675_wb_daylight_regs[] = {
	{0x01,0x56}, {0x02,0x5c},
	{0x6a,0x42},
	ENDMARKER,
};

static const struct regval_list ov7675_wb_fluorescent_regs[] = {
	{0x01,0x8b}, {0x02,0x42},
	{0x6a,0x40},
	ENDMARKER,
};

static const struct regval_list ov7675_wb_cloud_regs[] = {
	{0x01,0x58}, {0x02,0x60},
	{0x6a,0x40},
	ENDMARKER,
};

static const struct mode_list ov7675_balance[] = {
	{0, ov7675_wb_auto_regs}, {1, ov7675_wb_incandescence_regs},
	{2, ov7675_wb_daylight_regs}, {3, ov7675_wb_fluorescent_regs},
	{4, ov7675_wb_cloud_regs},
};


static const struct regval_list ov7675_effect_normal_regs[] = {
	{0x3A,0x0C}, {0x67,0x80},
   	{0x68,0x80}, {0x56,0x40},
	ENDMARKER,
};

static const struct regval_list ov7675_effect_grayscale_regs[] = {
	{0x3A,0x1C}, {0x67,0x80},
   	{0x68,0x80}, {0x56,0x40},
	ENDMARKER,
};

static const struct regval_list ov7675_effect_sepia_regs[] = {
    {0x3A,0x1C}, {0x67,0x40},
    {0x68,0xa0}, {0x56,0x40},
	ENDMARKER,
};

static const struct regval_list ov7675_effect_colorinv_regs[] = {
    {0x3A,0x2C}, {0x67,0x80},
    {0x68,0x80}, {0x56,0x40},
	ENDMARKER,
};

static const struct regval_list ov7675_effect_sepiabluel_regs[] = {
    {0x3A,0x1C}, {0x67,0xc0},
    {0x68,0x80}, {0x56,0x40},
	ENDMARKER,
};

static const struct mode_list ov7675_effect[] = {
	{0, ov7675_effect_normal_regs}, {1, ov7675_effect_grayscale_regs},
	{2, ov7675_effect_sepia_regs}, {3, ov7675_effect_colorinv_regs},
	{4, ov7675_effect_sepiabluel_regs},
};

#define OV7675_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r }

static struct ov7675_win_size ov7675_supported_win_sizes[] = {
	OV7675_SIZE("QCIF", W_QCIF, H_QCIF, ov7675_qcif_regs),
	OV7675_SIZE("QVGA", W_QVGA, H_QVGA, ov7675_qvga_regs),
	OV7675_SIZE("CIF", W_CIF, H_CIF, ov7675_cif_regs),
	OV7675_SIZE("VGA", W_VGA, H_VGA, ov7675_vga_regs),
};

#define N_WIN_SIZES (ARRAY_SIZE(ov7675_supported_win_sizes))

static const struct regval_list ov7675_yuv422_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov7675_rgb565_regs[] = {
	ENDMARKER,
};

static enum v4l2_mbus_pixelcode ov7675_codes[] = {
	V4L2_MBUS_FMT_YUYV8_2X8,
	V4L2_MBUS_FMT_YUYV8_1_5X8,
	V4L2_MBUS_FMT_JZYUYV8_1_5X8,
};

/*
 * Supported controls
 */
static const struct v4l2_queryctrl ov7675_controls[] = {
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
	{
		.id		= V4L2_CID_VFLIP,
		.type		= V4L2_CTRL_TYPE_BOOLEAN,
		.name		= "Flip Vertically",
		.minimum	= 0,
		.maximum	= 1,
		.step		= 1,
		.default_value	= 0,
	}, {
		.id		= V4L2_CID_HFLIP,
		.type		= V4L2_CTRL_TYPE_BOOLEAN,
		.name		= "Flip Horizontally",
		.minimum	= 0,
		.maximum	= 1,
		.step		= 1,
		.default_value	= 0,
	},
};


/*
 * Supported balance menus
 */
static const struct v4l2_querymenu ov7675_balance_menus[] = {
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
static const struct v4l2_querymenu ov7675_effect_menus[] = {
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
static struct ov7675_priv *to_ov7675(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ov7675_priv,
			    subdev);
}

static int ov7675_write_array(struct i2c_client *client,
			      const struct regval_list *vals)
{
	int ret;

	while ((vals->reg_num != 0xff) || (vals->value != 0xff)) {
		ret = i2c_smbus_write_byte_data(client,
						vals->reg_num, vals->value);
		dev_vdbg(&client->dev, "array: 0x%02x, 0x%02x",
			 vals->reg_num, vals->value);

		if (ret < 0)
			return ret;
		vals++;
	}
	return 0;
}


static int ov7675_mask_set(struct i2c_client *client,
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

/* current use hardware reset */
#if 0
static int ov7675_reset(struct i2c_client *client)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, 0xfe, 0x80);
	if (ret)
		goto err;

	msleep(5);
err:
	dev_dbg(&client->dev, "%s: (ret %d)", __func__, ret);
	return ret;
}
#endif

/*
 * soc_camera_ops functions
 */
static int ov7675_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int ov7675_set_bus_param(struct soc_camera_device *icd,
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

static unsigned long ov7675_query_bus_param(struct soc_camera_device *icd)
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

static int ov7675_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov7675_priv *priv = to_ov7675(client);

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

/* FIXME: Flip function should be update according to specific sensor */
static int ov7675_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov7675_priv *priv = to_ov7675(client);
	int ret = 0;
	int i = 0;
	u8 value;

	int balance_count = ARRAY_SIZE(ov7675_balance);
	int effect_count = ARRAY_SIZE(ov7675_effect);

	switch (ctrl->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		if(ctrl->value > balance_count)
			return -EINVAL;

		for(i = 0; i < balance_count; i++) {
			if(ctrl->value == ov7675_balance[i].index) {
				ret = ov7675_write_array(client,
						ov7675_balance[ctrl->value].mode_regs);
				priv->balance_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		if(ctrl->value > effect_count)
			return -EINVAL;

		for(i = 0; i < effect_count; i++) {
			if(ctrl->value == ov7675_effect[i].index) {
				ret = ov7675_write_array(client,
						ov7675_effect[ctrl->value].mode_regs);
				priv->effect_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_VFLIP:
		value = ctrl->value ? REG14_VFLIP_IMG : 0x00;
		priv->flag_vflip = ctrl->value ? 1 : 0;
		ret = ov7675_mask_set(client, REG14, REG14_VFLIP_IMG, value);
		break;

	case V4L2_CID_HFLIP:
		value = ctrl->value ? REG14_HFLIP_IMG : 0x00;
		priv->flag_hflip = ctrl->value ? 1 : 0;
		ret = ov7675_mask_set(client, REG14, REG14_HFLIP_IMG, value);
		break;

	default:
		dev_err(&client->dev, "no V4L2 CID: 0x%x ", ctrl->id);
		return -EINVAL;
	}

	return ret;
}

static int ov7675_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *id)
{
	id->ident    = SUPPORT_HIGH_RESOLUTION_PRE;
	id->revision = 0;

	return 0;
}


static int ov7675_querymenu(struct v4l2_subdev *sd,
					struct v4l2_querymenu *qm)
{
	switch (qm->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		memcpy(qm->name, ov7675_balance_menus[qm->index].name,
				sizeof(qm->name));
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		memcpy(qm->name, ov7675_effect_menus[qm->index].name,
				sizeof(qm->name));
		break;
	}

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov7675_g_register(struct v4l2_subdev *sd,
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

static int ov7675_s_register(struct v4l2_subdev *sd,
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
static const struct ov7675_win_size *ov7675_select_win(u32 *width, u32 *height)
{
	int i, default_size = ARRAY_SIZE(ov7675_supported_win_sizes) - 1;

	for (i = 0; i < ARRAY_SIZE(ov7675_supported_win_sizes); i++) {
		if (ov7675_supported_win_sizes[i].width  >= *width &&
		    ov7675_supported_win_sizes[i].height >= *height) {
			*width = ov7675_supported_win_sizes[i].width;
			*height = ov7675_supported_win_sizes[i].height;
			return &ov7675_supported_win_sizes[i];
		}
	}

	*width = ov7675_supported_win_sizes[default_size].width;
	*height = ov7675_supported_win_sizes[default_size].height;
	return &ov7675_supported_win_sizes[default_size];
}

static int ov7675_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov7675_priv *priv = to_ov7675(client);

	int bala_index = priv->balance_value;
	int effe_index = priv->effect_value;

	/* initialize the sensor with default data */
	ret = ov7675_write_array(client, ov7675_init_regs);
	ret = ov7675_write_array(client, ov7675_balance[bala_index].mode_regs);
	ret = ov7675_write_array(client, ov7675_effect[effe_index].mode_regs);
	if (ret < 0)
		goto err;

	dev_info(&client->dev, "%s: Init default", __func__);
	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	return ret;
}

#if 0
static int ov7675_set_params(struct i2c_client *client, u32 *width, u32 *height,
			     enum v4l2_mbus_pixelcode code)
{
	struct ov7675_priv *priv = to_ov7675(client);
	const struct regval_list *selected_cfmt_regs;
	int ret;

	/* select win */
	priv->win = ov7675_select_win(width, height);

	/* select format */
	priv->cfmt_code = 0;
	switch (code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
		dev_dbg(&client->dev, "%s: Selected cfmt YUV422", __func__);
		selected_cfmt_regs = ov7675_rgb565_regs;
		break;
	default:
	case V4L2_MBUS_FMT_YUYV8_1_5X8:
		dev_dbg(&client->dev, "%s: Selected cfmt YUV422", __func__);
		selected_cfmt_regs = ov7675_yuv422_regs;
		break;
	}

	/* initialize the sensor with default data */
	dev_info(&client->dev, "%s: Init default", __func__);
	ret = ov7675_write_array(client, ov7675_init_regs);
	if (ret < 0)
		goto err;

	/* set size win */
	ret = ov7675_write_array(client, priv->win->regs);
	if (ret < 0)
		goto err;

	priv->cfmt_code = code;
	*width = priv->win->width;
	*height = priv->win->height;

	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	ov7675_reset(client);
	priv->win = NULL;

	return ret;
}
#endif

static int ov7675_g_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov7675_priv *priv = to_ov7675(client);

	mf->width = priv->win->width;
	mf->height = priv->win->height;
	mf->code = priv->cfmt_code;

	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field	= V4L2_FIELD_NONE;

	return 0;
}

static int ov7675_s_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	/* current do not support set format, use unify format yuv422i */
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov7675_priv *priv = to_ov7675(client);
	int ret;

	priv->win = ov7675_select_win(&mf->width, &mf->height);
	/* set size win */
	ret = ov7675_write_array(client, priv->win->regs);
	if (ret < 0) {
		dev_err(&client->dev, "%s: Error\n", __func__);
		return ret;
	}
	return 0;
}

static int ov7675_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *mf)
{
	const struct ov7675_win_size *win;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	/*
	 * select suitable win
	 */
	win = ov7675_select_win(&mf->width, &mf->height);

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


static int ov7675_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= ARRAY_SIZE(ov7675_codes))
		return -EINVAL;

	*code = ov7675_codes[index];
	return 0;
}

static int ov7675_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	return 0;
}

static int ov7675_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	return 0;
}


/*
 * Frame intervals.  Since frame rates are controlled with the clock
 * divider, we can only do 30/n for integer n values.  So no continuous
 * or stepwise options.  Here we just pick a handful of logical values.
 */

static int ov7675_frame_rates[] = { 30, 15, 10, 5, 1 };

static int ov7675_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	if (interval->index >= ARRAY_SIZE(ov7675_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = ov7675_frame_rates[interval->index];
	return 0;
}


/*
 * Frame size enumeration
 */
static int ov7675_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	for (i = 0; i < N_WIN_SIZES; i++) {
		struct ov7675_win_size *win = &ov7675_supported_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int ov7675_video_probe(struct soc_camera_device *icd,
			      struct i2c_client *client)
{
	unsigned char retval = 0;
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
	retval = i2c_smbus_write_byte_data(client, 0x12, 0x80);
	if(retval) {
		dev_err(&client->dev, "i2c write failed!\n");
		return -1;	
	}

	retval = i2c_smbus_read_byte_data(client, REG_CHIP_ID);
	if(retval == PID_OV7675) {
		dev_info(&client->dev, "read sensor %s id ok\n",
				client->name);
		return 0;
	}

	dev_err(&client->dev, "read sensor %s id: 0x%x, failed!\n",
			client->name, retval);
	return -1;
}

static struct soc_camera_ops ov7675_ops = {
	.set_bus_param		= ov7675_set_bus_param,
	.query_bus_param	= ov7675_query_bus_param,
	.controls		= ov7675_controls,
	.num_controls		= ARRAY_SIZE(ov7675_controls),
};

static struct v4l2_subdev_core_ops ov7675_subdev_core_ops = {
	.init 		= ov7675_init,
	.g_ctrl		= ov7675_g_ctrl,
	.s_ctrl		= ov7675_s_ctrl,
	.g_chip_ident	= ov7675_g_chip_ident,
	.querymenu  = ov7675_querymenu,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register	= ov7675_g_register,
	.s_register	= ov7675_s_register,
#endif
};

static struct v4l2_subdev_video_ops ov7675_subdev_video_ops = {
	.s_stream	= ov7675_s_stream,
	.g_mbus_fmt	= ov7675_g_fmt,
	.s_mbus_fmt	= ov7675_s_fmt,
	.try_mbus_fmt	= ov7675_try_fmt,
	.cropcap	= ov7675_cropcap,
	.g_crop		= ov7675_g_crop,
	.enum_mbus_fmt	= ov7675_enum_fmt,
	.enum_framesizes = ov7675_enum_framesizes,
	.enum_frameintervals = ov7675_enum_frameintervals,
};

static struct v4l2_subdev_ops ov7675_subdev_ops = {
	.core	= &ov7675_subdev_core_ops,
	.video	= &ov7675_subdev_video_ops,
};

/*
 * i2c_driver functions
 */

static int ov7675_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct ov7675_priv *priv;
	struct soc_camera_device *icd = client->dev.platform_data;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct soc_camera_link *icl;
	int ret = 0;

	if(!strcmp(client->name, "ov7675-back")) {
		client->addr -= 1;
	}

	if (!icd) {
		dev_err(&adapter->dev, "OV7675: missing soc-camera data!\n");
		return -EINVAL;
	}

	icl = to_soc_camera_link(icd);
	if (!icl) {
		dev_err(&adapter->dev,
			"OV7675: Missing platform_data for driver\n");
		return -EINVAL;
	}

	if(!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE
			| I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "client not i2c capable\n");
		return -ENODEV;
	}

	priv = kzalloc(sizeof(struct ov7675_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&adapter->dev,
			"Failed to allocate memory for private data!\n");
		return -ENOMEM;
	}

	priv->info = icl->priv;

	v4l2_i2c_subdev_init(&priv->subdev, client, &ov7675_subdev_ops);

	icd->ops = &ov7675_ops;

	ret = ov7675_video_probe(icd, client);
	if (ret) {
		icd->ops = NULL;
		kfree(priv);
	}
	return ret;
}

static int ov7675_remove(struct i2c_client *client)
{
	struct ov7675_priv       *priv = to_ov7675(client);
	struct soc_camera_device *icd = client->dev.platform_data;

	icd->ops = NULL;
	kfree(priv);
	return 0;
}

static const struct i2c_device_id ov7675_id[] = {
	{ "ov7675-front",  0 },
	{ "ov7675-back",  0 },
	{ }
};


MODULE_DEVICE_TABLE(i2c, ov7675_id);

static struct i2c_driver ov7675_i2c_driver = {
	.driver = {
		.name = "ov7675",
	},
	.probe    = ov7675_probe,
	.remove   = ov7675_remove,
	.id_table = ov7675_id,
};

/*
 * Module functions
 */
static int __init ov7675_module_init(void)
{
	return i2c_add_driver(&ov7675_i2c_driver);
}

static void __exit ov7675_module_exit(void)
{
	i2c_del_driver(&ov7675_i2c_driver);
}

module_init(ov7675_module_init);
module_exit(ov7675_module_exit);

MODULE_DESCRIPTION("camera sensor ov7675 driver");
MODULE_AUTHOR("YeFei <feiye@ingenic.cn>");
MODULE_LICENSE("GPL");
