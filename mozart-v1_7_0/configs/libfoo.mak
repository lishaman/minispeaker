libfoo_DIR := $(TOPDIR)/src/libfoo
libfoo_DEPENDENCIES := hostapd

TARGETS += libfoo

define libfoo_BUILD_CMDS
	$(MAKE) -C $(@D)
endef

define libfoo_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) DESTDIR=$(MOZART_DIR) install
endef

define libfoo_CLEAN_CMDS
	-$(MAKE) -C $(@D) clean
endef

define libfoo_UNINSTALL_TARGET_CMDS
	-$(MAKE) -C $(@D) DESTDIR=$(MOZART_DIR) uninstall
endef

$(eval $(call install_rules,libfoo,$(libfoo_DIR),target))
