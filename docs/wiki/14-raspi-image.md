# 14 Building the Image and Flashing It to a Raspberry Pi

> Language: **English** | [中文](14-raspi-image.zh.md)
>
> Goal: pack the whole system into an SD card and boot it on a real
> Raspberry Pi. Includes: an inventory of build artifacts, the SD card
> partition layout, config.txt, flashing steps, and watching over serial.

## 14.1 What Kind of SD Card Does a Pi Need

Recall the boot conventions from Ch. 03. The SD card must have this layout:

```
SD card
├── Partition 1 (FAT32, label bootfs)       ← the GPU firmware only understands FAT32
│   ├── bootcode.bin / start.elf / fixup.dat   GPU firmware
│   ├── bcm2710-rpi-3-b.dtb etc.               device trees (per model)
│   ├── config.txt                             boot configuration
│   └── kernel8.img                            ★ our kernel
│
└── Partition 2 (ext3, label rootfs)
    └── the complete root filesystem (/bin /sbin /etc /drivers ...)
```

The GPU firmware (`bootcode.bin` etc.) and the `config.txt` templates for
all boards are in [tools/bootfs/raspix/](../../tools/bootfs/raspix/), ready
to use out of the box.

The key contents of `config.txt`
([config.txt](../../tools/bootfs/raspix/config.txt)):

```ini
[all]
arm_64bit=1          # 64-bit boot (loads kernel8.img)
enable_uart=1        # enable the UART (essential for watching boot logs)
force_turbo=1

[pi3]
core_freq=250        # fixed GPU clock → stable Mini UART baud rate
...
```

## 14.2 Building All the Artifacts

```bash
> cd machines/raspix/system
> make          # produces ../kernel/kernel8.img and the rootfs
> make sd       # packs the rootfs into root_aarch64.img (a 512MB ext3 image)
```

Where the artifacts live:

| File | Description |
|------|------|
| `machines/raspix/kernel/kernel8.img` | the kernel (goes to partition 1) |
| `machines/raspix/system/root_aarch64.img` | the root filesystem image (written to partition 2) |

> **Important pitfall**: building a single driver/app directory only
> updates the files under `system/build/aarch64/raspix/rootfs/` — it does
> **not** update `root_aarch64.img`. You must re-run `make sd` before
> flashing, or you'll be flashing the old system. When in doubt, compare
> timestamps: `ls -l root_aarch64.img` should be newer than the binaries
> you just compiled.

## 14.3 Making the SD Card (macOS)

The repo provides a one-shot partitioning script
[tools/makesd.sh](../../tools/makesd.sh). What it does: unmount all
partitions → create an MBR partition table → format partition 1 as FAT32
(label `bootfs`) → format partition 2 with `mke2fs -t ext3 -b 4096 -I 128`.

```bash
# First confirm the SD card's device number (be very sure — don't pick your hard drive!)
> diskutil list
> sudo tools/makesd.sh /dev/disk4
```

Then:

```bash
# 1. Write the firmware and kernel into partition 1 (FAT32; macOS mounts it directly)
> cp tools/bootfs/raspix/* /Volumes/bootfs/
> cp machines/raspix/kernel/kernel8.img /Volumes/bootfs/
> diskutil unmountDisk /dev/disk4
```

**Note**: macOS cannot mount ext3, so partition 2's content must be written
in a Linux environment (or a Linux VM/QEMU). Two ways:

```bash
# Method A (recommended, on Linux): dd the image straight into partition 2
> sudo dd if=systems/raspix/root_aarch64.img of=/dev/sdX2 bs=1M
> sudo e2fsck -f /dev/sdX2   # verify

# Method B (on Linux): copy in with e2cp
> sudo e2cp -rap rootfs-dir/* /dev/sdX2:/
```

> Don't "find a way" to mount ext3 on macOS to copy files — third-party
> drivers will corrupt permissions, symlinks, and device nodes.

## 14.4 Making the SD Card (Linux)

```bash
> lsblk                                # confirm the device, e.g. /dev/sdX
> if sudo tools/makesd.sh is unavailable, partition manually:
> sudo parted /dev/sdX mklabel msdos
> sudo parted /dev/sdX mkpart primary fat32 4MiB 132MiB
> sudo parted /dev/sdX mkpart primary ext3 132MiB 100%
> sudo mkfs.vfat -F 32 -n bootfs /dev/sdX1
> sudo dd if=.../root_aarch64.img of=/dev/sdX2 bs=1M
> sudo mount /dev/sdX1 /mnt/boot
> sudo cp tools/bootfs/raspix/* machines/raspix/kernel/kernel8.img /mnt/boot/
```

## 14.5 Hook Up the Serial Cable and Witness the Boot

Strongly recommended: use a USB-to-TTL serial cable to watch the boot
process (it also serves as the interactive terminal):

```
USB-TTL cable         Pi 40-pin header
─────────────────────────────────────
GND (black)  ──────────► Pin 6  (GND)
RXD (white)  ──────────► Pin 8  (GPIO14 TXD)
TXD (green)  ──────────► Pin 10 (GPIO15 RXD)
```

> Cross-connect: the cable's RX goes to the Pi's TX, the cable's TX to the
> Pi's RX. Wire it up before powering on.

Open a serial terminal on your computer:

```bash
# macOS
> screen /dev/tty.usbserial-XXXX 115200     # Ctrl-A K to exit

# Linux
> minicom -D /dev/ttyUSB0 -b 115200
```

Insert the SD card, connect the cable, power on. A few seconds later the
familiar text appears in the serial console:

```
=== ewokos booting ===
kernel: init kernel malloc     ... [OK]
...
-----------------------------------------------------
 ______           ______  _    _ ...
```

Then init starts the services, and finally the prompt appears —
**this is your own operating system, running on real hardware**.

If there's no graphics output, don't panic: first confirm over serial that
the system is fine, then check the HDMI/display and the display settings in
`config.txt`.

## 14.6 Notes for Real-Hardware Debugging

Experience from practice:

1. **Hardware QEMU can't emulate can only be tested on real machines**: the
   Pi's SDIO/WiFi (`wland`) and similar hardware are not emulated by QEMU,
   so related changes can only be verified by flashing a card;
2. **Flash a fresh image after every change**: see the pitfall note in
   §14.2;
3. There's no GDB on a real machine; the serial log is the primary scene —
   build the habit of adding `klog/slog` on suspicious paths (Ch. 15);
4. Model differences: Pi2/3's MMIO base is `0x3F000000`, Pi4's is
   `0xFE000000` — that's why
   [start.c](../../machines/raspix/kernel/bsp/start.c) branches on the CPU
   model when mapping the page tables.

## 14.7 Exercises

1. Walk the whole path once: build → flash → serial boot; screenshot your
   first real-hardware boot;
2. Change `enable_uart=0` in `config.txt`, observe what happens, then
   change it back;
3. Run the `hello` command you wrote in Ch. 12 on the real machine.

## 14.8 Summary

- The SD card has two partitions: FAT32 bootfs (firmware + config.txt +
  kernel) + ext3 rootfs;
- `make sd` produces the root filesystem image; after changing the system
  you must repack;
- macOS handles partitioning and the bootfs; writing the ext3 rootfs must
  happen in a Linux environment;
- The serial cable is the lifeline of real-machine development.

Next chapter: systematic debugging methods and the roadmap ahead — the
course's final chapter.
