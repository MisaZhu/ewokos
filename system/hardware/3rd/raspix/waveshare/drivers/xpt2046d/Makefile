include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

XPTOUCHD_OBJS = xpt2046d.o

XPTOUCHD = $(TARGET_DIR)/drivers/raspix/xpt2046d

$(XPTOUCHD): $(XPTOUCHD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libxpt2046.a 
	$(LD) -Ttext=100 $(XPTOUCHD_OBJS) -o $(XPTOUCHD) $(LDFLAGS) -lxpt2046 -larch_bcm283x -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(XPTOUCHD_OBJS)
