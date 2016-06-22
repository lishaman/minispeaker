host-lzo_DIR := $(TOPDIR)/tools/host-tools/lzo-2.09

TARGETS += host-lzo


define host-lzo_CONFIGURE_CMDS
	(cd $(@D); \
		./configure --prefix=$(OUTPUT_DIR)/host/usr \
	)
endef

define host-lzo_BUILD_CMDS
	$(MAKE) -C $(host-lzo_DIR)
endef

define host-lzo_INSTALL_HOST_CMDS
	$(MAKE) -C $(host-lzo_DIR) install
endef

define host-lzo_CLEAN_CMDS
	-$(MAKE1) -C $(host-lzo_DIR) clean
endef

define host-lzo_DISTCLEAN_CMDS
	-$(MAKE1) -C $(host-lzo_DIR) distclean
endef

define host-lzo_UNINSTALL_HOST_CMDS
	-$(MAKE1) -C $(host-lzo_DIR) uninstall
endef

$(eval $(call install_rules,host-lzo,$(host-lzo_DIR),host))
