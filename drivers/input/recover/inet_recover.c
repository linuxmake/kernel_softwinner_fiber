/*
 *  inet-cover driver Copyright (c) 2014 inet-tek
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <linux/stat.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <mach/sys_config.h> 
#include <mach/gpio.h>

#include <linux/earlysuspend.h>
#include "inet_recover.h"

#define INET_DBG_EN   		0
#if(INET_DBG_EN == 1)		
#define inet_dbg(x,arg...) printk("[---inet---]"x,##arg)
#else
#define inet_dbg(x,arg...)
#endif
#define SWITCH_NUM 2

static int early_suspend_flag = 0;
static int suspend_flag = 0;
static int switch_enable=1;

static struct input_dev *key;
struct device *dev;

struct dev_pm_domain smart_cover_pm_domain;

static int top_used = 0;
static int right_used = 0;

static int not_first_suspend = 0;
static int top_irq_flag,right_irq_flag;
struct cover_switch_info cover_switch_info[SWITCH_NUM]={
		{.name = "top"},
		{.name = "right"},
};

static void wakeup_system(int i)
{
	int val;
	val = __gpio_get_value(cover_switch_info[i].gpio);	
	inet_dbg("%s [%s val:%d]\n", __func__, val? "open":"close", val);
#if 0	
	if(val == 0 && early_suspend_flag == 0)
	{
		state_store(NULL, NULL, "mem\n", 4);   
		inet_dbg("write mem to /sys/power/state \n");    
	}
	else if((val == 1 && early_suspend_flag == 1))
	{
		state_store(NULL, NULL, "on\n", 3);
		inet_dbg("write on to /sys/power/state \n");    
	}
#else
	if(val == early_suspend_flag)
	{
	  input_report_key(key, KEY_POWER, 1);
	  input_sync(key);
	  input_report_key(key, KEY_POWER, 0);
	  input_sync(key); 
	}
#endif
	return;
}
static int int_count;
static u32 irq_handler(void *para)
{
	//printk("%s: para 0x%08x\n", __func__, (u32)para);
	struct cover_switch_info *switch_info;
	switch_info = (struct cover_switch_info*)para;
	//printk("virp=%d,gpio=%#x\n",switch_info->virp,switch_info->gpio);
	inet_dbg("func:%s id:%s switch_enable:%d\n", __func__, switch_info->name, switch_enable);
	if(switch_enable == 0)
		return 0;

	if(strcmp(switch_info->name,"top") == 0)
	{
		top_irq_flag = 1;
		wakeup_system(0);
	}
	else
	{
		right_irq_flag = 1;
		wakeup_system(1);
	}
	return 0;
}

int smart_cover_fentch_config(void)
{
	script_item_u   item;
	script_item_value_type_e   type;
	int i;
	char sub_name[32];
	char *smart_cover[SWITCH_NUM]={"smart_cover_top","smart_cover_right"};
	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("smart_cover", "cover_used", &item))
		{
			printk("get cover_used from fex erro!\n");
			return -1;
		}
  if(item.val == 0)
  	{
  		printk("smart cover not used\n");
  		return -1;
  	}
	for(i=0; i<SWITCH_NUM; i++)
	{
		sprintf(sub_name, "%s", smart_cover[i]);
		type = script_get_item("smart_cover", sub_name, &item);
		if(SCIRPT_ITEM_VALUE_TYPE_PIO != type)
		{
			printk("script_get_item gpio %s err\n", smart_cover[i]);
			return -1;
		}
		cover_switch_info[i].gpio = item.gpio.gpio;
	}	
	return 0;
}



static int __do_request_irq(int i)
{
	int err;
	cover_switch_info[i].handler = irq_handler;	
	       
  err = sw_gpio_irq_request(cover_switch_info[i].gpio,TRIG_EDGE_DOUBLE,(peint_handle)irq_handler,&cover_switch_info[i]);	  				       				
			
	if (IS_ERR_VALUE(err)) {
		printk("request virq %d failed, errno = %d\n", 
		         cover_switch_info[i].gpio, err);
		return -EINVAL;
	}
	else
		printk("request_irq::%#x, name:%s\n",cover_switch_info[i].gpio, cover_switch_info[i].name);		
	
	return 0;
}
//extern int inet_covers_gpio[2];
static int smart_cover_request_irq(void)
{
	int i;
	for(i=0; i<SWITCH_NUM; i++)
	{		
		cover_switch_info[i].virp = gpio_to_irq(cover_switch_info[i].gpio);
		//inet_covers_gpio[i] = cover_switch_info[i].gpio-SUNXI_PL_BASE;
		if (IS_ERR_VALUE(cover_switch_info[i].virp))
		{
			printk("%s gpio to virq failed\n",cover_switch_info[i].name);
			return -EINVAL;
		}
		printk("cover_switch_info[%d].virp=%d\n",i,cover_switch_info[i].virp);
		if(__do_request_irq(i) < 0)
			return -1;
	}
	return 0;
}

static void smart_cover_enable(void)
{
	int i;
	for(i=0; i<SWITCH_NUM; i++)
	{
		 sw_gpio_eint_set_enable(cover_switch_info[i].gpio,1);
	}
}

static void smart_cover_disable(void)
{	
	int i;
	for(i=0; i<SWITCH_NUM; i++)
	{
		if (IS_ERR_VALUE(cover_switch_info[i].gpio))
		{
			sw_gpio_eint_set_enable(cover_switch_info[i].gpio,0);
		}
	}
}

static int smart_cover_show(struct inode *inode, struct file *file)
{
	inet_dbg("smart_cover_open irq:%s!!!!!!!!!\n",switch_enable? "enable":"disable");
	return 0;	
}

static ssize_t smart_cover_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	inet_dbg("smart_cover_open irq:%s!!!!!!!!!\n",switch_enable? "enable":"disable");
	return 0;	
}

static ssize_t smart_cover_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char c;
	if (get_user(c, buffer))
		return -EFAULT;

	if(c=='0' && switch_enable)
	{
		inet_dbg("disable\n");
		switch_enable = 0;
	}
	else if(c=='1' && !switch_enable)
	{
		inet_dbg("enable\n");	
		switch_enable = 1;	
		top_used = 1;		
	}
	else if(c=='2' && !switch_enable)
	{
		inet_dbg("enable\n");	
		switch_enable = 1;	
		right_used = 1;		
	}
	
	return strlen(buffer);
}
//----------------------------------------------------------------------------------------------------------------------

static unsigned int smart_cover_major =0;
static struct class *smart_cover_class =NULL;

static struct file_operations smart_cover_fops ={
	.owner	=THIS_MODULE,
	.open	= smart_cover_show,
	.read   = smart_cover_read,
	.write	=smart_cover_write,
};

void smart_cover_early_suspend(struct early_suspend *h)
{
	inet_dbg("%s irq_flag[top %d][right %d] not_first_irq[%d]\n", __func__, top_irq_flag, right_irq_flag, not_first_suspend);
	early_suspend_flag = 1;
	//recheck 
	if(top_irq_flag && not_first_suspend)
	{
		wakeup_system(0);
	}
	if(right_irq_flag && not_first_suspend)
	{
		wakeup_system(1);
	}
	not_first_suspend = 1;
	right_irq_flag = top_irq_flag = 0;	
}

void smart_cover_late_resume(struct early_suspend *h)
{
		//recheck
	inet_dbg("%s\n", __func__);
	early_suspend_flag = 0;
	right_irq_flag = top_irq_flag = 0;	
	//recheck
	/*
	if(irq_flag)
	{
		wakeup_system(1);
		wakeup_system(0);
	}
	irq_flag = 0;
	*/
}
static struct early_suspend smart_cover_suspend_handler = {
	.level   = 50, //50 100 150
	.suspend = smart_cover_early_suspend,
	.resume = smart_cover_late_resume,
};

static int __allocate_input_dev(void)
{
	int ret;
	key = input_allocate_device();
	key->name          = "inet_recover_key";
	//key->phys          = "modem/input0";
//	key->id.bustype    = BUS_HOST;
	//key->id.vendor     = 0xf001;
//	key->id.product    = 0xffff;
//	key->id.version    = 0x0100;
	
	__set_bit(EV_KEY, key->evbit);
	__set_bit(KEY_POWER, key->keybit);		
	
	ret = input_register_device(key);
	if(ret)
	{
	  printk("err: input_register_device failed\n");
	  input_free_device(key);
	  return -ENOMEM;
	}
	return 0;
}

static int before_top_val;
static int before_right_val;
static int smart_cover_suspend(struct device *dev)
{
	suspend_flag = 1;
	
	before_top_val = __gpio_get_value(cover_switch_info[0].gpio);
	before_right_val = __gpio_get_value(cover_switch_info[1].gpio);
	inet_dbg("smart_cover_suspend\n");
	return 0;
}

static int after_top_val;
static int after_right_val; 
static int smart_cover_resume(struct device *dev)
{
	int val1,val2;
	after_top_val   = __gpio_get_value(cover_switch_info[0].gpio);
	after_right_val = __gpio_get_value(cover_switch_info[1].gpio);
	
	right_irq_flag = top_irq_flag = 0;	
	inet_dbg("after_top_val:%d before_right_val:%d after_top_val:%d before_top_val:%d\n",after_top_val,before_right_val,after_top_val,before_top_val);
#if 1	
	if((after_right_val != before_right_val) && after_right_val)		
			wakeup_system(1);
	if((after_top_val != before_top_val) && after_top_val )
			wakeup_system(0);
#else
	if((val1 || 0/*val2*/) && suspend_flag==1)
	{		
		input_report_key(key, KEY_POWER, 1);
	  input_sync(key);
	  input_report_key(key, KEY_POWER, 0);
	  input_sync(key); 	  
		//suspend_flag = 0;	
	}
#endif
	suspend_flag = 0;	
	int_count = 0;	
	inet_dbg("smart_cover_resume\n");
	return 0;
}

static int __init smart_cover_init(void)
{
	inet_dbg("%s\n", __func__);

	if(smart_cover_fentch_config() < 0)
		return -1;

	smart_cover_major =register_chrdev(0,"smart_cover",&smart_cover_fops); 
	smart_cover_class =class_create(THIS_MODULE,"smart_cover_class");
	dev = device_create(smart_cover_class,NULL,MKDEV(smart_cover_major,0),NULL,"smart_cover");
	
	__allocate_input_dev();

	if(smart_cover_request_irq() < 0)
		return -2;

register_early_suspend(&smart_cover_suspend_handler);
	//test
	/*
	smart_cover_pm_domain.ops.suspend = smart_cover_suspend;
	smart_cover_pm_domain.ops.resume = smart_cover_resume;
	dev->pm_domain = &smart_cover_pm_domain;
	*/
	return 0;
}

static void __exit smart_cover_exit(void)
{
	inet_dbg("%s\n", __func__);
	unregister_chrdev(smart_cover_major,"buttons");
	device_destroy(smart_cover_class,MKDEV(smart_cover_major,0));
	class_destroy(smart_cover_class);
	unregister_early_suspend(&smart_cover_suspend_handler);
	input_unregister_device(key);
	input_free_device(key);
	//free_smart_cover_irq();
}

module_init(smart_cover_init);
module_exit(smart_cover_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("inet_recover driver");
