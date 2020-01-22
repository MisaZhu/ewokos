RASPI2_JOYSTICKD_OBJS = $(ROOT_DIR)/sbin/dev/arch/raspi2/joystickd/joystickd.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi2/lib/gpio_arch.o

RASPI2_JOYSTICKD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi2/joystickd

PROGS += $(RASPI2_JOYSTICKD)
CLEAN += $(RASPI2_JOYSTICKD_OBJS)

$(RASPI2_JOYSTICKD): $(RASPI2_JOYSTICKD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI2_JOYSTICKD_OBJS) $(LIB_OBJS) -o $(RASPI2_JOYSTICKD) $(LDFLAGS)
