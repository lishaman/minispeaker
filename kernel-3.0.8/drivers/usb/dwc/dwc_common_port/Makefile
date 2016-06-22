#
# Makefile for DWC_common library
#

EXTRA_CFLAGS	+= -DDWC_LINUX
EXTRA_CFLAGS	+= -DDEBUG
EXTRA_CFLAGS	+= -DDWC_DEBUG_REGS
EXTRA_CFLAGS	+= -DDWC_DEBUG_MEMORY

EXTRA_CFLAGS	+= -DDWC_LIBMODULE
EXTRA_CFLAGS	+= -DDWC_CCLIB
EXTRA_CFLAGS	+= -DDWC_NOTIFYLIB
EXTRA_CFLAGS	+= -DDWC_UTFLIB
#EXTRA_CFLAGS	+= -DDWC_CRYPTOLIB  // dwc driver doesn't used and has a complie bug which depends on _divdid3

obj-$(CONFIG_USB_DWC_OTG)			 := dwc_common_port_lib.o
dwc_common_port_lib-objs += dwc_cc.o dwc_modpow.o dwc_dh.o \
			    dwc_crypto.o dwc_notifier.o \
			    dwc_common_linux.o dwc_mem.o
