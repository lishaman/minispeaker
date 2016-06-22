host-fakeroot_DIR := $(TOPDIR)/tools/host-tools/fakeroot-1.18.2

TARGETS += host-fakeroot


define host-fakeroot_CONFIGURE_CMDS
	(cd $(@D); \
		./configure \
		--prefix=$(OUTPUT_DIR)/host/usr --sysconfdir=$(OUTPUT_DIR)/host/etc \
		--enable-shared --disable-static \
	)
endef

define host-fakeroot_BUILD_CMDS
	$(MAKE) -C $(host-fakeroot_DIR)
endef

define host-fakeroot_INSTALL_HOST_CMDS
	$(MAKE) -C $(host-fakeroot_DIR) install
endef

define host-fakeroot_CLEAN_CMDS
	-$(MAKE1) -C $(host-fakeroot_DIR) clean
endef

define host-fakeroot_DISTCLEAN_CMDS
	-$(MAKE1) -C $(host-fakeroot_DIR) distclean
endef

define host-fakeroot_UNINSTALL_HOST_CMDS
	-$(MAKE1) -C $(host-fakeroot_DIR) uninstall
endef

$(eval $(call install_rules,host-fakeroot,$(host-fakeroot_DIR),host))
