host-mtd-utils_DIR := $(TOPDIR)/tools/host-tools/mtd-utils-1.5.1
host-mtd-utils_DEPENDENCIES := host-e2fsprogs host-lzo host-zlib

mtd_DEPENDS_PREFIX := $(OUTPUT_DIR)/host/usr

mtd_ZLIBFLAGS := \
	ZLIBCPPFLAGS=-I$(mtd_DEPENDS_PREFIX)/include \
	ZLIBLDFLAGS=-L$(mtd_DEPENDS_PREFIX)/lib

mtd_LZOFLAGS := \
	LZOCPPFLAGS=-I$(mtd_DEPENDS_PREFIX)/include/lzo \
	LZOLDFLAGS=-L$(mtd_DEPENDS_PREFIX)/lib

define host-mtd-utils_BUILD_CMDS
	( \
		$(mtd_ZLIBFLAGS) \
		$(mtd_LZOFLAGS) \
		$(MAKE) -C $(host-mtd-utils_DIR) WITHOUT_XATTR=1 \
	)
endef

define host-mtd-utils_INSTALL_HOST_CMDS
	( \
		$(mtd_ZLIBFLAGS) \
		$(mtd_LZOFLAGS) \
		$(MAKE) -C $(host-mtd-utils_DIR) WITHOUT_XATTR=1 DESTDIR=$(OUTPUT_DIR)/host install \
	)
endef

define host-mtd-utils_DISTCLEAN_CMDS
        -$(MAKE1) -C $(host-mtd-utils_DIR) clean
endef

$(eval $(call install_rules,host-mtd-utils,$(host-mtd-utils_DIR),host))
