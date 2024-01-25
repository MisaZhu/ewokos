SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs
LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I include -I $(BUILD_DIR)/include -I pce -D EOF=0 -D M_PI=3.1415926 -D SEEK_SET=0
CXXFLAGS += -I include -I $(BUILD_DIR)/include -I pce -D EOF=0 -D M_PI=3.1415926 -D SEEK_SET=0
MACINTOSH_OBJS = \
	pce/drivers/video/terminal.o \
	pce/drivers/video/keys.o \
	pce/drivers/video/null.o \
	pce/drivers/char/char-null.o \
	pce/drivers/char/char.o \
	pce/drivers/char/char-mouse.o \
	pce/drivers/char/char-stdio.o \
	pce/drivers/pri/pri-img-pbit.o \
	pce/drivers/pri/pri-enc-fm.o \
	pce/drivers/pri/pri-img-pri.o \
	pce/drivers/pri/pri-enc-mfm.o \
	pce/drivers/pri/pri-img.o \
	pce/drivers/pri/pri-img-tc.o \
	pce/drivers/pri/pri-img-woz.o \
	pce/drivers/pri/pri.o \
	pce/drivers/pri/pri-enc-gcr.o \
	pce/drivers/pti/pti-img-txt.o \
	pce/drivers/pti/pti-img-pcm.o \
	pce/drivers/pti/pti.o \
	pce/drivers/pti/pti-img-tap.o \
	pce/drivers/pti/pti-io.o \
	pce/drivers/pti/pti-img-pti.o \
	pce/drivers/pti/pti-img-cas.o \
	pce/drivers/pti/pti-img-wav.o \
	pce/drivers/psi/psi-img.o \
	pce/drivers/psi/psi-io.o \
	pce/drivers/psi/psi-img-pfdc4.o \
	pce/drivers/psi/psi-img-raw.o \
	pce/drivers/psi/psi-img-pfdc0.o \
	pce/drivers/psi/psi-img-mac.o \
	pce/drivers/psi/psi-img-ana.o \
	pce/drivers/psi/psi-img-stx.o \
	pce/drivers/psi/psi-img-msa.o \
	pce/drivers/psi/psi-img-xdf.o \
	pce/drivers/psi/psi-img-imd.o \
	pce/drivers/psi/psi-img-pfdc2.o \
	pce/drivers/psi/psi-img-pfdc.o \
	pce/drivers/psi/psi-img-dc42.o \
	pce/drivers/psi/psi-img-tc.o \
	pce/drivers/psi/psi-img-psi.o \
	pce/drivers/psi/psi.o \
	pce/drivers/psi/psi-img-pfdc1.o \
	pce/drivers/psi/psi-img-cp2.o \
	pce/drivers/psi/psi-img-td0.o \
	pce/drivers/sound/sound-wav.o \
	pce/drivers/sound/filter.o \
	pce/drivers/sound/sound.o \
	pce/drivers/sound/sound-null.o \
	pce/drivers/options.o \
	pce/drivers/block/blkqed.o \
	pce/drivers/block/blkpsi.o \
	pce/drivers/block/blkpce.o \
	pce/drivers/block/block.o \
	pce/drivers/block/blkpri.o \
	pce/drivers/block/blkram.o \
	pce/drivers/block/blkcow.o \
	pce/drivers/block/blkchd.o \
	pce/drivers/block/blkpbi.o \
	pce/drivers/block/blkpart.o \
	pce/drivers/block/blkdosem.o \
	pce/drivers/block/blkraw.o \
	pce/cpu/e68000/disasm.o \
	pce/cpu/e68000/ea.o \
	pce/cpu/e68000/ops-020.o \
	pce/cpu/e68000/e68000.o \
	pce/cpu/e68000/cc.o \
	pce/cpu/e68000/opcodes.o \
	pce/arch/macplus/sony.o \
	pce/arch/macplus/iwm-io.o \
	pce/arch/macplus/msg.o \
	pce/arch/macplus/serial.o \
	pce/arch/macplus/adb_keyboard.o \
	pce/arch/macplus/rtc.o \
	pce/arch/macplus/video.o \
	pce/arch/macplus/keyboard.o \
	pce/arch/macplus/iwm.o \
	pce/arch/macplus/hotkey.o \
	pce/arch/macplus/traps.o \
	pce/arch/macplus/mem.o \
	pce/arch/macplus/adb.o \
	pce/arch/macplus/macplus.o \
	pce/arch/macplus/scsi.o \
	pce/arch/macplus/cmd_68k.o \
	pce/arch/macplus/main.o \
	pce/arch/macplus/sound.o \
	pce/arch/macplus/hook.o \
	pce/arch/macplus/adb_mouse.o \
	pce/lib/srec.o \
	pce/lib/log.o \
	pce/lib/msgdsk.o \
	pce/lib/monitor.o \
	pce/lib/msg.o \
	pce/lib/cfg.o \
	pce/lib/iniram.o \
	pce/lib/iniata.o \
	pce/lib/string.o \
	pce/lib/brkpt.o \
	pce/lib/ihex.o \
	pce/lib/cmd.o \
	pce/lib/sysdep.o \
	pce/lib/console.o \
	pce/lib/inidsk.o \
	pce/lib/path.o \
	pce/lib/initerm.o \
	pce/lib/load.o \
	pce/lib/getopt.o \
	pce/lib/mhex.o \
	pce/lib/thex.o \
	pce/libini/scanner.o \
	pce/libini/strings.o \
	pce/libini/read.o \
	pce/libini/write.o \
	pce/libini/value.o \
	pce/libini/expr.o \
	pce/libini/section.o \
	pce/chipset/e6522.o \
	pce/chipset/e8530.o \
	pce/chipset/e6845.o \
	pce/chipset/e6560.o \
	pce/chipset/e68901.o \
	pce/chipset/clock/mc146818a.o \
	pce/chipset/clock/ds1743.o \
	pce/devices/pci-ata.o \
	pce/devices/video/video.o \
	pce/devices/parport.o \
	pce/devices/memory.o \
	pce/devices/pci.o \
	pce/devices/hdc.o \
	pce/devices/ata.o \
	pce/devices/cassette.o \
	pce/devices/nvram.o \
	pce/devices/slip.o \
	pce/devices/device.o \
	pce/devices/clock/ds1743.o \
	main.o

MACINTOSH_DIR = $(TARGET_DIR)/apps/macintosh
MACINTOSH = $(MACINTOSH_DIR)/macintosh

all: $(MACINTOSH_DIR) $(MACINTOSH)

$(MACINTOSH): $(MACINTOSH_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libopenlibm.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libcxx.a  
	$(LD) -Ttext=100 $(MACINTOSH_OBJS) -o $(MACINTOSH) $(LDFLAGS) -lx++ -lttf -lfont -lgraph -lbsp -lx  -lupng -ltextview -lsconf -lopenlibm  -lewoksys -lc -lgloss -lcxx 

$(MACINTOSH_DIR):
	mkdir -p $(MACINTOSH_DIR)
	cp -r res $(MACINTOSH_DIR)

clean:
	rm -f $(MACINTOSH_OBJS)
	rm -fr $(MACINTOSH_DIR)
