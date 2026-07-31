KERNEL_TO_ARCH_OBJ = $(if $(filter ./,$(dir $(1))),$(ARCH)/$(notdir $(1)),$(dir $(1))$(ARCH)/$(notdir $(1)))
KERNEL_TO_ARCH_OBJS = $(foreach obj,$(1),$(call KERNEL_TO_ARCH_OBJ,$(obj)))

define KERNEL_OBJ_RULE_C
$(call KERNEL_TO_ARCH_OBJ,$(1)): $(basename $(1)).c
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
endef

define KERNEL_OBJ_RULE_CC
$(call KERNEL_TO_ARCH_OBJ,$(1)): $(basename $(1)).cc
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) -c $$< -o $$@
endef

define KERNEL_OBJ_RULE_CPP
$(call KERNEL_TO_ARCH_OBJ,$(1)): $(basename $(1)).cpp
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) -c $$< -o $$@
endef

define KERNEL_OBJ_RULE_S
$(call KERNEL_TO_ARCH_OBJ,$(1)): $(basename $(1)).S
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
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
