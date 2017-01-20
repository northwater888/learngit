/* gpio_ctrl.c
 *
 * Author: zhang qiang <sunson.zhang@mic.com.tw>
 *
 * Copyright (c) 2008-, MiTAC Inc.
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This code is the gpio_ctrl module to test gpioory.
 *
 *
 */
#include"gpio_ctrl.h"

int gpio_ctrl_major=0;
int gpio_ctrl_minor=0;
struct gpio_ctrl_dev *ldev;

#include <asm/irq.h>

static DEFINE_SPINLOCK( status_lock );

/*open function*/
int gpio_ctrl_open(struct inode *inode,struct file *filp)
{
	struct gpio_ctrl_dev *ldev;
	ldev=container_of(inode->i_cdev,struct gpio_ctrl_dev,gpio_set_dev);
	filp->private_data=ldev;
	
	if((filp->f_flags&O_ACCMODE) == O_WRONLY)
	{
		if(down_interruptible(&ldev->gpiosem))
			return -ERESTARTSYS;
		memset(&ldev->gpio_data,0,sizeof(int));
		up(&ldev->gpiosem);
	}
	return 0;
}

int gpio_ctrl_release(struct inode *inode,struct file *filp)
{
	return 0;
}


int set_gpio_data(unsigned long data)
{
   unsigned long flags;
    char value;
    spin_lock_irqsave( &status_lock, flags );

    value = inl( 0x538 );

    spin_unlock_irqrestore( &status_lock, flags );
    
    if(data==0)
    	value&=0xfb;
    else
    	value|=0x4;
     
     printk("write data is:%x\n",value);
    
    spin_lock_irqsave( &status_lock, flags );

    
    //outb(0x2e,0x538);
    outb(value,0x538);

    spin_unlock_irqrestore( &status_lock, flags );
    
  
   return value;

}

int get_gpio_data(unsigned long data)
{
   unsigned long flags;
    char value;
    spin_lock_irqsave( &status_lock, flags );

    value = inl( 0x538 );

    spin_unlock_irqrestore( &status_lock, flags );
    
    printk("read  GPIO34 (538):%x\n",value);
    
  
   return value;

}


int gpio_ctrl_ioctl(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long arg)
{
	int err=0,ret=0;
	
	if(_IOC_TYPE(cmd)!=GPIO_IOC_MAGIC) 
		return -ENOTTY;
	if(_IOC_NR(cmd)>GPIO_IOC_MAXNR)
		return -ENOTTY;
	
	if(_IOC_DIR(cmd)&_IOC_READ)
		err=!access_ok(VERIFY_WRITE,(void __user*)arg,_IOC_SIZE(cmd));
	if(_IOC_DIR(cmd)&_IOC_WRITE)
		err=!access_ok(VERIFY_READ,(void __user*)arg,_IOC_SIZE(cmd));
		
	if(err!=0)
		return -EFAULT;
	
	switch(cmd)
	{
		case GPIO_I0C_SET:
			ret=set_gpio_data(arg);//set the register
			break;
		case GPIO_I0C_QUERY:
			ret=get_gpio_data(0);//set the register
			break;
		
		default:
			return -ENOTTY;
	}
	return ret;
}


struct file_operations fops=
{
	.owner=			THIS_MODULE,
	.ioctl=			gpio_ctrl_ioctl,
	.open=			gpio_ctrl_open,
	.release=		gpio_ctrl_release,
};

void gpio_ctrl_cleanup_module(void)
{
	dev_t devno=MKDEV(gpio_ctrl_major,gpio_ctrl_minor);
	
	if(ldev)
	{
		memset(ldev,0,sizeof(ldev));
		kfree(ldev);
		cdev_del(&ldev->gpio_set_dev);
	}
	unregister_chrdev_region(devno,1);
}	

int gpio_ctrl_init_module(void)
{
	int result,err;
	dev_t devno=0;
	
	if(gpio_ctrl_major!=0)
	{
		devno=MKDEV(gpio_ctrl_major,gpio_ctrl_minor);
		result=register_chrdev_region(devno,1,"gpio_ctrl");
	}
	else
	{
		result=alloc_chrdev_region(&devno,gpio_ctrl_minor,1,"gpio_ctrl");
		gpio_ctrl_major=MAJOR(devno);
	}
	printk("zzzzzzm %d\nzzzzzzn  %d\n",gpio_ctrl_major,gpio_ctrl_minor);

	if(result<0) 
	{
		printk(KERN_WARNING"gpio_ctrl can't get major:%d",gpio_ctrl_major);
		return result;
	}
	
	ldev=(struct gpio_ctrl_dev *)kmalloc(sizeof(struct gpio_ctrl_dev),GFP_KERNEL);
	if(ldev==NULL)
	{
		result=-ENOMEM;
		goto fail;
	}
	memset(ldev,0,sizeof(struct gpio_ctrl_dev));
		
	init_MUTEX(&ldev->gpiosem);
	devno=MKDEV(gpio_ctrl_major,gpio_ctrl_minor);
	cdev_init(&ldev->gpio_set_dev,&fops);
	ldev->gpio_set_dev.owner=THIS_MODULE;
	ldev->gpio_set_dev.ops=&fops;
	
	err=cdev_add(&ldev->gpio_set_dev,devno,1);
	if(err)
	{
		printk(KERN_NOTICE"Error %d when add gpio_ctrldev",err);
		return -1;
	}
		
	return 0;
	
	fail:
		gpio_ctrl_cleanup_module();
		return result;
}


MODULE_AUTHOR("ZHANGQIANG");
MODULE_LICENSE("GPL");

module_init(gpio_ctrl_init_module);
module_exit(gpio_ctrl_cleanup_module);

