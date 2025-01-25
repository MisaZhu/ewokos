ROOT_DIR = ../../../../../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

EXTRA_LIBS_DIR = ../../../libs/xpt2046

include $(ROOT_DIR)/hardware/$(ARCH)/make.rule

LDFLAGS = -L$(BUILD_DIR)/lib 

CFLAGS += -I $(BUILD_DIR)/include -I $(EXTRA_LIBS_DIR)/include

XPTOUCHD_OBJS = xpt2046d.o $(EXTRA_LIBS_DIR)/src/xpt2046.o

XPTOUCHD = $(TARGET_DIR)/drivers/$(MF)/xpt2046d

$(XPTOUCHD): $(XPTOUCHD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(XPTOUCHD_OBJS) -o $(XPTOUCHD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(XPTOUCHD_OBJS)
