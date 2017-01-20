#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
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

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned int	u_32t;
typedef unsigned long	DWORD;

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
	{ 0x005210de, 0x24, 0xffff}, //CK804 SMBus Controller
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

#define slaveadd 0x2D
#define slaveadd1 0x2E

/* Return -1 on error. */
static int i2c_read_byte(int sa, unsigned char offset)
{
	//unsigned char temp = 0x0;
	outb_p(0, DATA0);
	outb_p(0, STS);

	outb_p(offset, CMD);
	outb_p(sa << 1, ADDR);
	outb_p(0x07, PRTCL);

	usleep(500);
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
	  			/*printf ("SMBus Device at bus %d device/function %d/%d, %8.8x.\n",
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
				//printf("SMBus base address %8.8x\n", smbus_base_addr);
				
                                

   	/*for (i = 0x00; i <= 0x3e; i++)
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
	int chipid = 0;
//	int slaveadd = 0x0;

	vendorid = i2c_read_byte(slaveadd, 0x3E);
/*        if (vendorid != 0x5c)
	{
		printf("Company ID: 0x%x not 0x5c error!\n", vendorid); 
		return -1; 
	} */
//		printf("Company ID: 0x%x\n", vendorid); 
        chipid = i2c_read_byte (slaveadd, 0x3F);
//		printf("Version: 0x%x\n", chipid); 
        return 0;	
}


/*AMD Thermtrip Status Register*/
#define AMD_Config_Addr_Reg		0x0CF8
#define AMD_Config_Data_Reg		0x0CFC
u_32t io_CF8;
u_32t io_CFC;

u_32t ReadThermState(int cpunode)
{
	u_32t busnum=0;
	u_32t devnum=24;
	u_32t funnum=3;
	u_32t regnum=0xe4;
	u_32t retval=-1;

	iopl(3);
	devnum = devnum+cpunode;
	io_CF8 = (0x80000000|(busnum<<16)|(devnum<<11)|(funnum<<8))+regnum;
	outl(io_CF8,AMD_Config_Addr_Reg);
	retval=inl(AMD_Config_Data_Reg);
	if(retval==0xFFFFFFFF)
		retval=-1;
	return retval;
	iopl(0);
}

enum TEST_TYPE
{
	EMC_VTR5 = 0,
	EMC_Vccp,
	DME_VID,
	EMC_VID,
	EMC_VCC,
	EMC_V12,
	DME_VTR5,
	DME_Vccp,
	DME_VCC,
	DME_V5,
	DME_V12,
	DME_VTR,
	DME_Vbat,
	CPU0_FAN,
	CPU1_FAN,
	CPU0_Temp,
	CPU1_Temp,
	Sys_FAN1,
	Sys_FAN2,
	Sys_FAN3,
	Sys_FAN4,
	Sys_FAN5,
	Sys_FAN6,
	Sys_Temp1,
	Sys_Temp2,
	Sys_Temp3,
	Sys_Temp4
};

struct hwm_item
{
	char name[20];
	unsigned char slave_addr;
	unsigned char addr;
	union
	{
		int	ival;
		double fval;
	} get_val, get_min, get_max;
	double p;
	int min;
	int max;
	float stand_value;
};

int i;

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
//temperature
#define CPU_TEM				0x25
#define SYS_TEM1			0x26
#define SYS_TEM2			0x27
#define SYS_TEM1_OFFSET		0x1D
#define SYS_TEM2_OFFSET		0x1E


static struct hwm_item tests[] =
{
  	{"emc_2.5", slaveadd, VTR5, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 2.47},
	{"emc_vccp_1.25", slaveadd, Vccp, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 1.32},
	{"dme_cpu0_vid_1.5", slaveadd1, VID, {0.0}, {0.0}, {0.0}, 1.0, 98, 102, 1.4},
	{"emc_cpu1_vid_1.5", slaveadd, VID, {0.0}, {0.0}, {0.0}, 1.0, 98, 102, 1.4},
	{"emc_vcc_3.3", slaveadd, VCC, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 3.3},
	{"emc_12", slaveadd, V12, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 11.93},
	{"dme_sio_vtr_5.0", slaveadd1, VTR5, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 4.96},
	{"dme_sio_vccp_1.25", slaveadd1, Vccp, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 1.33},
	{"dme_sio_vcc_3.3", slaveadd1, VCC, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 3.3},
	{"dme_sio_5", slaveadd1, V5, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 5.0},
	{"dme_sio_12", slaveadd1, V12, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 11.93},
	{"dme_sio_vtr_3.3", slaveadd1, VTR, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 3.3},
	{"dme_vbat_2.7", slaveadd1, Vbat, {0.0}, {0.0}, {0.0}, 1.0, 95, 105, 2.7},
	{"dme_cpu0_fan", slaveadd1, FANTACH3_L, {0.0}, {0.0}, {0.0}, 1.0, 4000, 8000, 1.0},
	{"dme_cpu1_fan", slaveadd1, FANTACH2_L, {0.0}, {0.0}, {0.0}, 1.0, 4000, 8000, 1.0},
	{"dme_cpu0_temp", slaveadd1, SYS_TEM2, {0.0}, {0.0}, {0.0}, 1.0, 20, 60, 1.0},
	{"dme_cpu1_temp", slaveadd1, CPU_TEM, {0.0}, {0.0}, {0.0}, 1.0, 20, 60, 1.0},
	{"dme_system_fan1", slaveadd1, FANTACH1_L, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0},
	{"dme_system_fan2", slaveadd1, FANTACH4_L, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0},
	{"emc_system_fan3", slaveadd, FANTACH1_L, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0},
	{"emc_system_fan4", slaveadd, FANTACH2_L, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0},
	{"emc_system_fan5", slaveadd, FANTACH3_L, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0},
	{"emc_system_fan6", slaveadd, FANTACH4_L, {0.0}, {0.0}, {0.0}, 1.0, 2500, 6500, 1.0},
	{"emc_system_temp1", slaveadd, SYS_TEM1, {0.0}, {0.0}, {0.0}, 1.0, 10, 60, 1.0},
	{"emc_system_temp2", slaveadd, SYS_TEM2, {0.0}, {0.0}, {0.0}, 1.0, 10, 60, 1.0},
	{"emc_system_temp3", slaveadd, CPU_TEM, {0.0}, {0.0}, {0.0}, 1.0, 10, 60, 1.0},
	{"dme_system_temp4", slaveadd1, SYS_TEM1, {0.0}, {0.0}, {0.0}, 1.0, 10, 60, 1.0},
	{"", 0, 0, {0}, {0}, {0}, 0.0, 0, 0, 0.0}
};

int test_hwm(void)
{

	int temp = 0, temp1 = 0, temp2 = 0, temp3 = 0, speed = 0;;
	int diode_val=0, signbit=0;
	int type;

	type = i;

	temp = i2c_read_byte(tests[i].slave_addr, tests[i].addr);
	if(temp == 0xff)
	{
		printf("%-26s :  Function Error.\n", tests[i].name);
		return 1;
	}

	diode_val = (ReadThermState(i-15) >> 8) & 0x3F;
	signbit = (ReadThermState(i-15) >> 23) & 0x1;

	switch(type)
	{
		case EMC_VTR5:
			tests[i].get_val.fval = temp*(3.32/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case EMC_Vccp:
			tests[i].get_val.fval = temp*(3.00/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case DME_VID:
			tests[i].get_val.fval = 1.55-temp*0.025;
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case EMC_VID:
			tests[i].get_val.fval = 1.55-temp*0.025;
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case EMC_VCC:
			tests[i].get_val.fval = temp*(4.38/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case EMC_V12:
			tests[i].get_val.fval = temp*(15.93/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case DME_VTR5:
			tests[i].get_val.fval = temp*(6.64/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case DME_Vccp:
			tests[i].get_val.fval = temp*(3.0/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case DME_VCC:
			tests[i].get_val.fval = temp*(4.38/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case DME_V5:
			tests[i].get_val.fval = temp*(6.64/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case DME_V12:
			tests[i].get_val.fval = temp*(16.0/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case DME_VTR:
			tests[i].get_val.fval = temp*(4.38/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case DME_Vbat:
			tests[i].get_val.fval = temp*(4.38/255.0);
			tests[i].get_min.fval = (tests[i].stand_value) * (tests[i].min)/100;
			tests[i].get_max.fval = (tests[i].stand_value) * (tests[i].max)/100;
			if (((tests[i].get_val.fval) < (tests[i].get_min.fval)) || ((tests[i].get_val.fval) > (tests[i].get_max.fval)))
			{
				printf("%s Error: %4.3f V,  min %4.3f V,  max %4.3f V\n",
					tests[i].name, tests[i].get_val.fval, tests[i].get_min.fval, tests[i].get_max.fval);
				return 1;
			}
			printf("%-26s :  %f V\n", tests[i].name, tests[i].get_val.fval);
			break;
		case CPU0_FAN:
			temp3 = i2c_read_byte(tests[i].slave_addr, (tests[i].addr+1));
			speed = (temp3 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				printf("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case CPU1_FAN:
			temp3 = i2c_read_byte(tests[i].slave_addr, (tests[i].addr+1));
			speed = (temp3 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				printf("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case CPU0_Temp:
			if(signbit == 0)
				tests[i].get_val.ival = temp - diode_val;
			else
				tests[i].get_val.ival = temp + diode_val;
			if ((tests[i].get_val.ival < tests[i].min) || (tests[i].get_val.ival > tests[i].max))
				{
					printf("%s Error: %d, min %d max %d\n",
						tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
					return 1;
				}
			printf("%-26s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
			break;
		case CPU1_Temp:
			if(signbit == 0)
				tests[i].get_val.ival = temp - diode_val;
			else
				tests[i].get_val.ival = temp + diode_val;
			if ((tests[i].get_val.ival < tests[i].min) || (tests[i].get_val.ival > tests[i].max))
				{
					printf("%s Error: %d, min %d max %d\n",
						tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
					return 1;
				}
			printf("%-26s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
			break;
		case Sys_FAN1:
			temp3 = i2c_read_byte(tests[i].slave_addr, (tests[i].addr+1));
			speed = (temp3 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				printf("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case Sys_FAN2:
			temp3 = i2c_read_byte(tests[i].slave_addr, (tests[i].addr+1));
			speed = (temp3 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				printf("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case Sys_FAN3:
			temp3 = i2c_read_byte(tests[i].slave_addr, (tests[i].addr+1));
			speed = (temp3 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				printf("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case Sys_FAN4:
			temp3 = i2c_read_byte(tests[i].slave_addr, (tests[i].addr+1));
			speed = (temp3 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				printf("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case Sys_FAN5:
			temp3 = i2c_read_byte(tests[i].slave_addr, (tests[i].addr+1));
			speed = (temp3 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				printf("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case Sys_FAN6:
			temp3 = i2c_read_byte(tests[i].slave_addr, (tests[i].addr+1));
			speed = (temp3 << 8) + temp;
			tests[i].get_val.fval = (60.0/(speed * 11.111))*1000000.0;
			if ((((int)(tests[i].get_val.fval)) < (tests[i].min)) || (((int)(tests[i].get_val.fval)) > (tests[i].max)))
			{
				printf("%s speed Error: %d RPM,  min %d RPM,  max %d RPM\n",
					tests[i].name, (int)tests[i].get_val.fval, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d RPM\n", tests[i].name, (int)tests[i].get_val.fval);
			break;
		case Sys_Temp1:
			temp1 = i2c_read_byte (tests[i].slave_addr, SYS_TEM1_OFFSET);
			tests[i].get_val.ival = temp + temp1;
			if ((tests[i].get_val.ival < tests[i].min) || (tests[i].get_val.ival > tests[i].max))
			{
				printf("%s Error: %d, min %d max %d\n",
					tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
			break;
		case Sys_Temp2:
			temp2 = i2c_read_byte (tests[i].slave_addr, SYS_TEM2_OFFSET);
			tests[i].get_val.ival = temp + temp2;
			if ((tests[i].get_val.ival < tests[i].min) || (tests[i].get_val.ival > tests[i].max))
			{
				printf("%s Error: %d, min %d max %d\n",
					tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
			break;
		case Sys_Temp3:
			tests[i].get_val.ival = temp;
			if ((tests[i].get_val.ival < tests[i].min) || (tests[i].get_val.ival > tests[i].max))
			{
				printf("%s Error: %d, min %d max %d\n",
					tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
			break;
		case Sys_Temp4:
			tests[i].get_val.ival = temp;
			if ((tests[i].get_val.ival < tests[i].min) || (tests[i].get_val.ival > tests[i].max))
			{
				printf("%s Error: %d, min %d max %d\n",
					tests[i].name, tests[i].get_val.ival, tests[i].min, tests[i].max);
				return 1;
			}
			printf("%-26s :  %d C Degree\n", tests[i].name, tests[i].get_val.ival);
			break;
	}

	return 0;

}

void usage(void)
{
	printf("Usage: hwmodiag [-i test_item] [-L low_limit] [-H high_limit]\n");
	printf("  -i  Set item to test, eg.\n");
	printf("      (About Voltage)\n");
	printf("        emc_2.5   emc_vccp_1.25   dme_cpu0_vid_1.5   emc_cpu1_vid_1.5   emc_vcc_3.3   emc_12\n"); 
	printf("        dme_sio_vtr_5.0   dme_sio_vccp_1.25   dme_sio_vcc_3.3   dme_sio_5   dme_sio_12\n");
	printf("        dme_sio_vtr_3.3   dme_vbat_2.7\n");
	printf("      (About CPU FAN)\n");
	printf("        dme_cpu0_fan   dme_cpu1_fan\n");
        printf("      (About CPU Temperature)\n");
        printf("        dme_cpu0_temp   dme_cpu1_temp\n");
	printf("      (About System Fan)\n");
        printf("        dme_system_fan1   dme_system_fan2   emc_system_fan3   emc_system_fan4   emc_system_fan5   emc_system_fan6\n");
	printf("      (About System Temperature)\n");
	printf("        emc_system_temp1   emc_system_temp2   emc_system_temp3  emc_system_temp4\n");
//	printf("  -n  Set number of	cpu, this use for cpu test items\n");
	printf("  -L  Set low limit.\n");
	printf("  -H  Set high limit.\n");
	printf("  -v  Get version information.\n");
	printf("  -h  Get this message.\n");
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
					printf("hwmodiag for Sirius, Version 1.0 (c) 2005, MiTAC Inc.\n");
					return 0;
				case 'h':
					usage();
					return 0;
				case 'i':
					for(i=0; i<27; i++)
					{
						if (!(strncmp(optarg, tests[i].name, strlen(optarg))))
							break;
						else
							continue;
					}
					if(i>26)
					{
						printf("test item error, no such name!\n\n");
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
		printf("..........FAIL\n\n");
		printf("Can not get smbus addr\n\n");
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
		printf("Please input High/Low Limits!\n\n");
		usage();
		return 1;
	}
	if (ret)
	{
		printf("..........%s test FAIL\n", tests[i].name);
		return ret;
	}
	else
		printf("..........%s test PASS\n", tests[i].name);
	return ret;
}
