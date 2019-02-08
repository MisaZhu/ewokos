#!/bin/bash
cd kernel
make clean
cd ../system
make clean
cd ../lib/src
rm -fr *.o
