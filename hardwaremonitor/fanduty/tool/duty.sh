echo "Please input fan duty between 10 to 100 step 10:"
read number
case "$number" in
10)
  duty=0x60;;
20)
  duty=0x73;;
30)
  duty=0x84;;
40)
  duty=0x96;;
50)
  duty=0xa7;;
60)
  duty=0xb9;;
70)
  duty=0xca;;
80)
  duty=0xdc;;
90)
  duty=0xed;;
100)
  duty=0xff;;
esac

ipmitool raw 0x3E 0x86 0x80 0xff   >NULL  
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x0 0x02 >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x02 0x0 >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x03 0x0 >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x04 0x0 >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x05 0x0 >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x06 0x0 >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x07 0x0 >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x10 $duty >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x11 $duty >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x12 $duty >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x13 $duty >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x14 $duty >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x15 $duty >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x16 $duty >NULL
ipmitool raw  0x06 0x52 0x0b 0x58 0x0 0x17 $duty >NULL
ipmitool raw 0x3E 0x86 0x80 0x0      >NULL
echo "please check fan speed! wait 40 seconds!"
sleep 40
exit 0
