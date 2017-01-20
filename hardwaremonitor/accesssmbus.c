#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/io.h>

typedef unsigned char 		u8;
typedef unsigned int 		u16;
typedef unsigned long 		u32;
typedef unsigned long long 	u64;

static u32 smbus_base_addr = 0;

#define pciid  0x43851002

int smbussel,readoff,writeoff,wdata,data,opt,slaveadd;
#define PRTCL  (smbus_base_addr + 2)
#define STS    (smbus_base_addr + 0)
#define ADDR   (smbus_base_addr + 4)
#define CMD    (smbus_base_addr + 3)
#define DATA0  (smbus_base_addr + 5)

u32 pcibios_read_config_u32(u32 busnum, u32 devnum, u32 funnum, u32 regnum);
int probe_smbus (u32 pci_id);
int get_smbus_addr (void);
int read_data (void);

int main (int argc, char **argv)
{
  if(strcmp(argv[1],"--help")==0||strcmp(argv[1],"-h")==0)
  {
  	puts("atismbus slaveaddress smbussel readoff ");
    puts("atismbus slaveaddress smbussel writeoff data");
    exit(0);
  }
  else if(argc==4)
  {
  	opt=0;
  	slaveadd=strtol(argv[1],NULL,16);
  	if(strcmp(argv[2],"1")==0)
  	smbussel=1;
  	else if(strcmp(argv[2],"2")==0)
  	smbussel=0x41;
  	if(strcmp(argv[2],"3")==0)
  	smbussel=0x81;
  	if(strcmp(argv[2],"0")==0)
  	smbussel=1;
  	readoff=strtol(argv[3],NULL,16);
  }
  else 
  {
  	opt=1;
  	slaveadd=strtol(argv[1],NULL,16);
  	if(strcmp(argv[2],"1")==0)
  	smbussel=1;
  	else if(strcmp(argv[2],"2")==0)
  	smbussel=0x41;
  	if(strcmp(argv[2],"3")==0)
  	smbussel=0x81;
  	if(strcmp(argv[2],"0")==0)
  	smbussel=1;
  	writeoff=strtol(argv[3],NULL,16);
  	wdata=strtol(argv[4],NULL,16);
  }
  
  iopl(3);
  if (get_smbus_addr() < 0)
  {
		iopl(0);
		return 1;
  }
  if(opt==0)
  {
  	 printf(" slaveadd:%x  smbussel:%x readoff:%x  \n",slaveadd,smbussel,readoff);
  	 read_data();
  }	 
  else 
  	{
  		printf(" slaveadd:%x  smbussel:%x writeoff:%x data:%x \n",slaveadd,smbussel,writeoff,wdata);
  		writedata();
  		read_Ati_slave_data(writeoff);
  	}
  iopl(0);
}

u32 pcibios_read_config_u32(u32 busnum, u32 devnum, u32 funnum, u32 regnum)
{
	u32 value=-1;
	u32 config;

	config = (0x80000000|(busnum<<16)|(devnum<<11)|(funnum<<8)) + regnum;
	outl(config, 0x0CF8);
	value = inl(0x0CFC);
	if(value == 0xFFFFFFFF)
		value = -1;
  return value;
}

int probe_smbus (u32 pci_id)
{
	int  ret = 1;
	if ( pciid == pci_id)
	  ret = 0;
	return ret;
}

int get_smbus_addr (void)
{
  u32 pci_bus = 0, pci_dev = 0, pci_fn = 0, pci_id = 0,data;

  for(pci_bus=0; pci_bus<256; pci_bus++)
		for(pci_dev=0; pci_dev<32; pci_dev++)
    	for(pci_fn=0; pci_fn<8; pci_fn++)
	    {
				pci_id = pcibios_read_config_u32 (pci_bus, pci_dev, pci_fn, 0);
				if (pci_id == 0xffffffff)
				    continue;
				if (probe_smbus(pci_id) != 0)
				    continue;
				else
				{
				  //  printf("%x %x %x %x\n", pci_bus, pci_dev, pci_fn, smbus_base_offset);
		  	  smbus_base_addr = pcibios_read_config_u32 (pci_bus, pci_dev, pci_fn, 0x90);
			    smbus_base_addr &= 0xfffffffc;
			    printf("SMBus base address %8.8x\n", smbus_base_addr);
			  
			    return 0;
				}
	    }
  return 1;
}



void write_Ati_slave_data(int offset,int value)
{

    int addr;//protocol,
    int hostbusy;
    printf("write offset is :%x,data is:%x\n ",offset,value);
    outb(0x1f,STS);//clear  status
    outb(offset,CMD);//; Set target I2C CMD - offset for which vaules do you need to read and write.
    outb(value,DATA0);//write vaule
    usleep(100);
    outb(slaveadd,ADDR); //Set target I2C address  bit 0 =1 (read:1b write:0b)
    outb(0x48,PRTCL);//init controller bit6=1,bit 4~2(010=byte read & write)

    usleep(100);
    do
    {
          usleep(10);
           hostbusy = inb(STS);
    }
    while((hostbusy & 0x01) !=0);
}

int read_Ati_slave_data( int offset)
{   
    int hostbusy,tmp;//James002
    int timeout = 0;
    outb(0x1f,STS);//clear  status
    do
    {
      usleep(10);
      hostbusy = inb(STS);
    }
    while ((hostbusy & 0x01) !=0);        
    
    outb(offset,CMD);//; Set target I2C CMD - offset for which vaules do you need to read and write.
    outb(slaveadd+1,ADDR);//Set target I2C address  bit 0 =1 (read:1b write:0b)
    outb(0x48,PRTCL);//init controller bit6=1,bit 4~2(010=byte read & write)
    do
    {
      usleep(10);
      hostbusy = inb(STS);
    }
    while((hostbusy & 0x01) !=0);

    tmp = inb(DATA0);
    printf("read offset is :%x,data is:%x\n ",offset,tmp);
    return tmp;       
}
 

int read_data (void)
{
	int i,data;

  outl (0x8000a0D0 ,	0x0cf8);
	data = inb (0x0cfc + 0x2);
	printf("D2 datais:%x\n",data);
	  
	outl (0x8000a0D0 ,	0x0cf8);
	outb(smbussel,0x0cfc+2);
	 
	outl (0x8000a0D0 ,	0x0cf8);
	data = inb (0x0cfc + 0x2);
	//printf("D2 datais:%x\n",data);
	
	read_Ati_slave_data(readoff);
}

int writedata()
{
	outl (0x8000a0D0 ,	0x0cf8);
	data = inb (0x0cfc + 0x2);
	printf("D2 datais:%x\n",data);
	  
	outl (0x8000a0D0 ,	0x0cf8);
	outb(smbussel,0x0cfc+2);
	 
	outl (0x8000a0D0 ,	0x0cf8);
	data = inb (0x0cfc + 0x2);
	write_Ati_slave_data(writeoff,wdata);
	//printf("D2 datais:%x\n",data);
}
