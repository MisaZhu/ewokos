include  ../../make.inc

XGO_OBJS = xgo.o

XGO_DIR = $(TARGET_DIR)/apps/xgo
XGO = $(XGO_DIR)/xgo

all: $(XGO_DIR) $(XGO)

$(XGO): $(XGO_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libxgo.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XGO_OBJS) -o $(XGO) $(LDFLAGS) -lwidget++ -lgraph -lbsp -lttf -lfont -lx++ -lx -lsconf -ltinyjson -lupng -lewokstl -lxgo $(EWOK_LIBC) -lcxx

$(XGO_DIR):
	mkdir -p $(XGO_DIR)
	cp -r res $(XGO_DIR)

clean:
	rm -f $(XGO_OBJS)
	rm -fr $(XGO_DIR)

