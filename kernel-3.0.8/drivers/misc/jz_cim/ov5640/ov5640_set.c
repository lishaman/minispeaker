#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>
#include "ov5640_camera.h"

#include "ov5640_init.h"
#include "ov5642_init.h"
#include "ov5640_set.h"

#define CAMERA_MODE_PREVIEW 0
#define CAMERA_MODE_CAPTURE 1
#define FPS_15 0
#define FPS_7 1
static int mode = CAMERA_MODE_CAPTURE;
static int g_width = 640;
static int g_height = 480;
struct i2c_client * g_client ;

u16 g_chipid = 0;

//---------------------------------------------------
//		debug code
//---------------------------------------------------
int print_write_op = 0;

void dump_all_regs(struct i2c_client *client)
{
	static u16 all_regs[] = {
		0x3000,
		0x3002,
		0x3004,
		0x3006,
		0x3008,
		0x300e,
		0x3017,
		0x3018,
		0x3022,
		0x3023,
		0x302c,
		0x302d,
		0x302e,
		0x3031,
		0x3034,
		0x3035,
		0x3036,
		0x3037,
		0x3038,
		0x3039,
		0x3100,
		0x3103,
		0x3108,
		0x3212,
		0x3503,
		0x3600,
		0x3601,
		0x3612,
		0x3618,
		0x3620,
		0x3621,
		0x3622,
		0x3630,
		0x3631,
		0x3632,
		0x3633,
		0x3634,
		0x3635,
		0x3636,
		0x3703,
		0x3704,
		0x3705,
		0x3708,
		0x3709,
		0x370b,
		0x370c,
		0x3715,
		0x3717,
		0x371b,
		0x3731,
		0x3800,
		0x3801,
		0x3802,
		0x3803,
		0x3804,
		0x3805,
		0x3806,
		0x3807,
		0x3808,
		0x3809,
		0x380a,
		0x380b,
		0x380c,
		0x380d,
		0x380e,
		0x380f,
		0x3810,
		0x3811,
		0x3812,
		0x3813,
		0x3814,
		0x3815,
		0x3820,
		0x3821,
		0x3824,
		0x3901,
		0x3905,
		0x3906,
		0x3a00,
		0x3a02,
		0x3a03,
		0x3a08,
		0x3a09,
		0x3a0a,
		0x3a0b,
		0x3a0d,
		0x3a0e,
		0x3a0f,
		0x3a10,
		0x3a11,
		0x3a13,
		0x3a14,
		0x3a15,
		0x3a18,
		0x3a19,
		0x3a1b,
		0x3a1e,
		0x3a1f,
		0x3c04,
		0x3c05,
		0x3c06,
		0x3c07,
		0x3c08,
		0x3c09,
		0x3c0a,
		0x3c0b,
		0x4001,
		0x4004,
		0x4005,
		0x4050,
		0x4051,
		0x4202,
		0x4300,
		0x4407,
		0x440e,
		0x460b,
		0x460c,
		0x4713,
		0x471c,
		0x4740,
		0x4837,
		0x5000,
		0x5001,
		0x501f,
		0x5025,
		0x5180,
		0x5181,
		0x5182,
		0x5183,
		0x5184,
		0x5185,
		0x5186,
		0x5187,
		0x5188,
		0x5189,
		0x518a,
		0x518b,
		0x518c,
		0x518d,
		0x518e,
		0x518f,
		0x5190,
		0x5191,
		0x5192,
		0x5193,
		0x5194,
		0x5195,
		0x5196,
		0x5197,
		0x5198,
		0x5199,
		0x519a,
		0x519b,
		0x519c,
		0x519d,
		0x519e,
		0x5300,
		0x5301,
		0x5302,
		0x5303,
		0x5304,
		0x5305,
		0x5306,
		0x5307,
		0x5309,
		0x530a,
		0x530b,
		0x530c,
		0x5381,
		0x5382,
		0x5383,
		0x5384,
		0x5385,
		0x5386,
		0x5387,
		0x5388,
		0x5389,
		0x538a,
		0x538b,
		0x5480,
		0x5481,
		0x5482,
		0x5483,
		0x5484,
		0x5485,
		0x5486,
		0x5487,
		0x5488,
		0x5489,
		0x548a,
		0x548b,
		0x548c,
		0x548d,
		0x548e,
		0x548f,
		0x5490,
		0x5580,
		0x5583,
		0x5584,
		0x5585,
		0x5586,
		0x5589,
		0x558a,
		0x558b,
		0x5688,
		0x5689,
		0x568a,
		0x568b,
		0x568c,
		0x568d,
		0x568e,
		0x568f,
		0x5800,
		0x5801,
		0x5802,
		0x5803,
		0x5804,
		0x5805,
		0x5806,
		0x5807,
		0x5808,
		0x5809,
		0x580a,
		0x580b,
		0x580c,
		0x580d,
		0x580e,
		0x580f,
		0x5810,
		0x5811,
		0x5812,
		0x5813,
		0x5814,
		0x5815,
		0x5816,
		0x5817,
		0x5818,
		0x5819,
		0x581a,
		0x581b,
		0x581c,
		0x581d,
		0x581e,
		0x581f,
		0x5820,
		0x5821,
		0x5822,
		0x5823,
		0x5824,
		0x5825,
		0x5826,
		0x5827,
		0x5828,
		0x5829,
		0x582a,
		0x582b,
		0x582c,
		0x582d,
		0x582e,
		0x582f,
		0x5830,
		0x5831,
		0x5832,
		0x5833,
		0x5834,
		0x5835,
		0x5836,
		0x5837,
		0x5838,
		0x5839,
		0x583a,
		0x583b,
		0x583c,
		0x583d,
	};

	u8 val;
	int ret;
	int i;

	print_write_op = 1;

	printk("============dump_all_regs start===============\n");
	for (i = 0; i < sizeof(all_regs) / sizeof(u16); i++) {
		ret = ov5640_reg_read(client, all_regs[i], &val);
		if (ret < 0) {
			break;
		}

		printk("===>reg[0x%04x] = 0x%02x\n", all_regs[i], val);
	}
	printk("============dump_all_regs end===============\n");
}

struct i2c_client *my_client = NULL;

void __dump_all_regs(void) {
	if (my_client)
		dump_all_regs(my_client);
}


//---------------------------------------------------
//		i2c
//---------------------------------------------------
/**
 * ov5640_reg_read - Read a value from a register in an ov5640 sensor device
 * @client: i2c driver client structure
 * @reg: register address / offset
 * @val: stores the value that gets read
 *
 * Read a value from a register in an ov5640 sensor device.
 * The value is returned in 'val'.
 * Returns zero if successful, or non-zero otherwise.
 */
int ov5640_reg_read(struct i2c_client *client, u16 reg, u8 *val)
{
	int ret;
	u8 data[2] = { 0 };
	struct i2c_msg msgs[2] = {
		[0] = {
			.addr   = client->addr,
			.flags  = 0,
			.len    = 2,
			.buf    = data,
		},
		[1] = {
			.addr   = client->addr,
			.flags  = I2C_M_RD,
			.len    = 1,
			.buf    = val,
		}
	};

	data[0] = (u8)(reg >> 8);
	data[1] = (u8)(reg & 0xff);

	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret != 2) {
		printk("===>ov5640: read reg 0x%04x failed\n", reg);
		return ret;
	}

	//printk("===>ov5640: read reg 0x%04x got 0x%02x\n", reg, *val);

	/* bus free time before new start must be at least 1.3us */
	udelay(2);

	return 0;
}

/**
 * Write a value to a register in ov5640 sensor device.
 * @client: i2c driver client structure.
 * @reg: Address of the register to read value from.
 * @val: Value to be written to a specific register.
 * Returns zero if successful, or non-zero otherwise.
 */
int ov5640_reg_write(struct i2c_client *client, u16 reg, u8 val)
{
	int ret;
	unsigned char data[3] = { (u8)(reg >> 8), (u8)(reg & 0xff), val };
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 3,
		.buf	= data,
	};

	if (print_write_op)
		printk("===>ov5640: write reg 0x%04x to 0x%02x\n", reg, val);

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev, "Failed writing register 0x%02x!\n", reg);
		return ret;
	}
#if 0
	switch (reg) {
	case 0x3000:
	case 0x3002:
	case 0x3004:
	case 0x3006:
	case 0x3034:
	case 0x3035:
	case 0x3036:
	case 0x3037:
	case 0x3039:
	case 0x3008:
		mdelay(30);
		break;
	default:
		// bus free time before new start must be at least 1.3us
		udelay(2);
		break;
	}
#else
//	udelay(2);
#endif

	return 0;
}

/**
 * Initialize a list of ov5640 registers.
 * The list of registers is terminated by the pair of values
 * @client: i2c driver client structure.
 * @reglist[]: List of address of the registers to write data.
 * Returns zero if successful, or non-zero otherwise.
 */
int ov5640_reg_writes(struct i2c_client *client, const struct ov5640_reg reglist[])
{
	int err = 0;
	int i = 0;

	while (reglist[i].reg != 0xffff) {
		err = ov5640_reg_write(client, reglist[i].reg, reglist[i].val);
	//	err = ov5640_write_reg(client, reglist[i].reg, reglist[i].val);
		if (err)
			return err;
		i++;
	}
	return 0;
}


static int ov5640_reg_reads(struct i2c_client *client, const struct ov5640_reg reglist[])
{
	int err = 0;
	int i = 0;

	u8 val;

	printk("xfge==>%s L%d: enter\n", __func__, __LINE__);
	while (reglist[i].reg != 0xffff) {
		err = ov5640_reg_read(client, reglist[i].reg, &val);
	//	err = ov5640_write_reg(client, reglist[i].reg, reglist[i].val);
		if (err)
			return err;
		printk("xfge==>%s L%d: {0x%04x, 0x%02x}\n", __func__, __LINE__, reglist[i].reg, val);
		i++;
		msleep(10);
	}
	printk("xfge==>%s L%d: leave\n", __func__, __LINE__);
	return 0;
}


static int ov5640_reg_reads2(struct i2c_client *client, const struct ov5640_reg reglist[])
{
	int err = 0;
	int i = 0;

	u8 val;
	printk("xfge==>%s L%d: enter\n", __func__, __LINE__);

	while (reglist[i].reg != 0xffff) {
		err = ov5640_reg_read(client, reglist[i].reg, &val);
	//	err = ov5640_write_reg(client, reglist[i].reg, reglist[i].val);
		if (err)
			return err;
		if (reglist[i].val != val) {
			printk("%s(): {reg 0x%04x, w 0x%02x:r 0x%02x}\n", __func__, reglist[i].reg, reglist[i].val, val);
		}
		i++;
		msleep(10);
	}
	printk("xfge==>%s L%d: leave\n", __func__, __LINE__);
	return 0;
}

//---------------------------------------------------
//		legacy code
//---------------------------------------------------
static int ov5640_write_reg(struct i2c_client *client,unsigned short reg, unsigned char val)
{
	return ov5640_reg_write(client, reg, val);
}

static unsigned char ov5640_read_reg(struct i2c_client *client,unsigned short reg)
{
	u8 val;

	ov5640_reg_read(client, reg, &val);
	return val;
}

//---------------------------------------------------
//		i2c group write
//---------------------------------------------------
static int ov5640_write_i2c_group(struct i2c_client *client, u16 reg, const u8* val, int length)
{
	int ret;
	u8 data[length+2];
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= length+2,
		.buf	= data,
	};

	memset(data,0,length+2);
	data[0] = (u8)(reg >>8);
	data[1] = (u8)(reg & 0xff);
	if(val!=NULL){
		memcpy(data+2,val,length);
	}

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev, "Failed writing register 0x%02x!\n", reg);
		return ret;
	}
	return 0;
}

static int ov5640_download_firmware_group(struct cim_sensor *sensor_info)
{
	struct ov5640_sensor *s = container_of(sensor_info, struct ov5640_sensor, cs);
	struct i2c_client *client = s->client;

	int i,j,length,address,group;
	int group_size=256;

	if (s->cs.chipid == 0x5640) {

		length = sizeof(ov5640_af_firmware)/sizeof(u8);
		address = 0x8000;
	
		group = length/group_size;
		ov5640_write_reg(client, 0x3000, 0x20);
		for(i=0; i<group; i++)
		{
			ov5640_write_i2c_group(client,address,&ov5640_af_firmware[i*group_size],group_size);
			address += group_size;
		}
		for(j=i*group_size;j<length;j++)
		{
			ov5640_write_reg(client,address,ov5640_af_firmware[j]);
			address++;
		}
		
		ov5640_write_reg(client, 0x3022, 0x00);
		ov5640_write_reg(client, 0x3023, 0x00);
		ov5640_write_reg(client, 0x3024, 0x00);
		ov5640_write_reg(client, 0x3025, 0x00);
		ov5640_write_reg(client, 0x3026, 0x00);
		ov5640_write_reg(client, 0x3027, 0x00);
		ov5640_write_reg(client, 0x3028, 0x00);
		ov5640_write_reg(client, 0x3029, 0x7f);
		ov5640_write_reg(client, 0x3000, 0x00);
	
	} else if (s->cs.chipid == 0x5642) {

		length = sizeof(ov5642_af_firmware)/sizeof(u8);
		address = 0x8000;
		group = length/group_size;

		ov5640_reg_write(client, 0x3000, 0x20);

		for(i=0; i<group; i++) 
		{
			ov5640_write_i2c_group(client,address,&ov5642_af_firmware[i*group_size],group_size);
			address += group_size;
		}

		for(j=i*group_size; j<length; j++)
		{
			ov5640_reg_write(client,address,ov5642_af_firmware[j]);
			address++;
		}

		ov5640_reg_write(client, 0x3024, 0x00);
		ov5640_reg_write(client, 0x3025, 0x00);
		ov5640_reg_write(client, 0x5082, 0x00);
		ov5640_reg_write(client, 0x5083, 0x00);
		ov5640_reg_write(client, 0x5084, 0x00);
		ov5640_reg_write(client, 0x5085, 0x00);
		ov5640_reg_write(client, 0x3026, 0x00);
		ov5640_reg_write(client, 0x3027, 0xff);
		ov5640_reg_write(client, 0x3000, 0x00);

	} else {
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, s->cs.chipid);
		return -EINVAL;
	}

	return 0;
}

//---------------------------------------------------
//		delay work
//---------------------------------------------------
void ov5640_late_work(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work, struct delayed_work, work);
	struct ov5640_sensor *s = container_of(dwork, struct ov5640_sensor, work);
	int ret = 0;

	if ( s->download_af_fw ) {

		//download auto-focus firmware
		//ov5640_reg_writes(s->client, ov5640_af_regs);
	
		//dev_info(&s->client->dev, "download AF fw done\n");

		//start timer: fix AE
		//mod_timer(&s->timer, jiffies + HZ);
	}

	if ( s->fix_ae ) {
		ret = ov5640_fix_ae(s);
		if (ret)
			dev_err(&s->client->dev, "fix AE fail!");
	}
}

//---------------------------------------------------
//		init routine
//---------------------------------------------------
int ov5640_init(struct cim_sensor *sensor_info)
{
	struct ov5640_sensor *s;
	struct i2c_client * client ;
	int ret = 0;
	u8 val;

	s = container_of(sensor_info, struct ov5640_sensor, cs);

	client = s->client;
	g_client = client;
	mode = CAMERA_MODE_PREVIEW;
	dev_info(&client->dev, "ov5640_init: chip id: 0x%04x, module vendor: 0x%02x\n", s->cs.chipid, s->cs.vendor & 0xff);

	if (s->cs.chipid == 0x5640) {

		switch (s->cs.vendor & 0xff) {
		case MODULE_VENDOR_TRULY:		//0x55  信利
#ifdef CONFIG_OV5640_RAW_BAYER
			printk("==>%s L%d: CONFIG_OV5640_RAW_BAYER!\n", __func__, __LINE__);

			// VGA Raw bayer 
			ret |= ov5640_reg_writes(client,ov5640_vga_raw_bayer_reset);
			mdelay(5);
			ret |= ov5640_reg_writes(client, ov5640_vga_raw_bayer);

//			ret |= ov5640_reg_writes(client, ov5640_colorbar);
#else
			ret |= ov5640_reg_writes(client,ov5640_init_reset);
			mdelay(5);
		//	ret |= ov5640_reg_writes(client, ov5640_af_regs);	//download auto-focus firmware
		//	ov5640_download_firmware_group(sensor_info);
			ret |= ov5640_reg_writes(client, ov5640_init_table);
#endif
			break;
		case MODULE_VENDOR_GLOBALOPTICS:	//0xaa  光阵
		 	ret |= ov5640_reg_writes(client,ov5640_init_reset);
	                mdelay(5);
		//	ret |= ov5640_reg_writes(client, ov5640_af_regs);	//download auto-focus firmware
		//	ov5640_download_firmware_group(sensor_info);
			ret |= ov5640_reg_writes(client, ov5640_init_table_gz);
			break;
		default:
			dev_err(&client->dev, "invalid vendor(0x%02x) !", s->cs.vendor & 0xff);
			ret = -EINVAL;
		}

	} else if (s->cs.chipid == 0x5642) {
		ret |= ov5640_reg_writes(client,ov5642_init_reset);
		mdelay(5);
		ret |= ov5640_reg_writes(client, ov5642_init_table);
	//	ret |= ov5640_reg_writes(client, ov5642_af_regs);	//download auto-focus firmware
		ov5640_download_firmware_group(sensor_info);
	} else {
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, s->cs.chipid);
		ret = -EINVAL;
	}
	if (ret)
		goto err;

	ov5640_reg_read(client, 0x3031, &val);
	//printk("==>%s L%d: Reg(3031)=0x%02x\n", __func__, __LINE__, val);

	dev_info(&client->dev, "ov5640 init ok\n");

	//ov5640_reg_reads(client, ov5640_init_table);
	return 0;
err:
	dev_info(&client->dev, "ov5640 init fail\n");
	return ret;
}

int ov5640_preview_set(struct cim_sensor *sensor_info)                   
{
	struct ov5640_sensor *s;
	struct i2c_client * client ;

	s = container_of(sensor_info, struct ov5640_sensor, cs);
	client = s->client;

	dev_info(&client->dev, "%s L%d\n", __func__, __LINE__);
    	mode = CAMERA_MODE_PREVIEW;

	return 0;
} 

//---------------------------------------------------
//		auto-exposure
//---------------------------------------------------
// static uint32_t stop_ae(struct i2c_client *client)
// {
// 	return 0;
// }

// static uint32_t start_ae(struct i2c_client *client)
// {
// 	return 0;
// }

//---------------------------------------------------
//		preview/capture resolution
//---------------------------------------------------
int ov5640_size_switch(struct cim_sensor *sensor_info,int width,int height)
{
	struct ov5640_sensor *s;
	struct i2c_client *client;

	s = container_of(sensor_info, struct ov5640_sensor, cs);
	client = s->client;

	dev_info(&client->dev,"%s L%d: OV%x set %s size (%d, %d)\n", __func__, __LINE__, s->cs.chipid, (mode == CAMERA_MODE_PREVIEW) ? "preview" : "capture", width, height);

	g_width = width;
	g_height = height;

#ifndef CONFIG_OV5640_RAW_BAYER
	if (s->cs.chipid == 0x5640) {

		if (mode == CAMERA_MODE_PREVIEW) {
			if ((width == 320) && (height == 240)) {
				ov5640_reg_writes(client, ov5640_qvga_regs);
			} else if ((width == 640) && (height == 480)) {
				ov5640_reg_writes(client, ov5640_vga_regs);
			} else if ((width == 1280) && (height == 720)) {
				ov5640_reg_writes(client, ov5640_720p_regs);
			} else {
				printk("%s L%d: invalid preview size %dx%d, default vga!\n", __func__, __LINE__, width, height);
				ov5640_reg_writes(client, ov5640_vga_regs);
				g_width = 640;
				g_height = 480;
			}
		} else if (mode == CAMERA_MODE_CAPTURE) {
			if ((width == 320) && (height == 240)) {
				ov5640_reg_writes(client, ov5640_qvga_regs);
			} else if ((width == 640) && (height == 480)) {
				ov5640_reg_writes(client, ov5640_vga_regs);
			} else if ((width == 1280) && (height == 720)) {
				ov5640_reg_writes(client, ov5640_720p_regs);
			} else if ((width == 1920) && (height == 1080)) {
				ov5640_reg_writes(client, ov5640_1080p_regs);
			} else if ((width == 2592) && (height == 1944)) {
				ov5640_reg_writes(client, ov5640_5mp_regs);
			} else {
				printk("%s L%d: invalid capture size %dx%d, default 720p!\n", __func__, __LINE__, width, height);
				ov5640_reg_writes(client, ov5640_720p_regs);
				g_width = 1280;
				g_height = 720;
			}
		} else {
			printk("%s L%d: invalid mode %d\n", __func__, __LINE__, mode);
		}

	} else if (s->cs.chipid == 0x5642) {

		if (mode == CAMERA_MODE_PREVIEW) {
			ov5640_reg_writes(client, ov5642_vga_regs);
		} else if (mode == CAMERA_MODE_CAPTURE) {
			ov5640_reg_writes(client, ov5642_5mp_regs);
		} else {
			printk("%s L%d: invalid mode %d\n", __func__, __LINE__, mode);
		}

	} else {
		printk("%s L%d: invalid chipid 0x%x\n", __func__, __LINE__, s->cs.chipid);
	}
#endif //CONFIG_OV5640_RAW_BAYER
	
	return 0;
}

int ov5640_capture_set(struct cim_sensor *sensor_info)
{
	struct ov5640_sensor *s;
	struct i2c_client *client;

	uint32_t temp = 0;
	uint8_t temp1, temp2, temp3;

	s = container_of(sensor_info, struct ov5640_sensor, cs);
	client = s->client;

	dev_info(&client->dev, "%s L%d\n", __func__, __LINE__);

	mode = CAMERA_MODE_CAPTURE;

	if (s->cs.chipid == 0x5640) {

		ov5640_reg_write(client, 0x3503, 0x07);	//fix AE
		ov5640_reg_write(client, 0x3a00, 0x38);	//disable night
	
		ov5640_reg_read(client, 0x3500, &temp1);
		ov5640_reg_read(client, 0x3501, &temp2);
		ov5640_reg_read(client, 0x3502, &temp3);
	
		temp = (temp1 << 12) | (temp2 << 4) | (temp3 >> 4);
		temp = temp * 5 / 8;
		temp = temp * 16;
	
		temp1 = temp & 0xff;
		temp2 = (temp & 0xff00) >> 8;
		temp3 = (temp & 0xff0000) >> 16;
	
		ov5640_reg_write(client, 0x3500, temp3);
		ov5640_reg_write(client, 0x3501, temp2);
		ov5640_reg_write(client, 0x3502, temp1);
		//printk("==>%s L%d: temp=0x%08x, temp1=0x%02x, temp2=0x%02x, temp3=0x%02x\n", __func__, __LINE__, temp, temp1, temp2, temp3);
	
		//mdelay(200);
	}

	return 0;
}

//---------------------------------------------------
//		scene
//---------------------------------------------------
static void ov5640_set_scene_auto(struct i2c_client *client)
{
	if (g_chipid == 0x5640) {
		ov5640_reg_writes(client, ov5640_scene_auto);
	} else {}
}

static void ov5640_set_scene_night(struct i2c_client *client)
{
	if (g_chipid == 0x5640) {
		ov5640_reg_writes(client, ov5640_scene_night);
	} else {}
}

static void ov5640_set_scene_sports(struct i2c_client *client)
{
	if (g_chipid == 0x5640) {
		ov5640_reg_writes(client, ov5640_scene_sports);
	} else {}
}

int ov5640_set_scene(struct cim_sensor *sensor_info, unsigned short arg)
{
	struct ov5640_sensor *s = container_of(sensor_info, struct ov5640_sensor, cs);
	struct i2c_client *client = s->client;

	dev_info(&client->dev,"ov5640_set_scene: 0x%x\n", arg);

	switch (arg) {
	case SCENE_MODE_AUTO:
		ov5640_set_scene_auto(client);
		break;
	case SCENE_MODE_NIGHT:
		ov5640_set_scene_night(client);
		break;
	case SCENE_MODE_SPORTS:
		ov5640_set_scene_sports(client);
		break;
	default:
		ov5640_set_scene_auto(client);
		break;
	};
	return 0;
}

//---------------------------------------------------
//		saturation
//---------------------------------------------------
int ov5640_set_saturation(struct cim_sensor *sensor_info, unsigned short arg)
{
	struct ov5640_sensor *s = container_of(sensor_info, struct ov5640_sensor, cs);
	struct i2c_client *client = s->client;

	dev_info(&client->dev,"ov5640_set_saturation: 0x%x\n", arg);

	if (s->cs.chipid == 0x5640) {

		switch (arg) {
		case SATURATION_P4:	// +4
			ov5640_reg_writes(client, ov5640_saturation_regs[0]);
			break;
		case SATURATION_P3:	// +3
			ov5640_reg_writes(client, ov5640_saturation_regs[1]);
			break;
		case SATURATION_P2:	// +2
			ov5640_reg_writes(client, ov5640_saturation_regs[2]);
			break;
		case SATURATION_P1:	// +1
			ov5640_reg_writes(client, ov5640_saturation_regs[3]);
			break;
		case SATURATION_P0:	// 0
			ov5640_reg_writes(client, ov5640_saturation_regs[4]);
			break;
		case SATURATION_M1:	// -1
			ov5640_reg_writes(client, ov5640_saturation_regs[5]);
			break;
		case SATURATION_M2:	// -2
			ov5640_reg_writes(client, ov5640_saturation_regs[6]);
			break;
		case SATURATION_M3:	// -3
			ov5640_reg_writes(client, ov5640_saturation_regs[7]);
			break;
		case SATURATION_M4:	// -4
			ov5640_reg_writes(client, ov5640_saturation_regs[8]);
			break;
		default:		// default 0
			ov5640_reg_writes(client, ov5640_saturation_regs[4]);
			break;
		};

	} else if (s->cs.chipid == 0x5642) {
		ov5640_reg_write(client, 0x5001, ov5640_read_reg(client, 0x5001) | 0x80);
		ov5640_reg_write(client, 0x5580, ov5640_read_reg(client, 0x5580) | 0x02);

		switch (arg) {
		case SATURATION_P4:	// +4
			ov5640_reg_write(client, 0x5583, 0x53);
			ov5640_reg_write(client, 0x5584, 0x53);
			break;
		case SATURATION_P3:	// +3
			ov5640_reg_write(client, 0x5583, 0x50);
			ov5640_reg_write(client, 0x5584, 0x50);
			break;
		case SATURATION_P2:	// +2
			ov5640_reg_write(client, 0x5583, 0x4d);
			ov5640_reg_write(client, 0x5584, 0x4d);
			break;
		case SATURATION_P1:	// +1
			ov5640_reg_write(client, 0x5583, 0x46);
			ov5640_reg_write(client, 0x5584, 0x46);
			break;
		case SATURATION_P0:	// 0
			ov5640_reg_write(client, 0x5583, 0x40);
			ov5640_reg_write(client, 0x5584, 0x40);
			break;
		case SATURATION_M1:	// -1
			ov5640_reg_write(client, 0x5583, 0x30);
			ov5640_reg_write(client, 0x5584, 0x30);
			break;
		case SATURATION_M2:	// -2
			ov5640_reg_write(client, 0x5583, 0x20);
			ov5640_reg_write(client, 0x5584, 0x20);
			break;
		case SATURATION_M3:	// -3
			ov5640_reg_write(client, 0x5583, 0x10);
			ov5640_reg_write(client, 0x5584, 0x10);
			break;
		case SATURATION_M4:	// -4
			ov5640_reg_write(client, 0x5583, 0x00);
			ov5640_reg_write(client, 0x5584, 0x00);
			break;
		default:		// default 0
			ov5640_reg_write(client, 0x5583, 0x40);
			ov5640_reg_write(client, 0x5584, 0x40);
			break;
		};

	} else {
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, s->cs.chipid);
		return -EINVAL;
	}

	return 0;
}

//---------------------------------------------------
//		sharpness
//---------------------------------------------------
int ov5640_set_sharpness(struct cim_sensor *sensor_info, unsigned short arg)
{
	struct ov5640_sensor *s = container_of(sensor_info, struct ov5640_sensor, cs);
	struct i2c_client *client = s->client;

	dev_info(&client->dev,"ov5640_set_sharpness: 0x%x\n", arg);

	if (s->cs.chipid == 0x5640) {
		switch (arg) {
		case SHARP_P4:		// +4
			ov5640_reg_write(client, 0x5302, 0x40);
			break;
		case SHARP_P3:		// +3
			ov5640_reg_write(client, 0x5302, 0x3c);
			break;
		case SHARP_P2:		// +2
			ov5640_reg_write(client, 0x5302, 0x38);
			break;
		case SHARP_P1:		// +1
			ov5640_reg_write(client, 0x5302, 0x34);
			break;
		case SHARP_P0:		// 0
			ov5640_reg_write(client, 0x5302, 0x30);
			break;
		case SHARP_M1:		// -1
			ov5640_reg_write(client, 0x5302, 0x2c);
			break;
		case SHARP_M2:		// -2
			ov5640_reg_write(client, 0x5302, 0x28);
			break;
		case SHARP_M3:		// -3
			ov5640_reg_write(client, 0x5302, 0x24);
			break;
		case SHARP_M4:		// -4
			ov5640_reg_write(client, 0x5302, 0x20);
			break;
		default:		// 0
			ov5640_reg_write(client, 0x5302, 0x30);
			break;
		}
	} else if (s->cs.chipid == 0x5642) {
		// ask OV for settings
	} else {
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, s->cs.chipid);
		return -EINVAL;
	}

	return 0;
}

//---------------------------------------------------
//		contrast
//---------------------------------------------------
int ov5640_set_contrast(struct cim_sensor *sensor_info, unsigned short arg)
{
	struct ov5640_sensor *s = container_of(sensor_info, struct ov5640_sensor, cs);
	struct i2c_client *client = s->client;

	dev_info(&client->dev,"ov5640_set_contrast: 0x%x\n", arg);

	if (s->cs.chipid == 0x5640) {

		switch (arg) {
		case CONTRAST_P5:	// +5
			ov5640_reg_writes(client, ov5640_contrast_regs[0]);
			break;
		case CONTRAST_P4:	// +4
			ov5640_reg_writes(client, ov5640_contrast_regs[1]);
			break;
		case CONTRAST_P3:	// +3
			ov5640_reg_writes(client, ov5640_contrast_regs[2]);
			break;
		case CONTRAST_P2:	// +2
			ov5640_reg_writes(client, ov5640_contrast_regs[3]);
			break;
		case CONTRAST_P1:	// +1
			ov5640_reg_writes(client, ov5640_contrast_regs[4]);
			break;
		case CONTRAST_P0:	// 0
			ov5640_reg_writes(client, ov5640_contrast_regs[5]);
			break;
		case CONTRAST_M1:	// -1
			ov5640_reg_writes(client, ov5640_contrast_regs[6]);
			break;
		case CONTRAST_M2:	// -2
			ov5640_reg_writes(client, ov5640_contrast_regs[7]);
			break;
		case CONTRAST_M3:	// -3
			ov5640_reg_writes(client, ov5640_contrast_regs[8]);
			break;
		case CONTRAST_M4:	// -4
			ov5640_reg_writes(client, ov5640_contrast_regs[9]);
			break;
		case CONTRAST_M5:	// -5
			ov5640_reg_writes(client, ov5640_contrast_regs[10]);
			break;
		default:		// default 0
			ov5640_reg_writes(client, ov5640_contrast_regs[5]);
			break;
		};

	} else if (s->cs.chipid == 0x5642) {

		ov5640_reg_write(client, 0x5001, ov5640_read_reg(client, 0x5001) | 0x80);
		ov5640_reg_write(client, 0x5580, ov5640_read_reg(client, 0x5580) | 0x04);
		ov5640_reg_write(client, 0x5587, 0x00);

		switch (arg) {
		case CONTRAST_P5:	// +5
		case CONTRAST_P4:	// +4
			ov5640_reg_write(client, 0x5588, 0x30);
			break;
		case CONTRAST_P3:	// +3
			ov5640_reg_write(client, 0x5588, 0x2c);
			break;
		case CONTRAST_P2:	// +2
			ov5640_reg_write(client, 0x5588, 0x28);
			break;
		case CONTRAST_P1:	// +1
			ov5640_reg_write(client, 0x5588, 0x24);
			break;
		case CONTRAST_P0:	// 0
			ov5640_reg_write(client, 0x5588, 0x20);
			break;
		case CONTRAST_M1:	// -1
			ov5640_reg_write(client, 0x5588, 0x1c);
			break;
		case CONTRAST_M2:	// -2
			ov5640_reg_write(client, 0x5588, 0x18);
			break;
		case CONTRAST_M3:	// -3
			ov5640_reg_write(client, 0x5588, 0x14);
			break;
		case CONTRAST_M4:	// -4
		case CONTRAST_M5:	// -5
			ov5640_reg_write(client, 0x5588, 0x10);
			break;
		default:		// default 0
			ov5640_reg_write(client, 0x5588, 0x20);
			break;
		};

		ov5640_reg_write(client, 0x558a, 0x00);

	} else {
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, s->cs.chipid);
		return -EINVAL;
	}

	return 0;
}

//---------------------------------------------------
//		auto-focus
//---------------------------------------------------
int ov5640_set_focus(struct cim_sensor *sensor_info, unsigned short arg)
{
	struct ov5640_sensor *s;
	struct i2c_client * client ;

	int i = 60;	//60x50ms=3s，保证超时时间大于对焦所需时间
	u8 reg_value = 0x00;

	s = container_of(sensor_info, struct ov5640_sensor, cs);
	client = s->client;
	dev_info(&client->dev,"ov5640_set_focus\n");

	if (s->cs.chipid == 0x5640) {

		ov5640_reg_write(client, 0x3023, 0x01);	//循环判断标志
		ov5640_reg_write(client, 0x3022, 0x03);	//单次对焦
	//	ov5640_reg_write(client, 0x3022, 0x04);	//连续对焦
		while (i > 0)	// i<0,强制退出循环
		{
			reg_value = ov5640_read_reg(client, 0x3023);
			if (reg_value == 0)
				break;
			msleep(50);	//50ms
			i--;
		}

	} else if (s->cs.chipid == 0x5642) {

		ov5640_reg_read(client, 0x3027, &reg_value);	//AF Status

		ov5640_reg_write(client, 0x3025, 0x01);
		ov5640_reg_write(client, 0x3024, 0x03);
		while (i > 0)	// i<0,强制退出循环
		{
			reg_value = ov5640_read_reg(client, 0x3025);
			if (reg_value == 0) {
				if (ov5640_read_reg(client, 0x3026) != 0)	//对焦成功
					break;
			}
			msleep(50);	//50ms
			i--;
		}
	
	} else {
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, s->cs.chipid);
		return -EINVAL;
	}

	//对焦超时并非对焦失败，关键是看镜头有无前后移动!
	if (i <= 0)
		dev_dbg(&client->dev, "auto focus timeout");

	return 0;
}

//---------------------------------------------------
//		anti-banding 50/60 Hz
//---------------------------------------------------
void ov5640_set_ab_50hz(struct i2c_client *client)
{
//	OV5640_set_bandingfilter(client, 50);
}

void ov5640_set_ab_60hz(struct i2c_client *client)
{
//	OV5640_set_bandingfilter(client, 60);
}

int ov5640_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct ov5640_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov5640_sensor, cs);
	client = s->client;
	dev_info(&client->dev,"ov5640_set_antibanding");
	switch(arg)
	{
		case ANTIBANDING_AUTO :
			ov5640_set_ab_50hz(client);
			dev_info(&client->dev,"ANTIBANDING_AUTO ");
			break;
		case ANTIBANDING_50HZ :
			ov5640_set_ab_50hz(client);
			dev_info(&client->dev,"ANTIBANDING_50HZ ");
			break;
		case ANTIBANDING_60HZ :
			ov5640_set_ab_60hz(client);
			dev_info(&client->dev,"ANTIBANDING_60HZ ");
			break;
		case ANTIBANDING_OFF :
			dev_info(&client->dev,"ANTIBANDING_OFF ");
			break;
	}
	return 0;
}

//---------------------------------------------------
//		effect
//---------------------------------------------------
void ov5640_set_effect_normal(struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		ov5640_reg_writes(client, ov5640_effect_normal_regs);
		break;
	case 0x5642:
		ov5640_reg_write(client, 0x5001, 0xff);
		ov5640_reg_write(client, 0x5580, ov5640_read_reg(client, 0x5580) & ~0xf9);
		ov5640_reg_write(client, 0x558a, ov5640_read_reg(client, 0x558a) & ~0x80);
		ov5640_reg_write(client, 0x5585, 0x80);
		ov5640_reg_write(client, 0x5586, 0x80);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}
}

void ov5640_set_effect_yellowish(struct i2c_client *client)
{
}

void ov5640_set_effect_reddish(struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		break;
	case 0x5642:
		ov5640_reg_write(client, 0x5001, ov5640_read_reg(client, 0x5001) | 0x80);
		ov5640_reg_write(client, 0x558a, ov5640_read_reg(client, 0x558a) & ~0x80);
		ov5640_reg_write(client, 0x5580, (ov5640_read_reg(client, 0x5580) & ~0xf9) | 0x18);
		ov5640_reg_write(client, 0x5585, 0x80);
		ov5640_reg_write(client, 0x5586, 0xc0);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}
}

void ov5640_set_effect_sepia(struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		ov5640_reg_writes(client, ov5640_effect_sepia_regs);
		break;
	case 0x5642:
		ov5640_reg_write(client, 0x5001, ov5640_read_reg(client, 0x5001) | 0x80);
		ov5640_reg_write(client, 0x558a, ov5640_read_reg(client, 0x558a) & ~0x80);
		ov5640_reg_write(client, 0x5580, (ov5640_read_reg(client, 0x5580) & ~0xf9) | 0x18);
		ov5640_reg_write(client, 0x5585, 0x40);
		ov5640_reg_write(client, 0x5586, 0xa0);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}
}

void ov5640_set_effect_negative(struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		ov5640_reg_writes(client, ov5640_effect_colorinv_regs);
		break;
	case 0x5642:
		ov5640_reg_write(client, 0x5001, ov5640_read_reg(client, 0x5001) | 0x80);
		ov5640_reg_write(client, 0x558a, ov5640_read_reg(client, 0x558a) & ~0x80);
		ov5640_reg_write(client, 0x5580, (ov5640_read_reg(client, 0x5580) & ~0xf9) | 0x40);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}

}

void ov5640_set_effect_black_and_white (struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		ov5640_reg_writes(client, ov5640_effect_grayscale_regs);
		break;
	case 0x5642:
		ov5640_reg_write(client, 0x5001, 0xff);
		ov5640_reg_write(client, 0x558a, ov5640_read_reg(client, 0x558a) & ~0x80);
		ov5640_reg_write(client, 0x5580, (ov5640_read_reg(client, 0x5580) & ~0xf9) | 0x20);
		ov5640_reg_write(client, 0x5585, 0x80);
		ov5640_reg_write(client, 0x5586, 0x80);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}

}

void ov5640_set_effect_sepiagreen(struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		ov5640_reg_writes(client, ov5640_effect_sepiabluel_regs);
		break;
	case 0x5642:
		ov5640_reg_write(client, 0x5001, ov5640_read_reg(client, 0x5001) | 0x80);
		ov5640_reg_write(client, 0x558a, ov5640_read_reg(client, 0x558a) & ~0x80);
		ov5640_reg_write(client, 0x5580, (ov5640_read_reg(client, 0x5580) & ~0xf9) | 0x18);
	//	ov5640_reg_write(client, 0x5585, 0x60);
	//	ov5640_reg_write(client, 0x5586, 0x60);
		ov5640_reg_write(client, 0x5585, 0xa0);
		ov5640_reg_write(client, 0x5586, 0x40);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}
}

int ov5640_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct ov5640_sensor *s;
	struct i2c_client * client ;

	s = container_of(sensor_info, struct ov5640_sensor, cs);
	client = s->client;

	dev_info(&client->dev,"ov5640_set_effect: 0x%x\n", arg);
	switch(arg)
	{
		case EFFECT_NONE:
			ov5640_set_effect_normal(client);
			dev_info(&client->dev,"EFFECT_NONE");
			break;
		case EFFECT_MONO :
			ov5640_set_effect_black_and_white(client);
			dev_info(&client->dev,"EFFECT_MONO ");
			break;
		case EFFECT_NEGATIVE :
			ov5640_set_effect_negative(client);
			dev_info(&client->dev,"EFFECT_NEGATIVE ");
			break;
		case EFFECT_SOLARIZE ://bao guang
			dev_info(&client->dev,"EFFECT_SOLARIZE ");
			break;
		case EFFECT_SEPIA :
			ov5640_set_effect_sepia(client);
			dev_info(&client->dev,"EFFECT_SEPIA ");
			break;
		case EFFECT_POSTERIZE ://se diao fen li
			dev_info(&client->dev,"EFFECT_POSTERIZE ");
			break;
		case EFFECT_WHITEBOARD :
			dev_info(&client->dev,"EFFECT_WHITEBOARD ");
			break;
		case EFFECT_BLACKBOARD :
			dev_info(&client->dev,"EFFECT_BLACKBOARD ");
			ov5640_set_effect_black_and_white(client);
			break;
		case EFFECT_AQUA  ://qian lv se
			ov5640_set_effect_sepiagreen(client);
			dev_info(&client->dev,"EFFECT_AQUA  ");
			break;
		case EFFECT_PASTEL:
			ov5640_set_effect_reddish(client);  
			dev_info(&client->dev,"EFFECT_PASTEL");
			break;
		case EFFECT_MOSAIC:
			dev_info(&client->dev,"EFFECT_MOSAIC");
			break;
		case EFFECT_RESIZE:
			dev_info(&client->dev,"EFFECT_RESIZE");
			break;
	}
	return 0;
}

//---------------------------------------------------
//		white-balance
//---------------------------------------------------
void ov5640_set_wb_auto(struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		ov5640_reg_writes(client, ov5640_wb_auto_regs);
		break;
	case 0x5642:
		ov5640_reg_writes(client, ov5642_wb_auto_regs);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}
}

void ov5640_set_wb_cloud(struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		ov5640_reg_writes(client, ov5640_wb_cloud_regs);
		break;
	case 0x5642:
		ov5640_reg_writes(client, ov5642_wb_cloud_regs);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}
}

void ov5640_set_wb_daylight(struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		ov5640_reg_writes(client, ov5640_wb_daylight_regs);
		break;
	case 0x5642:
		ov5640_reg_writes(client, ov5642_wb_daylight_regs);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}
}

void ov5640_set_wb_incandescence(struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		ov5640_reg_writes(client, ov5640_wb_incandescence_regs);
		break;
	case 0x5642:
		ov5640_reg_writes(client, ov5642_wb_incandescence_regs);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}
}

void ov5640_set_wb_fluorescent(struct i2c_client *client)
{
	switch (g_chipid) {
	case 0x5640:
		ov5640_reg_writes(client, ov5640_wb_fluorescent_regs);
		break;
	case 0x5642:
		ov5640_reg_writes(client, ov5642_wb_fluorescent_regs);
		break;
	default:
		dev_err(&client->dev, "%s: invalid chip id (0x%04x) !", __func__, g_chipid);
	}
}

int ov5640_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct ov5640_sensor *s = container_of(sensor_info, struct ov5640_sensor, cs);
	struct i2c_client *client = s->client;
	
	dev_info(&client->dev,"ov5640_set_balance");

	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			ov5640_set_wb_auto(client);
			dev_info(&client->dev,"WHITE_BALANCE_AUTO");
			break;
		case WHITE_BALANCE_INCANDESCENT :	//白炽灯(钨光)
			ov5640_set_wb_incandescence(client);
			dev_info(&client->dev,"WHITE_BALANCE_INCANDESCENT ");
			break;
		case WHITE_BALANCE_FLUORESCENT :	//荧光
			ov5640_set_wb_fluorescent(client);
			dev_info(&client->dev,"WHITE_BALANCE_FLUORESCENT ");
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :	//荧光 暖白
			ov5640_set_wb_fluorescent(client);
			dev_info(&client->dev,"WHITE_BALANCE_WARM_FLUORESCENT ");
			break;
		case WHITE_BALANCE_DAYLIGHT :
			ov5640_set_wb_daylight(client);
			dev_info(&client->dev,"WHITE_BALANCE_DAYLIGHT ");
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT :
			ov5640_set_wb_cloud(client);
			dev_info(&client->dev,"WHITE_BALANCE_CLOUDY_DAYLIGHT ");
			break;
		case WHITE_BALANCE_TWILIGHT :
			dev_info(&client->dev,"WHITE_BALANCE_TWILIGHT ");
			break;
		case WHITE_BALANCE_SHADE :
			dev_info(&client->dev,"WHITE_BALANCE_SHADE ");
			break;
	}

	return 0;
}

//---------------------------------------------------
//		read otp
//---------------------------------------------------
char *vendor_str[] = {
	"Truly Opto",
	"Global Optics",
};

int ov5640_read_otp(struct cim_sensor *sensor_info)
{
	struct ov5640_sensor *s = container_of(sensor_info, struct ov5640_sensor, cs);
	struct i2c_client *client = s->client;
	u16 reg = 0;
	u8 module_id = 0;
	int ret = 0;

	if (s->cs.chipid != 0x5640)
		return 0;

	// Before OTP operation, sensor should be working and previewing in full size image.
	ret |= ov5640_reg_writes(client, ov5640_init_table);
//	ret |= ov5640_reg_writes(client, ov5640_vga_regs);

	// 1. set 0x3d00 ~ 0x3d1f to 0x00
	for (reg = 0x3d00; reg <= 0x3d1f; reg++)
		ov5640_write_reg(s->client, reg, 0x00);

	// 2. set 0x3d21 to 0x01
	ov5640_write_reg(s->client, 0x3d21, 0x01);
	// 3. wait 10 ms
	mdelay(10);
	// 4. set 0x3d21 to 0x00
	ov5640_write_reg(s->client, 0x3d21, 0x00);

	// 5. read out
	module_id = ov5640_read_reg(s->client, 0x3d05);
	switch (module_id) {
	case 0x00:
		s->cs.vendor = MODULE_VENDOR_TRULY;		//0x55  信利
		break;
	case 0x04:
		s->cs.vendor = MODULE_VENDOR_GLOBALOPTICS;	//0xaa  光阵
		break;
	default:
		dev_err(&s->client->dev, "Invalid Module ID: 0x%02x\n", module_id);
		ret = -EINVAL;
		break;
	};

	// 6. set 0x3d00 ~ 0x3d1f to 0x00
	for (reg = 0x3d00; reg <= 0x3d1f; reg++)
		ov5640_write_reg(s->client, reg, 0x00);

	if (!ret)
		dev_info(&s->client->dev, "Module ID: 0x%02x, Vendor: %s\n", module_id, module_id ? vendor_str[1] : vendor_str[0]);

	return ret;
}

//---------------------------------------------------
//		fix AE
//---------------------------------------------------
int ov5640_fix_ae(struct ov5640_sensor *s)
{
	u8 temp1,temp2,temp3,temp4,temp5,temp6,temp7,temp8,temp9,temp10,temp11,temp12,temp13,temp14,temp15,temp16;
	bool state1=false;
	bool state2=true;
	const u8 VAL = 0x48;
	int ret = 0;

	temp1 = ov5640_read_reg(s->client, 0x5691);
	temp2 = ov5640_read_reg(s->client, 0x5692);
	temp3 = ov5640_read_reg(s->client, 0x5693);
	temp4 = ov5640_read_reg(s->client, 0x5694);
	temp5 = ov5640_read_reg(s->client, 0x5695);
	temp6 = ov5640_read_reg(s->client, 0x5696);
	temp7 = ov5640_read_reg(s->client, 0x5697);
	temp8 = ov5640_read_reg(s->client, 0x5698);
	temp9 = ov5640_read_reg(s->client, 0x5699);
	temp10 = ov5640_read_reg(s->client, 0x569a);
	temp11 = ov5640_read_reg(s->client, 0x569b);
	temp12 = ov5640_read_reg(s->client, 0x569c);
	temp13 = ov5640_read_reg(s->client, 0x569d);
	temp14 = ov5640_read_reg(s->client, 0x569e);
	temp15 = ov5640_read_reg(s->client, 0x569f);
	temp16 = ov5640_read_reg(s->client, 0x56a0); 

	if ( (temp1>VAL && temp2>VAL) || (temp2>VAL && temp3>VAL) || (temp3>VAL && temp4>VAL) ) {
		if (state1==false && state2==true)
		{
			state1=true;
			state2=false;
			ret |= ov5640_write_reg(s->client, 0x3a0f, 0x30);
			ret |= ov5640_write_reg(s->client, 0x3a10, 0x28);
			ret |= ov5640_write_reg(s->client, 0x3a1b, 0x30);
			ret |= ov5640_write_reg(s->client, 0x3a1e, 0x28);
			ret |= ov5640_write_reg(s->client, 0x3a11, 0x51);
			ret |= ov5640_write_reg(s->client, 0x3a1f, 0x10);
		}
	} else if ( (temp5>VAL && temp6>VAL) || (temp6>VAL && temp7>VAL) || (temp7>VAL && temp8>VAL) ) {
		if ((state1==false && state2==true))
		{
			state1=true;
			state2=false;
			ret |= ov5640_write_reg(s->client, 0x3a0f, 0x30);
			ret |= ov5640_write_reg(s->client, 0x3a10, 0x28);
			ret |= ov5640_write_reg(s->client, 0x3a1b, 0x30);
			ret |= ov5640_write_reg(s->client, 0x3a1e, 0x28);
			ret |= ov5640_write_reg(s->client, 0x3a11, 0x51);
			ret |= ov5640_write_reg(s->client, 0x3a1f, 0x10);
		}
	} else if ( (temp9>VAL && temp10>VAL) || (temp10>VAL && temp11>VAL) || (temp11>VAL && temp12>VAL) ) {
		if ((state1==false && state2==true))
		{
			state1=true;
			state2=false;
			ret |= ov5640_write_reg(s->client, 0x3a0f, 0x30);
			ret |= ov5640_write_reg(s->client, 0x3a10, 0x28);
			ret |= ov5640_write_reg(s->client, 0x3a1b, 0x30);
			ret |= ov5640_write_reg(s->client, 0x3a1e, 0x28);
			ret |= ov5640_write_reg(s->client, 0x3a11, 0x51);
			ret |= ov5640_write_reg(s->client, 0x3a1f, 0x10);
		}
	} else if ( (temp13>VAL && temp14>VAL) || (temp14>VAL && temp15>VAL) || (temp15>VAL && temp16>VAL) ) {
		if ((state1==false && state2==true))
		{
			state1=true;
			state2=false;
			ret |= ov5640_write_reg(s->client, 0x3a0f, 0x30);
			ret |= ov5640_write_reg(s->client, 0x3a10, 0x28);
			ret |= ov5640_write_reg(s->client, 0x3a1b, 0x30);
			ret |= ov5640_write_reg(s->client, 0x3a1e, 0x28);
			ret |= ov5640_write_reg(s->client, 0x3a11, 0x51);
			ret |= ov5640_write_reg(s->client, 0x3a1f, 0x10);
		}
	} else {
		if ((state1==true && state2==false))
		{
			state1=false;
			state2=true;
			ret |= ov5640_write_reg(s->client, 0x3a0f, 0x38);
			ret |= ov5640_write_reg(s->client, 0x3a10, 0x28);
			ret |= ov5640_write_reg(s->client, 0x3a1b, 0x38);
			ret |= ov5640_write_reg(s->client, 0x3a1e, 0x28);
			ret |= ov5640_write_reg(s->client, 0x3a11, 0x61);
			ret |= ov5640_write_reg(s->client, 0x3a1f, 0x10);
		}
	}

	return ret;
}

#define OV5640_DEBUG_FS
#ifdef OV5640_DEBUG_FS
#include <linux/proc_fs.h>

static int camera_read_proc(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	//struct ov5640_sensor *s;
	u8 chipid_high;
	u8 chipid_low;
	u8 revision;
	int ret = 0;
	int camera_chipid;

	/*
	 * check and show chip ID
	 */
	chipid_high = ov5640_read_reg(g_client, 0x300a);
	chipid_low = ov5640_read_reg(g_client, 0x300b);
	revision = ov5640_read_reg(g_client, 0x302a);

	g_chipid = camera_chipid = chipid_high << 8 | chipid_low;

	if ( (camera_chipid != 0x5640) && (camera_chipid != 0x5642) ) {
		dev_err(&g_client->dev, "read sensor %s ID: 0x%x, Revision: 0x%x, failed!\n", g_client->name, camera_chipid, revision);
		ret = -1;
		return 0;
	}

	dev_err(&g_client->dev, "Sensor ID: 0x%x, Revision: 0x%x\n", camera_chipid, revision);


	ov5640_reg_reads2(g_client, ov5640_init_table);
	ov5640_reg_reads(g_client, ov5640_init_table);

	return 0;
}

static int camera_write_proc(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int ret;
	unsigned int flags;
	unsigned int reg;
	unsigned int val;
	char buf[32];

	if (count > 32)
		count = 32;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	ret = sscanf(buf,"%x: %x %x",&flags, &reg,&val);
	printk("flags=0x%x, reg = 0x%x, val =0x%x\n", flags, reg, val);
	if (flags == 0xff) {
		ov5640_write_reg(g_client,(unsigned short)reg, (unsigned char)val);
	}
	val = ov5640_read_reg(g_client, (unsigned short)reg);
	printk("================read val = 0x%x===================\n",val);

	return count;
}

static int __init init_camera_proc(void)
{
	struct proc_dir_entry *res;

	res = create_proc_entry("camera", 0666, NULL);
	if (res) {
		res->read_proc = camera_read_proc;
		res->write_proc = camera_write_proc;
		res->data = NULL;
	}
	return 0;
}
module_init(init_camera_proc);
#endif
