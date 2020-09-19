RASPI3_MBOX_ACTLEDD_OBJS = $(ROOT_DIR)/drivers/arch/arm/raspix/mbox_actledd/actledd.o

RASPI3_MBOX_ACTLEDD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/arm/raspix/mbox_actledd

PROGS += $(RASPI3_MBOX_ACTLEDD)
CLEAN += $(RASPI3_MBOX_ACTLEDD_OBJS)

$(RASPI3_MBOX_ACTLEDD): $(RASPI3_MBOX_ACTLEDD_OBJS) \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(RASPI3_MBOX_ACTLEDD_OBJS) -o $(RASPI3_MBOX_ACTLEDD) $(LDFLAGS) -larch_bcm283x -lewokc -lc
