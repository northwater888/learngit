#!/bin/sh
rmmod bypassdrv
rmmod i2c-i801
rmmod i2c-core
insmod  /lib/modules/`uname -r`/kernel/drivers/i2c/i2c-core.ko
insmod  /lib/modules/`uname -r`/kernel/drivers/i2c/busses/i2c-i801.ko
rm -f /dev/bypass
cp /tmp/rc.local /etc/rc.d/
