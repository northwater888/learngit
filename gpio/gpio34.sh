setpci -s 00:1f.0 4c.b=10
echo "pci: bus 0 device 1f function 0 register 4c is:"
lspci -s 00:1f.0 -xxx|awk '{ if(NR==6)print }'|awk '{ print $14  }'
./gpio34 538 0

usleep 100000

./gpio34 538 1

