include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CXXFLAGS += -Iinclude -I$(TARGET_DIR)/include

LIB_WIDGET_OBJS =  src/Widget.o \
		src/Container.o \
		src/Image.o \
		src/Label.o \
		src/RootWidget.o \
		src/WidgetWin.o

WIDGET = $(TARGET_DIR)/lib/libwidget++.a

$(WIDGET): $(LIB_WIDGET_OBJS)
	$(AR) rT $(WIDGET) $(LIB_WIDGET_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_WIDGET_OBJS)
	rm -f $(WIDGET)

