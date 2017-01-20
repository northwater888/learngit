#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include<stdio.h>
#include <stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<linux/ioctl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>

#define ULONG unsigned long
#define UCHAR unsigned char
#define VER_STR 01
#define OPTS_SIZE 20

#define BYPASS_IOC_MAGIC 'T'
#define IOCTL_BYPASS_GET_DATA _IOWR(BYPASS_IOC_MAGIC,0,int)
#define IOCTL_BYPASS_SET_DATA _IOWR(BYPASS_IOC_MAGIC,1,int)
#define IOCTL_BYPASS_FORCE_NORMAL _IOWR(BYPASS_IOC_MAGIC,2,int)
#define IOCTL_BYPASS_FORCE_DISCONNECT _IOWR(BYPASS_IOC_MAGIC,3,int)
#define IOCTL_BYPASS_FORCE_BYPASS _IOWR(BYPASS_IOC_MAGIC,4,int)
#define IOCTL_BYPASS_SET_NORMAL _IOWR(BYPASS_IOC_MAGIC,5,int)
#define IOCTL_BYPASS_SET_SIGNAL _IOWR(BYPASS_IOC_MAGIC,6,int)
#define IOCTL_BYPASS_POWEROFF_SET _IOWR(BYPASS_IOC_MAGIC,7,int)
#define IOCTL_BYPASS_GET_CLIENT_ADDRESS _IOWR(BYPASS_IOC_MAGIC,9,int)

typedef struct ALL_DATA
{
	ULONG Scanflag;

	ULONG uid;
	ULONG vid;
	UCHAR base;
	ULONG offset;
	UCHAR Timeflag;
	UCHAR ByPass_Mode;
	UCHAR ByPass_Enable;
	UCHAR VERSIONvalue;
	UCHAR Time_outvalue;
	UCHAR WD_Control_Statusvalue;
	UCHAR PTC_Control_Statusvalue;
	UCHAR driverload_status;
	UCHAR poweron_status;
	UCHAR s5_status;
	UCHAR Register;
	UCHAR force_status;
	UCHAR addr;
	UCHAR HardWVersion[10];
} ALL_SensorData;

void threadfun(void* sas)
{
  FILE *fp;
  int i,ret,fd;
  ALL_SensorData sensordata;
  ALL_SensorData bpstatus;
  int sa = *(int*)sas;
      
  char filename[100] = "//usr//bypass//bypass__",str[5];
  sprintf(filename,"%s%x",filename,sa*2);
  
//  puts(filename);
  if((fp=fopen(filename,"r"))==0)
  {
    puts("bypass config file open fail!!&&&&&&&\n");
    exit(-1);
  }
  bpstatus.addr = sa;
  
  fgets(str,5,fp);
  bpstatus.ByPass_Mode = strtol(str,NULL,10);
  fgets(str,5,fp);
  bpstatus.Time_outvalue = strtol(str,NULL,10);
  fgets(str,5,fp);
  bpstatus.driverload_status = strtol(str,NULL,10);
  fgets(str,5,fp);
  bpstatus.poweron_status = strtol(str,NULL,10);
  fgets(str,5,fp);
  bpstatus.s5_status = strtol(str,NULL,10);
  fclose(fp);
  
//  printf("bpstatus.addr:%x.ByPass_Mode:%d,time_out:%d,driverloadstatusis:%d,poweronstatusis:%d,s5statusis:%d\n",bpstatus.addr,bpstatus.ByPass_Mode,bpstatus.Time_outvalue ,bpstatus.driverload_status,bpstatus.poweron_status,bpstatus.s5_status );

  fd = open("//dev//bypass", O_RDWR, S_IRUSR | S_IWUSR);
  if (fd != -1 )
  {
    ret=ioctl(fd,IOCTL_BYPASS_SET_DATA,&bpstatus);
   // printf("bpstatus.addr:%x,retis:%d,driverload_statusis:%d,Time_outvalueis:%d\n",bpstatus.addr,ret,bpstatus.driverload_status,bpstatus.Time_outvalue);
    if(bpstatus.driverload_status==0)
      if(bpstatus.Time_outvalue!=0)
      {
        ioctl(fd,IOCTL_BYPASS_GET_DATA, &bpstatus); 
        //while(bpstatus.PTC_Control_Statusvalue>>5%2&&bpstatus.Time_outvalue&&((bpstatus.WD_Control_Statusvalue>>4)%2==0))
        while(bpstatus.PTC_Control_Statusvalue>>5%2&&bpstatus.Time_outvalue)
        {
          sleep(bpstatus.Time_outvalue/3);
          ioctl(fd,IOCTL_BYPASS_SET_SIGNAL,&bpstatus); 
          ioctl(fd,IOCTL_BYPASS_GET_DATA, &bpstatus); 
          //printf("1is:%x,2is:%x\n",bpstatus.PTC_Control_Statusvalue,bpstatus.Time_outvalue);
        }  
        puts("exit while");
      }
  }
  else
  {
    puts("device open fail!!\n");
  }
  close(fd);
}

void poweroffset()
{
  int fd;
  fd = open("//dev//bypass", O_RDWR, S_IRUSR | S_IWUSR);
  //puts("appoweroff!!");
  if (fd != -1 )
    ioctl(fd,IOCTL_BYPASS_POWEROFF_SET,NULL); 
  else
    puts("device open fail!!\n");
  close(fd);
  
}

int main()
{
    int fd,sa[8]={0},ret,i=0,j=0,cardnum;
    pthread_t id;
    
    fd = open("//dev//bypass", O_RDWR, S_IRUSR | S_IWUSR);
    if (fd != -1 )
    {
      ret=ioctl(fd,IOCTL_BYPASS_GET_CLIENT_ADDRESS, sa);
      for(i=0;i<8;i++)
      if(sa[i]==0)
        break;
      cardnum=i;
    }
    else
    {
      puts("device open fail!!\n");
    }
    close(fd);
    
    struct sigaction act;
    act.sa_handler = poweroffset;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGTERM, &act, 0);

    for(i=0;i<cardnum;i++)
    {
      ret=pthread_create(&id,NULL,(void *)threadfun,(void*)&sa[i]);
      
    }
    while(1);
}



