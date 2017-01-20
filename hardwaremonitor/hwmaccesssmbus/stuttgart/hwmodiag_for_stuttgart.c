//***********************************************************
//*    This utility is to test Stuttgart Hardware Monitor   *
//***********************************************************
//* Create date: 2006/7/17
//* Auther: MiTAC Shanghai ESBU/SW Yuanjie Xu
//*
//* Chipset:   NVDIA MCP55
//* HWmonitor: DME1737, EMC6D103
//******************************************************
//* Rev.   Date        By            Description       *
//******************************************************
//* 0.1   2006/7/17   Yuanjie Xu   Initial version
//* 0.2   2006/11/6   Victor.zhou  Add two DIMM fan check

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <../lh/sys/mman.h>
#include <../lh/sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <../lhsys/io.h>
#include <syslog.h>
#include "utils.h"
#include "lib/pci.h"



/*typedef unsigned char 		u8;
typedef unsigned int 		u16;
typedef unsigned long 		u32;
typedef unsigned long long 	u64;*/

char *VER_STR="hwmodiag for Stuttgart, Version 0.1 (c) 2006, MiTAC Inc.";

static u32 smbus_base_addr = 0;
static u32 smbus_base_offset = 0;
static u32 smbus_value_bit = 0;

struct smbus_controller
{
	unsigned long pci_id;
	unsigned long base_offset;
	unsigned long value_bit;
};

static struct smbus_controller smbus_controller_list[] = 
{
	{ 0x25a48086, 0x20, 0xffe0}, //Hance Rapids ICH SMBus Controller
	{ 0x24838086, 0x20, 0xffe0}, //ICH3 SMBus Controller
	{ 0x005210de, 0x24, 0xffff}, //CK804 SMBus Controller, NVDIA
	{ 0x036810de, 0x24, 0xffff}, //MCP55 SMBus Controller, NVDIA
	{ 0, 0, 0},
};

struct hwmo_chip
{
	int device_id;
	int company_id;
};

struct hwmo_chip chip_id[] = 
{
	{0,0},
	{0,0},
	{0,0}
};

/*Hardware Monitor Define*/
#define slaveadd	0x2D //EMC
#define slaveadd1	0x2E //DME

/*NVDIA MCP55 SMBus address offsets */
#define PRTCL  (smbus_base_addr + 0)
#define STS    (smbus_base_addr + 1)
#define ADDR   (smbus_base_addr + 2)
#define CMD    (smbus_base_addr + 3)
#define DATA0  (smbus_base_addr + 4)

/*SMBUS Retry time*/
#define Retry 10

struct hwm_item
{
	char name[30];
	u8 slave_addr;
	u8 addr;
	u8 addr_offset;
	union
	{
	    int	ival;
	    double fval;
	} get_val, get_min, get_max;
	double p;
	int min;
	int max;
	float stand_value;
	char start[5];
};

//vol
#define VTR5    0x20
#define Vccp    0x21
#define VCC     0x22
#define V5	0x23
#define V12	0x24
#define VID	0x43
#define VTR     0x99
#define Vbat    0x9A
//fan
#define FANTACH1_L 	0x28
#define FANTACH1_H 	0x29
#define FANTACH2_L 	0x2A
#define FANTACH2_H 	0x2B
#define FANTACH3_L 	0x2C
#define FANTACH3_H	0x2D
#define FANTACH4_L 	0x2E
#define FANTACH4_H 	0x2F
#define FANTACHA_L 	0xA9
#define FANTACHA_H 	0xAA
#define FANTACHB_L 	0xAB
#define FANTACHB_H 	0xAC
//temperature
#define R1_Temp		0x25
#define L_Temp		0x26
#define R2_Temp		0x27
#define R1_OFFSET	0x1F
#define L_OFFSET	0x1D
#define R2_OFFSET	0x1E


static struct hwm_item tests[] =
{
	{"emc_2.5", slaveadd, VTR5, 0, {0.0}, {0.0}, {0.0}, 3.32, 95, 105, 2.47, "YES"},
	{"emc_vccp", slaveadd, Vccp, 0, {0.0}, {0.0}, {0.0}, 3.0, 95, 105, 0.90, "YES"},
	{"dme_cpu0_vid_1.3", slaveadd1, VID, 0, {0.0}, {0.0}, {0.0}, 0.025, 98, 102, 1.3, "YES"},
	{"emc_cpu1_vid_1.3", slaveadd, VID, 0, {0.0}, {0.0}, {0.0}, 0.025, 98, 102, 1.3, "YES"},
	{"emc_vcc_3.3", slaveadd, VCC, 0, {0.0}, {0.0}, {0.0}, 4.38, 95, 105, 3.3, "YES"},
	{"emc_12", slaveadd, V12, 0, {0.0}, {0.0}, {0.0}, 15.93, 95, 105, 11.93, "YES"},
	{"dme_sio_vtr_5", slaveadd1, VTR5, 0, {0.0}, {0.0}, {0.0}, 6.64, 95, 105, 4.96, "YES"},
	{"dme_sio_vccp", slaveadd1, Vccp, 0, {0.0}, {0.0}, {0.0}, 3.0, 95, 105, 0.9, "YES"},
	{"dme_sio_vcc_3.3", slaveadd1, VCC, 0, {0.0}, {0.0}, {0.0}, 4.38, 95, 105, 3.3, "YES"},
	{"dme_sio_5", slaveadd1, V5, 0, {0.0}, {0.0}, {0.0}, 6.64, 95, 105, 5.0, "YES"},
	{"dme_sio_12", slaveadd1, V12, 0, {0.0}, {0.0}, {0.0}, 16.0, 95, 105, 11.93, "YES"},
	{"dme_sio_vtr_3.3", slaveadd1, VTR, 0, {0.0}, {0.0}, {0.0}, 4.38, 95, 105, 3.3, "YES"},
	{"dme_vbat_3.0", slaveadd1, Vbat, 0, {0.0}, {0.0}, {0.0}, 4.38, 95, 105, 3.0, "YES"},
	{"emc_cpu0_fan", slaveadd, FANTACH1_L, 0, {0.0}, {0.0}, {0.0}, 1.0, 4000, 8000, 1.0, "YES"},
	{"emc_cpu1_fan", slaveadd, FANTACH2_L, 0, {0.0}, {0.0}, {0.0}, 1.0, 4000, 8000, 1.0, "YES"},
	{"dme_sys_fan1", slaveadd1, FANTACH1_L, 0, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0, "YES"},
	{"dme_sys_fan2", slaveadd1, FANTACH2_L, 0, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0, "YES"},
	{"dme_sys_fan3", slaveadd1, FANTACH4_L, 0, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0, "YES"},
	{"dme_dimm_fan0", slaveadd1, FANTACHA_L, 0, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0, "YES"},
	{"dme_dimm_fan1", slaveadd1, FANTACHB_L, 0, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0, "YES"},
	{"emc_cpu0_temp", slaveadd, R1_Temp, R1_OFFSET, {0.0}, {0.0}, {0.0}, 1.0, 20, 60, 1.0, "YES"},
	{"emc_cpu1_temp", slaveadd, R2_Temp, R2_OFFSET, {0.0}, {0.0}, {0.0}, 1.0, 20, 60, 1.0, "YES"},
	{"dme_sys_temp1", slaveadd1, R1_Temp, R1_OFFSET, {0.0}, {0.0}, {0.0}, 1.0, 10, 60, 1.0, "YES"},
	{"dme_sys_temp2", slaveadd1, R2_Temp, R2_OFFSET, {0.0}, {0.0}, {0.0}, 1.0, 10, 60, 1.0, "YES"},
	{"dme_sys_temp3", slaveadd1, L_Temp, L_OFFSET, {0.0}, {0.0}, {0.0}, 1.0, 10, 60, 1.0, "YES"},
	{"emc_sys_temp4", slaveadd, L_Temp, L_OFFSET, {0.0}, {0.0}, {0.0}, 1.0, 10, 60, 1.0, "YES"},
	{"", 0, 0, 0, {0}, {0}, {0}, 0.0, 0, 0, 0.0, ""}
};

/*Function Define*/
//void outb_p(int value, int addr);
//unsigned int inb_p(int addr);
//#define inp(p) inportb(p)
//#define outp(v,p) outportb(v,p)
u32 pcibios_read_config_u32(u32 busnum, u32 devnum, u32 funnum, u32 regnum);
int probe_smbus (u32 pci_id);
int get_smbus_addr (void);
inline int i2c_read_u8(int sa, u8 offset);
void i2c_write_u8(int sa, u8 offset, u8 data);
int read_chip_id (void);
void hwmtest_start(void);
u32 ReadThermState(int cpunode);

int i;
int test_hwm(void)
{
    int type = 0;
    int temp = 0;

    temp = i2c_read_u8 (tests[i].slave_addr, tests[i].addr);
//    printf("temp: %d\n", temp);
    if((temp == 0xff) && (i<13))
    {
	tprintf_err("%-25s :  Function Error", tests[i].name);
	tprintf_out("\n");
	return 1;
    }

    type = i;

//vol
    if (type>=0 && type<13)
    {
	if(type>1 && type<4)
		tests[i].get_val.fval = 1.550 - (temp & 0x1F)*0.025;
	else
		tests[i].get_val.fval = temp*(tests[i].p/255.0);
	tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
	tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;

	if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
	{
	    tprintf_err("%s Error: %4.3f V, min %4.3f V, max %4.3f V",
		tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
	    tprintf_out("\n");	
	    return 1;
	}
	tprintf_out("%-25s :  %4.3f V\n", tests[i].name, tests[i].get_val.fval);
    }

//fan
    else if(type>=13 && type<20)
    {
	int l = 0, m = 0, speed = 0;
	l = temp;
	m = i2c_read_u8 (tests[i].slave_addr, (tests[i].addr)+1);
	if((l == 0xff) && (m == 0xff))
	{
	    tprintf_err("%-25s :  Function Error", tests[i].name);
	    tprintf_out("\n");
	    return 1;
	}
	speed = (m << 8) + l;
	tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
	if ((tests[i].get_val.fval) < (tests[i].min) || (tests[i].get_val.fval) > (tests[i].max))
	{
	    tprintf_err("%s speed Error: %d RPM,  min %d RPM,  max %d RPM",
		tests[i].name, (int)(tests[i].get_val.fval), tests[i].min, tests[i].max);
	    tprintf_out("\n");
	    return 1;
	}
	tprintf_out("%-25s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
    }

//temperature
    else
    {
	int temp1=0;
	if (temp == 0x80)
        {
            tprintf_out("%-25s :  No Function\n", tests[i].name);
            return 0;
        }
	temp1 = i2c_read_u8 (tests[i].slave_addr, tests[i].addr_offset);
	tests[i].get_val.ival = temp + temp1;
	if ((tests[i].get_val.ival < tests[i].min) || (tests[i].get_val.ival > tests[i].max))
	{
	    tprintf_err("%s Error: %d C Degree,  min %d C Degree,  max %d C Degree",
		tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
	    tprintf_out("\n");
	    return 1;
	}
	tprintf_out("%-25s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
    }
    return 0;
}

void usage(void)
{
	tprintf_out("Usage: hwmodiag_for_stuttgart [-i test_item] [-L low_limit] [-H high_limit]\n");
	tprintf_out("  -i  Set item to test, eg.\n");
	tprintf_out("      (About Voltage)\n");
	tprintf_out("        emc_2.5          emc_vccp         dme_cpu0_vid_1.3 emc_cpu1_vid_1.3\n");
	tprintf_out("        emc_vcc_3.3      emc_12           dme_sio_vtr_5    dme_sio_vccp\n");
	tprintf_out("        dme_sio_vcc_3.3  dme_sio_5        dme_sio_12       dme_sio_vtr_3.3\n");
	tprintf_out("        dme_vbat_3.0\n");
	tprintf_out("      (About Fan)\n");
	tprintf_out("        emc_cpu0_fan     emc_cpu1_fan\n");
	tprintf_out("        dme_sys_fan1     dme_sys_fan2     dme_sys_fan3\n");
	tprintf_out("        dme_dimm_fan0     dme_dimm_fan1\n");
	tprintf_out("      (About Temperature)\n");
	tprintf_out("        emc_cpu0_temp    emc_cpu1_temp\n");
	tprintf_out("        dme_sys_temp1    dme_sys_temp2    dme_sys_temp3    emc_sys_temp4\n");
	tprintf_out("  -L  Set low limit.\n");
	tprintf_out("  -H  Set high limit.\n");
	tprintf_out("  -v  Get version information.\n");
	tprintf_out("  -h  Get this message.\n");
	return;
}

u32 getSMBUSAdress()
{
      u32 smbusAddr=-1;
      struct pci_filter filter;
      struct pci_dev* dev=NULL;
      struct pci_dev* pci_dev;
      struct pci_access *pacc;	
      pacc = pci_alloc();           /* Get the pci_access structure */
      /* Set all options you want -- here we stick with the defaults */
      pci_init(pacc);               /* Initialize the PCI library */
      pci_scan_bus(pacc);
      pci_filter_init(pacc, &filter);
      unsigned int vendor=0x10de;
      unsigned int device=0x0368;		

      filter.vendor=vendor;
      filter.device=device;

      for(dev=pacc->devices; dev!=NULL; dev=dev->next)    /* Iterate over all devices */
      {
            if(pci_filter_match(&filter,dev))
            {
                pci_dev=dev;
                break;
             }
      }

      smbusAddr=pci_read_long(pci_dev,0x24);	
      pci_cleanup(pacc);

      smbus_base_addr=smbusAddr&0xfffffffc;
      syslog(LOG_ERR,"SMBUS Addr=%x",smbus_base_addr);	
      return smbusAddr &= 0xfffffffc;	

}


int main (int argc, char **argv)
{
    int ret = 1, ch = 0;
    int l=0, h=0;

    if(argc == 1)
    {
	usage();
	return 0;
    }

    while((ch = getopt(argc, argv, "vhi:L:H:")) != -1)
    {
	switch (ch)
	{
	    case 'v':
		tprintf_out("\n%s\n\n",VER_STR);
		exit(0);
	    case 'h':
		usage();
		return 0;
	    case 'i':
		for(i=0; i<26; i++)
		{
		    if (!(strncmp(optarg, tests[i].name, strlen(optarg))))
			break;
		    else
			continue;
		}
		if (i>25)
		{
		    tprintf_err("test item error, no such name!");
		    tprintf_out("\n\n");
		    usage();
		    return 1;
		}
	    case 'L':
		tests[i].min = atoi(optarg);
		l=1;
		break;
	    case 'H':
		tests[i].max = atoi(optarg);
		h=1;
		break;
	    default:
		usage();
		return 1;
	}
    }
    iopl(3);

    	
//    if (get_smbus_addr() < 0)

/*    if(getSMBUSAdress()<0)
    {
	tprintf_err("..........FAIL");
	tprintf_out("\n\n");
	tprintf_err("Can not get smbus addr");
	tprintf_out("\n\n");
	return 1;
    }*/
    smbus_base_addr=0x3400;

    openlog("HW Monitor",LOG_CONS,LOG_USER);  
    //syslog(LOG_ERR,"Begin test");	
    ret = read_chip_id();
    if (ret)
    {
	tprintf_err("Hardware Monitor Chip error: wrong chip or bad chip");
	tprintf_out("\n");
	tprintf_err("Read Company ID: Chip0 = 0x%x, Chip1 = 0x%x", chip_id[0].company_id, chip_id[1].company_id);
	tprintf_out("\n");
	tprintf_out("Respect Company ID: DME1737 = 0x5c, EMC6D103 = 0x5c\n");
	tprintf_out("1. Please check whether it is DME1737 or EMC6D103\n");
	tprintf_out("2. Please check whether it is broken\n");
	return(ret);
    }
    if(l && h)
	ret = test_hwm();
    else
    {
	tprintf_err("Please set Low/High Limits!");
	tprintf_out("\n\n");
	usage();
	return 1;
    }
    if (ret)
    {
	tprintf_err("hwmodiag_for_Stuttgart..........%s test FAIL", tests[i].name);
	tprintf_out("\n\n");
	iopl(0);
	return ret;
    }
    else
	tprintf_out("hwmodiag_for_Stuttgart..........%s test PASS\n", tests[i].name);
    closelog();
    iopl(0);
   //syslog(LOG_ERR,"End test");

    return ret;
}

/*
void outb_p(int value, int addr)
{
  asm{
	   mov	dx,addr
	   mov	al,value
	   out 	dx,al
     }
}

unsigned int inb_p(int addr)
{
   unsigned int value;
   asm{
           mov  dx, addr
           in   al, dx
           mov  value, al
      }
   return (value);
}
*/

u32 pcibios_read_config_u32(u32 busnum, u32 devnum, u32 funnum, u32 regnum)
{
	u32 value=-1;
	u32 config;

	config = (0x80000000|(busnum<<16)|(devnum<<11)|(funnum<<8)) + regnum;
//	iopl(3);
/*	asm{
		.386
		mov  dx, 0cf8h
		mov  eax, config
		out  dx, eax
		mov  dx, 0cfch
		in   eax, dx
		mov  value, eax
		}; */

	outl(config, 0x0CF8);
        usleep(50);
	value = inl(0x0CFC);

/*	if(value!=0x3400)
	{*/
		syslog(LOG_ERR,"SMBUS Address=%x\n",value);
	/*	return -1;
	}*/
        if(value == 0xFFFFFFFF)
		value = -1;
	return value;
}

int probe_smbus (u32 pci_id)
{
    int i = 0, ret = 1;
    while(smbus_controller_list[i].pci_id)
    { 
	if ( smbus_controller_list[i].pci_id == pci_id)
	{
	    smbus_base_offset = smbus_controller_list[i].base_offset;
	    smbus_value_bit = smbus_controller_list[i].value_bit;
	    ret = 0;
	    break;
	}
	i++;
    }
    return ret;
}

int get_smbus_addr (void)
{
    u32 pci_bus = 0, pci_dev = 0, pci_fn = 0, pci_id = 0;

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
//		    printf("%x %x %x %x\n", pci_bus, pci_dev, pci_fn, smbus_base_offset);
	  	    smbus_base_addr = pcibios_read_config_u32 (pci_bus, pci_dev, pci_fn, smbus_base_offset);
		    smbus_base_addr &= 0xfffffffc;
//		    printf("SMBus base address %8.8x\n", smbus_base_addr);
		    return 0;
		}
	    }
    return 1;
}

inline int i2c_read_u8(int sa, u8 offset)
{
	unsigned char status=0x80;
        unsigned char readcount=0;
	unsigned char value=0xFF;
/*	outb_p(0, DATA0);
	outb_p(0, STS);*/

 //syslog(LOG_ERR,"smbus add=%x",smbus_base_addr);
		if(status!=0x80)
		{
			readcount++;
			//printf("ERROR:Status=0x%x, 0x%x value=0x%x\n",status,offset,inb_p(DATA0));
		}
		outb_p(sa << 1, ADDR);
		status=inb_p(STS);
 //syslog(LOG_ERR,"Out ADDR END %x");
		
		if(~status&0x80)
                {
			syslog(LOG_ERR,"Out ADDR FAILED");
                        usleep(10);
                }
                
		outb_p(offset, CMD);
 //syslog(LOG_ERR,"Out CMD");

                status=inb_p(STS);

                if(~status&0x80)
                {
                        syslog(LOG_ERR,"Out CMD FAILED");
                        usleep(10);
                }

		outb_p(0x07, PRTCL);
// syslog(LOG_ERR,"Out PRTL END");
		status=inb_p(STS);
		if(~status&0x80)
		{	
			usleep(500);
			status=inb_p(STS);
		}

                if(~status&0x80)
                {       
		        syslog(LOG_ERR,"Read Second");
                        usleep(300000);
                        status=inb_p(STS);
                }

		if(~status&0x80 || status&0x1f )
		{ 
			tprintf_err("SMBUS TIMEOUT Status=%x Errorcode=%x ",status,status&0x1f );	
		        tprintf_out("SMBUS TIMEOUT Status=%x Errorcode=%x ",status,status&0x1f ); 
	
			syslog(LOG_ERR,"SMBUS TIMEOUT Status=%x Errorcode=%x ",status,status&0x1f);
                        return 0xFF;
		}

		//syslog(LOG_ERR,"READ END");
		return inb_p(DATA0);

/*
	outb_p(0, DATA0);
	outb_p(0, STS);
	outb_p(offset, CMD);
	outb_p(sa << 1, ADDR);
	outb_p(0x07, PRTCL);
	usleep(500);
	usleep(500);
	return inb_p(DATA0);
*/

}

void i2c_write_u8(int sa, u8 offset, u8 data)
{
/*	outb(0, DATA0);
	outb(0, STS);

	outb(offset, CMD);
	outb(sa << 1, ADDR);
	outb(data, DATA0);
	usleep(500);
	outb(0x06, PRTCL);
	usleep(500);*/
	unsigned char status=0x80;
	unsigned char readcount=0;
	do
	{
		if(status!=0x80);
		{
			readcount++;
			printf("ERROR:Status=0x%x; 0x%x read value=0x%x, respect write value=0x%x\n",status,offset,inb_p(DATA0),data);
		}
		outb(offset, CMD);
		outb(sa << 1, ADDR);
		outb(data, DATA0);
		usleep(500);
		outb(0x06, PRTCL);
		usleep(500);
	}while((status = inb_p(STS))!=0x80 && readcount<Retry);
}

int read_chip_id (void)
{
	int result = 1;

	chip_id[0].company_id = i2c_read_u8(slaveadd, 0x3e);
	chip_id[1].company_id = i2c_read_u8(slaveadd1, 0x3e);

	if((chip_id[0].company_id != 0x5c))
	{
		syslog(LOG_ERR,"Could not find DME chip");
		return result;
	}
        if (chip_id[1].company_id != 0x5c)
	{
		syslog(LOG_ERR,"Could not find EMC chip");
                return result;
	}

	result = 0;

	return (result);
}

u32 ReadThermState(int cpunode)
{
    u32 busnum=0;
    u32 devnum=24;
    u32 funnum=3;
    u32 regnum=0xe4;
    u32 value=-1;

    devnum = devnum + cpunode;
    value = pcibios_read_config_u32(busnum, devnum, funnum, regnum);
    return value;
}

/*
VIDTbl:
	DB      00000b     ;;00000 1.550v  1
	DB      00001b     ;;00001 1.525V  2
	DB      00010b     ;;00010 1.500v  3
	DB      00011b     ;;00011 1.475v  4
	DB      00100b     ;;00100 1.450v  5
	DB      00101b     ;;00101 1.425v  6	
	DB      00110b     ;;00110 1.400v  7
	DB      00111b     ;;00111 1.375v  8
	DB      01000b     ;;01000 1.350v  9
	DB      01001b     ;;01001 1.325v  10
	DB      01010b     ;;01010 1.300v  11
	DB      01011b     ;;01011 1.275v  12   
	DB      01100b     ;;01100 1.250v  13 
	DB      01101b     ;;01101 1.225v  14
	DB      01110b     ;;01110 1.200v  15
	DB      01111b     ;;01111 1.175v  16
	DB      10000b     ;;10000 1.150v  17
	DB      10001b     ;;10001 1.125v  18
	DB      10010b     ;;10010 1.100v  19
	DB      10011b     ;;10011 1.075v  20
	DB      10100b     ;;10100 1.050v  21
	DB      10101b     ;;10101 1.025v  22
	DB      10110b     ;;10110 1.000v  23
	DB      10111b     ;;10111 0.975v  24
	DB      11000b     ;;11000 0.950v  25
	DB      11001b     ;;11001 0.925v  26
	DB      11010b     ;;11010 0.900v  27
	DB      11011b     ;;11011 0.875v  28
	DB      11100b     ;;11100 0.850v  29
	DB      11101b     ;;11101 0.825v  30
        DB      11110b     ;;11110 0.800v  31
	DB      11111b     ;;11111 off     32
*/
