gcc -o gpio34 gpio34_set.c
make
insmod gpio34.ko 
major=`dmesg|grep zzzzzzm|awk {'print $2'}`
#minor=`dmesg|grep zzzzzzn|awk {'print $2'}`
mknod /dev/gpio34 c "$major" 0 

