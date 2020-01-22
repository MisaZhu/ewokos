RASPI_ACTLEDD_OBJS = $(ROOT_DIR)/sbin/dev/arch/raspi/actledd/actledd.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi/lib/gpio_arch.o

RASPI_ACTLEDD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi/actledd

PROGS += $(RASPI_ACTLEDD)
CLEAN += $(RASPI_ACTLEDD_OBJS)

$(RASPI_ACTLEDD): $(RASPI_ACTLEDD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI_ACTLEDD_OBJS) $(LIB_OBJS) -o $(RASPI_ACTLEDD) $(LDFLAGS)
