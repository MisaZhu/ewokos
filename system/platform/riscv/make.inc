KERNEL_DIR = $(EWOKOS_ROOT_DIR)/kernel
KERNEL_H = \
	$(KERNEL_DIR)/kernel/include/proto_t.h \
	$(KERNEL_DIR)/kernel/include/syscalls.h \
	$(KERNEL_DIR)/kernel/include/signals.h \
	$(KERNEL_DIR)/kernel/include/interrupt.h \
	$(KERNEL_DIR)/kernel/include/sysinfo.h \
	$(KERNEL_DIR)/kernel/include/kevent.h \
	$(KERNEL_DIR)/kernel/include/procinfo.h \
	$(KERNEL_DIR)/platform/$(ARCH)/arch/common/include/arch_context.h

kernel_heads:
	mkdir -p ${BUILD_DIR}/include
	mkdir -p ${BUILD_DIR}/lib
	mkdir -p ${BUILD_DIR}/rootfs
	cp -r $(KERNEL_H) $(BUILD_DIR)/include/
	
extra: 
	cd $(EWOKOS_ROOT_DIR)/sw.extra/; make HW=$(HW)
	cp -r $(EWOKOS_ROOT_DIR)/sw.extra/build/$(HW)/apps $(SYS_ROOT_DIR)/build/$(HW) 
	cp -r $(EWOKOS_ROOT_DIR)/sw.extra/build/$(HW)/bin $(SYS_ROOT_DIR)/build/$(HW)
	cp -r $(EWOKOS_ROOT_DIR)/sw.extra/build/$(HW)/sbin $(SYS_ROOT_DIR)/build/$(HW)
	cp -r $(EWOKOS_ROOT_DIR)/sw.extra/usr $(SYS_ROOT_DIR)/build/$(HW)