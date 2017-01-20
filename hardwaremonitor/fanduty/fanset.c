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
  int fd,ret=0,fannum,duty,i;
  

  fd = open("//dev//hwm", O_RDWR, S_IRUSR | S_IWUSR);
  if (fd != -1 )
	{ 
		
	  
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
			duty=0xf;

		//printf("%d,",strtol(argv[1],NULL,10));
		if(!(strtol(argv[1],NULL,10)==100||strtol(argv[1],NULL,10)==90||strtol(argv[1],NULL,10)==80
			||strtol(argv[1],NULL,10)==70||strtol(argv[1],NULL,10)==60||strtol(argv[1],NULL,10)==50
			||strtol(argv[1],NULL,10)==40||strtol(argv[1],NULL,10)==30||strtol(argv[1],NULL,10)==20))
		{
			puts("Invalid parameter! please input a percentage number(20 -- 100 step 10)!");
			exit(0);
		}
				
		//bank2
		ioctl(fd, HWM_I0C_SET, 0x00000002);
		//set register 0x02 to 0x0
	  ioctl(fd, HWM_I0C_SET, 0x02000000);

	  printf("fan model: manual!\n");	
			
		//bank0
		ioctl(fd, HWM_I0C_SET, 0x00000000);
	  
	  ioctl(fd, HWM_I0C_SET, 0xb3000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb4000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb5000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb6000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb7000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb8000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xb9000000+duty);
	  ioctl(fd, HWM_I0C_SET, 0xba000000+duty);
	  
	  sleep(3);
	  for(i=0;i<8;i++)
	  {
	  	ret=ioctl(fd, HWM_I0C_QUERY, 0x23+i*2);
	  	ret<<=8;
	    ret|=ioctl(fd, HWM_I0C_QUERY, 0x23+i*2+1);
	    ret&=0xfff;
	    printf("fan%d speed is:%dRPM\n",i+1,1350000/ret);
	    //printf("  %dRPM  ",1350000/ret);
	  }
	  sleep(15);
	  
	  //bank2
	  ioctl(fd, HWM_I0C_SET, 0x00000002);
	  //set register 0x02 to 0x38 
	  ioctl(fd, HWM_I0C_SET, 0x02000038);
	  printf("fan model : smart!\n");
		
		}
  else 
    printf("Device open failure\n");
   
  return 0;
}

