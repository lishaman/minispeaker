define ROOTFS_cpio_CMDS
cd $(1) && find . | cpio --quiet -oH  newc > $(2)
endef

define ROOTFS_tarball_CMDS
cd $(1) && tar cf $(2) .
endef

ROOTFS_ext4_DEPENDENCIES := host-genext2fs host-e2fsprogs
define ROOTFS_ext4_CMDS
GEN=4 REV=1 $(TOPDIR)/configs/genext2fs.sh -d $(1) $(2)
endef

ROOTFS_cramfs_DEPENDENCIES := host-mkfs.cramfs
define ROOTFS_cramfs_CMDS
$(OUTPUT_DIR)/host/usr/sbin/mkfs.cramfs $(1) $(2)
endef

ROOTFS_ubifs_DEPENDENCIES := host-mtd-utils
define ROOTFS_ubifs_CMDS
endef

define ROOTFS_SINGLE_ubifs_CMDS
$(OUTPUT_DIR)/host/usr/sbin/mkfs.ubifs -m $(FLASH_PAGE_SIZE) -e $(FLASH_LOGICAL_BLOCK_SIZE) \
	-c $(FLASH_ERASE_BLOCK_COUNT) -r $(1) -o $(2)
endef

ifeq ("$(SUPPORT_FS)", "ramdisk")
$(eval $(call install_fs_rules,cpio,1))
endif
ifeq ("$(SUPPORT_FS)", "ext4")
TARGETS += $(ROOTFS_ext4_DEPENDENCIES)
$(eval $(call install_fs_rules,ext4,0))
endif
ifeq ("$(SUPPORT_FS)", "cramfs")
TARGETS += $(ROOTFS_cramfs_DEPENDENCIES)
$(eval $(call install_fs_rules,cramfs,0))
endif
ifeq ("$(SUPPORT_FS)", "ubifs")
TARGETS += $(ROOTFS_ubifs_DEPENDENCIES)
$(eval $(call install_fs_rules,ubifs,0))
endif
#$(eval $(call install_fs_rules,tarball,0))

##################################################################
USRDATA_jffs2_DEPENDENCIES := host-mtd-utils
define USRDATA_jffs2_CMDS
$(OUTPUT_DIR)/host/usr/sbin/mkfs.jffs2 -m none -r $(1) -s$(FLASH_PAGE_SIZE) \
	-e$(FLASH_ERASE_BLOCK_SIZE) -p$(FLASH_USRDATA_PADSIZE) -o $(2)
endef

ifeq ("$(SUPPORT_USRDATA)", "jffs2")
TARGETS += $(USRDATA_jffs2_DEPENDENCIES)
$(eval $(call install_usrdata_rules,jffs2,0))
endif
