#ifndef __sp0838_SET_H__
#define __sp0838_SET_H__

int sp0838_preview_set(struct cim_sensor *sensor_info);
int sp0838_capture_set(struct cim_sensor *sensor_info);
int sp0838_size_switch(struct cim_sensor *sensor_info,int width,int height);

int sp0838_init(struct cim_sensor *sensor_info);
int sp0838_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int sp0838_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int sp0838_set_balance(struct cim_sensor *sensor_info,unsigned short arg);


#endif

