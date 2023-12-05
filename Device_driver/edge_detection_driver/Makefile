PWD := $(shell pwd)
obj-m += ip_edge_detect_driver.o
KERNEL = /home/sislab/frame_buffer/build/tmp/work-shared/zynq-generic/kernel-source
CROSS = /home/sislab/Downloads/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
EXTRA_CFLAGS = "-I$(KERNEL)/include"
all:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KERNEL)  M=`pwd` modules
clean:
	make -C $(KERNEL)  M=`pwd` clean

