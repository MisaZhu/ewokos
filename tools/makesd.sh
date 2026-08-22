#!/bin/zsh
# macOS SD card partition & format script
# Pure MBR layout:
#   1: FAT32 512M bootfs (lowercase volume label)
#   2: overwrite partition2 to ext3 rootfs with mke2fs
# mke2fs params: -F -t ext3 -L rootfs -b 4096 -I 128
# Usage: sudo ./makesd.sh /dev/disk4
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: sudo $0 /dev/diskN"
    echo "Example: sudo ./makesd.sh /dev/disk4"
    exit 1
fi
SD_DISK="$1"

# Safety guard: prevent operating on system disk disk0
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

echo "[1] Unmount all SD card partitions"
diskutil unmountDisk "${SD_DISK}" || true

echo "[2] Create MBR partition table, two placeholder FAT32 partitions"
diskutil eraseDisk FAT32 TMP MBRFormat "${SD_DISK}"
diskutil partitionDisk "${SD_DISK}" MBRFormat \
    FAT32 dummy1 512M \
    FAT32 dummy2 0B

DISK_NUM=${SD_DISK#/dev/disk}
PART_BOOT="/dev/disk${DISK_NUM}s1"
PART_ROOT="/dev/disk${DISK_NUM}s2"
RPART_BOOT="/dev/rdisk${DISK_NUM}s1"
RPART_ROOT="/dev/rdisk${DISK_NUM}s2"

echo ""
echo "bootfs partition(block): ${PART_BOOT}"
echo "rootfs partition(block): ${PART_ROOT}"

if [[ ! -b "${PART_ROOT}" ]]; then
    echo "ERROR: ${PART_ROOT} block device NOT found!"
    exit 1
fi

# 真实挂载检测：查询mount命令输出，不使用diskutil info文本解析
is_mounted() {
    local dev="$1"
    if mount | grep -q "${dev}"; then
        return 0 # true: is mounted
    else
        return 1 # false: not mounted
    fi
}

unmount_retry() {
    local dev="$1"
    local max=4
    for ((i=1; i<=max;i++)); do
        if ! is_mounted "${dev}"; then
            echo "  ${dev} already unmounted."
            return 0
        fi
        echo "  Try ${i}/${max}: unmount ${dev}"
        diskutil unmount "${dev}" 2>/dev/null || true
        sleep 1.0
    done
    # last resort: force unmountDisk
    echo "  Final fallback: unmountDisk force ${SD_DISK}"
    diskutil unmountDisk force "${SD_DISK}" 2>/dev/null || true
    sleep 1.5
}

echo "[2.1] Trying to unmount partitions (kernel may hold references, retry)"
unmount_retry "${PART_BOOT}"
unmount_retry "${PART_ROOT}"

# 最终校验：mount命令确认无挂载
if is_mounted "${PART_BOOT}"; then
    echo "ERROR: ${PART_BOOT} is STILL mounted according to mount command, abort!"
    exit 1
fi
if is_mounted "${PART_ROOT}"; then
    echo "ERROR: ${PART_ROOT} is STILL mounted according to mount command, abort!"
    exit 1
fi

echo "[3] Re‑format partition1 with newfs_msdos, set lowercase label bootfs"
sudo newfs_msdos -F 32 -v bootfs "${RPART_BOOT}"

echo "[4] Re‑format ${PART_ROOT} as ext3 (-F force skip confirmation prompt)"
sudo "${MKE2FS}" -F -t ext3 -L rootfs -b 4096 -I 128 "${RPART_ROOT}"

echo ""
echo "✅ All operations completed!"
diskutil list "${SD_DISK}"
echo ""
echo "==== Important Notes ===="
echo "1. Partition1 bootfs: FAT32 with lowercase volume label 'bootfs'."
echo "2. Partition2 rootfs: ext3 filesystem. macOS cannot mount ext3 natively."
echo "3. Do NOT copy rootfs files onto ext3 partition under macOS."
echo "   It will corrupt permissions, symlinks and device nodes."
echo "4. Populate rootfs inside QEMU / real Linux environment."
echo "5. MBR partition‑type id for partition2 remains 0x0C(FAT32). Most Linux bootloaders ignore this and read superblock."
