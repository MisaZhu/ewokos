include ../../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../../build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib -Llib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

PMU_OBJS = powerd.o

PMU = $(TARGET_DIR)/drivers/raspix/powerd

$(PMU): $(PMU_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(PMU_OBJS) -o $(PMU) $(LDFLAGS) -larch_bcm283x -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(PMU_OBJS)
