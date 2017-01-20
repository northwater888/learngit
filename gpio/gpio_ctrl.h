/* led_ctrl.h
 *
 * Author: zhang qiang <sunson.zhang@mic.com.tw>
 *
 * Copyright (c) 2008-, MiTAC Inc.
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This code is the led_ctrl module to test ledory.
 *
 *
 * 2008-06-03 made version 0.1
 *
 */
#ifndef _gpio_ctrl_H
#define _gpio_ctrl_H

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

#define GPIO_IOC_MAGIC 'L'
#define GPIO_I0C_QUERY _IOWR(GPIO_IOC_MAGIC,0,int)
#define GPIO_I0C_SET _IOWR(GPIO_IOC_MAGIC,1,int)
#define GPIO_IOC_MAXNR 1

struct gpio_ctrl_dev
{
	int    gpio_data;
	struct semaphore gpiosem;
	struct cdev gpio_set_dev;
};
int     set_led_data(unsigned long data);
int     led_ctrl_open(struct inode *inode,struct file *filp);
int     led_ctrl_release(struct inode *inode,struct file *filp);
int     led_ctrl_ioctl(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long arg);
void    led_ctrl_cleanup_module(void);
int     led_ctrl_init_module(void);

#endif

