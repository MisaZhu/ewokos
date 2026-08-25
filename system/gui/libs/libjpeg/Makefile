SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps
PREFIX := /opt/ewokos

LIBJPEG = $(TARGET_DIR)/lib/libjpeg.a
JPEG_CFLAGS = -Wno-incompatible-pointer-types \
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

OBJS = $(patsubst %.c,$(ARCH)/%.o,$(JPEG_SRCS))

$(LIBJPEG): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp -r *.h $(TARGET_DIR)/include

$(ARCH)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(JPEG_CFLAGS) -c $< -o $@

clean:
	rm -rf $(ARCH) libjpeg.a
	rm -rf $(DEPDIR)
