#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <media/v4l2-device.h>
#include <media/tvp5150.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-ctrls.h>
#include <mach/jz_cim.h>
#include "tvp5150_reg.h"
#include "tvp5150_camera.h"

//#define TVP5150_SET_KERNEL_PRINT


static void dump_reg_range(struct i2c_client * sd, char *s, u8 init,
				const u8 end, int max_line)
{
	int i = 0;

	while (init != (u8)(end + 1)) {
		if ((i % max_line) == 0) {
			if (i > 0)
				printk("\n");
			printk("tvp5150: %s reg 0x%02x = ", s, init);
		}
		printk("%02x ", tvp5150_read_reg(sd, init));

		init++;
		i++;
	}
	printk("\n");
}

int tvp5150_log_status(struct i2c_client * sd)
{
#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&sd->dev,"+++++++++++++++++begin dump_tvp5150 register+++++++++++++++++++++++\n");

	printk("tvp5150: Video input source selection #1 = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_VD_IN_SRC_SEL_1));
	printk("tvp5150: Analog channel controls = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_ANAL_CHL_CTL));
	printk("tvp5150: Operation mode controls = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_OP_MODE_CTL));
	printk("tvp5150: Miscellaneous controls = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_MISC_CTL));
	printk("tvp5150: Autoswitch mask= 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_AUTOSW_MSK));
	printk("tvp5150: Color killer threshold control = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_COLOR_KIL_THSH_CTL));
	printk("tvp5150: Luminance processing controls #1 #2 and #3 = %02x %02x %02x\n",
			tvp5150_read_reg(sd, TVP5150_LUMA_PROC_CTL_1),
			tvp5150_read_reg(sd, TVP5150_LUMA_PROC_CTL_2),
			tvp5150_read_reg(sd, TVP5150_LUMA_PROC_CTL_3));
	printk("tvp5150: Brightness control = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_BRIGHT_CTL));
	printk("tvp5150: Color saturation control = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_SATURATION_CTL));
	printk("tvp5150: Hue control = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_HUE_CTL));
	printk("tvp5150: Contrast control = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_CONTRAST_CTL));
	printk("tvp5150: Outputs and data rates select = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_DATA_RATE_SEL));
	printk("tvp5150: Configuration shared pins = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_CONF_SHARED_PIN));
	printk("tvp5150: Active video cropping start = 0x%02x%02x\n",
			tvp5150_read_reg(sd, TVP5150_ACT_VD_CROP_ST_MSB),
			tvp5150_read_reg(sd, TVP5150_ACT_VD_CROP_ST_LSB));
	printk("tvp5150: Active video cropping stop  = 0x%02x%02x\n",
			tvp5150_read_reg(sd, TVP5150_ACT_VD_CROP_STP_MSB),
			tvp5150_read_reg(sd, TVP5150_ACT_VD_CROP_STP_LSB));
	printk("tvp5150: Genlock/RTC = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_GENLOCK));
	printk("tvp5150: Horizontal sync start = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_HORIZ_SYNC_START));
	printk("tvp5150: Vertical blanking start = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_VERT_BLANKING_START));
	printk("tvp5150: Vertical blanking stop = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_VERT_BLANKING_STOP));
	printk("tvp5150: Chrominance processing control #1 and #2 = %02x %02x\n",
			tvp5150_read_reg(sd, TVP5150_CHROMA_PROC_CTL_1),
			tvp5150_read_reg(sd, TVP5150_CHROMA_PROC_CTL_2));
	printk("tvp5150: Interrupt reset register B = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_INT_RESET_REG_B));
	printk("tvp5150: Interrupt enable register B = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_INT_ENABLE_REG_B));
	printk("tvp5150: Interrupt configuration register B = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_INTT_CONFIG_REG_B));
	printk("tvp5150: Video standard = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_VIDEO_STD));
	printk("tvp5150: Chroma gain factor: Cb=0x%02x Cr=0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_CB_GAIN_FACT),
			tvp5150_read_reg(sd, TVP5150_CR_GAIN_FACTOR));
	printk("tvp5150: Macrovision on counter = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_MACROVISION_ON_CTR));
	printk("tvp5150: Macrovision off counter = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_MACROVISION_OFF_CTR));
	printk("tvp5150: ITU-R BT.656.%d timing(TVP5150AM1 only)\n",
			(tvp5150_read_reg(sd, TVP5150_REV_SELECT) & 1) ? 3 : 4);
	printk("tvp5150: Device ID = %02x%02x\n",
			tvp5150_read_reg(sd, TVP5150_MSB_DEV_ID),
			tvp5150_read_reg(sd, TVP5150_LSB_DEV_ID));
	printk("tvp5150: ROM version = (hex) %02x.%02x\n",
			tvp5150_read_reg(sd, TVP5150_ROM_MAJOR_VER),
			tvp5150_read_reg(sd, TVP5150_ROM_MINOR_VER));
	printk("tvp5150: Vertical line count = 0x%02x%02x\n",
			tvp5150_read_reg(sd, TVP5150_VERT_LN_COUNT_MSB),
			tvp5150_read_reg(sd, TVP5150_VERT_LN_COUNT_LSB));
	printk("tvp5150: Interrupt status register B = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_INT_STATUS_REG_B));
	printk("tvp5150: Interrupt active register B = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_INT_ACTIVE_REG_B));
	printk("tvp5150: Status regs #1 to #5 = %02x %02x %02x %02x %02x\n",
			tvp5150_read_reg(sd, TVP5150_STATUS_REG_1),
			tvp5150_read_reg(sd, TVP5150_STATUS_REG_2),
			tvp5150_read_reg(sd, TVP5150_STATUS_REG_3),
			tvp5150_read_reg(sd, TVP5150_STATUS_REG_4),
			tvp5150_read_reg(sd, TVP5150_STATUS_REG_5));

	dump_reg_range(sd, "Teletext filter 1",   TVP5150_TELETEXT_FIL1_INI,
			TVP5150_TELETEXT_FIL1_END, 8);
	dump_reg_range(sd, "Teletext filter 2",   TVP5150_TELETEXT_FIL2_INI,
			TVP5150_TELETEXT_FIL2_END, 8);

	printk("tvp5150: Teletext filter enable = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_TELETEXT_FIL_ENA));
	printk("tvp5150: Interrupt status register A = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_INT_STATUS_REG_A));
	printk("tvp5150: Interrupt enable register A = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_INT_ENABLE_REG_A));
	printk("tvp5150: Interrupt configuration = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_INT_CONF));
	printk("tvp5150: VDP status register = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_VDP_STATUS_REG));
	printk("tvp5150: FIFO word count = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_FIFO_WORD_COUNT));
	printk("tvp5150: FIFO interrupt threshold = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_FIFO_INT_THRESHOLD));
	printk("tvp5150: FIFO reset = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_FIFO_RESET));
	printk("tvp5150: Line number interrupt = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_LINE_NUMBER_INT));
	printk("tvp5150: Pixel alignment register = 0x%02x%02x\n",
			tvp5150_read_reg(sd, TVP5150_PIX_ALIGN_REG_HIGH),
			tvp5150_read_reg(sd, TVP5150_PIX_ALIGN_REG_LOW));
	printk("tvp5150: FIFO output control = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_FIFO_OUT_CTRL));
	printk("tvp5150: Full field enable = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_FULL_FIELD_ENA));
	printk("tvp5150: Full field mode register = 0x%02x\n",
			tvp5150_read_reg(sd, TVP5150_FULL_FIELD_MODE_REG));

	dump_reg_range(sd, "CC   data",   TVP5150_CC_DATA_INI,
			TVP5150_CC_DATA_END, 8);

	dump_reg_range(sd, "WSS  data",   TVP5150_WSS_DATA_INI,
			TVP5150_WSS_DATA_END, 8);

	dump_reg_range(sd, "VPS  data",   TVP5150_VPS_DATA_INI,
			TVP5150_VPS_DATA_END, 8);

	dump_reg_range(sd, "VITC data",   TVP5150_VITC_DATA_INI,
			TVP5150_VITC_DATA_END, 10);

	dump_reg_range(sd, "Line mode",   TVP5150_LINE_MODE_INI,
			TVP5150_LINE_MODE_END, 8);

	dev_info(&sd->dev,"+++++++++++++++++end dump_tvp5150 register+++++++++++++++++++++++\n");

#endif
	return 0;
}

/****************************************************************************
			Basic functions
 ****************************************************************************/

static inline void tvp5150_selmux(struct i2c_client * client)
{
#if 0
	int opmode = 0;
	struct tvp5150 *decoder = to_tvp5150(sd);
	int input = 0;
	unsigned char val;

	if ((decoder->output & TVP5150_BLACK_SCREEN) || !decoder->enable)
		input = 8;

	switch (decoder->input) {
	case TVP5150_COMPOSITE1:
		input |= 2;
		/* fall through */
	case TVP5150_COMPOSITE0:
		break;
	case TVP5150_SVIDEO:
	default:
		input |= 1;
		break;
	}

	v4l2_dbg(1, debug, sd, "Selecting video route: route input=%i, output=%i "
			"=> tvp5150 input=%i, opmode=%i\n",
			decoder->input, decoder->output,
			input, opmode);

	tvp5150_write_reg(sd, TVP5150_OP_MODE_CTL, opmode);
	tvp5150_write_reg(sd, TVP5150_VD_IN_SRC_SEL_1, input);

	/* Svideo should enable YCrCb output and disable GPCL output
	 * For Composite and TV, it should be the reverse
	 */
	val = tvp5150_read_reg(sd, TVP5150_MISC_CTL);
	if (decoder->input == TVP5150_SVIDEO)
		val = (val & ~0x40) | 0x10;
	else
		val = (val & ~0x10) | 0x40;
	tvp5150_write_reg(sd, TVP5150_MISC_CTL, val);
#endif

	int opmode = 0;
	int input = 0;
	unsigned char val;

	//input |= 1; /* Svideo should enable*/



#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"Selecting video route: tvp5150 input=%i, opmode=%i\n",
			input, opmode);
#endif

	tvp5150_write_reg(client, TVP5150_OP_MODE_CTL, opmode);
	tvp5150_write_reg(client, TVP5150_VD_IN_SRC_SEL_1, input);

	/* Svideo should enable YCrCb output and disable GPCL output
	 * For Composite and TV, it should be the reverse
	 */

	val = tvp5150_read_reg(client, TVP5150_MISC_CTL);
	if (input == TVP5150_SVIDEO)
		val = (val & ~0x40) | 0x10;
	else
		val = (val & ~0x10) | 0x40;
	tvp5150_write_reg(client, TVP5150_MISC_CTL, val);
};

struct i2c_reg_value {
	unsigned char reg;
	unsigned char value;
};

/* Default values as sugested at TVP5150AM1 datasheet */
static const struct i2c_reg_value tvp5150_init_default[] = {
	{ /* 0x00 */
		TVP5150_VD_IN_SRC_SEL_1,0x00
	},
	{ /* 0x01 */
		TVP5150_ANAL_CHL_CTL,0x15
	},
	{ /* 0x02 */
		TVP5150_OP_MODE_CTL,0x00
	},
	{ /* 0x03 */
		TVP5150_MISC_CTL,0x01
	},
	{ /* 0x06 */
		TVP5150_COLOR_KIL_THSH_CTL,0x10
	},
	{ /* 0x07 */
		TVP5150_LUMA_PROC_CTL_1,0x60
	},
	{ /* 0x08 */
		TVP5150_LUMA_PROC_CTL_2,0x00
	},
	{ /* 0x09 */
		TVP5150_BRIGHT_CTL,0x80
	},
	{ /* 0x0a */
		TVP5150_SATURATION_CTL,0x80
	},
	{ /* 0x0b */
		TVP5150_HUE_CTL,0x00
	},
	{ /* 0x0c */
		TVP5150_CONTRAST_CTL,0x80
	},
	{ /* 0x0d */
		TVP5150_DATA_RATE_SEL,0x47
	},
	{ /* 0x0e */
		TVP5150_LUMA_PROC_CTL_3,0x00
	},
	{ /* 0x0f */
		TVP5150_CONF_SHARED_PIN,0x08
	},
	{ /* 0x11 */
		TVP5150_ACT_VD_CROP_ST_MSB,0x00
	},
	{ /* 0x12 */
		TVP5150_ACT_VD_CROP_ST_LSB,0x00
	},
	{ /* 0x13 */
		TVP5150_ACT_VD_CROP_STP_MSB,0x00
	},
	{ /* 0x14 */
		TVP5150_ACT_VD_CROP_STP_LSB,0x00
	},
	{ /* 0x15 */
		TVP5150_GENLOCK,0x01
	},
	{ /* 0x16 */
		TVP5150_HORIZ_SYNC_START,0x80
	},
	{ /* 0x18 */
		TVP5150_VERT_BLANKING_START,0x00
	},
	{ /* 0x19 */
		TVP5150_VERT_BLANKING_STOP,0x00
	},
	{ /* 0x1a */
		TVP5150_CHROMA_PROC_CTL_1,0x0c
	},
	{ /* 0x1b */
		TVP5150_CHROMA_PROC_CTL_2,0x14
	},
	{ /* 0x1c */
		TVP5150_INT_RESET_REG_B,0x00
	},
	{ /* 0x1d */
		TVP5150_INT_ENABLE_REG_B,0x00
	},
	{ /* 0x1e */
		TVP5150_INTT_CONFIG_REG_B,0x00
	},
	{ /* 0x28 */
		TVP5150_VIDEO_STD,0x00
	},
	{ /* 0x2e */
		TVP5150_MACROVISION_ON_CTR,0x0f
	},
	{ /* 0x2f */
		TVP5150_MACROVISION_OFF_CTR,0x01
	},
	{ /* 0xbb */
		TVP5150_TELETEXT_FIL_ENA,0x00
	},
	{ /* 0xc0 */
		TVP5150_INT_STATUS_REG_A,0x00
	},
	{ /* 0xc1 */
		TVP5150_INT_ENABLE_REG_A,0x00
	},
	{ /* 0xc2 */
		TVP5150_INT_CONF,0x04
	},
	{ /* 0xc8 */
		TVP5150_FIFO_INT_THRESHOLD,0x80
	},
	{ /* 0xc9 */
		TVP5150_FIFO_RESET,0x00
	},
	{ /* 0xca */
		TVP5150_LINE_NUMBER_INT,0x00
	},
	{ /* 0xcb */
		TVP5150_PIX_ALIGN_REG_LOW,0x4e
	},
	{ /* 0xcc */
		TVP5150_PIX_ALIGN_REG_HIGH,0x00
	},
	{ /* 0xcd */
		TVP5150_FIFO_OUT_CTRL,0x01
	},
	{ /* 0xcf */
		TVP5150_FULL_FIELD_ENA,0x00
	},
	{ /* 0xd0 */
		TVP5150_LINE_MODE_INI,0x00
	},
	{ /* 0xfc */
		TVP5150_FULL_FIELD_MODE_REG,0x7f
	},
	{ /* end of data */
		0xff,0xff
	}
};

/* Default values as sugested at TVP5150AM1 datasheet */
static const struct i2c_reg_value tvp5150_init_enable[] = {
	{
		TVP5150_CONF_SHARED_PIN, 2
	},{	/* Automatic offset and AGC enabled */
		TVP5150_ANAL_CHL_CTL, 0x15
	},{	/* Activate YCrCb output 0x9 or 0xd ? */
		TVP5150_MISC_CTL, 0x09
	},{	/* Activates video std autodetection for all standards */
		TVP5150_AUTOSW_MSK, 0x0
	},{	/* Default format: 0x47. For 4:2:2: 0x40 */
		TVP5150_DATA_RATE_SEL, 0x47
	},{
		TVP5150_CHROMA_PROC_CTL_1, 0x0c
	},{
		TVP5150_CHROMA_PROC_CTL_2, 0x54
	},{	/* Non documented, but initialized on WinTV USB2 */
		0x27, 0x20
	},{
		0xff,0xff
	}
};



struct tvp5150_vbi_type {
	unsigned int vbi_type;
	unsigned int ini_line;
	unsigned int end_line;
	unsigned int by_field :1;
};

struct i2c_vbi_ram_value {
	u16 reg;
	struct tvp5150_vbi_type type;
	unsigned char values[16];
};

/* This struct have the values for each supported VBI Standard
 * by
 tvp5150_vbi_types should follow the same order as vbi_ram_default
 * value 0 means rom position 0x10, value 1 means rom position 0x30
 * and so on. There are 16 possible locations from 0 to 15.
 */

static struct i2c_vbi_ram_value vbi_ram_default[] =
{
	/* FIXME: Current api doesn't handle all VBI types, those not
	   yet supported are placed under #if 0 */
#if 0
	{0x010, /* Teletext, SECAM, WST System A */
		{V4L2_SLICED_TELETEXT_SECAM,6,23,1},
		{ 0xaa, 0xaa, 0xff, 0xff, 0xe7, 0x2e, 0x20, 0x26,
		  0xe6, 0xb4, 0x0e, 0x00, 0x00, 0x00, 0x10, 0x00 }
	},
#endif
	{0x030, /* Teletext, PAL, WST System B */
		{V4L2_SLICED_TELETEXT_B,6,22,1},
		{ 0xaa, 0xaa, 0xff, 0xff, 0x27, 0x2e, 0x20, 0x2b,
		  0xa6, 0x72, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00 }
	},
#if 0
	{0x050, /* Teletext, PAL, WST System C */
		{V4L2_SLICED_TELETEXT_PAL_C,6,22,1},
		{ 0xaa, 0xaa, 0xff, 0xff, 0xe7, 0x2e, 0x20, 0x22,
		  0xa6, 0x98, 0x0d, 0x00, 0x00, 0x00, 0x10, 0x00 }
	},
	{0x070, /* Teletext, NTSC, WST System B */
		{V4L2_SLICED_TELETEXT_NTSC_B,10,21,1},
		{ 0xaa, 0xaa, 0xff, 0xff, 0x27, 0x2e, 0x20, 0x23,
		  0x69, 0x93, 0x0d, 0x00, 0x00, 0x00, 0x10, 0x00 }
	},
	{0x090, /* Tetetext, NTSC NABTS System C */
		{V4L2_SLICED_TELETEXT_NTSC_C,10,21,1},
		{ 0xaa, 0xaa, 0xff, 0xff, 0xe7, 0x2e, 0x20, 0x22,
		  0x69, 0x93, 0x0d, 0x00, 0x00, 0x00, 0x15, 0x00 }
	},
	{0x0b0, /* Teletext, NTSC-J, NABTS System D */
		{V4L2_SLICED_TELETEXT_NTSC_D,10,21,1},
		{ 0xaa, 0xaa, 0xff, 0xff, 0xa7, 0x2e, 0x20, 0x23,
		  0x69, 0x93, 0x0d, 0x00, 0x00, 0x00, 0x10, 0x00 }
	},
	{0x0d0, /* Closed Caption, PAL/SECAM */
		{V4L2_SLICED_CAPTION_625,22,22,1},
		{ 0xaa, 0x2a, 0xff, 0x3f, 0x04, 0x51, 0x6e, 0x02,
		  0xa6, 0x7b, 0x09, 0x00, 0x00, 0x00, 0x27, 0x00 }
	},
#endif
	{0x0f0, /* Closed Caption, NTSC */
		{V4L2_SLICED_CAPTION_525,21,21,1},
		{ 0xaa, 0x2a, 0xff, 0x3f, 0x04, 0x51, 0x6e, 0x02,
		  0x69, 0x8c, 0x09, 0x00, 0x00, 0x00, 0x27, 0x00 }
	},
	{0x110, /* Wide Screen Signal, PAL/SECAM */
		{V4L2_SLICED_WSS_625,23,23,1},
		{ 0x5b, 0x55, 0xc5, 0xff, 0x00, 0x71, 0x6e, 0x42,
		  0xa6, 0xcd, 0x0f, 0x00, 0x00, 0x00, 0x3a, 0x00 }
	},
#if 0
	{0x130, /* Wide Screen Signal, NTSC C */
		{V4L2_SLICED_WSS_525,20,20,1},
		{ 0x38, 0x00, 0x3f, 0x00, 0x00, 0x71, 0x6e, 0x43,
		  0x69, 0x7c, 0x08, 0x00, 0x00, 0x00, 0x39, 0x00 }
	},
	{0x150, /* Vertical Interval Timecode (VITC), PAL/SECAM */
		{V4l2_SLICED_VITC_625,6,22,0},
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x6d, 0x49,
		  0xa6, 0x85, 0x08, 0x00, 0x00, 0x00, 0x4c, 0x00 }
	},
	{0x170, /* Vertical Interval Timecode (VITC), NTSC */
		{V4l2_SLICED_VITC_525,10,20,0},
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x6d, 0x49,
		  0x69, 0x94, 0x08, 0x00, 0x00, 0x00, 0x4c, 0x00 }
	},
#endif
	{0x190, /* Video Program System (VPS), PAL */
		{V4L2_SLICED_VPS,16,16,0},
		{ 0xaa, 0xaa, 0xff, 0xff, 0xba, 0xce, 0x2b, 0x0d,
		  0xa6, 0xda, 0x0b, 0x00, 0x00, 0x00, 0x60, 0x00 }
	},
	/* 0x1d0 User programmable */

	/* End of struct */
	{ (u16)-1 }
};


static int tvp5150_write_inittab(struct i2c_client * client,
				const struct i2c_reg_value *regs)
{
	while (regs->reg != 0xff) {
		tvp5150_write_reg(client, regs->reg, regs->value);
		regs++;
	}
	return 0;
}



static int tvp5150_vdp_init(struct i2c_client * client,
				const struct i2c_vbi_ram_value *regs)
{
	unsigned int i;

	/* Disable Full Field */
	tvp5150_write_reg(client, TVP5150_FULL_FIELD_ENA, 0);

	/* Before programming, Line mode should be at 0xff */
	for (i = TVP5150_LINE_MODE_INI; i <= TVP5150_LINE_MODE_END; i++)
		tvp5150_write_reg(client, i, 0xff);

	/* Load Ram Table */
	while (regs->reg != (u16)-1) {
		tvp5150_write_reg(client, TVP5150_CONF_RAM_ADDR_HIGH, regs->reg >> 8);
		tvp5150_write_reg(client, TVP5150_CONF_RAM_ADDR_LOW, regs->reg);

		for (i = 0; i < 16; i++)
			tvp5150_write_reg(client,  TVP5150_VDP_CONF_RAM_DATA, regs->values[i]);

		regs++;
	}
	return 0;
}

static int tvp5150_set_std(struct i2c_client * client)
{
//	struct tvp5150 *decoder = to_tvp5150(sd);
	int fmt = V4L2_STD_ALL;
#if 0
	decoder->norm = std;

	/* First tests should be against specific std */

	if (std == V4L2_STD_ALL) {
		fmt = 0;	/* Autodetect mode */
	} else if (std & V4L2_STD_NTSC_443) {
		fmt = 0xa;
	} else if (std & V4L2_STD_PAL_M) {
		fmt = 0x6;
	} else if (std & (V4L2_STD_PAL_N | V4L2_STD_PAL_Nc)) {
		fmt = 0x8;
	} else {
		/* Then, test against generic ones */
		if (std & V4L2_STD_NTSC)
			fmt = 0x2;
		else if (std & V4L2_STD_PAL)
			fmt = 0x4;
		else if (std & V4L2_STD_SECAM)
			fmt = 0xc;
	}
#endif

#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"Set video std register to %02x.\n", fmt);
#endif
	tvp5150_write_reg(client, TVP5150_VIDEO_STD, fmt);
	return 0;
}

int tvp5150_init(struct cim_sensor *sensor_info)
{
	struct tvp5150_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct tvp5150_sensor, cs);
	client = s->client;
	/***************** init reg set **************************/
	/*** VGA preview (640X480) 30fps 24MCLK input ***********/

#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"tvp5150 init\n");
#endif
	/* Initializes TVP5150 to its default values */
	tvp5150_write_inittab(client, tvp5150_init_default);

	/* Initializes VDP registers */
	//tvp5150_vdp_init(client, vbi_ram_default);

	/* Selects decoder input */
	//tvp5150_selmux(client);

	/* Initializes TVP5150 to stream enabled values */
	//tvp5150_write_inittab(client, tvp5150_init_enable);
	tvp5150_write_reg(client, TVP5150_MISC_CTL, 0x09);
	mdelay(10);
	/* Initialize fmt */
	//tvp5150_set_std(client);

#ifdef TVP5150_SET_KERNEL_PRINT
	tvp5150_log_status(client);
#endif

	return 0;
}

int tvp5150_preview_set(struct cim_sensor *sensor_info)
{

	struct tvp5150_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct tvp5150_sensor, cs);
	client = s->client;
	/***************** preview reg set **************************/
#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"preview reg set \n");
#endif

	return 0;
}


int tvp5150_size_switch(struct cim_sensor *sensor_info,int width,int height)
{
	struct tvp5150_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct tvp5150_sensor, cs);
	client = s->client;
#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"++++++++++++++++++++++++++++++++++++tvp5150 size switch %d * %d\n",width,height);
#endif

	if(width == 640 && height == 480)
	{

	}
	else if(width == 352 && height == 288)
	{
	}
	else if(width == 176 && height == 144)
	{
	}
	else if(width == 320 && height == 240)
	{

	}
	else
		return 0;


	return 0;
}

int tvp5150_capture_set(struct cim_sensor *sensor_info)
{

	struct tvp5150_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct tvp5150_sensor, cs);
	client = s->client;
	/***************** capture reg set **************************/
#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"capture\n");
#endif
	return 0;
}


void tvp5150_set_ab_50hz(struct i2c_client *client)
{
}

void tvp5150_set_ab_60hz(struct i2c_client *client)
{
}

int tvp5150_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct tvp5150_sensor *s;
	struct i2c_client * client;
	unsigned char *str_antibanding;
	s = container_of(sensor_info, struct tvp5150_sensor, cs);
	client = s->client;
#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"tvp5150_set_antibanding");
#endif
#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev, "set antibanding to %s\n", str_antibanding);
#endif
	return 0;
}

void tvp5150_set_effect_normal(struct i2c_client *client)
{
}

void tvp5150_set_effect_grayscale(struct i2c_client *client)
{
}

void tvp5150_set_effect_sepia(struct i2c_client *client)
{
}

void tvp5150_set_effect_colorinv(struct i2c_client *client)
{
}

void tvp5150_set_effect_sepiagreen(struct i2c_client *client)
{
}

void tvp5150_set_effect_sepiablue(struct i2c_client *client)
{
}

int tvp5150_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct tvp5150_sensor *s;
	struct i2c_client * client;
	unsigned char *str_effect;

	s = container_of(sensor_info, struct tvp5150_sensor, cs);
	client = s->client;
#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"tvp5150_set_effect");
#endif

#if 0
	switch(arg)
	{
		case EFFECT_NONE:
			tvp5150_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
		case EFFECT_MONO :
			tvp5150_set_effect_grayscale(client);
			str_effect = "EFFECT_MONO";
			break;
		case EFFECT_NEGATIVE :
			tvp5150_set_effect_colorinv(client);
			str_effect = "EFFECT_NEGATIVE";
			break;
		case EFFECT_SOLARIZE ://bao guang
			str_effect = "EFFECT_SOLARIZE";
			break;
		case EFFECT_SEPIA :
			tvp5150_set_effect_sepia(client);
			str_effect = "EFFECT_SEPIA";
			break;
		case EFFECT_POSTERIZE ://se diao fen li
			str_effect = "EFFECT_POSTERIZE";
			break;
		case EFFECT_WHITEBOARD :
			str_effect = "EFFECT_WHITEBOARD";
			break;
		case EFFECT_BLACKBOARD :
			str_effect = "EFFECT_BLACKBOARD";
			break;
		case EFFECT_AQUA  ://qian lv se
			tvp5150_set_effect_sepiagreen(client);
			str_effect = "EFFECT_AQUA";
			break;
		case EFFECT_PASTEL:
			str_effect = "EFFECT_PASTEL";
			break;
		case EFFECT_MOSAIC:
			str_effect = "EFFECT_MOSAIC";
			break;
		case EFFECT_RESIZE:
			str_effect = "EFFECT_RESIZE";
			break;
		default:
			tvp5150_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
	}
#endif

#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"set effect to %s\n", str_effect);
#endif
	return 0;
}


void tvp5150_set_wb_auto(struct i2c_client *client)
{
}

void tvp5150_set_wb_cloud(struct i2c_client *client)
{
}

void tvp5150_set_wb_daylight(struct i2c_client *client)
{
}

void tvp5150_set_wb_incandescence(struct i2c_client *client)
{
}

void tvp5150_set_wb_fluorescent(struct i2c_client *client)
{
}

void tvp5150_set_wb_tungsten(struct i2c_client *client)
{
}

int tvp5150_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct tvp5150_sensor *s;
	struct i2c_client * client ;
	unsigned char *str_balance;
	s = container_of(sensor_info, struct tvp5150_sensor, cs);
	client = s->client;
#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"tvp5150_set_balance");
#endif
#if 0
	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			tvp5150_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
		case WHITE_BALANCE_INCANDESCENT :
			tvp5150_set_wb_incandescence(client);
			str_balance = "WHITE_BALANCE_INCANDESCENT";
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			tvp5150_set_wb_fluorescent(client);
			str_balance = "WHITE_BALANCE_FLUORESCENT";
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			str_balance = "WHITE_BALANCE_WARM_FLUORESCENT";
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			tvp5150_set_wb_daylight(client);
			str_balance = "WHITE_BALANCE_DAYLIGHT";
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			tvp5150_set_wb_cloud(client);
			str_balance = "WHITE_BALANCE_CLOUDY_DAYLIGHT";
			break;
		case WHITE_BALANCE_TWILIGHT :
			str_balance = "WHITE_BALANCE_TWILIGHT";
			break;
		case WHITE_BALANCE_SHADE :
			str_balance = "WHITE_BALANCE_SHADE";
			break;
		default:
			tvp5150_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
	}
#endif

#ifdef TVP5150_SET_KERNEL_PRINT
	dev_info(&client->dev,"set balance to %s\n", str_balance);
#endif
	return 0;
}

