include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I include -I $(BUILD_DIR)/include 

BRCMFMACD_OBJS = main.o \
				 sdhci.o \
				 mmc.o \
				 brcm.o \
				 skb.o \
				 sdio.o \
				 chip.o \
				 firmware.o \
				 command.o \
				 console.o 
#			 	cfg80211.o
 
BRCMFMACD = $(TARGET_DIR)/drivers/raspix/wland

$(BRCMFMACD): $(BRCMFMACD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(BRCMFMACD_OBJS) -o $(BRCMFMACD) $(LDFLAGS) -larch_bcm283x -lewoksys -lc -lgloss 

clean:
	rm -f $(BRCMFMACD_OBJS)
