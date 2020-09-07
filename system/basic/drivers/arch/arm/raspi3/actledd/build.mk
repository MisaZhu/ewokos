RASPI3_ACTLEDD_OBJS = $(ROOT_DIR)/drivers/arch/arm/raspi3/actledd/actledd.o

RASPI3_ACTLEDD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/arm/raspi3/actledd

PROGS += $(RASPI3_ACTLEDD)
CLEAN += $(RASPI3_ACTLEDD_OBJS)

$(RASPI3_ACTLEDD): $(RASPI3_ACTLEDD_OBJS) \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(RASPI3_ACTLEDD_OBJS) -o $(RASPI3_ACTLEDD) $(LDFLAGS) -larch_bcm283x -lewokc -lc
