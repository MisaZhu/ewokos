include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

HEAD_CP:
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -fr $(TARGET_DIR)/include/widget

