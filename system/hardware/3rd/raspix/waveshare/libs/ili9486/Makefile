include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

# flags
CFLAGS = -Wstrict-prototypes \
	 -marm \
	 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
	-fno-builtin-printf \
	-fno-builtin-memset \
	-fno-builtin-snprintf \
	-fno-builtin-sprintf \
	-fno-builtin-fprintf \
	-Wno-overlength-strings \
	-fno-builtin-exit -fno-builtin-stdio \
	 -std=c99 

#CFLAGS +=  -g
CFLAGS +=  -O2

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_ILI9486_OBJS = src/ili9486.o 

ILI9486 = $(TARGET_DIR)/lib/libili9486.a

$(ILI9486): $(LIB_ILI9486_OBJS)
	$(AR) rT $(ILI9486) $(LIB_ILI9486_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_ILI9486_OBJS)
	rm -f $(ILI9486)


