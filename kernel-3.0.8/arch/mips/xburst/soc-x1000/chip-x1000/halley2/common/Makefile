
obj-y += board_base.o misc.o
obj-$(CONFIG_INPUT_KEYBOARD)            += keyboard_gpio.o
obj-$(CONFIG_SOUND) 			+= sound.o
obj-$(CONFIG_JZ_SPI) 			+= spi_bus.o
obj-$(CONFIG_JZ_SFC) 			+= spi_bus.o
obj-$(CONFIG_SPI_GPIO) 			+= spi_bus.o
obj-$(CONFIG_JZMMC_V12) 		+= mmc.o
obj-$(CONFIG_I2C_GPIO)			+= i2c_bus.o
obj-$(CONFIG_I2C0_V12_JZ)		+= i2c_bus.o
obj-$(CONFIG_I2C1_V12_JZ)		+= i2c_bus.o
obj-$(CONFIG_I2C2_V12_JZ)		+= i2c_bus.o
ifeq ($(CONFIG_SSV6XXX), m)
	obj-y				+= ssv6051_wlan_power_control.o
endif
obj-y += lcd/
