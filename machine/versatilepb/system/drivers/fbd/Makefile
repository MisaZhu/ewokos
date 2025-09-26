SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

FBD_OBJS = fbd.o 

FBD = $(TARGET_DIR)/drivers/versatilepb/fbd

$(FBD): $(FBD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libpng.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libarch_vpb.a
	$(LD) -Ttext=100 $(FBD_OBJS) -o $(FBD) $(LDFLAGS) -lgterminal -ltextgrid -lfbd -ltinyjson $(EWOK_LIB_GRAPH) $(EWOK_LIBC)

clean:
	rm -f $(FBD_OBJS)
