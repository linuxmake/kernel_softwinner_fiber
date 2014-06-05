/* drivers/input/touchscreen/zet622x_i2c.c
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * ZEITEC Semiconductor Co., Ltd
 * Tel: +886-3-579-0045
 * Fax: +886-3-579-9960
 * http://www.zeitecsemi.com
 */

#include <linux/input/mt.h>
//#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/pm.h>
#include <linux/earlysuspend.h>
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
#include <asm/uaccess.h>

#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/gpio.h> 
#include <mach/sys_config.h>
#include <linux/ctp.h>

//#define DOWNLOAD_ENABLE //DOWNLOAD ON OFF

#include "zet622x_fw.h"

#define ZET622X_I2C_ADDR 0x76

#define ZET622X_USED     "\n \
												 \n+++++++++++++++++++++++++++++++++ \
                         \n++++++    ZET622X  used +++++++++ \
                         \n+++++++++++++++++++++++++++++++++ \
                         \n"
                            
#define ZET622X_IC_INFO  "\n============================================================== \
											   \nIC     :ZET622X \
                         \nAUTHOR :mbgalex@163.com \
                         \nVERSION:2013-01-30_13:14\n"  
                         
extern int m_inet_ctpState;

static u32 firmware_len;
static unsigned char *zeitec_zet622x_firmware;
static int ctp_zet_upgrade = 0;

extern struct ctp_config_info config_info;
static const unsigned short normal_i2c[2] = {ZET622X_I2C_ADDR,I2C_CLIENT_END};

/***A31**/
#define CTP_IRQ_NUMBER                  (config_info.irq_gpio_number)
#define CTP_IRQ_MODE					(TRIG_EDGE_NEGATIVE)

static u32 int_handle = 0;

static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static int ctp_cob;
/*****/


#define TS_RESET_LOW_PERIOD		(1)
#define TS_INITIAL_HIGH_PERIOD		(30)
#define TS_WAKEUP_LOW_PERIOD	(10)
#define TS_WAKEUP_HIGH_PERIOD	(20)
#define TS_POLL_DELAY			(10)	/* ms delay between samples */
#define TS_POLL_PERIOD			(10)	/* ms delay between samples */
#define PRESS_MAX			(255)
//#define  PRINT_POINT_INFO
#ifdef PRINT_POINT_INFO 
#define print_point_info(fmt, args...)   \
        do{                              \
                pr_info(fmt, ##args);     \
        }while(0)
#else
#define print_point_info(fmt, args...)   //
#endif

//#define PRINT_INT_INFO
#ifdef PRINT_INT_INFO 
#define print_int_info(fmt, args...)     \
        do{                              \
                pr_info(fmt, ##args);     \
        }while(0)
#else
#define print_int_info(fmt, args...)   //
#endif


extern int enable_cmd;
extern int disable_cmd;
//int enable_cmd;
//int disable_cmd;

static u8 ChargeChange = 0;//discharge

void ts_write_charge_enable_cmd(void);
void ts_write_charge_disable_cmd(void);

struct timer_list write_timer; //  write_cmd

static struct i2c_client *this_client; // 

struct i2c_dev{
struct list_head list;	
struct i2c_adapter *adap;
struct device *dev;
};

s32 zet622x_i2c_write_tsdata(struct i2c_client *client, u8 *data, u8 length);
static struct class *i2c_dev_class;
static LIST_HEAD (i2c_dev_list);
static DEFINE_SPINLOCK(i2c_dev_list_lock);

#define I2C_MINORS 	256
//#define I2C_MAJOR 	125
#define I2C_MAJOR 	126 
//static int	int_cfg_addr[]={PIO_INT_CFG0_OFFSET,PIO_INT_CFG1_OFFSET,
//			PIO_INT_CFG2_OFFSET, PIO_INT_CFG3_OFFSET};
/* Addresses to scan */
static __u32 twi_id = 0;

/* -------------- global variable definition -----------*/
#define _MACH_MSM_TOUCH_H_

#define ZET_TS_ID_NAME "zet622x-ts"

//#define MJ5_TS_NAME "zet622x_touchscreen"
#define MJ5_TS_NAME	ZET_TS_ID_NAME 

#define RSTPIN_ENABLE
#define TPINFO	1
#define VIRTUAL_KEY
//#define X_MAX	1536
//#define Y_MAX	832
//#define X_MAX	1216
//#define Y_MAX	704

#define MT_TYPE_B

#define Y_MAX 480
#define X_MAX 800
#define TP_AA_X_MAX	1041
#define TP_AA_Y_MAX	640

//#define FINGER_NUMBER 4
#define FINGER_NUMBER 5
#define KEY_NUMBER 0	//please assign correct default key number 0/8 
//#define KEY_NUMBER 8  
#define DEBOUNCE_NUMBER	    1
#define P_MAX	1
#define D_POLLING_TIME	25000
#define U_POLLING_TIME	25000
#define S_POLLING_TIME  100
#define REPORT_POLLING_TIME  5

#define MAX_KEY_NUMBER      	8
#define MAX_FINGER_NUMBER	16
#define TRUE 		1
#define FALSE 		0

#define debug_mode 0
#define DPRINTK(fmt,args...)	do { if (debug_mode) printk(KERN_EMERG "[%s][%d] "fmt"\n", __FUNCTION__, __LINE__, ##args);} while(0)
//#define DPRINTK(fmt,args...)	printk(KERN_EMERG "[%s][%d] "fmt"\n", __FUNCTION__, __LINE__, ##args)

//#define TRANSLATE_ENABLE 1
#define TOPRIGHT 	0
#define TOPLEFT  	1
#define BOTTOMRIGHT	2
#define BOTTOMLEFT	3
#define ORIGIN		BOTTOMRIGHT

//Jack.wu 03/02/2012 for debug INT
#define gpio_range               (0x400)
#define ph2_ctrl_offset          0x104
#define ph_data_offset           0x10c
 __u32 temp_data1,iii;
//Jack.wu 03/02/2012 for debug INT

struct msm_ts_platform_data {
	unsigned int x_max;
	unsigned int y_max;
	unsigned int pressure_max;
};

struct zet622x_tsdrv {
	struct i2c_client *i2c_ts;
	struct work_struct work1;
	struct work_struct work2; //  write_cmd
	struct workqueue_struct *ts_workqueue; // 
	struct workqueue_struct *ts_workqueue1; //write_cmd
	struct input_dev *input;
	struct timer_list polling_timer;
	struct early_suspend early_suspend;
	unsigned int gpio; /* GPIO used for interrupt of TS1*/
	unsigned int irq;
	unsigned int x_max;
	unsigned int y_max;
	unsigned int pressure_max;
};

static u16 polling_time = S_POLLING_TIME;

static int __devinit zet622x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int __devexit zet622x_ts_remove(struct i2c_client *dev);

static unsigned char zeitec_zet622x_page[131] __initdata;
static unsigned char zeitec_zet622x_page_in[131] __initdata;

//static int filterCount = 0; 
//static u32 filterX[MAX_FINGER_NUMBER][2], filterY[MAX_FINGER_NUMBER][2]; 

static u8  key_menu_pressed = 0x0; //0x1;
static u8  key_back_pressed = 0x0; //0x1;
static u8  key_home_pressed = 0x0; //0x1;
static u8  key_search_pressed = 0x0; //0x1;

static u16 ResolutionX=X_MAX;
static u16 ResolutionY=Y_MAX;
static u16 FingerNum=0;
static u16 KeyNum=0;
static int bufLength=0;	
static u8 inChargerMode=0;
static u8 xyExchange=0;
static int f_up_cnt=0;
static u8 pc[8];
static u16 fb[8] = {0x3DF1,0x3DF4,0x3DF7,0x3DFA,0x3EF6,0x3EF9,0x3EFC,0x3EFF}; 
static u8 ic_model = 0;	// 0:6221 / 1:6223
static u16 fb21[8] = {0x3DF1,0x3DF4,0x3DF7,0x3DFA,0x3EF6,0x3EF9,0x3EFC,0x3EFF}; 
static u16 fb23[8] = {0x7BFC,0x7BFD,0x7BFE,0x7BFF,0x7C04,0x7C05,0x7C06,0x7C07};
static u8 vkEnable=0;
static u8 vkMode=0;
static u8 vkVersion=0;
static int tp_va_x_max= 1041;
static int tp_va_y_max= 640;

static struct i2c_client *this_client;

static int resetCount = 0;          //albert++ 20120807

//Touch Screen
static const struct i2c_device_id zet622x_ts_idtable[] = {
       { ZET_TS_ID_NAME, 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, zet622x_ts_idtable);

static struct i2c_driver zet622x_ts_driver = {
	.class = I2C_CLASS_HWMON,// 
	.probe	  = zet622x_ts_probe,
	.remove		= __devexit_p(zet622x_ts_remove),
	.id_table = zet622x_ts_idtable,
	.driver = {
		.owner = THIS_MODULE,
		.name  = ZET_TS_ID_NAME,
	},
	.address_list	= normal_i2c, // 
};


///////////////



/**
 * ctp_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
            return -ENODEV;

	if(twi_id == adapter->nr)
	{
		msleep(500);
		pr_info("%s: Detected chip %s at adapter %d, address 0x%02x\n",
			 __func__, ZET_TS_ID_NAME, i2c_adapter_id(adapter), client->addr);

		strlcpy(info->type, ZET_TS_ID_NAME, I2C_NAME_SIZE);
		return 0;
	}else{
		return -ENODEV;
	}
}

static struct i2c_dev *get_free_i2c_dev(struct i2c_adapter *adap) 
{
	struct i2c_dev *i2c_dev;

	if (adap->nr >= I2C_MINORS){
		pr_info("i2c-dev:out of device minors (%d) \n",adap->nr);
		return ERR_PTR (-ENODEV);
	}

	i2c_dev = kzalloc(sizeof(*i2c_dev), GFP_KERNEL);
	if (!i2c_dev){
		return ERR_PTR(-ENOMEM);
	}
	i2c_dev->adap = adap;

	spin_lock(&i2c_dev_list_lock);
	list_add_tail(&i2c_dev->list, &i2c_dev_list);
	spin_unlock(&i2c_dev_list_lock);
	
	return i2c_dev;
}

static struct i2c_dev *i2c_dev_get_by_minor(unsigned index)
{
	struct i2c_dev *i2c_dev;
	spin_lock(&i2c_dev_list_lock);
	
	list_for_each_entry(i2c_dev,&i2c_dev_list,list){
		pr_info("--line = %d ,i2c_dev->adapt->nr = %d,index = %d.\n",__LINE__,i2c_dev->adap->nr,index);
		if(i2c_dev->adap->nr == index){
		     goto found;
		}
	}
	i2c_dev = NULL;
	
found: 
	spin_unlock(&i2c_dev_list_lock);
	
	return i2c_dev ;
}

///////////////

/***********************************************************************
    [function]: 
		        callback: Timer Function if there is no interrupt fuction;
    [parameters]:
			    arg[in]:  arguments;
    [return]:
			    NULL;
************************************************************************/

static void polling_timer_func(unsigned long arg)
{

	struct zet622x_tsdrv *ts_drv = (struct zet622x_tsdrv *)arg;
	queue_work(ts_drv->ts_workqueue1, &ts_drv->work2);
	mod_timer(&ts_drv->polling_timer,jiffies + msecs_to_jiffies(polling_time));	
}

static void write_cmd_work(struct work_struct *_work)
{
	if(enable_cmd != ChargeChange)
	{	
		if(enable_cmd == 1) {
			ts_write_charge_enable_cmd();
			
		}else if(enable_cmd == 0)
		{
			ts_write_charge_disable_cmd();
		}
		ChargeChange = enable_cmd;
	}

}

/***********************************************************************
    [function]: 
		        callback: read data by i2c interface;
    [parameters]:
			    client[in]:  struct i2c_client ??represent an I2C slave device;
			    data [out]:  data buffer to read;
			    length[in]:  data length to read;
    [return]:
			    Returns negative errno, else the number of messages executed;
************************************************************************/
s32 zet622x_i2c_read_tsdata(struct i2c_client *client, u8 *data, u8 length)
{
	struct i2c_msg msg;
	msg.addr = client->addr;
	msg.flags = I2C_M_RD;
	msg.len = length;
	msg.buf = data;
	return i2c_transfer(client->adapter,&msg, 1);
}

/***********************************************************************
    [function]: 
		        callback: write data by i2c interface;
    [parameters]:
			    client[in]:  struct i2c_client ??represent an I2C slave device;
			    data [out]:  data buffer to write;
			    length[in]:  data length to write;
    [return]:
			    Returns negative errno, else the number of messages executed;
************************************************************************/
s32 zet622x_i2c_write_tsdata(struct i2c_client *client, u8 *data, u8 length)
{
	struct i2c_msg msg;
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = length;
	msg.buf = data;
	return i2c_transfer(client->adapter,&msg, 1);
}

/***********************************************************************
    [function]: 
		        callback: coordinate traslating;
    [parameters]:
			    px[out]:  value of X axis;
			    py[out]:  value of Y axis;
				p [in]:   pressed of released status of fingers;
    [return]:
			    NULL;
************************************************************************/
void touch_coordinate_traslating(u32 *px, u32 *py, u8 p)
{
	int i;
	u8 pressure;

	#if ORIGIN == TOPRIGHT
	for(i=0;i<MAX_FINGER_NUMBER;i++){
		pressure = (p >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		if(pressure)
		{
			px[i] = X_MAX - px[i];
		}
	}
	#elif ORIGIN == BOTTOMRIGHT
	for(i=0;i<MAX_FINGER_NUMBER;i++){
		pressure = (p >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		if(pressure)
		{
			px[i] = X_MAX - px[i];
			py[i] = Y_MAX - py[i];
		}
	}
	#elif ORIGIN == BOTTOMLEFT
	for(i=0;i<MAX_FINGER_NUMBER;i++){
		pressure = (p >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		if(pressure)
		{
			py[i] = Y_MAX - py[i];
		}
	}
	#endif
}

/***********************************************************************
    [function]: 
		        callback: read finger information from TP;
    [parameters]:
    			client[in]:  struct i2c_client ??represent an I2C slave device;
			    x[out]:  values of X axis;
			    y[out]:  values of Y axis;
			    z[out]:  values of Z axis;
				pr[out]:  pressed of released status of fingers;
				ky[out]:  pressed of released status of keys;
    [return]:
			    Packet ID;
************************************************************************/
u8 zet622x_ts_get_xy_from_panel(struct i2c_client *client, u32 *x, u32 *y, u32 *z, u32 *pr, u32 *ky)
{
	u8  ts_data[70];
	int ret;
	int i;
	
	memset(ts_data,0,70);

	ret=zet622x_i2c_read_tsdata(client, ts_data, bufLength);
	
	*pr = ts_data[1];
	*pr = (*pr << 8) | ts_data[2];
		
	for(i=0;i<FingerNum;i++)
	{
		x[i]=(u8)((ts_data[3+4*i])>>4)*256 + (u8)ts_data[(3+4*i)+1];
		y[i]=(u8)((ts_data[3+4*i]) & 0x0f)*256 + (u8)ts_data[(3+4*i)+2];
		z[i]=(u8)((ts_data[(3+4*i)+3]) & 0x0f);
	}
		
	//if key enable
	if(KeyNum > 0)
		*ky = ts_data[3+4*FingerNum];

	return ts_data[0];
}

/***********************************************************************
    [function]: 
		        callback: get dynamic report information;
    [parameters]:
    			client[in]:  struct i2c_client ??represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet622x_ts_get_report_mode(struct i2c_client *client)
{
	u8 ts_report_cmd[1] = {178};
	//u8 ts_reset_cmd[1] = {176};
	u8 ts_in_data[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int ret;
	int i;
	
	ret=zet622x_i2c_write_tsdata(client, ts_report_cmd, 1);

	if (ret > 0)
	{
			//udelay(10);
			msleep(10);
			printk ("=== zet622x_ts_get_report_mode ===\n");
			ret=zet622x_i2c_read_tsdata(client, ts_in_data, 17);
			
			if(ret > 0)
			{
				
				for(i=0;i<8;i++)
				{
					pc[i]=ts_in_data[i] & 0xff;
					printk("pc[%d]=0x%x\n",i,pc[i]);
				}

				xyExchange = (ts_in_data[16] & 0x8) >> 3;
				if(xyExchange == 1)
				{
					ResolutionY= ts_in_data[9] & 0xff;
					ResolutionY= (ResolutionY << 8)|(ts_in_data[8] & 0xff);
					ResolutionX= ts_in_data[11] & 0xff;
					ResolutionX= (ResolutionX << 8) | (ts_in_data[10] & 0xff);
				}
				else
				{
					ResolutionX = ts_in_data[9] & 0xff;
					ResolutionX = (ResolutionX << 8)|(ts_in_data[8] & 0xff);
					ResolutionY = ts_in_data[11] & 0xff;
					ResolutionY = (ResolutionY << 8) | (ts_in_data[10] & 0xff);
				}
					
				FingerNum = (ts_in_data[15] & 0x7f);
				KeyNum = (ts_in_data[15] & 0x80);
				inChargerMode = (ts_in_data[16] & 0x2) >> 1;

				if(KeyNum==0)
					bufLength  = 3+4*FingerNum;
				else
					bufLength  = 3+4*FingerNum+1;
				
			}
			else
			{
				printk ("===zet622x_ts_get_report_mode READ ERROR ===\n");
				return ret;
			}
							
	}
	else
	{
		return ret;
	}
	return 1;
}


/***********************************************************************
    [function]: 
		        callback: interrupt function;
    [parameters]:
    			irq[in]:  irq value;
    			dev_id[in]: dev_id;

    [return]:
			    NULL;
************************************************************************/

static u32 zet622x_ts_interrupt(struct zet622x_tsdrv *ts_drv)
{

	//print_int_info("==========------zet622x_ts TS Interrupt-----============\n"); 
	//if(!ctp_ops.judge_int_occur()){
		//print_int_info("==IRQ_EINT21=  %d\n",gpio_read_one_pin_value(gpio_int_hdle_read, NULL));
	//	ctp_ops.clear_penirq();
//		schedule_work(&ts_drv->work1);
		queue_work(ts_drv->ts_workqueue, &ts_drv->work1);

	//}
	return 0;
}

/***********************************************************************
    [function]: 
		        callback: touch information handler;
    [parameters]:
    			_work[in]:  struct work_struct;

    [return]:
			    NULL;
************************************************************************/
static void zet622x_ts_work(struct work_struct *_work)
{
	u32 x[MAX_FINGER_NUMBER], y[MAX_FINGER_NUMBER], z[MAX_FINGER_NUMBER], pr, ky, points;
	u32 px,py,pz;
	u8 ret;
	u8 pressure;
	int i;
	
	struct zet622x_tsdrv *ts =
		container_of(_work, struct zet622x_tsdrv, work1);
		
	struct i2c_client *tsclient1 = ts->i2c_ts;

	if (bufLength == 0)
	{
		return;
	}
	
	if(resetCount == 1)
	{
		resetCount = 0;
		return;
	}

#if 0	
	// be sure to get coordinate data when INT=LOW
	if( gpio_read_one_pin_value(gpio_int_hdle, NULL) != 0)
	{
		return;
	}
#endif

	ret = zet622x_ts_get_xy_from_panel(tsclient1, x, y, z, &pr, &ky);
	
	//write_cmd_work();

	if(ret == 0x3C)
	{

		//DPRINTK( " [KY] = %02X\n", ky);
		
		points = pr;
		
		#if defined(TRANSLATE_ENABLE)
		touch_coordinate_traslating(x, y, points);
		#endif
		
		if(points == 0)
		{
			f_up_cnt++;
			if(f_up_cnt>=DEBOUNCE_NUMBER)
			{
			//printk("=========finger up==========\n");
				f_up_cnt = 0;
				#ifdef MT_TYPE_B
				for(i=0;i<FingerNum;i++){
					input_mt_slot(ts->input, i);
					input_mt_report_slot_state(ts->input, MT_TOOL_FINGER,false);
					input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
				}
				input_mt_report_pointer_emulation(ts->input, true);
				#else
				input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 0);
				input_mt_sync(ts->input);
				#endif
			}
			goto no_finger;
		}
#if defined(VIRTUAL_KEY)
		KeyNum = 4;
		if(points == 0x8000 && vkEnable == 0xf1) // only finger 1 enable. 
		{

			switch(vkMode)
			{
				case 0:
					if(x[0] > tp_va_x_max)
					{
						goto key_pos;
					}else
					{
						goto finger_pos;
					}
					
					break;
				case 1:
					if(y[0] > tp_va_y_max)
					{
						goto key_pos;
					}else
					{
						goto finger_pos;
					}
					
					break;
				default:
					goto finger_pos;
					break;
			}
			
key_pos:
			switch(vkVersion)
			{
				case 0:

					if( x[0]>=1092 && x[0]<=1159 && y[0]>=69 && y[0]<=136 )
					{
						//printk("ky=0x2\n");
						ky=0x2;
					}else if(x[0]>=1092 && x[0]<=1159 && y[0]>=280 && y[0]<=348 ) 
					{
						//printk("ky=0x4\n");
						ky=0x4;
					}else if(x[0]>=1092 && x[0]<=1159 && y[0]>=492 && y[0]<=559 ) 
					{
						//printk("ky=0x8\n");
						ky=0x8;
					}

					goto Key_finger;

					break;
				default:

					break;
			}			
			
		}
#endif

finger_pos:
		f_up_cnt = 0;
		for(i=0;i<FingerNum;i++){
			pressure = (points >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		 // DPRINTK( "valid=%04X pressure[%d]= %d x= %d y= %d\n",points , i, pressure,x[i],y[i]);

			if(pressure)
			{
				px = x[i];
				py = y[i];
				pz = z[i];
				
#if defined(VIRTUAL_KEY)
if(vkEnable == 0xf1)
{
				switch(vkMode)
				{
					case 0:
						if(px > tp_va_x_max)    
							continue;
						break;
					case 1:
						if(py > tp_va_y_max)    
							continue;
						break;
					default:
						break;
				}
}
#endif

#ifdef MT_TYPE_B
				input_mt_slot(ts->input, i);
				input_mt_report_slot_state(ts->input, MT_TOOL_FINGER,true);
#endif

				input_report_abs(ts->input, ABS_MT_TRACKING_ID, i);
	    		input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, P_MAX);
	    		//input_report_abs(ts->input, ABS_MT_POSITION_X, x[i]);
	    		//input_report_abs(ts->input, ABS_MT_POSITION_Y, y[i]);
	    		input_report_abs(ts->input, ABS_MT_POSITION_X, px);
	    		input_report_abs(ts->input, ABS_MT_POSITION_Y, py);
	    		
#ifndef MT_TYPE_B	    		
				input_mt_sync(ts->input);
#endif

			}else
			{
				#ifdef MT_TYPE_B
				input_mt_slot(ts->input, i);
				input_mt_report_slot_state(ts->input, MT_TOOL_FINGER,false);
				input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
				#endif		
				
			}

#ifdef MT_TYPE_B
		input_mt_report_pointer_emulation(ts->input, true);
#endif

		}

Key_finger:
no_finger:
		if(KeyNum > 0)
		{
			for(i=0;i<MAX_KEY_NUMBER;i++)
			{			
				pressure = ky & ( 0x01 << i );
				switch(i)
				{
					case 0:
						if(pressure)
						{
							if(!key_search_pressed)
							{
                                input_report_key(ts->input, KEY_SEARCH, 1);
                                key_search_pressed = 0x1;
							}
						}else
						{
							if(key_search_pressed)
							{
                                input_report_key(ts->input, KEY_SEARCH, 0);
                                key_search_pressed = 0x0;
							}
						}
						
						break;
					case 1:
						if(pressure)
						{
							if(!key_back_pressed)
							{
                                input_report_key(ts->input, KEY_BACK, 1);
                                key_back_pressed = 0x1;
							}
						}else
						{
							if(key_back_pressed)
							{
                                input_report_key(ts->input, KEY_BACK, 0);
                                key_back_pressed = 0x0;
							}
						}
						
						break;
					case 2:
						if(pressure)
						{
							if(!key_home_pressed)
							{
                                input_report_key(ts->input, KEY_HOME, 1);
                                key_home_pressed = 0x1;
							}
						}else
						{
							if(key_home_pressed)
							{
                                input_report_key(ts->input, KEY_HOME, 0);
                                key_home_pressed = 0x0;
							}
						}
						
						break;
					case 3:
						if(pressure)
						{
							if(!key_menu_pressed)
							{
                                input_report_key(ts->input, KEY_MENU, 1);
                                key_menu_pressed = 0x1;
							}
						}else
						{
							if(key_menu_pressed)
							{
                                input_report_key(ts->input, KEY_MENU, 0);
                                key_menu_pressed = 0x0;
							}
						}
						break;
					case 4:
						break;
					case 5:
						break;
					case 6:
						break;
					case 7:
						break;
				}

			}
		}

		input_sync(ts->input);		
	}

}

void ts_write_charge_enable_cmd(void)
{
	u8 ts_write_charge_cmd[1] = {0xb5}; 
	int ret=0;
	
	printk("%s is running ==========",__FUNCTION__);
	ret=zet622x_i2c_write_tsdata(this_client, ts_write_charge_cmd, 1);
}
EXPORT_SYMBOL_GPL(ts_write_charge_enable_cmd);

void ts_write_charge_disable_cmd(void)
{
	u8 ts_write_cmd[1] = {0xb6}; 
	int ret=0;
	
	printk("%s is running ==========",__FUNCTION__);
	ret=zet622x_i2c_write_tsdata(this_client, ts_write_cmd, 1);
}
EXPORT_SYMBOL_GPL(ts_write_charge_disable_cmd);

static void ts_early_suspend(struct early_suspend *handler)
{
	//Sleep Mode
#if 1
	u8 ts_sleep_cmd[1] = {0xb1}; 
	int ret=0;
	ret=zet622x_i2c_write_tsdata(this_client, ts_sleep_cmd, 1);
#endif

	return;	        
}

static void ts_late_resume(struct early_suspend *handler)
{
#if 1
	resetCount = 1;
	//ctp_ops.ts_wakeup();
	
	/*
	gpio_status = sw_gpio_getcfg(config_info.wakeup_gpio_number);
    if(gpio_status != 1){
        sw_gpio_setcfg(config_info.wakeup_gpio_number,1);
    }
    __gpio_set_value(config_info.wakeup_gpio_number, 1);
    msleep(10);
    __gpio_set_value(config_info.wakeup_gpio_number, 0);
    msleep(10);
    __gpio_set_value(config_info.wakeup_gpio_number, 1);
    msleep(20);
	
	msleep(100);
	*/
	ctp_wakeup_inet(1,10);
  ctp_wakeup_inet(0,10);
  ctp_wakeup_inet(1,120);
	
#else
	struct zet622x_tsdrv *zet622x_ts;
	u8 ts_wakeup_cmd[1] = {0xb4};
	zet622x_i2c_write_tsdata(this_client, ts_wakeup_cmd, 1);
	
#endif

	ChargeChange = 0;

	return;
}

/*************************6221_FW*******************************/

u8 zet622x_ts_sndpwd(struct i2c_client *client)
{
	u8 ts_sndpwd_cmd[3] = {0x20,0xC5,0x9D};
	
	int ret;

	ret=zet622x_i2c_write_tsdata(client, ts_sndpwd_cmd, 3);

	return ret;
}

u8 zet622x_ts_option(struct i2c_client *client)
{

	u8 ts_cmd[1] = {0x27};
	u8 ts_cmd_erase[1] = {0x28};
	u8 ts_in_data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u8 ts_out_data[18] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int ret;
	u16 model;
	int i;
	
	printk("\noption write : "); 

	ret=zet622x_i2c_write_tsdata(client, ts_cmd, 1);

	msleep(1);
	
	printk("%02x ",ts_cmd[0]); 
	
	printk("\nread : "); 

	ret=zet622x_i2c_read_tsdata(client, ts_in_data, 16);

	msleep(1);

	for(i=0;i<16;i++)
	{
		printk("%02x ",ts_in_data[i]); 
	}
	printk("\n"); 

	model = 0x0;
	model = ts_in_data[7];
	model = (model << 8) | ts_in_data[6];
	
	switch(model) { 
        case 0xFFFF: 
        	ret = 1;
            ic_model = 0;
			for(i=0;i<8;i++)
			{
				fb[i]=fb21[i];
			}
			
#if defined(High_Impendence_Mode)

			if(ts_in_data[2] != 0xf1)
			{
			
				if(zet622x_ts_sfr(client)==0)
				{
					return 0;
				}
			
				ret=zet622x_i2c_write_tsdata(client, ts_cmd_erase, 1);
				msleep(1);
			
				for(i=2;i<18;i++)
				{
					ts_out_data[i]=ts_in_data[i-2];
				}
				ts_out_data[0] = 0x21;
				ts_out_data[1] = 0xc5;
				ts_out_data[4] = 0xf1;
			
				ret=zet622x_i2c_write_tsdata(client, ts_out_data, 18);
				msleep(1);
			
				//
				ret=zet622x_i2c_write_tsdata(client, ts_cmd, 1);

				msleep(1);
	
				printk("%02x ",ts_cmd[0]); 
	
				printk("\n(2)read : "); 

				ret=zet622x_i2c_read_tsdata(client, ts_in_data, 16);

				msleep(1);

				for(i=0;i<16;i++)
				{
					printk("%02x ",ts_in_data[i]); 
				}
				printk("\n"); 
				
			}
									
#endif					
            break; 
            
        case 0x6231:
        case 0x6223:
        	ret = 1;
			ic_model = 1;
			for(i=0;i<8;i++)
			{
				fb[i]=fb23[i];
			}
            break; 
        default: 
        	ret = 1;
			ic_model = 1;
			for(i=0;i<8;i++)
			{
				fb[i]=fb23[i];
			}
            break; 
            //ret=0; 
    } 

	return ret;
}

u8 zet622x_ts_sfr(struct i2c_client *client)
{

	u8 ts_cmd[1] = {0x2C};
	u8 ts_in_data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u8 ts_cmd17[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int ret;
	int i;
	
	printk("\nwrite : "); 

	ret=zet622x_i2c_write_tsdata(client, ts_cmd, 1);

	msleep(1);
	
	printk("%02x ",ts_cmd[0]); 
	
	printk("\nread : "); 

	ret=zet622x_i2c_read_tsdata(client, ts_in_data, 16);

	msleep(1);

	for(i=0;i<16;i++)
	{
		ts_cmd17[i+1]=ts_in_data[i];
		printk("%02x ",ts_in_data[i]); 
	}
	printk("\n"); 

#if 1
	if(ts_in_data[14]!=0x3D && ts_in_data[14]!=0x7D)
	{
		return 0;
	}
#endif

	if(ts_in_data[14]!=0x3D)
	{
		ts_cmd17[15]=0x3D;
		
		ts_cmd17[0]=0x2B;	
		
		ret=zet622x_i2c_write_tsdata(client, ts_cmd17, 17);
	}
	
	return 1;
}

u8 zet622x_ts_masserase(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x24};
	
	int ret;

	ret=zet622x_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

u8 zet622x_ts_pageerase(struct i2c_client *client,int npage)
{
	u8 ts_cmd[3] = {0x23,0x00,0x00};
	u8 len=0;
	int ret;

	switch(ic_model)
	{
		case 0: //6221
			ts_cmd[1]=npage;
			len=2;
			break;
		case 1: //6223
			ts_cmd[1]=npage & 0xff;
			ts_cmd[2]=npage >> 8;
			len=3;
			break;
	}

	ret=zet622x_i2c_write_tsdata(client, ts_cmd, len);

	return 1;
}

u8 zet622x_ts_resetmcu(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x29};
	
	int ret;

	ret=zet622x_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

u8 zet622x_ts_hwcmd(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0xB9};
	
	int ret;

	ret=zet622x_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

u8 zet622x_ts_version(void)
{	
	int i;
		
	printk("pc: ");
	for(i=0;i<8;i++)
		printk("%02x ",pc[i]);
	printk("\n");
	
	printk("src: ");
	for(i=0;i<8;i++)
		printk("%02x ",zeitec_zet622x_firmware[fb[i]]);
	printk("\n");
	
	for(i=0;i<8;i++)
		if(pc[i]!=zeitec_zet622x_firmware[fb[i]])
			return 0;
			
	return 1;
}

//support compatible
int __init zet622x_downloader( struct i2c_client *client )
{
	int BufLen=0;
	int BufPage=0;
	int BufIndex=0;
	int ret;
	int i;
	
	int nowBufLen=0;
	int nowBufPage=0;
	int nowBufIndex=0;
	int retryCount=0;
	
	int i2cLength=0;
	int bufOffset=0;
	
begin_download:
	
	//reset mcu
	/*
	gpio_status = sw_gpio_getcfg(config_info.wakeup_gpio_number);
    if(gpio_status != 1){
        sw_gpio_setcfg(config_info.wakeup_gpio_number,1);
    }
    __gpio_set_value(config_info.wakeup_gpio_number, 0);

	msleep(20);
	*/

   ctp_wakeup_inet(0,20);


	//send password
	ret=zet622x_ts_sndpwd(client);
	msleep(200);
	
	if(ret<=0)
		return ret;
		
	ret=zet622x_ts_option(client);
	msleep(200);
	
	if(ret<=0)
		return ret;

/*****compare version*******/

	//0~3
	memset(zeitec_zet622x_page_in,0x00,131);
	
	switch(ic_model)
	{
		case 0: //6221
			zeitec_zet622x_page_in[0]=0x25;
			zeitec_zet622x_page_in[1]=(fb[0] >> 7);//(fb[0]/128);
			
			i2cLength=2;
			break;
		case 1: //6223
			zeitec_zet622x_page_in[0]=0x25;
			zeitec_zet622x_page_in[1]=(fb[0] >> 7) & 0xff; //(fb[0]/128);
			zeitec_zet622x_page_in[2]=(fb[0] >> 7) >> 8; //(fb[0]/128);
			
			i2cLength=3;
			break;
	}
	
	ret=zet622x_i2c_write_tsdata(client, zeitec_zet622x_page_in, i2cLength);

	if(ret<=0)
		return ret;
	
	zeitec_zet622x_page_in[0]=0x0;
	zeitec_zet622x_page_in[1]=0x0;
	zeitec_zet622x_page_in[2]=0x0;

	ret=zet622x_i2c_read_tsdata(client, zeitec_zet622x_page_in, 128);


	if(ret<=0)
		return ret;
	
	printk("page=%d ",(fb[0] >> 7));//(fb[0]/128));
	for(i=0;i<4;i++)
	{
		pc[i]=zeitec_zet622x_page_in[(fb[i] & 0x7f)];//[(fb[i]%128)];
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f));//(fb[i]%128));
	}
	printk("\n");
	
	/*
	printk("page=%d ",(fb[0] >> 7));
	for(i=0;i<4;i++)
	{
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f));
	}
	printk("\n");
	*/
	
	//4~7
	memset(zeitec_zet622x_page_in,0x00,131);
	
	switch(ic_model)
	{
		case 0: //6221
			zeitec_zet622x_page_in[0]=0x25;
			zeitec_zet622x_page_in[1]=(fb[4] >> 7);//(fb[4]/128);
			
			i2cLength=2;
			break;
		case 1: //6223
			zeitec_zet622x_page_in[0]=0x25;
			zeitec_zet622x_page_in[1]=(fb[4] >> 7) & 0xff; //(fb[4]/128);
			zeitec_zet622x_page_in[2]=(fb[4] >> 7) >> 8; //(fb[4]/128);
			
			i2cLength=3;
			break;
	}
	
	ret=zet622x_i2c_write_tsdata(client, zeitec_zet622x_page_in, i2cLength);

	if(ret<=0)
		return ret;
	
	zeitec_zet622x_page_in[0]=0x0;
	zeitec_zet622x_page_in[1]=0x0;
	zeitec_zet622x_page_in[2]=0x0;

	ret=zet622x_i2c_read_tsdata(client, zeitec_zet622x_page_in, 128);


	printk("page=%d ",(fb[4] >> 7)); //(fb[4]/128));
	for(i=4;i<8;i++)
	{
		pc[i]=zeitec_zet622x_page_in[(fb[i] & 0x7f)]; //[(fb[i]%128)];
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f));  //(fb[i]%128));
	}
	printk("\n");
	
	if(ret<=0)
		return ret;
	

	if(zet622x_ts_version()!=0)
		goto exit_download;
		
/*****compare version*******/

//proc_sfr:
	//sfr
	if(zet622x_ts_sfr(client)==0)
	{
		//comment out to disable sfr checking loop
		return 0;
	}
	msleep(20);
	
	//comment out to enable download procedure
	//return 1;
	
	//erase
	if(BufLen==0)
	{
		//mass erase
		//DPRINTK( "mass erase\n");
		zet622x_ts_masserase(client);
		msleep(200);

		BufLen=firmware_len/sizeof(char);
	}else
	{
		zet622x_ts_pageerase(client,BufPage);
		msleep(200);
	}
	
	
	while(BufLen>0)
	{
download_page:

		memset(zeitec_zet622x_page,0x00,131);
		
		DPRINTK( "Start: write page%d\n",BufPage);
		nowBufIndex=BufIndex;
		nowBufLen=BufLen;
		nowBufPage=BufPage;
		
		switch(ic_model)
		{
			case 0: //6221
				bufOffset = 2;
				i2cLength=130;
				
				zeitec_zet622x_page[0]=0x22;
				zeitec_zet622x_page[1]=BufPage;				
				break;
			case 1: //6223
				bufOffset = 3;
				i2cLength=131;
				
				zeitec_zet622x_page[0]=0x22;
				zeitec_zet622x_page[1]=BufPage & 0xff;
				zeitec_zet622x_page[2]=BufPage >> 8;
				break;
		}
		
		if(BufLen>128)
		{
			for(i=0;i<128;i++)
			{
				zeitec_zet622x_page[i+bufOffset]=zeitec_zet622x_firmware[BufIndex];
				BufIndex+=1;
			}

			BufLen-=128;
		}
		else
		{
			for(i=0;i<BufLen;i++)
			{
				zeitec_zet622x_page[i+bufOffset]=zeitec_zet622x_firmware[BufIndex];
				BufIndex+=1;
			}

			BufLen=0;
		}
//		DPRINTK( "End: write page%d\n",BufPage);

		ret=zet622x_i2c_write_tsdata(client, zeitec_zet622x_page, i2cLength);

		msleep(200);
		
#if 1

		memset(zeitec_zet622x_page_in,0x00,131);
		switch(ic_model)
		{
			case 0: //6221
				zeitec_zet622x_page_in[0]=0x25;
				zeitec_zet622x_page_in[1]=BufPage;
				
				i2cLength=2;
				break;
			case 1: //6223
				zeitec_zet622x_page_in[0]=0x25;
				zeitec_zet622x_page_in[1]=BufPage & 0xff;
				zeitec_zet622x_page_in[2]=BufPage >> 8;

				i2cLength=3;
				break;
		}		
		
		ret=zet622x_i2c_write_tsdata(client, zeitec_zet622x_page_in, i2cLength);

		zeitec_zet622x_page_in[0]=0x0;
		zeitec_zet622x_page_in[1]=0x0;
		zeitec_zet622x_page_in[2]=0x0;
		
		ret=zet622x_i2c_read_tsdata(client, zeitec_zet622x_page_in, 128);

		
		for(i=0;i<128;i++)
		{
			if(i < nowBufLen)
			{
				if(zeitec_zet622x_page[i+bufOffset]!=zeitec_zet622x_page_in[i])
				{
					BufIndex=nowBufIndex;
					BufLen=nowBufLen;
					BufPage=nowBufPage;
				
					if(retryCount < 5)
					{
						retryCount++;
						goto download_page;
					}else
					{
						//BufIndex=0;
						//BufLen=0;
						//BufPage=0;
						retryCount=0;
						/*
						gpio_status = sw_gpio_getcfg(config_info.wakeup_gpio_number);
    					if(gpio_status != 1){
        					sw_gpio_setcfg(config_info.wakeup_gpio_number,1);
    					}
    					__gpio_set_value(config_info.wakeup_gpio_number, 1);
    					msleep(20);
    					__gpio_set_value(config_info.wakeup_gpio_number, 0);
    					msleep(1);
    					__gpio_set_value(config_info.wakeup_gpio_number, 1);
						msleep(20);
						*/
						ctp_wakeup_inet(1,20);
   					ctp_wakeup_inet(0,1);
   					ctp_wakeup_inet(1,20);
   					
						goto begin_download;
					}

				}
			}
		}
		
#endif
		retryCount=0;
		BufPage+=1;
	}

exit_download:

	zet622x_ts_resetmcu(client);
	msleep(100);

	/*
	gpio_status = sw_gpio_getcfg(config_info.wakeup_gpio_number);
    if(gpio_status != 1){
        sw_gpio_setcfg(config_info.wakeup_gpio_number,1);
    }
    __gpio_set_value(config_info.wakeup_gpio_number, 1);

	msleep(200);
	*/
	
	ctp_wakeup_inet(1,200);
	
	
	return 1;

}

static int zet622x_download_inet(struct i2c_client *client)
{
	if (ctp_cob == 0)
	{
#ifdef DOWNLOAD_ENABLE
	//zet_download:	
		printk("=====download default firmware====");
		firmware_len = sizeof(zeitec_zet622x_firmware_default);
		zeitec_zet622x_firmware = zeitec_zet622x_firmware_default;
		if(zet622x_downloader(client)<=0)
			return -1;	
			
		ctp_wakeup_inet(1,5);
	   	ctp_wakeup_inet(0,1);
	   	ctp_wakeup_inet(1,5);
#endif
	}
	else if (ctp_cob == 1)
	{
		switch (ctp_zet_upgrade)
		{
			case 3970:
				if (screen_max_x == 1024 && screen_max_y == 768)
				{
					printk("=====download default firmware====");
					firmware_len = sizeof(zeitec_zet622x_firmware_default);
					zeitec_zet622x_firmware = zeitec_zet622x_firmware_default;
				}
				else if (screen_max_x == 2048 && screen_max_y == 1536)
				{
					printk("=====download zeitec_zet622x_firmware_T970H firmware====");
					firmware_len = sizeof(zeitec_zet622x_firmware_T970H);
					zeitec_zet622x_firmware = zeitec_zet622x_firmware_T970H;
				}
				break;
			case 3100:
				if (screen_max_x == 1280 && screen_max_y == 800)
				{
					printk("=====download zeitec_zet622x_firmware_T100L firmware====");
					firmware_len = sizeof(zeitec_zet622x_firmware_T100L);
					zeitec_zet622x_firmware = zeitec_zet622x_firmware_T100L;
				}
				else if (screen_max_x == 1920 && screen_max_y == 1200)
				{
					printk("=====download zeitec_zet622x_firmware_T100H firmware====");
					firmware_len = sizeof(zeitec_zet622x_firmware_T100H);
					zeitec_zet622x_firmware = zeitec_zet622x_firmware_T100H;
				}
				break;
			default:
				printk("=====download default firmware====");
				firmware_len = sizeof(zeitec_zet622x_firmware_default);
				zeitec_zet622x_firmware = zeitec_zet622x_firmware_default;
				break;
		}
			
		if(zet622x_downloader(client)<=0)
			return -1;		
			
		ctp_wakeup_inet(1,5);
	   	ctp_wakeup_inet(0,1);
	   	ctp_wakeup_inet(1,5);
	}
	
	return 0;
}

/*************************6221_FW*******************************/


static int __devinit zet622x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int result;
	struct input_dev *input_dev;
	struct zet622x_tsdrv *zet622x_ts;
	struct i2c_dev *i2c_dev; // 
	struct device *dev;
	int err;

	zet622x_ts = kzalloc(sizeof(struct zet622x_tsdrv), GFP_KERNEL);
	zet622x_ts->i2c_ts = client;
	
	this_client = client;
	
	i2c_set_clientdata(client, zet622x_ts);

	client->driver = &zet622x_ts_driver;

	INIT_WORK(&zet622x_ts->work1, zet622x_ts_work);
	zet622x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev)); //  workqueue
	if (!zet622x_ts->ts_workqueue) {
		printk("ts_workqueue ts_probe error ==========\n");
		return -1;
	}
	
	/*   charger detect : write_cmd */
	INIT_WORK(&zet622x_ts->work2, write_cmd_work);
	zet622x_ts->ts_workqueue1 = create_singlethread_workqueue(dev_name(&client->dev)); //  workqueue
	if (!zet622x_ts->ts_workqueue1) {
	//	err = -ESRCH;
		printk("ts_workqueue1 ts_probe error ==========\n");
		return -1;
	}
	/*   charger detect : write_cmd */

	input_dev = input_allocate_device();
	if (!input_dev || !zet622x_ts) {
		result = -ENOMEM;
		goto fail_alloc_mem;
	}
	
	i2c_set_clientdata(client, zet622x_ts);

	input_dev->name = MJ5_TS_NAME;
	input_dev->phys = "zet622x_touch/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0002;
	input_dev->id.version = 0x0100;

	ic_model = 0; // 0:6221 1:6223
	
	if (zet622x_download_inet(client) < 0)
		goto exit_irq_request_failed;
	
#if defined(TPINFO)
			
		if(zet622x_ts_get_report_mode(client)<=0)
		{
			printk("zet622x_ts_get_report_mode error");
			goto exit_check_functionality_failed;
		}
		
		if(pc[3]!=0x8)  // not zeitec ic
		{
			goto exit_check_functionality_failed;
		}
			
			m_inet_ctpState=1;
			vkEnable = pc[4];
			vkMode = pc[5]; // 0: X is longer(major axis) 1: Y is longer(major axis)
			vkVersion = pc[6];
		
	
#else
	ResolutionX = X_MAX;
	ResolutionY = Y_MAX;
	FingerNum = FINGER_NUMBER;
	KeyNum = KEY_NUMBER;   
	if(KeyNum==0)
		bufLength  = 3+4*FingerNum;
	else
		bufLength  = 3+4*FingerNum+1;
#endif

	printk( "ResolutionX=%d ResolutionY=%d FingerNum=%d KeyNum=%d\n",ResolutionX,ResolutionY,FingerNum,KeyNum);

	//__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);	
	//input_mt_init_slots(input_dev, FingerNum);	

#ifdef MT_TYPE_B
	input_mt_init_slots(input_dev, FingerNum);	
#endif

    set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit); 
    set_bit(ABS_MT_POSITION_X, input_dev->absbit); 
    set_bit(ABS_MT_POSITION_Y, input_dev->absbit); 
    set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit); 
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, P_MAX, 0, 0);
#if defined(VIRTUAL_KEY)
if(vkEnable == 0xf1)
{

		switch(vkVersion)
		{
			//mbg var
			case 0:
				tp_va_x_max= 1041;
				tp_va_y_max= 640;
				break;
			default:
				tp_va_x_max= 1041;
				tp_va_y_max= 640;
				break;
		}

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, tp_va_x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, tp_va_y_max, 0, 0);
}
else
{
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ResolutionX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ResolutionY, 0, 0);
}
#else
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ResolutionX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ResolutionY, 0, 0);
#endif
	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(KEY_MENU, input_dev->keybit);
	set_bit(KEY_HOME, input_dev->keybit);
	set_bit(KEY_SEARCH, input_dev->keybit);

	input_dev->evbit[0] = BIT(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	//input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	//input_dev->keybit[BIT_WORD (KEY_HOME)] = BIT_MASK(KEY_HOME)|BIT_MASK(KEY_BACK);

	result = input_register_device(input_dev);
	if (result)
		goto fail_ip_reg;

//config early_suspend
	pr_info("==register_early_suspend =\n");
	zet622x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	zet622x_ts->early_suspend.suspend = ts_early_suspend;
	zet622x_ts->early_suspend.resume = ts_late_resume;
	register_early_suspend(&zet622x_ts->early_suspend);
//end config early_suspend

	zet622x_ts->input = input_dev;

	input_set_drvdata(zet622x_ts->input, zet622x_ts);


	setup_timer(&zet622x_ts->polling_timer, polling_timer_func, (unsigned long)zet622x_ts);
	mod_timer(&zet622x_ts->polling_timer,jiffies + msecs_to_jiffies(800));
	
	/*err = ctp_set_irq_mode("ctp_para", "ctp_int_port",  CTP_IRQ_MODE);

	if(0 != err){
		pr_info("%s:ctp_ops.set_irq_mode err. \n", __func__);
		goto exit_set_irq_mode;
	}

	err = request_irq(SW_INT_IRQNO_PIO, zet622x_ts_interrupt, IRQF_TRIGGER_FALLING | IRQF_SHARED, ZET_TS_ID_NAME, zet622x_ts);
   
    
	if (err < 0) {
		DPRINTK( "[TS] zet622x_ts_probe.request_irq failed. err=\n",err);    
		goto exit_irq_request_failed;
	}
	*/

   	int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)zet622x_ts_interrupt,zet622x_ts);
	if (!int_handle) {
		printk("zet622x_ts_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	

    i2c_dev = get_free_i2c_dev(client->adapter);	
	if (IS_ERR(i2c_dev)){	
		err = PTR_ERR(i2c_dev);		
		return err;	
	}
	dev = device_create(i2c_dev_class, &client->adapter->dev, MKDEV(I2C_MAJOR,client->adapter->nr), NULL, "zet_i2c_ts%d", client->adapter->nr);	
	if (IS_ERR(dev))	{		
			err = PTR_ERR(dev);		
			return err;	
	}

	return 0;

//request_irq_fail:
//	gpio_free(zet622x_ts->gpio);
//gpio_request_fail:
	//free_irq(zet622x_ts->irq, zet622x_ts);
	//input_unregister_device(input_dev);
	//input_dev = NULL;
fail_ip_reg:
fail_alloc_mem:
	input_free_device(input_dev);
	kfree(zet622x_ts);
	return result;

//////////////////////////////
exit_irq_request_failed:
	//enable_irq(SW_INT_IRQNO_PIO);
//exit_input_register_device_failed:
	//input_free_device(input_dev);
//exit_input_dev_alloc_failed:
	//free_irq(SW_INT_IRQNO_PIO, zet622x_ts);
//exit_create_singlethread:
	//pr_info("==singlethread error =\n");
	//i2c_set_clientdata(client, NULL);
	//kfree(zet622x_ts);
//exit_alloc_data_failed:
exit_check_functionality_failed:
	unregister_chrdev(I2C_MAJOR, " zet_i2c_ts ");
	return err;
		
}

static int __devexit zet622x_ts_remove(struct i2c_client *dev)
{
	struct zet622x_tsdrv *zet622x_ts = i2c_get_clientdata(dev);
	
	del_timer_sync(&write_timer);
	pr_info("==zet622x_ts_remove=\n");
	sw_gpio_irq_free(int_handle);
//#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&zet622x_ts->early_suspend);
//#endif
	input_unregister_device(zet622x_ts->input);
	input_free_device(zet622x_ts->input);
	destroy_workqueue(zet622x_ts->ts_workqueue); //  workqueue
	kfree(zet622x_ts);
    
	i2c_set_clientdata(dev, NULL);
	ctp_free_platform_resource();

	return 0;
}

static int aw_open(struct inode *inode, struct file *file)
{
	int subminor;
	int ret = 0;	
	struct i2c_client *client;
	struct i2c_adapter *adapter;	
	struct i2c_dev *i2c_dev;	

	pr_info("====%s======.\n", __func__);
	
	#ifdef AW_DEBUG	
	        pr_info("enter aw_open function\n");
	#endif	
	
	subminor = iminor(inode);
	#ifdef AW_DEBUG	
	      pr_info("subminor=%d\n",subminor);
	#endif	
	
	//lock_kernel();	
	i2c_dev = i2c_dev_get_by_minor(2);	
	if (!i2c_dev)	{	
		pr_info("error i2c_dev\n");		
		return -ENODEV;	
	}
	
	adapter = i2c_get_adapter(i2c_dev->adap->nr);	
	if (!adapter)	{		
		return -ENODEV;	
	}	
	
	client = kzalloc(sizeof(*client), GFP_KERNEL);	
	
	if (!client)	{		
		i2c_put_adapter(adapter);		
		ret = -ENOMEM;	
	}	
	snprintf(client->name, I2C_NAME_SIZE, "pctp_i2c_ts%d", adapter->nr);
	client->driver = &zet622x_ts_driver;
	client->adapter = adapter;		
	file->private_data = client;
		
	return 0;
}

static int aw_release (struct inode *inode, struct file *file) 
{
	struct i2c_client *client = file->private_data;
	#ifdef AW_DEBUG
	    pr_info("enter aw_release function.\n");
	#endif
	
	i2c_put_adapter(client->adapter);
	kfree(client);
	file->private_data = NULL;
	return 0;	  
		
}

static const struct file_operations zet_i2c_ts_fops ={	
	.owner = THIS_MODULE, 	
	.open = aw_open, 	
	.release = aw_release, 
};

static int ctp_get_system_config(void)
{   
		script_item_u   val;
        
        ctp_print_info(config_info,DEBUG_INIT);
        ctp_cob = config_info.ctp_cob;
        twi_id = config_info.twi_id;
        screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
        revert_x_flag = config_info.revert_x_flag;
        revert_y_flag = config_info.revert_y_flag;
        exchange_x_y_flag = config_info.exchange_x_y_flag;
        
        if (ctp_cob >= 1)
        {		
        	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_zet_upg", &val)){
	        	ctp_zet_upgrade = 0;
	        }
	        else
	        	ctp_zet_upgrade = val.val;
	        printk("ctp_zet_upgrade = %d\n",ctp_zet_upgrade);
        }
        
        if((twi_id == 0) || (screen_max_x == 0) || (screen_max_y == 0)){
                printk("%s:read config error!\n",__func__);
                return 0;
        }
        return 1;
}

static int __init zet622x_ts_init(void)
{
	int ret = -1;   
		
	if(m_inet_ctpState==1)
	{
		printk("m_inet_ctpState return true,just return \n");
		return -ENODEV;
	}
	else
	{
		printk("m_inet_ctpState return false,continue deteck \n");
	}
	
	printk("%s\n",ZET622X_IC_INFO);
	
	printk("[FUN]%s\n", __func__);
	   
	if(!ctp_get_system_config())
	{
     printk("%s:read config fail!\n",__func__);
     return ret;
  }
        
	/*ctp_wakeup(0,20);  
	gpio_status = sw_gpio_getcfg(config_info.wakeup_gpio_number);
    if(gpio_status != 1){
        sw_gpio_setcfg(config_info.wakeup_gpio_number,1);
    }
    __gpio_set_value(config_info.wakeup_gpio_number, 1);
    msleep(10);
    __gpio_set_value(config_info.wakeup_gpio_number, 0);
    msleep(10);
    __gpio_set_value(config_info.wakeup_gpio_number, 1);
    msleep(20);
   */
   ctp_wakeup_inet(1,10);
   ctp_wakeup_inet(0,10);
   ctp_wakeup_inet(1,20);
		
	zet622x_ts_driver.detect = ctp_detect;

	ret= register_chrdev(I2C_MAJOR,"zet_i2c_ts",&zet_i2c_ts_fops );
	
	if(ret) {	
		pr_info(KERN_ERR "%s:register chrdev failed\n",__FILE__);	
		return ret;
	}
	
	i2c_dev_class = class_create(THIS_MODULE,"zet_i2c_dev");
	if (IS_ERR(i2c_dev_class)) {		
		ret = PTR_ERR(i2c_dev_class);		
		class_destroy(i2c_dev_class);	
	}

	ret = i2c_add_driver(&zet622x_ts_driver);
	printk("****************************************************************\n");
	return ret;
	
}
module_init(zet622x_ts_init);

static void __exit zet622x_ts_exit(void)
{
    i2c_del_driver(&zet622x_ts_driver);
}
module_exit(zet622x_ts_exit);

void zet622x_set_ts_mode(u8 mode)
{
//	DPRINTK( "[Touch Screen]ts mode = %d \n", mode);
}
EXPORT_SYMBOL_GPL(zet622x_set_ts_mode);

//module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);

MODULE_DESCRIPTION("zet622x I2C Touch Screen driver");
MODULE_LICENSE("GPL v2");


