#!/bin/sh

dd if=/dev/zero of=rootfs.ext2 bs=1024 count=2048
mkfs.ext2 rootfs.ext2

mkdir tmp
sudo mount rootfs.ext2 ./tmp
sudo mkdir ./tmp/sbin
sudo cp -av build/* ./tmp/sbin
sudo ls -l ./tmp/sbin
sudo umount ./tmp
rm -r tmp
