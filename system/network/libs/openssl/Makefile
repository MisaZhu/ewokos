SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_OPENSSL_OBJS = src/ssl.o src/evp.o src/hash.o src/dh.o src/bn.o src/sha.o src/hmac.o src/rsa.o src/ed25519.o

OPENSSL = $(TARGET_DIR)/lib/libopenssl.a

$(OPENSSL): $(LIB_OPENSSL_OBJS)
	$(AR) rcs $(OPENSSL) $(LIB_OPENSSL_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_OPENSSL_OBJS)
	rm -f $(OPENSSL)
