RASPI_ACTLEDD_OBJS = $(ROOT_DIR)/drivers/arch/arm/raspi/actledd/actledd.o

RASPI_ACTLEDD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/arm/raspi/actledd

PROGS += $(RASPI_ACTLEDD)
CLEAN += $(RASPI_ACTLEDD_OBJS)

$(RASPI_ACTLEDD): $(RASPI_ACTLEDD_OBJS) \
		$(BUILD_DIR)/lib/libarch_raspi.a
	$(LD) -Ttext=100 $(RASPI_ACTLEDD_OBJS) -o $(RASPI_ACTLEDD) $(LDFLAGS) -larch_raspi -lewokc -lc
