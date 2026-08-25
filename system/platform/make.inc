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

LIBEWOKSYS_DIR = $(EWOKOS_ROOT_DIR)/system/basic/libc/libewoksys

kernel_heads:
	mkdir -p ${BUILD_DIR}/include
	mkdir -p ${BUILD_DIR}/lib
	mkdir -p ${BUILD_DIR}/rootfs
	cp -r $(KERNEL_H) $(BUILD_DIR)/include/

ewok_heads:
# pre-install libc headers so libs (arch/bsp) built before basic can use
# pthread.h/sys/*.h (same set as libewoksys install_headers)
	mkdir -p $(BUILD_DIR)/include/ewoksys $(BUILD_DIR)/include/sys
	cp -r $(LIBEWOKSYS_DIR)/ewoksys/include/ewoksys/* $(BUILD_DIR)/include/ewoksys/
	cp -r $(LIBEWOKSYS_DIR)/include/sys/* $(BUILD_DIR)/include/sys/
	cp $(LIBEWOKSYS_DIR)/include/*.h $(BUILD_DIR)/include/

FINAL_DIR = build_$(ARCH)

final:
	@mkdir -p $(FINAL_DIR)
	@cp -r $(BUILD_DIR)/rootfs/* $(FINAL_DIR)/
	@rm -fr $(BUILD_DIR)/rootfs
	@echo "all done."
	
extra: 
	make HW=$(HW) -C $(EWOKOS_ROOT_DIR)/sw.extra
	cp -r $(EWOKOS_ROOT_DIR)/sw.extra/usr $(EWOKOS_ROOT_DIR)/system/build_$(ARCH)/$(HW)/rootfs/
	cp -r $(EWOKOS_ROOT_DIR)/sw.extra/data $(EWOKOS_ROOT_DIR)/system/build_$(ARCH)/$(HW)/rootfs/
	@make final

projs:
	make HW=$(HW) -C $(EWOKOS_ROOT_DIR)/projects
	@make final