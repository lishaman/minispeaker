#ifndef __JZ_LED_RGB_H__
#define __JZ_LED_RGB_H__

#define JZ_LED_RGB_OFF		  0x00
#define JZ_LED_RGB_ON		  0x01
#define JZ_LED_RGB_SET		  0x02

#define JZ_LED_RGB_RED_BIT	  0x04
#define JZ_LED_RGB_GREEN_BIT  0x02
#define JZ_LED_RGB_BLUE_BIT	  0x01

struct jz_led_RGB_pdata{
	unsigned gpio_RGB_R;
	unsigned gpio_RGB_G;
	unsigned gpio_RGB_B;
};

#endif /* __JZ_LED_RGB_H__ */
