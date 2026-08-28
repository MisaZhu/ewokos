#!/bin/bash
# Headless QEMU with serial on a unix socket for interactive diagnosis.
cd /Users/mingmingzhu/work/ewokos/machine.virt/kernel || exit 1
rm -f /Users/mingmingzhu/work/ewokos/.serial.sock
exec qemu-system-aarch64 \
  -M virt,highmem=on,accel=hvf -cpu host -m 8192 -smp 4 \
  -serial unix:/Users/mingmingzhu/work/ewokos/.serial.sock,server,nowait \
  -nographic \
  -kernel kernel8.img \
  -drive file=../system/root_aarch64.img,format=raw,id=blk0,if=none \
  -device virtio-blk-device,drive=blk0 \
  -netdev user,id=net0,hostfwd=tcp:127.0.0.1:2222-:22 \
  -device virtio-net-device,netdev=net0
