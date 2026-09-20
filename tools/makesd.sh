#!/bin/zsh
# macOS SD card partition & format script (fast, diskutil-free partitioning)
# MBR layout:
#   1: FAT32 128M bootfs (lowercase volume label)
#   2: ext3 rootfs via mke2fs
#
# Why this design (learned the hard way on this host):
#   * `diskutil partitionDisk`/`unmountDisk` DO the work but then fail to return
#     (DiskArbitration deadlock) -> the script appears to hang forever.
#   * FAT32/ExFAT formatting a huge partition is slow; HFS+ placeholder still
#     leaves diskutil hanging at the final "Mounting disk".
#   So we partition with `fdisk` (instant, no format, no mount) and format the
#   raw slices ourselves with newfs_msdos/mke2fs. These write raw devices
#   directly and are unaffected by DiskArbitration state.
# Usage: sudo ./makesd.sh /dev/disk4
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: sudo $0 /dev/diskN"
    echo "Example: sudo ./makesd.sh /dev/disk4"
    exit 1
fi
SD_DISK="$1"

# Safety guard: never touch the system disk.
if [[ "${SD_DISK}" == "/dev/disk0" ]]; then
    echo "ERROR: Refuse to operate on disk0 (system disk)! Exit."
    exit 1
fi

MKE2FS="/opt/homebrew/opt/e2fsprogs/sbin/mke2fs"
if [[ ! -x "${MKE2FS}" ]]; then
    echo "ERROR: mke2fs not found, run: brew install e2fsprogs"
    exit 1
fi

echo "====================================="
echo "Target SD card: ${SD_DISK}"
echo "WARNING: ALL DATA on this card will be DESTROYED!"
read "ans?Proceed? [y/n]: "
if [[ "${ans}" != "y" && "${ans}" != "Y" ]]; then
    echo "Aborted by user."
    exit 0
fi

DISK_NUM=${SD_DISK#/dev/disk}
RDISK="/dev/rdisk${DISK_NUM}"
PART_BOOT="/dev/disk${DISK_NUM}s1"
PART_ROOT="/dev/disk${DISK_NUM}s2"
RPART_BOOT="/dev/rdisk${DISK_NUM}s1"
RPART_ROOT="/dev/rdisk${DISK_NUM}s2"

echo "[0] Pause Spotlight indexing (prevents mds from locking the card)"
mdutil -a -i off >/dev/null 2>&1 || true
trap 'mdutil -a -i on >/dev/null 2>&1 || true' EXIT

echo "[1] Unmount the disk (diskutil does the unmount but may not return; we"
echo "    wait until the volumes actually disappear, then reap the process)"
diskutil unmountDisk force "${SD_DISK}" >/dev/null 2>&1 &
UMPID=$!
for i in {1..40}; do
    mount | grep -q "${SD_DISK}" || break
    sleep 0.5
done
kill "${UMPID}" 2>/dev/null || true
wait "${UMPID}" 2>/dev/null || true
# Belt-and-suspenders: plain umount any slice that lingers (does not hang).
umount -f "${PART_BOOT}" 2>/dev/null || true
umount -f "${PART_ROOT}" 2>/dev/null || true

echo "[2] Wipe old MBR so the partition table is rebuilt from scratch"
dd if=/dev/zero of="${RDISK}" bs=512 count=1 2>/dev/null || true

echo "[3] Write MBR partition table with fdisk (instant, no format, no mount)"
# p1: FAT32(LBA,0x0C) start=2048 size=262144 (=128MiB)
# p2: Linux(0x83)     start=264192 size=rest (blank line => to end of disk)
# Leading 'y' answers the "initialize partition table? [y]" prompt that fdisk
# shows when the MBR signature is invalid (it is, we just wiped it).
fdisk -e "${RDISK}" >/dev/null 2>&1 <<'FDISK'
y
edit 1
0C
n
2048
262144
edit 2
83
n
264192

edit 3
0
edit 4
0
write
quit
FDISK

echo "[3.1] Wait for partition device nodes to appear"
for i in {1..20}; do
    [[ -e "${PART_ROOT}" ]] && break
    sleep 0.5
done
if [[ ! -e "${PART_ROOT}" ]]; then
    echo "ERROR: ${PART_ROOT} did not appear after writing MBR. Abort."
    exit 1
fi
# The fresh slices have no valid filesystem yet, so macOS won't auto-mount them;
# a best-effort umount just in case (never hangs).
umount -f "${PART_BOOT}" 2>/dev/null || true
umount -f "${PART_ROOT}" 2>/dev/null || true

echo "[4] Format partition1 as FAT32 with lowercase label 'bootfs'"
newfs_msdos -F 32 -v bootfs "${RPART_BOOT}"

echo "[5] Format partition2 as ext3"
"${MKE2FS}" -F -t ext3 -L rootfs -b 4096 -I 128 "${RPART_ROOT}"

echo ""
echo "✅ All operations completed!"
# Print the table with fdisk (NOT diskutil, which hangs on this host).
fdisk "${RDISK}"
echo ""
echo "==== Important Notes ===="
echo "1. Partition1 bootfs: FAT32 with lowercase volume label 'bootfs'."
echo "2. Partition2 rootfs: ext3. macOS cannot mount ext3 natively."
echo "3. Populate rootfs inside QEMU / real Linux, not under macOS."
