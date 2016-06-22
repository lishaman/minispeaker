host-e2fsprogs_DIR := $(TOPDIR)/tools/host-tools/e2fsprogs-1.42.8

TARGETS += host-e2fsprogs


define host-e2fsprogs_CONFIGURE_CMDS
	(cd $(@D); \
		./configure --prefix=$(OUTPUT_DIR)/host/usr \
	)
endef

define host-e2fsprogs_BUILD_CMDS
	$(MAKE) -C $(host-e2fsprogs_DIR)
endef

define host-e2fsprogs_INSTALL_HOST_CMDS
	$(MAKE) -C $(host-e2fsprogs_DIR) install
	$(MAKE) -C $(host-e2fsprogs_DIR) install-libs
endef

define host-e2fsprogs_CLEAN_CMDS
	-$(MAKE1) -C $(host-e2fsprogs_DIR) clean
endef

define host-e2fsprogs_DISTCLEAN_CMDS
	-$(MAKE1) -C $(host-e2fsprogs_DIR) distclean
endef

define host-e2fsprogs_UNINSTALL_HOST_CMDS
	-$(MAKE1) -C $(host-e2fsprogs_DIR) uninstall
endef

$(eval $(call install_rules,host-e2fsprogs,$(host-e2fsprogs_DIR),host))
