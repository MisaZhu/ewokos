VERSION = 1
PATCHLEVEL = 0
SUBLEVEL = 0
EXTRAVERSION = .0
NAME = ZETALOG is everywhere!

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be located in ./README
# Comments in this file are targeted only to the developer, do not
# expect to learn how to build the kernel reading this file.

# Do not:
# o  use make's built-in rules and variables
#    (this increases performance and avoid hard-to-debug behavour);
# o  print "Entering directory ...";
MAKEFLAGS += -rR --no-print-directory

# We are using a recursive build, so we need to do a little thinking
# to get the ordering right.
#
# Most importantly: sub-Makefiles should only ever modify files in
# their own directory. If in some directory we have a dependency on
# a file in another dir (which doesn't happen often, but it's often
# unavoidable when linking the built-in.lib targets which finally
# turn into kernel), we will call a sub make in that other dir, and
# after that we are sure that everything which is in that other dir
# is now up to date.
#
# The only cases where we need to modify files which have global
# effects are thus separated out and done before the recursive
# descending is started. They are now explicitly listed as the
# prepare rule.

# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands

ifdef V
  ifeq ("$(origin V)", "command line")
    KBUILD_VERBOSE = $(V)
  endif
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

# Call a source code checker (by default, "sparse") as part of the
# C compilation.
#
# Use 'make C=1' to enable checking of only re-compiled files.
# Use 'make C=2' to enable checking of *all* source files, regardless
# of whether they are re-compiled or not.
#
# See the file "Documentation/sparse.txt" for more details, including
# where to get the "sparse" utility.

ifdef C
  ifeq ("$(origin C)", "command line")
    KBUILD_CHECKSRC = $(C)
  endif
endif
ifndef KBUILD_CHECKSRC
  KBUILD_CHECKSRC = 0
endif

# kbuild supports saving output files in a separate directory.
# To locate output files in a separate directory two syntaxes are supported.
# In both cases the working directory must be the root of the kernel src.
# 1) O=
# Use "make O=dir/to/store/output/files/"
#
# 2) Set KBUILD_OUTPUT
# Set the environment variable KBUILD_OUTPUT to point to the directory
# where the output files shall be placed.
# export KBUILD_OUTPUT=dir/to/store/output/files/
# make
#
# The O= assignment takes precedence over the KBUILD_OUTPUT environment
# variable.


# KBUILD_SRC is set on invocation of make in OBJ directory
# KBUILD_SRC is not intended to be used by the regular user (for now)
ifeq ($(KBUILD_SRC),)

# OK, Make called in directory where kernel src resides
# Do we want to locate output files in a separate directory?
ifdef O
  ifeq ("$(origin O)", "command line")
    KBUILD_OUTPUT := $(O)
  endif
endif

# That's our default target when none is given on the command line
PHONY := _all
_all:

ifneq ($(KBUILD_OUTPUT),)
# Invoke a second make in the output directory, passing relevant variables
# check that the output directory actually exists
saved-output := $(KBUILD_OUTPUT)
KBUILD_OUTPUT := $(shell cd $(KBUILD_OUTPUT) && /bin/pwd)
$(if $(KBUILD_OUTPUT),, \
     $(error output directory "$(saved-output)" does not exist))

PHONY += $(MAKECMDGOALS)

$(filter-out _all,$(MAKECMDGOALS)) _all:
	$(if $(KBUILD_VERBOSE:1=),@)$(MAKE) -C $(KBUILD_OUTPUT) \
	KBUILD_SRC=$(CURDIR) \
	-f $(CURDIR)/Makefile $@

# Leave processing to above invocation of make
skip-makefile := 1
endif # ifneq ($(KBUILD_OUTPUT),)
endif # ifeq ($(KBUILD_SRC),)

# We process the rest of the Makefile if this is the final invocation of make
ifeq ($(skip-makefile),)

# If building an external module we do not care about the all: rule
# but instead _all depend on modules
PHONY += all
_all: all

srctree		:= $(if $(KBUILD_SRC),$(KBUILD_SRC),$(CURDIR))
TOPDIR		:= $(srctree)
# FIXME - TOPDIR is obsolete, use srctree/objtree
objtree		:= $(CURDIR)
src		:= $(srctree)
obj		:= $(objtree)

VPATH		:= $(srctree)

export srctree objtree VPATH TOPDIR


# SUBARCH tells the usermode build what the underlying arch is.  That is set
# first, and if a usermode build is happening, the "ARCH=um" on the command
# line overrides the setting of ARCH below.  If a native build is happening,
# then ARCH is assigned, getting whatever value it gets normally, and
# SUBARCH is subsequently ignored.

SUBARCH := arm
SUBARCH_CROSS_COMPILE := arm-none-eabi-

# Cross compiling and selecting different set of gcc/bin-utils
# ---------------------------------------------------------------------------
#
# When performing cross compilation for other architectures ARCH shall be set
# to the target architecture. (See arch/* for the possibilities).
# ARCH can be set during invocation of make:
# make ARCH=mcs51
# Another way is to have ARCH set in the environment.
# The default ARCH is the host where make is executed.

# CROSS_COMPILE specify the prefix used for all executables used
# during compilation. Only gcc and related bin-utils executables
# are prefixed with $(CROSS_COMPILE).
# CROSS_COMPILE can be set on the command line
# make CROSS_COMPILE=mcs51-linux-
# Alternatively CROSS_COMPILE can be set in the environment.
# Default value for CROSS_COMPILE is not to prefix executables
# Note: Some architectures assign CROSS_COMPILE in their arch/*/Makefile

ARCH		?= $(SUBARCH)
CROSS_COMPILE	?= $(SUBARCH_CROSS_COMPILE)

# Architecture as present in compile.h
UTS_MACHINE := $(ARCH)

KCONFIG_CONFIG	?= .config

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

HOSTCC       = gcc
HOSTCXX      = g++
HOSTINCLUDE  = -Iinclude
HOSTCFLAGS   = -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer $(HOSTINCLUDE)
HOSTCXXFLAGS = -O2

# Decide whether to build built-in, modular, or both.
# Normally, just do built-in.

KBUILD_BUILTIN := 1

export KBUILD_BUILTIN
export KBUILD_CHECKSRC KBUILD_SRC

# Beautify output
# ---------------------------------------------------------------------------
#
# Normally, we echo the whole command before executing it. By making
# that echo $($(quiet)$(cmd)), we now have the possibility to set
# $(quiet) to choose other forms of output instead, e.g.
#
#         quiet_cmd_cc_o_c = Compiling $(RELDIR)/$@
#         cmd_cc_o_c       = $(CC) $(c_flags) -o $@ -c $<
#
# If $(quiet) is empty, the whole command will be printed.
# If it is set to "quiet_", only the short version will be printed.
# If it is set to "silent_", nothing will be printed at all, since
# the variable $(silent_cmd_cc_o_c) doesn't exist.
#
# A simple variant is to prefix commands with $(Q) - that's useful
# for commands that shall be hidden in non-verbose mode.
#
#	$(Q)ln $@ :<
#
# If KBUILD_VERBOSE equals 0 then the above command will be hidden.
# If KBUILD_VERBOSE equals 1 then the above command is displayed.

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands

ifneq ($(findstring s,$(MAKEFLAGS)),)
  quiet=silent_
endif

export quiet Q KBUILD_VERBOSE


# Look for make include files relative to root of kernel src
MAKEFLAGS += --include-dir=$(srctree)

# We need some generic definitions.
include $(srctree)/arch/$(ARCH)/Kbuild.include
include $(srctree)/scripts/Kbuild.include

# Use TARGETINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
TARGETINCLUDE   := -Iinclude -Iinclude/std \
                   $(if $(KBUILD_SRC),-Iinclude2 -I$(srctree)/include)

# Make variables (CC, etc...)

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CROSS_COMPILE)cpp
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
CPPFLAGS        := -D__KERNEL__ $(TARGETINCLUDE)
CFLAGS          := -Wall -DKBUILD_CONF $(TARGETINCLUDE)
AFLAGS          := -D__ASSEMBLY__
SYMLINK		:= ln -fsn
#SYMLINK		:= cp -rf
AWK		= awk
PERL		= perl
CHECK		= sparse

CHECKFLAGS     := -D__STDC__ $(CF)
CFLAGS_KERNEL	=
AFLAGS_KERNEL	=



# Read KERNELRELEASE from include/config/kernel.release (if it exists)
KERNELRELEASE = $(shell cat include/config/kernel.release 2> /dev/null)
KERNELVERSION = $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION)

export VERSION PATCHLEVEL SUBLEVEL KERNELRELEASE KERNELVERSION
export ARCH CONFIG_SHELL HOSTCC HOSTCFLAGS HOSTINCLUDE CROSS_COMPILE
export AS LD CC CPP AR NM STRIP OBJCOPY MAKE AWK PERL
export UTS_MACHINE HOSTCXX HOSTCXXFLAGS CHECK CHECKFLAGS

export CPPFLAGS NOSTDINC_FLAGS TARGETINCLUDE OBJCOPYFLAGS LDFLAGS
export CFLAGS CFLAGS_KERNEL
export AFLAGS AFLAGS_KERNEL

# Files to ignore in find ... statements

RCS_FIND_IGNORE := \( -name SCCS -o -name BitKeeper -o -name .svn -o -name CVS -o -name .pc -o -name .hg -o -name .git \) -prune -o
export RCS_TAR_IGNORE := --exclude SCCS --exclude BitKeeper --exclude .svn --exclude CVS --exclude .pc --exclude .hg --exclude .git

# ===========================================================================
# Rules shared between *config targets and build targets

# Basic helpers built in scripts/
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

# To avoid any implicit rule to kick in, define an empty command.
scripts/basic/%: scripts_basic ;

PHONY += outputmakefile
# outputmakefile generates a Makefile in the output directory, if using a
# separate output directory. This allows convenient use of make in the
# output directory.
outputmakefile:
ifneq ($(KBUILD_SRC),)
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkmakefile \
	    $(srctree) $(objtree) $(VERSION) $(PATCHLEVEL)
endif

# To make sure we do not include .config for any of the *config targets
# catch them early, and hand them over to scripts/kconfig/Makefile
# It is allowed to specify more targets when calling make, including
# mixing *config targets and build targets.
# For example 'make oldconfig all'.
# Detect when mixed targets is specified, and make a second invocation
# of make so .config is not included in this case either (for *config).

no-dot-config-targets := clean mrproper distclean \
			 cscope TAGS tags help %docs check% \
			 include/target/version.h headers_% \
			 kernelrelease kernelversion

config-targets := 0
mixed-targets  := 0
dot-config     := 1

ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
		dot-config := 0
	endif
endif

ifneq ($(filter config %config,$(MAKECMDGOALS)),)
        config-targets := 1
        ifneq ($(filter-out config %config,$(MAKECMDGOALS)),)
                mixed-targets := 1
        endif
endif

ifeq ($(mixed-targets),1)
# ===========================================================================
# We're called with mixed targets (*config and build targets).
# Handle them one by one.

%:: FORCE
	$(Q)$(MAKE) -C $(srctree) KBUILD_SRC= $@

else
ifeq ($(config-targets),1)
# ===========================================================================
# *config targets only - make sure prerequisites are updated, and descend
# in scripts/kconfig to make the *config target

# Read arch specific Makefile to set KBUILD_DEFCONFIG as needed.
# KBUILD_DEFCONFIG may point out an alternative default configuration
# used for 'make defconfig'
include $(srctree)/arch/$(ARCH)/Makefile
export KBUILD_DEFCONFIG

config: scripts_basic outputmakefile FORCE
	$(Q)mkdir -p include/target include/config
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

%config: scripts_basic outputmakefile FORCE
	$(Q)mkdir -p include/target include/config
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

else
# ===========================================================================
# Build targets only - this includes kernel, arch specific targets, clean
# targets and others. In general all targets except *config targets.

# Additional helpers built in scripts/
# Carefully list dependencies so we do not try to build scripts twice
# in parallel
PHONY += scripts
scripts: scripts_basic include/config/auto.conf
	$(Q)$(MAKE) $(build)=$(@)

# Objects we will link into kernel / subdirs we need to visit
init-y		:= init/
libs-y		:= lib/
drivers-y	:= drivers/
core-y		:=

ifeq ($(dot-config),1)
# Read in config
-include include/config/auto.conf

# Read in dependencies to all Kconfig* files, make sure to run
# oldconfig if changes are detected.
-include include/config/auto.conf.cmd

# To avoid any implicit rule to kick in, define an empty command
$(KCONFIG_CONFIG) include/config/auto.conf.cmd: ;

# If .config is newer than include/config/auto.conf, someone tinkered
# with it and forgot to run make oldconfig.
# if auto.conf.cmd is missing then we are probably in a cleaned tree so
# we execute the config step to be sure to catch updated Kconfig files
include/config/auto.conf: $(KCONFIG_CONFIG) include/config/auto.conf.cmd
	$(Q)$(MAKE) -f $(srctree)/Makefile silentoldconfig

else
# Dummy target needed, because used as prerequisite
include/config/auto.conf: ;
endif # $(dot-config)

# The all: target is the default when no target is given on the
# command line.
# This allow a user to issue only 'make' to build a kernel
# Defaults ewokos but it is usually overridden in the arch makefile
all: ewokos

include $(srctree)/arch/$(ARCH)/Makefile
ifdef CONFIG_CC_OPT_SIZE
CFLAGS		+= -Os
else
CFLAGS		+= -O2
endif

ifdef CONFIG_FRAME_POINTER
CFLAGS		+= -fno-omit-frame-pointer -fno-optimize-sibling-calls
else
CFLAGS		+= -fomit-frame-pointer
endif

ifdef CONFIG_CC_GEN_DEBUG
CFLAGS		+= -g
endif

NOSTDINC_FLAGS += -nostdinc -nostdlib

CHECKFLAGS     += $(NOSTDINC_FLAGS)

# Default ewokos image to build when no specific target is given.
# KBUILD_IMAGE may be overruled on the command line or
# set in the environment
# Also any assignments in arch/$(ARCH)/Makefile take precedence over
# this default value
export KBUILD_IMAGE ?= ewokos

#
# INSTALL_PATH specifies where to place the updated kernel and map
# images. Default is /boot, but you can set it to other values
export	INSTALL_PATH ?= /boot

core-y		+= kernel/ mm/

ewokos-dirs	:= $(patsubst %/,%,$(filter %/, $(init-y) $(init-m) \
		     $(libs-y) $(libs-m) \
		     $(core-y) $(core-m) \
		     $(drivers-y) $(drivers-m)))

ewokos-alldirs	:= $(sort $(ewokos-dirs) $(patsubst %/,%,$(filter %/, \
		     $(lib-n) $(lib-) \
		     $(init-n) $(init-) \
		     $(core-n) $(core-) \
		     $(drivers-n) $(drivers-))))

init-y		:= $(patsubst %/, %/built-in.lib, $(init-y))
libs-y		:= $(patsubst %/, %/built-in.lib, $(libs-y))
core-y		:= $(patsubst %/, %/built-in.lib, $(core-y))
drivers-y	:= $(patsubst %/, %/built-in.lib, $(drivers-y))

# Build kernel
# ---------------------------------------------------------------------------
# ewokos is built from the objects selected by $(ewokos-init) and
# $(ewokos-main). Most are built-in.lib files from top-level directories
# in the kernel tree, others are specified in arch/$(ARCH)/Makefile.
# Ordering when linking is important, and $(ewokos-init) must be first.
#
# ewokos
#   ^
#   |
#   +-< $(ewokos-init)
#   |   +--< init/version.o + more
#   |
#   +--< $(ewokos-main)
#        +--< driver/built-in.lib mm/built-in.lib + more
#
# kernel version (uname -v) cannot be updated during normal
# descending-into-subdirs phase since we do not yet know if we need to
# update kernel.
# Therefore this step is delayed until just before final link of kernel.
#
# System.map is generated to document addresses of all kernel symbols

ewokos-init := $(head-y) $(init-y)
ewokos-main := $(core-y) $(libs-y) $(drivers-y)
ewokos-all  := $(ewokos-init) $(ewokos-main)
ewokos-lds  := arch/$(ARCH)/common/ewokos.lds
export KBUILD_VMLINUX_OBJS := $(ewokos-all)

# Generate new kernel version
quiet_cmd_ewokos_version = GEN     .version
      cmd_ewokos_version = set -e;                        \
	if [ ! -r .version ]; then			\
	  rm -f .version;				\
	  echo 1 >.version;				\
	else						\
	  mv .version .old_version;			\
	  expr 0$$(cat .old_version) + 1 >.version;	\
	fi;						\
	$(MAKE) $(build)=init

# The finally linked kernel.
quiet_cmd_ewokos = LD      $@
      cmd_ewokos = $(LD) $(LDFLAGS) $(LDFLAGS_ewokos) -o $@ \
	-T $(ewokos-lds) $(ewokos-init) \
	--start-group $(ewokos-main) --end-group \
	$(filter-out $(ewokos-lds) $(ewokos-init) $(ewokos-main) FORCE ,$^)
define rule_ewokos
	:
	+$(call cmd,ewokos)
	$(Q)echo 'cmd_$@ := $(cmd_ewokos)' > $(dot-target).cmd
endef

# kernel image - including updated kernel symbols
ewokos: $(ewokos-lds) $(ewokos-init) $(ewokos-main) FORCE
#ifdef CONFIG_HEADERS_CHECK
#	$(Q)$(MAKE) -f $(srctree)/Makefile headers_check
#endif
	$(call if_changed_rule,ewokos)
	$(Q)rm -f .old_version

# The actual objects are generated when descending,
# make sure no implicit rule kicks in
$(sort $(ewokos-init) $(ewokos-main) $(ewokos-lds)): $(ewokos-dirs) ;

# Handle descending into subdirectories listed in $(ewokos-dirs)
# Preset locale variables to speed up the build process. Limit locale
# tweaks to this spot to avoid wrong language settings when running
# make menuconfig etc.
# Error messages still appears in the original language

PHONY += $(ewokos-dirs)
$(ewokos-dirs): prepare scripts
	$(Q)$(MAKE) $(build)=$@

# Build the kernel release string
#
# The KERNELRELEASE value built here is stored in the file
# include/config/kernel.release, and is used when executing several
# make targets, such as "make install" or "make modules_install."
#
# The eventual kernel release string consists of the following fields,
# shown in a hierarchical format to show how smaller parts are concatenated
# to form the larger and final value, with values coming from places like
# the Makefile, kernel config options, make command line options and/or
# SCM tag information.
#
#	$(KERNELVERSION)
#	  $(VERSION)			eg, 2
#	  $(PATCHLEVEL)			eg, 6
#	  $(SUBLEVEL)			eg, 18
#	  $(EXTRAVERSION)		eg, -rc6
#	$(localver-full)
#	  $(localver)
#	    localversion*		(files without backups, containing '~')
#	    $(CONFIG_LOCALVERSION)	(from kernel config setting)
#	  $(localver-auto)		(only if CONFIG_LOCALVERSION_AUTO is set)
#	    ./scripts/setlocalversion	(SCM tag, if one exists)
#	    $(LOCALVERSION)		(from make command line if provided)
#
#  Note how the final $(localver-auto) string is included *only* if the
# kernel config option CONFIG_LOCALVERSION_AUTO is selected.  Also, at the
# moment, only git is supported but other SCMs can edit the script
# scripts/setlocalversion and add the appropriate checks as needed.

pattern = ".*/localversion[^~]*"
string  = $(shell cat /dev/null \
	   `find $(objtree) $(srctree) -maxdepth 1 -regex $(pattern) | sort -u`)

localver = $(subst $(space),, $(string) \
			      $(patsubst "%",%,$(CONFIG_LOCALVERSION)))

# If CONFIG_LOCALVERSION_AUTO is set scripts/setlocalversion is called
# and if the SCM is know a tag from the SCM is appended.
# The appended tag is determined by the SCM used.
#
# Currently, only git is supported.
# Other SCMs can edit scripts/setlocalversion and add the appropriate
# checks as needed.
ifdef CONFIG_LOCALVERSION_AUTO
	_localver-auto = $(shell $(CONFIG_SHELL) \
	                  $(srctree)/scripts/setlocalversion $(srctree))
	localver-auto  = $(LOCALVERSION)$(_localver-auto)
endif

localver-full = $(localver)$(localver-auto)

# Store (new) KERNELRELASE string in include/config/kernel.release
kernelrelease = $(KERNELVERSION)$(localver-full)
include/config/kernel.release: include/config/auto.conf FORCE
	$(Q)rm -f $@
	$(Q)echo $(kernelrelease) > $@


# Things we need to do before we recursively start building the kernel
# or the modules are listed in "prepare".
# A multi level approach is used. prepareN is processed before prepareN-1.
# archprepare is used in arch Makefiles and when processed asm symlink,
# version.h and scripts_basic is processed / created.

# Listed in dependency order
PHONY += prepare archprepare prepare0 prepare1 prepare2 prepare3

# prepare3 is used to check if we are building in a separate output directory,
# and if so do:
# 1) Check that make has not been executed in the kernel src $(srctree)
# 2) Create the include2 directory, used for the second asm symlink
prepare3: include/config/kernel.release
ifneq ($(KBUILD_SRC),)
	@echo '  Using $(srctree) as source for kernel'
	$(Q)if [ -f $(srctree)/.config -o -d $(srctree)/include/config ]; then \
		echo "  $(srctree) is not clean, please run 'make mrproper'";\
		echo "  in the '$(srctree)' directory.";\
		/bin/false; \
	fi;
	$(Q)if [ ! -d include2 ]; then mkdir -p include2; fi;
	$(Q)ln -fsn $(srctree)/include/asm-$(ARCH) include2/asm
endif

# prepare2 creates a makefile if using a separate output directory
prepare2: prepare3 outputmakefile

prepare1: prepare2 include/target/version.h include/target/utsrelease.h \
                   include/asm include/config/auto.conf

archprepare: prepare1 scripts_basic

prepare0: archprepare FORCE
	$(Q)$(MAKE) $(build)=.

# All the preparing..
prepare: prepare0

# Leave this as default for preprocessing ewokos.lds.S, which is now
# done in arch/$(ARCH)/kernel/Makefile

export CPPFLAGS_ewokos.lds += -P -C -U$(ARCH)

# FIXME: The asm symlink changes when $(ARCH) changes. That's
# hard to detect, but I suppose "make mrproper" is a good idea
# before switching between archs anyway.
# FIXME: SDCC can not recognize cygwin's symlink

include/asm:
	@echo '  SYMLINK $@ -> arch/$(ARCH)/include/asm'
	$(Q)if [ ! -d include ]; then mkdir -p include; fi;
	@$(SYMLINK) $(TOPDIR)/arch/$(ARCH)/include/asm $@

# Generate some files
# ---------------------------------------------------------------------------

# KERNELRELEASE can change from a few different places, meaning version.h
# needs to be updated, so this check is forced on all builds

uts_len := 64
define filechk_utsrelease.h
	if [ `echo -n "$(KERNELRELEASE)" | wc -c ` -gt $(uts_len) ]; then \
	  echo '"$(KERNELRELEASE)" exceeds $(uts_len) characters' >&2;    \
	  exit 1;                                                         \
	fi;                                                               \
	(echo \#define UTS_RELEASE \"$(KERNELRELEASE)\";)
endef

define filechk_version.h
	(echo \#define LINUX_VERSION_CODE $(shell                             \
	expr $(VERSION) \* 65536 + $(PATCHLEVEL) \* 256 + $(SUBLEVEL));     \
	echo '#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))';)
endef

include/target/version.h: $(srctree)/Makefile FORCE
	$(call filechk,version.h)

include/target/utsrelease.h: include/config/kernel.release FORCE
	$(call filechk,utsrelease.h)

# ---------------------------------------------------------------------------

PHONY += depend dep
depend dep:
	@echo '*** Warning: make $@ is unnecessary now.'

# ---------------------------------------------------------------------------
# Kernel headers
INSTALL_HDR_PATH=$(objtree)/usr
export INSTALL_HDR_PATH

HDRARCHES=$(filter-out generic,$(patsubst $(srctree)/include/asm-%/Kbuild,%,$(wildcard $(srctree)/include/asm-*/Kbuild)))

PHONY += headers_install_all
headers_install_all: include/target/version.h scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts scripts/unifdef
	$(Q)for arch in $(HDRARCHES); do \
	 $(MAKE) ARCH=$$arch -f $(srctree)/scripts/Makefile.headersinst obj=include BIASMDIR=-bi-$$arch ;\
	 done

PHONY += headers_install
headers_install: include/target/version.h scripts_basic FORCE
	@if [ ! -r $(srctree)/include/asm-$(ARCH)/Kbuild ]; then \
	  echo '*** Error: Headers not exportable for this architecture ($(ARCH))'; \
	  exit 1 ; fi
	$(Q)$(MAKE) $(build)=scripts scripts/unifdef
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.headersinst obj=include

PHONY += headers_check_all
headers_check_all: headers_install_all
	$(Q)for arch in $(HDRARCHES); do \
	 $(MAKE) ARCH=$$arch -f $(srctree)/scripts/Makefile.headersinst obj=include BIASMDIR=-bi-$$arch HDRCHECK=1 ;\
	 done

PHONY += headers_check
headers_check: headers_install
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.headersinst obj=include HDRCHECK=1

###
# Cleaning is done on three levels.
# make clean     Delete most generated files
#                Leave enough to build external modules
# make mrproper  Delete the current configuration, and all generated files
# make distclean Remove editor backup files, patch leftover files and the like

# Directories & files removed with 'make clean'
CLEAN_FILES +=	ewokos ewokos.strip ewokos.map \
                .tmp_version .tmp_ewokos* .tmp_ewokos.map
CLEAN_DIRS += include/asm

# Directories & files removed with 'make mrproper'
MRPROPER_DIRS  += include/config include2 usr/include
MRPROPER_FILES += .config .config.old .version .old_version \
                  include/target/autoconf.h include/target/version.h      \
                  include/target/utsrelease.h                            \
		  Module.symvers tags TAGS cscope*

# clean - Delete most, but leave enough to build external modules
#
clean: rm-dirs  := $(CLEAN_DIRS)
clean: rm-files := $(CLEAN_FILES)
clean-dirs      := $(addprefix _clean_,$(srctree) $(ewokos-alldirs))

PHONY += $(clean-dirs) clean archclean
$(clean-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _clean_%,%,$@)

clean: archclean $(clean-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '*.rel' -o -name '*.sym' -o -name '*.lst' \
		-o -name '*.hex' -o -name '*.mem' -o -name '*.lnk' \
		-o -name '*.asm' -o -name '*.rst' -o -name '*.lib' \
		-o -name '*.adb' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
		-type f -print | xargs rm -f

# mrproper - Delete all generated files, including .config
#
mrproper: rm-dirs  := $(wildcard $(MRPROPER_DIRS))
mrproper: rm-files := $(wildcard $(MRPROPER_FILES))
mrproper-dirs      := $(addprefix _mrproper_,Documentation/DocBook scripts)

PHONY += $(mrproper-dirs) mrproper archmrproper
$(mrproper-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _mrproper_%,%,$@)

mrproper: clean archmrproper $(mrproper-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)

# distclean
#
PHONY += distclean

distclean: mrproper
	@find $(srctree) $(RCS_FIND_IGNORE) \
		\( -name '*.orig' -o -name '*.rej' -o -name '*~' \
		-o -name '*.bak' -o -name '#*#' -o -name '.*.orig' \
		-o -name '.*.rej' -o -size 0 \
		-o -name '*%' -o -name '.*.cmd' -o -name 'core' \) \
		-type f -print | xargs rm -f


# Packaging of the kernel to various formats
# ---------------------------------------------------------------------------
# rpm target kept for backward compatibility
package-dir	:= $(srctree)/scripts/package

%pkg: include/config/kernel.release FORCE
	$(Q)$(MAKE) $(build)=$(package-dir) $@
rpm: include/config/kernel.release FORCE
	$(Q)$(MAKE) $(build)=$(package-dir) $@


# Brief documentation of the typical targets used
# ---------------------------------------------------------------------------

boards := $(wildcard $(srctree)/arch/$(ARCH)/configs/*_defconfig)
boards := $(notdir $(boards))

help:
	@echo  'Cleaning targets:'
	@echo  '  clean		  - Remove most generated files but keep the config and'
	@echo  '                    enough build support to build external modules'
	@echo  '  mrproper	  - Remove all generated files + config + various backup files'
	@echo  '  distclean	  - mrproper + remove editor backup and patch files'
	@echo  ''
	@echo  'Configuration targets:'
	@$(MAKE) -f $(srctree)/scripts/kconfig/Makefile help
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  all		  - Build all targets marked with [*]'
	@echo  '* ewokos	  - Build the microkernel operating system'
	@echo  '  dir/            - Build all files in dir and below'
	@echo  '  dir/file.[ois]  - Build specified target only'
	@echo  '  rpm		  - Build a kernel as an RPM package'
	@echo  '  tags/TAGS	  - Generate tags file for editors'
	@echo  '  cscope	  - Generate cscope index'
	@echo  '  kernelrelease	  - Output the release version string'
	@echo  '  kernelversion	  - Output the version stored in Makefile'
	@if [ -r $(srctree)/include/asm-$(ARCH)/Kbuild ]; then \
	 echo  '  headers_install - Install sanitised kernel headers to INSTALL_HDR_PATH'; \
	 echo  '                    (default: $(INSTALL_HDR_PATH))'; \
	 fi
	@echo  ''
	@echo  'Static analysers'
	@echo  '  checkstack      - Generate a list of stack hogs'
	@echo  '  namespacecheck  - Name space analysis on compiled kernel'
	@if [ -r $(srctree)/include/asm-$(ARCH)/Kbuild ]; then \
	 echo  '  headers_check   - Sanity check on exported headers'; \
	 fi
	@echo  ''
	@echo  'Kernel packaging:'
	@$(MAKE) $(build)=$(package-dir) help
	@echo  ''
	@echo  'Documentation targets:'
	@$(MAKE) -f $(srctree)/Documentation/DocBook/Makefile dochelp
	@echo  ''
	@echo  'Architecture specific targets ($(ARCH)):'
	@$(if $(archhelp),$(archhelp),\
		echo '  No architecture specific help defined for $(ARCH)')
	@echo  ''
	@$(if $(boards), \
		$(foreach b, $(boards), \
		printf "  %-24s - Build for %s\\n" $(b) $(subst _defconfig,,$(b));) \
		echo '')

	@echo  '  make V=0|1 [targets] 0 => quiet build (default), 1 => verbose build'
	@echo  '  make V=2   [targets] 2 => give reason for rebuild of target'
	@echo  '  make O=dir [targets] Locate all output files in "dir", including .config'
	@echo  '  make C=1   [targets] Check all c source with $$CHECK (sparse by default)'
	@echo  '  make C=2   [targets] Force check of all c source with $$CHECK'
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets marked with [*] '
	@echo  'For further info see the ./README file'


# Documentation targets
# ---------------------------------------------------------------------------
%docs: scripts_basic FORCE
	$(Q)$(MAKE) $(build)=Documentation/DocBook $@

# Generate tags for editors
# ---------------------------------------------------------------------------

#We want __srctree to totally vanish out when KBUILD_OUTPUT is not set
#(which is the most common case IMHO) to avoid unneeded clutter in the big tags file.
#Adding $(srctree) adds about 20M on i386 to the size of the output file!

ifeq ($(src),$(obj))
__srctree =
else
__srctree = $(srctree)/
endif

ifeq ($(ALLSOURCE_ARCHS),)
ALLINCLUDE_ARCHS := $(ARCH)
else
#Allow user to specify only ALLSOURCE_PATHS on the command line, keeping existing behavour.
ALLINCLUDE_ARCHS := $(ALLSOURCE_ARCHS)
endif

ALLSOURCE_ARCHS := $(ARCH)

define find-sources
        ( for ARCH in $(ALLSOURCE_ARCHS) ; do \
	       find $(__srctree)arch/$${ARCH} $(RCS_FIND_IGNORE) \
	            -name $1 -print; \
	  done ; \
	  find $(__srctree)include $(RCS_FIND_IGNORE) \
	       \( -name config -o -name 'asm-*' \) -prune \
	       -o -name $1 -print; \
	  for ARCH in $(ALLINCLUDE_ARCHS) ; do \
	       find $(__srctree)include/asm-$${ARCH} $(RCS_FIND_IGNORE) \
	            -name $1 -print; \
	  done ; \
	  find $(__srctree)include/asm-generic $(RCS_FIND_IGNORE) \
	       -name $1 -print; \
	  find $(__srctree) $(RCS_FIND_IGNORE) \
	       \( -name include -o -name arch \) -prune -o \
	       -name $1 -print; \
	  )
endef

define all-sources
	$(call find-sources,'*.[chS]')
endef
define all-kconfigs
	$(call find-sources,'Kconfig*')
endef
define all-defconfigs
	$(call find-sources,'defconfig')
endef

define xtags
	if $1 --version 2>&1 | grep -iq exuberant; then \
	    $(all-sources) | xargs $1 -a \
		-I __initdata,__exitdata,__acquires,__releases \
		-I EXPORT_SYMBOL,EXPORT_SYMBOL_GPL \
		--extra=+f --c-kinds=+px \
		--regex-asm='/^ENTRY\(([^)]*)\).*/\1/'; \
	    $(all-kconfigs) | xargs $1 -a \
		--langdef=kconfig \
		--language-force=kconfig \
		--regex-kconfig='/^[[:blank:]]*config[[:blank:]]+([[:alnum:]_]+)/\1/'; \
	    $(all-defconfigs) | xargs -r $1 -a \
		--langdef=dotconfig \
		--language-force=dotconfig \
		--regex-dotconfig='/^#?[[:blank:]]*(CONFIG_[[:alnum:]_]+)/\1/'; \
	elif $1 --version 2>&1 | grep -iq emacs; then \
	    $(all-sources) | xargs $1 -a; \
	    $(all-kconfigs) | xargs $1 -a \
		--regex='/^[ \t]*config[ \t]+\([a-zA-Z0-9_]+\)/\1/'; \
	    $(all-defconfigs) | xargs -r $1 -a \
		--regex='/^#?[ \t]?\(CONFIG_[a-zA-Z0-9_]+\)/\1/'; \
	else \
	    $(all-sources) | xargs $1 -a; \
	fi
endef

quiet_cmd_cscope-file = FILELST cscope.files
      cmd_cscope-file = (echo \-k; echo \-q; $(all-sources)) > cscope.files

quiet_cmd_cscope = MAKE    cscope.out
      cmd_cscope = cscope -b

cscope: FORCE
	$(call cmd,cscope-file)
	$(call cmd,cscope)

quiet_cmd_TAGS = MAKE   $@
define cmd_TAGS
	rm -f $@; \
	$(call xtags,etags)
endef

TAGS: FORCE
	$(call cmd,TAGS)

quiet_cmd_tags = MAKE   $@
define cmd_tags
	rm -f $@; \
	$(call xtags,ctags)
endef

tags: FORCE
	$(call cmd,tags)


# Scripts to check various things for consistency
# ---------------------------------------------------------------------------

includecheck:
	find * $(RCS_FIND_IGNORE) \
		-name '*.[hcS]' -type f -print | sort \
		| xargs $(PERL) -w scripts/checkincludes.pl

versioncheck:
	find * $(RCS_FIND_IGNORE) \
		-name '*.[hcS]' -type f -print | sort \
		| xargs $(PERL) -w scripts/checkversion.pl

namespacecheck:
	$(PERL) $(srctree)/scripts/namespace.pl

endif #ifeq ($(config-targets),1)
endif #ifeq ($(mixed-targets),1)

PHONY += checkstack kernelrelease kernelversion

CHECKSTACK_ARCH := $(ARCH)
checkstack:
	$(PERL) $(src)/scripts/checkstack.pl $(CHECKSTACK_ARCH)

kernelrelease:
	$(if $(wildcard include/config/kernel.release), $(Q)echo $(KERNELRELEASE), \
	$(error kernelrelease not valid - run 'make prepare' to update it))
kernelversion:
	@echo $(KERNELVERSION)

# Single targets
# ---------------------------------------------------------------------------
# Single targets are compatible with:
# - build whith mixed source and output
# - build with separate output dir 'make O=...'
# - external modules
#
#  target-dir => where to store outputfile
#  build-dir  => directory in kernel source tree to use

build-dir  = $(patsubst %/,%,$(dir $@))
target-dir = $(dir $@)

%.s: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.rel: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.s: %.S prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.o: %.S prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

# Modules
/: prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir)
%/: prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir)

# FIXME Should go into a make.lib or something
# ===========================================================================

quiet_cmd_rmdirs = $(if $(wildcard $(rm-dirs)),CLEAN   $(wildcard $(rm-dirs)))
      cmd_rmdirs = rm -rf $(rm-dirs)

quiet_cmd_rmfiles = $(if $(wildcard $(rm-files)),CLEAN   $(wildcard $(rm-files)))
      cmd_rmfiles = rm -f $(rm-files)


a_flags = -Wp,-MD,$(depfile) $(AFLAGS) $(AFLAGS_KERNEL) \
	  $(NOSTDINC_FLAGS) $(CPPFLAGS) \
	  $(modkern_aflags) $(EXTRA_AFLAGS) $(AFLAGS_$(basetarget).o)

quiet_cmd_as_o_S = AS      $@
cmd_as_o_S       = $(CC) $(a_flags) -c -o $@ $<

# read all saved command lines

targets := $(wildcard $(sort $(targets)))
cmd_files := $(wildcard .*.cmd $(foreach f,$(targets),$(dir $(f)).$(notdir $(f)).cmd))

ifneq ($(cmd_files),)
  $(cmd_files): ;	# Do not try to update included dependency files
  include $(cmd_files)
endif

# Shorthand for $(Q)$(MAKE) -f scripts/Makefile.clean obj=dir
# Usage:
# $(Q)$(MAKE) $(clean)=dir
clean := -f $(if $(KBUILD_SRC),$(srctree)/)scripts/Makefile.clean obj

endif	# skip-makefile

PHONY += FORCE
FORCE:

# Cancel implicit rules on top Makefile, `-rR' will apply to sub-makes.
Makefile: ;

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable se we can use it in if_changed and friends.
.PHONY: $(PHONY)
