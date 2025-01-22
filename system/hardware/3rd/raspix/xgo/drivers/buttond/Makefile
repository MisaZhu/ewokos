include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

XGO_BUTTOND_OBJS = xgo_buttond.o

XGO_BUTTOND = $(TARGET_DIR)/drivers/$(MF)/xgo_buttond

$(XGO_BUTTOND): $(XGO_BUTTOND_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(XGO_BUTTOND_OBJS) -o $(XGO_BUTTOND) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(XGO_BUTTOND_OBJS)
