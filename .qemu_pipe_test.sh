#!/bin/bash
cd /Users/mingmingzhu/work/ewokos/machine.virt/kernel
{
  sleep 30
  for i in 1 2 3; do
    printf 'ps|dump|dump|dump|dump > /tmp/r%d\r' "$i"
    sleep 25
  done
  printf 'ls -l /tmp\r'
  sleep 8
  printf 'ps\r'
  sleep 12
  printf '\x01x'
  sleep 2
} | qemu-system-aarch64 -M virt,highmem=on,accel=hvf -cpu host -m 8192 -smp 4 -serial mon:stdio -nographic -kernel kernel8.img -drive file=../system/root_aarch64.img,format=raw,id=blk0,if=none -device virtio-blk-device,drive=blk0 -snapshot > /Users/mingmingzhu/work/ewokos/.qemu_pipe_test.log 2>&1
