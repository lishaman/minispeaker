target := main
main_DIR := $(TOPDIR)/src/main

TARGETS += main
TARGETS1 += main

SUPPORT_BT_MODULE ?= 0
SUPPORT_VR ?= 0
SUPPORT_LAPSULE ?= 0
SUPPORT_DMR ?= 0
SUPPORT_WEBRTC ?= 0
SUPPORT_DMS ?= 0
SUPPORT_AIRPLAY ?= 0
SUPPORT_LOCALPLAYER ?= 0
SUPPORT_ATALK ?= 0
SUPPORT_LCD ?= 0
# do nothing
define main_CONFIGURE_CMDS


endef

define main_OPTS
	BT=$(SUPPORT_BT_MODULE) VR=$(SUPPORT_VR) LAPSULE=$(SUPPORT_LAPSULE) WEBRTC=$(SUPPORT_WEBRTC) \
	   DMR=$(SUPPORT_DMR) DMS=$(SUPPORT_DMS) AIRPLAY=$(SUPPORT_AIRPLAY) LOCALPLAYER=$(SUPPORT_LOCALPLAYER) \
	   LCD=$(SUPPORT_LCD) ATALK=$(SUPPORT_ATALK)
endef

define main_BUILD_CMDS
	$(main_OPTS) \
			$(MAKE) -C $(main_DIR)
endef

define main_INSTALL_TARGET_CMDS
	$(main_OPTS) \
			$(MAKE) -C $(main_DIR) DESTDIR=$(MOZART_DIR) install
endef

define main_CLEAN_CMDS
	-$(main_OPTS) \
			$(MAKE) -C $(main_DIR) clean
endef

define main_UNINSTALL_TARGET_CMDS
	-$(main_OPTS) \
			$(MAKE1) -C $(main_DIR) DESTDIR=$(MOZART_DIR) uninstall
endef

$(eval $(call install_rules,main,$(main_DIR),target))
