include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

GAME_KBD_OBJS = gamekbd.o

GAME_KBD = $(TARGET_DIR)/drivers/raspix/hat13_gamekbd

$(GAME_KBD): $(GAME_KBD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(GAME_KBD_OBJS) -o $(GAME_KBD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(GAME_KBD_OBJS)
