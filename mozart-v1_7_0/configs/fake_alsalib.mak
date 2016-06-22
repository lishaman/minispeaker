fake_alsalib_DIR := $(TOPDIR)/src/fake_alsalib/

TARGETS += fake_alsalib
TARGETS1 += fake_alsalib

define fake_alsalib_BUILD_CMDS
	$(MAKE) -C $(@D)
endef

define fake_alsalib_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) DESTDIR=$(MOZART_UPDATER_DIR) install
endef

define fake_alsalib_CLEAN_CMDS
	-$(MAKE) -C $(@D) clean
endef

define fake_alsalib_UNINSTALL_TARGET_CMDS
	-$(MAKE) -C $(@D) DESTDIR=$(MOZART_UPDATER_DIR) uninstall
endef

$(eval $(call install_rules,fake_alsalib,$(fake_alsalib_DIR),target))
