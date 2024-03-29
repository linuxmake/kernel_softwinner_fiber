/*
 *  afa750.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor 
 *
 *  Copyright (C) 2009-2010 Freescale Semiconductor Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <linux/device.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif

#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <mach/gpio.h> 

/*
 * Defines
 */
#define assert(expr)\
	if (!(expr)) {\
		printk(KERN_ERR "Assertion failed! %s,%d,%s,%s\n",\
			__FILE__, __LINE__, __func__, #expr);\
	}

#define AFA750_DRV_NAME         "afa750"
#define SENSOR_NAME             AFA750_DRV_NAME

#define POLL_INTERVAL_MAX       500
#define POLL_INTERVAL           50


#define AFA_FULLRES_MAX_VAL     32767 
#define AFA_FULLRES_MIN_VAL     32768 

#define DATAX0                  0x10	/* R   X-Axis Data 0 */
#define DATAX1                  0x11	/* R   X-Axis Data 1 */
#define DATAY0                  0x12	/* R   Y-Axis Data 0 */
#define DATAY1                  0x13	/* R   Y-Axis Data 1 */
#define DATAZ0                  0x14	/* R   Z-Axis Data 0 */
#define DATAZ1                  0x15	/* R   Z-Axis Data 1 */

#define WHO_AM_I                0x37
#define WHO_AM_I_VALUE1         60
#define WHO_AM_I_VALUE2         61

#define MODE_CHANGE_DELAY_MS    100

#define POWER_CTL	0x03	/* R/W Power saving features control :Frances*/
#define POWER_DOWN  0x02
#define WAKE_UP     0x00
static struct device *hwmon_dev;
static struct i2c_client *afa750_i2c_client;

struct afa750_data_s {
        bool suspended;
        struct i2c_client       *client;
        struct input_polled_dev *pollDev; 
        struct mutex interval_mutex; 
    
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	volatile int suspend_indator;
#endif
} afa750_data;

static struct input_polled_dev *afa750_idev;
enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,	
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 0;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk(KERN_DEBUG fmt , ## arg)

/* Addresses to scan */
#define  AFASENSOR_I2C_ADDR0  0x3c
#define  AFASENSOR_I2C_ADDR1  0x3d
static const unsigned short normal_i2c[2] = {0x3c,I2C_CLIENT_END};
static const unsigned short i2c_address[2] = {AFASENSOR_I2C_ADDR0,AFASENSOR_I2C_ADDR1};
static __u32 twi_id = 0;
static int i2c_num = 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void afa750_early_suspend(struct early_suspend *h);
static void afa750_late_resume(struct early_suspend *h);
#endif


/**
 * gsensor_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int gsensor_fetch_sysconfig_para(void)
{
	int ret = -1;
	int device_used = -1;
	script_item_u	val;
	script_item_value_type_e  type;
		
	dprintk(DEBUG_INIT, "========%s===================\n", __func__);

	type = script_get_item("gsensor_para", "gsensor_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err device_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	device_used = val.val;
	
	if (1 == device_used) {
		type = script_get_item("gsensor_para", "gsensor_twi_id", &val);	
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err twi_id = %d. \n", __func__, val.val);
			goto script_get_err;
		}
		twi_id = val.val;
		
		dprintk(DEBUG_INIT, "%s: twi_id is %d. \n", __func__, twi_id);
		ret = 0;
		
	} else {
		pr_err("%s: gsensor_unused. \n",  __func__);
		ret = -1;
	}

	return ret;

script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;
}
/**
 * gsensor_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int gsensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret = -1;
	int i = 0;
	unsigned char buf[2] = {0x7e,0x7f};

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV;
	
	if(twi_id == adapter->nr)
	{
		pr_info("Begin gsensor i2c test\n");
		while(i < 2) 
		{
            client->addr = buf[i++];
			ret = i2c_smbus_write_byte_data(client, 0x36,0x01);	
			mdelay(10);
			if(ret == 0)
				break;
		}	
		
		while(i2c_num < 2) 
		{
            client->addr = i2c_address[i2c_num++];
			ret = i2c_smbus_read_byte_data(client,WHO_AM_I);
			if(ret < 0)
			{
				continue;
			}
	   	    printk("IIC connect Success! IIC addr %02x, Read ID value is: 0x%x\n",i2c_address[i2c_num-1], ret);
	        if (((ret &0x00FF) == WHO_AM_I_VALUE1)|| ((ret &0x00FF) == WHO_AM_I_VALUE2)) {
	                printk("afa750 Sensortec Device detected!\n" );
    			strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);
                        return 0; 
	    
	        }else{
	                printk("afa750 not found!\n" );
	                return -ENODEV;
	        }
		}
	}else{
		return -ENODEV;
	}
}


static ssize_t afa750_enable_store(struct device *dev,struct device_attribute *attr,
		                  const char *buf, size_t count)
{
	unsigned long data;
	int error;
    
	error = strict_strtoul(buf, 10, &data);
	
	if(error) {
		pr_err("%s strict_strtoul error\n", __FUNCTION__);
		goto exit;
	}

	dprintk(DEBUG_CONTROL_INFO, "enable store %ld\n", data);

	if(data) {
		afa750_data.suspended = false;
		error = i2c_smbus_write_byte_data(afa750_i2c_client, POWER_CTL,WAKE_UP);
		assert(error==0);
	} else {
		afa750_data.suspended = true;
		error = i2c_smbus_write_byte_data(afa750_i2c_client, POWER_CTL,POWER_DOWN);		
		assert(error==0);
	}

	return count;

exit:
	return error;
}

static ssize_t afa750_delay_store(struct device *dev,struct device_attribute *attr,
		const char *buf, size_t count)
{
        unsigned long data;
	int error;

        dprintk(DEBUG_CONTROL_INFO, "delay store %d\n", __LINE__);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (data > POLL_INTERVAL_MAX)
		data = POLL_INTERVAL_MAX;

         afa750_idev->poll_interval = data;


	return count;
}

static DEVICE_ATTR(enable, 0666,NULL, afa750_enable_store);		
static DEVICE_ATTR(delay, 0666,NULL, afa750_delay_store);

static struct attribute *afa750_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
	NULL
};

static struct attribute_group afa750_attribute_group = {
	.attrs = afa750_attributes
};

static void report_abs(void)
{
/*
        u8 buf[6]={0};
        short x = 0, y = 0, z = 0;
        if(i2c_smbus_read_i2c_block_data(afa750_i2c_client, DATAX0, (DATAZ1 - DATAX0 + 1), buf) < 6){
                printk("FrancesLog****: smbus read block fialed \n");
        }	
        x = ((buf[1] << 8) & 0xff00) | buf[0];
	y = ((buf[3] << 8) & 0xff00) | buf[2];
	z = ((buf[5] << 8) & 0xff00) | buf[4];
	dprintk(DEBUG_DATA_INFO,"x[0] = %d, y[1] = %d, z[2] = %d. \n", x, y, z);
	*/
	 short buf[3]={0,0,0};
    //s8 buf[6]={0};
    s16 x = 0, y = 0, z = 0;
  if(i2c_smbus_read_i2c_block_data(afa750_i2c_client, DATAX0, DATAZ1 - DATAX0 + 1, buf) < 6){
     printk("FrancesLog****: smbus read block fialed \n");
   }
	
//	x = (s16) le16_to_cpu(buf[0] | ((buf[1] << 8) & 0xff00) );
//	y = (s16) le16_to_cpu(buf[2] | ((buf[3] << 8) & 0xff00) );
//	z = (s16) le16_to_cpu(buf[4] | ((buf[5] << 8) & 0xff00) );
	x = (s16) le16_to_cpu(buf[0]);
	y = (s16) le16_to_cpu(buf[1]);
	z = (s16) le16_to_cpu(buf[2]);
	input_report_abs(afa750_idev->input, ABS_X, x);
	input_report_abs(afa750_idev->input, ABS_Y, y);
	input_report_abs(afa750_idev->input, ABS_Z, z);
	input_sync(afa750_idev->input);
}

static void afa750_dev_poll(struct input_polled_dev *dev)
{
	dprintk(DEBUG_DATA_INFO, "afa750_data.suspended:%d\n",afa750_data.suspended);
	if(afa750_data.suspended == false){
	        dprintk(DEBUG_DATA_INFO,"get sensor data!\n");
	        report_abs();
	}

} 
#ifdef CONFIG_HAS_EARLYSUSPEND
static void afa750_early_suspend(struct early_suspend *h)
{
	int result = 0;
	dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND:afa750 early suspend\n");
	afa750_data.suspended = true;
	i2c_smbus_write_byte_data(afa750_i2c_client, 0x41,0x01);
	i2c_smbus_write_byte_data(afa750_i2c_client, 0x00,0x04);
	afa750_idev->input->close(afa750_idev->input);
        assert(result==0);

	return;
}

static void afa750_late_resume(struct early_suspend *h)
{
	int result = 0;
	dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND:afa750 late resume\n");
        afa750_data.suspended = false;
	i2c_smbus_write_byte_data(afa750_i2c_client, 0x36,0x01);
	mdelay(50);
	afa750_idev->input->open(afa750_idev->input);
        assert(result==0);
	return;
}
#else
#ifdef CONFIG_PM
static int afa750_early_suspend(struct i2c_client *client,pm_message_t mesg)
{
	int result = 0;
	dprintk(DEBUG_SUSPEND,"CONFIG_PM:afa750 suspend\n");
	afa750_data.suspended = true;
	i2c_smbus_write_byte_data(afa750_i2c_client, 0x41,0x01);
	i2c_smbus_write_byte_data(afa750_i2c_client, 0x00,0x04);
	afa750_idev->input->close(afa750_idev->input);
        assert(result==0);

	return 0;
}

static int afa750_late_resume(struct i2c_client *client)
{
	int result = 0;
	dprintk(DEBUG_SUSPEND, "CONFIG_PM:afa750 resume\n");
        afa750_data.suspended = false;
	i2c_smbus_write_byte_data(afa750_i2c_client, 0x36,0x01);
	mdelay(50);
	afa750_idev->input->open(afa750_idev->input);
        assert(result==0);
	return 0;
}
#endif
#endif /* CONFIG_HAS_EARLYSUSPEND */
static int __devinit afa750_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int result;
	struct input_dev *idev;
	struct i2c_adapter *adapter;

	dprintk(DEBUG_INIT, "======afa750 probe=====\n");
	client->addr = i2c_address[i2c_num - 1];
	afa750_i2c_client = client;
	adapter = to_i2c_adapter(client->dev.parent);
 	result = i2c_check_functionality(adapter,
 					 I2C_FUNC_SMBUS_BYTE |
 					 I2C_FUNC_SMBUS_BYTE_DATA);
	assert(result);


	hwmon_dev = hwmon_device_register(&client->dev);
	assert(!(IS_ERR(hwmon_dev)));

	dev_info(&client->dev, "build time %s %s\n", __DATE__, __TIME__);
  
	/*input poll device register */
	afa750_idev = input_allocate_polled_device();
	if (!afa750_idev) {
		dev_err(&client->dev, "alloc poll device failed!\n");
		result = -ENOMEM;
		return result;
	}
	afa750_idev->poll = afa750_dev_poll;
	afa750_idev->poll_interval = POLL_INTERVAL;
	afa750_idev->poll_interval_max = POLL_INTERVAL_MAX;
	idev = afa750_idev->input;
	idev->name = AFA750_DRV_NAME;
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);


	input_set_abs_params(idev, ABS_X, -AFA_FULLRES_MIN_VAL, AFA_FULLRES_MAX_VAL, 0, 0);
	input_set_abs_params(idev, ABS_Y, -AFA_FULLRES_MIN_VAL, AFA_FULLRES_MAX_VAL, 0, 0);
	input_set_abs_params(idev, ABS_Z, -AFA_FULLRES_MIN_VAL, AFA_FULLRES_MAX_VAL, 0, 0);
	
	result = input_register_polled_device(afa750_idev);
	if (result) {
		dev_err(&client->dev, "register poll device failed!\n");
		return result;
	}
	result = sysfs_create_group(&afa750_idev->input->dev.kobj, &afa750_attribute_group);

	if(result) {
		dev_err(&client->dev, "create sys failed\n");
	}
	
	afa750_data.client  = client;
        afa750_data.pollDev = afa750_idev;
        afa750_data.suspended = false;
	
	

#ifdef CONFIG_HAS_EARLYSUSPEND
	afa750_data.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	afa750_data.early_suspend.suspend = afa750_early_suspend;
	afa750_data.early_suspend.resume = afa750_late_resume;
	register_early_suspend(&afa750_data.early_suspend);
	afa750_data.suspend_indator = 0;
#endif
	dprintk(DEBUG_INIT, "======afa750 probe end=====\n");
	return result;
}

static int __devexit afa750_remove(struct i2c_client *client)
{
	int result = 0;
	afa750_data.suspended = true;
	hwmon_device_unregister(hwmon_dev);
#ifdef CONFIG_HAS_EARLYSUSPEND	
	unregister_early_suspend(&afa750_data.early_suspend);
#endif
	afa750_idev->input->close(afa750_idev->input);
	input_unregister_polled_device(afa750_idev);
	input_free_polled_device(afa750_idev);
	
	i2c_set_clientdata(afa750_i2c_client, NULL);	

	return result;
}



static const struct i2c_device_id afa750_id[] = {
	{ AFA750_DRV_NAME, 1 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, afa750_id);

static struct i2c_driver afa750_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name	= AFA750_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.probe	= afa750_probe,
	.remove	= __devexit_p(afa750_remove),
	.id_table = afa750_id,
#ifdef CONFIG_HAS_EARLYSUSPEND

#else
#ifdef CONFIG_PM
	.suspend  =  afa750_early_suspend,
	.resume   =  afa750_late_resume,
#endif
#endif
	.address_list	= normal_i2c,
};

static int __init afa750_init(void)
{
	int ret = -1;
	dprintk(DEBUG_INIT, "****************************************************************\n");
	printk("======%s=========. \n", __func__);
	if(gsensor_fetch_sysconfig_para()){
		printk("%s: err.\n", __func__);
		return -1;
	}
	afa750_driver.detect = gsensor_detect;

	ret = i2c_add_driver(&afa750_driver);
	if (ret < 0) {
		printk(KERN_INFO "add afa750 i2c driver failed\n");
		return -ENODEV;
	}
	dprintk(DEBUG_INIT, "****************************************************************\n");

	return ret;
}

static void __exit afa750_exit(void)
{
	printk("remove afa750 i2c driver.\n");
	sysfs_remove_group(&afa750_idev->input->dev.kobj, &afa750_attribute_group);
	i2c_del_driver(&afa750_driver);
}

MODULE_AUTHOR("Chen Gang <gang.chen@freescale.com>");
MODULE_DESCRIPTION("afa750 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);
module_init(afa750_init);
module_exit(afa750_exit);

