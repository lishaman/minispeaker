#ifndef __gc2015_SET_H__
#define __gc2015_SET_H__

int gc2015_preview_set(struct cim_sensor *sensor_info);
int gc2015_capture_set(struct cim_sensor *sensor_info);
int gc2015_size_switch(struct cim_sensor *sensor_info,int width,int height);

int gc2015_init(struct cim_sensor *sensor_info);
int gc2015_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int gc2015_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int gc2015_set_balance(struct cim_sensor *sensor_info,unsigned short arg);


#endif

