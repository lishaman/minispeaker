/*
 * linux/drivers/misc/camera_source/adv7180/camera.h -- Ingenic CIM driver
 *
 * Copyright (C) 2005-2010, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef adv7180_CAMERA_H
#define adv7180_CAMERA_H


#define CAM_CLOCK						24000000

struct cam_sensor_plat_data {
        int facing;
        int orientation;
        int mirror;   //camera mirror
        //u16   gpio_vcc;       /* vcc enable gpio */   remove the gpio_vcc   , DO NOT use this pin for sensor power up ,cim will controls this
        uint16_t        gpio_rst;       /* resert  gpio */
        uint16_t        gpio_en;        /* camera enable gpio */
        int cap_wait_frame;   /* filter n frames when capture image */
};

struct adv7180_sensor {

        struct cim_sensor cs;
        struct i2c_client *client;
        //struct ov3640_platform_data *pdata;
        uint16_t        gpio_rst;       /* resert  gpio */
        uint16_t        gpio_en;        /* camera enable gpio */
        int mirror;   //camera mirror

        char name[8];

        unsigned int width;
        unsigned int height;
};

#endif
