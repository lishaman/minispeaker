host-inirw_DIR := $(TOPDIR)/tools/host-tools/inirw

TARGETS += host-inirw


# do nothing
define host-inirw_CONFIGURE_CMDS


endef

define host-inirw_BUILD_CMDS
	$(MAKE) -C $(host-inirw_DIR)
endef

define host-inirw_INSTALL_HOST_CMDS
	$(MAKE) -C $(host-inirw_DIR) install DESTDIR=$(OUTPUT_DIR)/host
endef

define host-inirw_CLEAN_CMDS
	-$(MAKE1) -C $(host-inirw_DIR) clean DESTDIR=$(OUTPUT_DIR)/host
endef

define host-inirw_DISTCLEAN_CMDS
	-$(MAKE1) -C $(host-inirw_DIR) distclean DESTDIR=$(OUTPUT_DIR)/host
endef

define host-inirw_UNINSTALL_HOST_CMDS
	-$(MAKE1) -C $(host-inirw_DIR) uninstall DESTDIR=$(OUTPUT_DIR)/host
endef

$(eval $(call install_rules,host-inirw,$(host-inirw_DIR),host))
