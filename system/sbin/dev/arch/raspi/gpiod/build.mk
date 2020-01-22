RASPI_GPIOD_OBJS = \
	$(ROOT_DIR)/sbin/dev/arch/raspi/lib/gpio_arch.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi/gpiod/gpiod.o 

RASPI_GPIOD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi/gpiod

PROGS += $(RASPI_GPIOD)
CLEAN += $(RASPI_GPIOD_OBJS)

$(RASPI_GPIOD): $(RASPI_GPIOD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI_GPIOD_OBJS) $(LIB_OBJS) -o $(RASPI_GPIOD) $(LDFLAGS)
