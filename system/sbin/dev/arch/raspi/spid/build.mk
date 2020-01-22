RASPI_SPID_OBJS = \
	$(ROOT_DIR)/sbin/dev/arch/raspi/lib/gpio_arch.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi/lib/spi_arch.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi/spid/spid.o 

RASPI_SPID = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi/spid

PROGS += $(RASPI_SPID)
CLEAN += $(RASPI_SPID_OBJS)

$(RASPI_SPID): $(RASPI_SPID_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI_SPID_OBJS) $(LIB_OBJS) -o $(RASPI_SPID) $(LDFLAGS)
