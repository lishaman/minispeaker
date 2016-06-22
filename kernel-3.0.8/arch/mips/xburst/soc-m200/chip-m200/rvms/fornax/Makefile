
obj-y += misc.o
obj-y += mmc.o
obj-$(CONFIG_REGULATOR_DA9024) += regulator.o
obj-$(CONFIG_REGULATOR_RICOH619) += board-power-619.o
obj-$(CONFIG_SOUND_OSS_XBURST)		    += sound.o
obj-$(CONFIG_VIDEO_OVISP) += camera.o

obj-$(CONFIG_BCM_PM_CORE) += bcm_power_control.o

obj-$(CONFIG_JZ_EPD_V12)	+= epd.o

ifdef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
obj-y += pm_slcd.o
else
obj-y += pm.o
endif
