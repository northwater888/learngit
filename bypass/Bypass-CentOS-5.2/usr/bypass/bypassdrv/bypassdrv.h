#ifndef _bypassdrv_H
#define _bypassdrv_H

#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>//printk()
#include<linux/slab.h>//kmalloc()
#include<linux/seq_file.h>//cdev
#include<linux/cdev.h>//cdev
#include<linux/types.h>//size_t
#include<linux/fs.h>
#include<linux/vmalloc.h>
#include <linux/unistd.h>
#include<linux/pci.h>
#include<linux/ioctl.h>//_IOWR
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include<asm/uaccess.h>//copy_to_user
#include<asm/page.h>

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

#define BYPASS_IOC_MAXNR 20
#define ULONG unsigned long
#define UCHAR unsigned char

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


#pragma pack(1)
struct bypass_dev
{
	int    bypass_data;
	//u16    wReg[20];
	//u8     cRegData[20];	
	struct semaphore bypasssem;
	struct cdev bypass_set_dev;
	spinlock_t lock;
	struct timer_list timer;
//	u16*   pwReg;
//	u8*    pcRegData;
	
};

struct bypass_data {
	struct i2c_client client;
	//struct i2c_client *lm75[2];
	struct class_device *bypassdrv_dev;
	struct mutex update_lock;
	char valid;			/* !=0 if following fields are valid */
	unsigned long last_updated;	/* In jiffies */
	unsigned long last_nonvolatile;	/* In jiffies, last time we update the
					   nonvolatile registers */
};

#pragma pack()

int     bypass_open(struct inode *inode,struct file *filp);
int     bypass_release(struct inode *inode,struct file *filp);
int     bypass_ioctl(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long arg);
//void	bypass_init(void);
//void    bypass_exit(void);
//void     bypass_init_module(void);
//void    bypass_cleanup_module(void);

#endif
