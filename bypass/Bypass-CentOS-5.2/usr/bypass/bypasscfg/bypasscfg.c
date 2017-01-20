/******************************************************************
** Filename			: bypasscfg.c
** Copyright (c)    : MiTAC 2009
** Description:		: Command line ethernet card bypass mode
					  configuration program
** Author			: Danny.xie
** Date				: 2009.9.4
** Version:			: 0.1
** Modifier			: 
** Date				: 
** Description      : 
******************************************************************/
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include<fcntl.h>
#include<unistd.h>
#include<linux/ioctl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <getopt.h>

#define ULONG unsigned long
#define UCHAR unsigned char
#define VER_STR 01
#define OPTS_SIZE 20
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

ALL_SensorData bpstatus;

int optnum=0;

#define BYPASS_IOC_MAGIC 'T'
#define IOCTL_BYPASS_GET_DATA _IOWR(BYPASS_IOC_MAGIC,0,int)
#define IOCTL_BYPASS_SET_DATA _IOWR(BYPASS_IOC_MAGIC,1,int)
#define IOCTL_BYPASS_FORCE_NORMAL _IOWR(BYPASS_IOC_MAGIC,2,int)
#define IOCTL_BYPASS_FORCE_DISCONNECT _IOWR(BYPASS_IOC_MAGIC,3,int)
#define IOCTL_BYPASS_FORCE_BYPASS _IOWR(BYPASS_IOC_MAGIC,4,int)
#define IOCTL_BYPASS_SET_NORMAL _IOWR(BYPASS_IOC_MAGIC,5,int)
#define IOCTL_BYPASS_SET_SIGNAL _IOWR(BYPASS_IOC_MAGIC,6,int)
#define IOCTL_BYPASS_GET_CLIENT_ADDRESS _IOWR(BYPASS_IOC_MAGIC,9,int)

void usage(void)
{
	fprintf(stderr, "  bpc version is: 1.00\n\n");
	fprintf(stderr, "  -d SMBUS_ADDR\n");
	fprintf(stderr, "          Specify the SMBus address of NIC card.\n");
	fprintf(stderr, "     -g \n");
	fprintf(stderr, "             Get NIC card current state.\n");
	fprintf(stderr, "     -f 0/1/2\n");
	fprintf(stderr, "             Force NIC card into:0 (normal) or 1(disconnect) or 2(bypass).\n");
	fprintf(stderr, "     -c \n");
	fprintf(stderr, "             Disable the bypass watchdog function of NIC card.\n");
	fprintf(stderr, "     -t timeoutvalue \n");
	fprintf(stderr, "             Set the watchdog timer timeout value(10<= timeoutvalue<=255).\n");
	fprintf(stderr, "     -p 0/1/2\n");
	fprintf(stderr, "             Set the state of NIC card when system power on:0 (normal) or 1(disconnect) or 2(bypass).\n");
	fprintf(stderr, "     -q 0/1/2\n");
	fprintf(stderr, "             Set the state of NIC card in S5:0 (normal) or 1(disconnect) or 2(bypass).\n");
	fprintf(stderr, "     -s \n");
	fprintf(stderr, "             Save the configuration of NIC card.\n");
	fprintf(stderr, "     -v \n");
	fprintf(stderr, "             Get Product Version and Pass Thru FW Version information.\n");
	fprintf(stderr, "\n");
	exit(0);
}

int parse(int argc, char **argv)
{
  int c,out=0;
  int digit_optind = 0;
  FILE *fp;
	char filename[100]="bypass_",str[5],filenamebak[100];
		
  while (1)
  {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = 
    {
        {"clear", 1, 0, 'c'},
        {"salve address", 0, 0, 'd'},
        {"force", 1, 0, 'f'},
        {"g", 1, 0, 'g'},
        {"h" ,2, 0, 'h'},
        {"p", 0, 0, 'p'},
        {"q", 0, 0, 'q'},
        {"s", 0, 0, 's'},
        {"time", 1, 0, 't'},
        {"v", 1, 0, 'v'},
        {0, 0, 0, 0}
    };
    c = getopt_long (argc, argv, "cd:f:ghp:q:st:v",
             long_options, &option_index);
    if (c == -1)
        break;
    switch (c) {
    case 'c':
        if(bpstatus.addr==0)
        {
          puts("You input the wrong SMBus address");
          exit(0);
        }
        bpstatus.ByPass_Mode = 0;
        bpstatus.Time_outvalue = 0;
        out=1;
        break;
    case 'd':
        {         
        int add=0;
        add = strtol(optarg,NULL,16);
        if((add%2)||(add>0x4e)||(add<0x40))
        {
          printf("You input the wrong SMBus address\n");
          exit(0);
        } 
        }
        bpstatus.addr = strtol(optarg,NULL,16)/2;
        {
   
          strcpy(filenamebak,filename);
          sprintf(filename,"%s_%x",filename,bpstatus.addr*2);
          fp=fopen(filename,"r");
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
          strcpy(filename,filenamebak);
        }
        break;
    case 'f':
        bpstatus.force_status = strtol(optarg,NULL,10);
        bpstatus.driverload_status = strtol(optarg,NULL,10);
        if(bpstatus.driverload_status !=0)
        {  
          bpstatus.Time_outvalue = 0;
          bpstatus.ByPass_Mode = 0;
        }
        else
        {  
          bpstatus.ByPass_Mode = 1;
        }
        optnum = 1;
        out=  1;
        break;
    case 'g':
        optnum = 2;
        out=  0;
        break;
    case 'p':
        bpstatus.poweron_status = strtol(optarg,NULL,10);
        out=  1;
        break;
    case 'q':
        bpstatus.s5_status = strtol(optarg,NULL,10);
        out=  1;
        break;
    case 's':
        optnum = 3;
        break;
    case 't':
        {
          int t=0;
          t= strtol(optarg,NULL,10);
          if(t<10||t>255)
          {
            printf("Timeout value must be from 10 to 255!!\n");
            exit(0);
          }
        
          if(bpstatus.driverload_status==0)
          {
            bpstatus.Time_outvalue = t;
            bpstatus.ByPass_Mode = 1;
          }
          else
          {
            puts("When you set timeout value,Current state must be normal.");
            exit(0);
          }
        }
        out=  1;
        break;
    case 'v':
        optnum = 4;
        break;
 		case 'h':
		default:
			usage();
			break;
    }
  }
  
  sprintf(filename,"%s_%x",filename,bpstatus.addr*2);
  fp=fopen(filename,"w");
  if(out==1)
  {
  if(bpstatus.ByPass_Mode==0)
    printf("ByPass Mode is:Disable\n");
  else 
    printf("ByPass Mode is:Enable\n");
  
  printf("Timeout value is:%d\n", bpstatus.Time_outvalue);
  
  if(bpstatus.driverload_status==0)
    printf("Driver load status is:Normal\n");
  else if(bpstatus.driverload_status==1)
    printf("Driver load status is:Disconnect\n");
  else if(bpstatus.driverload_status==2)
    printf("Driver load status is:Bypass\n");
    
  if(bpstatus.poweron_status==0)
    printf("Power on status is:Normal\n");
  else if(bpstatus.poweron_status==1)
    printf("Power on status is:Disconnect\n");
  else if(bpstatus.poweron_status==2)
    printf("Power on status is:Bypass\n");
  
  if(bpstatus.s5_status==0)
    printf("S5 status is:Normal\n");
  else if(bpstatus.s5_status==1)
    printf("S5 status is:Disconnect\n");
  else if(bpstatus.s5_status==2)
    printf("S5 status is:Bypass\n");
  }  
  
  fprintf(fp,"%d\n%d\n%d\n%d\n%d\n",bpstatus.ByPass_Mode,
  bpstatus.Time_outvalue,bpstatus.driverload_status,bpstatus.poweron_status,bpstatus.s5_status);
  fclose(fp);   
}

int main(int argc,char **argv)
{
	int fd,ret=0,address=bpstatus.addr,sa[8],cardnum,i;
	ALL_SensorData sensordata;
	char pstatus[20];
	
	parse(argc,argv);
	system("/usr/bypass/kill.sh   > /dev/null 2>&1");
	memset((char *)&sensordata,0,sizeof(ALL_SensorData));
  sensordata.addr=bpstatus.addr;
  if(address==0)
    address = 0x26;
  if(sensordata.addr==0)
    sensordata.addr=0x26;
		    
	fd = open("//dev//bypass", O_RDWR, S_IRUSR | S_IWUSR);
	if (fd != -1 )
	{  
    ret=ioctl(fd,IOCTL_BYPASS_GET_CLIENT_ADDRESS, sa);
    for(i=0;i<8;i++)
    if(sa[i]==0)
      break;
    cardnum=i;
      
	  switch(optnum)
	  {
	    case 1:
	      for(i=0;i<cardnum;i++)
	        if(sa[i]==sensordata.addr)
	          break;
	      if(cardnum==i)
	      {
	        printf("You input the wrong SMBus address\n");
	        return -1;
	      }
		    if(bpstatus.force_status==0)
		    {
		      ret=ioctl(fd,IOCTL_BYPASS_FORCE_NORMAL,&sensordata);
		      printf("Force normal ok! NIC card status is:Normal\n");   
		      
		    }
		    else if(bpstatus.force_status==1)
		    {
		      ret=ioctl(fd,IOCTL_BYPASS_FORCE_DISCONNECT,&sensordata);
		      printf("Force disconnect ok! NIC card status is:Disconnect\n");   
		    }
		    else if(bpstatus.force_status==2)
		    {
		      ret=ioctl(fd,IOCTL_BYPASS_FORCE_BYPASS,&sensordata);
		      printf("Force bypass ok! NIC card status is:Bypass\n");   
		    }
		    break;
		  case 2:
		    for(i=0;i<cardnum;i++)
	        if(sa[i]==sensordata.addr)
	          break;
	      if(cardnum==i)
	      {
	        printf("You input the wrong SMBus address\n");
	        return -1;
	      }
		    ret=ioctl(fd,IOCTL_BYPASS_GET_DATA, &sensordata);
      	printf("NIC card number is:%d\nPass Thru SMBus Addr is:0x%x\nProduct Ver is:",cardnum,sensordata.addr*2);
      	for(i=0;i<6;i++)
        {
          printf("%c",sensordata.HardWVersion[i]);
        }
        printf("\n");
      	printf("Timeout value is:%d\n", sensordata.Time_outvalue);
      	if((sensordata.PTC_Control_Statusvalue&0x20)==0x20)
      	{
      	  strcpy(pstatus,"Normal");
      	  printf("Present status is:%s\n",pstatus);
      	  printf("State on driver load is:Normal\n"); 
      	}
      	if(((sensordata.PTC_Control_Statusvalue&0x20)==0)&&((sensordata.PTC_Control_Statusvalue&0x10)==0x10)&&((sensordata.PTC_Control_Statusvalue&0x8)==0))
      	{
      	  strcpy(pstatus,"Disconnect");
      	  printf("Present status is:%s\n",pstatus);
      	  printf("State on driver load is:Disconnect\n"); 
      	}
      	if(((sensordata.PTC_Control_Statusvalue&0x20)==0)&&((sensordata.PTC_Control_Statusvalue&0x10)==0)&&((sensordata.PTC_Control_Statusvalue&0x8)==0x8))
      	{
      	  strcpy(pstatus,"Bypass");
      	  printf("Present status is:%s\n",pstatus);
      	  printf("State on driver load is:Bypass\n"); 
      	}
      	
      	
      	if(sensordata.poweron_status==0)
      	  printf("State on power on is:Normal\n"); 
      	else if(sensordata.poweron_status==1)
      	  printf("State on power on is:Disconnect\n"); 
      	else if(sensordata.poweron_status==2)
      	  printf("State on power on is:Bypass\n"); 
      	
      	if(sensordata.s5_status==0)
      	  printf("State on S5 is:Normal\n"); 
      	else if(sensordata.s5_status==1)
      	  printf("State on S5 is:Disconnect\n"); 
      	else if(sensordata.s5_status==2)
      	  printf("State on S5 is:Bypass\n"); 
      	
//      	if(bpstatus.Timeflag==0)
//      	  printf("Time unit is:second\n"); 
      	break;
		  case 3:
	      puts("Save the configuration of specified NIC card.");
	      system("/usr/bypass/ctl  &");
		    break;
		  case 4:
		    for(i=0;i<cardnum;i++)
	        if(sa[i]==sensordata.addr)
	          break;
	      if(cardnum==i)
	      {
	        printf("You input the wrong SMBus address\n");
	        return -1;
	      }
		    ret=ioctl(fd,IOCTL_BYPASS_GET_DATA, &sensordata);
      	printf("Pass Thru FW Ver is:R0.0%x\n",sensordata.VERSIONvalue);
      	exit(0);
      default:
		    break;
		}
  }
	else
	{
	  puts("device open fail!!\n");
	}
  return 0;
}

