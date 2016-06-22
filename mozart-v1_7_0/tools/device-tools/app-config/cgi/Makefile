CC = mipsel-linux-gcc
CXX= mipsel-linux-g++
STRIP = mipsel-linux-strip

CFLAGS = -I$(MOLIB_DIR)/usr/include -I$(MOLIB_UPDATER_DIR)/usr/include
LDFLAGS = -L$(MOLIB_DIR)/usr/lib -L$(MOLIB_UPDATER_DIR)/usr/lib
LDFLAGS += -Wl,-rpath-link,$(MOLIB_DIR)/usr/lib:$(MOLIB_UPDATER_DIR)/usr/lib

CFLAGS += -g -Wall -O2 -I$(MOZART_DIR)/usr/include
LDFLAGS += -lpthread -L$(MOZART_DIR)/usr/lib -lcgi -lm -lwifi -ljson-c -lutils -lini -lalarm -lpower -lota -ltips -lplayer -llinklist -lalarm_interface

DESTDIR ?=
PREFIX = /usr
SBIN_DIR = $(PREFIX)/sbin
CGI_BIN = /var/www/cgi-bin
INITDIR = /etc/init.d/

INSTALL = install

TARGETS = config_sta download_mp3 wifi_mode_switch app_service ap_config_page ap_config_confirm_page sta_config_page sta_config_confirm_page app_get_music_list app_get_deviceinfo app_get_play_position app_set_play_position app_get_play_status app_get_shortcut_music_list app_set_shortcut_music_list app_start_update app_start_recovery check_mp3file app_time_get_clock app_time_set_clock app_add_alarm app_get_alarm_list app_delete_alarm app_update_alarm app_check_sdcard app_get_downloadlist app_get_timer_timestamp app_set_timer

all:$(TARGETS)

app_service:app_service.o
	$(CC) $(LDFLAGS) $^ -o $@

app_get_music_list:cJSON.o app_get_music_list.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

install:
	$(INSTALL) -d $(DESTDIR)$(CGI_BIN)
	$(INSTALL) -d $(DESTDIR)$(SBIN_DIR)
	$(INSTALL) -d $(DESTDIR)$(INITDIR)

	cp $(OUTPUT_DIR)/molib/app/var/www/cgi-bin/* $(OUTPUT_DIR)/mozart/app/var/www/cgi-bin/ -a

	$(INSTALL) -m 755 app_service $(DESTDIR)$(SBIN_DIR)/
	$(INSTALL) -m 755 app_get_music_list $(DESTDIR)$(CGI_BIN)/

	$(INSTALL) -m 755 S88app.sh $(DESTDIR)$(INITDIR)
	$(INSTALL) -m 755 S99webserver.sh $(DESTDIR)$(INITDIR)

clean:
	-rm -rf *.o $(TARGETS)

uninstall:
	-rm -f $(DESTDIR)$(SBIN_DIR)/app_service
	-rm -f $(OUTPUT_DIR)/fs/appfs/usr/sbin/app_service
	-rm -rf $(DESTDIR)$(CGI_BIN)/
	-rm -f $(DESTDIR)$(INITDIR)/S88app.sh

.PHONY:all clean install uninstall $(TARGETS)
