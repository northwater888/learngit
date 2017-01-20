#!/bin/sh
bb=(`lsmod|grep sbs`)
if [ $bb ];then
  rmmod sbs
fi
bb=(`lsmod|grep i2c_ec`)
if [ $bb ];then
  rmmod i2c-ec
fi
bb=(`lsmod|grep i2c_i801`)
if [ $bb ];then
  rmmod i2c-i801
fi
bb=(`lsmod|grep i2c_core`)
if [ $bb ];then
  rmmod i2c-core
fi
insmod /usr/bypass/i2c-core.ko
insmod /usr/bypass/i2c-i801.ko
insmod /usr/bypass/bypassdrv.ko
mknod /dev/bypass c 243 0 

/usr/bypass/ctl &
