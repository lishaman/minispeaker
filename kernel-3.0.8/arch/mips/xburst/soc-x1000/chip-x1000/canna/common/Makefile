
obj-y += board_base.o misc.o

obj-$(CONFIG_SOUND) 			+= sound.o
obj-$(CONFIG_JZ_SPI) 			+= spi_bus.o
obj-$(CONFIG_JZ_SFC) 			+= spi_bus.o
obj-$(CONFIG_SPI_GPIO) 			+= spi_bus.o
obj-$(CONFIG_JZMMC_V12) 		+= mmc.o
obj-$(CONFIG_BCMDHD_1_141_66)		+= ap6212_wlan_power_control.o ap6212_wlan_device.o
obj-$(CONFIG_BCM4018X)    		+= ap6181_wlan_power_control.o ap6181_wlan_device.o
obj-$(CONFIG_RTL8189FS)    		+= rtl8189fs_wlan_power_control.o rtl8189fs_wlan_device.o
obj-$(CONFIG_BCMDHD_43455) 		+= ap6255_wlan_power_control.o ap6255_wlan_device.o
obj-$(CONFIG_BCM_AP6212_RFKILL)		+= ap6212_bt_power_control.o ap6212_wlan_power_control.o
obj-$(CONFIG_BCM_AP6255_RFKILL)		+= ap6255_bt_power_control.o ap6255_wlan_power_control.o
obj-$(CONFIG_I2C_GPIO)			+= i2c_bus.o
obj-$(CONFIG_I2C0_V12_JZ)		+= i2c_bus.o
obj-$(CONFIG_I2C1_V12_JZ)		+= i2c_bus.o
obj-$(CONFIG_I2C2_V12_JZ)		+= i2c_bus.o
ifeq ($(CONFIG_SSV6XXX), m)
	obj-y				+= ssv6051_wlan_power_control.o
endif
obj-y += lcd/
