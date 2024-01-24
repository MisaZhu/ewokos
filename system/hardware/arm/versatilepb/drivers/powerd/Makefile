include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -Llib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

PMU_OBJS = powerd.o

PMU = $(TARGET_DIR)/drivers/versatilepb/powerd

$(PMU): $(PMU_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  \
		$(BUILD_DIR)/lib/libarch_vpb.a
	$(LD) -Ttext=100 $(PMU_OBJS) -o $(PMU) $(LDFLAGS) -larch_vpb -lewoksys -lc -lgloss

clean:
	rm -f $(PMU_OBJS)
