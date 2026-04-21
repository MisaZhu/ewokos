SYS_ROOT_DIR=../../..
include $(SYS_ROOT_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(TARGET_DIR)/lib -L$(BUILD_DIR)/lib

CXXFLAGS += -I$(BUILD_DIR)/include -I libs/litehtml/include

BROWSER_OBJS = src/main.o

BROWSER_DIR = $(TARGET_DIR)/apps/xBrowser
BROWSER = $(BROWSER_DIR)/xBrowser

all: $(BROWSER)

$(BROWSER): $(BROWSER_OBJS) \
		$(BUILD_DIR)/lib/liblitehtml.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libjpeg.a \
		$(BUILD_DIR)/lib/libsvg.a \
		$(BUILD_DIR)/lib/libtga.a \
		$(BUILD_DIR)/lib/libcxx.a
	mkdir -p $(BROWSER_DIR)
	$(LD) -Ttext=100 $(BROWSER_OBJS) -o $(BROWSER) $(LDFLAGS) \
		$(EWOK_LIB_X) -llitehtml -ltinyhttpsc \
		$(EWOK_LIB_GRAPH) \
		$(EWOK_LIBC) -lcxx
	cp -r res $(BROWSER_DIR)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(BROWSER_OBJS)
	rm -fr $(BROWSER_DIR)
