#ifndef __OV2650_SET_H__
#define __OV2650_SET_H__

int ov2650_preview_set(struct cim_sensor *sensor_info);
int ov2650_capture_set(struct cim_sensor *sensor_info);
int ov2650_size_switch(struct cim_sensor *sensor_info,int width,int height);

int ov2650_init(struct cim_sensor *sensor_info);
int ov2650_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int ov2650_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int ov2650_set_balance(struct cim_sensor *sensor_info,unsigned short arg);


#endif

