RASPI_GPIO_ACTLEDD_OBJS = $(ROOT_DIR)/drivers/arch/arm/raspix/gpio_actledd/actledd.o

RASPI_GPIO_ACTLEDD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/arm/raspix/gpio_actledd

PROGS += $(RASPI_GPIO_ACTLEDD)
CLEAN += $(RASPI_GPIO_ACTLEDD_OBJS)

$(RASPI_GPIO_ACTLEDD): $(RASPI_GPIO_ACTLEDD_OBJS) \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(RASPI_GPIO_ACTLEDD_OBJS) -o $(RASPI_GPIO_ACTLEDD) $(LDFLAGS) -larch_bcm283x -lewokc -lc
