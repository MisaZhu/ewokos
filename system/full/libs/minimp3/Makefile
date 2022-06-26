include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(BUILD_DIR)/include

MP32PCM_OBJS = src/mp32pcm.o

MP32PCM = $(TARGET_DIR)/bin/mp32pcm

$(MP32PCM): $(MP32PCM_OBJS) \
		$(BUILD_DIR)/lib/libsoftfloat.a \
		$(BUILD_DIR)/lib/libewokc.a 
	$(LD) -Ttext=100 $(MP32PCM_OBJS) -o $(MP32PCM) $(LDFLAGS) -lsoftfloat -lewokc -lm -lc
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(MP32PCM_OBJS)

