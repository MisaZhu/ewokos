KERNEL_DIR = $(EWOKOS_ROOT_DIR)/kernel
KERNEL_H = \
	$(KERNEL_DIR)/kernel/include/proto_t.h \
	$(KERNEL_DIR)/kernel/include/syscalls.h \
	$(KERNEL_DIR)/kernel/include/signals.h \
	$(KERNEL_DIR)/kernel/include/interrupt.h \
	$(KERNEL_DIR)/kernel/include/sysinfo.h \
	$(KERNEL_DIR)/kernel/include/kevent.h \
	$(KERNEL_DIR)/kernel/include/procinfo.h \
	$(KERNEL_DIR)/platform/$(ARCH)/arch/common/include/arch_context.h \
	$(KERNEL_DIR)/platform/$(ARCH)/arch/common/include/ewokos_config.h

kernel_heads:
	mkdir -p ${BUILD_DIR}/include
	mkdir -p ${BUILD_DIR}/lib
	mkdir -p ${BUILD_DIR}/rootfs
	cp -r $(KERNEL_H) $(BUILD_DIR)/include/
	
extra: 
	cd $(EWOKOS_ROOT_DIR)/sw.extra/; make HW=$(HW)
	cp -r $(EWOKOS_ROOT_DIR)/sw.extra/usr $(EWOKOS_ROOT_DIR)/system/build/$(HW)/rootfs/
#	cp -r $(EWOKOS_ROOT_DIR)/sw.extra/etc $(EWOKOS_ROOT_DIR)/system/build/$(HW)/rootfs/
	cp -r $(EWOKOS_ROOT_DIR)/sw.extra/data $(EWOKOS_ROOT_DIR)/system/build/$(HW)/rootfs/
	mkdir -p build
	cp -r $(EWOKOS_ROOT_DIR)/system/build/$(HW)/rootfs/* build