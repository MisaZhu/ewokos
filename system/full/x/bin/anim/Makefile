include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

GANIM_OBJS = anim.o

GANIM = $(TARGET_DIR)/bin/x/anim

$(GANIM): $(GANIM_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(GANIM_OBJS) -o $(GANIM) $(LDFLAGS) -lfonts -lgraph -lx++ -lx -lupng -lewokc -lc -lcxx

clean:
	rm -f $(GANIM_OBJS)

