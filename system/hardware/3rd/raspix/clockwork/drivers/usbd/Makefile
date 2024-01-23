include ../../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../../build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib -Llib -lnosys

CFLAGS += -I $(BUILD_DIR)/include -Iinclude

USBD_OBJS = clockwork_usbd.o

USBD = $(TARGET_DIR)/drivers/raspix/clockwork_usbd

$(USBD): $(USBD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		lib/libcsud.a
	$(LD) -Ttext=100 $(USBD_OBJS) -o $(USBD) $(LDFLAGS) -larch_bcm283x -lewoksys -lc -lgloss -lcsud 

clean:
	rm -f $(USBD_OBJS)
