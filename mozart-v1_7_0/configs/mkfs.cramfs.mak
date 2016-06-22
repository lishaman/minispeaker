host-mkfs.cramfs_DIR := $(TOPDIR)/tools/host-tools/util-linux-2.6.2

define host-mkfs.cramfs_CONFIGURE_CMDS
        (cd $(@D); \
                ./autogen.sh; \
                ./configure --disable-all-programs --enable-cramfs \
                --prefix=$(OUTPUT_DIR)/host/usr \
        )
endef

define host-mkfs.cramfs_BUILD_CMDS
        $(MAKE) -C $(host-mkfs.cramfs_DIR) mkfs.cramfs
endef

define host-mkfs.cramfs_INSTALL_HOST_CMDS
        $(MAKE) -C $(host-mkfs.cramfs_DIR) install-sbinPROGRAMS
endef

define host-mkfs.cramfs_CLEAN_CMDS
        -$(MAKE1) -C $(host-mkfs.cramfs_DIR) clean
endef

define host-mkfs.cramfs_DISTCLEAN_CMDS
        -$(MAKE1) -C $(host-mkfs.cramfs_DIR) distclean
endef

define host-mkfs.cramfs_UNINSTALL_HOST_CMDS
        -$(MAKE1) -C $(host-mkfs.cramfs_DIR) uninstall-sbinPROGRAMS
endef

$(eval $(call install_rules,host-mkfs.cramfs,$(host-mkfs.cramfs_DIR),host))
