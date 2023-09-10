include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(BUILD_DIR)/include

MP3PLAY_OBJS = src/mp3play.o

MP3PLAY = $(TARGET_DIR)/bin/mp3play

$(MP3PLAY): $(MP3PLAY_OBJS) \
		$(BUILD_DIR)/lib/libsoftfloat.a \
		$(BUILD_DIR)/lib/libewokc.a 
	$(LD) -Ttext=100 $(MP3PLAY_OBJS) -o $(MP3PLAY) $(LDFLAGS) -lsoftfloat -lewokc -lm -lc
	cp -r include/* $(BUILD_DIR)/include

clean:
	rm -f $(MP3PLAY_OBJS)

