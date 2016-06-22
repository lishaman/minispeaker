/*
 * V4L2 Driver for camera sensor sp0838
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


#define REG_CHIP_ID		0x02
#define PID_SP0838		0x27

/* Private v4l2 controls */
#define V4L2_CID_PRIVATE_BALANCE  (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PRIVATE_EFFECT  (V4L2_CID_PRIVATE_BASE + 1)

#define REG31				0x31
#define REG31_HFLIP_IMG		0x20 /* Horizontal mirror image ON/OFF */
#define REG31_VFLIP_IMG     0x40 /* Vertical flip image ON/OFF */

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
enum sp0838_width {
	W_QCIF	= 176,
	W_QVGA	= 320,
	W_CIF	= 352,
	W_VGA	= 640,
};

enum sp0838_height {
	H_QCIF	= 144,
	H_QVGA	= 240,
	H_CIF	= 288,
	H_VGA	= 480,
};

struct sp0838_win_size {
	char *name;
	enum sp0838_width width;
	enum sp0838_height height;
	const struct regval_list *regs;
};


struct sp0838_priv {
	struct v4l2_subdev subdev;
	struct sp0838_camera_info *info;
	enum v4l2_mbus_pixelcode cfmt_code;
	const struct sp0838_win_size *win;
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

static const struct regval_list sp0838_init_regs[] = {
	{0xfd,0x00},

	{0x1B,0x02},
	{0x27,0xe8},
	{0x28,0x0B},
	{0x32,0x00},
	{0x22,0xc0},
	{0x26,0x10},
	{0x31,0x10 | 0x20},	//set mirror bit
	{0x5f,0x11},

	{0xfd,0x01},

	{0x25,0x1a},
	{0x26,0xfb},
	{0x28,0x75},
	{0x29,0x4e},
	{0x31,0x60},
	{0x32,0x18},
	{0x4d,0xdc},
	{0x4e,0x53},
	{0x41,0x8c},
	{0x42,0x57},
	{0x55,0xff},
	{0x56,0x00},
	{0x59,0x82},
	{0x5a,0x00},
	{0x5d,0xff},
	{0x5e,0x6f},
	{0x57,0xff},
	{0x58,0x00},
	{0x5b,0xff},
	{0x5c,0xa8},
	{0x5f,0x75},
	{0x60,0x00},
	{0x2d,0x00},
	{0x2e,0x00},
	{0x2f,0x00},
	{0x30,0x00},
	{0x33,0x00},
	{0x34,0x00},
	{0x37,0x00},
	{0x38,0x00},

	{0xfd,0x00},

	{0x33,0x6f}, //0x6f
	{0x51,0x3f},
	{0x52,0x09},
	{0x53,0x00},
	{0x54,0x00},
	{0x55,0x10},
	{0x4f,0x08},
	{0x50,0x08},
	{0x57,0x40},
	{0x58,0x40},
	{0x59,0x10},
	{0x56,0x71},  	//0x70
	{0x5a,0x02},
	{0x5b,0x10},	//0x02
	{0x5c,0x28},	//0x20
	{0x65,0x06},
	{0x66,0x01},
	{0x67,0x03},
	{0x68,0xc5},	//0xc6
	{0x69,0x7f},
	{0x6a,0x01},
	{0x6b,0x06},
	{0x6c,0x01},
	{0x6d,0x03},
	{0x6e,0xc5},	//0xc6
	{0x6f,0x7f},
	{0x70,0x01},
	{0x71,0x0a},
	{0x72,0x10},
	{0x73,0x03},
	{0x74,0xc4},	//0xc4
	{0x75,0x7f},
	{0x76,0x01},
	{0xcb,0x07},
	{0xcc,0x04},
	{0xce,0xff},
	{0xcf,0x10},
	{0xd0,0x20},
	{0xd1,0x00},
	{0xd2,0x1c},
	{0xd3,0x16},
	{0xd4,0x00},
	{0xd6,0x1c},
	{0xd7,0x16},
	{0xdd,0x70}, //0x70},
	{0xde,0xa5}, //0x90},
	{0x7f,0xd7},
	{0x80,0xbc},
	{0x81,0xed},
	{0x82,0xd7},
	{0x83,0xd4},
	{0x84,0xd6},
	{0x85,0xff},
	{0x86,0x89},
	{0x87,0xf8},
	{0x88,0x3c},
	{0x89,0x33},
	{0x8a,0x0f},
	{0x8b,0x00},
	{0x8c,0x1a},
	{0x8d,0x29},
	{0x8e,0x41},
	{0x8f,0x62},
	{0x90,0x7c},
	{0x91,0x90},
	{0x92,0xa2},
	{0x93,0xaf},
	{0x94,0xbc},
	{0x95,0xc5},
	{0x96,0xcd},
	{0x97,0xd5},
	{0x98,0xdd},
	{0x99,0xe5},
	{0x9a,0xed},
	{0x9b,0xf5},

	{0xfd,0x01},

	{0x8d,0xfd},
	{0x8e,0xff},

	{0xfd,0x00},

	{0xca,0xcf},
	{0xd8,0x50},	//0x48},
	{0xd9,0x50},	//0x48},
	{0xda,0x50},	//0x48},
	{0xdb,0x40},
	{0xb9,0x00},
	{0xba,0x04},
	{0xbb,0x08},
	{0xbc,0x10},
	{0xbd,0x20},
	{0xbe,0x30},
	{0xbf,0x40},
	{0xc0,0x50},
	{0xc1,0x60},
	{0xc2,0x70},
	{0xc3,0x80},
	{0xc4,0x90},
	{0xc5,0xA0},
	{0xc6,0xB0},
	{0xc7,0xC0},
	{0xc8,0xD0},
	{0xc9,0xE0},

	{0xfd,0x01},

	{0x89,0xf0},
	{0x8a,0xff},

	{0xfd,0x00},

	{0xe8,0x30},
	{0xe9,0x30},
	{0xea,0x40},
	{0xf4,0x1b},
	{0xf5,0x80},
	{0xf7,0x73}, //0x78},
	{0xf8,0x5d}, //0x63},
	{0xf9,0x68},
	{0xfa,0x53},

	{0xfd,0x01},

	{0x09,0x31},
	{0x0a,0x85},
	{0x0b,0x0b},
	{0x14,0x00}, //0x20},
	{0x15,0x0f},

	{0xfd,0x00},
	//fix 22fps
	{0x05,0x00},
	{0x06,0x00},
	{0x09,0x01},
	{0x0a,0x05},
	{0xf0,0x6c},
	{0xf1,0x00},
	{0xf2,0x62},
	{0xf5,0x7b},
	{0xfd,0x01},
	{0x00,0xa0},
	{0x0f,0x63},
	{0x16,0x63},
	{0x17,0x90},
	{0x18,0x98},
	{0x1b,0x63},
	{0x1c,0x98},
	{0xb4,0x20},
	{0xb5,0x3c},
	{0xb6,0x68},
	{0xb9,0x40},
	{0xba,0x4f},
	{0xbb,0x47},
	{0xbc,0x45},
	{0xbd,0x70},
	{0xbe,0x42},
	{0xbf,0x42},
	{0xc0,0x42},
	{0xc1,0x41},
	{0xc2,0x41},
	{0xc3,0x41},
	{0xc4,0x41},
	{0xc5,0x41},
	{0xc6,0x41},
	{0xca,0x70},
	{0xcb,0x04},

	{0x14,0x20},
	{0x15,0x0f},

	{0xfd,0x00},

	{0x32,0x0}, //0x15
	{0x34,0x66},
	{0x35,0x00},
	{0x36,0x00},	//00 40 80 c0


	ENDMARKER,
};


static const struct regval_list sp0838_qvga_regs[] = {
	{0xfd,0x00}, {0x31,0x14},
	{0xe7,0x03}, {0xe7,0x00},
	{0xfd,0x00}, {0x05,0x00},
	{0x06,0x00}, {0x09,0x07},
	{0x0a,0x4e}, {0xf0,0x64},
	{0xf1,0x00}, {0xf2,0x60},
	{0xf5,0x79}, {0xfd,0x01},
	{0x00,0xb3}, {0x0f,0x61},
	{0x16,0x61}, {0x17,0xa3},
	{0x18,0xab}, {0x1b,0x61},
	{0x1c,0xab}, {0xb4,0x20},
	{0xb5,0x3c}, {0xb6,0x60},
	{0xb9,0x40}, {0xba,0x4f},
	{0xbb,0x47}, {0xbc,0x45},
	{0xbd,0x43}, {0xbe,0x42},
	{0xbf,0x42}, {0xc0,0x42},
	{0xc1,0x41}, {0xc2,0x41},
	{0xc3,0x41}, {0xc4,0x41},
	{0xc5,0x70}, {0xc6,0x41},
	{0xca,0x70}, {0xcb,0x0c},
	{0xfd,0x00}, {0x7f,0xd0},
	ENDMARKER,
};

static const struct regval_list sp0838_vga_regs[] = {
	{0xfd,0x00}, {0x47,0x00},
	{0x48,0x00}, {0x49,0x01},
	{0x4a,0xe0}, {0x4b,0x00},
	{0x4c,0x00}, {0x4d,0x02},
	{0x4e,0x80},
	ENDMARKER,
};

static const struct regval_list sp0838_wb_auto_regs[] = {
	{0xfd,0x01}, {0x28,0x75},
	{0x29,0x4e}, {0xfd,0x00},
	{0xe7,0x03}, {0xe7,0x00},
	{0x32,0x15},
	ENDMARKER,
};

static const struct regval_list sp0838_wb_incandescence_regs[] = {
	{0xfd,0x00}, {0x32,0x05},
	{0xfd,0x01}, {0x28,0x41},
	{0x29,0x71}, {0xfd,0x00},
	ENDMARKER,
};

static const struct regval_list sp0838_wb_daylight_regs[] = {
	{0xfd,0x00}, {0x32,0x05},
	{0xfd,0x01}, {0x28,0x6b},
	{0x29,0x48}, {0xfd,0x00},
	ENDMARKER,
};

static const struct regval_list sp0838_wb_fluorescent_regs[] = {
	{0xfd,0x00}, {0x32,0x05},
	{0xfd,0x01}, {0x28,0x5a},
	{0x29,0x62}, {0xfd,0x00},
	ENDMARKER,
};

static const struct regval_list sp0838_wb_cloud_regs[] = {
	{0xfd,0x00}, {0x32,0x05},
	{0xfd,0x01}, {0x28,0x71},
	{0x29,0x41}, {0xfd,0x00},
	ENDMARKER,
};

static const struct mode_list sp0838_balance[] = {
	{0, sp0838_wb_auto_regs}, {1, sp0838_wb_incandescence_regs},
	{2, sp0838_wb_daylight_regs}, {3, sp0838_wb_fluorescent_regs},
	{4, sp0838_wb_cloud_regs},
};


static const struct regval_list sp0838_effect_normal_regs[] = {
	{0x62,0x00},
	ENDMARKER,
};

static const struct regval_list sp0838_effect_grayscale_regs[] = {
	{0x62,0x10},
	ENDMARKER,
};

static const struct regval_list sp0838_effect_sepia_regs[] = {
	{0x62, 0x20},
	ENDMARKER,
};

static const struct regval_list sp0838_effect_colorinv_regs[] = {
	{0x62,0x04},
	ENDMARKER,
};

static const struct mode_list sp0838_effect[] = {
	{0, sp0838_effect_normal_regs}, {1, sp0838_effect_grayscale_regs},
	{2, sp0838_effect_sepia_regs}, {3, sp0838_effect_colorinv_regs},
};

#define SP0838_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r }

static struct sp0838_win_size sp0838_supported_win_sizes[] = {
	SP0838_SIZE("QVGA", W_QVGA, H_QVGA, sp0838_qvga_regs),
	SP0838_SIZE("VGA", W_VGA, H_VGA, sp0838_vga_regs),
};

#define N_WIN_SIZES (ARRAY_SIZE(sp0838_supported_win_sizes))

static const struct regval_list sp0838_yuv422_regs[] = {
	ENDMARKER,
};

static const struct regval_list sp0838_rgb565_regs[] = {
	ENDMARKER,
};

static enum v4l2_mbus_pixelcode sp0838_codes[] = {
	V4L2_MBUS_FMT_YUYV8_2X8,
	V4L2_MBUS_FMT_YUYV8_1_5X8,
	V4L2_MBUS_FMT_JZYUYV8_1_5X8,
};

/*
 * Supported controls
 */
static const struct v4l2_queryctrl sp0838_controls[] = {
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
static const struct v4l2_querymenu sp0838_balance_menus[] = {
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
static const struct v4l2_querymenu sp0838_effect_menus[] = {
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
	},
};

/*
 * General functions
 */
static struct sp0838_priv *to_sp0838(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct sp0838_priv,
			    subdev);
}

static int sp0838_write_array(struct i2c_client *client,
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

static int sp0838_mask_set(struct i2c_client *client,
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
static int sp0838_reset(struct i2c_client *client)
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
static int sp0838_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int sp0838_set_bus_param(struct soc_camera_device *icd,
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

static unsigned long sp0838_query_bus_param(struct soc_camera_device *icd)
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

static int sp0838_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct sp0838_priv *priv = to_sp0838(client);

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


static int sp0838_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct sp0838_priv *priv = to_sp0838(client);
	int ret = 0;
	int i = 0;
	u8 value;

	int balance_count = ARRAY_SIZE(sp0838_balance);
	int effect_count = ARRAY_SIZE(sp0838_effect);

	switch (ctrl->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		if(ctrl->value > balance_count)
			return -EINVAL;

		for(i = 0; i < balance_count; i++) {
			if(ctrl->value == sp0838_balance[i].index) {
				ret = sp0838_write_array(client,
						sp0838_balance[ctrl->value].mode_regs);
				priv->balance_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		if(ctrl->value > effect_count)
			return -EINVAL;

		for(i = 0; i < effect_count; i++) {
			if(ctrl->value == sp0838_effect[i].index) {
				ret = sp0838_write_array(client,
						sp0838_effect[ctrl->value].mode_regs);
				priv->effect_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_VFLIP:
		value = ctrl->value ? REG31_VFLIP_IMG : 0x00;
		priv->flag_vflip = ctrl->value ? 1 : 0;
		ret = sp0838_mask_set(client, REG31, REG31_VFLIP_IMG, value);
		break;

	case V4L2_CID_HFLIP:
		value = ctrl->value ? REG31_HFLIP_IMG : 0x00;
		priv->flag_hflip = ctrl->value ? 1 : 0;
		ret = sp0838_mask_set(client, REG31, REG31_HFLIP_IMG, value);
		break;

	default:
		dev_err(&client->dev, "no V4L2 CID: 0x%x ", ctrl->id);
		return -EINVAL;
	}

	return ret;
}

static int sp0838_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *id)
{
	id->ident    = SUPPORT_HIGH_RESOLUTION_PRE;
	id->revision = 0;

	return 0;
}


static int sp0838_querymenu(struct v4l2_subdev *sd,
					struct v4l2_querymenu *qm)
{
	switch (qm->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		memcpy(qm->name, sp0838_balance_menus[qm->index].name,
				sizeof(qm->name));
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		memcpy(qm->name, sp0838_effect_menus[qm->index].name,
				sizeof(qm->name));
		break;
	}

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sp0838_g_register(struct v4l2_subdev *sd,
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

static int sp0838_s_register(struct v4l2_subdev *sd,
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
static const struct sp0838_win_size *sp0838_select_win(u32 *width, u32 *height)
{
	int i, default_size = ARRAY_SIZE(sp0838_supported_win_sizes) - 1;

	for (i = 0; i < ARRAY_SIZE(sp0838_supported_win_sizes); i++) {
		if (sp0838_supported_win_sizes[i].width  >= *width &&
		    sp0838_supported_win_sizes[i].height >= *height) {
			*width = sp0838_supported_win_sizes[i].width;
			*height = sp0838_supported_win_sizes[i].height;
			return &sp0838_supported_win_sizes[i];
		}
	}

	*width = sp0838_supported_win_sizes[default_size].width;
	*height = sp0838_supported_win_sizes[default_size].height;
	return &sp0838_supported_win_sizes[default_size];
}

static int sp0838_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct sp0838_priv *priv = to_sp0838(client);

	int bala_index = priv->balance_value;
	int effe_index = priv->effect_value;

	/* initialize the sensor with default data */
	ret = sp0838_write_array(client, sp0838_init_regs);
	ret = sp0838_write_array(client, sp0838_balance[bala_index].mode_regs);
	ret = sp0838_write_array(client, sp0838_effect[effe_index].mode_regs);
	if (ret < 0)
		goto err;

	dev_info(&client->dev, "%s: Init default", __func__);
	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	return ret;
}

#if 0
static int sp0838_set_params(struct i2c_client *client, u32 *width, u32 *height,
			     enum v4l2_mbus_pixelcode code)
{
	struct sp0838_priv *priv = to_sp0838(client);
	const struct regval_list *selected_cfmt_regs;
	int ret;

	/* select win */
	priv->win = sp0838_select_win(width, height);

	/* select format */
	priv->cfmt_code = 0;
	switch (code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
		dev_dbg(&client->dev, "%s: Selected cfmt YUV422", __func__);
		selected_cfmt_regs = sp0838_rgb565_regs;
		break;
	default:
	case V4L2_MBUS_FMT_YUYV8_1_5X8:
		dev_dbg(&client->dev, "%s: Selected cfmt YUV422", __func__);
		selected_cfmt_regs = sp0838_yuv422_regs;
		break;
	}

	/* initialize the sensor with default data */
	dev_info(&client->dev, "%s: Init default", __func__);
	ret = sp0838_write_array(client, sp0838_init_regs);
	if (ret < 0)
		goto err;

	/* set size win */
	ret = sp0838_write_array(client, priv->win->regs);
	if (ret < 0)
		goto err;

	priv->cfmt_code = code;
	*width = priv->win->width;
	*height = priv->win->height;

	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	sp0838_reset(client);
	priv->win = NULL;

	return ret;
}
#endif

static int sp0838_g_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct sp0838_priv *priv = to_sp0838(client);

	mf->width = priv->win->width;
	mf->height = priv->win->height;
	mf->code = priv->cfmt_code;

	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field	= V4L2_FIELD_NONE;

	return 0;
}

static int sp0838_s_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	/* current do not support set format, use unify format yuv422i */
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct sp0838_priv *priv = to_sp0838(client);
	int ret;

	priv->win = sp0838_select_win(&mf->width, &mf->height);
	/* set size win */
	ret = sp0838_write_array(client, priv->win->regs);
	if (ret < 0) {
		dev_err(&client->dev, "%s: Error\n", __func__);
		return ret;
	}
	return 0;
}

static int sp0838_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *mf)
{
	const struct sp0838_win_size *win;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	/*
	 * select suitable win
	 */
	win = sp0838_select_win(&mf->width, &mf->height);

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


static int sp0838_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= ARRAY_SIZE(sp0838_codes))
		return -EINVAL;

	*code = sp0838_codes[index];
	return 0;
}

static int sp0838_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	return 0;
}

static int sp0838_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	return 0;
}


/*
 * Frame intervals.  Since frame rates are controlled with the clock
 * divider, we can only do 30/n for integer n values.  So no continuous
 * or stepwise options.  Here we just pick a handful of logical values.
 */

static int sp0838_frame_rates[] = { 30, 15, 10, 5, 1 };

static int sp0838_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	if (interval->index >= ARRAY_SIZE(sp0838_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = sp0838_frame_rates[interval->index];
	return 0;
}


/*
 * Frame size enumeration
 */
static int sp0838_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	for (i = 0; i < N_WIN_SIZES; i++) {
		struct sp0838_win_size *win = &sp0838_supported_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int sp0838_video_probe(struct soc_camera_device *icd,
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
	retval = i2c_smbus_read_byte_data(client, REG_CHIP_ID);
	if(retval == PID_SP0838) {
		dev_info(&client->dev, "read sensor %s id ok\n",
				client->name);
		return 0;
	}

	dev_err(&client->dev, "read sensor %s id: 0x%x, failed!\n",
			client->name, retval);
	return -1;
}

static struct soc_camera_ops sp0838_ops = {
	.set_bus_param		= sp0838_set_bus_param,
	.query_bus_param	= sp0838_query_bus_param,
	.controls		= sp0838_controls,
	.num_controls		= ARRAY_SIZE(sp0838_controls),
};

static struct v4l2_subdev_core_ops sp0838_subdev_core_ops = {
	.init 		= sp0838_init,
	.g_ctrl		= sp0838_g_ctrl,
	.s_ctrl		= sp0838_s_ctrl,
	.g_chip_ident	= sp0838_g_chip_ident,
	.querymenu  = sp0838_querymenu,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register	= sp0838_g_register,
	.s_register	= sp0838_s_register,
#endif
};

static struct v4l2_subdev_video_ops sp0838_subdev_video_ops = {
	.s_stream	= sp0838_s_stream,
	.g_mbus_fmt	= sp0838_g_fmt,
	.s_mbus_fmt	= sp0838_s_fmt,
	.try_mbus_fmt	= sp0838_try_fmt,
	.cropcap	= sp0838_cropcap,
	.g_crop		= sp0838_g_crop,
	.enum_mbus_fmt	= sp0838_enum_fmt,
	.enum_framesizes = sp0838_enum_framesizes,
	.enum_frameintervals = sp0838_enum_frameintervals,
};

static struct v4l2_subdev_ops sp0838_subdev_ops = {
	.core	= &sp0838_subdev_core_ops,
	.video	= &sp0838_subdev_video_ops,
};

/*
 * i2c_driver functions
 */

static int sp0838_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct sp0838_priv *priv;
	struct soc_camera_device *icd = client->dev.platform_data;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct soc_camera_link *icl;
	int ret = 0;

	if(!strcmp(client->name, "sp0838-back")) {
		client->addr -= 1;
	}

	if (!icd) {
		dev_err(&adapter->dev, "SP0838: missing soc-camera data!\n");
		return -EINVAL;
	}

	icl = to_soc_camera_link(icd);
	if (!icl) {
		dev_err(&adapter->dev,
			"SP0838: Missing platform_data for driver\n");
		return -EINVAL;
	}

	if(!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE
			| I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "client not i2c capable\n");
		return -ENODEV;
	}

	priv = kzalloc(sizeof(struct sp0838_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&adapter->dev,
			"Failed to allocate memory for private data!\n");
		return -ENOMEM;
	}

	priv->info = icl->priv;

	v4l2_i2c_subdev_init(&priv->subdev, client, &sp0838_subdev_ops);

	icd->ops = &sp0838_ops;

	ret = sp0838_video_probe(icd, client);
	if (ret) {
		icd->ops = NULL;
		kfree(priv);
	}
	return ret;
}

static int sp0838_remove(struct i2c_client *client)
{
	struct sp0838_priv       *priv = to_sp0838(client);
	struct soc_camera_device *icd = client->dev.platform_data;

	icd->ops = NULL;
	kfree(priv);
	return 0;
}

static const struct i2c_device_id sp0838_id[] = {
	{ "sp0838-front",  0 },
	{ "sp0838-back",  0 },
	{ }
};


MODULE_DEVICE_TABLE(i2c, sp0838_id);

static struct i2c_driver sp0838_i2c_driver = {
	.driver = {
		.name = "sp0838",
	},
	.probe    = sp0838_probe,
	.remove   = sp0838_remove,
	.id_table = sp0838_id,
};

/*
 * Module functions
 */
static int __init sp0838_module_init(void)
{
	return i2c_add_driver(&sp0838_i2c_driver);
}

static void __exit sp0838_module_exit(void)
{
	i2c_del_driver(&sp0838_i2c_driver);
}

module_init(sp0838_module_init);
module_exit(sp0838_module_exit);

MODULE_DESCRIPTION("camera sensor sp0838 driver");
MODULE_AUTHOR("YeFei <feiye@ingenic.cn>");
MODULE_LICENSE("GPL");
