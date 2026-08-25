SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_X_OBJS = $(ARCH)/src/X.o $(ARCH)/src/XWin.o $(ARCH)/src/XWM.o $(ARCH)/src/XTheme.o

LIB_X = $(TARGET_DIR)/lib/libx++.a

$(LIB_X): $(LIB_X_OBJS)
	$(AR) crs $(LIB_X) $(LIB_X_OBJS) 
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_X_OBJS)
	rm -f $(LIB_X)

