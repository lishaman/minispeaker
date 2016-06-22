host-genext2fs_DIR := $(TOPDIR)/tools/host-tools/genext2fs-1.4.1

# will be included by rootfs rules.
#TARGETS += host-genext2fs

define host-genext2fs_AUTOMAKE_FILES_STAMP_RESET
        (cd $(@D); \
                $(TOPDIR)/configs/stamp_reset.sh \
        )
endef

host-genext2fs_PRE_CONFIGURE_HOOKS += host-genext2fs_AUTOMAKE_FILES_STAMP_RESET


define host-genext2fs_CONFIGURE_CMDS
	(cd $(@D); \
		./configure --prefix=$(OUTPUT_DIR)/host/usr \
	)
endef

define host-genext2fs_BUILD_CMDS
	$(MAKE) -C $(host-genext2fs_DIR)
endef

define host-genext2fs_INSTALL_HOST_CMDS
	$(MAKE) -C $(host-genext2fs_DIR) install
endef

define host-genext2fs_CLEAN_CMDS
	-$(MAKE1) -C $(host-genext2fs_DIR) clean
endef

define host-genext2fs_DISTCLEAN_CMDS
	-$(MAKE1) -C $(host-genext2fs_DIR) distclean
endef

define host-genext2fs_UNINSTALL_HOST_CMDS
	-$(MAKE1) -C $(host-genext2fs_DIR) uninstall
endef

$(eval $(call install_rules,host-genext2fs,$(host-genext2fs_DIR),host))
