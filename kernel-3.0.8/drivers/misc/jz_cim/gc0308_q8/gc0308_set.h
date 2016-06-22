#ifndef __gc0308_SET_H__
#define __gc0308_SET_H__

int gc0308_preview_set(struct cim_sensor *sensor_info);
int gc0308_capture_set(struct cim_sensor *sensor_info);
int gc0308_size_switch(struct cim_sensor *sensor_info,int width,int height);

int gc0308_init(struct cim_sensor *sensor_info);
int gc0308_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int gc0308_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int gc0308_set_balance(struct cim_sensor *sensor_info,unsigned short arg);


#endif

