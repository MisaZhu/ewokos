INIT_OBJS = $(ROOT_DIR)/sbin/init/init.o \
		$(ROOT_DIR)/sbin/init/sdinit.o \
		$(ROOT_DIR)/sbin/dev/arch/raspi/lib/gpio_arch.o \
		$(ROOT_DIR)/sbin/dev/arch/raspi/lib/spi_arch.o
	
INIT = $(TARGET_DIR)/$(ROOT_DIR)/sbin/init

PROGS += $(INIT)
CLEAN += $(INIT_OBJS)

$(INIT): $(INIT_OBJS) $(LIB_OBJS) $(LIB_CONSOLE_OBJS)
	$(LD) -Ttext=100 $(INIT_OBJS) $(LIB_OBJS) $(LIB_CONSOLE_OBJS) -o $(INIT) $(LDFLAGS)
	$(OBJDUMP) -D $(INIT) > $(BUILD_DIR)/asm/init.asm
