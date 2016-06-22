#ifndef __BOARD_BASE_H__
#define __BOARD_BASE_H__

#include <board.h>
#include <linux/spi/spi.h>

#ifdef CONFIG_LEDS_GPIO
extern struct platform_device jz_led_rgb;
#endif

#ifdef CONFIG_JZ4775_INTERNAL_CODEC
extern struct snd_codec_data codec_data;
#endif

#ifdef CONFIG_MMC1_JZ4775
extern struct jzmmc_platform_data tf_pdata;
#endif
#ifdef CONFIG_MMC2_JZ4775
extern struct jzmmc_platform_data sdio_pdata;
#endif

#ifdef CONFIG_BROADCOM_RFKILL
extern struct platform_device	bt_power_device;
extern struct platform_device	bluesleep_device;
#endif

#ifdef CONFIG_USB_DWC2
extern struct platform_device   jz_dwc_otg_device;
#endif
#ifdef CONFIG_USB_OHCI_HCD
extern struct platform_device jz_ohci_device;
#endif

#ifdef CONFIG_SPI0_JZ47XX
extern struct jz47xx_spi_info spi0_info_cfg;
#endif
#ifdef CONFIG_JZ_SPI_NOR
extern struct spi_board_info jz_spi0_board_info[];
extern int jz_spi0_devs_size;
#endif
#ifdef CONFIG_SPI_GPIO
extern struct platform_device jz47xx_spi_gpio_device;
#endif

#ifdef CONFIG_JZ4775_EFUSE
extern struct jz_efuse_platform_data jz_efuse_pdata;
#endif

#ifdef CONFIG_I2C0_JZ4775
extern struct platform_device jz_i2c0_device;
#endif
#ifdef CONFIG_I2C1_JZ4775
extern struct platform_device jz_i2c1_device;
#endif
#ifdef CONFIG_I2C2_JZ4775
extern struct platform_device jz_i2c2_device;
#endif
#ifdef CONFIG_I2C3_JZ4775
extern struct platform_device jz_i2c3_device;
#endif
#ifdef CONFIG_MFD_JZ_SADC_V12
extern struct platform_device jz_adc_device;
#endif
#endif
