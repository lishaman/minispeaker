#ifndef __OV5640_SET_H__
#define __OV5640_SET_H__

extern int ov5640_reg_read(struct i2c_client *client, u16 reg, u8 *val);
extern int ov5640_reg_write(struct i2c_client *client, u16 reg, u8 val);

int ov5640_preview_set(struct cim_sensor *sensor_info);
int ov5640_capture_set(struct cim_sensor *sensor_info);
int ov5640_size_switch(struct cim_sensor *sensor_info,int width,int height);

int ov5640_init(struct cim_sensor *sensor_info);
int ov5640_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int ov5640_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int ov5640_set_balance(struct cim_sensor *sensor_info,unsigned short arg);
int ov5640_set_focus(struct cim_sensor *sensor_info, unsigned short arg);
int ov5640_set_scene(struct cim_sensor *sensor_info, unsigned short arg);
int ov5640_set_contrast(struct cim_sensor *sensor_info, unsigned short arg);
int ov5640_set_sharpness(struct cim_sensor *sensor_info, unsigned short arg);
int ov5640_set_saturation(struct cim_sensor *sensor_info, unsigned short arg);

int ov5640_read_otp(struct cim_sensor *sensor_info);
int ov5640_fix_ae(struct ov5640_sensor *s);

extern void ov5640_late_work(struct work_struct *work);

#endif

