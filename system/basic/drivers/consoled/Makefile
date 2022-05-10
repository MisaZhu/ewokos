include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

CONSOLED_OBJS = consoled.o

CONSOLED = $(TARGET_DIR)/drivers/consoled

$(CONSOLED): $(CONSOLED_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libfb.a \
		$(BUILD_DIR)/lib/libconsole.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libfonts.a  \
		$(BUILD_DIR)/lib/libdisp.a 
	$(LD) -Ttext=100 $(CONSOLED_OBJS) -o $(CONSOLED) $(LDFLAGS) -lfonts -lfb -ldisp -lconsole -lgraph -lupng -lsconf -lewokc -lc

clean:
	rm -f $(CONSOLED_OBJS)
