/* 
 * drivers/input/touchscreen/ct36x_ts.c
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

#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/gpio.h> 
#include <linux/ctp.h>

// for android 4.x only
#include <linux/input/mt.h>

#include "ct36x_ts.h"


//#define CONFIG_SUPPORT_FTS_CTP_UPG
#define CT36X_TS_CHIP_DEBUG 0

#define CT36X_TS_NAME "ct36x"

#define CTP_IRQ_NUMBER          (config_info.irq_gpio_number)
#define CTP_IRQ_MODE			(TRIG_EDGE_NEGATIVE)

static u32 debug_mask = 0;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("***CTP***"fmt, ## arg)

//for CT36X
#define CT36X_I2C_ADDR 0x53

#define CT36X_USED     "\n \
												 \n+++++++++++++++++++++++++++++++++ \
                         \n++++++      CT36X  used +++++++++ \
                         \n+++++++++++++++++++++++++++++++++ \
                         \n"
                            
#define CT36X_IC_INFO  "\n============================================================== \
											   \nIC     :CT36X \
                         \nAUTHOR :p.huan @163.com \
                         \nVERSION:2013-02-03_09:56\n"  
                         
extern int m_inet_ctpState;
extern struct ctp_config_info config_info;

void ct36x_chip_go_sleep(struct i2c_client *client, unsigned char *buf);

static struct i2c_client *this_client;

static __u32 twi_id = 0;
static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {CT36X_I2C_ADDR,I2C_CLIENT_END};

static int ct36x_ts_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if(twi_id == adapter->nr)
	{
		printk("%s: Detected chip %s at adapter %d, address 0x%02x\n",
			 __func__, CT36X_TS_NAME, i2c_adapter_id(adapter), client->addr);
		strlcpy(info->type, CT36X_TS_NAME, I2C_NAME_SIZE);
			
		return 0;
	}
	else
	{
		return -ENODEV;
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

	ret=i2c_master_recv(this_client, pbt_buf, dw_lenth);

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


	if(!i2c_write_interface(CT36X_I2C_ADDR, read_cmd, cmd_len))	{//change by zhengdixu
		return FTS_FALSE;
	}

	/*call the read callback function to get the register value*/        
	if(!i2c_read_interface(CT36X_I2C_ADDR, pbt_buf, bt_len)){
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
	//return i2c_write_interface(CT36X_I2C_ADDR, &write_cmd, 2);
	return i2c_write_interface(CT36X_I2C_ADDR, write_cmd, 2); //change by zhengdixu
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
	//return i2c_write_interface(CT36X_I2C_ADDR, &write_cmd, num);
	return i2c_write_interface(CT36X_I2C_ADDR, write_cmd, num);//change by zhengdixu
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
	return i2c_write_interface(CT36X_I2C_ADDR, pbt_buf, dw_len);
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
	return i2c_read_interface(CT36X_I2C_ADDR, pbt_buf, bt_len);
	//ft5x_i2c_rxdata
}

static int ct36x_i2c_rxdata(void *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = 
	{
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

static int ct36x_i2c_txdata(char *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = 
	{
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

	ret = i2c_transfer(this_client->adapter, msg, 1);
	
	if (ret < 0)
		printk("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

static int ct36x_read_data(void)
{
	struct ct36x_ts_data *data = i2c_get_clientdata(this_client);
	struct ct36x_finger_info buf[10]={{0}};
	
	int ret = -1;
	int iter = 0;
	int x, y;
	static int sync, press, release;

	ret = ct36x_i2c_rxdata(buf, sizeof(struct ct36x_finger_info)*10);
	if (ret < 0) 
	{
		dprintk(DEBUG_X_Y_INFO,"%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	/* report points */
	sync = 0; press = 0;
	for ( iter = 0; iter < 10; iter++ ) {
		if ( buf[iter].xhi != 0xFF && buf[iter].yhi != 0xFF &&
		     (buf[iter].status == 1 || buf[iter].status == 2) ) {
		#if 0//def CONFIG_TOUCHSCREEN_CT36X_MISC_XY_SWAP
			x = (buf[iter].yhi<<4)|(buf[iter].ylo&0xF);
			y = (buf[iter].xhi<<4)|(buf[iter].xlo&0xF);
		#else
			x = (buf[iter].xhi<<4)|(buf[iter].xlo&0xF);
		 	y = (buf[iter].yhi<<4)|(buf[iter].ylo&0xF);
		#endif
		#if 0//def CONFIG_TOUCHSCREEN_CT36X_MISC_X_REVERSE
			x = CT36X_TS_ABS_X_MAX - x;
		#endif
		#if 0//def CONFIG_TOUCHSCREEN_CT36X_MISC_Y_REVERSE
			y = CT36X_TS_ABS_Y_MAX - y;
		#endif
		
			
			//printk("X: %d   Y:    %d\n", x, y);
			//printk("Y:        %d\n", y);

		#if 1
			input_mt_slot(data->input_dev, buf[iter].id - 1);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, true);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 30);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 128);
		#else
			input_report_abs(ts->input, ABS_MT_TRACKING_ID, ts->data.pts[iter].id - 1);
			input_report_abs(ts->input, ABS_MT_POSITION_X,  x);
			input_report_abs(ts->input, ABS_MT_POSITION_Y,  y);
			input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 30);
			input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 128);
			
			input_mt_sync(ts->input);
		#endif

		
			sync = 1;
			press |= 0x01 << (buf[iter].id - 1);
		}
	}

	release &= release ^ press;
	for ( iter = 0; iter < 10; iter++ ) {
		if ( release & (0x01<<iter) ) {
		#if 1	// android 4.x
			input_mt_slot(data->input_dev, iter);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
		#else	// android 2.x / others
			input_report_abs(ts->input, ABS_MT_TRACKING_ID, iter);
			input_report_abs(ts->input, ABS_MT_POSITION_X,  x);
			input_report_abs(ts->input, ABS_MT_POSITION_Y,  y);
			input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 0);

			input_mt_sync(ts->input);
		#endif
			sync = 1;
		}
	}
	release = press;

	if ( sync ) 
		input_sync(data->input_dev);

        return 0;
}

static void ct36x_ts_irq_work(struct work_struct *work)
{
	ct36x_read_data();
}

static u32 ct36x_ts_interrupt(struct ct36x_ts_data *ct36x_ts)
{
	//pr_info("==========ct36x_ts TS Interrupt============\n"); 
	queue_work(ct36x_ts->ts_workqueue, &ct36x_ts->work);
	
	return 0;
}

static int slpflg = 0;
#ifdef CONFIG_HAS_EARLYSUSPEND
static void ct36x_ts_suspend(struct early_suspend *handler)
{
	struct ct36x_ts_data *data = i2c_get_clientdata(this_client);
	
	dprintk(DEBUG_SUSPEND,"==ct36x_ts_suspend=\n");
	
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
	cancel_work_sync(&data->work);
	flush_workqueue(data->ts_workqueue);

	if ( slpflg == 0 )
	{	// state of wake
		char buf[16];
		ct36x_chip_go_sleep(this_client, buf);
		slpflg = 1;
	}
}

static void ct36x_ts_resume(struct early_suspend *handler)
{
	dprintk(DEBUG_SUSPEND,"==CONFIG_HAS_EARLYSUSPEND:ct36x_ts_resume== \n");
	
	ctp_wakeup(0,20);
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
	
	if(STANDBY_WITH_POWER_OFF == standby_level)
	{
	    msleep(100);
	}
	
	slpflg = 0;
}
#else //CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_PM
static int ct36x_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
    struct ct36x_ts_data *data = i2c_get_clientdata(client);
	
    dprintk(DEBUG_SUSPEND,"==ct36x_ts_suspend==\n");
    dprintk(DEBUG_SUSPEND,"CONFIG_PM: write FT5X0X_REG_PMODE .\n");
	
    sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
    cancel_work_sync(&data->work);
    flush_workqueue(data->ts_workqueue);
    
    if ( slpflg == 0 ) 
	{	// state of wake
	    char buf[16];
		ct36x_chip_go_sleep(this_client, buf);
		slpflg = 1;
	}
	
    return 0;
}
static int ct36x_ts_resume(struct i2c_client *client)
{
	dprintk(DEBUG_SUSPEND,"==CONFIG_PM:ct36x_ts_resume== \n");
	
	ctp_wakeup(0,20);
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
	slpflg = 0;
	
	return 0;		
}
#endif
#endif

#if 1
static unsigned char binary_data[] = {
//#include "CT363_XKP_97_V01121229A.dat"
#include "HBS_97_CT363_V05_F65A_130130.dat"
//#include "CT363_SCOPE_97_SP0917_V02_121229.dat"
};
#endif
//extern  char  binary_data [32768];

void ct36x_ts_reg_read(struct i2c_client *client, unsigned short addr, char *buf, int len)
{
	struct i2c_msg msgs;

	msgs.addr = addr;
	msgs.flags = 0x01;  // 0x00: write 0x01:read 
	msgs.len = len;
	msgs.buf = buf;

	i2c_transfer(client->adapter, &msgs, 1);
}

void ct36x_ts_reg_write(struct i2c_client *client, unsigned short addr, char *buf, int len)
{
	struct i2c_msg msgs;

	msgs.addr = addr;
	msgs.flags = 0x00;  // 0x00: write 0x01:read 
	msgs.len = len;
	msgs.buf = buf;

	i2c_transfer(client->adapter, &msgs, 1);
}

static void ct36x_chip_set_idle(struct i2c_client *client, unsigned char *buf)
{
	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	buf[0] = 0x00;
	buf[1] = 0xA5;
	ct36x_ts_reg_write(client, 0x7F, buf, 2);
	mdelay(10);
}

static void ct36x_chip_rst_offset(struct i2c_client *client, unsigned char *buf)
{
	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	buf[0] = 0x00;
	ct36x_ts_reg_write(client, 0x7F, buf, 1);
	mdelay(10);
}

static int ct36x_chip_get_busstatus(struct i2c_client *client, unsigned char *buf)
{
	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	ct36x_ts_reg_read(client, 0x7F, buf, 1);
	mdelay(10);

	return buf[0];
}

static int ct36x_chip_erase_flash(struct i2c_client *client, unsigned char *buf)
{
	int ret = -1;

	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	// Erase 32k flash
	buf[0] = 0x00;
	buf[1] = 0x33;
	buf[2] = 0x00;
	ct36x_ts_reg_write(client, 0x7F, buf, 3);
	mdelay(10);

	// Reset I2C offset address
	ct36x_chip_rst_offset(client, buf);

	// Read I2C bus status
	ret = ct36x_chip_get_busstatus(client, buf);
	if ( ret != 0xAA ) {
		return -1;
	}

	return 0;
}

/*
** Prepare code segment
*/
static int ct36x_chip_set_code(unsigned int flash_addr, unsigned char *buf)
{
	unsigned char cod_chksum;

	//if ( CT36X_TS_CHIP_DEBUG )
	//printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	// Flash address
	// data length
	buf[2] = (char)(flash_addr >> 8);
	buf[3] = (char)(flash_addr & 0xFF);
	buf[4] = 0x08;

	// Fill firmware source data
	//if ( (sec == 1 && cod == 4) || (sec == 1 && cod == 5) ) {
	//if ( flash_addr == (CT36X_CHIP_FLASH_SECTOR_SIZE + 32) || 
	//flash_addr == (CT36X_CHIP_FLASH_SECTOR_SIZE + 40) ) {
	if ( flash_addr == (160) || flash_addr == (168) ) {
		buf[6] = ~binary_data[flash_addr + 0];
		buf[7] = ~binary_data[flash_addr + 1];
		buf[8] = ~binary_data[flash_addr + 2];
		buf[9] = ~binary_data[flash_addr + 3];
		buf[10] = ~binary_data[flash_addr + 4];
		buf[11] = ~binary_data[flash_addr + 5];
		buf[12] = ~binary_data[flash_addr + 6];
		buf[13] = ~binary_data[flash_addr + 7];
	} else {
		buf[6] = binary_data[flash_addr + 0];
		buf[7] = binary_data[flash_addr + 1];
		buf[8] = binary_data[flash_addr + 2];
		buf[9] = binary_data[flash_addr + 3];
		buf[10] = binary_data[flash_addr + 4];
		buf[11] = binary_data[flash_addr + 5];
		buf[12] = binary_data[flash_addr + 6];
		buf[13] = binary_data[flash_addr + 7];
	}
			
	/* Calculate a checksum by Host controller. 
	** Checksum =  ~(FLASH_ADRH+FLASH_ADRL+LENGTH+
	** Binary_Data1+Binary_Data2+Binary_Data3+Binary_Data4+
	** Binary_Data5+Binary_Data6+Binary_Data7+Binary_Data8) + 1
	*/
	cod_chksum = ~(buf[2]+buf[3]+buf[4]+
				buf[6]+buf[7]+buf[8]+buf[9]+
				buf[10]+buf[11]+buf[12]+buf[13]) + 1;
	buf[5] = cod_chksum;

	return cod_chksum;
}

static int ct36x_chip_write_firmware(struct i2c_client *client, unsigned char *buf)
{
//	int ret = -1;
	int sec, cod;
	unsigned char cod_chksum;
	unsigned int fin_chksum;
	unsigned int flash_addr;

	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	// Code checksum, final checksum
	cod_chksum = 0x00; fin_chksum = 0x00;

	// Flash write command
	buf[0] = 0x00;
	buf[1] = 0x55;

	// 256 sectors, 128 bytes per sectors
	for ( sec = 0; sec < CT36X_CHIP_FLASH_SECTOR_NUM; sec++ ) {
		flash_addr = sec * CT36X_CHIP_FLASH_SECTOR_SIZE;
		// 16 segments, 8 bytes per segment
		for ( cod = 0; cod < (CT36X_CHIP_FLASH_SECTOR_SIZE/CT36X_CHIP_FLASH_SOURCE_SIZE); cod++ ) {
			// Fill binary data
			cod_chksum = ct36x_chip_set_code(flash_addr, buf);
			fin_chksum += cod_chksum;

			// Write firmware source data
			ct36x_ts_reg_write(client, 0x7F, buf, 14);

			// 
			mdelay(1);

			// Increase flash address 8bytes for each write command
			flash_addr += CT36X_CHIP_FLASH_SOURCE_SIZE;
		}
		//
		mdelay(20);
	}

	return 0;
}

int ct36x_chip_get_binchksum(unsigned char *buf)
{
	int sec, cod;
	unsigned char cod_chksum;
	unsigned int fin_chksum = 0;
	unsigned int flash_addr;

	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	// 256 sectors, 128 bytes per sectors
	for ( sec = 0; sec < CT36X_CHIP_FLASH_SECTOR_NUM; sec++ ) {
		flash_addr = sec * CT36X_CHIP_FLASH_SECTOR_SIZE;
		// 16 segments, 8 bytes per segment
		for ( cod = 0; cod < (CT36X_CHIP_FLASH_SECTOR_SIZE/CT36X_CHIP_FLASH_SOURCE_SIZE); cod++ ) {
			// Fill binary data
			cod_chksum = ct36x_chip_set_code(flash_addr, buf);
			fin_chksum += cod_chksum;

			// Increase flash address 8bytes for each write command
			flash_addr += CT36X_CHIP_FLASH_SOURCE_SIZE;
		}
	}

	return (unsigned short)fin_chksum;
}

int ct36x_chip_get_fwchksum(struct i2c_client *client, unsigned char *buf)
{
	int fwchksum = 0x00;

	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	buf[0] = 0xFF;
	buf[1] = 0x8F;
	buf[2] = 0xFF;
	ct36x_ts_reg_write(client, client->addr, buf, 3);
	mdelay(20);

	buf[0] = 0x00;
	buf[1] = 0xE1;
	ct36x_ts_reg_write(client, client->addr, buf, 2);
	mdelay(500);

	buf[0] = 0xFF;
	buf[1] = 0x8E;
	buf[2] = 0x0E;
	ct36x_ts_reg_write(client, client->addr, buf, 3);
	mdelay(20);

	ct36x_chip_rst_offset(client, buf);

	ct36x_ts_reg_read(client, client->addr, buf, 3);
	mdelay(20);

	fwchksum = ((buf[0]<<8) | buf[1]);

	return fwchksum;
}

int ct36x_chip_get_ver(struct i2c_client *client, unsigned char *buf)
{
	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	// Read version command
	buf[0] = 0xFF;
	buf[1] = 0x3F;
	buf[2] = 0xFF;

	ct36x_ts_reg_write(client, client->addr, buf, 3);
	mdelay(10);

	buf[0] = 0x00;
	ct36x_ts_reg_write(client, client->addr, buf, 1);
	mdelay(10);

	// do read version
	ct36x_ts_reg_read(client, client->addr, buf, 1);
	mdelay(10);

	return buf[0];
}

int ct36x_chip_get_vendor(struct i2c_client *client, unsigned char *buf)
{
	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	return 0;
}

void ct36x_chip_go_sleep(struct i2c_client *client, unsigned char *buf)
{
	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	buf[0] = 0xFF;
	buf[1] = 0x8F;
	buf[2] = 0xFF;
	ct36x_ts_reg_write(client, client->addr, buf, 3);
	mdelay(3);

	buf[0] = 0x00;
	buf[1] = 0xAF;
	ct36x_ts_reg_write(client, client->addr, buf, 2);
	mdelay(3);

	//mdelay(50);
}

static int ct36x_chip_read_infoblk(struct i2c_client *client, unsigned char *buf)
{
	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	return 0;
}

static int ct36x_chip_erase_infoblk(struct i2c_client *client, unsigned char *buf)
{
	int ret = -1;

	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	// info block erase command
	buf[0] = 0x00;
	buf[1] = 0x60;
	buf[2] = 0x00;
	ct36x_ts_reg_write(client, 0x7F, buf, 3);
	mdelay(10);

	// Reset I2C offset address
	ct36x_chip_rst_offset(client, buf);

	// Read I2C bus status
	ret = ct36x_chip_get_busstatus(client, buf);
	if ( ret != 0xAA ) {
		printk("trim data erase error!!! \n");
		return -1;
	}

	return 0;
}

static int ct36x_chip_write_infoblk(struct i2c_client *client, unsigned char *buf)
{
	//int ret = -1;
	int sec, cod;
	unsigned int flash_addr;

	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	flash_addr = 0x00;

	// write info block 0
	buf[0] = 0x00;
	buf[1] = 0x61;

	for ( cod = 0; cod < 16; cod++ ) {
	// Flash address
	// data length
	buf[2] = (char)(flash_addr >> 8);
	buf[3] = (char)(flash_addr & 0xFF);
	buf[4] = 0x08;
	if ( flash_addr == 0x0000 )
	buf[6] = 0x17;
	else
	buf[6] = 0x00;
	
	buf[7] = 0x00;
	buf[8] = 0x00;
	buf[9] = 0x00;
	buf[10] = 0x00;
	buf[11] = 0x00;
	buf[12] = 0x00;
	buf[13] = 0x00;

	buf[5] = (~(buf[2]+buf[3]+buf[4]+buf[6]+buf[7]+buf[8]+buf[9]+buf[10]+buf[11]+buf[12]+buf[13]))+1;
		
	ct36x_ts_reg_write(client, 0x7F, buf, 14);
	mdelay(10);

	flash_addr += 8;
	}

	return 0;
}

int ct36x_chip_go_bootloader(struct i2c_client *client, unsigned char *buf)
{
	int ret = -1;

	if ( CT36X_TS_CHIP_DEBUG )
	printk(">>>>> %s() called <<<<< \n", __FUNCTION__);

	// Init bootloader
	ct36x_chip_set_idle(client, buf);

	// Reset I2C offset address
	ct36x_chip_rst_offset(client, buf);

	// Get I2C bus status
	ret = ct36x_chip_get_busstatus(client, buf);
	if ( ret != 0xAA ) {
		printk("I2C bus status: 0x%x.\n", ret);
		return -1;
	}

	// trim adc
	ct36x_chip_read_infoblk(client, buf);
	ct36x_chip_erase_infoblk(client, buf);
	ct36x_chip_write_infoblk(client, buf);

	// Erase flash
	ret = ct36x_chip_erase_flash(client, buf);
	if ( ret ) {
		printk("Erase flash failed.\n");
		return -1;
	}

	// Write source data
	ct36x_chip_write_firmware(client, buf);
	
	return 0;
}


static int test_i2c(void)
{
	int ret;
	int retries = 5;
	u8 buf[1];

	ctp_wakeup(0,50);
	msleep(500);
	
	while(retries--)
	{
		ret = byte_read(buf, 1);
		printk("ret =%d\n",ret);
		if (ret > 0)
		{
			m_inet_ctpState = 1;
			printk("I2C communication ok!\n");
			
			return 0;
		}
	}

	m_inet_ctpState = 0;
	printk("I2C communication ERROR!\n");

	return -1;
}


static int 
ct36x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ct36x_ts_data *ct36x_ts;
	struct input_dev *input_dev;
	int err = 0;
	unsigned char buf[18];
	int binchksum, fwchksum, updcnt;

	this_client = client;
	err = test_i2c();
	if (err < 0)
		goto exit_test_i2c_failed;

	printk("%s",CT36X_USED);

	dprintk(DEBUG_INIT,"====%s begin=====.  \n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		printk("check_functionality_failed\n");
		goto exit_check_functionality_failed;
	}

	ct36x_ts = kzalloc(sizeof(*ct36x_ts), GFP_KERNEL);
	if (ct36x_ts == NULL)	
	{
		err = -ENOMEM;
		printk("alloc_data_failed\n");
		goto exit_alloc_data_failed;
	}

	i2c_set_clientdata(client, ct36x_ts);


#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
//	ctp_wakeup(0,50);
//	mdelay(500);
	// Get binary Checksum
	binchksum = ct36x_chip_get_binchksum(buf);
	printk("Bin checksum: 0x%x\n", binchksum);

	// Get firmware Checksum
	fwchksum = ct36x_chip_get_fwchksum(client, buf);
	printk("Fw checksum: 0x%x\n", fwchksum);

	updcnt = 1;
	while (binchksum != fwchksum && updcnt--) 
	{
		/* Update Firmware */
		ct36x_chip_go_bootloader(client, buf);

		/* Hardware reset */
		//ct36x_platform_hw_reset(ts);
		ctp_wakeup(0,50);
		mdelay(500);

		// Get firmware Checksum
		fwchksum = ct36x_chip_get_fwchksum(client, buf);
		printk("Fw checksum: 0x%x\n", fwchksum);
	}

	printk("Fw update %s. 0x%x, 0x%x\n", binchksum != fwchksum ? "Failed" : "Success", binchksum, fwchksum);
#endif

	ct36x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ct36x_ts->ts_workqueue) {
		err = -ESRCH;
		printk("ts_workqueue fail!\n");
		goto exit_create_workqueue;
	}
	
	INIT_WORK(&ct36x_ts->work, ct36x_ts_irq_work);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	ct36x_ts->input_dev = input_dev;

	// For android 4.x only
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	
	// For android 4.x only
	input_mt_init_slots(input_dev, 10);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, 1024, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, 768, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);

	input_dev->name	= CT36X_TS_NAME;		//dev_name(&client->dev)
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,"ct36x_ts_probe: failed to register input device: %s\n",
		        dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	printk("==register_early_suspend =\n");
	ct36x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ct36x_ts->early_suspend.suspend = ct36x_ts_suspend;
	ct36x_ts->early_suspend.resume	= ct36x_ts_resume;
	register_early_suspend(&ct36x_ts->early_suspend);
#endif

    int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)ct36x_ts_interrupt, ct36x_ts);
	if (!int_handle) {
		printk("ct36x_ts_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}

	ctp_wakeup(0,50);

	dprintk(DEBUG_INIT,"==%s over =\n", __func__);
	return 0;

exit_irq_request_failed:
exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	cancel_work_sync(&ct36x_ts->work);
	destroy_workqueue(ct36x_ts->ts_workqueue);
exit_create_workqueue:
	i2c_set_clientdata(client, NULL);
	kfree(ct36x_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
exit_test_i2c_failed:
	
	return err;
}

static void ct36x_ts_shutdown(struct i2c_client *client)
{
	char buf[16];
	ct36x_chip_go_sleep(client, buf);
}

static int __devexit ct36x_ts_remove(struct i2c_client *client)
{

	struct ct36x_ts_data *ct36x_ts = i2c_get_clientdata(client);

	printk("==ct36x_ts_remove==\n");
	sw_gpio_irq_free(int_handle);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ct36x_ts->early_suspend);
#endif
	input_unregister_device(ct36x_ts->input_dev);
	input_free_device(ct36x_ts->input_dev);
	cancel_work_sync(&ct36x_ts->work);
	destroy_workqueue(ct36x_ts->ts_workqueue);
	kfree(ct36x_ts);
    
	i2c_set_clientdata(client, NULL);
	ctp_free_platform_resource();

	return 0;

}

static const struct i2c_device_id ct36x_ts_id[] = {
	{ CT36X_TS_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ct36x_ts_id);

static struct i2c_driver ct36x_ts_driver = {
	.class = I2C_CLASS_HWMON,
	.probe		= ct36x_ts_probe,
	.remove		= __devexit_p(ct36x_ts_remove),
	.shutdown	= ct36x_ts_shutdown,
#ifdef CONFIG_HAS_EARLYSUSPEND

#else
#ifdef CONFIG_PM
	.suspend  =  ct36x_ts_suspend,
	.resume   =  ct36x_ts_resume,
#endif
#endif
	.id_table	= ct36x_ts_id,
	.driver	= {
		.name	= CT36X_TS_NAME,
		.owner	= THIS_MODULE,
	},
	.address_list	= normal_i2c,
};

#if 0
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
	
	switch (cmd) {
	case UPGRADE:
	        dprintk(DEBUG_OTHERS_INFO,"==UPGRADE_WORK=\n");
		fts_ctpm_fw_upgrade_with_i_file();
		// calibrate();
		break;
	default:
		break;			 
	}	
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

static const struct file_operations aw_i2c_ts_fops ={	
	.owner = THIS_MODULE, 		
	.open = aw_open, 	
	.unlocked_ioctl = aw_ioctl,	
	.release = aw_release, 
};
#endif

static int ctp_get_system_config(void)
{   
	ctp_print_info(config_info,DEBUG_INIT);
	
	twi_id = config_info.twi_id;
	screen_max_x = config_info.screen_max_x;
	screen_max_y = config_info.screen_max_y;
	revert_x_flag = config_info.revert_x_flag;
	revert_y_flag = config_info.revert_y_flag;
	exchange_x_y_flag = config_info.exchange_x_y_flag;
	
	if((twi_id == 0) || (screen_max_x == 0) || (screen_max_y == 0))
	{
		printk("%s:read config error!\n",__func__);
		
		return -1;
	}
	
	return 0;
}

static int __init ct36x_ts_init(void)
{ 
	if(m_inet_ctpState == 1)
	{
		printk("m_inet_ctpState return true,just return \n");
		return -ENODEV;
	}
	else
	{
		printk("m_inet_ctpState return false,continue deteck \n");
	}
	
	printk("%s\n", CT36X_IC_INFO);
	
	printk("[FUN]%s\n", __func__);
	
	int ret;

	ret = ctp_get_system_config();
	if(ret != 0)
  	{
		printk("%s:read config fail!\n", __func__);
		
		return ret;
  	}

	ct36x_ts_driver.detect = ct36x_ts_detect;

    return i2c_add_driver(&ct36x_ts_driver);
}

static void __exit ct36x_ts_exit(void)
{
	printk("==ct36x exit==\n");
	
	i2c_del_driver(&ct36x_ts_driver);
}

late_initcall(ct36x_ts_init);
module_exit(ct36x_ts_exit);
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x TouchScreen driver");
MODULE_LICENSE("GPL");


