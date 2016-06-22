updater_DIR := $(TOPDIR)/src/updater

TARGETS += updater
TARGETS1 += updater

define updater_BUILD_CMDS
	$(MAKE) -C $(@D)
endef

define updater_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) DESTDIR=$(MOZART_UPDATER_DIR) install
endef

define updater_CLEAN_CMDS
	-$(MAKE) -C $(@D) DESTDIR=$(MOZART_UPDATER_DIR) clean
endef

define updater_UNINSTALL_TARGET_CMDS
	-$(MAKE) -C $(@D) DESTDIR=$(MOZART_UPDATER_DIR) uninstall
endef

$(eval $(call install_rules,updater,$(updater_DIR),target))
