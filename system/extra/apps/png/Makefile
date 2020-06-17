CXX = arm-none-eabi-g++
LD = arm-none-eabi-ld

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

# flags
CXXFLAGS = -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-memset \
         -fno-builtin-snprintf \
         -fno-builtin-sprintf \
         -fno-builtin-fprintf \
				 -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -fno-rtti -fno-exceptions

#CXXFLAGS +=  -g
CXXFLAGS +=  -O2

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CXXFLAGS += -I $(TARGET_DIR)/include

PNG_OBJS = png.o

PNG = $(TARGET_DIR)/apps/png

$(PNG): $(PNG_OBJS) \
		$(TARGET_DIR)/lib/libewokc.a \
		$(TARGET_DIR)/lib/libx++.a \
		$(TARGET_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(PNG_OBJS) -o $(PNG) $(LDFLAGS)  -lgraphxx -lupng -lx++ -lewokc -lc -lcxx

clean:
	rm -f $(PNG_OBJS)

