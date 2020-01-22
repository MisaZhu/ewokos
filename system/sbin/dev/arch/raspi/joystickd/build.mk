RASPI_JOYSTICKD_OBJS = $(ROOT_DIR)/sbin/dev/arch/raspi/joystickd/joystickd.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi/lib/gpio_arch.o

RASPI_JOYSTICKD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi/joystickd

PROGS += $(RASPI_JOYSTICKD)
CLEAN += $(RASPI_JOYSTICKD_OBJS)

$(RASPI_JOYSTICKD): $(RASPI_JOYSTICKD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI_JOYSTICKD_OBJS) $(LIB_OBJS) -o $(RASPI_JOYSTICKD) $(LDFLAGS)
