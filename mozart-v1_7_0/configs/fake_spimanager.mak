fake_spimanager_DIR := $(TOPDIR)/src/fake_spimanager/

define fake_spimanager_BUILD_CMDS
	$(MAKE) -C $(@D)
endef

define fake_spimanager_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) DESTDIR=$(MOZART_DIR) install
endef

define fake_spimanager_CLEAN_CMDS
	-$(MAKE) -C $(@D) clean
endef

define fake_spimanager_UNINSTALL_TARGET_CMDS
	-$(MAKE) -C $(@D) DESTDIR=$(MOZART_DIR) uninstall
endef

$(eval $(call install_rules,fake_spimanager,$(fake_spimanager_DIR),target))
