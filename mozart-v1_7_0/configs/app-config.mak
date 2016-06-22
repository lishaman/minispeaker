app-config_DIR := $(TOPDIR)/tools/device-tools/app-config

TARGETS += app-config
TARGETS1 += app-config


# do nothing
define app-config_CONFIGURE_CMDS


endef

define app-config_BUILD_CMDS
	$(MAKE) -C $(app-config_DIR)
endef

define app-config_INSTALL_TARGET_CMDS
	$(MAKE) -C $(app-config_DIR) DESTDIR=$(MOZART_DIR) install
endef

define app-config_CLEAN_CMDS
	-$(MAKE) -C $(app-config_DIR) clean
endef

define app-config_UNINSTALL_TARGET_CMDS
	-$(MAKE1) -C $(app-config_DIR) DESTDIR=$(MOZART_DIR) uninstall
endef

$(eval $(call install_rules,app-config,$(app-config_DIR),target))
