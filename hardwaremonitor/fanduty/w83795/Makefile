#
# Makefile for sensor chip drivers.
#
ifneq ($(KERNELRELEASE),)
   obj-m := w83795.o

   
else
   KERNELDIR ?= /lib/modules/$(shell uname -r)/build
   PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf *.ko *.mod.* *.o .tmp_versions *.cmd
endif

EXTRA_CFLAGS += -DDEBUG

