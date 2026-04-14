SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps
PREFIX := /opt/ewokos

LIBJPEG = $(TARGET_DIR)/lib/libjpeg.a
JPEG_CFLAGS = -march=armv8-a \
        -Wno-incompatible-pointer-types \
        -Wno-int-conversion \
        -Wno-implicit-function-declaration \
        -Wall -Wextra -fPIC \
        -fno-builtin-memset \
        -Wno-unused-parameter \
        -I $(TARGET_DIR)/include \
        -DHAVE_CONFIG_H \
        -I. \
        -O2 -ffreestanding \
        -std=gnu99

all: $(LIBJPEG)

# libjpeg source files - complete set for decompression
JPEG_SRCS = jcapimin.c jcapistd.c jdapimin.c jdapistd.c \
            jctrans.c jcparam.c jdatadst.c jcinit.c jcmaster.c \
            jcmarker.c jcomapi.c jutils.c jerror.c jmemmgr.c \
            jcprepct.c jccoefct.c jccolor.c jcsample.c jchuff.c \
            jdmaster.c jdinput.c jdcoefct.c jdpostct.c jdmainct.c jdmerge.c jdmarker.c \
            jdatasrc.c \
            jdcolor.c jdsample.c jdtrans.c \
            jdphuff.c jdhuff.c \
            jddctmgr.c \
            jquant1.c jquant2.c \
            jmemnobs.c jdarith.c \
            jidctint.c jidctred.c jidctfst.c jidctflt.c \
            jfdctint.c \
            jcdctmgr.c jcphuff.c jchuff.c \
            jfdctfst.c jfdctflt.c \
            jsimd_none.c

OBJS = $(JPEG_SRCS:.c=.o)

$(LIBJPEG): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp -r *.h $(TARGET_DIR)/include

%.o: %.c
	$(CC) $(JPEG_CFLAGS) -c $< -o $@

clean:
	rm -f *.o libjpeg.a
	rm -rf $(DEPDIR)
