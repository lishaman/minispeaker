##################################################################
# @file Makefile.in
# @brief
# @author ylguo <yonglei.guo@ingenic.com>
# @version 0.1
# @date 2015-02-06
#
# Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
#
# The program is not free, Ingenic without permission,
# no one shall be arbitrarily (including but not limited
# to: copy, to the illegal way of communication, display,
# mirror, upload, download) use, or by unconventional
# methods (such as: malicious intervention Ingenic data)
# Ingenic's normal service, no one shall be arbitrarily by
# software the program automatically get Ingenic data
# Otherwise, Ingenic will be investigated for legal responsibility
# according to law.
###################################################################


# MESSAGE Macro -- display a message in bold type
pkgname     = $(lastword $(subst /, ,$(1)))
MESSAGE     = echo "$(TERM_BOLD)>>> $(call pkgname, $(PACKAGE_PATH)) $(1)$(TERM_RESET)"
TERM_BOLD  := $(shell tput smso)
TERM_RESET := $(shell tput rmso)
STRIP      := mipsel-linux-strip

# make a newline stamp
define sep


endef

define \n


endef

# Patch
%/.stamp_patched:
	$(foreach hook,$($(PACKAGE_NAME)_PRE_PATCH_HOOKS),$(call $(hook))$(sep))
	@$(call MESSAGE,"Patching")
	$($(PACKAGE_NAME)_PATCH_CMDS)
	$(foreach hook,$($(PACKAGE_NAME)_POST_PATCH_HOOKS),$(call $(hook))$(sep))
	$(Q)touch $@

# Configure
%/.stamp_configured:
	$(foreach hook,$($(PACKAGE_NAME)_PRE_CONFIGURE_HOOKS),$(call $(hook))$(sep))
	@$(call MESSAGE,"Configuring")
	$($(PACKAGE_NAME)_CONFIGURE_CMDS)
	$(foreach hook,$($(PACKAGE_NAME)_POST_CONFIGURE_HOOKS),$(call $(hook))$(sep))
	$(Q)touch $@

# Build
%/.stamp_built:
	$(foreach hook,$($(PACKAGE_NAME)_PRE_BUILD_HOOKS),$(call $(hook))$(sep))
	@$(call MESSAGE,"Building")
	$($(PACKAGE_NAME)_BUILD_CMDS)
	$(foreach hook,$($(PACKAGE_NAME)_POST_BUILD_HOOKS),$(call $(hook))$(sep))
	$(Q)touch $@

# Install to host dir
%/.stamp_host_installed:
	$(foreach hook,$($(PACKAGE_NAME)_PRE_INSTALL_HOST_HOOKS),$(call $(hook))$(sep))
	@$(call MESSAGE,"Installing to host directory")
	$($(PACKAGE_NAME)_INSTALL_HOST_CMDS)
	$(foreach hook,$($(PACKAGE_NAME)_POST_INSTALL_HOST_HOOKS),$(call $(hook))$(sep))
	$(Q)touch $@

# Install to target dir
%/.stamp_target_installed:
	$(foreach hook,$($(PACKAGE_NAME)_PRE_INSTALL_TARGET_HOOKS),$(call $(hook))$(sep))
	@$(call MESSAGE,"Installing to target directory")
	$($(PACKAGE_NAME)_INSTALL_TARGET_CMDS)
	$(foreach hook,$($(PACKAGE_NAME)_POST_INSTALL_TARGET_HOOKS),$(call $(hook))$(sep))
	$(Q)touch $@

# Clean package
%/.stamp_cleaned:
	@$(call MESSAGE,"Cleaning up")
	$($(PACKAGE_NAME)_CLEAN_CMDS)

# Distclean package
%/.stamp_distcleaned:
	@$(call MESSAGE,"Distcleaning up")
	$($(PACKAGE_NAME)_DISTCLEAN_CMDS)

%/.stamp_stampfile_removed:
	@$(call MESSAGE,"Removing stamp files")
	rm -f $(@D)/.stamp_patched
	rm -f $(@D)/.stamp_configured
	rm -f $(@D)/.stamp_built
	rm -f $(@D)/.stamp_target_installed
	rm -f $(@D)/.stamp_host_installed

# Uninstall package from target and staging
# Uninstall commands tend to fail, so remove the stamp files first
%/.stamp_target_uninstalled:
	@$(call MESSAGE,"Uninstalling from target directory")
	$($(PACKAGE_NAME)_UNINSTALL_TARGET_CMDS)

%/.stamp_host_uninstalled:
	@$(call MESSAGE,"Uninstalling from host directory")
	$($(PACKAGE_NAME)_UNINSTALL_HOST_CMDS)


##############################################
#     package build rules install handler    #
##############################################
define install_rules
# Params 1: package name:
# Params 2: package path:
# Params 3: package type:
#    available values:
#           host: package will run in Host machine.
#         target: package will run in Ingenic platform.
#

# Define default values for various package-related variables
$(1)-TYPE                       =  $(3)
$(1)-NAME			=  $(1)
$(1)-DIR			=  $(2)


# define sub-target stamps
$(1)_TARGET_PATCH =		$(2)/.stamp_patched
$(1)_TARGET_CLEAN =		$(2)/.stamp_cleaned
$(1)_TARGET_REMOVE_STAMP =	$(2)/.stamp_stampfile_removed
$(1)_TARGET_DISTCLEAN =		$(2)/.stamp_distcleaned
$(1)_TARGET_CONFIGURE =		$(2)/.stamp_configured
$(1)_TARGET_BUILD =		$(2)/.stamp_built
$(1)_TARGET_INSTALL_TARGET =	$(2)/.stamp_target_installed
$(1)_TARGET_INSTALL_HOST =      $(2)/.stamp_host_installed
$(1)_TARGET_UNINSTALL_TARGET =		$(2)/.stamp_target_uninstalled
$(1)_TARGET_UNINSTALL_HOST =		$(2)/.stamp_host_uninstalled



# hooks
$(1)_PRE_PATCH_HOOKS            ?=
$(1)_POST_PATCH_HOOKS           ?=
$(1)_PRE_CONFIGURE_HOOKS        ?=
$(1)_POST_CONFIGURE_HOOKS       ?=
$(1)_PRE_BUILD_HOOKS            ?=
$(1)_POST_BUILD_HOOKS           ?=
$(1)_PRE_INSTALL_HOOKS          ?=
$(1)_POST_INSTALL_HOOKS         ?=


# human-friendly targets and target sequencing
$(1): $(1)-install

ifeq ($$($(1)-TYPE),host)
$(1)-install:	        $(1)-install-host
else
$(1)-install:		$(1)-install-target
endif

$(1)-install-target:	$(1)-build $$($(1)_TARGET_INSTALL_TARGET)
$(1)-install-host:      $(1)-build $$($(1)_TARGET_INSTALL_HOST)

$(1)-build:             $(1)-configure $$($(1)_TARGET_BUILD)

$(1)-configure:         $(1)-depends $(1)-patch $$($(1)_TARGET_CONFIGURE)

$(1)-depends:           $($(1)_DEPENDENCIES)

$(1)-patch:	        $$($(1)_TARGET_PATCH)

$(1)-clean:		$(1)-uninstall \
	$$($(1)_TARGET_CLEAN) \
	$$($(1)_TARGET_DISTCLEAN) \
	$$($(1)_TARGET_REMOVE_STAMP)

$(1)-distclean:		$$($(1)_TARGET_DISTCLEAN)

ifeq ($$($(1)-TYPE),host)
$(1)-uninstall:		$$($(1)_TARGET_UNINSTALL_HOST)
else
$(1)-uninstall:		$$($(1)_TARGET_UNINSTALL_TARGET)
endif

$(1)-rebuild: $(1)-clean $(1)

$(1)-show-depends:
			@echo $$($(1)_DEPENDENCIES)


# define the PACKAGE_NAME variable for all targets, containing the
# uppercase package variable prefix
$$($(1)_TARGET_INSTALL_TARGET):		PACKAGE_PATH=$(2)
$$($(1)_TARGET_INSTALL_TARGET):		PACKAGE_NAME=$(1)

$$($(1)_TARGET_INSTALL_HOST):           PACKAGE_PATH=$(2)
$$($(1)_TARGET_INSTALL_HOST):           PACKAGE_NAME=$(1)

$$($(1)_TARGET_BUILD):			PACKAGE_PATH=$(2)
$$($(1)_TARGET_BUILD):			PACKAGE_NAME=$(1)

$$($(1)_TARGET_CONFIGURE):		PACKAGE_PATH=$(2)
$$($(1)_TARGET_CONFIGURE):		PACKAGE_NAME=$(1)

$$($(1)_TARGET_PATCH):			PACKAGE_PATH=$(2)
$$($(1)_TARGET_PATCH):			PACKAGE_NAME=$(1)

$$($(1)_TARGET_UNINSTALL_TARGET):		PACKAGE_PATH=$(2)
$$($(1)_TARGET_UNINSTALL_TARGET):		PACKAGE_NAME=$(1)

$$($(1)_TARGET_UNINSTALL_HOST):		PACKAGE_PATH=$(2)
$$($(1)_TARGET_UNINSTALL_HOST):		PACKAGE_NAME=$(1)

$$($(1)_TARGET_CLEAN):			PACKAGE_PATH=$(2)
$$($(1)_TARGET_CLEAN):			PACKAGE_NAME=$(1)

$$($(1)_TARGET_DISTCLEAN):			PACKAGE_PATH=$(2)
$$($(1)_TARGET_DISTCLEAN):			PACKAGE_NAME=$(1)

$$($(1)_TARGET_REMOVE_STAMP):			PACKAGE_PATH=$(2)
$$($(1)_TARGET_REMOVE_STAMP):			PACKAGE_NAME=$(1)

endef


##############################################
#         upper case --> lower case          #
##############################################
define LOWERCASE
$(shell echo $(1) | tr '[A-Z]' '[a-z]')
endef

##############################################
#         lower case --> upper case          #
##############################################
define UPPERCASE
$(shell echo $(1) | tr '[a-z]' '[A-Z]')
endef
