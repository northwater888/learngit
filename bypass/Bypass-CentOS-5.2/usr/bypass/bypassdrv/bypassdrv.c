#include "bypassdrv.h"

//define Bypass slave address
#define ByPassDevice_SLAVE_ADDR      0x26 

#define R6_Write_Data                0x01

#define VERSION                      0x00
#define Time_out                     0x02
#define WD_Control_Status            0x03
#define PTC_Control_Status           0x04
#define R6_Control_Status           0x06
#define R6_Data			            0x07

int bypass_major=243;
int bypass_minor=0;
struct bypass_dev *bypass_dev=NULL;

//struct device *golbaldev=NULL;
//define structure to store different client (up to 8)
#define MAX_CLIENT_NUMBER 8
typedef struct i2c_client  i2cclient;
i2cclient client_array[MAX_CLIENT_NUMBER];
//static DEFINE_SPINLOCK(client_array_lock);
int client_number = 0;

static  UCHAR ByPass_Mode;
static	UCHAR Time_outvalue;
static  UCHAR POvalue;
static  UCHAR S5value;

/* Addresses to scan */
static unsigned short normal_i2c[] = { 0x25,0x26,0x27,0x24,0x23,0x22,0x21,0x20,
						I2C_CLIENT_END };

/* Insmod parameters */
I2C_CLIENT_INSMOD_1(bypassdrv);
static int bypass_attach_adapter(struct i2c_adapter *adapter);
static int bypass_detect(struct i2c_adapter *adapter, int address, int kind);
static int bypass_detach_client(i2cclient *client);

static struct i2c_driver bypass_driver = {
	.driver = {
		   .name = "bypassdrv",
	},
	.attach_adapter = bypass_attach_adapter,
	.detach_client = bypass_detach_client,
};

static int bypass_attach_adapter(struct i2c_adapter *adapter)
{
	//printk("==> In bypass_attach_adapter:\n");
	//if (!(adapter->class & I2C_CLASS_HWMON))
	//	return 0;
	return i2c_probe(adapter, &addr_data, bypass_detect);
}

static int bypass_detach_client(i2cclient *client)
{

	struct bypass_data *data = i2c_get_clientdata(client);
//	struct device *dev = &client->dev;
	int err;
	//printk("==> In bypass_attach_adapter:\n");
    
	/* main client */
	
	if ((err = i2c_detach_client(client)))
		return err;

	/* main client */
	if (data)
		kfree(data);
	/* subclient */
	else
		kfree(client);

	return 0;
}

static int bypass_detect(struct i2c_adapter *adapter, int address, int kind)
{
	i2cclient *client;
	struct bypass_data *data;
  int err = 0;
  UCHAR value1;
	UCHAR value2;
	UCHAR value3;
  //printk("==>In bypass_detect: \n"); 
	
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		goto exit;
	}
	//printk("In bypass_detect: \n I2C adapter function is OK...client_number is %d\n",client_number);
	
	/* OK. For now, we presume we have a valid client. We now create the
	   client structure, even though we cannot fill it completely yet.
	   But it allows us to access bypass_{read,write}_value. */

	if (!(data = kzalloc(sizeof(struct bypass_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}

	client = &data->client;
	i2c_set_clientdata(client, data);
	client->addr = address;
	client->adapter = adapter;
	client->driver = &bypass_driver;
	// Fill in the remaining client fields and put into the global list 
	strlcpy(client->name, "bypass", I2C_NAME_SIZE);

	// Tell the I2C layer a new client has arrived 
	if ((err = i2c_attach_client(client)))
	{	
		goto free_mem;
	}
	else
	{	
	  
	  value1 = i2c_smbus_read_byte_data (client, 16);
	  value2 = i2c_smbus_read_byte_data (client, 21);
	  value3 = i2c_smbus_read_byte_data (client, 31);
	  
	  if(value1==0x4d&&value2==0x50&&value3==0x43)
		{
		 // printk("In bypass_detect: \n One client detected and current client number is %d",client_number);
		  memcpy(client_array+client_number, client, sizeof(*client));
		  client_number++;
		  return 0;
	  }
	  else return -1;
	
	}
free_mem:
	kfree(data);
exit:
	return err;
}


int bypass_open(struct inode *inode,struct file *filp)
{
	// save dev entension structure into private space
	struct bypass_dev *bypass_dev;
	bypass_dev=container_of(inode->i_cdev,struct bypass_dev,bypass_set_dev);
	filp->private_data=bypass_dev;

	//这个宏作为一个掩码以与文件状态标识值做AND位运算，产生一个表示文件访问模式的值。这模式将是O_RDONLY, O_WRONLY, 或 O_RDWR（在GNU系统中，也可能是零，并且从不包括 O_EXEC 位）
	//O_ACCMODE<0003>：读写文件操作时，用于取出flag的低2位
	//O_RDONLY<00>：只读打开
	//O_WRONLY<01>：只写打开
	//O_RDWR<02>：读写打开 
//	if((filp->f_flags&O_ACCMODE) == O_WRONLY)
//	{
//		if(down_interruptible(&bypass_dev->bypasssem))
//			return -ERESTARTSYS;
//		memset(&bypass_dev->bypass_data,0,sizeof(int));
//		up(&bypass_dev->bypasssem);
//	}


	return 0;
}

int bypass_release(struct inode *inode,struct file *filp)
{
	return 0;
}

int retryread(i2cclient *client,UCHAR index)
{
  int i,wb,wa;
  int ret;
  if(index!=2)
  {  
    for(i=0;i<3;i++)
    {
      ret = i2c_smbus_read_byte_data (client, index);
      if(ret == -1)
      {
        //usleep(50+i*10);
        wb = jiffies;
        while(1)
        {
          wa = jiffies;
          if(wa-wb>=HZ/20)
            break;         
        }
        
        continue;
      }  
      else break;
    }
  }
  else
  {
    ret = i2c_smbus_read_byte_data (client, index);
  }
      return ret;  
}

int retrywrite(i2cclient *client,UCHAR index,UCHAR data)
{
  int i,wb,wa;
  int ret;
  for(i=0;i<5;i++)
  {
    ret = i2c_smbus_write_byte_data (client, index, data);
    if(ret == -1)
    {
       wb = jiffies;
        while(1)
        {
          wa = jiffies;
          if(wa-wb>=HZ/20)
            break;         
        }//usleep(50+i*10);
      continue;
    }  
    else break;
  }
  
  return ret;  
}

//get all data
UCHAR getalldata(i2cclient *client,ALL_SensorData *sensordata)
{
  UCHAR VERSIONVALUEEX,Time_outVALUEEX,WD_Control_StatusVALUEEX,PTC_Control_StatusVALUEEX,S5VALUEEX,S5VALUEEXEX;
	UCHAR POEX,POEXEX;
	int i;
 // printk("==>getalldata\n");
  //if(client->addr==NULL) return 0;
	
 	VERSIONVALUEEX =  retryread (client, VERSION);
  Time_outVALUEEX = retryread (client, Time_out);
	WD_Control_StatusVALUEEX = retryread (client, WD_Control_Status);

	//retrywrite (client, WD_Control_Status,WD_Control_StatusVALUEEX|0x80);

	WD_Control_StatusVALUEEX = retryread (client, WD_Control_Status);
	PTC_Control_StatusVALUEEX  = retryread (client, PTC_Control_Status);
	//printk("PTCIS:%x,WDCIS:%x,Timeoutis:%x\n",PTC_Control_StatusVALUEEX,WD_Control_StatusVALUEEX,Time_outVALUEEX );

	retrywrite (client, R6_Data,0x00);
	retrywrite (client, R6_Control_Status,0x30);
	S5VALUEEXEX=retryread (client, R6_Data);

	S5VALUEEX=100;
	if((S5VALUEEXEX & 0x20)==0x20)
		S5VALUEEX=0;
	else
	{
		if(((S5VALUEEXEX & 0x10)==0x10)&&((S5VALUEEXEX & 0x08)==0))
			S5VALUEEX=1;
		if(((S5VALUEEXEX & 0x10)==0)&&((S5VALUEEXEX & 0x08)==0x08))
			S5VALUEEX=2;
	}

	retrywrite (client, R6_Data,0x00);
	retrywrite (client, R6_Control_Status,0x31);
	POEXEX=retryread (client, R6_Data);

	POEX=100;
	if((POEXEX & 0x20)==0x20)
		POEX=0;
	else
	{
		if(((POEXEX & 0x10)==0x10)&&((POEXEX & 0x08)==0))
			POEX=1;
		if(((POEXEX & 0x10)==0)&&((POEXEX & 0x08)==0x08))
			POEX=2;
	}
	
  for(i=0;i<6;i++)
  {
     sensordata->HardWVersion[i]=retryread (client, 21+i);
  }
  


  sensordata->VERSIONvalue = VERSIONVALUEEX;
  sensordata->Time_outvalue = Time_outVALUEEX;
  sensordata->WD_Control_Statusvalue  = WD_Control_StatusVALUEEX;
  sensordata->PTC_Control_Statusvalue = PTC_Control_StatusVALUEEX;
  sensordata->s5_status = S5VALUEEX;
  sensordata->poweron_status = POEX;
 // printk("timeout:%x,s5:%x,poweron:%x\n",sensordata->Time_outvalue,sensordata->s5_status,sensordata->poweron_status);
  return 0;
}

UCHAR setnormal(i2cclient *client)
{
	UCHAR PTC_Control_StatusVALUEEXEX,PTC_Control_StatusVALUEEX;
  //printk("==>setnormal\n");
	//if(client->addr==NULL) return 0;
	
	PTC_Control_StatusVALUEEX=retryread (client, PTC_Control_Status);

	if((PTC_Control_StatusVALUEEX & 0x20)==0x20)
			return 1;
	else
	{
		PTC_Control_StatusVALUEEXEX=PTC_Control_StatusVALUEEX | 0x20;
	
		retrywrite (client, PTC_Control_Status,PTC_Control_StatusVALUEEXEX);
	}

	return 0;
}


UCHAR setforce(i2cclient *client,UCHAR index)
{
  UCHAR PTC_Control_StatusVALUEEXEX,PTC_Control_StatusVALUEEX;
  UCHAR WD_Control_StatusVALUEEX;
  //printk("==> setforce\n");
	//if(client->addr==NULL) return 0;
	
	retrywrite (client, Time_out,0);
  WD_Control_StatusVALUEEX=retryread (client, WD_Control_Status);
	if((WD_Control_StatusVALUEEX&0x10)==0x10)
		retrywrite (client, WD_Control_Status,WD_Control_StatusVALUEEX&0xef);

	PTC_Control_StatusVALUEEX=retryread (client, PTC_Control_Status);
	if(index==2)
	{ 
	  if(((PTC_Control_StatusVALUEEX & 0x20)==0)&&((PTC_Control_StatusVALUEEX & 0x10)==0)&&((PTC_Control_StatusVALUEEX & 0x08)==0x08))
			return 1;
		else
		{
			PTC_Control_StatusVALUEEXEX=(PTC_Control_StatusVALUEEX & 0xcf) | 0x08;
			
			retrywrite (client, PTC_Control_Status,PTC_Control_StatusVALUEEXEX);
		}
		//printk("setforce BYPASS success!\n");
	}
	if(index==0)
	{
		if((PTC_Control_StatusVALUEEX & 0x20)==0x20)
			return 1;
		else
		{
			PTC_Control_StatusVALUEEXEX=PTC_Control_StatusVALUEEX | 0x20;
			
			retrywrite (client, PTC_Control_Status,PTC_Control_StatusVALUEEXEX);
		}
    //printk("setforce NORMAL success!\n");
	}
	if(index==1)
	{
	  //printk("indexis:%d\n",index);
		if(((PTC_Control_StatusVALUEEX & 0x20)==0)&&((PTC_Control_StatusVALUEEX & 0x10)==0x10)&&((PTC_Control_StatusVALUEEX & 0x08)==0))
			return 1;
		else
		{
			PTC_Control_StatusVALUEEXEX=(PTC_Control_StatusVALUEEX & 0xd7) | 0x10;
			
			retrywrite (client, PTC_Control_Status,PTC_Control_StatusVALUEEXEX);
		}
		//printk("ptcis:%x\n",PTC_Control_StatusVALUEEX);
		
		//printk("setforce DISCONNECT success!\n");
	}
	//printk("setforce success!\n");
	return 0;
}

UCHAR set_all_data(i2cclient *client,ALL_SensorData *sensordata)
{
  UCHAR Time_outVALUEEX,WDFlag,PTC_Control_StatusVALUEEX;
	UCHAR S5VALUEEXEX;
	UCHAR POEXEX;
	UCHAR driverload_status;
	int wb,wa;
//	printk("==>setalldata\n");

//#if 0
	WDFlag = retryread(client, WD_Control_Status);
	//printk("1WDFlagis:%x\n",WDFlag);
  while((WDFlag&0x80)!=0x80)
  {
    retrywrite(client,WD_Control_Status,WDFlag|0x80);
    WDFlag = retryread(client, WD_Control_Status);
  }
	
	wb = jiffies;
  while(1)
  {
    wa = jiffies;
    if(wa-wb>=HZ*2)
      break;         
  }
	
	//setnormal(client);
	Time_outVALUEEX=0;
	retrywrite(client,Time_out,Time_outVALUEEX);
	Time_outVALUEEX = retryread(client, Time_out);
	//printk("timeoutis:%d\n",Time_outVALUEEX);
	WDFlag = retryread(client, WD_Control_Status);
	retrywrite(client,WD_Control_Status,WDFlag&0xef); 
	
	driverload_status = sensordata->driverload_status;
	setforce(client,driverload_status);
		
	//printk("==>setalldataclear\n");
	  
  Time_outvalue = sensordata->Time_outvalue;
	PTC_Control_StatusVALUEEX = sensordata->PTC_Control_Statusvalue;
	S5value = sensordata->s5_status;
	POvalue= sensordata->poweron_status;
	ByPass_Mode = sensordata->ByPass_Mode;

	
	retrywrite (client, R6_Data,0x00);
	retrywrite (client, R6_Control_Status,0x30);
	
	S5VALUEEXEX=retryread (client, R6_Data);
	if(S5value==0)
	{
		if(((S5VALUEEXEX & 0x20)==0x20))
			;
		else
		{
			retrywrite (client, R6_Write_Data,0x20);
			retrywrite (client, R6_Control_Status,0x20);
		}
	}

	if(S5value==1)
	{
  	if(((S5VALUEEXEX & 0x20)==0)&&((S5VALUEEXEX & 0x10)==0x10)&&((S5VALUEEXEX & 0x08)==0))
			;
		else
		{
			retrywrite (client, R6_Write_Data,0x10);
			retrywrite (client, R6_Control_Status,0x20);
		}
	}

	if(S5value==2)
	{
		if(((S5VALUEEXEX & 0x20)==0)&&((S5VALUEEXEX & 0x10)==0)&&((S5VALUEEXEX & 0x08)==0x08))
			;
		else
		{
			retrywrite (client, R6_Write_Data,0x08);
			retrywrite (client, R6_Control_Status,0x20);
		}
	}

	retrywrite (client, R6_Data,0x00);
	retrywrite (client, R6_Control_Status,0x31);
	POEXEX=retryread (client, R6_Data);
	//printk("POEXEX:%x,\n",POEXEX);
	if(POvalue==0)
	{
		if(((POEXEX & 0x20)==0x20))
			;
		else
		{
			retrywrite (client, R6_Write_Data,0x20);
			retrywrite (client, R6_Control_Status,0x21);
		}
	}
	if(POvalue==1)
	{
		if(((POEXEX & 0x20)==0)&&((POEXEX & 0x10)==0x10)&&((POEXEX & 0x08)==0))
				  ;
		else
		{
		  retrywrite (client, R6_Write_Data,0x10);
			retrywrite (client, R6_Control_Status,0x21);
	  }
	}

	if(POvalue==2)
	{
	 if(((POEXEX & 0x20)==0)&&((POEXEX & 0x10)==0)&&((POEXEX & 0x08)==0x08))
		;
		else
		{
			retrywrite (client, R6_Write_Data,0x08);
			retrywrite (client, R6_Control_Status,0x21);
		}
	}
	
	Time_outVALUEEX = retryread(client, Time_out);
	if(Time_outvalue==0)
	  ;
	else
	{
	  if(driverload_status==0)
		{
      if(ByPass_Mode==1)
      {    		 
		    Time_outVALUEEX=Time_outvalue;
		    retrywrite(client,Time_out,Time_outVALUEEX);
		  }
		}
		else 
		retrywrite(client,Time_out,0);
  }
	
//#endif
	return 0;
}

UCHAR poweroffset(void)
{
  i2cclient *client;
  int index;
  UCHAR WD_Control_StatusVALUEEX;
  //printk("poweroffset\n");
	//if(client->addr==NULL) return 0;
	for(index=0;index<client_number;index++)
	{
	  client = &client_array[index];
		//printk("addressis:%x\n",client_array[index].addr);
		WD_Control_StatusVALUEEX = retryread (client, WD_Control_Status);
	  retrywrite (client, WD_Control_Status,WD_Control_StatusVALUEEX&0x7f);
    WD_Control_StatusVALUEEX = retryread (client, WD_Control_Status);
	  //printk("poweroffsetok%x!!\n",WD_Control_StatusVALUEEX);
	}  
	
	return 0;
}



UCHAR setsignal(i2cclient *client)
{
	UCHAR WD_Control_StatusVALUEEX;
  //printk("setsignal\n");
	//if(client->addr==NULL) return 0;
	
	WD_Control_StatusVALUEEX = retryread (client, WD_Control_Status);
	retrywrite (client, WD_Control_Status,WD_Control_StatusVALUEEX|0x20);
	WD_Control_StatusVALUEEX = retryread (client, WD_Control_Status);
	return 0;
}

int bypass_ioctl(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long arg)
{
	
	//printk("==>ioctlsais:\n");
  int err=0,ret=0,sa[8];
  ALL_SensorData sensordata,sensordata1;
	int index;
	unsigned short address;
	void __user *argp=(void __user *)arg;
  i2cclient *client=NULL;
  struct semaphore iosem;


  if(_IOC_DIR(cmd)&_IOC_READ)
		err=!access_ok(VERIFY_WRITE,(void __user*)arg,_IOC_SIZE(cmd));
	if(_IOC_DIR(cmd)&_IOC_WRITE)
		err=!access_ok(VERIFY_READ,(void __user*)arg,_IOC_SIZE(cmd));
	
	if(err!=0)
    return -EFAULT;
  
  if (copy_from_user((caddr_t)&sensordata, (caddr_t)arg, sizeof(sensordata)) == -1) 
	{
		printk(KERN_ERR "ioctl: copy_from_user failed\n");
	  return(-EFAULT);
	}
	init_MUTEX(&iosem);
	
  memcpy(&sensordata1,&sensordata,sizeof(sensordata));
	address = sensordata1.addr;
	if(address==0)
	address = 0x26;
	//printk("sais:%x\n",address);
	for(index=0;index<8;index++)
	{
		if(client_array[index].addr == address)
		client = &client_array[index];
		sa[index] = client_array[index].addr;
	}  
	
	if(down_interruptible(&iosem))
			return -ERESTARTSYS;
	  switch(cmd)
	{
    case IOCTL_BYPASS_GET_CLIENT_ADDRESS:
      if (copy_to_user(argp, sa, sizeof(int)*8))
        ret = -EFAULT;
      break;
    case IOCTL_BYPASS_GET_DATA:
			ret = getalldata(client,&sensordata1);
			if (copy_to_user(argp, &sensordata1, sizeof(ALL_SensorData)))
				ret = -EFAULT;
			break;
		case IOCTL_BYPASS_SET_DATA:
			ret = set_all_data(client,&sensordata1);
			break;
		case IOCTL_BYPASS_FORCE_BYPASS:
		  ret = setforce(client,2);
			break;
		case IOCTL_BYPASS_FORCE_NORMAL:
		  ret = setforce(client,0);
			break;
		case IOCTL_BYPASS_FORCE_DISCONNECT:
		  ret = setforce(client,1);
			break;
		case IOCTL_BYPASS_SET_NORMAL:
		  ret = setnormal(client);
			break;
		case IOCTL_BYPASS_SET_SIGNAL:
		  ret = setsignal(client);
			break;
	 case IOCTL_BYPASS_POWEROFF_SET:
	    //printk("iopoweroff\n");
		  ret = poweroffset();
			break;
		default:
			ret = -ENOTTY;
			break;
	}
	up(&iosem);

	return ret;
}

struct file_operations fops=
{
	.owner=			THIS_MODULE,
	.ioctl=			bypass_ioctl,
	.open=			bypass_open,
	.release=		bypass_release,
};

static void __exit bypass_exit(void)
{
	i2c_del_driver(&bypass_driver);				// Remove driver
	cdev_del(&bypass_dev->bypass_set_dev);		// Free cdev 
	kfree(bypass_dev);	
	unregister_chrdev_region(MKDEV(bypass_major,bypass_minor),1);   // Release char device 
}

static int __init bypass_init(void)
{
  
	int result,err;
	dev_t devno=0;
    i2c_add_driver(&bypass_driver);
	if(bypass_major!=0)
	{
		devno=MKDEV(bypass_major,bypass_minor);
		result=register_chrdev_region(devno,1,"bypassdrv");
	}
	else
	{
		result=alloc_chrdev_region(&devno,bypass_minor,1,"bypassdrv");
		bypass_major=MAJOR(devno);
	}
	//printk("In bypass_init:\n bypass_major       %d\n bypassminor         %d\n",bypass_major,bypass_minor);
	if(result<0) 
	{
		printk(KERN_WARNING"bypass can't get major:%d",bypass_major);
		return result;
	}
	// Apply memory for device structuere
	bypass_dev=(struct bypass_dev *)kmalloc(sizeof(struct bypass_dev),GFP_KERNEL);
	if(bypass_dev==NULL)
	{
		result=-ENOMEM;
		goto fail;
	}
	memset(bypass_dev,0,sizeof(struct bypass_dev));
	
	// Initialize and register the device
//	init_MUTEX(&bypass_dev->bypasssem);
	devno=MKDEV(bypass_major,bypass_minor);
	cdev_init(&bypass_dev->bypass_set_dev,&fops);
	bypass_dev->bypass_set_dev.owner=THIS_MODULE;
	bypass_dev->bypass_set_dev.ops=&fops;
    spin_lock_init(&bypass_dev->lock);
	err=cdev_add(&bypass_dev->bypass_set_dev,devno,1);
	if(err)
	{
		printk(KERN_NOTICE"Error %d when add passbydev",err);
		return -1;
	}
	fail:
		//bypass_exit();
		return result;
	return 0;
}

MODULE_AUTHOR("Danny");
MODULE_DESCRIPTION("bypass driver");
MODULE_LICENSE("GPL");

module_init(bypass_init);
module_exit(bypass_exit);
