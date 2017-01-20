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
  int fd,ret=0,fannum,duty;

  fd = open("//dev//hwm", O_RDWR, S_IRUSR | S_IWUSR);
  if (fd != -1 )
	{ 
		ioctl(fd, HWM_I0C_SET, 0x00000002);
	  ioctl(fd, HWM_I0C_SET, 0x02000000);
	  printf("set fan control to manual fan!\n");
	  
		if (!strcmp(argv[1],"100"))
			duty=0x3f;
		if (!strcmp(argv[1],"90"))
			duty=0x39;
		if (!strcmp(argv[1],"80"))
			duty=0x32;
		if (!strcmp(argv[1],"70"))
			duty=0x2c;
		if (!strcmp(argv[1],"60"))
			duty=0x26;
		if (!strcmp(argv[1],"50"))
			duty=0x20;
		if (!strcmp(argv[1],"40"))
			duty=0x1a;
		if (!strcmp(argv[1],"30"))
			duty=0x14;
		if (!strcmp(argv[1],"20"))
			duty=0xe;
		if (!strcmp(argv[1],"10"))
			duty=0x7;
		if (!strcmp(argv[1],"0"))
			duty=0x0;
		
		ioctl(fd, HWM_I0C_SET, 0x00000000);
	  ioctl(fd, HWM_I0C_SET, 0xb3000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb4000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb5000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb6000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb7000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb8000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb9000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xba000000+duty);
	  
	  sleep(20);
	    
		ioctl(fd, HWM_I0C_SET, 0x00000002);
	  ioctl(fd, HWM_I0C_SET, 0x02000038);
	  printf("set fan control to smart fan!\n");
		
//		ret=ioctl(fd, HWM_I0C_QUERY, 0xb3);
//		printf("        %x\n ",ret); 
//		ret=ioctl(fd, HWM_I0C_QUERY, 0xb4);
//		printf("        %x\n ",ret); 
//		ret=ioctl(fd, HWM_I0C_QUERY, 0xb5);
//		printf("        %x \n",ret); 
//		ret=ioctl(fd, HWM_I0C_QUERY, 0xb6);
//		printf("        %x\n ",ret); 
//		ret=ioctl(fd, HWM_I0C_QUERY, 0xb7);
//		printf("        %x\n ",ret); 
//		ret=ioctl(fd, HWM_I0C_QUERY, 0xb8);
//		printf("        %x\n ",ret); 
//		ret=ioctl(fd, HWM_I0C_QUERY, 0xb9);
//		printf("        %x\n ",ret); 
//		ret=ioctl(fd, HWM_I0C_QUERY, 0xba);
//		printf("        %x\n ",ret); 
//		
//		ret=ioctl(fd, HWM_I0C_QUERY, 0x5e);
//		printf(" 5eis: %x \n",ret);
		}
  else 
    printf("Device open failure\n");
   
  return 0;
}

