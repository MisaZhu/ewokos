RASPI2_TTYD_OBJS = $(ROOT_DIR)/drivers/arch/arm/raspi2/ttyd/ttyd.o

RASPI2_TTYD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/arm/raspi2/ttyd

PROGS += $(RASPI2_TTYD)
CLEAN += $(RASPI2_TTYD_OBJS)

$(RASPI2_TTYD): $(RASPI2_TTYD_OBJS) \
	$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(RASPI2_TTYD_OBJS) -o $(RASPI2_TTYD) $(LDFLAGS) -larch_bcm283x -lewokc -lc
