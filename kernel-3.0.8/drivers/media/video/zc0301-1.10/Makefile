#############################################################################
# Makefile for 'ZC0301[P] Image Processor and Control Chip' driver for      #
# Linux                                                                     #
#                                                                           #
#  Copyright (C) 2006-2007 by Luca Risolia <luca.risolia@studio.unibo.it>   #
#                                                                           #
#  This program is free software; you can redistribute it and/or modify     #
#  it under the terms of the GNU General Public License as published by     #
#  the Free Software Foundation; either version 2 of the License, or        #
#  (at your option) any later version.                                      #
#                                                                           #
#  This program is distributed in the hope that it will be useful,          #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of           #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            #
#  GNU General Public License for more details.                             #
#                                                                           #
#  You should have received a copy of the GNU General Public License        #
#  along with this program; if not, write to the Free Software              #
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                #
#############################################################################

DRIVER_VERSION := 1.10

##############################################################################

KVERSION := `uname -r`
KDIR := /lib/modules/$(KVERSION)/build
PWD := $(shell pwd)
INSTALL_MOD_DIR := kernel/drivers/media/video/zc0301/

default: modules

modules:
	@echo '**************************************************************************'
	@echo '* Building Video4Linux2 driver for                                       *'
	@echo '*          ZC0301 and ZC0301P Image Processor and Control Chip...        *'
	@echo '* Official Linux 2.6.19 is the minimum version for this driver.          *'
	@echo '* Read the documentation "zc0301.txt" for more informations.             *'
	@echo '* Type "make help" for a list of available targets.                      *'
	@echo '**************************************************************************'
	@echo
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install: modules_install

modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_DIR=$(INSTALL_MOD_DIR) modules_install

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf Module.symvers

##############################################################################

help:
	@echo 'List of available targets. Type:'
	@echo '- "make modules" to build the modules'
	@echo '- "make modules_install" to install the built modules'
	@echo '- "make clean" to remove all generated files in the current directory'
	@echo '- "make tar" to create and place an archive of the current directory in $(TARFILE)'
	@echo '- "make help" to print the list of available targets'

##############################################################################

TARFILE :=../zc0301-$(DRIVER_VERSION).tar.gz
DIR := ../
FILE := zc0301-$(DRIVER_VERSION)

tar:
	tar -choz --verbose -C $(DIR) --file=$(TARFILE) $(FILE)
