include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CXXFLAGS += -Iinclude -I$(TARGET_DIR)/include

LIB_WIDGET_OBJS =  src/Widget/Widget.o \
		src/Widget/Blank.o \
		src/Widget/Container.o \
		src/Widget/Image.o \
		src/Widget/Label.o \
		src/Widget/ListBase.o \
		src/Widget/List.o \
		src/Widget/Grid.o \
		src/Widget/Scrollable.o \
		src/Widget/Scroller.o \
		src/Widget/Button.o \
		src/Widget/LabelButton.o \
		src/Widget/RootWidget.o \
		src/Widget/EditLine.o \
		src/Widget/WidgetWin.o \
		\
		src/WidgetEx/FileWidget.o \
		src/WidgetEx/Dialog.o \
		src/WidgetEx/FileDialog.o

WIDGET = $(TARGET_DIR)/lib/libwidget++.a

$(WIDGET): $(LIB_WIDGET_OBJS)
	$(AR) rT $(WIDGET) $(LIB_WIDGET_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_WIDGET_OBJS)
	rm -f $(WIDGET)

