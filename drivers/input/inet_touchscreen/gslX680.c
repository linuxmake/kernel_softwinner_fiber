/*
 * drivers/input/touchscreen/gslX680.c
 *
 * Copyright (c) 2012 Shanghai Basewin
 *	Guan Yuwei<guanyuwei@basewin.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 
   mbgalex@163.com 2013-06-01_13:27
    modify resume for tp unlock,if define "gsl_resume_wq_mode " use resume workqueen,else not use resume_workqueen
   
    
 */



#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pm_runtime.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <linux/input/mt.h>

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/device.h>
#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

#include <mach/gpio.h> 
#include <linux/ctp.h>

#include "gslX680.h" //resolution:1024*768
#include "gsl2682B_PG_Q790_HT.h"

//Q70 PG
#include "Q70_GSL1680E_PG_INET.h"
#include "Q70_GSL1680D_PG_INET.h"
#include "Q70_GSL1680E_PG_CTD_YPD.h"
#include "Q70_GSL1680E_PG_DPT_LIANMOAXIN.h"


//Q71
#include "Q71_GSL2682B_1280800_PG_QSD_SDSM_JD050250.h"

static int ctp_ft5402_num = 0;


#define FOR_TSLIB_TEST
//#define GSL_TIMER
//#define PRINT_POINT_INFO 
//#define REPORT_DATA_ANDROID_4_0
static u32 gslX680_debug_mask = 1;

//#define HAVE_TOUCH_KEY

//#define gsl_resume_wq_mode

#define SCREEN_MAX_X		(screen_max_x)
#define SCREEN_MAX_Y		(screen_max_y)

#define GSLX680_I2C_NAME 	"gslX680"
//#define GSLX680_I2C_ADDR 	0x40
//#define TPD_PROC_DEBUG
#ifdef TPD_PROC_DEBUG
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
static struct proc_dir_entry *gsl_config_proc = NULL;
#define GSL_CONFIG_PROC_FILE "gsl_config"
#define CONFIG_LEN 31
static char gsl_read[CONFIG_LEN];
static u8 gsl_data_proc[8] = {0};
static u8 gsl_proc_flag = 0;
#endif



#define GSL_DATA_REG		0x80
#define GSL_STATUS_REG		0xe0
#define GSL_PAGE_REG		0xf0

#define PRESS_MAX    			255
#define MAX_FINGERS 		10//5 //??????
#define MAX_CONTACTS 		10
#define DMA_TRANS_LEN		0x20

#define PHO_CFG2_OFFSET	(0X104)
#define PHO_DAT_OFFSET		(0X10C)
#define PHO_PULL1_OFFSET	(0X11C)
#define GPIOF_CON			0x7f0080a0
#define GPIOF_DAT			0x7f0080a4
#define GPIOF_PUD			0x7f0080a8

#ifdef HAVE_TOUCH_KEY       
static u16 key = 0;
static int key_state_flag = 0;
struct key_data {
	u16 key;
	u16 x_min;
	u16 x_max;
	u16 y_min;
	u16 y_max;	
};

#define KEY_BACK	1
#define KEY_HOME	2
#define KEY_MENU	3
#define KEY_SEARCH	4

const u16 key_array[]={
                                      KEY_BACK,
                                      KEY_HOME,
                                      KEY_MENU,
                                      KEY_SEARCH,
                                     }; 
#define MAX_KEY_NUM     (sizeof(key_array)/sizeof(key_array[0]))

struct key_data gsl_key_data[MAX_KEY_NUM] = {
	{KEY_BACK, 2048, 2048, 2048, 2048},
	{KEY_HOME, 2048, 2048, 2048, 2048},	
	{KEY_MENU, 2048, 2048, 2048, 2048},
	{KEY_SEARCH, 2048, 2048, 2048, 2048},
};
#endif

struct gsl_ts_data {
	u8 x_index;
	u8 y_index;
	u8 z_index;
	u8 id_index;
	u8 touch_index;
	u8 data_reg;
	u8 status_reg;
	u8 data_size;
	u8 touch_bytes;
	u8 update_data;
	u8 touch_meta_data;
	u8 finger_size;
};

static struct gsl_ts_data devices[] = {
	{
		.x_index = 6,
		.y_index = 4,
		.z_index = 5,
		.id_index = 7,
		.data_reg = GSL_DATA_REG,
		.status_reg = GSL_STATUS_REG,
		.update_data = 0x4,
		.touch_bytes = 4,
		.touch_meta_data = 4,
		.finger_size = 70,
	},
};

struct gsl_ts {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct gsl_ts_data *dd;
	u8 *touch_data;
	u8 device_id;
	u8 prev_touches;
	bool is_suspended;
	bool int_pending;
	struct mutex sus_lock;
	int irq;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#ifdef GSL_TIMER
	struct timer_list gsl_timer;
#endif

};
extern struct ctp_config_info config_info;

static u32 ctp_debug = DEBUG_INIT;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(ctp_debug & level_mask)) \
        printk("***CTP***"fmt, ## arg)

static u32 id_sign[MAX_CONTACTS+1] = {0};
static u8 id_state_flag[MAX_CONTACTS+1] = {0};
static u8 id_state_old_flag[MAX_CONTACTS+1] = {0};
static u16 x_old[MAX_CONTACTS+1] = {0};
static u16 y_old[MAX_CONTACTS+1] = {0};
static u16 x_new = 0;
static u16 y_new = 0;


///////////////////////////////////////////////
//specific tp related macro: need be configured for specific tp

#define CTP_IRQ_NUMBER                  (config_info.irq_gpio_number)
#define CTP_IRQ_MODE			(TRIG_EDGE_NEGATIVE)
#define CTP_NAME			GSLX680_I2C_NAME
#define SCREEN_MAX_X		        (screen_max_x)
#define SCREEN_MAX_Y		        (screen_max_y)

#define GSLX680_I2C_ADDR 0x40
						 
#define GSLX680_USED     "\n \
												  \n+++++++++++++++++++++++++++++++++ \
												  \n++++++	  GSLX680  used +++++++++ \
												  \n+++++++++++++++++++++++++++++++++ \
												  \n"
													 
#define GSLX680_IC_INFO  "\n============================================================== \
												  \nIC	 :GSLX680 \
												  \nAUTHOR :p.huan @163.com \
												  \nVERSION:2013-06-01_13:27\n"
												  
extern int m_inet_ctpState;

static int gsl_chipType_new = 0;
static int ctp_cob_gslX680 = 0;
static int ctp_gslX680_OGS = 0;
static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;

static __u32 twi_id = 0;

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {GSLX680_I2C_ADDR, I2C_CLIENT_END};


static void glsX680_resume_events(struct work_struct *work);
struct workqueue_struct *gslX680_resume_wq;
static DECLARE_WORK(glsX680_resume_work, glsX680_resume_events);
struct i2c_client *glsX680_i2c;

static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret;

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;
    
	if(twi_id == adapter->nr){
                pr_info("%s: addr= %x\n",__func__,client->addr);
                ret = ctp_i2c_test(client);
                if(!ret){
        		pr_info("%s:I2C connection might be something wrong \n",__func__);
        		return -ENODEV;
        	}else{      
        				m_inet_ctpState=1;
            	        pr_info("I2C connection sucess!\n");
            	        strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
						pr_info("%s", GSLX680_USED);
    		    return 0;	
	             }

	}else{
		return -ENODEV;
	}
}



static ssize_t gslX680_debug_enable_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	return sprintf(buf, "0x%x", gslX680_debug_mask);
}

static ssize_t gslX680_debug_enable_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	if (buf[0] >= '0' && buf[0] <= '9')
	{
		gslX680_debug_mask = (buf[0] - '0');
	}
	else if (buf[0] >= 'a' && buf[0] <= 'f')
	{
		gslX680_debug_mask = 0x0A + (buf[0] - 'a');
	}
	else
	{
		gslX680_debug_mask = 0;
	}
	return count;
}

static DEVICE_ATTR(debug_enable, 0666, gslX680_debug_enable_show, gslX680_debug_enable_store);


static int gslX680_chip_init(void)
{	
         ctp_wakeup(1,0);
         msleep(20);
         return 0;   
}

static int gslX680_shutdown_low(void)
{
        ctp_wakeup(0,0);
	return 0;
}

static int gslX680_shutdown_high(void)
{
        ctp_wakeup(1,0);
	return 0;
}

static inline u16 join_bytes(u8 a, u8 b)
{
	u16 ab = 0;
	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}

static u32 gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[2];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = &reg;

	xfer_msg[1].addr = client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags |= I2C_M_RD;
	xfer_msg[1].buf = buf;

	if (reg < 0x80) {
		i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg));
		msleep(5);
	}

	return i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg)) == ARRAY_SIZE(xfer_msg) ? 0 : -EFAULT;
}
static u32 gsl_write_interface(struct i2c_client *client, const u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];

	buf[0] = reg;

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = buf;

	return i2c_transfer(client->adapter, xfer_msg, 1) == 1 ? 0 : -EFAULT;
}



static int gsl_ts_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125){
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	
	tmp_buf[0] = addr;
	bytelen++;
	
	if (datalen != 0 && pdata != NULL){
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}
	
	ret = i2c_master_send(client, tmp_buf, bytelen);
	return ret;
}

static int gsl_ts_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126){
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	ret = gsl_ts_write(client, addr, NULL, 0);
	if (ret < 0){
		printk("%s set data address fail!\n", __func__);
		return ret;
	}
	
	return i2c_master_recv(client, pdata, datalen);
}

static __inline__ void fw2buf(u8 *buf, const u32 *fw)
{
	u32 *u32_buf = (int *)buf;
	*u32_buf = *fw;
}


static void gsl_load_fw(struct i2c_client *client)
{
	printk("[FUN]%s\n", __func__);
	
	u8 buf[DMA_TRANS_LEN*4 + 1] = {0};
	u8 send_flag = 1;
	u8 *cur = buf + 1;
	u32 source_line = 0;
	u32 source_len;
	struct fw_data *ptr_fw;
	
	u8 read_buf[4] = {0};

	gsl_ts_read(client, 0xfc, read_buf, 4);
	printk("read 0xfc = %x %x %x %x\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	if(read_buf[2] != 0x82 && read_buf[2] != 0x88 )
	{
		msleep(100);
		gsl_ts_read(client, 0xfc, read_buf, 4);
		printk("read 0xfc = %x %x %x %x\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	}
	
	if(read_buf[2] == 0x82)
	{
		gsl_chipType_new =1 ;
	}

		
	printk("gsl_chipType_new=%d\n",gsl_chipType_new);
		
	printk("=============gsl_load_fw start==============\n");

	if(gsl_chipType_new == 1)
	{
		printk("gsl new chip\n");
		if(ctp_cob_gslX680 !=0)
			{
				switch(ctp_cob_gslX680)
				{
					case 1:
						
						printk("Download GSL2682B_FW_Q711C_OGS_V3");
						ptr_fw = GSL2682B_FW_Q711C_OGS_V3;
						source_len = ARRAY_SIZE(GSL2682B_FW_Q711C_OGS_V3);
						break;
					case 2:
						printk("Download GSL2682B_FW_Q711C_PG");
						ptr_fw = GSL2682B_FW_Q711C_PG;
						source_len = ARRAY_SIZE(GSL2682B_FW_Q711C_PG);
						break;
					case 201:
						printk("Download Q71_GSL2682B_1280800_PG_QSD_SDSM_JD050250\n");
						ptr_fw = Q71_GSL2682B_1280800_PG_QSD_SDSM_JD050250;
						source_len = ARRAY_SIZE(Q71_GSL2682B_1280800_PG_QSD_SDSM_JD050250);
						break;
					case 3:
						printk("Download Q70_GSL1680E_PG_INET");
						ptr_fw = Q70_GSL1680E_PG_INET;
						source_len = ARRAY_SIZE(Q70_GSL1680E_PG_INET);
						break;
					case 301:
						printk("Q70_GSL1680E_PG_CTD_YPD\n");
						ptr_fw = Q70_GSL1680E_PG_CTD_YPD;
						source_len = ARRAY_SIZE(Q70_GSL1680E_PG_CTD_YPD);
						break;
					case 302:
						printk("Q70_GSL1680E_PG_DPT_LIANMOAXIN\n");
						ptr_fw = Q70_GSL1680E_PG_DPT_LIANMOAXIN;
						source_len = ARRAY_SIZE(Q70_GSL1680E_PG_DPT_LIANMOAXIN);
						break;
					case 5:
						printk("Download GSL2682B_FW_Q790_PG_V2");
						ptr_fw = GSL2682B_FW_Q790_PG_V2;
						source_len = ARRAY_SIZE(GSL2682B_FW_Q790_PG_V2);
							
						break;
					case 501:
						printk("Download GSL2682B_FW_Q790_PG_C196131A1_CMW05020_U");
						ptr_fw = GSL2682B_FW_Q790_PG_C196131A1_CMW05020_U;
						source_len = ARRAY_SIZE(GSL2682B_FW_Q790_PG_C196131A1_CMW05020_U);
							
						break;
					default:
						printk("Download Q70_GSL1680E_PG_INET");
						ptr_fw = Q70_GSL1680E_PG_INET;
						source_len = ARRAY_SIZE(Q70_GSL1680E_PG_INET);
						break;
				}
			}
			
	}
	else
	{
		printk("gsl old chip\n");
		switch(ctp_cob_gslX680)
				{
					case 3:
						printk("Download Q70_GSL1680D_PG_INET");
						ptr_fw = Q70_GSL1680D_PG_INET;
						source_len = ARRAY_SIZE(Q70_GSL1680D_PG_INET);
						break;
					default:
						printk("Download Q70_GSL1680D_PG_INET");
						ptr_fw = Q70_GSL1680D_PG_INET;
						source_len = ARRAY_SIZE(Q70_GSL1680D_PG_INET);
						break;
				}
	}
			
	
	

	for (source_line = 0; source_line < source_len; source_line++) 
	{
		/* init page trans, set the page val */
		if (GSL_PAGE_REG == ptr_fw[source_line].offset)
		{
			fw2buf(cur, &ptr_fw[source_line].val);
			gsl_write_interface(client, GSL_PAGE_REG, buf, 4);
			send_flag = 1;
		}
		else 
		{
			if (1 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20))
	    			buf[0] = (u8)ptr_fw[source_line].offset;

			fw2buf(cur, &ptr_fw[source_line].val);
			cur += 4;

			if (0 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20)) 
			{
	    			gsl_write_interface(client, buf[0], buf, cur - buf - 1);
	    			cur = buf + 1;
			}

			send_flag++;
		}
	}

	printk("=============gsl_load_fw end==============\n");

}
static void startup_chip(struct i2c_client *client)
{
	u8 tmp = 0x00;
#if 1
	u8 buf[4] = {0x00};
	buf[3] = 0x01;
	buf[2] = 0xfe;
	buf[1] = 0x10;
	buf[0] = 0x00;	
	gsl_ts_write(client, 0xf0, buf, sizeof(buf));
	buf[3] = 0x00;
	buf[2] = 0x00;
	buf[1] = 0x00;
	buf[0] = 0x0f;	
	gsl_ts_write(client, 0x04, buf, sizeof(buf));
	msleep(20);	
#endif
	gsl_ts_write(client, 0xe0, &tmp, 1);
	msleep(10);	
}

static void reset_chip(struct i2c_client *client)
{
	u8 tmp = 0x88;
	u8 buf[4] = {0x00};
	
	gsl_ts_write(client, 0xe0, &tmp, sizeof(tmp));
	msleep(20);
	tmp = 0x04;
	gsl_ts_write(client, 0xe4, &tmp, sizeof(tmp));
	msleep(10);

	gsl_ts_write(client, 0xbc, buf, sizeof(buf));
	msleep(10);
}

static void clr_reg(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};

	write_buf[0] = 0x88;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);
	write_buf[0] = 0x01;
	gsl_ts_write(client, 0x80, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x04;
	gsl_ts_write(client, 0xe4, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x00;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);
}

static void init_chip(struct i2c_client *client)
{
	gslX680_shutdown_low();	
	msleep(50); 	
	gslX680_shutdown_high();	
	msleep(30); 		
	//test_i2c(client);
	clr_reg(client);
	reset_chip(client);
	gsl_load_fw(client);			
	startup_chip(client);
	reset_chip(client);
	startup_chip(client);	
}

static void check_mem_data(struct i2c_client *client)
{
	u8 read_buf[4]  = {0};

	//if(gsl_chipType_new == 1)	
	{
		msleep(30);
		gsl_ts_read(client,0xb0, read_buf, sizeof(read_buf));
		printk("#########check mem read 0xb0 = %x %x %x %x #########\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	
		if (read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
		{	
			init_chip(client);
		}
	}
}
#ifdef STRETCH_FRAME
static void stretch_frame(u16 *x, u16 *y)
{
	u16 temp_x = *x;
	u16 temp_y = *y;
	u16 temp_0, temp_1, temp_2;

	if(temp_x < X_STRETCH_MAX + X_STRETCH_CUST)
	{
		temp_0 = temp_1 = temp_2 = 0;
		temp_0 = X_STRETCH_MAX + X_STRETCH_CUST - temp_x;
		temp_0 = temp_0 > X_STRETCH_CUST ? X_STRETCH_CUST : temp_0;
		temp_0 = temp_0*(100 + X_RATIO_CUST)/100;
		if(temp_x < X_STRETCH_MAX)
		{
			temp_1 = X_STRETCH_MAX - temp_x;
			temp_1 = temp_1 > X_STRETCH_MAX/4 ? X_STRETCH_MAX/4 : temp_1;
			temp_1 = temp_1*(100 + 2*XL_RATIO_1)/100;
		}	
		if(temp_x < 3*X_STRETCH_MAX/4)
		{
			temp_2 = 3*X_STRETCH_MAX/4 - temp_x;
			temp_2 = temp_2*(100 + 2*XL_RATIO_2)/100;
		}
		*x = (temp_0 + temp_1 +temp_2) < (X_STRETCH_MAX + X_STRETCH_CUST) ? ((X_STRETCH_MAX + X_STRETCH_CUST) - (temp_0 + temp_1 +temp_2)) : 1;
	}
	else if(temp_x > (CTP_MAX_X -X_STRETCH_MAX - X_STRETCH_CUST))
	{
		temp_0 = temp_1 = temp_2 = 0;
		temp_0 = temp_x - (CTP_MAX_X -X_STRETCH_MAX - X_STRETCH_CUST);
		temp_0 = temp_0 > X_STRETCH_CUST ? X_STRETCH_CUST : temp_0;
		temp_0 = temp_0*(100 + X_RATIO_CUST)/100;
		if(temp_x > (CTP_MAX_X -X_STRETCH_MAX))
		{
			temp_1 = temp_x - (CTP_MAX_X -X_STRETCH_MAX);
			temp_1 = temp_1 > X_STRETCH_MAX/4 ? X_STRETCH_MAX/4 : temp_1;
			temp_1 = temp_1*(100 + 2*XR_RATIO_1)/100;
		}	
		if(temp_x > (CTP_MAX_X -3*X_STRETCH_MAX/4))
		{
			temp_2 = temp_x - (CTP_MAX_X -3*X_STRETCH_MAX/4);
			temp_2 = temp_2*(100 + 2*XR_RATIO_2)/100;
		}
		*x = (temp_0 + temp_1 +temp_2) < (X_STRETCH_MAX + X_STRETCH_CUST) ? ((CTP_MAX_X -X_STRETCH_MAX - X_STRETCH_CUST) + (temp_0 + temp_1 +temp_2)) : (CTP_MAX_X - 1);
	}
		
	if(temp_y < Y_STRETCH_MAX + Y_STRETCH_CUST)
	{
		temp_0 = temp_1 = temp_2 = 0;
		temp_0 = Y_STRETCH_MAX + Y_STRETCH_CUST - temp_y;
		temp_0 = temp_0 > Y_STRETCH_CUST ? Y_STRETCH_CUST : temp_0;
		temp_0 = temp_0*(100 + Y_RATIO_CUST)/100;
		if(temp_y < Y_STRETCH_MAX)
		{
			temp_1 = Y_STRETCH_MAX - temp_y;
			temp_1 = temp_1 > Y_STRETCH_MAX/4 ? Y_STRETCH_MAX/4 : temp_1;
			temp_1 = temp_1*(100 + 2*YL_RATIO_1)/100;
		}	
		if(temp_y < 3*Y_STRETCH_MAX/4)
		{
			temp_2 = 3*Y_STRETCH_MAX/4 - temp_y;
			temp_2 = temp_2*(100 + 2*YL_RATIO_2)/100;
		}
		*y = (temp_0 + temp_1 +temp_2) < (Y_STRETCH_MAX + Y_STRETCH_CUST) ? ((Y_STRETCH_MAX + Y_STRETCH_CUST) - (temp_0 + temp_1 +temp_2)) : 1;
	}
	else if(temp_y > (CTP_MAX_Y -Y_STRETCH_MAX - Y_STRETCH_CUST))
	{
		temp_0 = temp_1 = temp_2 = 0;	
		temp_0 = temp_y - (CTP_MAX_Y -Y_STRETCH_MAX - Y_STRETCH_CUST);
		temp_0 = temp_0 > Y_STRETCH_CUST ? Y_STRETCH_CUST : temp_0;
		temp_0 = temp_0*(100 + Y_RATIO_CUST)/100;
		if(temp_y > (CTP_MAX_Y -Y_STRETCH_MAX))
		{
			temp_1 = temp_y - (CTP_MAX_Y -Y_STRETCH_MAX);
			temp_1 = temp_1 > Y_STRETCH_MAX/4 ? Y_STRETCH_MAX/4 : temp_1;
			temp_1 = temp_1*(100 + 2*YR_RATIO_1)/100;
		}	
		if(temp_y > (CTP_MAX_Y -3*Y_STRETCH_MAX/4))
		{
			temp_2 = temp_y - (CTP_MAX_Y -3*Y_STRETCH_MAX/4);
			temp_2 = temp_2*(100 + 2*YR_RATIO_2)/100;
		}
		*y = (temp_0 + temp_1 +temp_2) < (Y_STRETCH_MAX + Y_STRETCH_CUST) ? ((CTP_MAX_Y -Y_STRETCH_MAX - Y_STRETCH_CUST) + (temp_0 + temp_1 +temp_2)) : (CTP_MAX_Y - 1);
	}
}
#endif

#ifdef FILTER_POINT
static void filter_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;
	u16 filter_step_x = 0, filter_step_y = 0;
	
	id_sign[id] = id_sign[id] + 1;
	if(id_sign[id] == 1)
	{
		x_old[id] = x;
		y_old[id] = y;
	}
	
	x_err = x > x_old[id] ? (x -x_old[id]) : (x_old[id] - x);
	y_err = y > y_old[id] ? (y -y_old[id]) : (y_old[id] - y);

	if( (x_err > FILTER_MAX && y_err > FILTER_MAX/3) || (x_err > FILTER_MAX/3 && y_err > FILTER_MAX) )
	{
		filter_step_x = x_err;
		filter_step_y = y_err;
	}
	else
	{
		if(x_err > FILTER_MAX)
			filter_step_x = x_err; 
		if(y_err> FILTER_MAX)
			filter_step_y = y_err;
	}

	if(x_err <= 2*FILTER_MAX && y_err <= 2*FILTER_MAX)
	{
		filter_step_x >>= 2; 
		filter_step_y >>= 2;
	}
	else if(x_err <= 3*FILTER_MAX && y_err <= 3*FILTER_MAX)
	{
		filter_step_x >>= 1; 
		filter_step_y >>= 1;
	}	
	else if(x_err <= 4*FILTER_MAX && y_err <= 4*FILTER_MAX)
	{
		filter_step_x = filter_step_x*3/4; 
		filter_step_y = filter_step_y*3/4;
	}	
	
	x_new = x > x_old[id] ? (x_old[id] + filter_step_x) : (x_old[id] - filter_step_x);
	y_new = y > y_old[id] ? (y_old[id] + filter_step_y) : (y_old[id] - filter_step_y);

	x_old[id] = x_new;
	y_old[id] = y_new;
}
#else

static void record_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;

	id_sign[id]=id_sign[id]+1;
	
	if(id_sign[id]==1){
		x_old[id]=x;
		y_old[id]=y;
	}

	x = (x_old[id] + x)/2;
	y = (y_old[id] + y)/2;
		
	if(x>x_old[id]){
		x_err=x -x_old[id];
	}
	else{
		x_err=x_old[id]-x;
	}

	if(y>y_old[id]){
		y_err=y -y_old[id];
	}
	else{
		y_err=y_old[id]-y;
	}

	if( (x_err > 3 && y_err > 1) || (x_err > 1 && y_err > 3) ){
		x_new = x;     x_old[id] = x;
		y_new = y;     y_old[id] = y;
	}
	else{
		if(x_err > 3){
			x_new = x;     x_old[id] = x;
		}
		else
			x_new = x_old[id];
		if(y_err> 3){
			y_new = y;     y_old[id] = y;
		}
		else
			y_new = y_old[id];
	}

	if(id_sign[id]==1){
		x_new= x_old[id];
		y_new= y_old[id];
	}
	
}
#endif
#ifdef TPD_PROC_DEBUG
static int char_to_int(char ch)
{
	if(ch>='0' && ch<='9')
		return (ch-'0');
	else
		return (ch-'a'+10);
}

static int gsl_config_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *ptr = page;
	//char temp_data[4] = {0};
	char temp_data[5] = {0};
	//int i;
	unsigned int tmp=0;
	if('v'==gsl_read[0]&&'s'==gsl_read[1])
	{
#ifdef GSL_NOID_VERSION
		tmp=gsl_version_id();
#else 
		tmp=0x20121215;
#endif
		ptr += sprintf(ptr,"version:%x\n",tmp);
	}
	else if('r'==gsl_read[0]&&'e'==gsl_read[1])
	{
		if('i'==gsl_read[3])
		{
#ifdef GSL_NOID_VERSION 
			tmp=(gsl_data_proc[5]<<8) | gsl_data_proc[4];
			ptr +=sprintf(ptr,"gsl_config_data_id[%d] = ",tmp);
			if(tmp>=0&&tmp<256)
				ptr +=sprintf(ptr,"%d\n",gsl_config_data_id[tmp]); 
#endif
		}
		else 
		{
			gsl_ts_write(glsX680_i2c,0xf0,&gsl_data_proc[4],4);
			gsl_ts_read(glsX680_i2c,gsl_data_proc[0],temp_data,4);
			gsl_ts_read(glsX680_i2c,gsl_data_proc[0],temp_data,4);
			ptr +=sprintf(ptr,"offset : {0x%02x,0x",gsl_data_proc[0]);
			ptr +=sprintf(ptr,"%02x",temp_data[3]);
			ptr +=sprintf(ptr,"%02x",temp_data[2]);
			ptr +=sprintf(ptr,"%02x",temp_data[1]);
			ptr +=sprintf(ptr,"%02x};\n",temp_data[0]);
		}
	}
	*eof = 1;
	return (ptr - page);
}
static int gsl_config_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
	u8 buf[8] = {0};
	//u8 addr = 0;
	int tmp = 0;
	int tmp1 = 0;
	//print_info("[tp-gsl][%s] \n",__func__);
	
	if(count > CONFIG_LEN)
	{
		//print_info("size not match [%d:%ld]\n", CONFIG_LEN, count);
        	return -EFAULT;
	}
	
	if(copy_from_user(gsl_read, buffer, (count<CONFIG_LEN?count:CONFIG_LEN)))
	{
		//print_info("copy from user fail\n");
        	return -EFAULT;
	}
	//print_info("[tp-gsl][%s][%s]\n",__func__,gsl_read);

	buf[3]=char_to_int(gsl_read[14])<<4 | char_to_int(gsl_read[15]);	
	buf[2]=char_to_int(gsl_read[16])<<4 | char_to_int(gsl_read[17]);
	buf[1]=char_to_int(gsl_read[18])<<4 | char_to_int(gsl_read[19]);
	buf[0]=char_to_int(gsl_read[20])<<4 | char_to_int(gsl_read[21]);
	
	buf[7]=char_to_int(gsl_read[5])<<4 | char_to_int(gsl_read[6]);
	buf[6]=char_to_int(gsl_read[7])<<4 | char_to_int(gsl_read[8]);
	buf[5]=char_to_int(gsl_read[9])<<4 | char_to_int(gsl_read[10]);
	buf[4]=char_to_int(gsl_read[11])<<4 | char_to_int(gsl_read[12]);
	if('v'==gsl_read[0]&& 's'==gsl_read[1])//version //vs
	{
		printk("gsl version\n");
	}
	else if('s'==gsl_read[0]&& 't'==gsl_read[1])//start //st
	{
		gsl_proc_flag = 1;
		reset_chip(glsX680_i2c);
		/*msleep(20);
		mt_set_gpio_out(GPIO_CTP_RST_PIN, 0);
		msleep(20);
		mt_set_gpio_out(GPIO_CTP_RST_PIN, 1);
		msleep(20);
		*/
		//gsl_start_core(ddata->client);
	}
	else if('e'==gsl_read[0]&&'n'==gsl_read[1])//end //en
	{
		msleep(20);
		reset_chip(glsX680_i2c);
		startup_chip(glsX680_i2c);
#ifdef GSL_NOID_VERSION
		gsl_DataInit(gsl_config_data_id);
#endif
		gsl_proc_flag = 0;
	}
	else if('r'==gsl_read[0]&&'e'==gsl_read[1])//read buf //
	{
		memcpy(gsl_data_proc,buf,8);
	}
	else if('w'==gsl_read[0]&&'r'==gsl_read[1])//write buf
	{
		gsl_ts_write(glsX680_i2c,buf[4],buf,4);
	}
	
#ifdef GSL_NOID_VERSION
	else if('i'==gsl_read[0]&&'d'==gsl_read[1])//write id config //
	{
		tmp1=(buf[7]<<24)|(buf[6]<<16)|(buf[5]<<8)|buf[4];
		tmp=(buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
		if(tmp1>=0 && tmp1<256)
		{
			gsl_config_data_id[tmp1] = tmp;
		}
	}
#endif
	return count;
}
#endif
#ifdef HAVE_TOUCH_KEY
static void report_key(struct gsl_ts *ts, u16 x, u16 y)
{
	u16 i = 0;
	for(i = 0; i < MAX_KEY_NUM; i++) {
		if((gsl_key_data[i].x_min < x) && (x < gsl_key_data[i].x_max)&&(gsl_key_data[i].y_min < y) &&\
		  (y < gsl_key_data[i].y_max)){
			key = gsl_key_data[i].key;	
			input_report_key(ts->input, key, 1);
			input_sync(ts->input); 		
			key_state_flag = 1;
			break;
		}
	}
}
#endif

static void report_data(struct gsl_ts *ts, u16 x, u16 y, u8 pressure, u8 id)
{
	
	if(gsl_chipType_new != 1 && ctp_cob_gslX680 ==3)
		{
			swap(x, y);
		}
		else if(gsl_chipType_new != 1 && ctp_cob_gslX680 ==4)
		{
			swap(x,y);
		}
		else
		{
			x = SCREEN_MAX_X - x;
			y = SCREEN_MAX_Y - y;
		}
		
	
	//swap(x, y);
	//x = SCREEN_MAX_X - x;
	//y = SCREEN_MAX_Y - y;
	
	//printk("x = %d, y = %d\n", x, y);
	
	/*	
	if(1 == exchange_x_y_flag){
                swap(x, y);
        }
        if(1 == revert_x_flag){
                x = SCREEN_MAX_X - x;
        }
        if(1 == revert_y_flag){
                y = SCREEN_MAX_Y - y;
        }
     */  
	dprintk(DEBUG_X_Y_INFO,"#####id=%d,x=%d,y=%d######\n",id,x,y);
	//printk("#####id=%d,x=%d,y=%d######\n",id,x,y);

	if(x>SCREEN_MAX_X||y>SCREEN_MAX_Y)
	{
		//printk("x>SCREEN_MAX_X||y>SCREEN_MAX_Y\n");
	#ifdef HAVE_TOUCH_KEY
		report_key(ts,x,y);
	#endif
		return;
	}
	
#ifdef REPORT_DATA_ANDROID_4_0
	input_mt_slot(ts->input, id);		
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);	
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
#else
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_POSITION_X,x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
	input_mt_sync(ts->input);
#endif
}

static void process_gslX680_data(struct gsl_ts *ts)
{
	u8 id, touches;
	u16 x, y;
	int i = 0;

	touches = ts->touch_data[ts->dd->touch_index];
	//printk("touches=%d\n",touches);
	for(i=1;i<=MAX_CONTACTS;i++){
		if(touches == 0)
			id_sign[i] = 0;	
		id_state_flag[i] = 0;
	}
	for(i= 0;i < (touches > MAX_FINGERS ? MAX_FINGERS : touches);i ++){
		x = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		y = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i ]);
		id = ts->touch_data[ts->dd->id_index + 4 * i] >> 4;

		if(1 <=id && id <= MAX_CONTACTS){

		//printk("test !!!!!!!!!!!!!!!!\n");
		//printk("gsl_chipType_new=%d,ctp_cob_gslX680=%d\n",gsl_chipType_new,ctp_cob_gslX680);
		if(gsl_chipType_new == 1 && ctp_cob_gslX680 ==1)
		{
		
		//#ifdef STRETCH_FRAME
			stretch_frame(&x, &y);
		//#endif
		}
		#ifdef FILTER_POINT
			filter_point(x, y ,id);
		#else
			record_point(x, y , id);
		#endif
			report_data(ts, x_new, y_new, 10, id);		
			id_state_flag[id] = 1;
		}
	}
	for(i=1;i<=MAX_CONTACTS;i++){	
		if( (0 == touches) || ((0 != id_state_old_flag[i]) && (0 == id_state_flag[i])) ){
		#ifdef REPORT_DATA_ANDROID_4_0
			input_mt_slot(ts->input, i);
			input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(ts->input, MT_TOOL_FINGER, false);
		#endif
			input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 0);//for dragonboard
			id_sign[i]=0;
		}
		id_state_old_flag[i] = id_state_flag[i];
	}
#ifndef REPORT_DATA_ANDROID_4_0
	if(0 == touches){	
		input_mt_sync(ts->input);
	#ifdef HAVE_TOUCH_KEY
		if(key_state_flag){
        		input_report_key(ts->input, key, 0);
			input_sync(ts->input);
			key_state_flag = 0;
		}
	#endif			
	}
#endif
	
	//printk("222#####touches=%d,x=%d,y=%d######\n",touches,x,y);
	input_sync(ts->input);
	ts->prev_touches = touches;
	//printk("333#####touches=%d,x=%d,y=%d######\n",touches,x,y);
}


static void gsl_ts_xy_worker(struct work_struct *work)
{
	int rc;
	u8 read_buf[4] = {0};
	struct gsl_ts *ts = container_of(work, struct gsl_ts,work);

	dprintk(DEBUG_X_Y_INFO,"---gsl_ts_xy_worker---\n");				 
#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1){
		goto schedule;
	}
#endif
	/* read data from DATA_REG */
	rc = gsl_ts_read(ts->client, 0x80, ts->touch_data, ts->dd->data_size);
	dprintk(DEBUG_X_Y_INFO,"---touches: %d ---\n",ts->touch_data[0]);			
	if (rc < 0) {
		dev_err(&ts->client->dev, "read failed\n");
		goto schedule;
	}

	if (ts->touch_data[ts->dd->touch_index] == 0xff) {
		goto schedule;
	}

	rc = gsl_ts_read( ts->client, 0xbc, read_buf, sizeof(read_buf));
	if (rc < 0) {
		dev_err(&ts->client->dev, "read 0xbc failed\n");
		goto schedule;
	}
	dprintk(DEBUG_X_Y_INFO,"reg %x : %x %x %x %x\n",0xbc, read_buf[3], read_buf[2], read_buf[1], read_buf[0]);		
	if (read_buf[3] == 0 && read_buf[2] == 0 && read_buf[1] == 0 && read_buf[0] == 0){
		process_gslX680_data(ts);
	}
	else
	{
		reset_chip(ts->client);
		startup_chip(ts->client);
	}
	
schedule:
        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);

}

static u32 gsl_ts_irq(struct gsl_ts *ts)
{	
	dprintk(DEBUG_INT_INFO,"==========GSLX680 Interrupt============\n");				 
	queue_work(ts->wq, &ts->work);	
	return 0;
}

#ifdef GSL_TIMER
static void gsl_timer_handle(unsigned long data)
{
	struct gsl_ts *ts = (struct gsl_ts *)data;

#ifdef GSL_DEBUG	
	printk("----------------gsl_timer_handle-----------------\n");	
#endif
        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
	check_mem_data(ts->client);
	ts->gsl_timer.expires = jiffies + 3 * HZ;
	add_timer(&ts->gsl_timer);
	enable_irq(ts->irq);
	
}
#endif

static int gsl_ts_init_ts(struct i2c_client *client, struct gsl_ts *ts)
{
	struct input_dev *input_device;
	int  rc = 0;
	
	printk("[GSLX680] Enter %s\n", __func__);
	ts->dd = &devices[ts->device_id];

	if (ts->device_id == 0) {
		ts->dd->data_size = MAX_FINGERS * ts->dd->touch_bytes + ts->dd->touch_meta_data;
		ts->dd->touch_index = 0;
	}

	ts->touch_data = kzalloc(ts->dd->data_size, GFP_KERNEL);
	if (!ts->touch_data) {
		pr_err("%s: Unable to allocate memory\n", __func__);
		return -ENOMEM;
	}

	ts->prev_touches = 0;

	input_device = input_allocate_device();
	if (!input_device) {
		rc = -ENOMEM;
		goto error_alloc_dev;
	}

	ts->input = input_device;
	input_device->name = GSLX680_I2C_NAME;
	input_device->id.bustype = BUS_I2C;
	input_device->dev.parent = &client->dev;
	input_set_drvdata(input_device, ts);

#ifdef REPORT_DATA_ANDROID_4_0
	__set_bit(EV_ABS, input_device->evbit);
	__set_bit(EV_KEY, input_device->evbit);
	__set_bit(EV_REP, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_mt_init_slots(input_device, (MAX_CONTACTS+1));
#else
	input_set_abs_params(input_device,ABS_MT_TRACKING_ID, 0, (MAX_CONTACTS+1), 0, 0);
	set_bit(EV_ABS, input_device->evbit);
	set_bit(EV_KEY, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_device->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif
#ifdef HAVE_TOUCH_KEY
	input_device->evbit[0] = BIT_MASK(EV_KEY);
	for (i = 1; i <= MAX_KEY_NUM; i++)
		set_bit(i, input_device->keybit);
#endif

	set_bit(ABS_MT_POSITION_X, input_device->absbit);
	set_bit(ABS_MT_POSITION_Y, input_device->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_device->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_device->absbit);

	input_set_abs_params(input_device,ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_device,ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_device,ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_device,ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	ts->wq = create_singlethread_workqueue("kworkqueue_ts");
	if (!ts->wq) {
		dev_err(&client->dev, "Could not create workqueue\n");
		goto error_wq_create;
	}
	flush_workqueue(ts->wq);	

	INIT_WORK(&ts->work, gsl_ts_xy_worker);

	rc = input_register_device(input_device);
	if (rc)
		goto error_unreg_device;

	return 0;

error_unreg_device:
	destroy_workqueue(ts->wq);
error_wq_create:
	input_free_device(input_device);
error_alloc_dev:
	kfree(ts->touch_data);
	return rc;
}
#ifdef gsl_resume_wq_mode
static void glsX680_resume_events (struct work_struct *work)
{
	ctp_wakeup(1,0);
	startup_chip(glsX680_i2c);
	check_mem_data(glsX680_i2c);
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
}

static int gsl_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
        struct gsl_ts *ts = i2c_get_clientdata(client);
        dprintk(DEBUG_SUSPEND,"%s,start\n",__func__);
        
        cancel_work_sync(&glsX680_resume_work);
  	flush_workqueue(gslX680_resume_wq);
  	
#ifndef CONFIG_HAS_EARLYSUSPEND
        ts->is_suspended = true;
#endif

#ifdef GSL_TIMER
	dprintk(DEBUG_SUSPEND,"gsl_ts_suspend () : delete gsl_timer\n");
	del_timer(&ts->gsl_timer);
#endif
        if(ts->is_suspended == true ){
                sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
        	flush_workqueue(gslX680_resume_wq);
        	cancel_work_sync(&ts->work);
        	flush_workqueue(ts->wq);
        	gslX680_shutdown_low(); 
        }
        
        ts->is_suspended = true;
        return 0;
	
}

static int gsl_ts_resume(struct i2c_client *client)
{
	//printk("[FUN]%s\n", __func__);
	
	struct gsl_ts *ts = i2c_get_clientdata(client);
	
	ts->is_suspended = true;
  	dprintk(DEBUG_SUSPEND,"I'am in gsl_ts_resume() start\n");
  	cancel_work_sync(&ts->work);
	flush_workqueue(ts->wq);
	queue_work(gslX680_resume_wq, &glsX680_resume_work);
	
#ifdef GSL_TIMER
	dprintk(DEBUG_SUSPEND, "gsl_ts_resume () : add gsl_timer\n");
	init_timer(&ts->gsl_timer);
	ts->gsl_timer.expires = jiffies + 3 * HZ;
	ts->gsl_timer.function = &gsl_timer_handle;
	ts->gsl_timer.data = (unsigned long)ts;
	add_timer(&ts->gsl_timer);
#endif
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void gsl_ts_early_suspend(struct early_suspend *h)
{
	struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	dprintk(DEBUG_SUSPEND,"[GSL1680] Enter %s\n", __func__);
	cancel_work_sync(&glsX680_resume_work);
	flush_workqueue(gslX680_resume_wq);
        ts->is_suspended = false;
        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);

	cancel_work_sync(&ts->work);
	flush_workqueue(ts->wq);
	gslX680_shutdown_low(); 
}

static void gsl_ts_late_resume(struct early_suspend *h)
{
	struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	dprintk(DEBUG_SUSPEND,"[GSL1680] Enter %s\n", __func__);
	cancel_work_sync(&ts->work);
	flush_workqueue(ts->wq);
	
#ifndef CONFIG_PM
        gsl_ts_resume(ts->client);
#endif
      if(ts->is_suspended == false){
                queue_work(gslX680_resume_wq, &glsX680_resume_work);
      }
	dprintk(DEBUG_SUSPEND,"ts->is_suspended:%d\n",ts->is_suspended);
}
#endif
#else
static void glsX680_resume_events (struct work_struct *work)
{ 
	reset_chip(glsX680_i2c);
	startup_chip(glsX680_i2c);
	check_mem_data(glsX680_i2c);
}

static int gsl_ts_suspend(struct device *dev)
{
	struct gsl_ts *ts = dev_get_drvdata(dev);
	dprintk(DEBUG_SUSPEND,"%s,start\n",__func__);
#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1){
		return;
	}
#endif
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);


	gslX680_shutdown_low();

#ifdef SLEEP_CLEAR_POINT
	msleep(10); 		
	#ifdef REPORT_DATA_ANDROID_4_0
	for(i = 1; i <= MAX_CONTACTS ;i ++)
	{	
		input_mt_slot(ts->input, i);
		input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
		input_mt_report_slot_state(ts->input, MT_TOOL_FINGER, false);
	}
	#else	
	input_mt_sync(ts->input);
	#endif
	input_sync(ts->input);
	msleep(10); 	
	report_data(ts, 1, 1, 10, 1);		
	input_sync(ts->input);	
#endif	

	return 0;
}

static int gsl_ts_resume(struct device *dev)
{
	struct gsl_ts *ts = dev_get_drvdata(dev);
  	dprintk(DEBUG_SUSPEND,"%s,start\n",__func__);
#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1){
		return;
	}
#endif
	gslX680_shutdown_high();
	msleep(20); 	
	//queue_work(gslX680_resume_wq, &glsX680_resume_work);
	reset_chip(glsX680_i2c);
	startup_chip(glsX680_i2c);
	check_mem_data(glsX680_i2c);
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
#ifdef SLEEP_CLEAR_POINT
	#ifdef REPORT_DATA_ANDROID_4_0
	for(i =1;i<=MAX_CONTACTS;i++)
	{	
		input_mt_slot(ts->input, i);
		input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
		input_mt_report_slot_state(ts->input, MT_TOOL_FINGER, false);
	}
	#else	
	input_mt_sync(ts->input);
	#endif
	input_sync(ts->input);	
#endif
#ifdef GSL_MONITOR
	printk( "gsl_ts_resume () : queue gsl_monitor_work\n");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 300);
#endif	
	
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void gsl_ts_early_suspend(struct early_suspend *h)
{
	struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND:Enter %s\n", __func__);
	gsl_ts_suspend(&ts->client->dev);
}

static void gsl_ts_late_resume(struct early_suspend *h)
{
	struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND: Enter %s\n", __func__);
	gsl_ts_resume(&ts->client->dev);
}
#endif
#endif
static int __devinit gsl_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct gsl_ts *ts;
	int rc;

	printk("GSLX680 Enter %s\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -ENODEV;
	}
 
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (!ts){
	        printk("allocate data fail!\n");
		return -ENOMEM;
	}
        
	gslX680_resume_wq = create_singlethread_workqueue("gslX680_resume");
	if (gslX680_resume_wq == NULL) {
		printk("create gslX680_resume_wq fail!\n");
		return -ENOMEM;
	}
	
        glsX680_i2c = client;
	ts->client = client;
	i2c_set_clientdata(client, ts);
	ts->device_id = id->driver_data;

	ts->is_suspended = false;
	ts->int_pending = false;
	mutex_init(&ts->sus_lock);
	
	rc = gsl_ts_init_ts(client, ts);
	if (rc < 0) {
		dev_err(&client->dev, "GSLX680 init failed\n");
		goto error_mutex_destroy;
	}
	gslX680_chip_init();    	
	init_chip(ts->client);
	check_mem_data(ts->client);
	
	device_enable_async_suspend(&client->dev);
	//rc=  request_irq(client->irq, gsl_ts_irq, IRQF_TRIGGER_RISING | IRQF_SHARED, client->name, ts);
	int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)gsl_ts_irq,ts);
	if (!int_handle) {
		printk( "gsl_probe: request irq failed\n");
		goto error_req_irq_fail;
	}

#ifdef GSL_TIMER
	printk( "gsl_ts_probe () : add gsl_timer\n");

	init_timer(&ts->gsl_timer);
	ts->gsl_timer.expires = jiffies + 3 * HZ;	//??3  ??
	ts->gsl_timer.function = &gsl_timer_handle;
	ts->gsl_timer.data = (unsigned long)ts;
	add_timer(&ts->gsl_timer);
#endif
#ifdef TPD_PROC_DEBUG
	gsl_config_proc = create_proc_entry(GSL_CONFIG_PROC_FILE, 0666, NULL);
	if (gsl_config_proc == NULL)
	{
		//print_info("create_proc_entry %s failed\n", GSL_CONFIG_PROC_FILE);
	}
	else
	{
		gsl_config_proc->read_proc = gsl_config_read_proc;
		gsl_config_proc->write_proc = gsl_config_write_proc;
	}
	gsl_proc_flag = 0;
#endif
	/* create debug attribute */
	rc = device_create_file(&ts->input->dev, &dev_attr_debug_enable);

	gslX680_debug_mask = 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	ts->early_suspend.suspend = gsl_ts_early_suspend;
	ts->early_suspend.resume = gsl_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	printk("[GSLX680] End %s\n", __func__);

	return 0;

	
error_req_irq_fail:
        sw_gpio_irq_free(int_handle);	
error_mutex_destroy:
	mutex_destroy(&ts->sus_lock);
	input_free_device(ts->input);
	kfree(ts);
	return rc;
}

static int __devexit gsl_ts_remove(struct i2c_client *client)
{
	struct gsl_ts *ts = i2c_get_clientdata(client);
	printk("==gsl_ts_remove=\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif

	device_init_wakeup(&client->dev, 0);
	cancel_work_sync(&ts->work);
	cancel_work_sync(&glsX680_resume_work);
	sw_gpio_irq_free(int_handle);
	destroy_workqueue(ts->wq);
	input_unregister_device(ts->input);
	destroy_workqueue(gslX680_resume_wq);
	mutex_destroy(&ts->sus_lock);
	device_remove_file(&ts->input->dev, &dev_attr_debug_enable);
	kfree(ts->touch_data);
	kfree(ts);

	return 0;
}

static const struct i2c_device_id gsl_ts_id[] = {
	{GSLX680_I2C_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gsl_ts_id);

static struct i2c_driver gsl_ts_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name = GSLX680_I2C_NAME,
		.owner = THIS_MODULE,
	},
	.probe		= gsl_ts_probe,
	.remove		= __devexit_p(gsl_ts_remove),
	.id_table		= gsl_ts_id,
	.address_list	= normal_i2c,
#ifdef gsl_resume_wq_mode
	.suspend        =  gsl_ts_suspend,
	.resume         =  gsl_ts_resume,
#else
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = gsl_ts_suspend,
	.resume	= gsl_ts_resume,
#endif
#endif
};
static int ctp_get_system_config(void)
{   
			
        script_item_u   val;
        

	   	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_gslX680_OGS", &val))
		{
			ctp_gslX680_OGS=0;
		}
		else
		{
			ctp_gslX680_OGS = val.val;
		}
		//ctp_gslX680_OGS =  1;
		if(ctp_gslX680_OGS==1)
		{
			printk("This is GSLX680 OGS TP,but this driver is not for ogs,so quit!\n");
			return 0;
		}

		int temp = 1;
		if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_ft5402_num", &val))
		{
			ctp_ft5402_num=0;
		}
		else
		{
			ctp_ft5402_num = val.val;
		}
		switch(ctp_ft5402_num)
		{
			case 2:
				temp = 0;
				break;
			default:
				temp= 1;
				break;
		}
		if(temp == 0)
		{
			return temp;
		}
		
		if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_cob_gslX680", &val))
		{
			ctp_cob_gslX680=0;
		}
		else
		{
			ctp_cob_gslX680 = val.val;
		}

		//tag this is for engineer modify 
				
		//ctp_cob_gslX680 = 201; //engineer can modify here insmod different driver
		 
		printk("ctp_cob_gslX680=%d\n",ctp_cob_gslX680);
				
        ctp_print_info(config_info,DEBUG_INIT);
 
        twi_id = config_info.twi_id;
        screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
        revert_x_flag = config_info.revert_x_flag;
        revert_y_flag = config_info.revert_y_flag;
        exchange_x_y_flag = config_info.exchange_x_y_flag;
        if((twi_id == 0) || (screen_max_x == 0) || (screen_max_y == 0)){
                printk("%s:read config error!\n",__func__);
                return 0;
        }
        return 1;
}
static int __init gsl_ts_init(void)
{
	if(m_inet_ctpState==1)
	{
		printk("m_inet_ctpState return true,just return \n");
		return -ENODEV;
	}
	else
	{
		printk("m_inet_ctpState return false,continue deteck \n");
	}

	printk("%s\n",GSLX680_IC_INFO);
	
	printk("[FUN]%s\n", __func__);
	
	int ret = -1;

	if(!ctp_get_system_config()){
                printk("%s:read config fail!\n",__func__);
                return ret;
        }
	
	gsl_ts_driver.detect = ctp_detect;
	ret = i2c_add_driver(&gsl_ts_driver);
	printk("****************************************************************\n");
	return ret;
}

static void __exit gsl_ts_exit(void)
{
	printk("==gsl_ts_exit==\n");
	i2c_del_driver(&gsl_ts_driver);
	ctp_free_platform_resource();
	return;
}

module_init(gsl_ts_init);
module_exit(gsl_ts_exit);
module_param_named(ctp_debug,ctp_debug,int,S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GSLX680 touchscreen controller driver");
MODULE_AUTHOR("Guan Yuwei, guanyuwei@basewin.com");
MODULE_ALIAS("platform:gsl_ts");
