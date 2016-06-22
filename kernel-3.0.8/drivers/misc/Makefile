#
# Makefile for misc devices that really don't fit anywhere else.
#

EXTRA_CFLAGS    += -Idrivers/nand/manager/inc/ \
		   -Idrivers/nand/manager/inc/linux/ \
		   -Idrivers/nand/manager/lib/inc/  \
		   -Idrivers/nand/inc/

obj-$(CONFIG_IBM_ASM)		+= ibmasm/
obj-$(CONFIG_AD525X_DPOT)	+= ad525x_dpot.o
obj-$(CONFIG_AD525X_DPOT_I2C)	+= ad525x_dpot-i2c.o
obj-$(CONFIG_AD525X_DPOT_SPI)	+= ad525x_dpot-spi.o
0bj-$(CONFIG_INTEL_MID_PTI)	+= pti.o
obj-$(CONFIG_ATMEL_PWM)		+= atmel_pwm.o
obj-$(CONFIG_ATMEL_SSC)		+= atmel-ssc.o
obj-$(CONFIG_ATMEL_TCLIB)	+= atmel_tclib.o
obj-$(CONFIG_BMP085)		+= bmp085.o
obj-$(CONFIG_ICS932S401)	+= ics932s401.o
obj-$(CONFIG_LKDTM)		+= lkdtm.o
obj-$(CONFIG_TIFM_CORE)       	+= tifm_core.o
obj-$(CONFIG_TIFM_7XX1)       	+= tifm_7xx1.o
obj-$(CONFIG_PHANTOM)		+= phantom.o
obj-$(CONFIG_SENSORS_BH1780)	+= bh1780gli.o
obj-$(CONFIG_SENSORS_BH1770)	+= bh1770glc.o
obj-$(CONFIG_SENSORS_APDS990X)	+= apds990x.o
obj-$(CONFIG_ANDROID_PMEM)	+= pmem.o
obj-$(CONFIG_SGI_IOC4)		+= ioc4.o
obj-$(CONFIG_ENCLOSURE_SERVICES) += enclosure.o
obj-$(CONFIG_KGDB_TESTS)	+= kgdbts.o
obj-$(CONFIG_SGI_XP)		+= sgi-xp/
obj-$(CONFIG_SGI_GRU)		+= sgi-gru/
obj-$(CONFIG_CS5535_MFGPT)	+= cs5535-mfgpt.o
obj-$(CONFIG_HP_ILO)		+= hpilo.o
obj-$(CONFIG_APDS9802ALS)	+= apds9802als.o
obj-$(CONFIG_ISL29003)		+= isl29003.o
obj-$(CONFIG_ISL29020)		+= isl29020.o
obj-$(CONFIG_SENSORS_TSL2550)	+= tsl2550.o
obj-$(CONFIG_EP93XX_PWM)	+= ep93xx_pwm.o
obj-$(CONFIG_DS1682)		+= ds1682.o
obj-$(CONFIG_TI_DAC7512)	+= ti_dac7512.o
obj-$(CONFIG_UID_STAT)		+= uid_stat.o
obj-$(CONFIG_C2PORT)		+= c2port/
obj-$(CONFIG_IWMC3200TOP)      += iwmc3200top/
obj-$(CONFIG_HMC6352)		+= hmc6352.o
obj-y				+= eeprom/
obj-y				+= cb710/
obj-$(CONFIG_SPEAR13XX_PCIE_GADGET)	+= spear13xx_pcie_gadget.o
obj-$(CONFIG_VMWARE_BALLOON)	+= vmw_balloon.o
obj-$(CONFIG_ARM_CHARLCD)	+= arm-charlcd.o
obj-$(CONFIG_PCH_PHUB)		+= pch_phub.o
obj-y				+= ti-st/
obj-$(CONFIG_AB8500_PWM)	+= ab8500-pwm.o
obj-y				+= lis3lv02d/
obj-y				+= carma/
obj-$(CONFIG_WL127X_RFKILL)	+= wl127x-rfkill.o
obj-$(CONFIG_BCM_AP6212_RFKILL) += bt-power_bluesleep.o
obj-$(CONFIG_BCM4330_RFKILL) 	+= bt-power.o
obj-$(CONFIG_BCM_PM_CORE) 	+= bcm-pm-core.o
obj-$(CONFIG_BROADCOM_RFKILL)   += bt-power-bluesleep.o bluesleep.o
obj-$(CONFIG_GPS_RFKILL) 	+= gps-power.o
obj-$(CONFIG_APANIC)		+= apanic.o nand2mtd.o
obj-$(CONFIG_SENSORS_AK8975)	+= akm8975.o
obj-y				+= jz_cim/
obj-$(CONFIG_JZ_X2D)    	+= jz_x2d/
obj-$(CONFIG_JZ_DMMU)           += jz_dmmu/
obj-$(CONFIG_JZ_PWM)            += jz_pwm.o
obj-$(CONFIG_JZ_MODEM)		+= jz_modem/
obj-$(CONFIG_JZ_SPI_NOR)        += jz_spi_nor/
obj-$(CONFIG_JZ4780_EFUSE)	+= jz4780_efuse.o
obj-$(CONFIG_JZ4775_EFUSE)	+= jz4775_efuse.o
obj-$(CONFIG_JZ_EFUSE_V12)	+= jz_efuse_v12.o
obj-$(CONFIG_JZ_EFUSE_V13)	+= jz_efuse_v13.o
obj-$(CONFIG_MISC_BMA250E)	+= bma250e.o
obj-$(CONFIG_JZ_LED_RGB)           += jz_ledrgb.o
obj-$(CONFIG_FP8102_DET)     += fp8102_det.o

obj-$(CONFIG_TD_SC8800S)        += sc8800s.o
obj-$(CONFIG_JZ_SPI_NOR)        += jz_spi_nor/
