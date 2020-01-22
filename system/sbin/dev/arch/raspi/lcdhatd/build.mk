RASPI_LCDHATD_OBJS = $(ROOT_DIR)/sbin/dev/arch/raspi/lcdhatd/lcdhatd.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi/lib/gpio_arch.o \
	$(ROOT_DIR)/sbin/dev/arch/raspi/lib/spi_arch.o 

RASPI_LCDHATD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi/lcdhatd

PROGS += $(RASPI_LCDHATD)
CLEAN += $(RASPI_LCDHATD_OBJS)

$(RASPI_LCDHATD): $(RASPI_LCDHATD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI_LCDHATD_OBJS) $(LIB_OBJS) -o $(RASPI_LCDHATD) $(LDFLAGS)
