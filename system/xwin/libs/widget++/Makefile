SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include -I $(TARGET_DIR)/include/litehtml

LIB_WIDGET_OBJS =  $(ARCH)/src/Widget/Widget.o \
		$(ARCH)/src/Widget/Blank.o \
		$(ARCH)/src/Widget/Splitter.o \
		$(ARCH)/src/Widget/Split.o \
		$(ARCH)/src/Widget/SpriteWin.o \
		$(ARCH)/src/Widget/Container.o \
		$(ARCH)/src/Widget/Image.o \
		$(ARCH)/src/Widget/Label.o \
		$(ARCH)/src/Widget/ListBase.o \
		$(ARCH)/src/Widget/List.o \
		$(ARCH)/src/Widget/Grid.o \
		$(ARCH)/src/Widget/Scrollable.o \
		$(ARCH)/src/Widget/Scroller.o \
		$(ARCH)/src/Widget/Slider.o \
		$(ARCH)/src/Widget/Button.o \
		$(ARCH)/src/Widget/LabelButton.o \
		$(ARCH)/src/Widget/RootWidget.o \
		$(ARCH)/src/Widget/EditLine.o \
		$(ARCH)/src/Widget/Text.o \
		$(ARCH)/src/Widget/Stage.o \
		$(ARCH)/src/Widget/Columns.o \
		$(ARCH)/src/Widget/WidgetWin.o \
		$(ARCH)/src/Widget/WidgetX.o \
		$(ARCH)/src/Widget/RoundButton.o \
		$(ARCH)/src/Widget/RoundLabelButton.o \
		$(ARCH)/src/Widget/SpriteAnim.o \
		\
		$(ARCH)/src/WidgetEx/FileWidget.o \
		$(ARCH)/src/WidgetEx/Dialog.o \
		$(ARCH)/src/WidgetEx/ConfirmDialog.o \
		$(ARCH)/src/WidgetEx/FileDialog.o \
		$(ARCH)/src/WidgetEx/ConsoleWidget.o \
		$(ARCH)/src/WidgetEx/FontDialog.o \
		$(ARCH)/src/WidgetEx/ColorDialog.o \
		$(ARCH)/src/WidgetEx/Popup.o \
		$(ARCH)/src/WidgetEx/LayoutWidget.o \
		$(ARCH)/src/WidgetEx/LayoutWin.o \
		$(ARCH)/src/WidgetEx/Menu.o \
		$(ARCH)/src/WidgetEx/Menubar.o

WIDGET = $(TARGET_DIR)/lib/libwidget++.a

$(WIDGET): $(LIB_WIDGET_OBJS)
	$(AR) rcs $(WIDGET) $(LIB_WIDGET_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_WIDGET_OBJS)
	rm -f $(WIDGET)
