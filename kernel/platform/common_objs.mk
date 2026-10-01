# default to parallel build; sub-makes share the jobserver via $(MAKE)
ifeq ($(MAKELEVEL),0)
ifeq ($(filter -j%,$(MAKEFLAGS)),)
NPROCS := $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
MAKEFLAGS += -j$(NPROCS)
endif
endif

# ---------------------------------------------------------------------------
# Per-project object isolation.
#
# Every machine (machines/<name>/kernel, machine.virt/kernel, ...) compiles the
# SAME shared kernel/ sources, but with its own codegen flags: ARCH_VER, SMP,
# PAGE_SIZE_4K/16K/64K, DEBUG, -mcpu/-march and machine headers via -I. make
# tracks only source timestamps, never compiler flags, so if two projects
# emitted objects into one directory the second build would treat the first
# project's .o as "up to date" and link it silently -- e.g. an SMP board
# linking a UP object (local TLBIALL instead of broadcast TLBIALLIS, ACTLR.SMP
# unset), or a 16K-page image linking 4K-page objects. The result is an image
# that boots "sometimes" and mimics a hardware cache/coherency fault.
#
# Fix: key the object subdirectory by the owning PROJECT, so objects compiled
# for one machine can never be picked up by another. The kernel Makefile always
# lives at <machine>/kernel, hence the machine name is CURDIR's parent; objects
# land in e.g. kernel/kernel/src/arm/miyoo/. A machine may export MACHINE=...
# in its config.mk to override the derived name. ccache still de-duplicates
# identical compiles across projects, so the per-project split costs little.
#
# A platform make.rule may still set KERNEL_OBJ_DIR explicitly to opt out.
# ---------------------------------------------------------------------------
MACHINE ?= $(notdir $(abspath $(CURDIR)/..))
ifeq ($(strip $(MACHINE)),)
MACHINE := default
endif
KERNEL_OBJ_DIR ?= $(ARCH)/$(MACHINE)

KERNEL_TO_ARCH_OBJ = $(if $(filter ./,$(dir $(1))),$(KERNEL_OBJ_DIR)/$(notdir $(1)),$(dir $(1))$(KERNEL_OBJ_DIR)/$(notdir $(1)))
KERNEL_TO_ARCH_OBJS = $(foreach obj,$(1),$(call KERNEL_TO_ARCH_OBJ,$(obj)))

define KERNEL_OBJ_RULE_C
$(call KERNEL_TO_ARCH_OBJ,$(1)): $(basename $(1)).c
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) -MMD -MP -c $$< -o $$@
endef

define KERNEL_OBJ_RULE_CC
$(call KERNEL_TO_ARCH_OBJ,$(1)): $(basename $(1)).cc
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) -MMD -MP -c $$< -o $$@
endef

define KERNEL_OBJ_RULE_CPP
$(call KERNEL_TO_ARCH_OBJ,$(1)): $(basename $(1)).cpp
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) -MMD -MP -c $$< -o $$@
endef

define KERNEL_OBJ_RULE_S
$(call KERNEL_TO_ARCH_OBJ,$(1)): $(basename $(1)).S
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) -MMD -MP -c $$< -o $$@
endef

define KERNEL_OBJ_RULE_s
$(call KERNEL_TO_ARCH_OBJ,$(1)): $(basename $(1)).s
	@mkdir -p $$(dir $$@)
	$$(AS) $$(ASFLAGS) -o $$@ $$<
endef

define KERNEL_EMIT_OBJ_RULES
$(foreach obj,$(sort $(KERNEL_ALL_RAW_OBJS)),\
$(if $(wildcard $(basename $(obj)).c),$(eval $(call KERNEL_OBJ_RULE_C,$(obj))),)\
$(if $(wildcard $(basename $(obj)).cc),$(eval $(call KERNEL_OBJ_RULE_CC,$(obj))),)\
$(if $(wildcard $(basename $(obj)).cpp),$(eval $(call KERNEL_OBJ_RULE_CPP,$(obj))),)\
$(if $(wildcard $(basename $(obj)).S),$(eval $(call KERNEL_OBJ_RULE_S,$(obj))),)\
$(if $(wildcard $(basename $(obj)).s),$(eval $(call KERNEL_OBJ_RULE_s,$(obj))),))
endef
