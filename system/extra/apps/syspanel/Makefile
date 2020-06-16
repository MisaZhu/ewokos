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

SYSPANEL_OBJS = syspanel.o

SYSPANEL = $(TARGET_DIR)/apps/syspanel

$(SYSPANEL): $(SYSPANEL_OBJS) \
		$(TARGET_DIR)/lib/libewokc.a \
		$(TARGET_DIR)/lib/libx++.a \
		$(TARGET_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(SYSPANEL_OBJS) -o $(SYSPANEL) $(LDFLAGS)  -lgraphxx -lx++ -lewokc -lc -lcxx -lsconf

clean:
	rm -f $(SYSPANEL_OBJS)

