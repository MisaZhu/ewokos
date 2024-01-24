include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

FBD_OBJS = fbd.o

FBD = $(TARGET_DIR)/drivers/nezha/fbd

$(FBD): $(FBD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libarch_nezha.a
	$(LD) -Ttext=100 $(FBD_OBJS) -o $(FBD) $(LDFLAGS) -larch_nezha -lgraph -lbsp -lfbd -lewoksys -lc -lgloss

clean:
	rm -f $(FBD_OBJS)
