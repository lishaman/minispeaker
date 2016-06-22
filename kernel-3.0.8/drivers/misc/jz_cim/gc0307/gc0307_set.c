#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>

#include "gc0307_camera.h"
#include "gc0307_set.h"

//#define OV3640_SET_KERNEL_PRINT

int gc0307_reg_writes(struct i2c_client *client, const struct gc0307_reg reglist[])
{
        int err = 0;
        int i = 0;
        while (reglist[i].reg != 0xff) {
                err = gc0307_write_reg(client, reglist[i].reg, reglist[i].val);
                if (err)
                        return err;
        //      printk("reg(0x%x)= 0x%x\n",reglist[i].reg,sensor_read_reg(client,reglist[i].reg));
                i++;
        }
        return 0;
}
int gc0307_init(struct cim_sensor *sensor_info)
{
	struct gc0307_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc0307_sensor, cs);
	client = s->client;

#ifdef GC0307_SET_KERNEL_PRINT
	dev_info(&client->dev,"gc0307 init\n");
#endif
	gc0307_reg_writes(client,gc0307_init_table);
        mdelay(20);
        gc0307_write_reg(client,0x43,0x40);
        gc0307_write_reg(client,0x44,0xe2);
        //gc0307_write_reg(client,0xf,0x82);
        //gc0307_write_reg(client,0x45,0x24);
        //gc0307_write_reg(client,0x47,0x20);		

	return 0;
}


static void enable_test_pattern(struct i2c_client *client)
{
}


int gc0307_preview_set(struct cim_sensor *sensor_info)
{

	struct gc0307_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc0307_sensor, cs);
	client = s->client;
//	unsigned char value;

	printk("%s %s()\n", __FILE__, __func__);
	return 0;
}


int gc0307_size_switch(struct cim_sensor *sensor_info,int width,int height)  //false
{
	struct gc0307_sensor *s;
	struct i2c_client * client ;
	char value;
	s = container_of(sensor_info, struct gc0307_sensor, cs);
	client = s->client;
	return 0;
}



int gc0307_capture_set(struct cim_sensor *sensor_info)
{

	struct gc0307_sensor *s;
	struct i2c_client * client ;
	printk("\n======>%s===\n",__func__);
	s = container_of(sensor_info, struct gc0307_sensor, cs);
	client = s->client;

	return 0;
}

void gc0307_set_ab_50hz(struct i2c_client *client)
{
}

void gc0307_set_ab_60hz(struct i2c_client *client)
{
}
int gc0307_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
	return 0;
}
void gc0307_set_effect_normal(struct i2c_client *client)
{
}

void gc0307_set_effect_grayscale(struct i2c_client *client)
{
}

void gc0307_set_effect_sepia(struct i2c_client *client)
{
}

void gc0307_set_effect_colorinv(struct i2c_client *client)
{
}

void gc0307_set_effect_sepiagreen(struct i2c_client *client)
{
}

void gc0307_set_effect_sepiablue(struct i2c_client *client)
{
}


int gc0307_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	return 0;
}

void gc0307_set_wb_auto(struct i2c_client *client)
{
}

void gc0307_set_wb_cloud(struct i2c_client *client)
{
}

void gc0307_set_wb_daylight(struct i2c_client *client)
{
}


void gc0307_set_wb_incandescence(struct i2c_client *client)
{
}

void gc0307_set_wb_fluorescent(struct i2c_client *client)
{
}

void gc0307_set_wb_tungsten(struct i2c_client *client)
{
}

int gc0307_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
	return 0;
}
