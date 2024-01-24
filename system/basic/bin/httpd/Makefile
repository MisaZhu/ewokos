include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

HTTPD_OBJS = httpd.o

HTTPD = $(TARGET_DIR)/bin/httpd

$(HTTPD): $(HTTPD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a 
	$(LD) -Ttext=100 $(HTTPD_OBJS) -o $(HTTPD) $(LDFLAGS)    -lsocket  -lewoksys -lc -lgloss

clean:
	rm -f $(HTTPD_OBJS)

