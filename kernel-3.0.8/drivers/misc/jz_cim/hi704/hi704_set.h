#ifndef __HI704_SET_H__
#define __HI704_SET_H__

int hi704_preview_set(struct cim_sensor *sensor_info);
int hi704_capture_set(struct cim_sensor *sensor_info);
int hi704_size_switch(struct cim_sensor *sensor_info,int width,int height);

int hi704_init(struct cim_sensor *sensor_info);
int hi704_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int hi704_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int hi704_set_balance(struct cim_sensor *sensor_info,unsigned short arg);


#endif

