deps_config := \
	init/Kconfig \
	lib/Kconfig \
	drivers/Kconfig \
	kernel/Kconfig \
	arch/arm/mach-raspi2/Kconfig \
	arch/arm/mach-versatilepb/Kconfig \
	arch/arm/common/Kconfig \
	arch/arm/Kconfig

include/config/auto.conf: \
	$(deps_config)

$(deps_config): ;
