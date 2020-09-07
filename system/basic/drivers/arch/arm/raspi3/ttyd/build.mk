RASPI3_TTYD_OBJS = $(ROOT_DIR)/drivers/arch/arm/raspi3/ttyd/ttyd.o

RASPI3_TTYD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/arm/raspi3/ttyd

PROGS += $(RASPI3_TTYD)
CLEAN += $(RASPI3_TTYD_OBJS)

$(RASPI3_TTYD): $(RASPI3_TTYD_OBJS) \
	$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(RASPI3_TTYD_OBJS) -o $(RASPI3_TTYD) $(LDFLAGS) -larch_bcm283x -lewokc -lc
