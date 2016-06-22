#ifndef  __MC32X0_H__
#define  __MC32X0_H__

#define MC32X0_NAME	"gsensor_mc32x0"

enum {
	MC32X0_2G = 0,
	MC32X0_4G,
	MC32X0_8G_10BITS,
	MC32X0_8G_14BITS,
};

enum {
	MC32X0_AXIS_X = 0,
	MC32X0_AXIS_Y,
	MC32X0_AXIS_Z,
};

struct mc32x0_cali_data {
	short x_offset;
	short y_offset;
	short z_offset;
};

#endif
