#
# Makefile for sensor chip drivers.
#
ifneq ($(KERNELRELEASE),)
   obj-m := w83795.o
   #obj-m += i2c-core.o
   #obj-m += i2c-i801.o
   
else
   KERNELDIR ?= /lib/modules/$(shell uname -r)/build
   PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf *.ko *.mod.* *.o .tmp_versions *.cmd
endif


