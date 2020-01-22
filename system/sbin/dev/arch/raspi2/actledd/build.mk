RASPI2_ACTLEDD_OBJS = $(ROOT_DIR)/sbin/dev/arch/raspi2/actledd/actledd.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi2/lib/gpio_arch.o

RASPI2_ACTLEDD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi2/actledd

PROGS += $(RASPI2_ACTLEDD)
CLEAN += $(RASPI2_ACTLEDD_OBJS)

$(RASPI2_ACTLEDD): $(RASPI2_ACTLEDD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI2_ACTLEDD_OBJS) $(LIB_OBJS) -o $(RASPI2_ACTLEDD) $(LDFLAGS)
