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
  int fd,ret=0,duty,i;
  
  if (!strcmp(argv[1],"100"))
			duty=0xff;
		if (!strcmp(argv[1],"90"))
			duty=0xed;
		if (!strcmp(argv[1],"80"))
			duty=0xdc;
		if (!strcmp(argv[1],"70"))
			duty=0xca;
		if (!strcmp(argv[1],"60"))
			duty=0xb9;
		if (!strcmp(argv[1],"50"))
			duty=0xa7;
		if (!strcmp(argv[1],"40"))
			duty=0x96;
		if (!strcmp(argv[1],"30"))
			duty=0x84;
		if (!strcmp(argv[1],"20"))
			duty=0x73;
    if (!strcmp(argv[1],"10"))
			duty=0x61;
		
		//printf("%d,",strtol(argv[1],NULL,10));
		if(!(strtol(argv[1],NULL,10)==100||strtol(argv[1],NULL,10)==90||strtol(argv[1],NULL,10)==80
			||strtol(argv[1],NULL,10)==70||strtol(argv[1],NULL,10)==60||strtol(argv[1],NULL,10)==50
			||strtol(argv[1],NULL,10)==40||strtol(argv[1],NULL,10)==30||strtol(argv[1],NULL,10)==20||strtol(argv[1],NULL,10)==10))
		{
			puts("Invalid parameter! please input a percentage number(10 -- 100 step 10)!");
			exit(0);
		}
				
  fd = open("//dev//hwm", O_RDWR, S_IRUSR | S_IWUSR);
  if (fd != -1 )
	{ 
		
	//bank2
		ioctl(fd, HWM_I0C_SET, 0x00000002);

	  	
  	ioctl(fd, HWM_I0C_SET, 0x02000000);
		ioctl(fd, HWM_I0C_SET, 0x03000000);
		ioctl(fd, HWM_I0C_SET, 0x04000000);
		ioctl(fd, HWM_I0C_SET, 0x05000000);
		ioctl(fd, HWM_I0C_SET, 0x06000000);
		ioctl(fd, HWM_I0C_SET, 0x07000000);
	  
	  printf("fan model: manual!\n");	
		
		ioctl(fd, HWM_I0C_SET, 0x10000000+duty);
		ioctl(fd, HWM_I0C_SET, 0x11000000+duty);
		ioctl(fd, HWM_I0C_SET, 0x12000000+duty);
		ioctl(fd, HWM_I0C_SET, 0x13000000+duty);
		ioctl(fd, HWM_I0C_SET, 0x14000000+duty);
		ioctl(fd, HWM_I0C_SET, 0x15000000+duty);
		ioctl(fd, HWM_I0C_SET, 0x16000000+duty);
		ioctl(fd, HWM_I0C_SET, 0x17000000+duty);
	
	 sleep(5);
	  ioctl(fd, HWM_I0C_SET, 0x00000000);
   
	  for(i=0;i<8;i++)
	  {
	  	ret=ioctl(fd, HWM_I0C_QUERY, 0x2e + i);
	  	
	  	ret<<=4;
	  	ret+=(ioctl(fd, HWM_I0C_QUERY, 0x3c)&0xf0)>>4;
	    printf("fan%d speed is:%dRPM\n",i+1,1350000/ret);
	  }
		sleep(5);
		ioctl(fd, HWM_I0C_SET, 0x00000002);

		ioctl(fd, HWM_I0C_SET, 0x0200001f);
		ioctl(fd, HWM_I0C_SET, 0x0300001f);
		ioctl(fd, HWM_I0C_SET, 0x04000000);
		ioctl(fd, HWM_I0C_SET, 0x05000060);
		ioctl(fd, HWM_I0C_SET, 0x06000000);
		ioctl(fd, HWM_I0C_SET, 0x07000000);
		printf("fan model : smart!\n");
		
		}
  else 
    printf("Device open failure\n");
   
  return 0;
}

