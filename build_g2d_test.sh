#!/bin/bash
  export PATH=/opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/:$PATH
  export ROOTFS_USR=/opt/freescale/usr/local/ubuntu_rootfs/usr
  export CROSS_COMPILE=arm-fsl-linux-gnueabi-
  make clean
  make
