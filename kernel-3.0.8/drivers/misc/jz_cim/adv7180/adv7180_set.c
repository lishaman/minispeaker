 /*
  2  * adv7180.c Analog Devices ADV7180 video decoder driver
  3  * Copyright (c) 2009 Intel Corporation
  */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <mach/jz_cim.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <media/v4l2-ioctl.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <linux/mutex.h>

#include "adv7180_set.h"
#include "adv7180_reg.h"

extern void v4l2_device_unregister_subdev(struct v4l2_subdev *sd);

#define DRIVER_NAME "adv7180"

struct adv7180_state {
		struct v4l2_subdev      sd;
		struct work_struct      work;
		struct mutex            mutex; /* mutual excl. when accessing chip */
		int                     irq;
		v4l2_std_id             curr_norm;
		bool                    autodetect;
	};

static v4l2_std_id adv7180_std_to_v4l2(u8 status1)
{
	switch (status1 & ADV7180_STATUS1_AUTOD_MASK) {
	case ADV7180_STATUS1_AUTOD_NTSM_M_J:
			return V4L2_STD_NTSC;
	case ADV7180_STATUS1_AUTOD_NTSC_4_43:
			return V4L2_STD_NTSC_443;
	case ADV7180_STATUS1_AUTOD_PAL_M:
			return V4L2_STD_PAL_M;
	case ADV7180_STATUS1_AUTOD_PAL_60:
			return V4L2_STD_PAL_60;
	case ADV7180_STATUS1_AUTOD_PAL_B_G:
			return V4L2_STD_PAL;
	case ADV7180_STATUS1_AUTOD_SECAM:
			return V4L2_STD_SECAM;
	case ADV7180_STATUS1_AUTOD_PAL_COMB:
			return V4L2_STD_PAL_Nc | V4L2_STD_PAL_N;
	case ADV7180_STATUS1_AUTOD_SECAM_525:
			return V4L2_STD_SECAM;
	default:
			return V4L2_STD_UNKNOWN;
	}
}

static int v4l2_std_to_adv7180(v4l2_std_id std)
{
	if (std == V4L2_STD_PAL_60)
			return ADV7180_INPUT_CONTROL_PAL60;
	if (std == V4L2_STD_NTSC_443)
			return ADV7180_INPUT_CONTROL_NTSC_443;
	if (std == V4L2_STD_PAL_N)
			return ADV7180_INPUT_CONTROL_PAL_N;
	if (std == V4L2_STD_PAL_M)
			return ADV7180_INPUT_CONTROL_PAL_M;
	if (std == V4L2_STD_PAL_Nc)
			return ADV7180_INPUT_CONTROL_PAL_COMB_N;
	if (std & V4L2_STD_PAL)
			return ADV7180_INPUT_CONTROL_PAL_BG;
	if (std & V4L2_STD_NTSC)
			return ADV7180_INPUT_CONTROL_NTSC_M;
	if (std & V4L2_STD_SECAM)
			return ADV7180_INPUT_CONTROL_PAL_SECAM;

	return -EINVAL;
}

static u32 adv7180_status_to_v4l2(u8 status1)
{
	if (!(status1 & ADV7180_STATUS1_IN_LOCK))
			return V4L2_IN_ST_NO_SIGNAL;

	return 0;
}

static int __adv7180_status(struct i2c_client *client, u32 *status,
								v4l2_std_id *std)
{
	int status1 = i2c_smbus_read_byte_data(client, ADV7180_STATUS1_REG);

			if (status1 < 0)
			return status1;

			if (status)
			*status = adv7180_status_to_v4l2(status1);
			if (std)
			*std = adv7180_std_to_v4l2(status1);

			return 0;
}

static inline struct adv7180_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct adv7180_state, sd);
}

int adv7180_read(struct i2c_client *client, char *writebuf,
		    int writelen, char *readbuf, int readlen)
{
	int ret;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0){
			dev_err(&client->dev, "%s: -i2c read error.\n",
				__func__);
		}
	} else {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0){
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
		}
	}
	return ret;
}
int adv7180_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};
	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0){
		dev_err(&client->dev, "%s i2c write error.\n", __func__);
	}
	return ret;
}
int adv7180_set_reg(struct i2c_client *client, u8 addr, u8 para)
{
	u8 buf[3];
	int ret = -1;

	buf[0] = addr;
	buf[1] = para;
	ret = adv7180_write(client, buf, 2);
	if (ret < 0) {
		pr_err("write reg failed! %#x ret: %d", buf[0], ret);
		return -1;
	}
	return 0;
}




static int adv7180_querystd(struct v4l2_subdev *sd, v4l2_std_id *std)
{
	struct adv7180_state *state = to_state(sd);
	int err = mutex_lock_interruptible(&state->mutex);
	if (err)
		return err;
	/* when we are interrupt driven we know the state */
	if (!state->autodetect || state->irq > 0)
		*std = state->curr_norm;
	else
	err = __adv7180_status(v4l2_get_subdevdata(sd), NULL, std);

	mutex_unlock(&state->mutex);
	return err;
}

static int adv7180_g_input_status(struct v4l2_subdev *sd, u32 *status)
{
	struct adv7180_state *state = to_state(sd);
	int ret = mutex_lock_interruptible(&state->mutex);
	if (ret)
		return ret;

	ret = __adv7180_status(v4l2_get_subdevdata(sd), status, NULL);
	mutex_unlock(&state->mutex);
	return ret;
}

static int adv7180_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_ADV7180, 0);
}

static int adv7180_s_std(struct v4l2_subdev *sd, v4l2_std_id std)
{
	struct adv7180_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = mutex_lock_interruptible(&state->mutex);
	if (ret)
		return ret;

	/* all standards -> autodetect */
	if (std == V4L2_STD_ALL) {
	ret = i2c_smbus_write_byte_data(client,
	ADV7180_INPUT_CONTROL_REG,
	ADV7180_INPUT_CONTROL_AD_PAL_BG_NTSC_J_SECAM);
	if (ret < 0)
		goto out;

	__adv7180_status(client, NULL, &state->curr_norm);
	state->autodetect = true;
	} else {
		ret = v4l2_std_to_adv7180(std);
		if (ret < 0)
			goto out;

		ret = i2c_smbus_write_byte_data(client,
			ADV7180_INPUT_CONTROL_REG, ret);
		if (ret < 0)
			goto out;

			state->curr_norm = std;
			state->autodetect = false;
		}
		ret = 0;
	out:
		mutex_unlock(&state->mutex);
		return ret;
}

static const struct v4l2_subdev_video_ops adv7180_video_ops = {
	.querystd = adv7180_querystd,
	.g_input_status = adv7180_g_input_status,
};

static const struct v4l2_subdev_core_ops adv7180_core_ops = {
	.g_chip_ident = adv7180_g_chip_ident,
	.s_std = adv7180_s_std,
};

static const struct v4l2_subdev_ops adv7180_ops = {
	.core = &adv7180_core_ops,
	.video = &adv7180_video_ops,
};

static void adv7180_work(struct work_struct *work)
{
	struct adv7180_state *state = container_of(work, struct adv7180_state,
			work);
	struct i2c_client *client = v4l2_get_subdevdata(&state->sd);
			u8 isr3;

	mutex_lock(&state->mutex);
	i2c_smbus_write_byte_data(client, ADV7180_ADI_CTRL_REG,
			ADV7180_ADI_CTRL_IRQ_SPACE);
	isr3 = i2c_smbus_read_byte_data(client, ADV7180_ISR3_ADI);
	/* clear */
	i2c_smbus_write_byte_data(client, ADV7180_ICR3_ADI, isr3);
	i2c_smbus_write_byte_data(client, ADV7180_ADI_CTRL_REG, 0);

	if (isr3 & ADV7180_IRQ3_AD_CHANGE && state->autodetect)
		__adv7180_status(client, NULL, &state->curr_norm);
		mutex_unlock(&state->mutex);
		enable_irq(state->irq);
}

static irqreturn_t adv7180_irq(int irq, void *devid)
{
	struct adv7180_state *state = devid;
	schedule_work(&state->work);

	disable_irq_nosync(state->irq);
	return IRQ_HANDLED;
}

void size_switch(struct i2c_client *client,int width,int height) {

}
void preview_set(struct i2c_client *client) {
	printk("==>%s L%d$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n", __func__, __LINE__);
}
void capture_set(struct i2c_client *client) {
	printk("==>%s L%d##############################################\n", __func__, __LINE__);
}

/*
* Generic i2c probe
* concerning the addresses: i2c wants 7 bit (without the r/w bit), so '>>1'
*/

int adv7180_probe(struct i2c_client *client)
{
		struct adv7180_state *state;
		struct v4l2_subdev *sd;
		int ret;

		printk("adv7180_probe!!!\n");
		/* Check if the adapter supports the needed features */
		if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
				return -EIO;

		v4l_info(client, "chip found @ 0x%02x (%s)\n",
						client->addr << 1, client->adapter->name);

		state = kzalloc(sizeof(struct adv7180_state), GFP_KERNEL);
		if (state == NULL) {
		ret = -ENOMEM;
		goto err;
		}

		state->irq = client->irq;
		INIT_WORK(&state->work, adv7180_work);
		mutex_init(&state->mutex);
		state->autodetect = true;
		sd = &state->sd;
//		v4l2_i2c_subdev_init(sd, client, &adv7180_ops);
		/* Initialize adv7180 */
		/* Enable autodetection */
		ret = i2c_smbus_write_byte_data(client, ADV7180_INPUT_CONTROL_REG,
		ADV7180_INPUT_CONTROL_AD_PAL_BG_NTSC_J_SECAM);
		if (ret < 0)
		goto err_unreg_subdev;

		ret = i2c_smbus_write_byte_data(client, ADV7180_AUTODETECT_ENABLE_REG,
						ADV7180_AUTODETECT_DEFAULT);
		if (ret < 0)
		goto err_unreg_subdev;

		/* ITU-R BT.656-4 compatible */
		ret = i2c_smbus_write_byte_data(client,
		ADV7180_EXTENDED_OUTPUT_CONTROL_REG,
		ADV7180_EXTENDED_OUTPUT_CONTROL_NTSCDIS);
		if (ret < 0)
		goto err_unreg_subdev;

		/* read current norm */
		__adv7180_status(client, NULL, &state->curr_norm);

		/* register for interrupts */
		if (state->irq > 0) {
				ret = request_irq(state->irq, adv7180_irq, 0, DRIVER_NAME,
						state);
				if (ret)
						goto err_unreg_subdev;

				ret = i2c_smbus_write_byte_data(client, ADV7180_ADI_CTRL_REG,
						ADV7180_ADI_CTRL_IRQ_SPACE);
				if (ret < 0)
						goto err_unreg_subdev;

				/* config the Interrupt pin to be active low */
				ret = i2c_smbus_write_byte_data(client, ADV7180_ICONF1_ADI,
						ADV7180_ICONF1_ACTIVE_LOW | ADV7180_ICONF1_PSYNC_ONLY);
				if (ret < 0)
						goto err_unreg_subdev;

				ret = i2c_smbus_write_byte_data(client, ADV7180_IMR1_ADI, 0);
				if (ret < 0)
						goto err_unreg_subdev;

				ret = i2c_smbus_write_byte_data(client, ADV7180_IMR2_ADI, 0);
				if (ret < 0)
						goto err_unreg_subdev;

				/* enable AD change interrupts interrupts */
				ret = i2c_smbus_write_byte_data(client, ADV7180_IMR3_ADI,
						ADV7180_IRQ3_AD_CHANGE);
				if (ret < 0)
						goto err_unreg_subdev;

				ret = i2c_smbus_write_byte_data(client, ADV7180_IMR4_ADI, 0);
				if (ret < 0)
						goto err_unreg_subdev;

				ret = i2c_smbus_write_byte_data(client, ADV7180_ADI_CTRL_REG,
						0);
				if (ret < 0)
				goto err_unreg_subdev;
		}
	return 0;

	err_unreg_subdev:
			mutex_destroy(&state->mutex);
//			v4l2_device_unregister_subdev(sd);
			kfree(state);
	err:
//			printk(KERN_ERR DRIVER_NAME ": Failed to probe: %d\n", ret);
			return ret;

}


int adv7180_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct adv7180_state *state = to_state(sd);

	if (state->irq > 0) {
	free_irq(client->irq, state);
	if (cancel_work_sync(&state->work)) {
		/*
		* Work was pending, therefore we need to enable
		* IRQ here to balance the disable_irq() done in the
		* interrupt handler.
		*/
	enable_irq(state->irq);
	}
	}
/*
		mutex_destroy(&state->mutex);
		v4l2_device_unregister_subdev(sd);
		kfree(to_state(sd));
*/
		return 0;
	}

static const struct i2c_device_id adv7180_id[] = {
	{DRIVER_NAME, 0},
	{},
};




