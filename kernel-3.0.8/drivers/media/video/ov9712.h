#ifndef _OV9712_H_
#define _OV9712_H_
int ov9712_read(struct v4l2_subdev *sd, unsigned char reg, unsigned char *value);
int ov9712_write(struct v4l2_subdev *sd, unsigned char reg, unsigned char value);
#endif
