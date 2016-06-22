host-zlib_DIR := $(TOPDIR)/tools/host-tools/zlib-1.2.8

TARGETS += host-zlib


define host-zlib_CONFIGURE_CMDS
	(cd $(@D); \
		./configure --prefix=$(OUTPUT_DIR)/host/usr --static \
	)
endef

define host-zlib_BUILD_CMDS
	$(MAKE) -C $(host-zlib_DIR)
endef

define host-zlib_INSTALL_HOST_CMDS
	$(MAKE) -C $(host-zlib_DIR) install
endef

define host-zlib_CLEAN_CMDS
	-$(MAKE1) -C $(host-zlib_DIR) clean
endef

define host-zlib_DISTCLEAN_CMDS
	-$(MAKE1) -C $(host-zlib_DIR) distclean
endef

define host-zlib_UNINSTALL_HOST_CMDS
	-$(MAKE1) -C $(host-zlib_DIR) uninstall
endef

$(eval $(call install_rules,host-zlib,$(host-zlib_DIR),host))
