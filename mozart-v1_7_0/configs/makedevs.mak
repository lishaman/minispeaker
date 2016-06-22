host-makedevs_DIR := $(TOPDIR)/tools/host-tools/makedevs

TARGETS += host-makedevs


# do nothing
define host-makedevs_CONFIGURE_CMDS


endef

define host-makedevs_BUILD_CMDS
	$(MAKE) -C $(host-makedevs_DIR)
endef

define host-makedevs_INSTALL_HOST_CMDS
	$(MAKE) -C $(host-makedevs_DIR) install DESTDIR=$(OUTPUT_DIR)/host
endef

define host-makedevs_CLEAN_CMDS
	-$(MAKE1) -C $(host-makedevs_DIR) clean
endef

define host-makedevs_DISTCLEAN_CMDS
	-$(MAKE1) -C $(host-makedevs_DIR) distclean
endef

define host-makedevs_UNINSTALL_HOST_CMDS
	rm -f $(OUTPUT_DIR)/host/usr/bin/makedevs
endef

$(eval $(call install_rules,host-makedevs,$(host-makedevs_DIR),host))
