#ifndef __gt2005_SET_H
#define __gt2005_SET_H

int gt2005_preview_set(struct cim_sensor *sensor_info);
int gt2005_capture_set(struct cim_sensor *sensor_info);
int gt2005_size_switch(struct cim_sensor *sensor_info,int width,int height);

int gt2005_init(struct cim_sensor *sensor_info);
int gt2005_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int gt2005_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int gt2005_set_balance(struct cim_sensor *sensor_info,unsigned short arg);

#endif
