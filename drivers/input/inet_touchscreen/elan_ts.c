/* include/linux/i2c/elan_ektf2k.h - ELAN EKTF2136 touchscreen driver
 *
 * Copyright (C) 2011 Elan Microelectronics Corporation.
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
 */
#include <linux/i2c.h>
#include <linux/input.h>
#include "elan_ts.h"
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
#include <linux/miscdevice.h>
#include <linux/kthread.h>
#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/gpio.h> 
#include <linux/ctp.h>


#define ELAN_USED     "\n \
												 \n+++++++++++++++++++++++++++++++++ \
                         \n++++++    elan used   +++++++++ \
                         \n+++++++++++++++++++++++++++++++++ \
                         \n"
                               
#define ELAN_IC_INFO  "\n============================================================== \
											   \nIC     :Elan 33XX \
                         \nAUTHOR :mbgalex@163.com \
                         \nVERSION:2014-01-02_16:18\n"
                         
//static int m_inet_ctpState=0;
extern int m_inet_ctpState;


//#define ELAN_ESD_CHECK
#ifdef ELAN_ESD_CHECK
	static int have_interrupts = 0;
	static struct workqueue_struct *esd_wq = NULL;
	static struct delayed_work esd_work;
	static unsigned long delay = 2*HZ;
#endif
/*********************************platform data********************************************/
//must be init first
extern struct ctp_config_info config_info;

#define CTP_IRQ_NUMBER			(config_info.irq_gpio_number)
#define CTP_IRQ_MODE				(TRIG_EDGE_NEGATIVE)
#define SCREEN_MAX_X				(screen_max_x)
#define SCREEN_MAX_Y				(screen_max_y)
#define CTP_NAME					ELAN_TS_NAME	
#define PRESS_MAX				(255)

static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;
static __u32 twi_id = 0;


static const struct i2c_device_id elan_ts_id[] = {
	{CTP_NAME, 0 },
	{ }
};

static const unsigned short normal_i2c[2] = {
	ELAN_7BITS_ADDR,
	I2C_CLIENT_END
};

/**********************************elan struct*********************************************/

struct elan_ts_data{
	struct i2c_client *client;
	struct input_dev *input_dev;
//queue or thread handler interrupt	
	struct task_struct *work_thread;
	struct work_struct  work;
#ifdef CONFIG_HAS_EARLYSUSPEND
//used for early_suspend
	struct early_suspend early_suspend;
#endif
	
//Firmware Information
	int fw_ver;
	int fw_id;
	int fw_bcd;
	int x_resolution;
	int y_resolution;
	int recover;//for iap mod
//for suspend or resum lock
 	int power_lock;
 	int circuit_ver;
//for button state
 	int button_state;
//For Firmare Update 
	struct miscdevice firmware;
};

/************************************global elan data*************************************/
static int tpd_flag = 0;
static struct elan_ts_data *private_ts;
static DECLARE_WAIT_QUEUE_HEAD(waiter);

static void elan_resume_events(struct work_struct *work);
static struct workqueue_struct *elan_resume_wq;
static DECLARE_WORK(elan_resume_work, elan_resume_events);

/*********************************global elan function*************************************/
static int __hello_packet_handler(struct i2c_client *client);
static int __fw_packet_handler(struct i2c_client *client);
static int elan_ts_rough_calibrate(struct i2c_client *client);
static int elan_ts_resume(struct i2c_client *client);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void elan_ts_early_suspend(struct early_suspend *h);
static void elan_ts_late_resume(struct early_suspend *h);
#endif

/************************************** function list**************************************/
static void elan_reset(void)
{
	printk("[elan]:%s enter\n", __func__);
	ctp_wakeup(0,20);
	msleep(20);
}

static void elan_switch_irq(int on)
{
	int elan_irq = CTP_IRQ_NUMBER;
	printk("[elan] %s enter, irq = %d, on = %d\n", __func__, elan_irq, on);
	if(on){
		sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
	}
	else{
		sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
	}
}

static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret;
	char tmp[8] = {0};

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;
	if(twi_id == adapter->nr){
		
		ret = i2c_master_recv(client, tmp, 8);
		
		if (8 != ret) {
			printk("[elan] %s:I2C connection might be something wrong \n", __func__);
			return -ENODEV;
		}else{           
			m_inet_ctpState = 1;			
			printk("%s\n",ELAN_USED);
			strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
			return 0;	
		}
	}else{
		return -ENODEV;
	}
}

static int elan_ts_send_cmd(struct i2c_client *client, uint8_t *cmd, size_t size)
{
	printk("[elan] dump cmd: %02x, %02x, %02x, %02x\n", cmd[0], cmd[1], cmd[2], cmd[3]);
	if (i2c_master_send(client, cmd, size) != size) {
		printk("[elan error]%s: elan_ts_send_cmd failed\n", __func__);
		return -EINVAL;
	}
	else{
		elan_info("[elan] elan_ts_send_cmd ok");
	}
	return size;
}

static int elan_ts_get_data(struct i2c_client *client, uint8_t *cmd, uint8_t *buf, size_t size)
{
	if (buf == NULL)
		return -EINVAL;

	if (elan_ts_send_cmd(client, cmd, size) != size)
		return -EINVAL;

	mdelay(5);
	
	if (i2c_master_recv(client, buf, size) != size ||buf[0] != CMD_S_PKT){
		printk("[elan error]%s: i2c_master_recv failed\n", __func__);
		return -EINVAL;
	}
	printk("[elan] %s: respone packet %2x:%2X:%2x:%2x\n", __func__, buf[0], buf[1], buf[2], buf[3]);
	
	return 0;
}

static int __hello_packet_handler(struct i2c_client *client)
{
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	int rc;
	uint8_t buf_recv[8] = { 0 };
	
	rc = i2c_master_recv(client, buf_recv, sizeof(buf_recv));
	if(rc != sizeof(buf_recv)){
		printk("[elan error] __hello_packet_handler recv error, retry\n");
		rc = i2c_master_recv(client, buf_recv, sizeof(buf_recv));
	}
	printk("[elan] %s: hello packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);
	
	if(buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x80 && buf_recv[3]==0x80){
		printk("[elan] %s: boot code packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[4], buf_recv[5], buf_recv[6], buf_recv[7]);
		ts->recover = 0x80;
	}
	else if(buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x55 && buf_recv[3]==0x55){
		printk("[elan] __hello_packet_handler recv ok, then recv 66 66 66 66\n");
		msleep(400);
		rc = i2c_master_recv(client, buf_recv, 4);
		printk("[elan] %s: RK packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);
		ts->recover = 0;
	}
	else{
		ts->recover = -EINVAL;
	}

	rc = ts->recover;
	return rc;
}

static int __fw_packet_handler(struct i2c_client *client)
{
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	int rc;
	int major, minor;
	uint8_t cmd[]			= {0x53, 0x00, 0x00, 0x01};/* Get Firmware Version*/
	
	uint8_t cmd_id[] 		= {0x53, 0xf0, 0x00, 0x01}; /*Get firmware ID*/
	uint8_t cmd_bc[]		= {0x53, 0x01, 0x00, 0x01};/* Get BootCode Version*/
#ifdef ELAN_3K_XX
	int x, y;
	uint8_t cmd_getinfo[] = {0x5B, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t adcinfo_buf[17]={0};
#else
	uint8_t cmd_x[] 		= {0x53, 0x60, 0x00, 0x00}; /*Get x resolution*/
	uint8_t cmd_y[] 		= {0x53, 0x63, 0x00, 0x00}; /*Get y resolution*/
#endif	
	uint8_t buf_recv[4] 	= {0};
// Firmware version
	rc = elan_ts_get_data(client, cmd, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	ts->fw_ver = major << 8 | minor;
	
// Firmware ID
	rc = elan_ts_get_data(client, cmd_id, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	ts->fw_id = major << 8 | minor;
#ifndef ELAN_3K_XX
	// X Resolution
	rc = elan_ts_get_data(client, cmd_x, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
	ts->x_resolution = minor;

	// Y Resolution	
	rc = elan_ts_get_data(client, cmd_y, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
	ts->y_resolution = minor;
#else
	i2c_master_send(client, cmd_getinfo, sizeof(cmd_getinfo));
	msleep(10);
	i2c_master_recv(client, adcinfo_buf, 17);
	x  = adcinfo_buf[2]+adcinfo_buf[6]+adcinfo_buf[10]+adcinfo_buf[14];
	y  = adcinfo_buf[3]+adcinfo_buf[7]+adcinfo_buf[11]+adcinfo_buf[15];

	printk( "[elan] %s: x= %d, y=%d\n",__func__,x,y);

	ts->x_resolution=(x-1)*64;
	ts->y_resolution=(y-1)*64;
#endif
	
// Firmware BC
	rc = elan_ts_get_data(client, cmd_bc, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	ts->fw_bcd = major << 8 | minor;	
	
	printk( "[elan] %s: firmware version: 0x%4.4x\n",__func__, ts->fw_ver);
	printk( "[elan] %s: firmware ID: 0x%4.4x\n",__func__, ts->fw_id);
	printk( "[elan] %s: firmware BC: 0x%4.4x\n",__func__, ts->fw_bcd);
	printk( "[elan] %s: x resolution: %d, y resolution: %d\n",__func__, ts->x_resolution, ts->y_resolution);
	
	return 0;
}

#if defined IAP_PORTION || defined ELAN_RAM_XX
int WritePage(struct i2c_client *client, uint8_t * szPage, int byte, int which)
{
	int len = 0;
	
#if 1//132 mod
	len = i2c_master_send(client, szPage,  byte);
	if (len != byte) {
		printk("[elan] ERROR: write the %d th page error, write error. len=%d\n", which, len);
		return -1;
	}
#else//8bit mod
	int i = 0;
	for(i=0; i<16; i++){
		len = i2c_master_send(client, (szPage+i*8),  8);
		if (len != 8) {
			printk("[elan] ERROR: write the %d th page error, write error. len=%d\n", which, len);
			return -1;
		}
	}
	
	len = i2c_master_send(client, (szPage+i*8),  4);
	if (len != 4) {
		printk("[elan] ERROR: write the %d th page error, write error. len=%d\n", which, len);
		return -1;
	}
#endif
	
	return 0;
}

/*every page write to recv 2 bytes ack */
int GetAckData(struct i2c_client *client, uint8_t *ack_buf)
{
	int len = 0;
	
	len=i2c_master_recv(client, ack_buf, 2);
	if (len != 2) {
		printk("[elan] ERROR: GetAckData. len=%d\r\n", len);
		return -1;
	}
	
	if (ack_buf[0] == 0xaa && ack_buf[1] == 0xaa) {
		return ACK_OK;
	}
	else if (ack_buf[0] == 0x55 && ack_buf[1] == 0x55){
		return ACK_REWRITE;
	}
	else{
		return ACK_Fail;
	}
	return 0;
}

/* Check Master & Slave is "55 aa 33 cc" */
int CheckIapMode(struct i2c_client *client)
{
	char buff[4] = {0};
	int rc = 0;
	
	rc = i2c_master_recv(client, buff, 4);
	if (rc != 4) {
		printk("[elan] ERROR: CheckIapMode. len=%d\r\n", rc);
		return -1;
	}
	else
		printk("[elan] Mode= 0x%x 0x%x 0x%x 0x%x\r\n", buff[0], buff[1], buff[2], buff[3]);
		
	return 0;	
}

void update_fw_one(struct i2c_client *client)
{
	uint8_t ack_buf[2] = {0};
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	
	int res = 0;
	int iPage = 0;
	
	uint8_t data;

	const int PageSize = 132;
	const int PageNum = sizeof(file_fw_data)/PageSize;
	
	const int PAGERETRY = 10;
	const int IAPRESTART = 3;
	
	int restartCnt = 0; // For IAP_RESTART
	int rewriteCnt = 0;// For IAP_REWRITE
	
	int iap_mod;
	
	uint8_t *szBuff = NULL;
	int curIndex = 0;

#ifdef ELAN_2K_XX	
	uint8_t isp_cmd[] = {0x54, 0x00, 0x12, 0x34};	 //54 00 12 34
	iap_mod = 2;
#endif

#ifdef ELAN_3K_XX 
	uint8_t isp_cmd[] = {0x45, 0x49, 0x41, 0x50};	 //45 49 41 50
	iap_mod = 3;	
#endif

#ifdef ELAN_RAM_XX 
	uint8_t isp_cmd[] = {0x22, 0x22, 0x22, 0x22};	 //22 22 22 22
	iap_mod = 1;	
#endif

	elan_switch_irq(0);
	ts->power_lock = 1;
	
	data=ELAN_7BITS_ADDR;
	printk( "[elan] %s: address data=0x%x iap_mod=%d PageNum = %d\r\n", __func__, data, iap_mod, PageNum);
	
IAP_RESTART:
	//reset tp
	if(iap_mod == 3){
		elan_reset();
	}
	
	if((iap_mod!=2) || (ts->recover != 0x80)){
		printk("[elan] Firmware update normal mode !\n");
		//Step 1 enter isp mod
		res = elan_ts_send_cmd(client, isp_cmd, sizeof(isp_cmd));
		//Step 2 Chech IC's status is 55 aa 33 cc
		if(iap_mod == 2){
			res = CheckIapMode(client);
		}
	} else{
		printk("[elan] Firmware update recovery mode !\n");	
	}
	
	//Step 3 Send Dummy Byte
	res = i2c_master_send(client, &data,  sizeof(data));
	if(res!=sizeof(data)){
		printk("[elan] dummy error code = %d\n",res);
		return;
	}
	else
		printk("[elan] send Dummy byte sucess data:%x", data);
	
	msleep(10);
	

	//Step 4 Start IAP
	for( iPage = 1; iPage <= PageNum; iPage++ ) {
			
		szBuff = file_fw_data + curIndex;
		curIndex =  curIndex + PageSize;

PAGE_REWRITE:
		res = WritePage(client, szBuff, PageSize, iPage);
		
		if(iPage==PageNum || iPage==1){
			msleep(300); 			 
		}
		else{
			msleep(50); 			 
		}
		
		res = GetAckData(client, ack_buf);
		if (ACK_OK != res) {
			
			msleep(50); 
			printk("[elan]: %d page ack error: ack0:%x ack1:%x\n",  iPage, ack_buf[0], ack_buf[1]);
			
			if ( res == ACK_REWRITE ){
				rewriteCnt = rewriteCnt + 1;
				if (rewriteCnt != PAGERETRY){
					printk("[elan] ---%d--- page ReWrite %d times!\n",  iPage, rewriteCnt);
					return;
				}
				else{
					printk("[elan] ---%d--- page ReWrite %d times! failed\n",  iPage, rewriteCnt);
					goto PAGE_REWRITE;
				}
			}
			else
			{
				restartCnt = restartCnt + 1;
				if (restartCnt != IAPRESTART){
					printk("[elan] try to ReStart %d times !\n", restartCnt);
					return;
				}
				else{
					printk("[elan] ReStart %d times fails!\n", restartCnt);
					curIndex = 0;
					goto IAP_RESTART;
				}
			}
		}
		else{
			elan_info("---%d--- page flash ok", iPage);
			rewriteCnt=0;
		}
	}

	elan_reset();
	elan_switch_irq(1);
	ts->power_lock = 0;
	
	printk("[elan] Update ALL Firmware successfully!\n");
	
	return;
}
#endif
static inline int elan_ts_parse_xy(uint8_t *data, uint16_t *x, uint16_t *y)
{
	*x = *y = 0;

	*x = (data[0] & 0xf0);
	*x <<= 4;
	*x |= data[1];

	*y = (data[0] & 0x0f);
	*y <<= 8;
	*y |= data[2];

	return 0;
}
static int elan_ts_setup(struct i2c_client *client)
{
	int rc;

	elan_reset();
	msleep(500);
	
	rc = __hello_packet_handler(client);
	if (rc < 0){
		printk("[elan error] %s, hello_packet_handler fail, rc = %d\n", __func__, rc);
		return rc;
	}
	
	// for firmware update
	if(rc == 0x80){
		printk("[elan error] %s, fw had bening miss, rc = %d\n", __func__, rc);
		return rc;
	}
	
	mdelay(10);
	
	rc = __fw_packet_handler(client);
	if (rc < 0){
		printk("[elan error] %s, fw_packet_handler fail, rc = %d\n", __func__, rc);
	}
	
	return rc;
}

static int elan_ts_rough_calibrate(struct i2c_client *client)
{
	uint8_t cmd[] = {CMD_W_PKT, 0x29, 0x00, 0x01};
	int size = sizeof(cmd);

	if (elan_ts_send_cmd(client, cmd, size) != size)
		return -EINVAL;

	return 0;
}

static int elan_ts_set_power_state(struct i2c_client *client, int state)
{
	uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};
	int size = sizeof(cmd);
	
	cmd[1] |= (state << 3);
	if (elan_ts_send_cmd(client, cmd, size) != size)
		return -EINVAL;

	return 0;
}

static void elan_ts_touch_down(struct elan_ts_data* ts,s32 id,s32 x,s32 y,s32 w)
{
	x = ts->x_resolution-x;
	//y = ts->y_resolution-y;

	x = x * screen_max_x/ts->x_resolution;
	y = y * screen_max_y/ts->y_resolution;
	
#ifdef ELAN_ICS_SLOT_REPORT
	input_mt_slot(ts->input_dev, id);
	input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
	input_report_abs(ts->input_dev, ABS_MT_PRESSURE, w);
#else
	input_report_key(ts->input_dev, BTN_TOUCH, 1);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
	input_report_abs(ts->input_dev, ABS_MT_PRESSURE, w);
	input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
	input_mt_sync(ts->input_dev);
#endif

	elan_info("Touch ID:%d, X:%d, Y:%d, W:%d down", id, x, y, w);
}

static void elan_ts_touch_up(struct elan_ts_data* ts,s32 id,s32 x,s32 y)
{
#ifdef ELAN_ICS_SLOT_REPORT
	input_mt_slot(ts->input_dev, id);
	input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
	elan_info("Touch id[%2d] release!", id);
#else
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
	input_mt_sync(ts->input_dev);
	elan_info("Touch all release!");
#endif
}
static void elan_ts_report_key(struct elan_ts_data *ts, uint8_t button_data)
{	
	static unsigned int key_value = 0;
	static unsigned int x = 0, y = 0;

	switch (button_data) {
		case ELAN_KEY_BACK:
			key_value = KEY_BACK;
			input_report_key(ts->input_dev, key_value, 1);
			break;
		case ELAN_KEY_HOME:
			key_value = KEY_HOME;
			input_report_key(ts->input_dev, key_value, 1);
			break;
		case ELAN_KEY_MENU:
			key_value = KEY_MENU;
			input_report_key(ts->input_dev, key_value, 1);
			break;
		default:
			if(key_value != 0){
				input_report_key(ts->input_dev, key_value, 0);
				key_value = 0;
			}
			else{
				elan_ts_touch_up(ts, 0, x, y);
			}	
			break;
	}
}


#if defined ELAN_RAM_XX
static void elan_ts_iap_ram_continue(struct i2c_client *client)
{
	uint8_t cmd[] = { 0x33, 0x33, 0x33, 0x33 };
	int size = sizeof(cmd);

	elan_ts_send_cmd(client, cmd, size);
}
#endif

static void elan_ts_handler_event(struct elan_ts_data *ts, uint8_t *buf, int len)
{
	if(buf[0] == 0x55){
		if(buf[2] == 0x55){
			ts->recover = 0;
			if(ts->x_resolution == 0 || ts->y_resolution == 0){
				elan_switch_irq(0);
				__fw_packet_handler(ts->client);
				elan_switch_irq(1);
			}
		}
		else if(buf[2] == 0x80){
			ts->recover = 0x80;
		#if defined IAP_PORTION
			update_fw_one (ts->client);
		#endif
		}
	}
}

#ifdef ELAN_IAP_DEV
int elan_iap_open(struct inode *inode, struct file *filp)
{ 
	elan_info("%s enter", __func__);
	if (private_ts == NULL)
		printk("private_ts is NULL~~~");
	return 0;
}

int elan_iap_release(struct inode *inode, struct file *filp)
{    
	elan_info("%s enter", __func__);
	return 0;
}

static ssize_t elan_iap_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{  
	int ret;
	char *tmp;
	struct i2c_client *client = private_ts->client;
	
	elan_info("%s enter", __func__);
	if (count > 8192)
		count = 8192;

	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;
	
	if (copy_from_user(tmp, buff, count)) {
		return -EFAULT;
	}
	
	ret = i2c_master_send(client, tmp, count);
	if (ret != count) 
		printk("elan i2c_master_send fail, ret=%d \n", ret);
	
	kfree(tmp);
	
	return ret;
}

ssize_t elan_iap_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{    
	char *tmp;
	int ret;  
	long rc;
	
	struct i2c_client *client = private_ts->client;
	
	elan_info("%s enter", __func__);
	
	if (count > 8192)
		count = 8192;
	
	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;
	
	ret = i2c_master_recv(client, tmp, count);
	if (ret != count) 
		printk("elan i2c_master_recv fail, ret=%d \n", ret);
	
	if (ret == count)
		rc = copy_to_user(buff, tmp, count);
	
	kfree(tmp);
	return ret;
}

static long elan_iap_ioctl( struct file *filp, unsigned int cmd, unsigned long arg)
{
	int __user *ip = (int __user *)arg;
	
	elan_info("%s enter", __func__);
	elan_info("cmd value %x\n",cmd);

	switch (cmd) {        
		case IOCTL_I2C_SLAVE:
			printk("[elan debug] pre addr is %X\n",  private_ts->client->addr); 
			private_ts->client->addr = (int __user)arg;
			printk("[elan debug] new addr is %X\n",  private_ts->client->addr); 
			return 0;   
		case IOCTL_MAJOR_FW_VER:            
			break;        
		case IOCTL_MINOR_FW_VER:            
			break;        
		case IOCTL_RESET:
			elan_reset();
			break;
		case IOCTL_IAP_MODE_LOCK:
			if(private_ts->power_lock==0){
				private_ts->power_lock=1;
				elan_switch_irq(0);
			}
			break;
		case IOCTL_IAP_MODE_UNLOCK:
			if(private_ts->power_lock==1){			
				private_ts->power_lock=0;
				elan_switch_irq(1);
			}
			break;
		case IOCTL_CHECK_RECOVERY_MODE:
			return private_ts->recover;;
			break;
		case IOCTL_FW_VER:
			__fw_packet_handler(private_ts->client);
			return private_ts->fw_ver;;
			break;
		case IOCTL_X_RESOLUTION:
			__fw_packet_handler(private_ts->client);
			return private_ts->x_resolution;
			break;
		case IOCTL_Y_RESOLUTION:
			__fw_packet_handler(private_ts->client);
			return private_ts->y_resolution;
			break;
		case IOCTL_FW_ID:
			__fw_packet_handler(private_ts->client);
			return private_ts->fw_id;
			break;
		case IOCTL_ROUGH_CALIBRATE:
			return elan_ts_rough_calibrate(private_ts->client);
		case IOCTL_I2C_INT:
			put_user( tpd_flag, ip);
			break;	
		case IOCTL_RESUME:
			break;
		case IOCTL_POWER_LOCK:
			private_ts->power_lock=1;
			break;
		case IOCTL_POWER_UNLOCK:
			private_ts->power_lock=0;
			break;
		default:            
			break;   
	}       
	return 0;
}

struct file_operations elan_touch_fops = {    
        .open =         	elan_iap_open,    
        .write =        	elan_iap_write,    
        .read =			elan_iap_read,    
        .release =		elan_iap_release,    
	.unlocked_ioctl=	elan_iap_ioctl, 
 };

#endif
#ifdef ELAN_ESD_CHECK
static void elan_touch_esd_func(struct work_struct *work)
{	
	int res;	
	uint8_t cmd[] = {0x53, 0x00, 0x00, 0x01};	
	struct i2c_client *client = private_ts->client;	

	elan_info("esd %s: enter.......", __FUNCTION__);
	
	if(private_ts->power_lock == 1)
		goto out_esd;
	if(have_interrupts == 1){
		elan_info("esd %s: had interrup not need check", __func__);
	}
	else{
		res = elan_ts_send_cmd(client, cmd, sizeof(cmd));
		if (res != sizeof(cmd)){
			printk("[elan esd] %s: i2c_master_send failed reset now", __func__);
			//reset here
			elan_reset();
		}
		else{
			elan_info(" esd %s: i2c_master_send successful", __func__);
			msleep(20);
			if(have_interrupts == 1){
				elan_info("esd %s: i2c_master_send successful, had response", __func__);
			}
			else{
				printk("[elan esd] %s: i2c_master_send successful, no response need reset", __func__);
				elan_reset();
			}
		}
	}

out_esd:	
	have_interrupts = 0;	
	queue_delayed_work(esd_wq, &esd_work, delay);
	elan_info("[elan esd] %s: out.......", __FUNCTION__);
}
#endif	


#ifdef SYS_ATTR_FILE
static ssize_t elan_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
#ifdef PRINT_INT_INFO 	
	debug_flage = !debug_flage;
	if(debug_flage)
		printk("elan debug switch open\n");
	else
		printk("elan debug switch close\n");
#endif	
	return ret;
}
static DEVICE_ATTR(debug, S_IRUGO, elan_debug_show, NULL);


static ssize_t elan_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct elan_ts_data *ts = private_ts;
	elan_switch_irq(0);
	__fw_packet_handler(ts->client);
	elan_switch_irq(1);
	sprintf(buf, "elan fw ver:%X,id:%X,x:%d,y:%d\n", ts->fw_ver, ts->fw_id, ts->x_resolution, ts->y_resolution);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(info, S_IRUGO, elan_info_show, NULL);


static ssize_t elan_rk_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	ret = elan_ts_rough_calibrate(private_ts->client);
	
	return ret;
}
static DEVICE_ATTR(rk, S_IRUGO, elan_rk_show, NULL);

static ssize_t set_cmd_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	char cmd[4] = {0};
	if (size > 4)
		return -EINVAL;
	
	if (sscanf(buf, "%02x:%02x:%02x:%02x\n", (int *)&cmd[0], (int *)&cmd[1], (int *)&cmd[2], (int *)&cmd[3]) != 4){
		printk("elan cmd format error\n");
		return -EINVAL;
	}
	elan_ts_send_cmd(private_ts->client, cmd, 4);
	return size;
}
static DEVICE_ATTR(set_cmd, S_IRUGO, NULL, set_cmd_store);

static struct attribute *sysfs_attrs_ctrl[] = {
	&dev_attr_debug.attr,
	&dev_attr_info.attr,
	&dev_attr_rk.attr,
	&dev_attr_set_cmd.attr,
	NULL
};
static struct attribute_group elan_attribute_group[] = {
	{.attrs = sysfs_attrs_ctrl },
};
#endif

static void elan_touch_node_init(void)
{
	int ret ;
	struct elan_ts_data *ts = private_ts;
#ifdef SYS_ATTR_FILE		
	android_touch_kobj = kobject_create_and_add("android_touch", NULL) ;
	if (android_touch_kobj == NULL) {
		printk(KERN_ERR "[elan]%s: kobject_create_and_add failed\n", __func__);
		return;
	}
	
	ret = sysfs_create_group(android_touch_kobj, elan_attribute_group);
	if (ret < 0) {
		printk(KERN_ERR "[elan]%s: sysfs_create_group failed\n", __func__);
	}
#endif
	
#ifdef ELAN_IAP_DEV	
	ts->firmware.minor = MISC_DYNAMIC_MINOR;
	ts->firmware.name = "elan-iap";
	ts->firmware.fops = &elan_touch_fops;
	ts->firmware.mode = S_IFREG|S_IRWXUGO; 

	if (misc_register(&ts->firmware) < 0)
		printk("[elan debug] misc_register failed!!\n");
	else
		printk("[elan debug] misc_register ok!!\n");
#endif
	return;
}

static void elan_touch_node_deinit(void)
{
#ifdef SYS_ATTR_FILE
	sysfs_remove_group(android_touch_kobj, elan_attribute_group);
	kobject_del(android_touch_kobj);
#endif	
}

static int elan_ts_recv_data(struct elan_ts_data *ts, uint8_t *buf)
{
	int rc;
	rc = i2c_master_recv(ts->client, buf, PACKET_SIZE);
	if(PACKET_SIZE != rc){
		printk("[elan error] elan_ts_recv_data\n");
		return -1;
	}
#ifdef PRINT_INT_INFO
	elan_info("%x %x %x %x %x %x %x %x", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
	if(PACKET_SIZE >= 18)
		elan_info("%x %x %x %x %x %x %x %x", buf[8],buf[9],buf[10],buf[11],buf[12],buf[13],buf[14],buf[15]);
	if(PACKET_SIZE >= 34){
		elan_info("%x %x %x %x %x %x %x %x", buf[16],buf[17],buf[18],buf[19],buf[20],buf[21],buf[22],buf[23]);
		elan_info("%x %x %x %x %x %x %x %x", buf[24],buf[25],buf[26],buf[27],buf[28],buf[29],buf[30],buf[31]);
	#ifndef ELAN_BUFFER_MODE
		elan_info("%x %x %x", buf[32],buf[33], buf[34]);
	#else
		elan_info("%x %x %x %x %x %x %x %x", buf[32],buf[33],buf[34],buf[35],buf[36],buf[37],buf[38],buf[39]);
		elan_info("%x %x %x %x %x", buf[40],buf[41],buf[42],buf[43],buf[44]);
	#endif	
	}
	else if(PACKET_SIZE == 18)
		elan_info("%x %x", buf[16],buf[17]);
#endif

	if(FINGERS_PKT != buf[0]){
		printk("[elan] not normal packet:%x %x %x %x %x %x %x %x\n", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
		elan_ts_handler_event(ts, buf, PACKET_SIZE);
		return -1;
	}
	
	return 0;
}

static void elan_ts_report_data(struct elan_ts_data *ts, uint8_t *buf)
{
	uint16_t fbits=0;
#ifdef ELAN_ICS_SLOT_REPORT	
	static uint16_t pre_fbits=0;
	uint16_t fbits_tmp=0;
#else
	int reported = 0;
#endif
	uint8_t idx;
	int finger_num;
	int num = 0;
	uint16_t x = 0;
	uint16_t y = 0;
	int position = 0;
	uint8_t button_byte = 0;

	finger_num = FINGERS_NUM;	
#ifdef TWO_FINGERS
	num = buf[7] & 0x03; 
	fbits = buf[7] & 0x03;
	idx=1;
	button_byte = buf[PACKET_SIZE-1];
#endif
	
#ifdef FIVE_FINGERS
	num = buf[1] & 0x07; 
	fbits = buf[1] >>3;
	idx=2;
	button_byte = buf[PACKET_SIZE-1];
#endif
	
#ifdef TEN_FINGERS
	fbits = buf[2] & 0x30;	
	fbits = (fbits << 4) | buf[1];  
	num = buf[2] &0x0f;
	idx=3;
	button_byte = buf[PACKET_SIZE-1];
#endif

#ifdef ELAN_ICS_SLOT_REPORT
	fbits_tmp = fbits;
	if(fbits || pre_fbits){
		for(position=0; position<finger_num;position++){
			if(fbits&0x01){
				elan_ts_parse_xy(&buf[idx], &x, &y); 
				elan_ts_touch_down(ts, position, x, y, 8);
			}
			else if(pre_fbits&0x01){
				elan_ts_touch_up(ts, position, x, y);
			}
			fbits >>= 1;
			pre_fbits >>= 1;
			idx += 3;	
		}
	}	
	else{
		elan_ts_report_key(ts, button_byte);
	}	
	pre_fbits = fbits_tmp;
#else
	if (num == 0){
		elan_ts_report_key(ts, button_byte);
	} 
	else{
		elan_info( "[elan] %d fingers", num);
				
		for(position=0; (position<finger_num) && (reported < num);position++){
			if((fbits & 0x01)){
				elan_ts_parse_xy(&buf[idx],&x, &y);
				elan_ts_touch_down(ts, position, x, y, 8);
				reported++;
			}
			fbits = fbits >> 1;
			idx += 3;
		}
	}
#endif
	
	input_sync(ts->input_dev);
	return;
}

static irqreturn_t elan_ts_irq_handler(int irq, void *dev_id)
{
	elan_info("----------elan_ts_irq_handler----------");
#ifdef ELAN_ESD_CHECK
	have_interrupts = 1;
#endif
	tpd_flag = 1;
	wake_up_interruptible(&waiter);

	return 0;
}

static int elan_ts_register_interrupt(struct elan_ts_data *ts )
{
	int elan_irq = CTP_IRQ_NUMBER;
	
	int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)elan_ts_irq_handler,ts);
	if (int_handle == 0){
		printk("[elan error] %s: request_irq %d failed\n",__func__, elan_irq);
	}
	
	ctp_set_int_port_rate(1);
	ctp_set_int_port_deb(0x07);
	printk("[elan]:reg clk: 0x%08x\n", readl(0xf1c20a18));
	return int_handle;
}

#if defined IAP_PORTION
static int check_update_flage(struct elan_ts_data *ts)
{
	int NEW_FW_VERSION = 0;
	int New_FW_ID = 0;

#ifdef ELAN_2K_XX
	New_FW_ID = file_fw_data[0x7DB3]<<8  | file_fw_data[0x7DB2] ;        
	NEW_FW_VERSION = file_fw_data[0x7DB1]<<8  | file_fw_data[0x7DB0] ;
#endif

#ifdef ELAN_3K_XX
	New_FW_ID  = file_fw_data[0x7D67]<<8  | file_fw_data[0x7D66] ;
	NEW_FW_VERSION = file_fw_data[0x7D65]<<8  | file_fw_data[0x7D64] ;
	
	//New_FW_ID = file_fw_data[size - 2*PageSize + 120] | (file_fw_data[size - 2*PageSize + 121] << 8); 
	//NEW_FW_VERSION = file_fw_data[size - 2*PageSize + 122] | (file_fw_data[size - 2*PageSize + 123] << 8);

#endif
	
	printk("FW_ID=0x%x, New_FW_ID=0x%x \n",ts->fw_id, New_FW_ID);
	printk("FW_VERSION=0x%x,New_FW_VER=0x%x \n",ts->fw_ver,NEW_FW_VERSION);

	if((ts->fw_id&0xff) != (New_FW_ID&0xff)){
		printk("[elan] fw id is different, can not update !\n");
		return 0;
	}
	else{
		printk("[elan] fw id is same !\n");
	}
	
	if((ts->fw_ver&0xff) < (NEW_FW_VERSION&0xff)){
		return 1;
	}
	else{
		printk("[elan] fw version is same !\n");
	}
	return 0;
}
#endif

static int touch_event_handler(void *unused)
{
	uint8_t buf[PACKET_SIZE] = {0};
#ifdef  ELAN_BUFFER_MODE
	uint8_t head[4] = {0};
	int cnt = 0;
#endif	
	int rc = 0;
	struct elan_ts_data *ts = private_ts;
	struct sched_param param = { .sched_priority = 4};
	sched_setscheduler(current, SCHED_RR, &param);

#if defined IAP_PORTION
	rc = check_update_flage(ts);
	msleep(2000);
	if(rc == 1){
		update_fw_one(ts->client);
	}
#endif	
	
	do{
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, tpd_flag != 0);
		tpd_flag = 0;
		set_current_state(TASK_RUNNING);
#ifdef  ELAN_BUFFER_MODE
		rc = i2c_master_recv(ts->client, head, 4);
		if(4 != rc){
			printk("[elan error] elan_ts_recv_data\n");
			continue;
		}
		if(head[0] == BUFF_PKT){
			cnt = head[1] & 0x03;
			while(cnt--){
				rc = elan_ts_recv_data(ts, buf);
				if(rc < 0){
					continue;
				}
				elan_ts_report_data(ts, buf);
			}
		}
		else{
			printk("[elan] %02x %02x %02x %02x\n", head[0], head[1], head[2], head[3]);
		}
#else
		rc = elan_ts_recv_data(ts, buf);
		if(rc < 0){
			continue;
		}
		elan_ts_report_data(ts, buf);
#endif		

	}while(!kthread_should_stop());

    return 0;
}

static int elan_request_input_dev(struct elan_ts_data *ts)
{
	int err = 0;
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		err = -ENOMEM;
		printk("[elan error] Failed to allocate input device\n");
		return err;
	}
	
	ts->input_dev->evbit[0] = BIT(EV_KEY)|BIT_MASK(EV_REP);

#ifdef TPD_HAVE_BUTTON
	for (int i = 0; i < ARRAY_SIZE(button); i++){
		set_bit(button[i] & KEY_MAX, ts->input_dev->keybit);
	}
#endif	

	ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
	ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
#ifdef ELAN_ICS_SLOT_REPORT
	__set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
	input_mt_init_slots(ts->input_dev, FINGERS_NUM);
#else
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif
	
	printk( "[elan] %s: x resolution: %d, y resolution: %d\n",__func__, ts->x_resolution, ts->y_resolution);
	//input_set_abs_params(ts->input_dev,ABS_MT_POSITION_X,  0, ts->x_resolution, 0, 0);
	//input_set_abs_params(ts->input_dev,ABS_MT_POSITION_Y,  0, ts->y_resolution, 0, 0);

	input_set_abs_params(ts->input_dev,ABS_MT_POSITION_X,  0, screen_max_x, 0, 0);
	input_set_abs_params(ts->input_dev,ABS_MT_POSITION_Y,  0, screen_max_y, 0, 0);
	
	
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, PRESS_MAX, 0, 0);	 
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 255, 0, 0);

	ts->input_dev->name = CTP_NAME;
	ts->input_dev->phys = "input/ts"; 
	ts->input_dev->id.bustype = BUS_I2C; 
	ts->input_dev->id.vendor = 0xDEAD; 
	ts->input_dev->id.product = 0xBEEF; 
	ts->input_dev->id.version = 2013;

	err = input_register_device(ts->input_dev);
	if (err) {
		input_free_device(ts->input_dev);
		printk("[elan error]%s: unable to register %s input device\n", __func__, ts->input_dev->name);
		return err;
	}
	return 0;
}

static int elan_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	long retval;
	struct elan_ts_data *ts;
		
	printk("[elan] %s enter i2c addr %x\n", __func__, client->addr);
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("[elan error] %s: i2c check functionality error\n", __func__);
		return -ENODEV;
	}
	
	ts = kzalloc(sizeof(struct elan_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		printk("[elan error] %s: allocate elan_ts_data failed\n", __func__);
		return ENOMEM;
	}
	
	ts->client = client;
	i2c_set_clientdata(client, ts);
	private_ts = ts;
	
	elan_resume_wq = create_singlethread_workqueue("elan_resume");
	if (elan_resume_wq == NULL) {
		printk("create elan_resume_wq fail!\n");
		return -ENOMEM;
	}
	
	ret = elan_request_input_dev(ts);
	if (ret < 0) {
		printk("[elan error]: %s elan_request_input_dev\n", __func__);
		return -EINVAL;
	}
	
	ret = elan_ts_register_interrupt(ts);
	if (ret == 0) {
		printk("[elan error]: %s elan_ts_register_interrupt\n", __func__);
		return -EINVAL;
	}
	
	ts->work_thread = kthread_run(touch_event_handler, 0, CTP_NAME);
	if(IS_ERR(ts->work_thread)) {
		retval = PTR_ERR(ts->work_thread);
		printk("[elan error] failed to create kernel thread: %ld\n", retval);
		return -EINVAL;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1;
	ts->early_suspend.suspend = elan_ts_early_suspend;
	ts->early_suspend.resume = elan_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
#ifdef ELAN_ESD_CHECK
	INIT_DELAYED_WORK(&esd_work, elan_touch_esd_func);
	esd_wq = create_singlethread_workqueue("esd_wq");	
	if (!esd_wq) {
		retval = -ENOMEM;
	}
	queue_delayed_work(esd_wq, &esd_work, delay);
#endif
	
	elan_touch_node_init();
	printk("[elan]+++++++++end porbe+++++++++!\n");
	elan_reset();
	
	return 0;
}

static int elan_ts_remove(struct i2c_client *client)
{
	struct elan_ts_data *ts = i2c_get_clientdata(client);

#ifdef SYS_ATTR_FILE
	elan_touch_node_deinit();
#endif
	
	destroy_workqueue(elan_resume_wq);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
	sw_gpio_irq_free(int_handle);
	
	input_unregister_device(ts->input_dev);
	kfree(ts);

	return 0;
}

static int elan_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct elan_ts_data *ts = private_ts;
	int rc = 0;

	printk( "[elan] %s: enter\n", __func__);
	if(ts->power_lock==0){
		elan_switch_irq(0);
		rc = elan_ts_set_power_state(ts->client, PWR_STATE_DEEP_SLEEP);
	}
#ifdef ELAN_ESD_CHECK	
	cancel_delayed_work_sync(&esd_work);
#endif	
	return rc;
}

static void elan_resume_events (struct work_struct *work)
{
	struct elan_ts_data *ts = private_ts;
	int rc = 0;

	printk("[elan] %s: enter\n", __func__);
	if(ts->power_lock==0){		
		rc = elan_ts_set_power_state(ts->client, PWR_STATE_NORMAL);
		if(rc < 0){
			printk("[elan error] %s: can not send cmd to resum, reset now\n", __func__);
			elan_reset();
		}
		else{
			printk("[elan error] %s: send cmd to resum ok\n", __func__);
		}
		elan_switch_irq(1);
	}
#ifdef ELAN_ESD_CHECK
	queue_delayed_work(esd_wq, &esd_work, delay);	
#endif
	return;
	
}

static int elan_ts_resume(struct i2c_client *client)
{
	queue_work(elan_resume_wq, &elan_resume_work);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void elan_ts_early_suspend(struct early_suspend *h)
{
	struct elan_ts_data *ts =  private_ts;
	elan_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void elan_ts_late_resume(struct early_suspend *h)
{
	struct elan_ts_data *ts =  private_ts;
	elan_ts_resume(ts->client);
}
#endif

static struct i2c_driver elan_ts_driver = {
	.class 		= I2C_CLASS_HWMON,
	.probe		= elan_ts_probe,
	.remove		= elan_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend		= elan_ts_suspend,
	.resume		= elan_ts_resume,
#endif
	.id_table		= elan_ts_id,
	.driver		= {
		.name   		= CTP_NAME,
		.owner  		= THIS_MODULE,
	},
	.address_list   = normal_i2c,
};

static int ctp_get_system_config(void)
{
	twi_id = config_info.twi_id;
	screen_max_x = config_info.screen_max_x;
	screen_max_y = config_info.screen_max_y;
	revert_x_flag = config_info.revert_x_flag;
	revert_y_flag = config_info.revert_y_flag;
	exchange_x_y_flag = config_info.exchange_x_y_flag;

	printk("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	printk("mbg ++ screen_max_x = %d,screen_max_y = %d\n",screen_max_x,screen_max_y);

	printk("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	
	if((twi_id == 0) || (screen_max_x == 0) || (screen_max_y == 0)){
		printk("%s:read config error!\n",__func__);
		return 0;
	}
	return 1;
}

static int __init elan_ts_init(void)
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
	
	printk("%s\n",ELAN_IC_INFO);
	
	printk("[FUN]%s\n", __func__);
	printk("[elan] %s driver 004 version : auto-mapping resolution\n", __func__);	
	
	if(config_info.ctp_used == 0){
	        printk("[elan error]:ctp_used set to 0 !\n");
	        printk("[elan error]:if use ctp,please put the sys_config.fex ctp_used set to 1. \n");
	        return 0;
	}
		
        if(!ctp_get_system_config()){
                printk("[elan error]:%s:read config fail!\n",__func__);
                return ret;
        }

	elan_reset();
	msleep(300);
	
	elan_ts_driver.detect = ctp_detect;

	if(m_inet_ctpState !=1)
	{
		return -1;
	}

	ret = i2c_add_driver(&elan_ts_driver);
	printk("[elan]: %s add do i2c_add_driver and the return value=%d\n",__func__,ret);
	return ret;
}

static void __exit elan_ts_exit(void)
{
	printk("[elan]: %s remove driver\n", __func__);
	i2c_del_driver(&elan_ts_driver);
	return;
}

module_init(elan_ts_init);
module_exit(elan_ts_exit);

MODULE_DESCRIPTION("elan Touchscreen Driver");
MODULE_LICENSE("GPL");
