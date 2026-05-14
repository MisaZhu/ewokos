SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

SND_PLAYER_OBJS = SndPlayer.o AudioPlayer.o

SND_PLAYER_DIR = $(TARGET_DIR)/apps/SndPlayer
SND_PLAYER = $(SND_PLAYER_DIR)/SndPlayer

all: $(SND_PLAYER_DIR) $(SND_PLAYER)

$(SND_PLAYER): $(SND_PLAYER_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libminimp3.a \
		$(BUILD_DIR)/lib/libvorbis.a \
		$(BUILD_DIR)/lib/libogg.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a
	$(LD) -Ttext=100 $(SND_PLAYER_OBJS) -o $(SND_PLAYER) $(LDFLAGS)  $(EWOK_LIB_X) -lminimp3 -lvorbis -logg -lm $(EWOK_LIB_GRAPH)  $(EWOK_LIBC) -lcxx

$(SND_PLAYER_DIR):		
	mkdir -p $(SND_PLAYER_DIR)
	cp -r res $(SND_PLAYER_DIR)

# Compile .cc files
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(SND_PLAYER_OBJS)
	rm -fr $(SND_PLAYER_DIR)
