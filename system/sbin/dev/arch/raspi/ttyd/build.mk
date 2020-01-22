RASPI_TTYD_OBJS = $(ROOT_DIR)/sbin/dev/arch/raspi/ttyd/ttyd.o

RASPI_TTYD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi/ttyd

PROGS += $(RASPI_TTYD)
CLEAN += $(RASPI_TTYD_OBJS)

$(RASPI_TTYD): $(RASPI_TTYD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI_TTYD_OBJS) $(LIB_OBJS) -o $(RASPI_TTYD) $(LDFLAGS)
