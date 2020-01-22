RASPI_SDD_OBJS = $(ROOT_DIR)/sbin/dev/arch/raspi/sdd/sdd.o
RASPI_SDD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi/sdd

PROGS += $(RASPI_SDD)
CLEAN += $(RASPI_SDD_OBJS)

$(RASPI_SDD): $(RASPI_SDD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI_SDD_OBJS) $(LIB_OBJS) -o $(RASPI_SDD) $(LDFLAGS)
