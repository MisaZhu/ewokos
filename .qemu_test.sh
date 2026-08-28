#!/bin/bash
# Regression test: login root/root on tty0, run cat /bin/ls|dump|dump|dump
cd "$(dirname "$0")/machine.virt/kernel" || exit 1
{
  sleep 12
  echo "root"
  sleep 5
  echo "root"
  sleep 5
  echo "cat /bin/ls|dump|dump|dump"
  sleep 60
  echo "ls /bin"
  sleep 30
  sleep 600
} | qemu-system-aarch64 -M virt,highmem=on,accel=hvf -cpu host -m 8192 \
    -serial mon:stdio -nographic \
    -kernel kernel8.img \
    -drive file=../system/root_aarch64.img,format=raw,id=blk0,if=none \
    -device virtio-blk-device,drive=blk0 \
    > "$(dirname "$0")/.qemu_test.log" 2>&1
