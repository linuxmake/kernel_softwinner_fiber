/* drivers/input/touchscreen/gt9xx.c
 * 
 * 2010 - 2012 Goodix Technology.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the GOODiX's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 * Version:1.2
 * Author:andrew@goodix.com
 * Release Date:2012/10/15
 * Revision record:
 *      V1.0:2012/08/31,first Release
 *      V1.2:2012/10/15,modify gtp_reset_guitar,slot report,tracking_id & 0x0F

 	mbgalex@163.com 2013-06-06_15:37
 	add gt9xx used tag
 *
 */
#include <linux/regulator/consumer.h>
#include <linux/irq.h>
#include "gt9xx.h"
#include "gt9xx_config.h"


#if GTP_ICS_SLOT_REPORT
    #include <linux/input/mt.h>
#endif

static int gt9xx_cob_number =  0;
static int Q978_index = 0;


static const char *goodix_ts_name = "gt9xx";
static struct workqueue_struct *goodix_wq;
struct i2c_client * i2c_connect_client = NULL; 
static u8 config[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH]
                = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};

#if GTP_HAVE_TOUCH_KEY
	static const u16 touch_key_array[] = GTP_KEY_TAB;
	#define GTP_MAX_KEY_NUM	 (sizeof(touch_key_array)/sizeof(touch_key_array[0]))
#endif

static s8 gtp_i2c_test(struct i2c_client *client);
void gtp_reset_guitar(s32 ms);
void gtp_int_sync(s32 ms);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif
 
#if GTP_CREATE_WR_NODE
extern s32 init_wr_node(struct i2c_client*);
extern void uninit_wr_node(void);
#endif

#if GTP_AUTO_UPDATE
extern u8 gup_init_update_proc(struct goodix_ts_data *);
#endif

#if GTP_ESD_PROTECT
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct * gtp_esd_check_workqueue = NULL;
static void gtp_esd_check_func(struct work_struct *);
#endif

#define GT9XX_I2C_ADDR 0x14

#define GT9XX_USED     "\n \
												 \n+++++++++++++++++++++++++++++++++ \
                         \n++++++      GT9XX  used +++++++++ \
                         \n+++++++++++++++++++++++++++++++++ \
                         \n"
                            
#define GT9XX_IC_INFO  "\n============================================================== \
											   \nIC     :GT9XX \
                         \nAUTHOR :Gavin\n" 
                         
#define CTP_NAME GTP_I2C_NAME

static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;


static __u32 twi_id = 0;

#define TP_LIGHT
#ifdef TP_LIGHT
//20140307 for tp light
static struct tp_light{
	struct gpio_config  gpio;
	struct timer_list timer;
	unsigned int timeout;	
}inet_tp_light;

static int have_light = 0;
#endif
static const unsigned short normal_i2c[2] = {GT9XX_I2C_ADDR,I2C_CLIENT_END};
static const uint8_t chip_id_value[] = {0x11,0x27,0x28,0};
static uint8_t read_chip_value[3] = {0x81,0x41,0};

  
                         
extern int m_inet_ctpState;

//#define CTP_IRQ_MODE		(TRIG_EDGE_NEGATIVE)

//#define GTP_MAX_HEIGHT   screen_max_x
//#define GTP_MAX_WIDTH    screen_max_y
/****************************************** 
**int2io:                                 
**status:                                 
**      1:io output 0:io input            
**level:                                  
**      1:output high 0:output low        
******************************************/
void int2io(int status,int level)
{
    int req_success;

    req_success = (0 == gpio_request(config_info.irq_gpio_number, NULL));
    if(status == 1)
    {
        gpio_direction_output(config_info.irq_gpio_number, level);
    }
    else if(status == 0)
    {
        gpio_direction_input(config_info.irq_gpio_number);
    }
    if(req_success){
        gpio_free(config_info.irq_gpio_number);
    }
    else
    {
        printk("int2io gpio request failed! config_info.irq_gpio_number = %d,ret = %d\n",config_info.irq_gpio_number,req_success);
    }

}

/*******************************************************	
Function:
	Read data from the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:read data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer
*********************************************************/
s32 gtp_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
    struct i2c_msg msgs[2];
    s32 ret=-1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr  = client->addr;
    msgs[0].len   = GTP_ADDR_LENGTH;
    msgs[0].buf   = &buf[0];

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr  = client->addr;
    msgs[1].len   = len - GTP_ADDR_LENGTH;
    msgs[1].buf   = &buf[GTP_ADDR_LENGTH];

    while(retries < 5)
    {
        ret = i2c_transfer(client->adapter, msgs, 2);
        if(ret == 2)break;
        retries++;
    }
    if(retries >= 5)
    {
        GTP_DEBUG("I2C retry timeout, reset chip.");
        gtp_reset_guitar(10);
    }
    return ret;
}

/*******************************************************	
Function:
	write data to the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:write data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer.
*********************************************************/
s32 gtp_i2c_write(struct i2c_client *client,u8 *buf,s32 len)
{
    struct i2c_msg msg;
    s32 ret=-1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();

    msg.flags = !I2C_M_RD;
    msg.addr  = client->addr;
    msg.len   = len;
    msg.buf   = buf;

    while(retries < 5)
    {
        ret = i2c_transfer(client->adapter, &msg, 1);
        if (ret == 1)break;
        retries++;
    }
    if(retries >= 5)
    {
        GTP_DEBUG("I2C retry timeout, reset chip.");
        gtp_reset_guitar(10);
    }
    return ret;
}

/**
 * ctp_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

    int  i = 0;
	int  ret = -1;
      
    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)){
    	printk("======return=====\n");
            return -ENODEV;
    }
	
	if(twi_id == adapter->nr){
        uint8_t id = 0;
        printk("%s: addr= %x\n",__func__,client->addr);
        msleep(50);
        
        gtp_i2c_read(client,read_chip_value,3);
        id = (read_chip_value[2] - '0')&0xf;
        read_chip_value[1] = 0x42;
        gtp_i2c_read(client,read_chip_value,3);
        id = ((id << 4)&0xf0)|((read_chip_value[2] - '0')&0x0f);
        read_chip_value[2] = id;
        printk("chip_id_value:0x%x\n",id);
        while(chip_id_value[i++]){
                if(read_chip_value[2] == chip_id_value[i - 1]){
                    printk("I2C connection sucess!\n");
	                strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
					m_inet_ctpState = 1;
					printk("%s", GT9XX_USED);
	                return 0;
                }                   
        }
    	printk("%s:I2C connection might be something wrong ! \n",__func__);
    	return -ENODEV;
	}else{
	        return -ENODEV;
	}
	
	 /*
	if(twi_id == adapter->nr){
            printk("%s: addr = %x\n", __func__, client->addr);
            ret = gtp_i2c_test(client);
			printk("detect ret %d\n",ret);
            if(ret!=0){
        		printk("%s:I2C connection might be something wrong \n", __func__);
        		return -ENODEV;
        	}else{    
        		m_inet_ctpState =  1;
				printk("%s\n",GT9XX_USED);
            	strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
				printk("======detect ok !=====\n");
				return 0;	
	        }
	}else{
	        return -ENODEV;
	}
	*/
}

/*******************************************************
Function:
	Send config Function.

Input:
	client:	i2c client.

Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
s32 gtp_send_cfg(struct i2c_client *client)
{
    s32 ret = 0;
    
#if GTP_DRIVER_SEND_CFG
    s32 retry = 0;

    for (retry = 0; retry < 5; retry++)
    {
        ret = gtp_i2c_write(client, config , GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH);
        if (ret > 0)
        {
            break;
        }
    }
#endif

    return ret;
}

/*******************************************************
Function:
	Enable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
/*void gtp_irq_disable(struct goodix_ts_data *ts)
{
    unsigned long irqflags;

    GTP_DEBUG_FUNC();

    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (!ts->irq_is_disable)
    {
        ts->irq_is_disable = 1; 
        disable_irq_nosync(ts->client->irq);
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}
*/
/*******************************************************
Function:
	Disable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
/*void gtp_irq_enable(struct goodix_ts_data *ts)
{
    unsigned long irqflags = 0;

    GTP_DEBUG_FUNC();
    
    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (ts->irq_is_disable) 
    {
        enable_irq(ts->client->irq);
        ts->irq_is_disable = 0;	
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}*/

/*******************************************************
Function:
	Touch down report function.

Input:
	ts:private data.
	id:tracking id.
	x:input x.
	y:input y.
	w:input weight.
	
Output:
	None.
*******************************************************/
static void gtp_touch_down(struct goodix_ts_data* ts,s32 id,s32 x,s32 y,s32 w)
{
	switch(Q978_index)
	{
		case 3:
		case 4:
			x = screen_max_x- x;
			break;
		default:
			break;
	}
	
#if GTP_CHANGE_X2Y
    GTP_SWAP(x, y);
#endif

#if GTP_ICS_SLOT_REPORT
    input_mt_slot(ts->input_dev, id);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
#else
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_mt_sync(ts->input_dev);
#endif

    GTP_DEBUG("ID:%d, X:%d, Y:%d, W:%d", id, x, y, w);
}

/*******************************************************
Function:
	Touch up report function.

Input:
	ts:private data.
	
Output:
	None.
*******************************************************/
static void gtp_touch_up(struct goodix_ts_data* ts, s32 id)
{
#if GTP_ICS_SLOT_REPORT
    input_mt_slot(ts->input_dev, id);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
    GTP_DEBUG("Touch id[%2d] release!", id);
#else
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
    input_mt_sync(ts->input_dev);
#endif
}
#ifdef TP_LIGHT
static void light_ctrl(int i)
{
	
	if(have_light == 0)
		return ;
	
	__gpio_set_value(inet_tp_light.gpio.gpio , i);
	
	if(i == 0)
		mod_timer( &(inet_tp_light.timer),0);
	else
		mod_timer( &(inet_tp_light.timer),jiffies + inet_tp_light.timeout/(1000/HZ));	
}
#endif
/*******************************************************
Function:
	Goodix touchscreen work function.

Input:
	work:	work_struct of goodix_wq.
	
Output:
	None.
*******************************************************/
static void goodix_ts_work_func(struct work_struct *work)
{
    u8  end_cmd[3] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF, 0};
    u8  point_data[2 + 1 + 8 * GTP_MAX_TOUCH + 1]={GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF};
    u8  touch_num = 0;
    u8  finger = 0;
    static u16 pre_touch = 0;
    static u8 pre_key = 0;
    u8  key_value = 0;
    u8* coor_data = NULL;
    s32 input_x = 0;
    s32 input_y = 0;
    s32 input_w = 0;
    s32 id = 0;
    s32 i  = 0;
    s32 ret = -1;
    struct goodix_ts_data *ts = NULL;

    GTP_DEBUG_FUNC();
#ifdef TP_LIGHT
	//wzh 20140307 for light
	light_ctrl(1);
#endif
    ts = container_of(work, struct goodix_ts_data, work);
    if (ts->enter_update)
    {
        return;
    }

    ret = gtp_i2c_read(ts->client, point_data, 12);
    if (ret < 0)
    {
        GTP_ERROR("I2C transfer error. errno:%d\n ", ret);
        goto exit_work_func;
    }

    finger = point_data[GTP_ADDR_LENGTH];    
    if((finger & 0x80) == 0)
    {
        goto exit_work_func;
    }

    touch_num = finger & 0x0f;
    if (touch_num > GTP_MAX_TOUCH)
    {
        goto exit_work_func;
    }

    if (touch_num > 1)
    {
        u8 buf[8 * GTP_MAX_TOUCH] = {(GTP_READ_COOR_ADDR + 10) >> 8, (GTP_READ_COOR_ADDR + 10) & 0xff};

        ret = gtp_i2c_read(ts->client, buf, 2 + 8 * (touch_num - 1)); 
        memcpy(&point_data[12], &buf[2], 8 * (touch_num - 1));
    }

#if GTP_HAVE_TOUCH_KEY
    key_value = point_data[3 + 8 * touch_num];
    
    if(key_value || pre_key)
    {
        for (i = 0; i < GTP_MAX_KEY_NUM; i++)
        {
            input_report_key(ts->input_dev, touch_key_array[i], key_value & (0x01<<i));   
        }
        touch_num = 0;
        pre_touch = 0;
    }
#endif
    pre_key = key_value;

    GTP_DEBUG("pre_touch:%02x, finger:%02x.", pre_touch, finger);

#if GTP_ICS_SLOT_REPORT
    if (pre_touch || touch_num)
    {
        s32 pos = 0;
        u16 touch_index = 0;

        coor_data = &point_data[3];
        if(touch_num)
        {
            id = coor_data[pos] & 0x0F;
            touch_index |= (0x01<<id);
        }

        GTP_DEBUG("id=%d,touch_index=0x%x,pre_touch=0x%x\n",id, touch_index,pre_touch);
        for (i = 0; i < GTP_MAX_TOUCH; i++)
        {
            if (touch_index & (0x01<<i))
            {
                input_x  = coor_data[pos + 1] | coor_data[pos + 2] << 8;
                input_y  = coor_data[pos + 3] | coor_data[pos + 4] << 8;
                input_w  = coor_data[pos + 5] | coor_data[pos + 6] << 8;

                gtp_touch_down(ts, id, input_x, input_y, input_w);
                pre_touch |= 0x01 << i;

                pos += 8;
                id = coor_data[pos] & 0x0F;
                touch_index |= (0x01<<id);
            }
            else// if (pre_touch & (0x01 << i))
            {
                gtp_touch_up(ts, i);
                pre_touch &= ~(0x01 << i);
            }
        }
    }

#else
    if (touch_num )
    {
        for (i = 0; i < touch_num; i++)
        {
            coor_data = &point_data[i * 8 + 3];

            id = coor_data[0] & 0x0F;
            input_x  = coor_data[1] | coor_data[2] << 8;
            input_y  = coor_data[3] | coor_data[4] << 8;
            input_w  = coor_data[5] | coor_data[6] << 8;
            gtp_touch_down(ts, id, input_x, input_y, input_w/10);//modify by mbg 20140422
        }
    }
    else if (pre_touch)
    {
        GTP_DEBUG("Touch Release!");
        gtp_touch_up(ts, 0);
    }

    pre_touch = touch_num;
    input_report_key(ts->input_dev, BTN_TOUCH, (touch_num || key_value));
#endif

    input_sync(ts->input_dev);

exit_work_func:
    if(!ts->gtp_rawdiff_mode)
    {
        ret = gtp_i2c_write(ts->client, end_cmd, 3);
        if (ret < 0)
        {
            GTP_INFO("I2C write end_cmd  error!"); 
        }
    }

    //if (ts->use_irq)
    //{
    //    gtp_irq_enable(ts);
    //}
}

/*******************************************************
Function:
	Timer interrupt service routine.

Input:
	timer:	timer struct pointer.
	
Output:
	Timer work mode. HRTIMER_NORESTART---not restart mode
*******************************************************/
static enum hrtimer_restart goodix_ts_timer_handler(struct hrtimer *timer)
{
    struct goodix_ts_data *ts = container_of(timer, struct goodix_ts_data, timer);

    GTP_DEBUG_FUNC();

    queue_work(goodix_wq, &ts->work);
    hrtimer_start(&ts->timer, ktime_set(0, (GTP_POLL_TIME+6)*1000000), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/*******************************************************
Function:
	External interrupt service routine.

Input:
	irq:	interrupt number.
	dev_id: private data pointer.
	
Output:
	irq execute status.
*******************************************************/
static u32 goodix_ts_irq_handler(struct goodix_ts_data *ts)
{
    queue_work(goodix_wq, &ts->work);
    return 0;
}
/*******************************************************
Function:
	Int sync Function.

Input:
	ms:sync time.
	
Output:
	None.
*******************************************************/
void gtp_int_sync(s32 ms)   //lbzhang
{
    struct gpio_config_eint_all cfg = {0};
    int2io(1,1);
    msleep(ms);
    /* config to eint, and set pull, drivel level, trig type */
	cfg.gpio 	= CTP_IRQ_NUMBER;
	cfg.pull 	= GPIO_PULL_DEFAULT;
	cfg.drvlvl 	= GPIO_DRVLVL_DEFAULT;
	cfg.enabled	= 0;
	cfg.trig_type	= CTP_IRQ_MODE;
	if(0 != sw_gpio_eint_setall_range(&cfg, 1)) {
        printk(KERN_ERR "sw_gpio_eint_setall_range failed\n");
	}
}

/*******************************************************
Function:
	Reset chip Function.

Input:
	ms:reset time.
	
Output:
	None.
*******************************************************/
void gtp_reset_guitar(s32 ms)
{
    GTP_DEBUG_FUNC();
    
    int2io(1,1);
    printk("%s,%d\n",__func__,__LINE__);
    msleep(2);
    gpio_direction_output(config_info.wakeup_gpio_number, 0);
    msleep(ms);
    __gpio_set_value(config_info.wakeup_gpio_number, 1);
    
    msleep(6);                          //must > 3ms
    gpio_direction_input(config_info.wakeup_gpio_number);

    gtp_int_sync(50);
}

/*******************************************************
Function:
	Eter sleep function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_enter_sleep(struct goodix_ts_data * ts)
{
    s8 ret = -1;
    s8 retry = 0;
    u8 i2c_control_buf[3] = {(u8)(GTP_REG_SLEEP >> 8), (u8)GTP_REG_SLEEP, 5};

    GTP_DEBUG_FUNC();

    int2io(1,1);
    msleep(5);
    while(retry++ < 5)
    {
        ret = gtp_i2c_write(ts->client, i2c_control_buf, 3);
        if (ret > 0)
        {
            GTP_DEBUG("GTP enter sleep!");
            return ret;
        }
        msleep(10);
    }
    GTP_ERROR("GTP send sleep cmd failed.");
    return ret;
}

/*******************************************************
Function:
	Wakeup from sleep mode Function.

Input:
	ts:	private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_wakeup_sleep(struct goodix_ts_data * ts)
{
    u8 retry = 0;
    s8 ret = -1;

    GTP_DEBUG_FUNC();

#if GTP_POWER_CTRL_SLEEP
    while(retry++ < 5)
    {
        gtp_reset_guitar(20);
        ret = gtp_send_cfg(ts->client);
        if (ret > 0)
        {
            GTP_DEBUG("Wakeup sleep send config success.");
            return ret;
        }
    }
#else
    while(retry++ < 10)
    {
        gpio_direction_output(config_info.wakeup_gpio_number, 1);

        msleep(5);
        
        ret = gtp_i2c_test(ts->client);
        if (ret > 0)
        {
            GTP_DEBUG("GTP wakeup sleep.");
            
            gtp_int_sync(25);
            return ret;
        }
        gtp_reset_guitar(20);
    }
#endif

    GTP_ERROR("GTP wakeup sleep failed.");
    return ret;
}

/*******************************************************
Function:
	GTP initialize function.

Input:
	ts:	i2c client private struct.
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
	static s32 gtp_init_panel(struct goodix_ts_data *ts)
	{
		s32 ret = -1;

		//mbg ++ 20130321
	u8 *cfg1,*cfg2,*cfg3,*cfg4,*cfg5,*cfg6;
	u8 cfg1_len, cfg2_len, cfg3_len,cfg4_len, cfg5_len, cfg6_len;
	//mbg --
	  
#if GTP_DRIVER_SEND_CFG
		s32 i;
		u8 check_sum = 0;
		u8 rd_cfg_buf[16];
		int index =0;

		if(gt9xx_cob_number == 130)
		{
			static u8 cfg_info_group1[] = CTP_CFG_GROUP1_1280_800_Q130_QSD; 
			static u8 cfg_info_group2[] = CTP_CFG_GROUP2_1280_800_Q130_QSD; 
			static u8 cfg_info_group3[] = CTP_CFG_GROUP3_1280_800_Q130_QSD; 
			static u8 cfg_info_group4[] = CTP_CFG_GROUP4_1280_800_Q130_QSD;
			static u8 cfg_info_group5[] = CTP_CFG_GROUP5_1280_800_Q130_QSD;
			static u8 cfg_info_group6[] = CTP_CFG_GROUP6_1280_800_Q130_QSD;

			cfg1 = cfg_info_group1;
			cfg2 = cfg_info_group2;
			cfg3 = cfg_info_group3;
			cfg4 = cfg_info_group4;
			cfg5 = cfg_info_group5;
			cfg6 = cfg_info_group6;

			cfg1_len = sizeof(cfg_info_group1)/sizeof(cfg_info_group1[0]);
			cfg2_len = sizeof(cfg_info_group2)/sizeof(cfg_info_group2[0]);
			cfg3_len = sizeof(cfg_info_group3)/sizeof(cfg_info_group3[0]);
			cfg4_len = sizeof(cfg_info_group4)/sizeof(cfg_info_group4[0]);
			cfg5_len = sizeof(cfg_info_group5)/sizeof(cfg_info_group5[0]);
			cfg6_len = sizeof(cfg_info_group6)/sizeof(cfg_info_group6[0]);
		}
		else
		{
			if(screen_max_x == 1920 && screen_max_y == 1200)
			{
				static u8 cfg_info_group1[] = CTP_CFG_GROUP1_1920_1200; 
				static u8 cfg_info_group2[] = CTP_CFG_GROUP2_1920_1200; 
				static u8 cfg_info_group3[] = CTP_CFG_GROUP3_1920_1200; 
				static u8 cfg_info_group4[] = CTP_CFG_GROUP4_1920_1200;
				static u8 cfg_info_group5[] = CTP_CFG_GROUP5_1920_1200;
				static u8 cfg_info_group6[] = CTP_CFG_GROUP6_1920_1200;

				cfg1 = cfg_info_group1;
				cfg2 = cfg_info_group2;
				cfg3 = cfg_info_group3;
				cfg4 = cfg_info_group4;
				cfg5 = cfg_info_group5;
				cfg6 = cfg_info_group6;

				cfg1_len = sizeof(cfg_info_group1)/sizeof(cfg_info_group1[0]);
				cfg2_len = sizeof(cfg_info_group2)/sizeof(cfg_info_group2[0]);
				cfg3_len = sizeof(cfg_info_group3)/sizeof(cfg_info_group3[0]);
				cfg4_len = sizeof(cfg_info_group4)/sizeof(cfg_info_group4[0]);
				cfg5_len = sizeof(cfg_info_group5)/sizeof(cfg_info_group5[0]);
				cfg6_len = sizeof(cfg_info_group6)/sizeof(cfg_info_group6[0]);
			}
			else if(screen_max_x == 1280 && screen_max_y == 800)
			{
				static u8 cfg_info_group1[] = CTP_CFG_GROUP1_1280_800; 
				static u8 cfg_info_group2[] = CTP_CFG_GROUP2_1280_800; 
				static u8 cfg_info_group3[] = CTP_CFG_GROUP3_1280_800; 
				static u8 cfg_info_group4[] = CTP_CFG_GROUP4_1280_800;
				static u8 cfg_info_group5[] = CTP_CFG_GROUP5_1280_800;
				static u8 cfg_info_group6[] = CTP_CFG_GROUP6_1280_800;

				cfg1 = cfg_info_group1;
				cfg2 = cfg_info_group2;
				cfg3 = cfg_info_group3;
				cfg4 = cfg_info_group4;
				cfg5 = cfg_info_group5;
				cfg6 = cfg_info_group6;

				cfg1_len = sizeof(cfg_info_group1)/sizeof(cfg_info_group1[0]);
				cfg2_len = sizeof(cfg_info_group2)/sizeof(cfg_info_group2[0]);
				cfg3_len = sizeof(cfg_info_group3)/sizeof(cfg_info_group3[0]);
				cfg4_len = sizeof(cfg_info_group4)/sizeof(cfg_info_group4[0]);
				cfg5_len = sizeof(cfg_info_group5)/sizeof(cfg_info_group5[0]);
				cfg6_len = sizeof(cfg_info_group6)/sizeof(cfg_info_group6[0]);
			}
			else if(screen_max_x == 800 && screen_max_y == 1280)
			{
				printk("come in QNV8");
				static u8 cfg_info_group1[] = CTP_CFG_GROUP1_800_1280; 
				static u8 cfg_info_group2[] = CTP_CFG_GROUP2_800_1280; 
				static u8 cfg_info_group3[] = CTP_CFG_GROUP3_800_1280; 
				static u8 cfg_info_group4[] = CTP_CFG_GROUP4_800_1280;
				static u8 cfg_info_group5[] = CTP_CFG_GROUP5_800_1280;
				static u8 cfg_info_group6[] = CTP_CFG_GROUP6_800_1280;

				cfg1 = cfg_info_group1;
				cfg2 = cfg_info_group2;
				cfg3 = cfg_info_group3;
				cfg4 = cfg_info_group4;
				cfg5 = cfg_info_group5;
				cfg6 = cfg_info_group6;

				cfg1_len = sizeof(cfg_info_group1)/sizeof(cfg_info_group1[0]);
				cfg2_len = sizeof(cfg_info_group2)/sizeof(cfg_info_group2[0]);
				cfg3_len = sizeof(cfg_info_group3)/sizeof(cfg_info_group3[0]);
				cfg4_len = sizeof(cfg_info_group4)/sizeof(cfg_info_group4[0]);
				cfg5_len = sizeof(cfg_info_group5)/sizeof(cfg_info_group5[0]);
				cfg6_len = sizeof(cfg_info_group6)/sizeof(cfg_info_group6[0]);
			}
			else if(screen_max_x == 1024 && screen_max_y == 768)
			{
				static u8 cfg_info_group1[] = CTP_CFG_GROUP1_Q978; 
				static u8 cfg_info_group2[] = CTP_CFG_GROUP2_Q978; 
				static u8 cfg_info_group3[] = CTP_CFG_GROUP3_Q978; 
				static u8 cfg_info_group4[] = CTP_CFG_GROUP4_Q978;
				static u8 cfg_info_group5[] = CTP_CFG_GROUP5_Q978;
				static u8 cfg_info_group6[] = CTP_CFG_GROUP6_Q978;

				cfg1 = cfg_info_group1;
				cfg2 = cfg_info_group2;
				cfg3 = cfg_info_group3;
				cfg4 = cfg_info_group4;
				cfg5 = cfg_info_group5;
				cfg6 = cfg_info_group6;

				cfg1_len = sizeof(cfg_info_group1)/sizeof(cfg_info_group1[0]);
				cfg2_len = sizeof(cfg_info_group2)/sizeof(cfg_info_group2[0]);
				cfg3_len = sizeof(cfg_info_group3)/sizeof(cfg_info_group3[0]);
				cfg4_len = sizeof(cfg_info_group4)/sizeof(cfg_info_group4[0]);
				cfg5_len = sizeof(cfg_info_group5)/sizeof(cfg_info_group5[0]);
				cfg6_len = sizeof(cfg_info_group6)/sizeof(cfg_info_group6[0]);
			}
		}
		/*
		u8 *send_cfg_buf[] = {cfg_info_group1,cfg_info_group2,cfg_info_group3,cfg_info_group4,cfg_info_group5,cfg_info_group6};
		u8 cfg_info_len[] = {sizeof(cfg_info_group1)/sizeof(cfg_info_group1[0]),\
				  sizeof(cfg_info_group2)/sizeof(cfg_info_group2[0]),\
				  sizeof(cfg_info_group3)/sizeof(cfg_info_group3[0]),\
				  sizeof(cfg_info_group4)/sizeof(cfg_info_group4[0]),\
				  sizeof(cfg_info_group5)/sizeof(cfg_info_group5[0]),\
				  sizeof(cfg_info_group6)/sizeof(cfg_info_group6[0])};
		*/
		u8 *send_cfg_buf[] = {cfg1, cfg2, cfg3,cfg4, cfg5, cfg6};
	u8 cfg_info_len[] = { cfg1_len,cfg2_len,cfg3_len,cfg4_len,cfg5_len,cfg6_len};
	
#if 0 //gandy
		for(i=0; i<3; i++)
		{
			if(cfg_info_len[i] > ts->gtp_cfg_len)
			{
				ts->gtp_cfg_len = cfg_info_len[i];
			}
		}
#endif
		
		GTP_DEBUG("len1=%d,len2=%d,len3=%d,send_len:%d",cfg_info_len[0],cfg_info_len[1],cfg_info_len[2],ts->gtp_cfg_len);
#if 0
		if ((!cfg_info_len[1]) && (!cfg_info_len[2]))
		{
			rd_cfg_buf[GTP_ADDR_LENGTH] = 0; 
		}
		else
#endif
		{
			rd_cfg_buf[0] = GTP_REG_SENSOR_ID >> 8;
			rd_cfg_buf[1] = GTP_REG_SENSOR_ID & 0xff;
			msleep(50);
			ret = gtp_i2c_read(ts->client, rd_cfg_buf, 3);
			if (ret < 0)
			{
				GTP_ERROR("Read SENSOR ID failed,default use group1 config!");
				rd_cfg_buf[GTP_ADDR_LENGTH] = 0;
			}
			printk("sensor id = %d\n",rd_cfg_buf[GTP_ADDR_LENGTH]);
			

			if(rd_cfg_buf[GTP_ADDR_LENGTH] >= 6)
			{
				rd_cfg_buf[GTP_ADDR_LENGTH] = 0;
			}

			
			//rd_cfg_buf[GTP_ADDR_LENGTH] &= 0x07;
			
		}


       
        
        index = rd_cfg_buf[GTP_ADDR_LENGTH];

		//d978_index = index;
		
		ts->gtp_cfg_len = cfg_info_len[index];
		GTP_DEBUG("gandy---SENSOR ID:%d\n", rd_cfg_buf[GTP_ADDR_LENGTH]);
		if(screen_max_x == 1024 && screen_max_y == 768)
		{
			switch(rd_cfg_buf[GTP_ADDR_LENGTH])
			{
				case 3:
					Q978_index = 3;
					break;
				case 4:
					Q978_index = 4;
					break;
				defualt:
					break;
			}
			
		}

		memset(&config[GTP_ADDR_LENGTH], 0, GTP_CONFIG_MAX_LENGTH);
		memcpy(&config[GTP_ADDR_LENGTH], send_cfg_buf[index], ts->gtp_cfg_len);
	
#if GTP_CUSTOM_CFG
		config[RESOLUTION_LOC]	   = (u8)GTP_MAX_WIDTH;
		config[RESOLUTION_LOC + 1] = (u8)(GTP_MAX_WIDTH>>8);
		config[RESOLUTION_LOC + 2] = (u8)GTP_MAX_HEIGHT;
		config[RESOLUTION_LOC + 3] = (u8)(GTP_MAX_HEIGHT>>8);
		
		if (GTP_INT_TRIGGER == 0)  //RISING
		{
			config[TRIGGER_LOC] &= 0xfe; 
		}
		else if (GTP_INT_TRIGGER == 1)	//FALLING
		{
			config[TRIGGER_LOC] |= 0x01;
		}
#endif  //endif GTP_CUSTOM_CFG
		
		check_sum = 0;
		for (i = GTP_ADDR_LENGTH; i < ts->gtp_cfg_len; i++)
		{
			check_sum += config[i];
		}
		config[ts->gtp_cfg_len] = (~check_sum) + 1;
		
#else //else DRIVER NEED NOT SEND CONFIG
	
		if(ts->gtp_cfg_len == 0)
		{
			ts->gtp_cfg_len = GTP_CONFIG_MAX_LENGTH;
		}
		ret = gtp_i2c_read(ts->client, config, ts->gtp_cfg_len + GTP_ADDR_LENGTH);
		if (ret < 0)
		{
			GTP_ERROR("GTP read resolution & max_touch_num failed, use default value!");
			ts->abs_x_max = GTP_MAX_WIDTH;
			ts->abs_y_max = GTP_MAX_HEIGHT;
			ts->int_trigger_type = GTP_INT_TRIGGER;
		}
#endif //endif GTP_DRIVER_SEND_CFG
	
		GTP_DEBUG_FUNC();
	
		ts->abs_x_max = (config[RESOLUTION_LOC + 1] << 8) + config[RESOLUTION_LOC];
		ts->abs_y_max = (config[RESOLUTION_LOC + 3] << 8) + config[RESOLUTION_LOC + 2];
		ts->int_trigger_type = (config[TRIGGER_LOC]) & 0x03;
		if ((!ts->abs_x_max)||(!ts->abs_y_max))
		{
			GTP_ERROR("GTP resolution & max_touch_num invalid, use default value!");
			ts->abs_x_max = GTP_MAX_WIDTH;
			ts->abs_y_max = GTP_MAX_HEIGHT;
		}
		
		msleep(100);
		ret = gtp_send_cfg(ts->client);
		if (ret < 0)
		{
			printk("\ngandy-----send config error.ret=%d\n",ret);
			GTP_ERROR("Send config error.");
		}
		GTP_DEBUG("X_MAX = %d,Y_MAX = %d,TRIGGER = 0x%02x",
				 ts->abs_x_max,ts->abs_y_max,ts->int_trigger_type);
	
		msleep(10);
	
		return 0;
	}

/*******************************************************
Function:
	Read goodix touchscreen version function.

Input:
	client:	i2c client struct.
	version:address to store version info
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
s32 gtp_read_version(struct i2c_client *client, u16* version)
{
    s32 ret = -1;
    s32 i = 0;
    u8 buf[8] = {GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff};

    GTP_DEBUG_FUNC();

    ret = gtp_i2c_read(client, buf, sizeof(buf));
    if (ret < 0)
    {
        GTP_ERROR("GTP read version failed");
        return ret;
    }

    if (version)
    {
        *version = (buf[7] << 8) | buf[6];
    }

    for(i=2; i<6; i++)
    {
        if(!buf[i])
        {
            buf[i] = 0x30;
        }
    }
    GTP_INFO("IC VERSION:%c%c%c%c_%02x%02x", 
              buf[2], buf[3], buf[4], buf[5], buf[7], buf[6]);

    return ret;
}

/*******************************************************
Function:
	I2c test Function.

Input:
	client:i2c client.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_i2c_test(struct i2c_client *client)
{
    u8 test[3] = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
    u8 retry = 0;
    s8 ret = -1;
  
    GTP_DEBUG_FUNC();
  
    while(retry++ < 5)
    {
        ret = gtp_i2c_read(client, test, 3);
        if (ret > 0)
        {
            return ret;
        }
        GTP_ERROR("GTP i2c test failed time %d.",retry);
        msleep(10);
    }
    return ret;
}

/*******************************************************
Function:
	Request input device Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_request_input_dev(struct goodix_ts_data *ts)
{
    s8 ret = -1;
    //s8 phys[32];
#if GTP_HAVE_TOUCH_KEY
    u8 index = 0;
#endif
  
    GTP_DEBUG_FUNC();
  
    ts->input_dev = input_allocate_device();
    if (ts->input_dev == NULL)
    {
        GTP_ERROR("Failed to allocate input device.");
        return -ENOMEM;
    }

    ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
#if GTP_ICS_SLOT_REPORT
    __set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
    input_mt_init_slots(ts->input_dev, 255);
#else
    ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif

#if GTP_HAVE_TOUCH_KEY
    for (index = 0; index < GTP_MAX_KEY_NUM; index++)
    {
        input_set_capability(ts->input_dev,EV_KEY,touch_key_array[index]);	
    }
#endif

#if GTP_CHANGE_X2Y
    GTP_SWAP(ts->abs_x_max, ts->abs_y_max);
#endif
    printk("ts->abs_x_max = %d,ts->abs_y_max = %d\n",ts->abs_x_max,ts->abs_y_max);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->abs_x_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);	
    input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 255, 0, 0);

    //sprintf(phys, "input/ts");
    ts->input_dev->name = goodix_ts_name;
    ts->input_dev->phys = "input/ts";//phys;
    ts->input_dev->id.bustype = BUS_I2C;
    ts->input_dev->id.vendor = 0xDEAD;
    ts->input_dev->id.product = 0xBEEF;
    ts->input_dev->id.version = 10427;
	
    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        GTP_ERROR("Register %s input device failed", ts->input_dev->name);
        return -ENODEV;
    }
    
#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = goodix_ts_early_suspend;
    ts->early_suspend.resume = goodix_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif

    return 0;
}

/*******************************************************
Function:
	Goodix touchscreen probe function.

Input:
	client:	i2c device struct.
	id:device id.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    s32 ret = -1;
    struct goodix_ts_data *ts;
    u16 version_info;

    GTP_DEBUG_FUNC();
    
    //do NOT remove these output log
    GTP_INFO("GTP Driver Version:%s",GTP_DRIVER_VERSION);
    GTP_INFO("GTP Driver build@%s,%s", __TIME__,__DATE__);
    GTP_INFO("GTP I2C Address:0x%02x", client->addr);

    i2c_connect_client = client;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
    {
        GTP_ERROR("I2C check functionality failed.");
        return -ENODEV;
    }
    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
        GTP_ERROR("Alloc GFP_KERNEL memory failed.");
        return -ENOMEM;
    }
   
    memset(ts, 0, sizeof(*ts));
    ts->client = client;
    i2c_set_clientdata(client, ts);
    ts->gtp_rawdiff_mode = 0;
	/*
    ret = gtp_i2c_test(client);
    if (ret < 0)
    {
    	m_inet_ctpState = 0;
        GTP_ERROR("I2C communication ERROR!");
		return -ENOMEM;
    }
	*/
	INIT_WORK(&ts->work, goodix_ts_work_func);	
	goodix_wq = create_singlethread_workqueue("goodix_wq");
	if (!goodix_wq)
	{
		GTP_ERROR("Creat workqueue failed.");
		return -ENOMEM;
	}
#if GTP_AUTO_UPDATE
    ret = gup_init_update_proc(ts);
    if (ret < 0)
    {
        GTP_ERROR("Create update thread error.");
    }
#endif
    
    ret = gtp_init_panel(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP init panel failed.");
    }

    ret = gtp_request_input_dev(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP request input dev failed");
    }
    int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)goodix_ts_irq_handler,ts);
    if (!int_handle)
    {
        GTP_INFO("GTP works in polling mode.");
    }
    else
    {
        ts->use_irq = 1;
        GTP_INFO("GTP works in interrupt mode.");
    }

    ret = gtp_read_version(client, &version_info);
    if (ret < 0)
    {
        GTP_ERROR("Read version failed.");
    }
#if GTP_CREATE_WR_NODE
    init_wr_node(client);
#endif

#if GTP_ESD_PROTECT
    INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
    gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
    queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE); 
#endif

    return 0;
}


/*******************************************************
Function:
	Goodix touchscreen driver release function.

Input:
	client:	i2c device struct.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
	
    GTP_DEBUG_FUNC();
	
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ts->early_suspend);
#endif

#if GTP_CREATE_WR_NODE
    uninit_wr_node();
#endif

#if GTP_ESD_PROTECT
    destroy_workqueue(gtp_esd_check_workqueue);
#endif

    if (ts) 
    {
        if (ts->use_irq)
        {
            int2io(1,1);
            sw_gpio_irq_free(int_handle);
            ts->use_irq = 0;
        }
        else
        {
            hrtimer_cancel(&ts->timer);
        }
    }	
	
    GTP_INFO("GTP driver is removing...");
    i2c_set_clientdata(client, NULL);
    input_unregister_device(ts->input_dev);
    kfree(ts);

    return 0;
}

/*******************************************************
Function:
	Early suspend function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    s8 ret = -1;	
    ts = container_of(h, struct goodix_ts_data, early_suspend);
	
    GTP_DEBUG_FUNC();

#if GTP_ESD_PROTECT
    ts->gtp_is_suspend = 1;
    cancel_delayed_work_sync(&gtp_esd_check_work);
#endif

    if (ts->use_irq)
    {
        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
    }
    else
    {
        hrtimer_cancel(&ts->timer);
    }
    ret = cancel_work_sync(&ts->work);
    flush_workqueue(goodix_wq);
#ifdef TP_LIGHT
	light_ctrl(0);//wzh
#endif
    ret = gtp_enter_sleep(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP early suspend failed.");
    }
}

/*******************************************************
Function:
	Late resume function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
static void goodix_ts_late_resume(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    s8 ret = -1;
    ts = container_of(h, struct goodix_ts_data, early_suspend);
	
    GTP_DEBUG_FUNC();
	
    ret = gtp_wakeup_sleep(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP later resume failed.");
    }

    if (ts->use_irq)
    {
        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1); 
    }
    else
    {
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }

#if GTP_ESD_PROTECT
    ts->gtp_is_suspend = 0;
    queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
#endif
}
#endif

#if GTP_ESD_PROTECT
static void gtp_esd_check_func(struct work_struct *work)
{
    s32 i;
    s32 ret = -1;
    struct goodix_ts_data *ts = NULL;
    u8 test[3] = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};

    GTP_DEBUG_FUNC();

    ts = i2c_get_clientdata(i2c_connect_client);

    if (ts->gtp_is_suspend)
    {
        return;
    }
    
    for (i = 0; i < 3; i++)
    {
        ret = gtp_i2c_read(i2c_connect_client, test, 3);
        if (ret >= 0)
        {
            break;
        }
    }
    
    if (i >= 3)
    {
        gtp_reset_guitar(50);
    }

    if(!ts->gtp_is_suspend)
    {
        queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
    }

    return;
}
#endif

static const struct i2c_device_id goodix_ts_id[] = {
    { GTP_I2C_NAME, 0 },
    { }
};

static struct i2c_driver goodix_ts_driver = {
    .class = I2C_CLASS_HWMON,
    .probe      = goodix_ts_probe,
    .remove     = goodix_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend    = goodix_ts_early_suspend,
    .resume     = goodix_ts_late_resume,
#endif
    .id_table   = goodix_ts_id,
    .driver = {
        .name     = GTP_I2C_NAME,
        .owner    = THIS_MODULE,
    },
    .address_list	= normal_i2c,
};

static int ctp_get_system_config(void)
{   
		script_item_u   val;
        

	   	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "gt9xx_cob_number", &val))
		{
			gt9xx_cob_number=0;
		}
		else
		{
			gt9xx_cob_number = val.val;
		}
		//gt9xx_cob_number = 130;
		printk("gt9xx_cob_number = %d\n",gt9xx_cob_number);
		
		
        ctp_print_info(config_info,DEBUG_INIT);
        twi_id = config_info.twi_id;
        screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
        revert_x_flag = config_info.revert_x_flag;
        revert_y_flag = config_info.revert_y_flag;
        exchange_x_y_flag = config_info.exchange_x_y_flag;
        if((twi_id == 0) || (screen_max_x == 0) || (screen_max_y == 0)){
            printk("%s:read config error!\n",__func__);
            return -1;
        }
        return 0;
}
#ifdef TP_LIGHT
int light_fetch_config(void)
{
	script_item_u   item;
	script_item_value_type_e   type;
	
    struct regulator *ldo = NULL;

    type = script_get_item("tp_light", "light_gpio_power", &item);
    if(type == SCIRPT_ITEM_VALUE_TYPE_STR){
        int ret = 0;
        printk("light_gpio_power:%s !\n", item.str);
        ldo = regulator_get(NULL, item.str);
        if (ldo) {
            regulator_set_voltage(ldo, 3300000, 3300000);
            ret = regulator_enable(ldo);
            if(ret < 0){
                printk("regulator_enable failed.\n");
            }
            regulator_put(ldo);
        } else {
            printk("get power regulator failed.\n");
        }
    } else {
        printk("ERR: get light_gpio_power failed");
    }

	type = script_get_item("tp_light", "tp_light_gpio", &item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("light_fentch_config %s err\n", "tp_light_gpio");
		return 0;
	}
	inet_tp_light.gpio = item.gpio;

	if(0 != gpio_request(inet_tp_light.gpio.gpio, NULL)) {
		printk("tp_light_gpio gpio_request is failed\n");
		return 0;
	}

	if (0 != gpio_direction_output(inet_tp_light.gpio.gpio, 1)) {
		printk("tp_light_gpio gpio set err!");
		return 0;
	}


printk("inet_tp_light.gpio.mul_sel[%#x]inet_tp_light.gpio.pull[%#x]inet_tp_light.gpio.data[%#x]\n",inet_tp_light.gpio.mul_sel,inet_tp_light.gpio.pull,inet_tp_light.gpio.data);
	
	
	
	type = script_get_item("tp_light", "timeout", &item);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
	{
		printk("light_fetch_config %s err\n", "timeout");
		return 0;
	}
	inet_tp_light.timeout = item.val;
	
	init_timer(&(inet_tp_light.timer));
	inet_tp_light.timer.function = light_ctrl;
	inet_tp_light.timer.data = 0;	// turn off light

	inet_tp_light.timer.expires = jiffies + 15000/(1000/HZ);	
	add_timer(&(inet_tp_light.timer));
	
	__gpio_set_value(inet_tp_light.gpio.gpio , 1); //turn on light
	
	return 1;
}
#endif

/*******************************************************	
Function:
	Driver Install function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static int __devinit goodix_ts_init(void)
{
    s32 ret = -1;

    GTP_DEBUG_FUNC();	
	
	if(m_inet_ctpState == 1)	
	{		
		printk("m_inet_ctpState return true,just return \n");		
		return -ENODEV;	
	}	
	else	
	{		
		printk("m_inet_ctpState return false,continue deteck \n");	
	}		
	
	printk("%s\n", GT9XX_IC_INFO);		
	printk("[FUN]%s\n", __func__);	
	
	ret = ctp_get_system_config();	
	if(ret != 0)  	
	{		
		printk("%s:read config fail!\n", __func__);				
		return ret;  	
	}
	gpio_direction_input(config_info.wakeup_gpio_number);
	gtp_reset_guitar(20);

	goodix_ts_driver.detect = ctp_detect;
	ret = i2c_add_driver(&goodix_ts_driver);
#ifdef TP_LIGHT
	//wzh 20140307 for light
    have_light = light_fetch_config();
#endif
	return ret; 
}

/*******************************************************	
Function:
	Driver uninstall function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static void __exit goodix_ts_exit(void)
{
    GTP_DEBUG_FUNC();
    GTP_INFO("GTP driver exited.");
    i2c_del_driver(&goodix_ts_driver);
    if (goodix_wq)
    {
        destroy_workqueue(goodix_wq);
    }
}

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("GTP Series Driver");
MODULE_LICENSE("GPL");
