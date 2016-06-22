
#ifndef _CSENSOR_H
#define _CSENSOR_H
struct cam_sensor_plat_data {
	int facing;
	int orientation;
	int mirror;   //camera mirror
	uint16_t	gpio_rst;	/* resert  gpio */
	uint16_t	gpio_en;	/* camera enable gpio */
	int cap_wait_frame;    /* filter n frames when capture image */
};
#endif
