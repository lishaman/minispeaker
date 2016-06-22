busybox_DIR := $(TOPDIR)/tools/device-tools/busybox

TARGETS += busybox


define busybox_CONFIGURE_CMDS
	make wifi_audio_defconfig -C $(@D)
endef

define busybox_BUILD_CMDS
	$(MAKE1) -C $(@D)
endef

define busybox_INSTALL_TARGET_CMDS
	$(MAKE1) -C $(@D) CONFIG_PREFIX=$(MOZART_UPDATER_DIR) install
endef

define busybox_CLEAN_CMDS
	-$(MAKE) -C $(@D) clean
endef

define busybox_UNINSTALL_TARGET_CMDS
	-$(MAKE1) -C $(@D) CONFIG_PREFIX=$(MOZART_UPDATER_DIR) uninstall
endef

$(eval $(call install_rules,busybox,$(busybox_DIR),target))
