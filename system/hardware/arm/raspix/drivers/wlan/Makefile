include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I. -I $(BUILD_DIR)/include

BRCMFMACD_OBJS = sdio/sdhci.o \
				 sdio/mmc.o \
				 sdio/sdio.o \
				 \
				 utils/qbuf.o \
				 utils/utils.o \
				 utils/log.o \
				 utils/skb.o \
				 utils/config.o \
				 utils/pbkdf2.o \
				 \
				 brcm/brcm.o \
				 brcm/chip.o \
				 brcm/firmware.o \
				 brcm/command.o \
				 brcm/console.o \
				 main.o

 
BRCMFMACD = $(TARGET_DIR)/drivers/raspix/wland

$(BRCMFMACD): $(BRCMFMACD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(BRCMFMACD_OBJS) -o $(BRCMFMACD) $(LDFLAGS) -ltinyjson -larch_bcm283x -lewoksys -lc -lgloss 

clean:
	rm -f $(BRCMFMACD_OBJS)
