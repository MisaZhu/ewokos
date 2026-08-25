SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CHGRP_OBJS = $(ARCH)/chgrp.o

CHGRP = $(TARGET_DIR)/bin/chgrp

$(CHGRP): $(CHGRP_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(CHGRP_OBJS) -o $(CHGRP) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(CHGRP_OBJS)
