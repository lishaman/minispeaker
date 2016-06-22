# ----------------------------------------------------------------------------
#
# Copyright (C) 2008 Ingenic Semiconductor Co., Ltd.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# Just a invoke entry, and will include each .mak file if needed
#
# Version,	Date, 		Engineer,	Mark
# v1.0, 	2014-04-13,	bliu,		Init version
#
# ----------------------------------------------------------------------------

BUILD_DIR := .

CURDIR	:= $(BUILD_DIR)

SRCTREE	:= $(CURDIR)

CROSS_COMPILE	:= mips-linux-gnu-


CC      := $(CROSS_COMPILE)gcc
OBJCOPY	:= $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
NM	:= $(CROSS_COMPILE)nm

DFLAGS	+=
CFLAGS	:= -mips32 -O2 -EL -Wall -static

SOBJS	:= $(SRCTREE)/memcpy.o $(SRCTREE)/memset.o
COBJS	:= $(SRCTREE)/ddr_cpy_rd_wr.o

SRCS	:= $(addprefix $(src),$(SOBJS:.o=.S) $(COBJS:.o=.c))
OBJS	:= $(SOBJS) $(COBJS)

APP	:= ddr_cpy_rd_wr

all:	$(APP)

$(APP):	$(OBJS)
	$(CC)  $(CFLAGS) -o $@ $(OBJS)


$(SRCTREE)/%.o: $(SRCTREE)/%.c
	$(CC) $(DFLAGS) $(CFLAGS) -o $@ -c $<

$(SRCTREE)/%.o: $(SRCTREE)/%.S
	$(CC)  $(DFLAGS) $(CFLAGS) -D_ASSEMBLER_ -D__ASSEMBLY__  -o $@ -c $<

clean:
	rm -fr *.o $(APP) $(OBJS)
