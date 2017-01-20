//*************************************************************
//*    This utility is to test MarrakechII Hardware Monitor   *
//*************************************************************
//* Create date: 2006/2/9
//* Auther: MiTAC Shanghai ESBU/SW Yuanjie Xu
//*
//* Chipset:   NVDIA MCP55
//* HWmonitor: ADT7476
//******************************************************
//* Rev.   Date        By            Description       *
//******************************************************
//* 0.1   2006/2/9    Yuanjie Xu   Initial version
//* 0.2   2006/3/24   Yuanjie Xu   VCCP max = 1.35, not 1.5
//* 0.3   2006/4/5    Yuanjie Xu   CPU Temperature should be modified by a 
//*                                Thermtrip Status Register
//* 0.4   2006/5/10   Yuanjie Xu   (1)delete fan4
//*                                (2)modify CPU temperature diode
//* 0.5   2006/6/14   Yuanjie Xu   BIOS modified CPU temp diode, so no need
//*                                to modify
//* 0.6   2006/7/12   Yuanjie Xu   0x40 bit 7 reset, for Fan2 bug
//* 0.7   2006/7/17   Yuanjie Xu   Modify CPU VCPP voltage stand value from 1.30 to 1.35
//* 0.8   2006/7/20   Yuanjie Xu   io open: iopl(3); io close: iopl(0)

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <sys/io.h>
#include "utils.h"

typedef unsigned char 		u8;
typedef unsigned int 		u16;
typedef unsigned long 		u32;
typedef unsigned long long 	u64;

char *VER_STR="hwmodiag for Munich, Version 0.8 (c) 2006, MiTAC Inc.";

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
}chip_id = {0,0};

/*Hardware Monitor Define*/
#define slaveadd	0x2E

/*NVDIA MCP55 SMBus address offsets */
#define PRTCL  (smbus_base_addr + 0)
#define STS    (smbus_base_addr + 1)
#define ADDR   (smbus_base_addr + 2)
#define CMD    (smbus_base_addr + 3)
#define DATA0  (smbus_base_addr + 4)

/*ADT7476 start monitor register*/
#define config_reg	0x40

enum TEST_TYPE
{
	enum_V25 = 0,
	enum_VCCP,
	enum_VCC,
	enum_V5,
	enum_V12,
	enum_VID,
	enum_Fan1,
	enum_Fan2,
	enum_Fan3,
	enum_Fan4,
	enum_R1Temp,
	enum_LTemp,
	enum_R2Temp
};

struct hwm_item
{
	char name[15];
	u8 slave_addr;
	u8 addr;
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

#define Ext_Reg1	0x76
#define Ext_Reg2	0x77

//vol
#define V2_5		0x20
#define VCCP		0x21
#define VCC			0x22
#define V5			0x23
#define V12			0x24
#define VID			0x43
//fan
#define FANTACH1_L 	0x28
#define FANTACH1_H 	0x29
#define FANTACH2_L 	0x2A
#define FANTACH2_H 	0x2B
#define FANTACH3_L 	0x2C
#define FANTACH3_H	0x2D
#define FANTACH4_L 	0x2E
#define FANTACH4_H 	0x2F
//temperature
#define R1_Temp		0x25
#define L_Temp		0x26
#define R2_Temp		0x27


static struct hwm_item tests[] =
{
	{"2.5", slaveadd, V2_5, {0.0}, {0.0}, {0.0}, 0.0032, 95, 105, 2.5, "YES"},
	{"vccp_1.5", slaveadd, VCCP, {0.0}, {0.0}, {0.0}, 0.00293, 95, 105, 1.35, "YES"},
	{"vcc_3.3", slaveadd, VCC, {0.0}, {0.0}, {0.0}, 0.0042, 95, 105, 3.3, "YES"},
  	{"5", slaveadd, V5, {0.0}, {0.0}, {0.0}, 0.0065, 95, 105, 5.0, "YES"},
	{"12", slaveadd, V12, {0.0}, {0.0}, {0.0}, 0.0156, 95, 105, 12.0, "YES"},
	{"vid_1.35", slaveadd, VID, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 1.35, "YES"},
	{"fan1", slaveadd, FANTACH1_L, {0.0}, {0.0}, {0.0}, 5400000.0, 2500, 14000, 0.0, "YES"},
	{"fan2", slaveadd, FANTACH2_L, {0.0}, {0.0}, {0.0}, 5400000.0, 2500, 14000, 0.0, "YES"},
	{"fan3", slaveadd, FANTACH3_L, {0.0}, {0.0}, {0.0}, 5400000.0, 2500, 14000, 0.0, "YES"},
//	{"fan4", slaveadd, FANTACH4_L, {0.0}, {0.0}, {0.0}, 5400000.0, 2500, 14000, 0.0, "YES"},
	{"cpu_temp", slaveadd, R1_Temp, {0}, {0}, {0}, 1.0, 10, 60, 0.0, "YES"},
	{"local_temp", slaveadd, L_Temp, {0}, {0}, {0}, 1.0, 10, 60, 0.0, "YES"},
	{"sys_temp", slaveadd, R2_Temp, {0}, {0}, {0}, 1.0, 10, 60, 0.0, "YES"},
	{"", 0, 0, {0}, {0}, {0}, 0.0, 0, 0, 0.0, ""}
};

/*Function Define*/
//void outb_p(int value, int addr);
//unsigned int inb_p(int addr);
//#define inp(p) inportb(p)
//#define outp(v,p) outportb(v,p)
u32 pcibios_read_config_u32(u32 busnum, u32 devnum, u32 funnum, u32 regnum);
int probe_smbus (u32 pci_id);
int get_smbus_addr (void);
int i2c_read_u8(int sa, u8 offset);
void i2c_write_u8(int sa, u8 offset, u8 data);
int read_chip_id (void);
void hwmtest_start(void);
u32 ReadThermState(int cpunode);
void hwm_reset(void);

int i;
int test_hwm(void)
{
    int type = 0;
    int temp = 0;

    temp = i2c_read_u8 (tests[i].slave_addr, tests[i].addr);
//    printf("temp: %d\n", temp);
    if(temp == 0xff)
    {
	tprintf_err("%-15s :  Function Error", tests[i].name);
	tprintf_out("\n");
	return 1;
    }

    type = i;

//vol
    if (type>=0 && type<6)
    {
	int temp1 = 0, temp2 = 0, vol = 0;
	temp1 = i2c_read_u8 (tests[i].slave_addr, Ext_Reg1);
	temp2 = i2c_read_u8 (tests[i].slave_addr, Ext_Reg2);
	switch(type)
	{
	    case enum_V25:
		vol = (temp << 2) + (temp1 & 0x03);
		break;
	    case enum_VCCP:
		vol = (temp << 2) + ((temp1 & 0x0C) >> 2);
		break;
	    case enum_VCC:
		vol = (temp << 2) + ((temp1 & 0x30) >> 4);
		break;
	    case enum_V5:
		vol = (temp << 2) + ((temp1 & 0xC0) >> 6);
		break;
	    case enum_V12:
		vol = (temp << 2) + (temp2 & 0x03);
		break;
	}	
	tests[i].get_val.fval = (float)vol * (tests[i].p);
//	tests[5].get_val.fval = 0.800 + (0x1E - (temp & 0x1F)) * 0.025; //VID
	tests[5].get_val.fval = 1.550 - (temp & 0x1F)*0.025;
	tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
	tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;

	if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
	{
	    tprintf_err("%s Error: %4.3f V, min %4.3f V, max %4.3f V",
		tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
	    tprintf_out("\n");	
	    return 1;
	}
	tprintf_out("%-15s :  %4.3f V\n", tests[i].name, tests[i].get_val.fval);
    }

//fan
    else if(type>=6 && type<9)
    {
	int l = 0, m = 0, speed = 0;
	l = temp;
	m = i2c_read_u8 (tests[i].slave_addr, (tests[i].addr)+1);
	if((l == 0xff) && (m == 0xff))
	{
	    tprintf_err("%-15s :  Function Error", tests[i].name);
	    tprintf_out("\n");
	    return 1;
	}
	speed = (m << 8) + l;
	tests[i].get_val.fval = (tests[i].p)/speed;
	if ((tests[i].get_val.fval) < (tests[i].min) || (tests[i].get_val.fval) > (tests[i].max))
	{
	    tprintf_err("%s speed Error: %d RPM,  min %d RPM,  max %d RPM",
		tests[i].name, (int)(tests[i].get_val.fval), tests[i].min, tests[i].max);
	    tprintf_out("\n");
	    return 1;
	}
	tprintf_out("%-15s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
    }

//temperature
    else
    {
//	int diode_val=0;
	if((temp == 0xff) || (temp == 0x80))
	{
	    tprintf_err("%-15s :  Function Error", tests[i].name);
	    tprintf_out("\n");
	    return 1;
	}
/*	if (i == 9)
	{
	    diode_val = (ReadThermState(0) >> 8) & 0x3F;
	    printf("diode_val: %d\n", diode_val);
	    if(diode_val!=0)
		tests[i].get_val.ival = temp + (11 - diode_val);
	    else
		tests[i].get_val.ival = temp;
	}
	else*/
	    tests[i].get_val.ival = temp;
	if ((tests[i].get_val.ival) < (tests[i].min) || (tests[i].get_val.ival) > (tests[i].max))
	{
	    tprintf_err("%s Error: %d C Degree,  min %d C Degree,  max %d C Degree",
		tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
	    tprintf_out("\n");
	    return 1;
	}
	tprintf_out("%-15s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
    }
    return 0;
}

void usage(void)
{
	tprintf_out("Usage: hwmodiag_for_munich [-i test_item] [-L low_limit] [-H high_limit]\n");
	tprintf_out("  -i  Set item to test, eg.\n");
	tprintf_out("      (About Voltage)\n");
	tprintf_out("        2.5        vccp_1.5   vcc_3.3    5          12         vid_1.35\n");
	tprintf_out("      (About Fan)\n");
	tprintf_out("        fan1       fan2       fan3\n");
	tprintf_out("      (About Temperature)\n");
	tprintf_out("        cpu_temp   local_temp sys_temp\n");
	tprintf_out("  -L  Set low limit.\n");
	tprintf_out("  -H  Set high limit.\n");
	tprintf_out("  -v  Get version information.\n");
	tprintf_out("  -h  Get this message.\n");	
    return;
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
		for(i=0; i<12; i++)
		{
		    if (!(strncmp(optarg, tests[i].name, strlen(optarg))))
			break;
		    else
			continue;
		}
		if (i>11)
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
    if (get_smbus_addr() < 0)
    {
	tprintf_err("..........FAIL");
	tprintf_out("\n\n");
	tprintf_err("Can not get smbus addr");
	tprintf_out("\n\n");
	iopl(0);
	return 1;
    }
    ret = read_chip_id();
    if (ret)
    {
	tprintf_err("Hardware Monitor Chip error: wrong chip or bad chip");
	tprintf_out("\n");
	tprintf_err("Hardware Monitor Chip ID: CompanyID = 0x%x,  DeviceID = 0x%x", chip_id.company_id, chip_id.device_id);
	tprintf_out("\n");
	tprintf_out("1. Please check whether it is ADT7463\n");
	tprintf_out("2. Please check whether it is broken\n");
	iopl(0);
	return(ret);
    }
    hwmtest_start();
    if(l && h)
    {
	hwm_reset();
	ret = test_hwm();
    }
    else
    {
	tprintf_err("Please set Low/High Limits!");
	tprintf_out("\n\n");
	iopl(0);
	usage();
	return 1;
    }
    if (ret)
    {
	tprintf_err("hwmodiag_for_munich..........%s test FAIL", tests[i].name);
	tprintf_out("\n");
	iopl(0);
	return ret;
    }
    else
	tprintf_out("hwmodiag_for_munich..........%s test PASS\n", tests[i].name);
    iopl(0);
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
	value = inl(0x0CFC);
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

int i2c_read_u8(int sa, u8 offset)
{
	outb_p(0, DATA0);
	outb_p(0, STS);

	outb_p(offset, CMD);
	outb_p(sa << 1, ADDR);
	outb_p(0x07, PRTCL);

	usleep(500);
	usleep(500);

	return inb_p(DATA0);
}

void i2c_write_u8(int sa, u8 offset, u8 data)
{
	outb(0, DATA0);
	outb(0, STS);

	outb(offset, CMD);
	outb(sa << 1, ADDR);
	outb(data, DATA0);
	usleep(500);
	outb(0x06, PRTCL);
	usleep(500);
}

int read_chip_id (void)
{
	int result = 1;

	chip_id.device_id = i2c_read_u8(slaveadd, 0x3d);
	chip_id.company_id = i2c_read_u8(slaveadd, 0x3e);
//	printf("\ndevice_id = 0x%x,  company_id = 0x%x\n", chip_id.device_id, chip_id.company_id);

	if ((chip_id.device_id == 0x76) && (chip_id.company_id == 0x41))
	    result = 0;
	
	return (result);
}

void hwmtest_start(void)
{
	u8 value = i2c_read_u8(slaveadd, config_reg);
//	printf("0x40: 0x%x\n", value);
	i2c_write_u8(slaveadd, config_reg, (value | 0x11));
//	printf("0x40: 0x%x\n", i2c_read_u8(slaveadd, config_reg));
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

void hwm_reset(void)
{
	/*ADT7476 Register 0x40 bit7 ---- Reset*/
	u8 value = i2c_read_u8(slaveadd, 0x40);
	i2c_write_u8(slaveadd, 0x40, value|0x80);
}
