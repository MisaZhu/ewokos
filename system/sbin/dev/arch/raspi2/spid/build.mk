RASPI2_SPID_OBJS = \
	$(ROOT_DIR)/sbin/dev/arch/raspi2/lib/gpio_arch.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi2/lib/spi_arch.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi2/spid/spid.o 

RASPI2_SPID = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi2/spid

PROGS += $(RASPI2_SPID)
CLEAN += $(RASPI2_SPID_OBJS)

$(RASPI2_SPID): $(RASPI2_SPID_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI2_SPID_OBJS) $(LIB_OBJS) -o $(RASPI2_SPID) $(LDFLAGS)
