#!/bin/sh
cp /etc/rc.d/rc.local /tmp
rm -f /etc/rc.d/rc.local
cp /usr/bypass/rc.local /etc/rc.d -f
