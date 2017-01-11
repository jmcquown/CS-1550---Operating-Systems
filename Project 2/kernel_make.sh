#!/usr/bin/env bash
cp sys.c linux-2.6.23.1/kernel/sys.c
gcc -m32 -o prodcons -I /u/OSLab/jmm330/linux-2.6.23.1/include/ prodcons.c
cd linux-2.6.23.1
make ARCH=i386 bzImage
cd ..