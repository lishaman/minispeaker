hostapd-realtek_DIR := $(TOPDIR)/tools/device-tools/wpa_supplicant_hostapd-0.8/hostapd/
hostapd-broadcom_DIR := $(TOPDIR)/tools/device-tools/hostapd-2.0/hostapd/

define hostapd_mk_config
	cp -f $(@D)/wifi_audio_defconfig $(@D)/.config
endef

hostapd_PRE_BUILD_HOOKS += hostapd_mk_config

define hostapd_BUILD_CMDS
	CC=mipsel-linux-gcc $(MAKE) -C $(@D)
endef

define hostapd-realtek_INSTALL_TARGET_CMDS
	mkdir -p $(MOZART_UPDATER_DIR)/usr/sbin; \
	cp $(@D)/hostapd $(MOZART_DIR)/usr/sbin/hostapd_realtek; \
	cp $(@D)/hostapd_cli $(MOZART_DIR)/usr/sbin/hostapd_cli_realtek
endef

define hostapd-broadcom_INSTALL_TARGET_CMDS
	mkdir -p $(MOZART_UPDATER_DIR)/usr/sbin; \
	cp $(@D)/hostapd $(MOZART_DIR)/usr/sbin/hostapd_broadcom; \
	cp $(@D)/hostapd_cli $(MOZART_DIR)/usr/sbin/hostapd_cli_broadcom
endef

define hostapd_CLEAN_CMDS
	-$(MAKE) -C $(@D) clean; rm -f $(@D)/.config
endef

define hostapd_DISTCLEAN_CMDS
	$(MAKE) -C $(hostapd-realtek_DIR) clean; rm -f $(hostapd-realtek_DIR)/.config; rm -rf $(hostapd-realtek_DIR)/.stamp_*
	$(MAKE) -C $(hostapd-broadcom_DIR) clean; rm -f $(hostapd-broadcom_DIR)/.config; rm -rf $(hostapd-broadcom_DIR)/.stamp_*
endef

define hostapd-realtek_UNINSTALL_TARGET_CMDS
-rm -f $(MOZART_UPDATER_DIR)/usr/sbin/{hostapd_realtek,hostapd_cli_realtek}
endef
define hostapd-broadcom_UNINSTALL_TARGET_CMDS
-rm -f $(MOZART_UPDATER_DIR)/usr/sbin/{hostapd_broadcom,hostapd_cli_broadcom}
endef

TARGETS += hostapd
ifeq ("$(call substring,$(SUPPORT_WIFI_MODULE),1,3)", "rtk") # realtek's wifi module.
	hostapd_INSTALL_TARGET_CMDS=$(hostapd-realtek_INSTALL_TARGET_CMDS)
	hostapd_UNINSTALL_TARGET_CMDS=$(hostapd-realtek_UNINSTALL_TARGET_CMDS)
$(eval $(call install_rules,hostapd,$(hostapd-realtek_DIR),target))
else
ifeq ("$(call substring,$(SUPPORT_WIFI_MODULE),1,3)", "bcm") # broadcom's wifi module.
	hostapd_INSTALL_TARGET_CMDS=$(hostapd-broadcom_INSTALL_TARGET_CMDS)
	hostapd_UNINSTALL_TARGET_CMDS=$(hostapd-broadcom_UNINSTALL_TARGET_CMDS)
$(eval $(call install_rules,hostapd,$(hostapd-broadcom_DIR),target))
else
	hostapd_INSTALL_TARGET_CMDS=$(hostapd-realtek_INSTALL_TARGET_CMDS)
	hostapd_UNINSTALL_TARGET_CMDS=$(hostapd-realtek_UNINSTALL_TARGET_CMDS)
$(eval $(call install_rules,hostapd,$(hostapd-realtek_DIR),target))
endif
endif
