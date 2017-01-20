#
# Makefile for sensor chip drivers.
#
ifneq ($(KERNELRELEASE),)
   obj-m := bypassdrv.o
#   obj-m += hwmon.o
#   obj-m += hwmon-vid.o
   
else
   KERNELDIR ?= /lib/modules/$(shell uname -r)/build
   PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
in:default
	/sbin/insmod bypassdrv.ko
	mknod /dev/bypass c 243 0 
#uninstall pesudo
un:
	rm -f /dev/bypass
	/sbin/rmmod bypassdrv

clean:
	rm -rf *.ko *.mod.* *.o .tmp_versions *.cmd
endif

EXTRA_CFLAGS += -DDEBUG

