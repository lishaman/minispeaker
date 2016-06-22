#ifndef __ov2659_SET_H__
#define __ov2659_SET_H__

struct reginfo
{
    u16 reg;
    u8 val;
};

int ov2659_preview_set(struct cim_sensor *sensor_info);
int ov2659_capture_set(struct cim_sensor *sensor_info);
int ov2659_size_switch(struct cim_sensor *sensor_info,int width,int height);

int ov2659_init(struct cim_sensor *sensor_info);
int ov2659_set_effect(struct cim_sensor *sensor_info,unsigned short arg);
int ov2659_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg);
int ov2659_set_balance(struct cim_sensor *sensor_info,unsigned short arg);


#endif

