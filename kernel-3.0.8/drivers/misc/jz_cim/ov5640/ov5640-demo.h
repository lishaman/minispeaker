
#ifndef __OV5640_DEMO_H__
#define __OV5640_DEMO_H__

extern int ov5640_reg_read(struct i2c_client *client, u16 reg, u8 *val);
extern int ov5640_reg_write(struct i2c_client *client, u16 reg, u8 val);
extern int ov5640_reg_writes(struct i2c_client *client, const struct ov5640_reg reglist[]);

extern int OV5640_capture(struct i2c_client *client);
extern int OV5640_video(struct i2c_client *client);
extern int OV5640_return_to_preview(struct i2c_client *client);
extern int OV5640_preview(struct i2c_client *client);
extern int OV5640_init(struct i2c_client *client);
extern void OV5640_set_bandingfilter(struct i2c_client *client, int light_freq);
extern int OV5640_af_init(struct i2c_client *client);
extern int OV5640_auto_focus(struct i2c_client *client);

#endif /* __OV5640_DEMO_H__ */
