#include<sys/types.h>
#include<sys/stat.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<linux/ioctl.h>
#include <stdlib.h>

#define HWM_IOC_MAGIC 'H'
#define HWM_I0C_QUERY _IOWR(HWM_IOC_MAGIC,0,int)

int main(int argc,char **argv)
{
  int fd,ret=0,fannum;
  
  fannum=strtol(argv[1],NULL,10);
  
  fd = open("//dev//hwm", O_RDWR, S_IRUSR | S_IWUSR);
  if (fd != -1 )
	{
	   ret=ioctl(fd, HWM_I0C_QUERY, 0x90+(fannum-1)*2);
	   ret<<=8;
	   ret|=ioctl(fd, HWM_I0C_QUERY, 0x91+(fannum-1)*2);
	   ret&=0xfff;
	   printf("%d,%d \n",ret,1350000/ret);
	   
   }
  else 
    printf("Device open failure\n");
   
  return 0;
}

