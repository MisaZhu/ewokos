include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

FBD_OBJS = fbd.o ../../../../basic/utility/src/draw_logo.o

FBD = $(TARGET_DIR)/drivers/versatilepb/fbd

$(FBD): $(FBD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_vpb.a
	$(LD) -Ttext=100 $(FBD_OBJS) -o $(FBD) $(LDFLAGS) -larch_vpb -lgraph -lfonts -lupng -lewokc -lc

clean:
	rm -f $(FBD_OBJS)
