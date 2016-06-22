/*
 * V4L2 Driver for camera sensor gc2015
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

#define REG_CHIP_ID		0x2005
/* Private v4l2 controls */
#define V4L2_CID_PRIVATE_BALANCE  (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PRIVATE_EFFECT  (V4L2_CID_PRIVATE_BASE + 1)


#define REG29				0x29
#define REG29_HFLIP_IMG		0x01 /* Horizontal mirror image ON/OFF */
#define REG29_VFLIP_IMG     0x02 /* Vertical flip image ON/OFF */

 /* whether sensor support high resolution (> vga) preview or not */
#define SUPPORT_HIGH_RESOLUTION_PRE		0
//
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
enum gc2015_width {
	W_QCIF	= 176,
	W_CIF	= 352,
	W_VGA	= 640,
	W_XGA	= 1024,
	W_UXGA	= 1600,
};

enum gc2015_height {
	H_QCIF	= 144,
	H_CIF	= 288,
	H_VGA	= 480,
	H_XGA	= 768,
	H_UXGA	= 1200,
};

struct gc2015_win_size {
	char *name;
	enum gc2015_width width;
	enum gc2015_height height;
	const struct regval_list *regs;
};


struct gc2015_priv {
	struct v4l2_subdev subdev;
	struct gc2015_camera_info *info;
	enum v4l2_mbus_pixelcode cfmt_code;
	const struct gc2015_win_size *win;
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

static const struct regval_list gc2015_init_regs[] = {
	{0xfe , 0x80}, //soft reset
	{0xfe , 0x80}, //soft reset
	{0xfe , 0x80}, //soft reset

	{0xfe , 0x00},
	{0x45 , 0x00}, //output_disable

	//preview
	{0x02 , 0x01}, //preview mode
	{0x2a , 0xca}, //[7]col_binning , 0x[6]even skip
	{0x48 , 0x40}, //manual_gain

	{0xfe , 0x01},

	{0xb0 , 0x13}, //[4]Y_LSC_en [3]lsc_compensate [2]signed_b4 [1:0]pixel array select
	{0xb1 , 0x20}, //P_LSC_red_b2
	{0xb2 , 0x20}, //P_LSC_green_b2
	{0xb3 , 0x20}, //P_LSC_blue_b2
	{0xb4 , 0x20}, //P_LSC_red_b4
	{0xb5 , 0x20}, //P_LSC_green_b4
	{0xb6 , 0x20}, //P_LSC_blue_b4
	{0xb7 , 0x00}, //P_LSC_compensate_b2
	{0xb8 , 0x80}, //P_LSC_row_center , 0x344 , 0x (0x600/2-100)/2=100
	{0xb9 , 0x80}, //P_LSC_col_center , 0x544 , 0x (0x800/2-200)/2=100

	{0xba , 0x13}, //[4]Y_LSC_en [3]lsc_compensate [2]signed_b4 [1:0]pixel array select
	{0xbb , 0x20}, //C_LSC_red_b2
	{0xbc , 0x20}, //C_LSC_green_b2
	{0xbd , 0x20}, //C_LSC_blue_b2
	{0xbe , 0x20}, //C_LSC_red_b4
	{0xbf , 0x20}, //C_LSC_green_b4
	{0xc0 , 0x20}, //C_LSC_blue_b4
	{0xc1 , 0x00}, //C_Lsc_compensate_b2
	{0xc2 , 0x80}, //C_LSC_row_center , 0x344 , 0x (0x1200/2-344)/2=128
	{0xc3 , 0x80}, //C_LSC_col_center , 0x544 , 0x (0x1600/2-544)/2=128

	{0xfe , 0x00},
	{0x29 , 0x00}, //cisctl mode 1


	{0x2b , 0x06}, //cisctl mode 3
	{0x32 , 0x0c}, //analog mode 1
	{0x33 , 0x0f}, //analog mode 2
	{0x34 , 0x00}, //[6:4]da_rsg

	{0x35 , 0x88}, //Vref_A25
	{0x37 , 0x03}, //Drive Current 16  13  03

	{0x40 , 0xff},
	{0x41 , 0x24}, //[5]skin_detectionenable[2]auto_gray , 0x[1]y_gamma
	{0x42 , 0x76}, //[7]auto_sa[6]auto_ee[5]auto_dndd[4]auto_lsc[3]na[2]abs , 0x[1]awb
	{0x4b , 0xea}, //[1]AWB_gain_mode , 0x1:atpregain0:atpostgain
	{0x4d , 0x03}, //[1]inbf_en
	{0x4f , 0x01}, //AEC enable

	{0x63 , 0x77}, //BLK mode 1
	{0x66 , 0x00}, //BLK global offset
	{0x6d , 0x04},
	{0x6e , 0x18}, //BLK offset submode,offset R
	{0x6f , 0x10},
	{0x70 , 0x18},
	{0x71 , 0x10},
	{0x73 , 0x03},


	{0x80 , 0x07}, //[7]dn_inc_or_dec [4]zero_weight_mode[3]share [2]c_weight_adap [1]dn_lsc_mode [0]dn_b
	{0x82 , 0x08}, //DN lilat b base

	{0x8a , 0x7c},
	{0x8c , 0x02},
	{0x8e , 0x02},
	{0x8f , 0x48},

	{0xb0 , 0x44},
	{0xb1 , 0xfe},
	{0xb2 , 0x00},
	{0xb3 , 0xf8},
	{0xb4 , 0x48},
	{0xb5 , 0xf8},
	{0xb6 , 0x00},
	{0xb7 , 0x04},
	{0xb8 , 0x00},

	//RGB_GAMMA
	{0xbf , 0x0e},
	{0xc0 , 0x1c},
	{0xc1 , 0x34},
	{0xc2 , 0x48},
	{0xc3 , 0x5a},
	{0xc4 , 0x6b},
	{0xc5 , 0x7b},
	{0xc6 , 0x95},
	{0xc7 , 0xab},
	{0xc8 , 0xbf},
	{0xc9 , 0xce},
	{0xca , 0xd9},
	{0xcb , 0xe4},
	{0xcc , 0xec},
	{0xcd , 0xf7},
	{0xce , 0xfd},
	{0xcf , 0xff},

	{0xd1 , 0x38}, //saturation
	{0xd2 , 0x38}, //saturation
	{0xde , 0x21}, //auto_gray

	{0x98 , 0x30},
	{0x99 , 0xf0},
	{0x9b , 0x00},

	{0xfe , 0x01},
	{0x10 , 0x45}, //AEC mode 1
	{0x11 , 0x32}, //[7]fix target
	{0x13 , 0x60},
	{0x17 , 0x00},
	{0x1c , 0x96},
	{0x1e , 0x11},
	{0x21 , 0xc0}, //max_post_gain
	{0x22 , 0x40}, //max_pre_gain
	{0x2d , 0x06}, //P_N_AEC_exp_level_1[12:8]
	{0x2e , 0x00}, //P_N_AEC_exp_level_1[7:0]
	{0x1e , 0x32},
	{0x33 , 0x00}, //[6:5]max_exp_level [4:0]min_exp_level

	{0x57 , 0x40}, //number limit
	{0x5d , 0x44}, //
	{0x5c , 0x35}, //show mode,close dark_mode
	{0x5e , 0x29}, //close color temp
	{0x5f , 0x50},
	{0x60 , 0x50},
	{0x65 , 0xc0},

	{0x80 , 0x82},
	{0x81 , 0x00},
	{0x83 , 0x00}, //ABS Y stretch limit

	{0xfe , 0x00},

//////qhliu
	{0x44 , 0xa2}, //YUV sequence
	{0x45 , 0x0f}, //output enable
	{0x46 , 0x03}, //sync mode

/////qhliu add end

//---------------Updated By Mormo 2011/03/14 Start----------------//
	{0xfe, 0x01},//page1
	{0x34, 0x02}, //Preview minimum exposure
	{0xfe, 0x00}, //page0
//---------------Updated By Mormo 2011/03/14 End----------------//
	ENDMARKER,
};


static const struct regval_list gc2015_qcif_regs[] = {
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

static const struct regval_list gc2015_cif_regs[] = {
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

static const struct regval_list gc2015_vga_regs[] = {
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


static const struct regval_list gc2015_xga_regs[] = {
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

static const struct regval_list gc2015_uxga_regs[] = {
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

static const struct regval_list gc2015_wb_auto_regs[] = {
	{0xfe,0x00}, {0x42,0x76},
	ENDMARKER,
};

static const struct regval_list gc2015_wb_incandescence_regs[] = {
	{0xfe,0x00}, {0x42,0x74},
	{0x7a,0x48}, {0x7b,0x40},
	{0x7c,0x5c},
	ENDMARKER,
};

static const struct regval_list gc2015_wb_daylight_regs[] = {
	{0xfe,0x00}, {0x42,0x74},
	{0x7a,0x74}, {0x7b,0x52},
	{0x7c,0x40},
	ENDMARKER,
};

static const struct regval_list gc2015_wb_fluorescent_regs[] = {
	{0xfe,0x00}, {0x42,0x74},
	{0x7a,0x40}, {0x7b,0x42},
	{0x7c,0x50},
	ENDMARKER,
};

static const struct regval_list gc2015_wb_cloud_regs[] = {
	{0xfe,0x00}, {0x42,0x74},
	{0x7a,0x8c}, {0x7b,0x50},
	{0x7c,0x40},
	ENDMARKER,
};

static const struct mode_list gc2015_balance[] = {
	{0, gc2015_wb_auto_regs}, {1, gc2015_wb_incandescence_regs},
	{2, gc2015_wb_daylight_regs}, {3, gc2015_wb_fluorescent_regs},
	{4, gc2015_wb_cloud_regs},
};


static const struct regval_list gc2015_effect_normal_regs[] = {
	{0xfe,0x00}, {0x43,0x00},
	ENDMARKER,
};

static const struct regval_list gc2015_effect_grayscale_regs[] = {
	{0xfe,0x00}, {0x43,0x02},
	{0xda,0x00}, {0xdb,0x00},
	ENDMARKER,
};

static const struct regval_list gc2015_effect_sepia_regs[] = {
	{0xfe,0x00}, {0x43,0x02},
	{0xda,0x28}, {0xdb,0x28},
	ENDMARKER,
};

static const struct regval_list gc2015_effect_colorinv_regs[] = {
	{0xfe,0x00}, {0x43,0x01},
	ENDMARKER,
};

static const struct regval_list gc2015_effect_sepiabluel_regs[] = {
	{0xfe,0x00}, {0x43,0x02},
	{0xda,0x50}, {0xdb,0xe0},
	ENDMARKER,
};

static const struct mode_list gc2015_effect[] = {
	{0, gc2015_effect_normal_regs}, {1, gc2015_effect_grayscale_regs},
	{2, gc2015_effect_sepia_regs}, {3, gc2015_effect_colorinv_regs},
	{4, gc2015_effect_sepiabluel_regs},
};

#define GC2015_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r }

static struct gc2015_win_size gc2015_supported_win_sizes[] = {
	GC2015_SIZE("QCIF", W_QCIF, H_QCIF, gc2015_qcif_regs),
	GC2015_SIZE("CIF", W_CIF, H_CIF, gc2015_cif_regs),
	GC2015_SIZE("VGA", W_VGA, H_VGA, gc2015_vga_regs),

	GC2015_SIZE("XGA", W_XGA, H_XGA, gc2015_xga_regs),
	GC2015_SIZE("UXGA", W_UXGA, H_UXGA, gc2015_uxga_regs),
};

#define N_WIN_SIZES (ARRAY_SIZE(gc2015_supported_win_sizes))

static const struct regval_list gc2015_yuv422_regs[] = {
	ENDMARKER,
};

static const struct regval_list gc2015_rgb565_regs[] = {
	ENDMARKER,
};

static enum v4l2_mbus_pixelcode gc2015_codes[] = {
	V4L2_MBUS_FMT_YUYV8_2X8,
	V4L2_MBUS_FMT_YUYV8_1_5X8,
	V4L2_MBUS_FMT_JZYUYV8_1_5X8,
};

/*
 * Supported controls
 */
static const struct v4l2_queryctrl gc2015_controls[] = {
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
static const struct v4l2_querymenu gc2015_balance_menus[] = {
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
static const struct v4l2_querymenu gc2015_effect_menus[] = {
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
static struct gc2015_priv *to_gc2015(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct gc2015_priv,
			    subdev);
}

static int gc2015_write_array(struct i2c_client *client,
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

static int gc2015_mask_set(struct i2c_client *client,
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
static int gc2015_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int gc2015_set_bus_param(struct soc_camera_device *icd,
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

static unsigned long gc2015_query_bus_param(struct soc_camera_device *icd)
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

static int gc2015_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct gc2015_priv *priv = to_gc2015(client);

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

static int gc2015_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct gc2015_priv *priv = to_gc2015(client);
	int ret = 0;
	int i = 0;
	u8 value;

	int balance_count = ARRAY_SIZE(gc2015_balance);
	int effect_count = ARRAY_SIZE(gc2015_effect);

	switch (ctrl->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		if(ctrl->value > balance_count)
			return -EINVAL;

		for(i = 0; i < balance_count; i++) {
			if(ctrl->value == gc2015_balance[i].index) {
				ret = gc2015_write_array(client,
						gc2015_balance[ctrl->value].mode_regs);
				priv->balance_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		if(ctrl->value > effect_count)
			return -EINVAL;

		for(i = 0; i < effect_count; i++) {
			if(ctrl->value == gc2015_effect[i].index) {
				ret = gc2015_write_array(client,
						gc2015_effect[ctrl->value].mode_regs);
				priv->effect_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_VFLIP:
		value = ctrl->value ? REG29_VFLIP_IMG : 0x00;
		priv->flag_vflip = ctrl->value ? 1 : 0;
		ret = gc2015_mask_set(client, REG29, REG29_VFLIP_IMG, value);
		break;

	case V4L2_CID_HFLIP:
		value = ctrl->value ? REG29_HFLIP_IMG : 0x00;
		priv->flag_hflip = ctrl->value ? 1 : 0;
		ret = gc2015_mask_set(client, REG29, REG29_HFLIP_IMG, value);
		break;

	default:
		dev_err(&client->dev, "no V4L2 CID: 0x%x ", ctrl->id);
		return -EINVAL;
	}

	return ret;
}

static int gc2015_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *id)
{
	id->ident    = SUPPORT_HIGH_RESOLUTION_PRE;
	id->revision = 0;

	return 0;
}

static int gc2015_querymenu(struct v4l2_subdev *sd,
					struct v4l2_querymenu *qm)
{
	switch (qm->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		memcpy(qm->name, gc2015_balance_menus[qm->index].name,
				sizeof(qm->name));
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		memcpy(qm->name, gc2015_effect_menus[qm->index].name,
				sizeof(qm->name));
		break;
	}

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int gc2015_g_register(struct v4l2_subdev *sd,
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

static int gc2015_s_register(struct v4l2_subdev *sd,
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
static const struct gc2015_win_size *gc2015_select_win(u32 *width, u32 *height)
{
	int i, default_size = ARRAY_SIZE(gc2015_supported_win_sizes) - 1;

	for (i = 0; i < ARRAY_SIZE(gc2015_supported_win_sizes); i++) {
		if (gc2015_supported_win_sizes[i].width  >= *width &&
		    gc2015_supported_win_sizes[i].height >= *height) {
			*width = gc2015_supported_win_sizes[i].width;
			*height = gc2015_supported_win_sizes[i].height;
			return &gc2015_supported_win_sizes[i];
		}
	}

	*width = gc2015_supported_win_sizes[default_size].width;
	*height = gc2015_supported_win_sizes[default_size].height;
	return &gc2015_supported_win_sizes[default_size];
}

static int gc2015_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct gc2015_priv *priv = to_gc2015(client);

	int bala_index = priv->balance_value;
	int effe_index = priv->effect_value;

	/* initialize the sensor with default data */
	ret = gc2015_write_array(client, gc2015_init_regs);
	ret = gc2015_write_array(client, gc2015_balance[bala_index].mode_regs);
	ret = gc2015_write_array(client, gc2015_effect[effe_index].mode_regs);
	if (ret < 0)
		goto err;

	dev_info(&client->dev, "%s: Init default", __func__);
	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	return ret;
}

#if 0
static int gc2015_set_params(struct i2c_client *client, u32 *width, u32 *height,
			     enum v4l2_mbus_pixelcode code)
{
	struct gc2015_priv *priv = to_gc2015(client);
	const struct regval_list *selected_cfmt_regs;
	int ret;

	/* select win */
	priv->win = gc2015_select_win(width, height);

	/* select format */
	priv->cfmt_code = 0;
	switch (code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
		dev_dbg(&client->dev, "%s: Selected cfmt YUV422", __func__);
		selected_cfmt_regs = gc2015_rgb565_regs;
		break;
	default:
	case V4L2_MBUS_FMT_YUYV8_1_5X8:
		dev_dbg(&client->dev, "%s: Selected cfmt YUV422", __func__);
		selected_cfmt_regs = gc2015_yuv422_regs;
		break;
	}

	/* initialize the sensor with default data */
	dev_info(&client->dev, "%s: Init default", __func__);
	ret = gc2015_write_array(client, gc2015_init_regs);
	if (ret < 0)
		goto err;

	/* set size win */
	ret = gc2015_write_array(client, priv->win->regs);
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

static int gc2015_g_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct gc2015_priv *priv = to_gc2015(client);

	mf->width = priv->win->width;
	mf->height = priv->win->height;
	mf->code = priv->cfmt_code;

	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field	= V4L2_FIELD_NONE;

	return 0;
}

static int gc2015_s_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	/* current do not support set format, use unify format yuv422i */
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct gc2015_priv *priv = to_gc2015(client);
	int ret;

	priv->win = gc2015_select_win(&mf->width, &mf->height);
	/* set size win */
	ret = gc2015_write_array(client, priv->win->regs);
	if (ret < 0) {
		dev_err(&client->dev, "%s: Error\n", __func__);
		return ret;
	}
	return 0;
}

static int gc2015_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *mf)
{
	const struct gc2015_win_size *win;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	/*
	 * select suitable win
	 */
	win = gc2015_select_win(&mf->width, &mf->height);

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


static int gc2015_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= ARRAY_SIZE(gc2015_codes))
		return -EINVAL;

	*code = gc2015_codes[index];
	return 0;
}

static int gc2015_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	return 0;
}

static int gc2015_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	return 0;
}


/*
 * Frame intervals.  Since frame rates are controlled with the clock
 * divider, we can only do 30/n for integer n values.  So no continuous
 * or stepwise options.  Here we just pick a handful of logical values.
 */

static int gc2015_frame_rates[] = { 30, 15, 10, 5, 1 };

static int gc2015_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	if (interval->index >= ARRAY_SIZE(gc2015_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = gc2015_frame_rates[interval->index];
	return 0;
}


/*
 * Frame size enumeration
 */
static int gc2015_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	for (i = 0; i < N_WIN_SIZES; i++) {
		struct gc2015_win_size *win = &gc2015_supported_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int gc2015_video_probe(struct soc_camera_device *icd,
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
	retval = i2c_smbus_read_byte_data(client, 0x00);
	retval <<= 8;
	retval |= i2c_smbus_read_byte_data(client, 0x01);
	if(retval == REG_CHIP_ID) {
		dev_info(&client->dev, "read sensor %s id ok\n",
				client->name);
		return 0;
	}

	dev_err(&client->dev, "read sensor %s id: 0x%x, failed!\n",
			client->name, retval);
	return -1;
}

static struct soc_camera_ops gc2015_ops = {
	.set_bus_param		= gc2015_set_bus_param,
	.query_bus_param	= gc2015_query_bus_param,
	.controls		= gc2015_controls,
	.num_controls		= ARRAY_SIZE(gc2015_controls),
};

static struct v4l2_subdev_core_ops gc2015_subdev_core_ops = {
	.init 		= gc2015_init,
	.g_ctrl		= gc2015_g_ctrl,
	.s_ctrl		= gc2015_s_ctrl,
	.g_chip_ident	= gc2015_g_chip_ident,
	.querymenu  = gc2015_querymenu,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register	= gc2015_g_register,
	.s_register	= gc2015_s_register,
#endif
};

static struct v4l2_subdev_video_ops gc2015_subdev_video_ops = {
	.s_stream	= gc2015_s_stream,
	.g_mbus_fmt	= gc2015_g_fmt,
	.s_mbus_fmt	= gc2015_s_fmt,
	.try_mbus_fmt	= gc2015_try_fmt,
	.cropcap	= gc2015_cropcap,
	.g_crop		= gc2015_g_crop,
	.enum_mbus_fmt	= gc2015_enum_fmt,
	.enum_framesizes = gc2015_enum_framesizes,
	.enum_frameintervals = gc2015_enum_frameintervals,
};

static struct v4l2_subdev_ops gc2015_subdev_ops = {
	.core	= &gc2015_subdev_core_ops,
	.video	= &gc2015_subdev_video_ops,
};

/*
 * i2c_driver functions
 */

static int gc2015_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct gc2015_priv *priv;
	struct soc_camera_device *icd = client->dev.platform_data;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct soc_camera_link *icl;
	int ret = 0;

	if(!strcmp(client->name, "gc2015-back")) {
		client->addr -= 1;
	}

	if (!icd) {
		dev_err(&adapter->dev, "GC2015: missing soc-camera data!\n");
		return -EINVAL;
	}

	icl = to_soc_camera_link(icd);
	if (!icl) {
		dev_err(&adapter->dev,
			"GC2015: Missing platform_data for driver\n");
		return -EINVAL;
	}

	if(!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE
			| I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "client not i2c capable\n");
		return -ENODEV;
	}

	priv = kzalloc(sizeof(struct gc2015_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&adapter->dev,
			"Failed to allocate memory for private data!\n");
		return -ENOMEM;
	}

	priv->info = icl->priv;

	v4l2_i2c_subdev_init(&priv->subdev, client, &gc2015_subdev_ops);

	icd->ops = &gc2015_ops;

	ret = gc2015_video_probe(icd, client);
	if (ret) {
		icd->ops = NULL;
		kfree(priv);
	}
	return ret;
}

static int gc2015_remove(struct i2c_client *client)
{
	struct gc2015_priv       *priv = to_gc2015(client);
	struct soc_camera_device *icd = client->dev.platform_data;

	icd->ops = NULL;
	kfree(priv);
	return 0;
}

static const struct i2c_device_id gc2015_id[] = {
	{ "gc2015-front",  0 },
	{ "gc2015-back",  0 },
	{ }
};


MODULE_DEVICE_TABLE(i2c, gc2015_id);

static struct i2c_driver gc2015_i2c_driver = {
	.driver = {
		.name = "gc2015",
	},
	.probe    = gc2015_probe,
	.remove   = gc2015_remove,
	.id_table = gc2015_id,
};

/*
 * Module functions
 */
static int __init gc2015_module_init(void)
{
	return i2c_add_driver(&gc2015_i2c_driver);
}

static void __exit gc2015_module_exit(void)
{
	i2c_del_driver(&gc2015_i2c_driver);
}

module_init(gc2015_module_init);
module_exit(gc2015_module_exit);

MODULE_DESCRIPTION("camera sensor gc2015 driver");
MODULE_AUTHOR("YeFei <feiye@ingenic.cn>");
MODULE_LICENSE("GPL");
