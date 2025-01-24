ROOT_DIR = ../../../../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

include $(ROOT_DIR)/hardware/$(ARCH)/make.rule


LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

EPAPERD_OBJS = epaperd.o fbd.o

EPAPERD = $(TARGET_DIR)/drivers/$(MF)/epaperd

$(EPAPERD): $(EPAPERD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(EPAPERD_OBJS) -o $(EPAPERD) $(LDFLAGS) -lfbd -lupng -lgraph -lbsp -lsconf  -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(EPAPERD_OBJS)
