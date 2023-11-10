include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

HEADS_DIR = include/stl
HEADS = $(HEADS_DIR)/*.hpp

HEAD_CP: $(HEADS)
	cp -r $(HEADS_DIR) $(TARGET_DIR)/include

clean:
	rm -fr $(TARGET_DIR)/include/widget

