SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

MP3PLAY_OBJS = src/mp3player.o

MP3PLAY = $(TARGET_DIR)/lib/libminimp3.a

$(MP3PLAY): $(MP3PLAY_OBJS) \
		$(EWOK_LIBC_A)  
	$(AR) rcs $(MP3PLAY) $(MP3PLAY_OBJS)
	cp -r include/* $(BUILD_DIR)/include

clean:
	rm -f $(MP3PLAY)
	rm -f $(MP3PLAY_OBJS)

