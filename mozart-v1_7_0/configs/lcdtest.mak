target := lcdtest
lcdtest_DIR := $(TOPDIR)/tools/device-tools/lcdtest

TARGETS += lcdtest
TARGETS1 += lcdtest

# do nothing
define lcdtest_CONFIGURE_CMDS


endef

define lcdtest_BUILD_CMDS
	$(MAKE) -C $(lcdtest_DIR)
endef

define lcdtest_INSTALL_TARGET_CMDS
	$(MAKE) -C $(lcdtest_DIR) DESTDIR=$(MOZART_APP_DIR) install
endef

define lcdtest_CLEAN_CMDS
	$(MAKE) -C $(lcdtest_DIR) clean
endef

define lcdtest_UNINSTALL_TARGET_CMDS
	$(MAKE1) -C $(lcdtest_DIR) DESTDIR=$(MOZART_APP_DIR) uninstall
endef

$(eval $(call install_rules,lcdtest,$(lcdtest_DIR),target))
