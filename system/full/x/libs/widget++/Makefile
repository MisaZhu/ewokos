include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

HEADS_DIR = include/Widget
HEADS = $(HEADS_DIR)/Widget.h \
		$(HEADS_DIR)/Container.h \
		$(HEADS_DIR)/WidgetWin.h \
		$(HEADS_DIR)/Image.h

HEAD_CP: $(HEADS)
	cp -r $(HEADS_DIR) $(TARGET_DIR)/include

clean:
	rm -fr $(TARGET_DIR)/include/widget

