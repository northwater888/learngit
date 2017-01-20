ifneq ($(KERNELRELEASE),)
   obj-m := gpio34.o
   
else
   KERNELDIR ?= /lib/modules/$(shell uname -r)/build
   PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf *.ko *.mod.* *.o *.unsigned *.order *.symvers .tmp_versions *.cmd  gpio34
endif
