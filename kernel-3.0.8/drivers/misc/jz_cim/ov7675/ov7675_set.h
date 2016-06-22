#ifndef __OV7675_SET_H__
#define __OV7675_SET_H__

int ov7675_preview_set(struct cim_sensor *sensor_info);
int ov7675_capture_set(struct cim_sensor *sensor_info);
int ov7675_size_switch(struct cim_sensor *sensor_info,int width,int height);

int ov7675_init(struct cim_sensor *sensor_info);
int ov7675_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int ov7675_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int ov7675_set_balance(struct cim_sensor *sensor_info,unsigned short arg);


#endif

