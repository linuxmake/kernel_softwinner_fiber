/* 
 * drivers/input/touchscreen/ft5x0x_ts.c
 *
 * FocalTech ft5x TouchScreen driver. 
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *	note: only support mulititouch	Wenfs 2010-10-01
 *  for this touchscreen to work, it's slave addr must be set to 0x7e | 0x70
 */
#include <linux/i2c.h>
#include <linux/input.h>
#include "ft5402_ts.h"
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif
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
#include <linux/device.h>
#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/gpio.h> 
#include <linux/ctp.h>
#include "ft5402_ini.h"


//#define FT5402_UPGRADE
//#define CHARGE_MODE

#ifdef CHARGE_MODE
#define S_POLLING_TIME  100

extern int enable_cmd;
//extern int disable_cmd;
//int enable_cmd;
//int disable_cmd;

static u8 ChargeChange = 0;//discharge

struct timer_list write_timer; //  write_cmd
#endif

#define FT5X_I2C_ADDR 0x38

#define FT5402_USED     "\n \
						 \n+++++++++++++++++++++++++++++++++ \
                         \n++++++    ft5402 used   +++++++++ \
                         \n+++++++++++++++++++++++++++++++++ \
                         \n"
                         
                               
#define FT5402_IC_INFO  "\n============================================================== \
						 \nIC     :Focal ft5402  \
                         \nAUTHOR :mbgalex@163.com \
                         \nVERSION:2013-04-11_00:00\n"


                       
static int CNT=0;
static int update_if= 0;
static char update_file_path[254];

#define FOR_TSLIB_TEST
//#define PRINT_INT_INFO//mbg
//#define PRINT_POINT_INFO//mbg
//#define DEBUG//mbg
//#define TOUCH_KEY_SUPPORT
#ifdef TOUCH_KEY_SUPPORT
#define TOUCH_KEY_FOR_EVB13
//#define TOUCH_KEY_FOR_ANGDA
#ifdef TOUCH_KEY_FOR_ANGDA
#define TOUCH_KEY_X_LIMIT	        (60000)
#define TOUCH_KEY_NUMBER	        (4)
#endif
#ifdef TOUCH_KEY_FOR_EVB13
#define TOUCH_KEY_LOWER_X_LIMIT	        (848)
#define TOUCH_KEY_HIGHER_X_LIMIT	(852)
#define TOUCH_KEY_NUMBER	        (5)
#endif
#endif

//#define CONFIG_SUPPORT_FTS_CTP_UPG
        
struct i2c_dev{
        struct list_head list;	
        struct i2c_adapter *adap;
        struct device *dev;
};

static struct class *i2c_dev_class;
static LIST_HEAD (i2c_dev_list);
static DEFINE_SPINLOCK(i2c_dev_list_lock);

#define FT5X_NAME	"ft5402_ts"

static struct i2c_client *this_client;

#ifdef TOUCH_KEY_SUPPORT
static int key_tp  = 0;
static int key_val = 0;
#endif
 
//#define FT5402_CONFIG_INI
static int chip_type = 3;//FT芯片类型

#define FT5402_CONFIG_NAME "fttpconfig_5402public.ini"

extern int ft5402_Init_IC_Param(struct i2c_client * client);
extern int ft5402_get_ic_param(struct i2c_client * client);
extern int ft5402_Get_Param_From_Ini(char *config_name);

//#define CONFIG_SUPPORT_FTS_CTP_UPG

//#define SYSFS_DEBUG
#ifdef SYSFS_DEBUG
	static struct mutex g_device_mutex;
	static int ft5402_create_sysfs_debug(struct i2c_client *client);
#endif

//#define FTS_APK_DEBUG//mbg test
#ifdef FTS_APK_DEBUG
	int ft5402_create_apk_debug_channel(struct i2c_client *client);
	void ft5402_release_apk_debug_channel(void);
#endif


#ifdef PRINT_POINT_INFO 
#define print_point_info(fmt, args...)   \
        do{                              \
                pr_info(fmt, ##args);     \
        }while(0)
#else
#define print_point_info(fmt, args...)   //
#endif

#ifdef PRINT_INT_INFO 
#define print_int_info(fmt, args...)     \
        do{                              \
                pr_info(fmt, ##args);     \
        }while(0)
#else
#define print_int_info(fmt, args...)   //
#endif

/*********************************************************************************************/
#define CTP_IRQ_NUMBER                  (config_info.irq_gpio_number)
#define CTP_IRQ_MODE			(TRIG_EDGE_NEGATIVE)
#define SCREEN_MAX_X			(screen_max_x)
#define SCREEN_MAX_Y			(screen_max_y)
#define CTP_NAME			 FT5X_NAME
#define PRESS_MAX			(255)

static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;
static __u32 twi_id = 0;
static bool is_suspend = false;

extern struct ctp_config_info config_info;

extern int m_inet_ctpState;
//extern int inet_ft5x_flag;

static u32 debug_mask = 0;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("***CTP***"fmt, ## arg)
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);
/*********************************************************************************************/
/*------------------------------------------------------------------------------------------*/        
/* Addresses to scan */
static const unsigned short normal_i2c[2] = {FT5X_I2C_ADDR,I2C_CLIENT_END};
#define CHIP_ID_VALUE_NUM 1
static const int chip_id_value[CHIP_ID_VALUE_NUM] = {0x03};

static void ft5x_resume_events(struct work_struct *work);
struct workqueue_struct *ft5x_resume_wq;
static DECLARE_WORK(ft5x_resume_work, ft5x_resume_events);
/*------------------------------------------------------------------------------------------*/ 

static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
        int ret = 0, i = 0;
        
        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;
    
	if(twi_id == adapter->nr)
	{
		
		  msleep(500);
	      ret = i2c_smbus_read_byte_data(client,0xA3);
  
          printk("addr:0x%x,chip_id_value:0x%x\n",client->addr,ret);
		  
          for(i =0;i<CHIP_ID_VALUE_NUM;i++)//while(chip_id_value[i++])
          {
             if(ret == chip_id_value[i])//if(ret == chip_id_value[i - 1])
             {
             	//detectFlag = 1;
             	 m_inet_ctpState = 1;
             	 printk("%s",FT5402_USED);
            	 strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
    		       return 0;
             }                   
          }
		  
          printk("%s:I2C connection might be something wrong ! \n",__func__);
          return -ENODEV;

	}
	else
	{
		return -ENODEV;
	}
}

//int fts_ctpm_fw_upgrade_with_i_file(void);

static struct i2c_dev *i2c_dev_get_by_minor(unsigned index)
{
	struct i2c_dev *i2c_dev;
	spin_lock(&i2c_dev_list_lock);
	
	list_for_each_entry(i2c_dev,&i2c_dev_list,list){
		dprintk(DEBUG_OTHERS_INFO,"--line = %d ,i2c_dev->adapt->nr = %d,index = %d.\n",\
		        __LINE__,i2c_dev->adap->nr,index);
		if(i2c_dev->adap->nr == index){
		     goto found;
		}
	}
	i2c_dev = NULL;
	
found: 
	spin_unlock(&i2c_dev_list_lock);
	
	return i2c_dev ;
}

static struct i2c_dev *get_free_i2c_dev(struct i2c_adapter *adap) 
{
	struct i2c_dev *i2c_dev;

	if (adap->nr >= I2C_MINORS){
		dprintk(DEBUG_OTHERS_INFO,"i2c-dev:out of device minors (%d) \n",adap->nr);
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


static int ft5x_i2c_rxdata(char *rxdata, int length);

struct ts_event {
	u16	x1;
	u16	y1;
	u16	x2;
	u16	y2;
	u16	x3;
	u16	y3;
	u16	x4;
	u16	y4;
	u16	x5;
	u16	y5;
	u16	x6;
	u16	y6;
	u16	x7;
	u16	y7;
	u16	x8;
	u16	y8;
	u16	x9;
	u16	y9;
	u16	x10;
	u16	y10;
	u16	pressure;
	s16 touch_ID1;
	s16 touch_ID2;
	s16 touch_ID3;
	s16 touch_ID4;
	s16 touch_ID5;
	s16 touch_ID6;
	s16 touch_ID7;
	s16 touch_ID8;
	s16 touch_ID9;
	s16 touch_ID10;
    u8  touch_point;
};

struct ft5x_ts_data {
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
#ifdef CHARGE_MODE
	struct work_struct 	pen_event_work_charge;
#endif
	struct workqueue_struct *ts_workqueue;
#ifdef CHARGE_MODE
	struct workqueue_struct *ts_workqueue_charge;
	struct timer_list polling_timer;
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
};
#ifdef CHARGE_MODE
static u16 polling_time = S_POLLING_TIME;
#endif

/* ---------------------------------------------------------------------
*
*   Focal Touch panel upgrade related driver
*
*
----------------------------------------------------------------------*/

typedef enum
{
	ERR_OK,
	ERR_MODE,
	ERR_READID,
	ERR_ERASE,
	ERR_STATUS,
	ERR_ECC,
	ERR_DL_ERASE_FAIL,
	ERR_DL_PROGRAM_FAIL,
	ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;

typedef unsigned char         FTS_BYTE;     //8 bit
typedef unsigned short        FTS_WORD;    //16 bit
typedef unsigned int          FTS_DWRD;    //16 bit
typedef unsigned char         FTS_BOOL;    //8 bit 

#define FTS_NULL                0x0
#define FTS_TRUE                0x01
#define FTS_FALSE               0x0

#define I2C_CTPM_ADDRESS        (0x70>>1)

static void delay_ms(FTS_WORD  w_ms)
{
	//platform related, please implement this function
	msleep( w_ms );
}

/*
*ft5402_i2c_Read-read data and write data by i2c
*@client: handle of i2c
*@writebuf: Data that will be written to the slave
*@writelen: How many bytes to write
*@readbuf: Where to store data read from slave
*@readlen: How many bytes to read
*
*Returns negative errno, else the number of messages executed
*
*
*/
int ft5402_i2c_Read(struct i2c_client *client,  char * writebuf, int writelen, 
							char *readbuf, int readlen)
{
	int ret;

	if(writelen > 0)
	{
		struct i2c_msg msgs[] = {
			{
				.addr	= client->addr,
				.flags	= 0,
				.len	= writelen,
				.buf	= writebuf,
			},
			{
				.addr	= client->addr,
				.flags	= I2C_M_RD,
				.len	= readlen,
				.buf	= readbuf,
			},
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			pr_err("function:%s. i2c read error: %d\n", __func__, ret);
	}
	else
	{
		struct i2c_msg msgs[] = {
			{
				.addr	= client->addr,
				.flags	= I2C_M_RD,
				.len	= readlen,
				.buf	= readbuf,
			},
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			pr_err("function:%s. i2c read error: %d\n", __func__, ret);
	}
	return ret;
}
/*
*write data by i2c 
*/
int ft5402_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= writelen,
			.buf	= writebuf,
		},
	};

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

int ft5402_write_reg(struct i2c_client * client, u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = {0};
	buf[0] = regaddr;
	buf[1] = regvalue;

	return ft5402_i2c_Write(client, buf, sizeof(buf));
}
EXPORT_SYMBOL_GPL(ft5402_write_reg);


int ft5402_read_reg(struct i2c_client * client, u8 regaddr, u8 * regvalue)
{
	return ft5402_i2c_Read(client, &regaddr, 1, regvalue, 1);
}
EXPORT_SYMBOL_GPL(ft5402_read_reg);

/*
 0 : not up software
 1 :up software
*/
static int inet_getChipA6(struct i2c_client *client)
{

		int ret = -1;
		unsigned char buf_A6[2]={0xA6};
		
		msleep(500);
		
		ret=ft5x_i2c_rxdata(buf_A6, 2);
		
		if(ret < 0)
		{
			return 0;
		}
		
		printk("ft5x06 getA6=0x%x\n",buf_A6[0]);
		//return 1;
		if(buf_A6[0] <= 0x18 || buf_A6[0] == 0xa6)
		{
			return 1;
		}
		else
		{
			return 0;
		}	
}



/*
[function]: 
    callback: read data from ctpm by i2c interface,implemented by special user;
[parameters]:
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[out]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    FTS_TRUE     :success;
    FTS_FALSE    :fail;
*/
int i2c_read_interface(u8 bt_ctpm_addr, u8* pbt_buf, u16 dw_lenth)
{
	int ret;

	ret = i2c_master_recv(this_client, pbt_buf, dw_lenth);

	if(ret != dw_lenth){
		printk("ret = %d. \n", ret);
		printk("i2c_read_interface error\n");
		return FTS_FALSE;
	}

	return FTS_TRUE;
}

/*
[function]: 
    callback: write data to ctpm by i2c interface,implemented by special user;
[parameters]:
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[in]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    FTS_TRUE     :success;
    FTS_FALSE    :fail;
*/
int i2c_write_interface(u8 bt_ctpm_addr, u8* pbt_buf, u16 dw_lenth)
{
	int ret;
	ret=i2c_master_send(this_client, pbt_buf, dw_lenth);
	if(ret != dw_lenth){
		printk("i2c_write_interface error\n");
		return FTS_FALSE;
	}

	return FTS_TRUE;
}


/*******************************************************	
Function:
	Read data from the slave
	Each read operation with two i2c_msg composition, for the first message sent from the machine address,
	Article 2 reads the address used to send and retrieve data; each message sent before the start signal
Parameters:
	client: i2c devices, including device address
	buf [0]: The first byte to read Address
	buf [1] ~ buf [len]: data buffer
	len: the length of read data
return:
	Execution messages
*********************************************************/
/*Function as i2c_master_send */
static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret=-1;
	
	msgs[0].flags=!I2C_M_RD;
	msgs[0].addr=client->addr;
	msgs[0].len=2;
	msgs[0].buf=&buf[0];
	//msgs[0].scl_rate=200000;

	msgs[1].flags=I2C_M_RD;
	msgs[1].addr=client->addr;
	msgs[1].len=len-2;
	msgs[1].buf=&buf[2];
	//msgs[1].scl_rate=200000;
	
	ret=i2c_transfer(client->adapter,msgs, 2);
	return ret;
}

/*******************************************************	
Function:
	Write data to a slave
Parameters:
	client: i2c devices, including device address
	buf [0]: The first byte of the write address
	buf [1] ~ buf [len]: data buffer
	len: data length
return:
	Execution messages
*******************************************************/
/*Function as i2c_master_send */
static int i2c_write_bytes(struct i2c_client *client,uint8_t *data,int len)
{
	struct i2c_msg msg;
	int ret=-1;
	//发送设备地址
	msg.flags=!I2C_M_RD;//写消息
	msg.addr=client->addr;
	msg.len=len;
	msg.buf=data;	
	//msg.scl_rate=200000;	
	
	ret=i2c_transfer(client->adapter,&msg, 1);
	return ret;
}

/***************************************************************************************/

/*
[function]: 
    read out the register value.
[parameters]:
    e_reg_name[in]    :register name;
    pbt_buf[out]    :the returned register value;
    bt_len[in]        :length of pbt_buf, should be set to 2;        
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
u8 fts_register_read(u8 e_reg_name, u8* pbt_buf, u8 bt_len)
{
	u8 read_cmd[3]= {0};
	u8 cmd_len     = 0;

	read_cmd[0] = e_reg_name;
	cmd_len = 1;    

	/*call the write callback function*/
	//    if(!i2c_write_interface(I2C_CTPM_ADDRESS, &read_cmd, cmd_len))
	//    {
	//        return FTS_FALSE;
	//    }


	if(!i2c_write_interface(I2C_CTPM_ADDRESS, read_cmd, cmd_len))	{//change by zhengdixu
		return FTS_FALSE;
	}

	/*call the read callback function to get the register value*/        
	if(!i2c_read_interface(I2C_CTPM_ADDRESS, pbt_buf, bt_len)){
		return FTS_FALSE;
	}
	return FTS_TRUE;
}

/*
[function]: 
    write a value to register.
[parameters]:
    e_reg_name[in]    :register name;
    pbt_buf[in]        :the returned register value;
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
int fts_register_write(u8 e_reg_name, u8 bt_value)
{
	FTS_BYTE write_cmd[2] = {0};

	write_cmd[0] = e_reg_name;
	write_cmd[1] = bt_value;

	/*call the write callback function*/
	//return i2c_write_interface(I2C_CTPM_ADDRESS, &write_cmd, 2);
	return i2c_write_interface(I2C_CTPM_ADDRESS, write_cmd, 2); //change by zhengdixu
}

/*
[function]: 
    send a command to ctpm.
[parameters]:
    btcmd[in]        :command code;
    btPara1[in]    :parameter 1;    
    btPara2[in]    :parameter 2;    
    btPara3[in]    :parameter 3;    
    num[in]        :the valid input parameter numbers, if only command code needed and no parameters followed,then the num is 1;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
int cmd_write(u8 btcmd,u8 btPara1,u8 btPara2,u8 btPara3,u8 num)
{
	FTS_BYTE write_cmd[4] = {0};

	write_cmd[0] = btcmd;
	write_cmd[1] = btPara1;
	write_cmd[2] = btPara2;
	write_cmd[3] = btPara3;
	//return i2c_write_interface(I2C_CTPM_ADDRESS, &write_cmd, num);
	return i2c_write_interface(I2C_CTPM_ADDRESS, write_cmd, num);//change by zhengdixu
}

/*
[function]: 
    write data to ctpm , the destination address is 0.
[parameters]:
    pbt_buf[in]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
int byte_write(u8* pbt_buf, u16 dw_len)
{
	return i2c_write_interface(I2C_CTPM_ADDRESS, pbt_buf, dw_len);
}

/*
[function]: 
    read out data from ctpm,the destination address is 0.
[parameters]:
    pbt_buf[out]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
int byte_read(u8* pbt_buf, u8 bt_len)
{
	return i2c_read_interface(I2C_CTPM_ADDRESS, pbt_buf, bt_len);
	//ft5x_i2c_rxdata
}


/*
[function]: 
    burn the FW to ctpm.
[parameters]:(ref. SPEC)
    pbt_buf[in]    :point to Head+FW ;
    dw_lenth[in]:the length of the FW + 6(the Head length);    
    bt_ecc[in]    :the ECC of the FW
[return]:
    ERR_OK        :no error;
    ERR_MODE    :fail to switch to UPDATE mode;
    ERR_READID    :read id fail;
    ERR_ERASE    :erase chip fail;
    ERR_STATUS    :status error;
    ERR_ECC        :ecc error.
*/


#define    FTS_PACKET_LENGTH       128 //2//4//8//16//32//64//128//256


static unsigned char CTPM_FW[]=
{
#include "ft5402_app.i"
};

//static  unsigned char  *CTPM_FW = CTPM_FW_;


void delay_qt_ms(unsigned long  w_ms)
{
	unsigned long i;
	unsigned long j;

	for (i = 0; i < w_ms; i++)
	{
		for (j = 0; j < 1000; j++)
		{
			 udelay(1);
		}
	}
}

void ft5402_upgrade_send_head(struct i2c_client * client)
{
	u8 ret = 0;
	u8 headbuf[2];
	headbuf[0] = 0xFA;
	headbuf[1] = 0xFA;

	ret = ft5402_i2c_Write(client, headbuf, 2);
	if(ret < 0)
		dev_err(&client->dev, "[FTS]--upgrading, send head error\n");
}
#if 0
static char *strdup(char *s){
		char *t=NULL;
		if(s&&(t=(char *)vmalloc(strlen(s)+1)))
		 		strcpy(t,s);
		 return t;
}

static int ChangeToHexData(char *line)
{
 		char *s=strdup(line);              	           	     
    char *p=strsep(&s,",");   
    while(p!=NULL){
    	 if(*p==' ') p=p+1;
       CTPM_FW[CNT++]=simple_strtol(p,NULL,16);
        p=strsep(&s,",");
    }
   vfree(s);
   CNT -=1;
   return 0;
}

static int  ReadDataFromFile(void)
{
    struct file *fp; 
    mm_segment_t fs; 
    loff_t pos=0; 
    char line[254];
    unsigned int i=0;
    
    pr_info("======%s. begin =======\n", __func__);	      
		
    CTPM_FW=vmalloc(30000); 
    fp = filp_open(update_file_path, O_RDWR | O_CREAT, 0644); 
    if (IS_ERR(fp)) { 
        printk("open file %s error\n",update_file_path); 
        return -1; 
    } 
    fs = get_fs(); 
    set_fs(KERNEL_DS); 
    while(vfs_read(fp, &line[i], 1, &pos)==1)
   {
              if (line[i] == '\n')
		{
              	line[i] ='\0';         
              	if(line[0]!='\0'&&line[0]!=';'&&line[0]!='/')
					ChangeToHexData(line);

               	i=0; 
			continue;		
             }
		if (i< 254)
              {
              	i++;
              }
    }			
    filp_close(fp, NULL); 
    set_fs(fs);

    line[i] ='\0';         
    if(line[0]!='\0'&&line[0]!=';'&&line[0]!='/')
    		ChangeToHexData(line);
   
    pr_info("%s.over:sizeof(CTPM_FW)=%d\n",__func__,CNT);
	
    return 0;
}
#endif

unsigned char fts_ctpm_get_i_file_ver(void)
{
        unsigned int ui_sz;
//    ui_sz = sizeof(CTPM_FW);
     ui_sz=CNT;
        if (ui_sz > 2){
                return CTPM_FW[ui_sz - 2];
        }else{
                //TBD, error handling?
                return 0xff; //default value
        }
}

E_UPGRADE_ERR_TYPE  fts_ctpm_fw_upgrade(u8* pbt_buf, u16 dw_lenth)
{
        u8 reg_val[2] = {0};
        FTS_BOOL i_ret = 0;
        u16 i = 0;
        
        
        u16  packet_number;
        u16  j;
        u16  temp;
        u16  lenght;
        u8  packet_buf[FTS_PACKET_LENGTH + 6];
        u8  auc_i2c_write_buf[10];
        u8 bt_ecc;

        /*********Step 1:Reset  CTPM *****/
        /*write 0xaa to register 0xfc*/
        delay_ms(100);
        fts_register_write(0xfc,0xaa);
        delay_ms(50);
        /*write 0x55 to register 0xfc*/
        fts_register_write(0xfc,0x55);
        printk("Step 1: Reset CTPM test\n");
        
        delay_ms(30);

        /*********Step 2:Enter upgrade mode *****/
        auc_i2c_write_buf[0] = 0x55;
        auc_i2c_write_buf[1] = 0xaa;
        i = 0;
        do{
                i++;
                i_ret = i2c_write_interface(I2C_CTPM_ADDRESS, auc_i2c_write_buf, 2);
                printk("Step 2: Enter update mode. \n");
                delay_ms(5);
        }while((FTS_FALSE == i_ret) && i<5);

        /*********Step 3:check READ-ID***********************/
        /*send the opration head*/
        i = 0;
        do{
                if(i > 3){
                        cmd_write(0x07,0x00,0x00,0x00,1);
		        return ERR_READID; 
                }
                /*read out the CTPM ID*/
                printk("====Step 3:check READ-ID====");
                cmd_write(0x90,0x00,0x00,0x00,4);
                byte_read(reg_val,2);
                i++;
                delay_ms(5);
                printk("Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
         }while((reg_val[1] != 0x03)&&(reg_val[1] != 0x06));//while(reg_val[0] != 0x79 || reg_val[1] != 0x03);

        /*********Step 4:erase app*******************************/
        cmd_write(0x61,0x00,0x00,0x00,1);
        delay_ms(1500);
        printk("Step 4: erase. \n");
        
        /*********Step 5:write firmware(FW) to ctpm flash*********/
        bt_ecc = 0;
        printk("Step 5: start upgrade. \n");
        dw_lenth = dw_lenth - 8;
        packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
        packet_buf[0] = 0xbf;
        packet_buf[1] = 0x00;
        for (j=0;j<packet_number;j++){
                temp = j * FTS_PACKET_LENGTH;
                packet_buf[2] = (FTS_BYTE)(temp>>8);
                packet_buf[3] = (FTS_BYTE)temp;
                lenght = FTS_PACKET_LENGTH;
                packet_buf[4] = (FTS_BYTE)(lenght>>8);
                packet_buf[5] = (FTS_BYTE)lenght;
        
                for (i=0;i<FTS_PACKET_LENGTH;i++){
                        packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i]; 
                        bt_ecc ^= packet_buf[6+i];
                }
        
                byte_write(&packet_buf[0],FTS_PACKET_LENGTH + 6);
                delay_ms(FTS_PACKET_LENGTH/6 + 1);
                if ((j * FTS_PACKET_LENGTH % 1024) == 0){
                        printk("upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
                }
        }

        if ((dw_lenth) % FTS_PACKET_LENGTH > 0){
                temp = packet_number * FTS_PACKET_LENGTH;
                packet_buf[2] = (FTS_BYTE)(temp>>8);
                packet_buf[3] = (FTS_BYTE)temp;
        
                temp = (dw_lenth) % FTS_PACKET_LENGTH;
                packet_buf[4] = (FTS_BYTE)(temp>>8);
                packet_buf[5] = (FTS_BYTE)temp;
        
                for (i=0;i<temp;i++){
                        packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
                        bt_ecc ^= packet_buf[6+i];
                }
        
                byte_write(&packet_buf[0],temp+6);    
                delay_ms(20);
        }

        //send the last six byte
        for (i = 0; i<6; i++){
                temp = 0x6ffa + i;
                packet_buf[2] = (FTS_BYTE)(temp>>8);
                packet_buf[3] = (FTS_BYTE)temp;
                temp =1;
                packet_buf[4] = (FTS_BYTE)(temp>>8);
                packet_buf[5] = (FTS_BYTE)temp;
                packet_buf[6] = pbt_buf[ dw_lenth + i]; 
                bt_ecc ^= packet_buf[6];
        
                byte_write(&packet_buf[0],7);  
                delay_ms(20);
        }

        /*********Step 6: read out checksum***********************/
        /*send the opration head*/
        //cmd_write(0xcc,0x00,0x00,0x00,1);//把0xcc当作寄存器地址，去读出一个字节
        // byte_read(reg_val,1);//change by zhengdixu

	fts_register_read(0xcc, reg_val,1);
	
        printk("Step 6:  ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
        if(reg_val[0] != bt_ecc){
                cmd_write(0x07,0x00,0x00,0x00,1);
		return ERR_ECC;
        }

        /*********Step 7: reset the new FW***********************/
        cmd_write(0x07,0x00,0x00,0x00,1);
        msleep(30);
        return ERR_OK;
}

#define FTS_PACKET_LENGTH 128
#define FTS_UPGRADE_LOOP 3
#define FT_UPGRADE_AA	0xAA
#define FT_UPGRADE_55 	0x55

/*upgrade config of FT5402*/
#define FT5402_UPGRADE_AA_DELAY 		50
#define FT5402_UPGRADE_55_DELAY 		30
#define FT5402_UPGRADE_ID_1			0x79
#define FT5402_UPGRADE_ID_2			0xa3
#define FT5402_UPGRADE_READID_DELAY 	1
#define FT5402_UPGRADE_EARSE_DELAY	2000

static int  ft5402_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j = 0;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;


	for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
		/*********Step 1:Reset  CTPM *****/
		/*write 0xaa to register 0xfc */
		ft5402_write_reg(client, 0xfc, FT_UPGRADE_AA);
		msleep(FT5402_UPGRADE_AA_DELAY);

		/*write 0x55 to register 0xfc */
	
		ft5402_write_reg(client, 0xfc, FT_UPGRADE_55);

		msleep(FT5402_UPGRADE_55_DELAY);
		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FT_UPGRADE_55;
		auc_i2c_write_buf[1] = FT_UPGRADE_AA;
		j=0;
		do {
			j++;
			i_ret = ft5402_i2c_Write(client, auc_i2c_write_buf, 2);
			msleep(5);
		} while (i_ret <= 0 && j < 5);


		/*********Step 3:check READ-ID***********************/
		msleep(FT5402_UPGRADE_READID_DELAY);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;
		ft5402_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);


		if (reg_val[0] == FT5402_UPGRADE_ID_1
			&& reg_val[1] == FT5402_UPGRADE_ID_2) {
			printk("[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			break;
		} else {
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
		}
	}
	if (i >= FTS_UPGRADE_LOOP) {
		printk("[FTS] out of upgrade loop\n");
		return -EIO;
	}

	/*Step 4:erase app and panel paramenter area*/
	printk("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = 0x61;
	ft5402_i2c_Write(client, auc_i2c_write_buf, 1);	/*erase app area */
	msleep(FT5402_UPGRADE_EARSE_DELAY);
	/*erase panel parameter area */
	auc_i2c_write_buf[0] = 0x63;
	ft5402_i2c_Write(client, auc_i2c_write_buf, 1);
	msleep(100);

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	printk("Step 5:write firmware(FW) to ctpm flash\n");

	dw_lenth = dw_lenth - 8;
	//dw_lenth = dw_lenth - 2;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) {
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) {
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		
		ft5402_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		msleep(FTS_PACKET_LENGTH / 6 + 1);
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) {
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		ft5402_i2c_Write(client, packet_buf, temp + 6);
		msleep(20);
	}

	/*send the last six byte */
	#if 1
	for (i = 0; i < 6; i++) {
		temp = 0x6ffa + i;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = 1;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;
		packet_buf[6] = pbt_buf[dw_lenth + i];
		bt_ecc ^= packet_buf[6];
		ft5402_i2c_Write(client, packet_buf, 7);
		msleep(20);
	}
	#endif

	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	printk("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0xcc;
	ft5402_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) {
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
					reg_val[0],
					bt_ecc);
		return -EIO;
	}

	/*********Step 7: reset the new FW***********************/
	printk("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	ft5402_i2c_Write(client, auc_i2c_write_buf, 1);
	msleep(300);	/*make sure CTP startup normally */

	return 0;
}
int fts_ctpm_auto_clb(void)
{
        unsigned char uc_temp;
        unsigned char i ;
        
        printk("[FTS] start auto CLB.\n");
        msleep(200);
        fts_register_write(0, 0x40);  
        delay_ms(100);                       //make sure already enter factory mode
        fts_register_write(2, 0x4);               //write command to start calibration
        delay_ms(300);
        for(i=0;i<100;i++){
                fts_register_read(0,&uc_temp,1);
                if (((uc_temp&0x70)>>4) == 0x0){    //return to normal mode, calibration finish
                        break;
                }
                delay_ms(200);
                printk("[FTS] waiting calibration %d\n",i);
        }
        
        printk("[FTS] calibration OK.\n");
        
        msleep(300);
        fts_register_write(0, 0x40);          //goto factory mode
        delay_ms(100);                       //make sure already enter factory mode
        fts_register_write(2, 0x5);          //store CLB result
        delay_ms(300);
        fts_register_write(0, 0x0);          //return to normal mode 
        msleep(300);
        printk("[FTS] store CLB result OK.\n");
        return 0;
}
void getVerNo(u8* buf, int len)
{
	u8 start_reg=0x0;
	int ret = -1;
	//int status = 0;
	int i = 0;
	start_reg = 0xa6;

#if 0
	printk("read 0xa6 one time. \n");
	if(FTS_FALSE == fts_register_read(0xa6, buf, len)){
                return ;
	}
	
	for (i=0; i< len; i++) {
		printk("=========buf[%d] = 0x%x \n", i, buf[i]);
	}
	
	printk("read 0xa8. \n");
	if(FTS_FALSE == fts_register_read(0xa8, buf, len)){
                return ;
	}
	for (i=0; i< len; i++) {
		printk("=========buf[%d] = 0x%x \n", i, buf[i]);
	}

	ft5x_i2c_rxdata(buf, len);
	
        for (i=0; i< len; i++) {
                printk("=========buf[%d] = 0x%x \n", i, buf[i]);
        }

        byte_read(buf, len);
        for (i=0; i< len; i++) {
                printk("=========buf[%d] = 0x%x \n", i, buf[i]);
        }
          
#endif

	ret =fts_register_read(0xa6, buf, len);
	//et = ft5406_read_regs(ft5x0x_ts_data_test->client,start_reg, buf, 2);
	if (ret < 0) {
		printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return;
	}
	for (i=0; i<2; i++) {
		printk("=========buf[%d] = 0x%x \n", i, buf[i]);
	}
	return;
}
/*
*get firmware size

@firmware_name:firmware name
*note:the firmware default path is sdcard.
	if you want to change the dir, please modify by yourself.
*/
static int ft5402_GetFirmwareSize(char *firmware_name)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize = 0;
	char filepath[128];
	memset(filepath, 0, sizeof(filepath));

	sprintf(filepath, "%s", firmware_name);

	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);

	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	filp_close(pfile, NULL);
	return fsize;
}

/*
*read firmware buf for .bin file.

@firmware_name: fireware name
@firmware_buf: data buf of fireware

note:the firmware default path is sdcard.
	if you want to change the dir, please modify by yourself.
*/
static int ft5402_ReadFirmware(char *firmware_name,
			       unsigned char *firmware_buf)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize;
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s", firmware_name);

	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_read(pfile, firmware_buf, fsize, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}

int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client,
				       char *firmware_name)
{
	u8 *pbt_buf = NULL;
	int i_ret;
	int fwsize = ft5402_GetFirmwareSize(firmware_name);

	if (fwsize <= 0) {
		dev_err(&client->dev, "%s ERROR:Get firmware size failed\n",
					__func__);
		return -EIO;
	}

	if (fwsize < 8 || fwsize > 32 * 1024) {
		dev_dbg(&client->dev, "%s:FW length error\n", __func__);
		return -EIO;
	}

	/*=========FW upgrade========================*/
	pbt_buf = kmalloc(fwsize + 1, GFP_ATOMIC);

	if (ft5402_ReadFirmware(firmware_name, pbt_buf)) {
		dev_err(&client->dev, "%s() - ERROR: request_firmware failed\n",
					__func__);
		kfree(pbt_buf);
		return -EIO;
	}

	/*call the upgrade function */
	i_ret = ft5402_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	if (i_ret != 0)
		dev_dbg(&client->dev, "%s() - ERROR:[FTS] upgrade failed..\n",
					__func__);
	kfree(pbt_buf);

	return i_ret;
}
#if 0
int fts_ctpm_fw_upgrade_with_i_file(void)
{
	FTS_BYTE*     pbt_buf = FTS_NULL;
	int i_ret = 0;
	unsigned char a;
	unsigned char b;
#define BUFFER_LEN (2)            //len == 2 
	unsigned char buf[BUFFER_LEN] = {0};

   	if(0!=ReadDataFromFile()){
		pr_info("%s:ReadDataFromFile failed\n",__func__); 
		return -1; 
	}	
	//=========FW upgrade========================*/
	printk("%s. \n", __func__);

	pbt_buf = CTPM_FW;
	//msleep(200);
        // cmd_write(0x07,0x00,0x00,0x00,1);
	msleep(100);
	getVerNo(buf, BUFFER_LEN);
	a = buf[0];
	b = fts_ctpm_get_i_file_ver();
	printk("a == %hu,  b== %hu \n",a, b);

	/*
	  * when the firmware in touch panel maybe corrupted,
	  * or the firmware in host flash is new, need upgrade
	  */
	if ( 0xa6 == a ||a < b ){
		/*call the upgrade function*/
//		i_ret =  fts_ctpm_fw_upgrade(&pbt_buf[0],sizeof(CTPM_FW));
		i_ret =  fts_ctpm_fw_upgrade(&pbt_buf[0],CNT);
		if (i_ret != 0){
			printk("[FTS] upgrade failed i_ret = %d.\n", i_ret);
		} else {
			printk("[FTS] upgrade successfully.\n");
			fts_ctpm_auto_clb();  //start auto CLB
		}
	}	
	return i_ret;
	
}
#endif
/*
upgrade with *.i file
*/

int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client * client)
{
	u8 * pbt_buf = NULL;
	int i_ret;
	int fw_len = sizeof(CTPM_FW);

	/*judge the fw that will be upgraded
	 * if illegal, then stop upgrade and return.
	*/
	if (fw_len<8 || fw_len>32*1024) {
		dev_err(&client->dev, "[FTS]----FW length error\n");
		return -EIO;
	}	
//	if((CTPM_FW[fw_len-8]^CTPM_FW[fw_len-6])==0xFF
//		&& (CTPM_FW[fw_len-7]^CTPM_FW[fw_len-5])==0xFF
//		&& (CTPM_FW[fw_len-3]^CTPM_FW[fw_len-4])==0xFF)
	{
		/*FW upgrade*/
		pbt_buf = CTPM_FW;
		/*call the upgrade function*/
		i_ret =  ft5402_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
		if (i_ret != 0)
			dev_err(&client->dev, "[FTS]---- upgrade failed. err=%d.\n", i_ret);
		else
			dev_dbg(&client->dev, "[FTS]----upgrade successful\n");
	}
//	else
//	{
//		dev_err(&client->dev, "[FTS]----FW format error\n");
//		return -EBADFD;
//	}
	return i_ret;
}


unsigned char fts_ctpm_get_upg_ver(void)
{
	unsigned int ui_sz;
//	ui_sz = sizeof(CTPM_FW);
	ui_sz =CNT;
	if (ui_sz > 2){
		return CTPM_FW[0];
	}
	else{
		return 0xff; //default value
	}
}

static int ft5x_i2c_rxdata(char *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};
	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		printk("msg %s i2c read error: %d\n", __func__, ret);
	
	return ret;
}

static int ft5x_i2c_txdata(char *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

   	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

static int ft5x_set_reg(u8 addr, u8 para)
{
	u8 buf[3];
	int ret = -1;

	buf[0] = addr;
	buf[1] = para;
	ret = ft5x_i2c_txdata(buf, 2);
	if (ret < 0) {
		pr_err("write reg failed! %#x ret: %d", buf[0], ret);
		return -1;
	}

	return 0;
}

static void ft5x_ts_release(void)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
#ifdef CONFIG_FT5X0X_MULTITOUCH	
#ifdef TOUCH_KEY_SUPPORT
	if(1 == key_tp){
		input_report_key(data->input_dev, key_val, 0);
		dprintk(DEBUG_KEY_INFO,"Release Key = %d\n",key_val);		
	} else{
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	}
#else
	input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
#endif

#else
	input_report_abs(data->input_dev, ABS_PRESSURE, 0);
	input_report_key(data->input_dev, BTN_TOUCH, 0);
#endif
	
	input_sync(data->input_dev);
	return;

}

static int ft5x_read_data(void)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	unsigned char buf[63]={0};
	int ret = -1;
        
#ifdef CONFIG_FT5X0X_MULTITOUCH
	ret = ft5x_i2c_rxdata(buf, 62);
#else
	ret = ft5x_i2c_rxdata(buf, 62);
#endif
	if (ret < 0) {
		dprintk(DEBUG_X_Y_INFO,"%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));

	event->touch_point = buf[2] & 0x0F;// 000 0111
	dprintk(DEBUG_X_Y_INFO,"touch point = %d\n",event->touch_point);

	if (event->touch_point == 0) {
		ft5x_ts_release();
		return 1; 
	}

	switch (event->touch_point) {
   	case 10:
   		{
				event->x10 = (s16)(buf[0x39] & 0x0F)<<8 | (s16)buf[0x3a];
				event->y10 = (s16)(buf[0x3b] & 0x0F)<<8 | (s16)buf[0x3c];
	

			if(1 == exchange_x_y_flag){
                                  swap(event->x10, event->y10);
			}	
			if(1 == revert_x_flag){
	                event->x10 = SCREEN_MAX_X - event->x10;
			}
			if(1 == revert_y_flag){
	                event->y10 = SCREEN_MAX_Y - event->y10;
			}
			
		
			event->touch_ID10=(s16)(buf[0x3b] & 0xF0)>>4;
			}
		case 9:
			{
				event->x9 = (s16)(buf[0x33] & 0x0F)<<8 | (s16)buf[0x34];
				event->y9 = (s16)(buf[0x35] & 0x0F)<<8 | (s16)buf[0x36];
				

				if(1 == exchange_x_y_flag){
                                  swap(event->x9, event->y9);
			}
			if(1 == revert_x_flag){
	                event->x9 = SCREEN_MAX_X - event->x9;
				}
			if(1 == revert_y_flag){
	                event->y9 = SCREEN_MAX_Y - event->y9;
				}
		
		
			event->touch_ID9=(s16)(buf[0x35] & 0xF0)>>4;
			}
		case 8:
			{
					event->x8 = (s16)(buf[0x2D] & 0x0F)<<8 | (s16)buf[0x2E];
					event->y8 = (s16)(buf[0x2F] & 0x0F)<<8 | (s16)buf[0x30];
					

			if(1 == exchange_x_y_flag){
                                  swap(event->x8, event->y8);
			}	
			if(1 == revert_x_flag){
	                event->x8 = SCREEN_MAX_X - event->x8;
			}
			if(1 == revert_y_flag){
	                event->y8 = SCREEN_MAX_Y - event->y8;
			}
			
		
			event->touch_ID8=(s16)(buf[0x2F] & 0xF0)>>4;
			}
		case 7:
			{
				event->x7 = (s16)(buf[0x27] & 0x0F)<<8 | (s16)buf[0x28];
				event->y7 = (s16)(buf[0x29] & 0x0F)<<8 | (s16)buf[0x2a];

				if(1 == exchange_x_y_flag){
                                  swap(event->x7, event->y7);
			}
			if(1 == revert_x_flag){
	                event->x7 = SCREEN_MAX_X - event->x7;
				}
			if(1 == revert_y_flag){
	                event->y7 = SCREEN_MAX_Y - event->y7;
				}
		
		
			event->touch_ID7=(s16)(buf[0x29] & 0xF0)>>4;
			}
		case 6:
			{
				event->x6 = (s16)(buf[0x21] & 0x0F)<<8 | (s16)buf[0x22];
				event->y6 = (s16)(buf[0x23] & 0x0F)<<8 | (s16)buf[0x24];
			

			if(1 == exchange_x_y_flag){
                                  swap(event->x6, event->y6);
			}
			if(1 == revert_x_flag){
	                event->x6 = SCREEN_MAX_X - event->x6;
				}
			if(1 == revert_y_flag){
	                event->y6 = SCREEN_MAX_Y - event->y6;
				}
			
		
			event->touch_ID6=(s16)(buf[0x23] & 0xF0)>>4;
    	}
	case 5:
		event->x5 = (s16)(buf[0x1b] & 0x0F)<<8 | (s16)buf[0x1c];
		event->y5 = (s16)(buf[0x1d] & 0x0F)<<8 | (s16)buf[0x1e];
		dprintk(DEBUG_X_Y_INFO,"source data:event->x5 = %d, event->y5 = %d. \n", event->x5, event->y5);
		
		if(1 == exchange_x_y_flag){
			swap(event->x5, event->y5);
		}
		if(1 == revert_x_flag){
			event->x5 = SCREEN_MAX_X - event->x5;
		}
		if(1 == revert_y_flag){
			event->y5 = SCREEN_MAX_Y - event->y5;
		}
		event->touch_ID5=(s16)(buf[0x1D] & 0xF0)>>4;
		
		dprintk(DEBUG_X_Y_INFO,"touch id : %d. \n",event->touch_ID5);
	case 4:
		event->x4 = (s16)(buf[0x15] & 0x0F)<<8 | (s16)buf[0x16];
		event->y4 = (s16)(buf[0x17] & 0x0F)<<8 | (s16)buf[0x18];
		dprintk(DEBUG_X_Y_INFO,"source data:event->x4 = %d, event->y4 = %d. \n", event->x4, event->y4);
		
		if(1 == exchange_x_y_flag){
			swap(event->x4, event->y4);
		}
		if(1 == revert_x_flag){
			event->x4 = SCREEN_MAX_X - event->x4;
		}
		if(1 == revert_y_flag){
			event->y4 = SCREEN_MAX_Y - event->y4;
		}	
		/**/
		event->touch_ID4=(s16)(buf[0x17] & 0xF0)>>4;
		
		dprintk(DEBUG_X_Y_INFO,"touch id : %d. \n",event->touch_ID4);
	case 3:
		event->x3 = (s16)(buf[0x0f] & 0x0F)<<8 | (s16)buf[0x10];
		event->y3 = (s16)(buf[0x11] & 0x0F)<<8 | (s16)buf[0x12];
		dprintk(DEBUG_X_Y_INFO,"source data:event->x3 = %d, event->y3 = %d. \n", event->x3, event->y3);
		
		if(1 == exchange_x_y_flag){
			swap(event->x3, event->y3);
		}
		if(1 == revert_x_flag){
			event->x3 = SCREEN_MAX_X - event->x3;
		}
		if(1 == revert_y_flag){
			event->y3 = SCREEN_MAX_Y - event->y3;
		}
		/**/
		event->touch_ID3=(s16)(buf[0x11] & 0xF0)>>4;
		dprintk(DEBUG_X_Y_INFO,"touch id : %d. \n",event->touch_ID3);
	case 2:
		event->x2 = (s16)(buf[9] & 0x0F)<<8 | (s16)buf[10];
		event->y2 = (s16)(buf[11] & 0x0F)<<8 | (s16)buf[12];
		dprintk(DEBUG_X_Y_INFO,"source data:event->x2 = %d, event->y2 = %d. \n", event->x2, event->y2);
		
		if(1 == exchange_x_y_flag){
			swap(event->x2, event->y2);
		}
		if(1 == revert_x_flag){
			event->x2 = SCREEN_MAX_X - event->x2;
		}
		if(1 == revert_y_flag){
			event->y2 = SCREEN_MAX_Y - event->y2;
		}
		/**/
		event->touch_ID2=(s16)(buf[0x0b] & 0xF0)>>4;
		
		dprintk(DEBUG_X_Y_INFO,"touch id : %d. \n",event->touch_ID2);
	case 1:
		event->x1 = (s16)(buf[3] & 0x0F)<<8 | (s16)buf[4];
		event->y1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
		dprintk(DEBUG_X_Y_INFO,"source data:event->x1 = %d, event->y1 = %d. \n", event->x1, event->y1);
		/* */
		if(1 == exchange_x_y_flag){
			swap(event->x1, event->y1);
		}
		if(1 == revert_x_flag){
			event->x1 = SCREEN_MAX_X - event->x1;
		}
		if(1 == revert_y_flag){
			event->y1 = SCREEN_MAX_Y - event->y1;
		}
		/* */
		event->touch_ID1=(s16)(buf[0x05] & 0xF0)>>4;
		dprintk(DEBUG_X_Y_INFO,"touch id : %d. \n",event->touch_ID1);
		break;
	default:
		return -1;
	}
	//event->pressure = 20;//Paul modified from 200 @20130117
	return 0;
}

#ifdef TOUCH_KEY_LIGHT_SUPPORT
static void ft5x_lighting(void)
{
        ctp_key_light(1,15);
	return;
}
#endif

static void ft5x_report_multitouch(void)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;

#ifdef TOUCH_KEY_SUPPORT
	if(1 == key_tp){
		return;
	}
#endif

	switch(event->touch_point) {
			case 10:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID10);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				if((event->x10)==0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x10+1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x10);
				}
				
				if((event->y10)==0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y10+1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y10);
				}
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				#ifdef PRINT_POINT_INFO
				    printk("===x10 = %d,y10 = %d ====\n",event->x10,event->y10);
				#endif			
			case 9:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID9);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				if((event->x9)==0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, (event->x9)+1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x9);
				}
				
				if((event->y9)==0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, (event->y9)+1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y9);
				}
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				#ifdef PRINT_POINT_INFO
				    printk("===x9 = %d,y9 = %d ====\n",event->x9,event->y9);
				#endif			
			case 8:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID8);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				if((event->x8)==0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, (event->x8)+1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x8);
				}
				
				if((event->y8)==0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, (event->y8)+1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y8);
				}
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				#ifdef PRINT_POINT_INFO
				    printk("===x8 = %d,y8 = %d ====\n",event->x8,event->y8);
				#endif			
			case 7:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID7);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				if((event->x7)==0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, (event->x7)+1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x7);
				}
				
				if((event->y7)==0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, (event->y7)+1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y7);
				}
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				#ifdef PRINT_POINT_INFO
				    printk("===x7 = %d,y7 = %d ====\n",event->x7,event->y7);
				#endif			
			case 6:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID6);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				if((event->x6)==0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, (event->x6)+1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x6);
				}
				
				if((event->y6)==0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, (event->y6)+1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y6);
				}
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				#ifdef PRINT_POINT_INFO
				    printk("===x6 = %d,y6 = %d ====\n",event->x6,event->y6);
				#endif
			case 5:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID5);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				if(event->x5 == 0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, 1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x5);
				}

				if(event->y5 == 0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, 1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y5);
				}
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				dprintk(DEBUG_X_Y_INFO,"report data:===x5 = %d,y5 = %d ====\n",event->x5,event->y5);
			case 4:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID4);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				if(event->x4== 0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, 1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x4);
				}

				if(event->y4== 0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, 1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y4);
				}
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				dprintk(DEBUG_X_Y_INFO,"report data:===x4 = %d,y4 = %d ====\n",event->x4,event->y4);
			case 3:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID3);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				if(event->x3== 0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, 1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x3);
				}

				if(event->y3== 0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, 1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y3);
				}
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				dprintk(DEBUG_X_Y_INFO,"report data:===x3 = %d,y3 = %d ====\n",event->x3,event->y3);
			case 2:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID2);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				if(event->x2== 0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, 1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x2);
				}

				if(event->y2== 0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, 1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y2);
				}
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				dprintk(DEBUG_X_Y_INFO,"report data:===x2 = %d,y2 = %d ====\n",event->x2,event->y2);
			case 1:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID1);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				if(event->x1== 0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, 1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x1);
				}

				if(event->y1== 0)
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, 1);
				}
				else
				{
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y1);
				}
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				dprintk(DEBUG_X_Y_INFO,"report data:===x1 = %d,y1 = %d ====\n",event->x1,event->y1);
				break;
	default:
		dprintk(DEBUG_X_Y_INFO,"report data:==touch_point default =\n");
		break;
	}
	
	input_sync(data->input_dev);
	return;
}

#ifndef CONFIG_FT5X0X_MULTITOUCH
static void ft5x_report_singletouch(void)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	
	if (event->touch_point == 1) {
		input_report_abs(data->input_dev, ABS_X, event->x1);
		input_report_abs(data->input_dev, ABS_Y, event->y1);
		input_report_abs(data->input_dev, ABS_PRESSURE, event->pressure);
	}
	dprintk(DEBUG_X_Y_INFO,"report:===x1 = %d,y1 = %d ====\n",event->x1,event->y1);
	input_report_key(data->input_dev, BTN_TOUCH, 1);
	input_sync(data->input_dev);
	return;
}
#endif

#ifdef TOUCH_KEY_SUPPORT
static void ft5x_report_touchkey(void)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;

#ifdef TOUCH_KEY_FOR_ANGDA
	if((1==event->touch_point)&&(event->x1 > TOUCH_KEY_X_LIMIT)){
		key_tp = 1;
		if(event->y1 < 40){
			key_val = 1;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);  
			dprintk(DEBUG_KEY_INFO,"===KEY 1====\n");
		}else if(event->y1 < 90){
			key_val = 2;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);     
			dprintk(DEBUG_KEY_INFO,"===KEY 2 ====\n");
		}else{
			key_val = 3;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);     
			dprintk(DEBUG_KEY_INFO,"===KEY 3====\n");	
		}
	} else{
		key_tp = 0;
	}
#endif
#ifdef TOUCH_KEY_FOR_EVB13
	if((1==event->touch_point)&&((event->x1 > TOUCH_KEY_LOWER_X_LIMIT)&&(event->x1<TOUCH_KEY_HIGHER_X_LIMIT))){
		key_tp = 1;
		if(event->y1 < 5){
			key_val = 1;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);  
			dprintk(DEBUG_KEY_INFO,"===KEY 1====\n");     
		}else if((event->y1 < 45)&&(event->y1>35)){
			key_val = 2;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);     
			dprintk(DEBUG_KEY_INFO,"===KEY 2 ====\n");
		}else if((event->y1 < 75)&&(event->y1>65)){
			key_val = 3;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);     
			dprintk(DEBUG_KEY_INFO,"===KEY 3====\n");
		}else if ((event->y1 < 105)&&(event->y1>95))	{
			key_val = 4;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);     
			dprintk(DEBUG_KEY_INFO,"===KEY 4====\n");	
		}
	}else{
		key_tp = 0;
	}
#endif

#ifdef TOUCH_KEY_LIGHT_SUPPORT
	ft5x_lighting();
#endif
	return;
}
#endif

static void ft5x_report_value(void)
{

#ifdef TOUCH_KEY_SUPPORT
	ft5x_report_touchkey();
#endif

#ifdef CONFIG_FT5X0X_MULTITOUCH
	ft5x_report_multitouch();
#else	/* CONFIG_FT5X0X_MULTITOUCH*/
	ft5x_report_singletouch();
#endif	/* CONFIG_FT5X0X_MULTITOUCH*/
	return;
}	

static void ft5x_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;
	ret = ft5x_read_data();
	if (ret == 0) {
		ft5x_report_value();
	}
	dprintk(DEBUG_INT_INFO,"%s:ret:%d\n",__func__,ret);
}
#ifdef CHARGE_MODE
static void polling_timer_func(unsigned long arg)
{

	struct ft5x_ts_data *ts_drv = (struct ft5x_ts_data *)arg;
	queue_work(ts_drv->ts_workqueue_charge, &ts_drv->pen_event_work_charge);
	mod_timer(&ts_drv->polling_timer,jiffies + msecs_to_jiffies(polling_time));	
}


//mbg ++ 20131115 for charge check
static void write_charge_cmd(struct work_struct *work)
{
	if(enable_cmd != ChargeChange)
	{	
		if(enable_cmd == 1) 
		{
			ft5x_set_reg(0xb5, 0x01);
			//printk("ft5402 set 0xb5 = 1\n");
			//printk("ft5402 charge\n");
		}
		else if(enable_cmd == 0)
		{
			ft5x_set_reg(0xb5, 0x00);
			//printk("ft5402 set 0xb5 = 0\n");
			//printk("ft5402 not charge\n");
		}
		ChargeChange = enable_cmd;
	}
}
//mbg --
#endif

static u32 ft5x_ts_interrupt(struct ft5x_ts_data *ft5x_ts)
{
	dprintk(DEBUG_INT_INFO,"==========ft5x_ts TS Interrupt============\n"); 
	queue_work(ft5x_ts->ts_workqueue, &ft5x_ts->pen_event_work);
	return 0;
}

static void ft5x_resume_events (struct work_struct *work)
{
	
	ctp_wakeup_inet(0,20);
	ctp_wakeup_inet(1,0);
	
#ifdef CONFIG_HAS_EARLYSUSPEND	
	if(STANDBY_WITH_POWER_OFF == standby_level){
	        msleep(100);
	}
#endif
	if(chip_type == 3) //chip_type = ft5402 add 2012.11.20 
	{
		//printk("resume ft5402_Init_IC_Param begin\n");
		msleep(350);
		ft5402_Init_IC_Param(this_client);
		msleep(50);
		
	}

	//revert_x_flag =1 ;
#ifdef FT5402_CONFIG_INI  //Debug	
	if (ft5402_Get_Param_From_Ini(FT5402_CONFIG_NAME) >= 0)
		ft5402_Init_IC_Param(this_client);
	else
		dev_err(&this_client->dev, "[FTS]-------Get ft5402 param from INI file failed\n");
#endif
#ifdef CHARGE_MODE
	ChargeChange = 0;
#endif

	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
}



static int ft5x_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
	dprintk(DEBUG_SUSPEND,"==ft5x_ts_suspend=\n");
	dprintk(DEBUG_SUSPEND,"CONFIG_PM: write FT5X0X_REG_PMODE .\n");
	
	is_suspend = false; 
	
	flush_workqueue(ft5x_resume_wq);
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
	cancel_work_sync(&data->pen_event_work);
	flush_workqueue(data->ts_workqueue);
	ft5x_set_reg(FT5X0X_REG_PMODE, PMODE_HIBERNATE);
	
	return 0;
}
static int ft5x_ts_resume(struct i2c_client *client)
{
	dprintk(DEBUG_SUSPEND,"==CONFIG_PM:ft5x_ts_resume== \n");
	if(is_suspend == false)
	        queue_work(ft5x_resume_wq, &ft5x_resume_work);
	        
	return 0;		
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ft5x_ts_early_suspend(struct early_suspend *handler)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
	dprintk(DEBUG_SUSPEND,"==ft5x_ts_suspend=\n");
	dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND: write FT5X0X_REG_PMODE .\n");
	
#ifndef CONFIG_HAS_EARLYSUSPEND
        is_suspend = true;
#endif  
        if(is_suspend == true){ 
	        flush_workqueue(ft5x_resume_wq);
	        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
	        cancel_work_sync(&data->pen_event_work);
	        flush_workqueue(data->ts_workqueue);
	        ft5x_set_reg(FT5X0X_REG_PMODE, PMODE_HIBERNATE);
	}
	
	is_suspend = true;
}

static void ft5x_ts_late_resume(struct early_suspend *handler)
{
	dprintk(DEBUG_SUSPEND,"==CONFIG_HAS_EARLYSUSPEND:ft5x_ts_resume== \n");

	ft5x_ts_release();
		
	queue_work(ft5x_resume_wq, &ft5x_resume_work);	
	is_suspend = true;
}
#endif


#ifdef SYSFS_DEBUG
/*
*init ic param
*for example: cat ft5402initparam
*/
static ssize_t ft5402_initparam_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	mutex_lock(&g_device_mutex);
	if (ft5402_Get_Param_From_Ini(FT5402_CONFIG_NAME) >= 0) {
		if (ft5402_Init_IC_Param(client) >= 0)
			num_read_chars = sprintf(buf, "%s",
				"ft5402 init param successful\r\n");
		else
			num_read_chars = sprintf(buf, "%s",
				"ft5402 init param failed!\r\n");
	} else {
		num_read_chars = sprintf(buf, "%s",
				"get ft5402 config ini failed!\r\n");
	}
	mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}


static ssize_t ft5402_initparam_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}

/*
*get ic param
*for example: cat ft5402getparam
*/
static ssize_t ft5402_getparam_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	mutex_lock(&g_device_mutex);
	
	ft5402_get_ic_param(client);
	
	mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}

static ssize_t ft5402_getparam_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}


static ssize_t ft5402_rwreg_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/*place holder for future use*/
	return -EPERM;
}

/*
*read and write register
*for example:
*read register: echo "88" > ft5402rwreg
*write register: echo "8808" > ft5402rwreg
*/
static ssize_t ft5402_rwreg_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 regaddr = 0xff, regvalue = 0xff;
	u8 valbuf[5] = {0};

	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 2) {
		if (num_read_chars != 4) {
			pr_info("please input 2 or 4 character\n");
			goto error_return;
		}
	}

	memcpy(valbuf, buf, num_read_chars);
	retval = strict_strtoul(valbuf, 16, &wmreg);

	if (0 != retval) {
		dev_err(&client->dev, "%s() - ERROR: Could not convert the "\
						"given input to a number." \
						"The given input was: \"%s\"\n",
						__func__, buf);
		goto error_return;
	}

	if (2 == num_read_chars) {
		/*read register*/
		regaddr = wmreg;
		if (ft5402_read_reg(client, regaddr, &regvalue) < 0)
			dev_err(&client->dev, "Could not read the register(0x%02x)\n",
						regaddr);
		else
			pr_info("the register(0x%02x) is 0x%02x\n",
					regaddr, regvalue);
	} else {
		regaddr = wmreg >> 8;
		regvalue = wmreg;
		if (ft5402_write_reg(client, regaddr, regvalue) < 0)
			dev_err(&client->dev, "Could not write the register(0x%02x)\n",
							regaddr);
		else
			dev_err(&client->dev, "Write 0x%02x into register(0x%02x) successful\n",
							regvalue, regaddr);
	}

error_return:
	mutex_unlock(&g_device_mutex);

	return count;
}

static DEVICE_ATTR(ft5402initparam, S_IRUGO | S_IWUSR, ft5402_initparam_show,
		   ft5402_initparam_store);

static DEVICE_ATTR(ft5402getparam, S_IRUGO | S_IWUSR, ft5402_getparam_show,
		   ft5402_getparam_store);

static DEVICE_ATTR(ft5402rwreg, S_IRUGO | S_IWUSR, ft5402_rwreg_show,
		   ft5402_rwreg_store);


/*add your attr in here*/
static struct attribute *ft5402_attributes[] = {
	&dev_attr_ft5402initparam.attr,
	&dev_attr_ft5402getparam.attr,
	&dev_attr_ft5402rwreg.attr,
	NULL
};
static struct attribute_group ft5402_attribute_group = {
	.attrs = ft5402_attributes
};

static int ft5402_create_sysfs_debug(struct i2c_client *client)
{
	int err = 0;
	err = sysfs_create_group(&client->dev.kobj, &ft5402_attribute_group);
	if (0 != err) {
		dev_err(&client->dev,
					 "%s() - ERROR: sysfs_create_group() failed.\n",
					 __func__);
		sysfs_remove_group(&client->dev.kobj, &ft5402_attribute_group);
		return -EIO;
	} else {
		mutex_init(&g_device_mutex);
		pr_info("ft5x0x:%s() - sysfs_create_group() succeeded.\n",
				__func__);
	}
	return err;
}
#endif

#ifdef FTS_APK_DEBUG
/*create apk debug channel*/

/*please don't modify these macro*/
#define PROC_UPGRADE			0
#define PROC_READ_REGISTER		1
#define PROC_WRITE_REGISTER	2
#define PROC_RESET_PARAM		3
#define PROC_NAME	"ft5402-debug"

static unsigned char proc_operate_mode = PROC_UPGRADE;
static struct proc_dir_entry *ft5402_proc_entry;
/*interface of write proc*/
static int ft5402_debug_write(struct file *filp, 
	const char __user *buff, unsigned long len, void *data)
{
	struct i2c_client *client = (struct i2c_client *)ft5402_proc_entry->data;
	unsigned char writebuf[FTS_PACKET_LENGTH];
	int buflen = len;
	int writelen = 0;
	int ret = 0;
	
	if (copy_from_user(&writebuf, buff, buflen)) {
		dev_err(&client->dev, "%s:copy from user error\n", __func__);
		return -EFAULT;
	}
	proc_operate_mode = writebuf[0];
	
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		{
			char upgrade_file_path[128];
			memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
			sprintf(upgrade_file_path, "%s", writebuf + 1);
			upgrade_file_path[buflen-1] = '\0';
			////DBG("%s\n", upgrade_file_path);
			disable_irq(client->irq);

			ret = fts_ctpm_fw_upgrade_with_app_file(client, upgrade_file_path);

			enable_irq(client->irq);
			if (ret < 0) {
				dev_err(&client->dev, "%s:upgrade failed.\n", __func__);
				return ret;
			}
		}
		break;
	case PROC_READ_REGISTER:
		writelen = 1;
		//DBG("%s:register addr=0x%02x\n", __func__, writebuf[1]);
		ret = ft5402_i2c_Write(client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_WRITE_REGISTER:
		writelen = 2;
		//DBG("%s:register addr=0x%02x register value=0x%02x\n", __func__, writebuf[1], writebuf[2]);
		ret = ft5402_write_reg(client, writebuf[1], writebuf[2]);
		if (ret < 0) {
			dev_err(&client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_RESET_PARAM:
#ifdef FT5402_CONFIG_INI	
		if (ft5402_Get_Param_From_Ini(FT5402_CONFIG_NAME) >= 0) {
			if (ft5402_Init_IC_Param(client) >= 0)
				//DBG("%s:ft5402 init param successful\r\n", __func__);
			else
				dev_err(&client->dev, "%s:ft5402 init param failed!\r\n", __func__);
		} else
			dev_err(&client->dev, "%s:get ft5402 config ini failed!\r\n", __func__);
#else
		if (ft5402_Init_IC_Param(client) >= 0)
			{
			//DBG("%s:ft5402 init param successful\r\n", __func__);
		}
		else
			{
			dev_err(&client->dev, "%s:ft5402 init param failed!\r\n", __func__);
		}
#endif
		break;
	default:
		break;
	}
	
	return len;
}

/*interface of read proc*/
static int ft5402_debug_read( char *page, char **start,
	off_t off, int count, int *eof, void *data )
{
	struct i2c_client *client = (struct i2c_client *)ft5402_proc_entry->data;
	int ret = 0;
	unsigned char buf[PAGE_SIZE];
	int num_read_chars = 0;
	int readlen = 0;
	u8 regvalue = 0x00, regaddr = 0x00;
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		/*after calling ft5x0x_debug_write to upgrade*/
		regaddr = 0xA6;
		ret = ft5402_read_reg(client, regaddr, &regvalue);
		if (ret < 0)
			num_read_chars = sprintf(buf, "%s", "get fw version failed.\n");
		else
			num_read_chars = sprintf(buf, "current fw version:0x%02x\n", regvalue);
		break;
	case PROC_READ_REGISTER:
		readlen = 1;
		ret = ft5402_i2c_Read(client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&client->dev, "%s:read iic error\n", __func__);
			return ret;
		} else
			//DBG("%s:value=0x%02x\n", __func__, buf[0]);
		num_read_chars = 1;
		break;
	default:
		break;
	}
	
	memcpy(page, buf, num_read_chars);

	return num_read_chars;
}
int ft5402_create_apk_debug_channel(struct i2c_client * client)
{
	ft5402_proc_entry = create_proc_entry(PROC_NAME, 0777, NULL);
	if (NULL == ft5402_proc_entry) {
		dev_err(&client->dev, "Couldn't create proc entry!\n");
		return -ENOMEM;
	} else {
		dev_info(&client->dev, "Create proc entry success!\n");
		ft5402_proc_entry->data = client;
		ft5402_proc_entry->write_proc = ft5402_debug_write;
		ft5402_proc_entry->read_proc = ft5402_debug_read;
	}
	return 0;
}

void ft5402_release_apk_debug_channel(void)
{
	if (ft5402_proc_entry)
		remove_proc_entry(PROC_NAME, NULL);
}
#endif

static int aw_open(struct inode *inode, struct file *file);
static long aw_ioctl(struct file *file, unsigned int cmd,unsigned long arg );
static int aw_release (struct inode *inode, struct file *file);

static const struct file_operations aw_i2c_ts_fops ={	
	.owner = THIS_MODULE, 		
	.open = aw_open, 	
	.unlocked_ioctl = aw_ioctl,	
	.release = aw_release, 
};

static int ft5x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ft5x_ts_data *ft5x_ts;
	struct input_dev *input_dev;
	struct device *dev;
	struct i2c_dev *i2c_dev;
	int err = 0;
        

#ifdef TOUCH_KEY_SUPPORT
	int i = 0;
#endif

	dprintk(DEBUG_INIT,"====%s begin=====.  \n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		printk("check_functionality_failed\n");
		goto exit_check_functionality_failed;
	}


	ft5x_ts = kzalloc(sizeof(*ft5x_ts), GFP_KERNEL);
	if (!ft5x_ts)	{
		err = -ENOMEM;
		printk("alloc_data_failed\n");
		goto exit_alloc_data_failed;
	}

	this_client = client;
	i2c_set_clientdata(client, ft5x_ts);

	/*
	if(inet_getChipA3(client)==0)
	{
			err=-ENODEV;
			goto exit_create_singlethread;
	}

	printk("%s\n",FT5402_USED);
	*/
	
#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
	//fts_ctpm_fw_upgrade_with_i_file();
#endif

	INIT_WORK(&ft5x_ts->pen_event_work, ft5x_ts_pen_irq_work);
	ft5x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ft5x_ts->ts_workqueue) {
		err = -ESRCH;
		printk("ts_workqueue fail!\n");
		goto exit_create_singlethread;
	}
#ifdef CHARGE_MODE
	//mbg ++ 20131115 for charge check
	INIT_WORK(&ft5x_ts->pen_event_work_charge, write_charge_cmd);
	ft5x_ts->ts_workqueue_charge = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ft5x_ts->ts_workqueue_charge) {
		err = -ESRCH;
		printk("ts_workqueue_charge fail!\n");
		goto exit_create_singlethread;
	}
	//mbg --
#endif
	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	ft5x_ts->input_dev = input_dev;
#ifdef CHARGE_MODE
	//mbg ++ 20131115 for charge check
	setup_timer(&ft5x_ts->polling_timer, polling_timer_func, (unsigned long)ft5x_ts);
	mod_timer(&ft5x_ts->polling_timer,jiffies + msecs_to_jiffies(800));
	//--
#endif
#ifdef CONFIG_FT5X0X_MULTITOUCH
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);	
#ifdef FOR_TSLIB_TEST
	set_bit(BTN_TOUCH, input_dev->keybit);
#endif
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_TRACKING_ID, 0, 4, 0, 0);
#ifdef TOUCH_KEY_SUPPORT
	key_tp = 0;
	input_dev->evbit[0] = BIT_MASK(EV_KEY);
	for (i = 1; i < TOUCH_KEY_NUMBER; i++)
		set_bit(i, input_dev->keybit);
#endif
#else
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(ABS_PRESSURE, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	input_set_abs_params(input_dev, ABS_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_PRESSURE, 0, PRESS_MAX, 0 , 0);
#endif

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);

	input_dev->name	= CTP_NAME;		//dev_name(&client->dev)
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,"ft5x_ts_probe: failed to register input device: %s\n",
		        dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

	ft5x_resume_wq = create_singlethread_workqueue("ft5x_resume");
	if (ft5x_resume_wq == NULL) {
		printk("create ft5x_resume_wq fail!\n");
		return -ENOMEM;
	}
   	 msleep(150);  /*make sure CTP already finish startup process*/
#ifdef FT5402_UPGRADE
	/*upgrade for program the app to RAM*/
	dev_dbg(&client->dev, "[FTS]----ready for upgrading---\n");
	if(inet_getChipA6(client)!=0)
	{
		if (fts_ctpm_fw_upgrade_with_i_file(client) < 0) {
			dev_err(&client->dev, "[FTS]-----upgrade failed!----\n");
		}
		else
			dev_dbg(&client->dev, "[FTS]-----upgrade successful!----\n");
	}
#endif
	msleep(200);
	ft5402_read_reg(client, 0xa3, &chip_type);
	printk("======chip_type = %d========\n", chip_type);
	if(chip_type == 3) //chip_type = ft5402
	{
//	#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
/*
	if(1==update_if){
		msleep(250);
		ft5402_ctpm_fw_upgrade_with_i_file(client);
		vfree(CTPM_FW);
		update_if=0;
	}
	*/
//	#endif
		//printk("begin ft5402_Init_IC_Param");
		msleep(50);
		ft5402_Init_IC_Param(client);
		msleep(50);
		//printk("end ft5402_Init_IC_Param");
	
		
	#ifdef SYSFS_DEBUG
		ft5402_get_ic_param(client);
		ft5402_create_sysfs_debug(client);
	#endif

	#ifdef FTS_APK_DEBUG
		ft5402_create_apk_debug_channel(client);
	#endif
	}
	else//chip_type = ft5x06
	{
//#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
/*
	if(1==update_if){
	msleep(250);
	fts_ctpm_fw_upgrade_with_i_file();
	vfree(CTPM_FW);
	update_if=0;
	}
	*/
//#endif
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	printk("==register_early_suspend =\n");
	ft5x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ft5x_ts->early_suspend.suspend = ft5x_ts_early_suspend;
	ft5x_ts->early_suspend.resume	= ft5x_ts_late_resume;
	register_early_suspend(&ft5x_ts->early_suspend);
#endif

#ifdef CONFIG_FT5X0X_MULTITOUCH
	dprintk(DEBUG_INIT,"CONFIG_FT5X0X_MULTITOUCH is defined. \n");
#endif
 	device_enable_async_suspend(&client->dev);//Paul added for async suspend @20130418
  int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)ft5x_ts_interrupt,ft5x_ts);
	if (!int_handle) {
		printk("ft5x_ts_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	
	ctp_set_int_port_rate(1);
	ctp_set_int_port_deb(0x07);
	dprintk(DEBUG_INIT,"reg clk: 0x%08x\n", readl(0xf1c20a18));

    	i2c_dev = get_free_i2c_dev(client->adapter);	
	if (IS_ERR(i2c_dev)){	
		err = PTR_ERR(i2c_dev);	
		printk("i2c_dev fail!");	
		return err;	
	}
	
	err = register_chrdev(I2C_MAJOR,"aw_i2c_ts",&aw_i2c_ts_fops );	
	if(err) {	
		printk("%s:register chrdev failed\n",__FILE__);	
		return err;
	}
	
	i2c_dev_class = class_create(THIS_MODULE,"aw_i2c_dev");
	if (IS_ERR(i2c_dev_class)) {		
		err = PTR_ERR(i2c_dev_class);		
		class_destroy(i2c_dev_class);
		unregister_chrdev(I2C_MAJOR, "aw_i2c_ts");	
	}
	else
	{
		dev = device_create(i2c_dev_class, &client->adapter->dev, MKDEV(I2C_MAJOR,client->adapter->nr),
		         NULL, "aw_i2c_ts%d", client->adapter->nr);	
		if (IS_ERR(dev))	{		
				err = PTR_ERR(dev);
				printk("dev fail!\n");		
				return err;	
		}
	}

	dprintk(DEBUG_INIT,"==%s over =\n", __func__);
	return 0;

exit_irq_request_failed:
        sw_gpio_irq_free(int_handle);
        cancel_work_sync(&ft5x_resume_work);
	destroy_workqueue(ft5x_resume_wq);	
exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
        i2c_set_clientdata(client, NULL);
        cancel_work_sync(&ft5x_ts->pen_event_work);
	destroy_workqueue(ft5x_ts->ts_workqueue);
#ifdef CHARGE_MODE
	cancel_work_sync(&ft5x_ts->pen_event_work_charge);
	destroy_workqueue(ft5x_ts->ts_workqueue_charge);
#endif
exit_create_singlethread:
	kfree(ft5x_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
        
	return err;
}

static int __devexit ft5x_ts_remove(struct i2c_client *client)
{
#ifdef CHARGE_MODE
	del_timer_sync(&write_timer);
#endif	
	struct ft5x_ts_data *ft5x_ts = i2c_get_clientdata(client);
	ft5x_set_reg(FT5X0X_REG_PMODE, PMODE_HIBERNATE);
	
	printk("==ft5x_ts_remove=\n");
	device_destroy(i2c_dev_class, MKDEV(I2C_MAJOR,client->adapter->nr));
	class_destroy(i2c_dev_class);
	unregister_chrdev(I2C_MAJOR, "aw_i2c_ts");
	sw_gpio_irq_free(int_handle);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ft5x_ts->early_suspend);
#endif
	cancel_work_sync(&ft5x_resume_work);
	destroy_workqueue(ft5x_resume_wq);
	input_unregister_device(ft5x_ts->input_dev);
	input_free_device(ft5x_ts->input_dev);
	cancel_work_sync(&ft5x_ts->pen_event_work);
	destroy_workqueue(ft5x_ts->ts_workqueue);
#ifdef CHARGE_MODE
	cancel_work_sync(&ft5x_ts->pen_event_work_charge);
	destroy_workqueue(ft5x_ts->ts_workqueue_charge);
#endif
	kfree(ft5x_ts);
    
	i2c_set_clientdata(this_client, NULL);
	ctp_free_platform_resource();

	return 0;

}

static const struct i2c_device_id ft5x_ts_id[] = {
	{ CTP_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ft5x_ts_id);

static struct i2c_driver ft5x_ts_driver = {
	.class          = I2C_CLASS_HWMON,
	.probe		= ft5x_ts_probe,
	.remove		= __devexit_p(ft5x_ts_remove),
	.id_table	= ft5x_ts_id,
	.suspend        = ft5x_ts_suspend,
	.resume         = ft5x_ts_resume,
	.driver	= {
		.name	= CTP_NAME,
		.owner	= THIS_MODULE,
	},
	.address_list	= normal_i2c,

};

static int aw_open(struct inode *inode, struct file *file)
{
	int subminor;
	int ret = 0;	
	struct i2c_client *client;
	struct i2c_adapter *adapter;	
	struct i2c_dev *i2c_dev;	

	printk("====%s======.\n", __func__);
	dprintk(DEBUG_OTHERS_INFO,"enter aw_open function\n");
	subminor = iminor(inode);
	dprintk(DEBUG_OTHERS_INFO,"subminor=%d\n",subminor);
	
	//lock_kernel();	
	i2c_dev = i2c_dev_get_by_minor(2);	
	if (!i2c_dev)	{	
		printk("error i2c_dev\n");		
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
	client->driver = &ft5x_ts_driver;
	client->adapter = adapter;		
	file->private_data = client;
		
	return 0;
}

static long aw_ioctl(struct file *file, unsigned int cmd,unsigned long arg ) 
{
	//struct i2c_client *client = (struct i2c_client *) file->private_data;

	dprintk(DEBUG_OTHERS_INFO,"====%s====\n",__func__);
	dprintk(DEBUG_OTHERS_INFO,"line :%d,cmd = %d,arg = %ld.\n",__LINE__,cmd,arg);
	/*
	switch (cmd) {
	case UPGRADE:
	        dprintk(DEBUG_OTHERS_INFO,"==UPGRADE_WORK=\n");
		fts_ctpm_fw_upgrade_with_i_file();
		// calibrate();
		break;
	default:
		break;			 
	}	
	*/
	return 0;
}

static int aw_release (struct inode *inode, struct file *file) 
{
	struct i2c_client *client = file->private_data;
	dprintk(DEBUG_OTHERS_INFO,"enter aw_release function.\n");		
	i2c_put_adapter(client->adapter);
	kfree(client);
	file->private_data = NULL;
	return 0;	  
}

static int ctp_get_system_config(void)
{   
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
static int __init ft5x_ts_init(void)
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
	
	printk("%s\n",FT5402_IC_INFO);
	
	printk("[FUN]%s\n", __func__);
	
	if(!ctp_get_system_config())
	{
		printk("%s:read config fail!\n",__func__);
		return ret;
	}
        
	ctp_wakeup(0,20);  
		
	ft5x_ts_driver.detect = ctp_detect;

    ret = i2c_add_driver(&ft5x_ts_driver);
    
    dprintk(DEBUG_INIT,"****************************************************************\n");
	return ret;
}

static void __exit ft5x_ts_exit(void)
{
	printk("==ft5x_ts_exit==\n");
	i2c_del_driver(&ft5x_ts_driver);
}



late_initcall(ft5x_ts_init);
module_exit(ft5x_ts_exit);
MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x TouchScreen driver");
MODULE_LICENSE("GPL");

