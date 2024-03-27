SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

GCORES_OBJS = gcores.o

GCORES_DIR = $(TARGET_DIR)/apps/gcores
GCORES = $(GCORES_DIR)/gcores

all: $(GCORES_DIR) $(GCORES)

$(GCORES): $(GCORES_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(GCORES_OBJS) -o $(GCORES) $(LDFLAGS) -lttf -lfont -lx++ -lx -lgraph -lbsp -lsconf -lupng $(EWOK_LIBC) -lcxx

$(GCORES_DIR):
	mkdir -p $(GCORES_DIR)
	cp -r res $(GCORES_DIR)

clean:
	rm -f $(GCORES_OBJS)
	rm -fr $(GCORES_DIR)

