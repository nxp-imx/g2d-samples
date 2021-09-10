#!/bin/bash
  export PATH=/opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/:$PATH
#  export ROOTFS_USR=/nfsroot/nfs_L3.0.35_4.0.0_130424/usr
  export ROOTFS_USR=/nfsroot/nfs_L3.0.35_1.1.0_121218/usr
  export CROSS_COMPILE=arm-fsl-linux-gnueabi-
  make clean
  make
