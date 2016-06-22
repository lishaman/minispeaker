#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>
#include <linux/jz_dwc.h>

/* PMU ricoh619 */
#ifdef CONFIG_REGULATOR_RICOH619
#define PMU_IRQ_N		GPIO_PA(3)
#endif /* CONFIG_REGULATOR_RICOH619 */

/* pmu d2041 or 9024 gpio def*/
#define GPIO_GSENSOR_INT1       GPIO_PA(15)

#ifndef CONFIG_BOARD_NAME
#define CONFIG_BOARD_NAME "doorbell"
#endif

extern struct jzmmc_platform_data inand_pdata;
extern struct jzmmc_platform_data tf_pdata;
extern struct jzmmc_platform_data sdio_pdata;

/*KEY gpio*/
#define GPIO_FUNCTION            GPIO_PA(30)
#define ACTIVE_LOW_FUNCTION      1


#define GPIO_HP_MUTE		-1	/*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL		-1		/*vaild level*/

//#define GPIO_SPEAKER_EN			-1/*speaker enable gpio*/
//#define GPIO_SPEAKER_EN_LEVEL	-1
#define GPIO_SPEAKER_EN			GPIO_PE(0)/*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL	1

#define GPIO_HANDSET_EN		  -1		/*handset enable gpio*/
#define GPIO_HANDSET_EN_LEVEL -1

#define	GPIO_HP_DETECT	-1		/*hp detect gpio*/
#define GPIO_HP_INSERT_LEVEL    1
#define GPIO_MIC_SELECT		-1		/*mic select gpio*/
#define GPIO_BUILDIN_MIC_LEVEL	-1		/*builin mic select level*/
#define GPIO_MIC_DETECT		-1
#define GPIO_MIC_INSERT_LEVEL -1
#define GPIO_MIC_DETECT_EN		-1  /*mic detect enable gpio*/
#define GPIO_MIC_DETECT_EN_LEVEL	-1 /*mic detect enable gpio*/

/* wifi gpio */
#define HOST_WAKE_WL	GPIO_PA(10)
#define WL_WAKE_HOST	GPIO_PA(9)
#define GPIO_WIFI_RST_N	GPIO_PA(8)

/**
 * USB detect pin
 **/
#define GPIO_USB_ID			GPIO_PA(14)
#define GPIO_USB_ID_LEVEL		LOW_ENABLE
#define GPIO_USB_DETE			GPIO_PE(31)
#define GPIO_USB_DETE_LEVEL		HIGH_ENABLE
#define GPIO_USB_DRVVBUS		GPIO_PE(10)
#define GPIO_USB_DRVVBUS_LEVEL		HIGH_ENABLE

extern struct ovisp_camera_platform_data ovisp_camera_info;

/**
 * sound platform data
 **/
extern struct snd_codec_data codec_data;

#endif /* __BOARD_H__ */
