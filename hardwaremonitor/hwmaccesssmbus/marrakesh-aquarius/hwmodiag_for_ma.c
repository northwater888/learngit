#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include "utils.h"
#if defined(__linux__)  &&  __GNU_LIBRARY__ == 1
#include <asm/io.h>		/* Newer libraries use <sys/io.h> instead. */
#else
#include <sys/io.h>
#endif
#if !defined(__OPTIMIZE__)
#error You must compile this driver with "-O"!
#endif

#undef PDEBUG /* undef it, just in case */
#ifdef _DEBUG_
#  define PDEBUG(fmt, args...) printf( "hwmodiag: " fmt, ## args)
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif
int pcibios_read_config_byte (unsigned char bus,
			      unsigned char dev_fn,
			      unsigned char where, unsigned char *val);
int pcibios_read_config_word (unsigned char bus, unsigned char dev_fn,
			      unsigned char regnum, unsigned short *val);
int pcibios_read_config_dword (unsigned char bus,
			       unsigned char dev_fn,
			       unsigned char regnum,
			       unsigned int *val);
void pcibios_write_config_byte (unsigned char bus, unsigned char dev_fn,
				unsigned char regnum, unsigned char val);
void pcibios_write_config_word (unsigned char bus, unsigned char dev_fn,
				unsigned char regnum, unsigned short val);
void pcibios_write_config_dword (unsigned char bus, unsigned char dev_fn,
				 unsigned char regnum, unsigned int val);

static unsigned int smbus_base_addr = 0;
static unsigned int smbus_base_offset = 0;
static unsigned int smbus_value_bit = 0;
char *VER_STR="hwmodiag for Marrakesh/Aqurius, Version 1.0 (c) 2005, MiTAC Inc.";

/* 
 * Data for SMBus Messages 
 */
union i2c_smbus_data {
        unsigned char byte;
        unsigned short word;
        unsigned char block[33]; /* block[0] is used for length */
};


/* smbus_access read or write markers */
#define I2C_SMBUS_READ  1
#define I2C_SMBUS_WRITE 0


/* SMBus transaction types (size parameter in the above functions) 
   Note: these no longer correspond to the (arbitrary) PIIX4 internal codes! */
#define I2C_SMBUS_QUICK             0
#define I2C_SMBUS_BYTE              1
#define I2C_SMBUS_BYTE_DATA         2 
#define I2C_SMBUS_WORD_DATA         3
#define I2C_SMBUS_PROC_CALL         4
#define I2C_SMBUS_BLOCK_DATA        5
#define I2C_SMBUS_I2C_BLOCK_DATA    6
#ifdef HAVE_PEC
#define I2C_SMBUS_BLOCK_DATA_PEC    7
#endif

#define I2C_SMBUS       0x0720 /* SMBus-level access */



/*SMBus address offsets */
#define PRTCL  (smbus_base_addr + 0)			
#define STS    (smbus_base_addr + 1)			
#define ADDR   (smbus_base_addr + 2)			
#define CMD    (smbus_base_addr + 3)			
#define DATA0  (smbus_base_addr + 4)			


/* PCI Address Constants */
#define SMBBA     0x020
#define SMBHSTCFG 0x040
#define SMBREV    0x008

/* Host configuration bits for SMBHSTCFG */
#define SMBHSTCFG_HST_EN      1
#define SMBHSTCFG_SMB_SMI_EN  2
#define SMBHSTCFG_I2C_EN      4

/* Other settings */
#define MAX_TIMEOUT 100
#define ENABLE_INT9 0   /* set to 0x01 to enable - untested */

/* I801 command constants */
#define I801_QUICK          0x00
#define I801_BYTE           0x04
#define I801_BYTE_DATA      0x08
#define I801_WORD_DATA      0x0C
#define I801_PROC_CALL      0x10        /* later chips only, unimplemented */
#define I801_BLOCK_DATA     0x14
#define I801_I2C_BLOCK_DATA 0x18        /* unimplemented */
#define I801_BLOCK_LAST     0x34
#define I801_I2C_BLOCK_LAST 0x38        /* unimplemented */
#define I801_START          0x40
#define I801_PEC_EN         0x80        /* ICH4 only */

#define w83627_a        0x295


/* SMBus command constants */
#define SMB_QUICK          0x00
#define SMB_BYTE           0x04
#define SMB_BYTE_DATA      0x08
#define SMB_WORD_DATA      0x0C

struct smbus_controller
{
	unsigned int pci_id;
	int base_offset;
	int value_bit;
};


static struct smbus_controller smbus_controller_list[] = 
{
	{ 0x25a48086, 0x20, 0xffe0}, //Hance Rapids ICH SMBus Controller
	{ 0x24838086, 0x20, 0xffe0}, //ICH3 SMBus Controller
	{ 0x005210de, 0x20, 0xffff}, //CK804 SMBus Controller
	{ 0, 0, 0},
};

struct hwmo_chip
{
	int slave_addr;
	int vendor_id_offset;
	int vendor_id;
	int chip_id_offset;
	int chip_id;
	int bank_offset;
	int chip_type;
};

/* Return -1 on error. */
static int i2c_read_byte(unsigned char offset)
{
        //unsigned char temp = 0x0;
	outb_p(0, DATA0);
	outb_p(0, STS);

	outb_p(offset, CMD);
	outb_p(0x2e << 1, ADDR);
	outb_p(0x07, PRTCL);

	usleep(500);

        //printf("STS: 0x%x\n", inb_p(STS));
        //printf("DATA0: 0x%x\n", inb_p(DATA0));

        /*temp = inb_p(SMBHSTSTS);

        if (temp == 0x1A ) {
                usleep(500);
                temp = inb_p(SMBHSTSTS);
        }
        if (temp == 0x1A ) {
                usleep(500);
                temp = inb_p(SMBHSTSTS);
        }

        if (temp)
        {
                printf("SMBus error: 0x%x\n", temp);
                return -1;
        }*/

	//data->byte = inb_p(DATA0);

        return inb_p(DATA0);
}


int probe_smbus (unsigned int pci_id)
{
	int i = 0, ret = -1;
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
	int pci_bus = 0, pci_dev_fn;
	/* Get access to all of I/O space. */
	if (iopl (3) < 0)
	{
      		perror ("pci-config: iopl()");
      		fprintf (stderr, "This program must be run as root.\n");
      		return -1;
    	}
  	for (pci_bus = 0; pci_bus < 32; pci_bus++)
    	{
      		for (pci_dev_fn = 0; pci_dev_fn < 256; pci_dev_fn++)
		{
	  		/*unsigned char cb; */
	  		unsigned int pci_id;
	  		pcibios_read_config_dword (pci_bus, pci_dev_fn, 0, &pci_id);
		  	if (pci_id == 0xffffffff)
	    			continue;
			if (probe_smbus(pci_id) < 0)
				continue;
			else
			{
/*	  			printf ("SMBus Device at bus %d device/function %d/%d, %8.8x.\n",
				pci_bus, pci_dev_fn >> 3, pci_dev_fn & 7, pci_id);
				pcibios_read_config_word (pci_bus, pci_dev_fn, 0x00, &smbus_base_addr);
				printf("ID %4.4x\n", smbus_base_addr);
				pcibios_read_config_word (pci_bus, pci_dev_fn, 0x02, &smbus_base_addr);
				printf("Dev ID %4.4x\n", smbus_base_addr);
				pcibios_read_config_word (pci_bus, pci_dev_fn, 0x04, &smbus_base_addr);
				printf("Command %4.4x\n", smbus_base_addr);
				pcibios_read_config_word (pci_bus, pci_dev_fn, 0x06, &smbus_base_addr);
				printf("Status %4.4x\n", smbus_base_addr);
				pcibios_read_config_byte (pci_bus, pci_dev_fn, 0x08, &temp1);
				printf("Revi ID %4.4x\n", temp1);
				pcibios_read_config_dword (pci_bus, pci_dev_fn, 0x09, &temp2);
				printf("class code %8.8x\n", temp2);
				pcibios_read_config_byte (pci_bus, pci_dev_fn, 0x0e, &temp1);
				printf("Header type %4.4x\n", temp1);*/
				pcibios_read_config_dword (pci_bus, pci_dev_fn, smbus_base_offset, &smbus_base_addr);
				smbus_base_addr &= 0xfffffffc;
	/*			printf("SMBus base address %8.8x\n", smbus_base_addr);
				
                                

   	for (i = 0x00; i <= 0x3e; i++)
                                {
					printf("index 0x%x: 0x%x\n", i, inb_p(smbus_base_addr+i));
				}*/
				return 0;
			}
	
	  		if ((pci_dev_fn & 7) == 0)
	    		{
	      			unsigned int cdw;
	      			pcibios_read_config_dword (pci_bus, pci_dev_fn, 3 * 4, &cdw);
	      			if ((cdw & 0x00800000) == 0)
				pci_dev_fn += 7;
	    		}
		}
    	}
    	return -1;
}

#define PCI_CONFIG_ADDR 0x0cf8
#define PCI_CONFIG_DATA 0x0cfc
int
pcibios_read_config_byte (unsigned char bus, unsigned char dev_fn,
			  unsigned char regnum, unsigned char *val)
{
  outl (0x80000000 | (bus << 16) | (dev_fn << 8) | (regnum & 0xfc),
	PCI_CONFIG_ADDR);
  *val = inb (PCI_CONFIG_DATA + (regnum & 3));
  return 0;
}

int
pcibios_read_config_word (unsigned char bus, unsigned char dev_fn,
			  unsigned char regnum, unsigned short *val)
{
  outl (0x80000000 | (bus << 16) | (dev_fn << 8) | (regnum & 0xfc),
	PCI_CONFIG_ADDR);
  *val = inw (PCI_CONFIG_DATA + (regnum & 2));
  return 0;
}

int
pcibios_read_config_dword (unsigned char bus, unsigned char dev_fn,
			   unsigned char regnum, unsigned int *val)
{
  outl (0x80000000 | (bus << 16) | (dev_fn << 8) | (regnum & 0xfc),
	PCI_CONFIG_ADDR);
  *val = inl (PCI_CONFIG_DATA);
  return 0;
}

void
pcibios_write_config_byte (unsigned char bus, unsigned char dev_fn,
			   unsigned char regnum, unsigned char val)
{
  outl (0x80000000 | (bus << 16) | (dev_fn << 8) | (regnum & 0xfc),
	PCI_CONFIG_ADDR);
  outb (val, PCI_CONFIG_DATA + (regnum & 3));
  return;
}

void
pcibios_write_config_word (unsigned char bus, unsigned char dev_fn,
			   unsigned char regnum, unsigned short val)
{
  outl (0x80000000 | (bus << 16) | (dev_fn << 8) | (regnum & 0xfc),
	PCI_CONFIG_ADDR);
  outw (val, PCI_CONFIG_DATA + (regnum & 2));
  return;
}

void
pcibios_write_config_dword (unsigned char bus, unsigned char dev_fn,
			    unsigned char regnum, unsigned int val)
{
  outl (0x80000000 | (bus << 16) | (dev_fn << 8) | (regnum & 0xfc),
	PCI_CONFIG_ADDR);
  outl (val, PCI_CONFIG_DATA);
  return;
}

int get_chip(void)
{
	int vendorid = 0;
	//int chipid = 0;
//	int slaveadd = 0x0;

	vendorid = i2c_read_byte(0x3E);
	if (vendorid != 0x5c)
	{
//		printf("Company ID: 0x%x not 0x5c error!\n", vendorid); 
		return -1; 
	} 
        /*printf("Company ID: 0x%x\n", vendorid); 
        chipid = i2c_read_byte (0x3F);
        printf("Version: 0x%x\n", chipid); */
        return 0;	
}

int i;

enum TEST_TYPE
{
	enum_VTR5 = 0,
	enum_Vccp,
	enum_VCC,
	enum_V5,
	enum_V12,
	enum_VTR,
	enum_Vbat,
	enum_CPU_FAN,
	enum_Sys_FAN1,
	enum_Sys_FAN2,
	enum_Sys_FAN3,
	enum_CPU_Temp,
	enum_Sys_Temp1,
	enum_Sys_Temp2
};

struct hwm_item
{
	char name[15];
	unsigned char addr;
	union
	{
		int	ival;
		double fval;
	} get_val, get_min, get_max, p;
	int min;
	int max;
	float stand_value;
};

//vol
#define VTR5    0x20
#define Vccp    0x21
#define VCC     0x22
#define V5 	    0x23
#define V12	    0x24
#define VID	    0x43
#define VTR     0x99
#define Vbat    0x9A
//fan
#define FANTACH1_L	0x28
#define FANTACH1_M	0x29
#define FANTACH2_L	0x2A
#define FANTACH2_M	0x2B
#define FANTACH3_L	0x2C
#define FANTACH3_M	0x2D
#define FANTACH4_L	0x2E
#define FANTACH4_M	0x2F
//temp
#define CPU_TEM				0x25
#define SYS_TEM1			0x26
#define SYS_TEM2			0x27
#define SYS_TEM1_OFFSET		0x1D
#define SYS_TEM2_OFFSET		0x1E


static struct hwm_item tests[] =
{
  	{"vdimm_2.6", VTR5, {0.0}, {0.0}, {0.0}, {1.0}, 95, 105, 2.6},
	{"vccp_1.5", Vccp, {0.0}, {0.0}, {0.0}, {1.0}, 95, 105, 1.5},
	{"vcc_3.3", VCC, {0.0}, {0.0}, {0.0}, {1.0}, 95, 105, 3.3},
	{"5", V5, {0.0}, {0.0}, {0.0}, {1.0}, 95, 105, 5.0},
	{"12", V12, {0.0}, {0.0}, {0.0}, {1.0}, 95, 105, 12.0},
	{"vtr_3.3", VTR, {0.0}, {0.0}, {0.0}, {1.0}, 95, 105, 3.3},
	{"vbat_3.0", Vbat, {0.0}, {0.0}, {0.0}, {1.0}, 95, 105, 3.0},
	{"cpu_fan", FANTACH1_L, {0.0}, {0.0}, {0.0}, {1.0}, 4000, 8000, 1.0},
	{"sys_fan1", FANTACH3_L, {0.0}, {0.0}, {0.0}, {1.0}, 2500, 6500, 1.0},
	{"sys_fan2", FANTACH4_L, {0.0}, {0.0}, {0.0}, {1.0}, 2500, 6500, 1.0},
	{"sys_fan3", FANTACH2_L, {0.0}, {0.0}, {0.0}, {1.0}, 2500, 6500, 1.0},
	{"cpu_temp", CPU_TEM, {0.0}, {0.0}, {0.0}, {1}, 20, 60, 1.0},
	{"sys_temp1", SYS_TEM1, {0.0}, {0.0}, {0.0}, {1.0}, 10, 60, 1.0},
	{"sys_temp2", SYS_TEM2, {0.0}, {0.0}, {0.0}, {1.0}, 10, 60, 1.0},
	{"", 0, {0}, {0}, {0}, {0}, 0, 0, 0.0}
};

int test_hwm(void)
{
	int temp=0, temp1=0, temp2=0, speed=0;
	int type=0;

	temp = i2c_read_byte(tests[i].addr);
	if(temp == 0xff)
	{
		tprintf_err("%-18s :  Function Error.\n", tests[i].name);
		return 1;
	}

	type = i;
	switch(type)
	{
		case enum_VTR5:
			tests[i].get_val.fval = temp*(6.64/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				tprintf_err("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			tprintf_out("%-18s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case enum_Vccp:
			tests[i].get_val.fval = temp*(3.00/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				tprintf_err("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			tprintf_out("%-18s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case enum_VCC:
			tests[i].get_val.fval = temp*(4.38/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				tprintf_err("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			tprintf_out("%-18s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case enum_V5:
			tests[i].get_val.fval = temp*(6.64/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				tprintf_err("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			tprintf_out("%-18s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case enum_V12:
			tests[i].get_val.fval = temp*(16.0/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				tprintf_err("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			tprintf_out("%-18s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case enum_VTR:
			tests[i].get_val.fval = temp*(4.38/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				tprintf_err("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			tprintf_out("%-18s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case enum_Vbat:
			tests[i].get_val.fval = temp*(4.38/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				tprintf_err("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			tprintf_out("%-18s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case enum_CPU_FAN:
			temp1 = i2c_read_byte((tests[i].addr+1));
			speed = (temp1 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				tprintf_err("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			tprintf_out("%-18s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case enum_Sys_FAN1:
			temp1 = i2c_read_byte((tests[i].addr+1));
			speed = (temp1 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				tprintf_err("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			tprintf_out("%-18s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case enum_Sys_FAN2:
			temp1 = i2c_read_byte((tests[i].addr+1));
			speed = (temp1 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				tprintf_err("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			tprintf_out("%-18s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case enum_Sys_FAN3:
			temp1 = i2c_read_byte((tests[i].addr+1));
			speed = (temp1 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				tprintf_err("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			tprintf_out("%-18s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case enum_CPU_Temp:
			tests[i].get_val.ival = temp;
			if ((tests[i].get_val.ival < tests[i].min) || (tests[i].get_val.ival > tests[i].max))
			{
				tprintf_err("%s Error: %d, min %d max %d\n",
					tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
				return 1;
			}
			tprintf_out("%-18s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
			break;
		case enum_Sys_Temp1:
			temp2 = i2c_read_byte (SYS_TEM1_OFFSET);
			tests[i].get_val.ival = temp + temp2;
			if ((tests[i].get_val.ival < tests[i].min) || (tests[i].get_val.ival > tests[i].max))
			{
				tprintf_err("%s Error: %d, min %d max %d\n",
					tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
				return 1;
			}
			tprintf_out("%-18s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
			break;
		case enum_Sys_Temp2:
			temp2 = i2c_read_byte (SYS_TEM2_OFFSET);
			tests[i].get_val.ival = temp + temp2;
			if ((tests[i].get_val.ival < tests[i].min) || (tests[i].get_val.ival > tests[i].max))
			{
				tprintf_err("%s Error: %d, min %d max %d\n",
					tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
				return 1;
			}
			tprintf_out("%-18s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
			break;
	}
	return 0;
}

void usage(void)
{
	tprintf_out("Usage: hwmodiag [-i test_item] [-L low_limit] [-H high_limit]\n");
	tprintf_out("  -i  Set item to test, eg.\n");
	tprintf_out("      (About Voltage)\n");
	tprintf_out("        vdimm_2.6 vccp_1.5 vcc_3.3 5 12 vtr_3.3 vbat_3.0\n"); 
	tprintf_out("      (About Fan)\n");
	tprintf_out("        cpu_fan sys_fan1 sys_fan2 sys_fan3\n");
	tprintf_out("      (About Temperature)\n");
	tprintf_out("        cpu_temp sys_temp1 sys_temp2\n");
//	tprintf_out("  -n  Set number of cpu, this use for cpu test items\n");
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
					for(i=0; i<14; i++)
					{
						if (!(strncmp(optarg, tests[i].name, strlen(optarg))))
							break;
						else
							continue;
					}
					if (i>13)
					{
						tprintf_err("test item error, no such name!\n\n");
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

	if (get_smbus_addr() < 0)
	{
		tprintf_out("..........FAIL\n\n");
		tprintf_out("Can not get smbus addr\n\n");
		return -1;
	}
/*	if (get_chip() < 0)
	{
		printf("..........FAIL\n\n");
		printf("Can not get any hardware monitor chip\n\n");
		return -1;
	}
*/
	if(l && h)
		ret = test_hwm();
	else
	{
		tprintf_err("Please set Low/High Limits!\n\n");
		usage();
		return 1;
	}
	if (ret)
	{
		tprintf_err("hwmodiag_for_ma..........%s test FAIL\n", tests[i].name);
		return ret;
	}
	else
		tprintf_out("hwmodiag_for_ma..........%s test PASS\n", tests[i].name);
	return ret;
}
