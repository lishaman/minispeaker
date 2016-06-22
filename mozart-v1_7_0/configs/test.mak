target := test
test_DIR := $(TOPDIR)/tools/device-tools/test

TARGETS += test
TARGETS1 += test

SUPPORT_LCD ?= 0


# do nothing
define test_CONFIGURE_CMDS


endef

define test_OPTS
	LCD=$(SUPPORT_LCD)
endef

define test_BUILD_CMDS
	$(test_OPTS) \
		$(MAKE) -C $(test_DIR)
endef

define test_INSTALL_TARGET_CMDS
	$(test_OPTS) \
		$(MAKE) -C $(test_DIR) install
endef

define test_CLEAN_CMDS
	$(test_OPTS) \
		$(MAKE) -C $(test_DIR) clean
endef

define test_UNINSTALL_TARGET_CMDS
	$(test_OPTS) \
		$(MAKE1) -C $(test_DIR) uninstall
endef

$(eval $(call install_rules,test,$(test_DIR),target))
