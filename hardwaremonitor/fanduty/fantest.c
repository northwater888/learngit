#include<sys/types.h>
#include<sys/stat.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<linux/ioctl.h>
#include <stdlib.h>

#define HWM_IOC_MAGIC 'H'
#define HWM_I0C_QUERY _IOWR(HWM_IOC_MAGIC,0,int)
#define HWM_I0C_SET _IOWR(HWM_IOC_MAGIC,1,int)

int main(int argc,char **argv)
{
  int fd,ret=0,fannum;

  fd = open("//dev//hwm", O_RDWR, S_IRUSR | S_IWUSR);
  if (fd != -1 )
	{ ioctl(fd, HWM_I0C_SET, 0x00000002);
	  ioctl(fd, HWM_I0C_SET, 0x02000038);
	  printf("set fan control to smart fan!\n");
	    
		ret=ioctl(fd, HWM_I0C_QUERY, 0x0);
		printf("bank2 0:%x\n ",ret);
		ret=ioctl(fd, HWM_I0C_QUERY, 0x1);
		printf("bank2 01:%x\n ",ret);
		ret=ioctl(fd, HWM_I0C_QUERY, 0x2);
		printf("bank2 02:%x\n ",ret);
		ret=ioctl(fd, HWM_I0C_QUERY, 0x3);
		printf("bank2 03:%x\n ",ret);
		ret=ioctl(fd, HWM_I0C_QUERY, 0x4);
		printf("bank2 04:%x\n ",ret);
		ret=ioctl(fd, HWM_I0C_QUERY, 0x5);
		printf("bank2 05:%x\n ",ret);
		ret=ioctl(fd, HWM_I0C_QUERY, 0x6);
		printf("bank2 06:%x\n ",ret);
				
		}
  else 
    printf("Device open failure\n");
   
  return 0;
}

