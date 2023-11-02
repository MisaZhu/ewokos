include ../../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../../build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

CLOCKWORK_FBD_OBJS = fbd.o

FBD = $(TARGET_DIR)/drivers/raspix/clockwork_fbd

$(FBD): $(CLOCKWORK_FBD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(CLOCKWORK_FBD_OBJS) -o $(FBD) $(LDFLAGS) -larch_bcm283x -lfbd -lupng -lgraph -lsconf -lewokc -lc

clean:
	rm -f $(CLOCKWORK_FBD_OBJS)
