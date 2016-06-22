#ifndef __tvp5150_SET_H__
#define __tvp5150_SET_H__
int tvp5150_log_status(struct i2c_client * sd);
int tvp5150_preview_set(struct cim_sensor *sensor_info);
int tvp5150_capture_set(struct cim_sensor *sensor_info);
int tvp5150_size_switch(struct cim_sensor *sensor_info,int width,int height);

int tvp5150_init(struct cim_sensor *sensor_info);
int tvp5150_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int tvp5150_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int tvp5150_set_balance(struct cim_sensor *sensor_info,unsigned short arg);


#endif

