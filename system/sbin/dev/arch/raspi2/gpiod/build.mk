RASPI2_GPIOD_OBJS = \
	$(ROOT_DIR)/sbin/dev/arch/raspi2/lib/gpio_arch.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi2/gpiod/gpiod.o 

RASPI2_GPIOD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi2/gpiod

PROGS += $(RASPI2_GPIOD)
CLEAN += $(RASPI2_GPIOD_OBJS)

$(RASPI2_GPIOD): $(RASPI2_GPIOD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI2_GPIOD_OBJS) $(LIB_OBJS) -o $(RASPI2_GPIOD) $(LDFLAGS)
