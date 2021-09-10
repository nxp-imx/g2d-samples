#/*---------------------------------------------------------------------------*/
#/* Copyright 2021 NXP                                                        */
#/*                                                                           */
#/* NXP Confidential. This software is owned or controlled by NXP and may only*/
#/* be used strictly in accordance with the applicable license terms.         */
#/* By expressly accepting such terms or by downloading, installing,          */
#/* activating and/or otherwise using the software, you are agreeing that you */
#/* have read, and that you agree to comply with and are bound by, such       */
#/* license terms. If you do not agree to be bound by the applicable license  */
#/* terms, then you may not retain, install, activate or otherwise use the    */
#/* software.                                                                 */
#/*---------------------------------------------------------------------------*/
#
#
# Linux build file for g2d ocl test
#
.PHONY: clean

ROOTFS ?= $(SDKTARGETSYSROOT)

ROOTFS_USR :=$(ROOTFS)/usr

OBJS := g2d_ocl_csc.o


CFLAGS += -I ../../include -I $(ROOTFS)/usr/include -I os/linux -g

LDFLAGS += -Wl,-rpath-link,$(ROOTFS)/usr/lib
LDFLAGS += -L ../../lib -lg2d -lOpenCL --sysroot=$(ROOTFS)

TARGET := g2d_ocl_csc

# build as shared library
$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	-@rm $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
