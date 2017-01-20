#include<sys/types.h>
#include<sys/stat.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<linux/ioctl.h>
#include <stdlib.h>

#define GPIO_IOC_MAGIC 'L'
#define GPIO_IOC_QUERY _IOWR(GPIO_IOC_MAGIC,0,int)
#define GPIO_I0C_SET _IOWR(GPIO_IOC_MAGIC,1,int)

int main(int argc,char **argv)
{
  int aa=0,add,data,fd,ret;
 if(argc<3)
    {
    	printf("input error!\n");
    	printf("   ./gpio34 [address] [data]\n");
      printf("eg:\n");
      printf("   ./gpio34 0x538 0x0\n");
      return -1;        
     }
    add=strtol(argv[1],NULL,16);
    data=strtol(argv[2],NULL,16);
    
  fd = open("//dev//gpio34", O_RDWR, S_IRUSR | S_IWUSR);
  if (fd != -1 )
	{
	  /* 1 read register */
	  ret = ioctl(fd, GPIO_IOC_QUERY, 0);
	  printf(" read 0x538 is:%x\n",ret);
	  
	  ret=ioctl(fd,GPIO_I0C_SET,data);
	  //printf(" ret is:%x\n",ret);
	  
	  ret = ioctl(fd, GPIO_IOC_QUERY, 0);
	  printf(" write to 0x538 is:%x\n",ret);
		
	    close(fd);
	    
  }
  else 
    printf("Device open failure\n");
   
  return 0;
}
