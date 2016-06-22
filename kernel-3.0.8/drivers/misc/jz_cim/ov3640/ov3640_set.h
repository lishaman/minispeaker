#ifndef __ov3640_SET_H__
#define __ov3640_SET_H__

int ov3640_preview_set(struct cim_sensor *sensor_info);
int ov3640_capture_set(struct cim_sensor *sensor_info);
int ov3640_size_switch(struct cim_sensor *sensor_info,int width,int height);

int ov3640_init(struct cim_sensor *sensor_info);
int ov3640_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int ov3640_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int ov3640_set_balance(struct cim_sensor *sensor_info,unsigned short arg);


#endif
