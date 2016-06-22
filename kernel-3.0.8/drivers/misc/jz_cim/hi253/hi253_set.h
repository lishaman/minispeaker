#ifndef __HI253_SET_H__
#define __HI253_SET_H__

int hi253_preview_set(struct cim_sensor *sensor_info);
int hi253_capture_set(struct cim_sensor *sensor_info);
int hi253_size_switch(struct cim_sensor *sensor_info,int width,int height);

int hi253_init(struct cim_sensor *sensor_info);
int hi253_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int hi253_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int hi253_set_balance(struct cim_sensor *sensor_info,unsigned short arg);


#endif

