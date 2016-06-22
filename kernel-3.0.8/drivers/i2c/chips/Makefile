#
# Makefile for miscellaneous I2C chip drivers.
#
# Do not add new drivers to this directory! It is DEPRECATED.
#
# Device drivers are better grouped according to the functionality they
# implement rather than to the bus they are connected to. In particular:
# * Hardware monitoring chip drivers go to drivers/hwmon
# * RTC chip drivers go to drivers/rtc
# * I/O expander drivers go to drivers/gpio
#

obj-$(CONFIG_SENSORS_MMA8452)	+= gsensor_mma8452.o
obj-$(CONFIG_SENSORS_MC32X0)	+= gsensor_mc32x0.o
obj-$(CONFIG_SENSORS_LIS3DH)	+= gsensor_lis3dh.o
obj-$(CONFIG_SENSORS_DMARD06)	+= gsensor_dmard06.o
obj-$(CONFIG_SENSORS_STK220X)	+= lsensor_stk220x.o
obj-$(CONFIG_SENSORS_BMA250E)   += bma250e.o
obj-$(CONFIG_SENSORS_EM7180)    += em7180.o pni_sentral_ram.o
obj-$(CONFIG_SENSORS_ORI)	+= gsensor_ori.o
obj-y				+= gsensor_bma223/

ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif

