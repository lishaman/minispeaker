#ifndef _BF3116_H_
#define _BF3116_H_
int bf3116_read(struct v4l2_subdev *sd, unsigned char reg, unsigned char *value);
int bf3116_write(struct v4l2_subdev *sd, unsigned char reg, unsigned char value);
#endif
