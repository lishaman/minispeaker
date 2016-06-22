CC = mipsel-linux-gcc
STRIP = mipsel-linux-strip

MOLIB_DIR=../../../output/molib
MOLIB_UPDATER_DIR=$(MOLIB_DIR)/updater
MOLIB_APPFS_DIR=$(MOLIB_DIR)/app

CFLAGS = -fPIC -Wall -Werror -g -O2 -I. -I$(MOLIB_DIR)/usr/include -I$(MOLIB_UPDATER_DIR)/usr/include
LDFLAGS = -g -L$(MOLIB_UPDATER_DIR)/usr/lib/ -L$(MOLIB_APPFS_DIR)/usr/lib/ -lpthread -lutils -lini -ljson-c

atalk_test: atalk_test.c atalk_control.c
	$(CC) $^ -o $@  $(CFLAGS) $(LDFLAGS)
