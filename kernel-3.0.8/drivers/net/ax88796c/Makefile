#
# Makefile for ax88796C ethernet driver
#

# AX88796C DMA and plat Init
obj-$(CONFIG_ARCH_S3C2410)	+= s3c2410_plat_dma.o
obj-$(CONFIG_SOC_4780)		+= jz4780_plat_dma.o

# AX88796C driver
obj-$(CONFIG_AX88796C)		+= ax88796c.o

ax88796c-objs := ax88796c_main.o ax88796c_ioctl.o dummy_plat_dma.o
