#ifndef _w83793_H
#define _w83793_H

#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>//printk()
#include<linux/slab.h>//kmalloc()
#include<linux/seq_file.h>//cdev
#include<linux/cdev.h>//cdev
#include<linux/types.h>//size_t
#include<linux/fs.h>//
#include<asm/page.h>
#include<linux/vmalloc.h>
#include<linux/ioctl.h>//_IOWR*
#include<asm/uaccess.h>//copy_to_user
#include <linux/pci.h>

#define HWM_IOC_MAGIC 'H'

#define HWM_I0C_QUERY _IOWR(HWM_IOC_MAGIC,0,int)
#define HWM_I0C_SET _IOWR(HWM_IOC_MAGIC,1,int)
//#define HWM_I0C_QUERY _IOWR(HWM_IOC_MAGIC,1,int)
//#define HWM_I0C_QUERY _IOWR(HWM_IOC_MAGIC,1,int)
//#define HWM_I0C_QUERY _IOWR(HWM_IOC_MAGIC,1,int)
//#define HWM_I0C_QUERY _IOWR(HWM_IOC_MAGIC,1,int)
//#define HWM_I0C_QUERY _IOWR(HWM_IOC_MAGIC,1,int)
//#define HWM_I0C_QUERY _IOWR(HWM_IOC_MAGIC,1,int)






#define HWM_IOC_MAXNR 1

struct hwm_dev
{
	int    hwm_data;
	struct semaphore hwmsem;
	struct cdev hwm_set_dev;
};

int     hwm_open(struct inode *inode,struct file *filp);
int     hwm_release(struct inode *inode,struct file *filp);
int     hwm_ioctl(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long arg);
void    hwm_cleanup_module(void);
int     hwm_init_module(void);

#endif
