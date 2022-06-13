include ../../../../../make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

GPIO_MONITOR_OBJS = gpiomonitor.o

GPIO_MONITOR = $(TARGET_DIR)/bin/x/gpiomonitor

$(GPIO_MONITOR): $(GPIO_MONITOR_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(GPIO_MONITOR_OBJS) -o $(GPIO_MONITOR) $(LDFLAGS) -lfonts -lgraph -lx++ -lx -lupng -larch_bcm283x -lewokc -lc -lcxx 

clean:
	rm -f $(GPIO_MONITOR_OBJS)

