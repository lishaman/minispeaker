target := trace_alloc
trace_alloc_DIR := $(TOPDIR)/tools/device-tools/debug/trace_alloc

TARGETS += trace_alloc
TARGETS1 += trace_alloc

# do nothing
define trace_alloc_CONFIGURE_CMDS


endef

define trace_alloc_BUILD_CMDS
	$(MAKE) -C $(trace_alloc_DIR)
endef

define trace_alloc_INSTALL_TARGET_CMDS
	$(MAKE) -C $(trace_alloc_DIR) DESTDIR=$(MOZART_APP_DIR) install
endef

define trace_alloc_CLEAN_CMDS
	$(MAKE) -C $(trace_alloc_DIR) clean
endef

define trace_alloc_UNINSTALL_TARGET_CMDS
	$(MAKE1) -C $(trace_alloc_DIR) DESTDIR=$(MOZART_APP_DIR) uninstall
endef

define trace_alloc_DISTCLEAN_CMDS
	$(MAKE1) -C $(trace_alloc_DIR) DESTDIR=$(MOZART_APP_DIR) distclean
endef

$(eval $(call install_rules,trace_alloc,$(trace_alloc_DIR),target))
