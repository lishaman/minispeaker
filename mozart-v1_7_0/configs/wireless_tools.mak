target := wireless_tools
wireless_tools_DIR := $(TOPDIR)/tools/device-tools/wireless_tools.29

TARGETS += wireless_tools

#do nothing
define wireless_tools_CONFIGURE_CMDS


endef

define wireless_tools_BUILD_CMDS
	CC=mipsel-linux-gcc $(MAKE) -C $(@D)
endef

define wireless_tools_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) STRIP=mipsel-linux-strip PREFIX=$(MOZART_DIR)/usr/ install
endef

define wireless_tools_CLEAN_CMDS
	-$(MAKE) -C $(@D) realclean
endef

define wireless_tools_UNINSTALL_TARGET_CMDS


endef

$(eval $(call install_rules,wireless_tools,$(wireless_tools_DIR),target))
