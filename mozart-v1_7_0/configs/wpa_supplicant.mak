wpa_supplicant_broadcom_DIR := $(TOPDIR)/tools/device-tools/wpa_supplicant-2.2/wpa_supplicant
wpa_supplicant_realtek_DIR := $(TOPDIR)/tools/device-tools/wpa_supplicant_hostapd-0.8/wpa_supplicant

#do nothing
define wpa_supplicant_CONFIGURE_CMDS


endef

define wpa_supplicant_mk_config
	cp -f $(@D)/wifi_audio_defconfig $(@D)/.config
endef

wpa_supplicant_PRE_BUILD_HOOKS += wpa_supplicant_mk_config


define wpa_supplicant_BUILD_CMDS
	CC=mipsel-linux-gcc $(MAKE) -C $(@D)
endef

define wpa_supplicant_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) STRIP=mipsel-linux-strip DESTDIR=$(MOZART_UPDATER_DIR) install
endef

define wpa_supplicant_realtek_INSTALL_TARGET_CMDS
	mkdir -p $(MOZART_UPDATER_DIR)/usr/sbin; \
	cp -f $(@D)/wpa_supplicant $(MOZART_UPDATER_DIR)/usr/sbin/wpa_supplicant_realtek; \
	cp -f $(@D)/wpa_cli $(MOZART_UPDATER_DIR)/usr/sbin/wpa_cli_realtek; \
	cp -f $(@D)/wpa_passphrase $(MOZART_UPDATER_DIR)/usr/sbin/wpa_passphrase_realtek
endef

define wpa_supplicant_broadcom_INSTALL_TARGET_CMDS
	mkdir -p $(MOZART_UPDATER_DIR)/usr/sbin; \
	cp -f $(@D)/wpa_supplicant $(MOZART_UPDATER_DIR)/usr/sbin/wpa_supplicant_broadcom; \
	cp -f $(@D)/wpa_cli $(MOZART_UPDATER_DIR)/usr/sbin/wpa_cli_broadcom; \
	cp -f $(@D)/wpa_passphrase $(MOZART_UPDATER_DIR)/usr/sbin/wpa_passphrase_broadcom
endef


define wpa_supplicant_CLEAN_CMDS
	-$(MAKE) -C $(@D) clean; rm -f $(@D)/.config
endef

define wpa_supplicant_DISTCLEAN_CMDS
	$(MAKE) -C $(wpa_supplicant_realtek_DIR) clean; rm -f $(wpa_supplicant_realtek_DIR)/.config; rm -rf $(wpa_supplicant_realtek_DIR)/.stamp_*
	$(MAKE) -C $(wpa_supplicant_broadcom_DIR) clean; rm -f $(wpa_supplicant_broadcom_DIR)/.config; rm -rf $(wpa_supplicant_broadcom_DIR)/.stamp_*
endef

define wpa_supplicant_realtek_UNINSTALL_TARGET_CMDS
-rm -f $(MOZART_UPDATER_DIR)/usr/sbin/{wpa_supplicant_realtek,wpa_cli_realtek,wpa_passphrase_realtek}
endef
define wpa_supplicant_broadcom_UNINSTALL_TARGET_CMDS
-rm -f $(MOZART_UPDATER_DIR)/usr/sbin/{wpa_supplciant_broadcom,wpa_cli_broadcom,wpa_passphrase_broadcom}
endef


TARGETS += wpa_supplicant
ifeq ("$(call substring,$(SUPPORT_WIFI_MODULE),1,3)", "rtk") # realtek's wifi module.
	wpa_supplicant_INSTALL_TARGET_CMDS=$(wpa_supplicant_realtek_INSTALL_TARGET_CMDS)
	wpa_supplicant_UNINSTALL_TARGET_CMDS=$(wpa_supplicant_realtek_UNINSTALL_TARGET_CMDS)
$(eval $(call install_rules,wpa_supplicant,$(wpa_supplicant_realtek_DIR),target))
else
ifeq ("$(call substring,$(SUPPORT_WIFI_MODULE),1,3)", "bcm") # broadcom's wifi module.
	wpa_supplicant_INSTALL_TARGET_CMDS=$(wpa_supplicant_broadcom_INSTALL_TARGET_CMDS)
	wpa_supplicant_UNINSTALL_TARGET_CMDS=$(wpa_supplicant_broadcom_UNINSTALL_TARGET_CMDS)
$(eval $(call install_rules,wpa_supplicant,$(wpa_supplicant_broadcom_DIR),target))
else
	wpa_supplicant_INSTALL_TARGET_CMDS=$(wpa_supplicant_realtek_INSTALL_TARGET_CMDS)
	wpa_supplicant_UNINSTALL_TARGET_CMDS=$(wpa_supplicant_realtek_UNINSTALL_TARGET_CMDS)
$(eval $(call install_rules,wpa_supplicant,$(wpa_supplicant_realtek_DIR),target))
endif
endif
