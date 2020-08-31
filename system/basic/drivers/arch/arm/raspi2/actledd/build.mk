RASPI2_ACTLEDD_OBJS = $(ROOT_DIR)/drivers/arch/arm/raspi2/actledd/actledd.o

RASPI2_ACTLEDD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/arm/raspi2/actledd

PROGS += $(RASPI2_ACTLEDD)
CLEAN += $(RASPI2_ACTLEDD_OBJS)

$(RASPI2_ACTLEDD): $(RASPI2_ACTLEDD_OBJS) \
		$(BUILD_DIR)/lib/libarch_raspi2.a
	$(LD) -Ttext=100 $(RASPI2_ACTLEDD_OBJS) -o $(RASPI2_ACTLEDD) $(LDFLAGS) -larch_raspi2 -lewokc -lc
