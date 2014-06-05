#include <linux/i2c.h>
#include <linux/input.h>
#include "byd693x-ts.h"
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

#ifdef CONFIG_HAS_EARLYSUSPEND
    #include <linux/pm.h>
    #include <linux/earlysuspend.h>
#endif

//#define	CONFIG_TS_FUNCTION_CALLED_DEBUG			//Display the debug information whitch function is called
//#define CONFIG_TS_PROBE_DEBUG		//Display the debug information in byd693x_ts_probe function
//#define CONFIG_TS_I2C_TRANSFER_DEBUG		//Display the debug information of IIC transfer
//#define CONFIG_TPKEY_STATUS_DEBUG			//Display the debug information of Touch Key status
//#define CONFIG_TS_WORKQUEUE_DEBUG		//Display the debug ihnformation of creating work queue
//#define CONFIG_TS_COORDIATE_DEBUG		//
//#define CONFIG_TS_CUTEDGE_DEBUG			//

#define BYD693X_I2C_ADDR 0x52

#define BYD693X_USED     "\n \
												 \n+++++++++++++++++++++++++++++++++ \
                         \n++++++    BYD693X  used +++++++++ \
                         \n+++++++++++++++++++++++++++++++++ \
                         \n"
                            
#define BYD693X_IC_INFO  "\n============================================================== \
											   \nIC     :BYD693X \
                         \nAUTHOR :mbgalex@163.com \
                         \nVERSION:2013-01-28_21:23\n"
                         
extern int m_inet_ctpState;

//#define CONFIG_SEND_CFG_PARA_DEBUG

/*	Config in sys_config1.fex file:
[ctp_para]
ctp_used                 = 1
ctp_twi_id               = 2			;if A10, Set the IIC channel 2, if A13,set to 1
ctp_boxchip_type					=	 0xA10		;if A10,set to 0xA10, if A13, set to 0xA13


;byd693x-ts.c
ctp5_used									= 1
ctp5_name									= "byd693x-ts"
ctp5_twi_addr							= 0x48
ctp5_finger_no						= 5
*/

//----------------------------------------//
//GPIO Config define
#define PIO_BASE_ADDRESS (0x01C20800)
#define PIO_RANGE_SIZE (SZ_1K)

//#define PB_CFG_REG0_OFFSET (0x24)
//#define PIO_INT_CFG2_OFFSET (0x208)
//#define PIO_INT_CTRL_OFFSET (0x210)
//#define PIO_INT_STAT_OFFSET (0x214)

//#define	TOUCH_INT_NO	SW_INT_IRQNO_PIO    //GPIO :set the interrupt 

//Define for A10
//#define IRQ_EINT21                   (21)

//Define for A13
//static user_gpio_set_t  gpio_int_info[1];
//#define CTP_IRQ_PORT		(gpio_int_info[0].port)

static __u32 twi_id = 0;		//init 0, if A10, IIC Channel is 2, if A13, IIC Channel is 1

//static int  CTP_IRQ_NUM;		//Get from .fex, if A10, set to IRQ_EINT21, if A13,(gpio_int_info[0].port_num);

//static int	int_cfg_addr[]={PIO_INT_CFG0_OFFSET,PIO_INT_CFG1_OFFSET,
			//PIO_INT_CFG2_OFFSET, PIO_INT_CFG3_OFFSET};
#define byd693x_I2C_NAME	"byd693x-ts"
static void* __iomem gpio_addr = NULL;
static int gpio_int_hdle = 0;

static int gpio_wakeup_hdle = 0;

static u32 debug_mask = 0;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("***CTP***"fmt, ## arg)

extern struct ctp_config_info config_info;       
//----------------------------------------//


#define CTP_IRQ_NUMBER                  (config_info.irq_gpio_number)
#define CTP_IRQ_MODE			(TRIG_EDGE_NEGATIVE)

unsigned short g_Default_Resolution_X = 0;
unsigned short g_Default_Resolution_Y = 0;
unsigned char g_Default_CTP_Type[64];
struct sTYPEID_Info *gp_Default_CTP_sInfo;


static struct i2c_client *this_client;

//=======================================================//

#define CTP_NAME	"byd693x-ts"
//120807: Add Reset process,init pull low wakeup pin then pull high
//		Finger no set to BYD_priv->FingerNo
//121018: Add Parameter Config process
//121210: Add release all finger in byd693x_ts_resume function
//				Change Resume table to 0,0x07,0x01,0x00
/* Addresses to scan */
static union{
	unsigned short dirty_addr_buf[2];
	const unsigned short normal_i2c[2];
}u_i2c_addr = {{0x00},};


//static int BoxChip_Type = 0xA10;
static u8 CONFIG_LANDSCAPE_SCREEN = 1;
static u8 LANDSCAPE_FLAG = 1;
static int SCREEN_MAX_X = 800;
static int SCREEN_MAX_Y = 480;
static int REVERT_X_FLAG = 0;
static int REVERT_Y_FLAG = 0;
static int EXCHANGE_X_Y_FLAG = 0;
static u32 int_handle = 0;
static int Get_Finger_Num = 10;

#define	RESO_X_NO		0
#define	RESO_Y_NO		1

struct ChipSetting byd693xcfg_Resolution[]={							
//{ 2,0x08,	200/256,	200%256},	//	1	FTHD_H;FTHD_L	//手指按键阈值
//{ 2,0x0A,	120/256,	120%256},	//	2	NTHD_H;NTHD_L	//噪声阈值
{ 2,BYD_RW_RESO_X,	800/256,	800%256},	//	3 RESX_H;RESX_L	//X分辨率
{ 2,BYD_RW_RESO_Y,	480/256,	480%256},	//	4	RESY_H;RESY_L	//Y分辨率
};

void deviceResume(struct i2c_client *client);
void deviceSuspend(struct i2c_client *client);
void byd693xdeviceInit(struct i2c_client *client); 

//static int byd693x_ts_open(struct input_dev *dev);
//static void byd693x_ts_close(struct input_dev *dev);
static int byd693x_ts_suspend(struct i2c_client *client, pm_message_t mesg);
static int byd693x_ts_resume(struct i2c_client *client);
#ifdef CONFIG_HAS_EARLYSUSPEND
static void byd693x_ts_early_suspend(struct early_suspend *h);
static void byd693x_ts_late_resume(struct early_suspend *h);
#endif /* CONFIG_HAS_EARLYSUSPEND */

//static irqreturn_t byd693x_ts_isr(int irq, void *dev_id);
static struct workqueue_struct *byd693x_wq;
static int suspend_opend;
static struct input_dev *BYD_input;
static int irq_handler;

struct byd_ts_priv {
	struct i2c_client *client;
	struct input_dev *input;
	struct hrtimer timer;
	struct work_struct  BYD_work;
	struct workqueue_struct *ts_workqueue;
#ifdef	CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif 

	int irq;
	int FingerNo;
	int FingerDetect;
	u8 btn_pre_TPKey;
	int suspend_opend;
};

/***********************************************************
Read Data from TP through IIC
***********************************************************/
int ReadRegister(struct i2c_client *client,uint8_t reg,unsigned char *buf, int ByteLen)
{
//	unsigned char buf[4];
	struct i2c_msg msg[2];
	int ret;

//	memset(buf, 0xFF, sizeof(buf));
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &reg;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = ByteLen;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);

	#ifdef CONFIG_TS_I2C_TRANSFER_DEBUG
	if(ret<0)	printk("		ReadRegister: i2c_transfer Error !\n");
	else		printk("		ReadRegister: i2c_transfer OK !\n");
	#endif
	if(ret<0)		{	return 0;	}
		else		{	return 1;	}
}

/***********************************************************
Write Data to TP through IIC
***********************************************************/
void WriteRegister(struct i2c_client *client,uint8_t Reg,unsigned char Data1,unsigned char Data2,int ByteNo)
{	
	struct i2c_msg msg;
	unsigned char buf[4];
	int ret;

	buf[0]=Reg;
	buf[1]=Data1;
	buf[2]=Data2;
	buf[3]=0;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = ByteNo+1;
	msg.buf = (char *)buf;
	ret = i2c_transfer(client->adapter, &msg, 1);

	#ifdef CONFIG_TS_I2C_TRANSFER_DEBUG
	if(ret<0)	printk("		WriteRegister: i2c_master_send Error !\n");
	else		printk("		WriteRegister: i2c_master_send OK !\n");
	#endif
}


/***********************************************************
Write Config parameter to CTP through IIC
***********************************************************/
int Write_CFG_Para(struct i2c_client *client,struct sCongig_Para *lp_Config_Tab)
{	
	struct i2c_msg msg;
	int ret;

	if (!lp_Config_Tab->sConfig_Update_Enable)			//此参数是否需要做更新
		{
			return 0;		//如果不需要更新，返回0
		}
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = (lp_Config_Tab->sConfig_Length) + 1;
	
	msg.buf = lp_Config_Tab->sConfig_Table;
	ret = i2c_transfer(client->adapter, &msg, 1);

	#ifdef CONFIG_TS_I2C_TRANSFER_DEBUG
	if(ret<0)	printk("		WriteRegister: i2c_master_send Config Error !\n");
	else		printk("		WriteRegister: i2c_master_send Config OK !\n");
	#endif
	
	#ifdef CONFIG_SEND_CFG_PARA_DEBUG
		printk("%s\n",lp_Config_Tab->sConfig_script);
	#endif
	if (ret<0)
		{
			return -1;		//参数更新失败，返回-1
		}
		else
		{
			return 1;		//参数更新成功，返回1
		}
}

void byd693xdeviceInit(struct i2c_client *client)
{	
	int i;
	
	for(i=0;i<sizeof(Suspend)/sizeof(Suspend[0]);i++)
	{
		WriteRegister(	client,Suspend[i].Reg,
				Suspend[i].Data1,Suspend[i].Data2,
				Suspend[i].No);
	}
	mdelay(50);

	for(i=0;i<sizeof(Resume)/sizeof(Resume[0]);i++)
	{
		WriteRegister(	client,Resume[i].Reg,
				Resume[i].Data1,Resume[i].Data2,
				Resume[i].No);
	}
	mdelay(20);
}

void byd693x_Write_All_Config(struct i2c_client *client)
{
	struct sCongig_Para *lp_Config_Tab;
	int l_byd_update_flag;
	
	l_byd_update_flag = 0;
	
	lp_Config_Tab = &(gs_HW_Config_Table);
	l_byd_update_flag |= Write_CFG_Para(client,lp_Config_Tab);
	mdelay(2);
	
	lp_Config_Tab = &(gs_RXSEQ_Config_Table);
	l_byd_update_flag |= Write_CFG_Para(client,lp_Config_Tab);
	mdelay(2);

	lp_Config_Tab = &(gs_TXSEQ_Config_Table);
	l_byd_update_flag |= Write_CFG_Para(client,lp_Config_Tab);
	mdelay(2);

	lp_Config_Tab = &(gs_Adapt_Config_Table);
	l_byd_update_flag |= Write_CFG_Para(client,lp_Config_Tab);
	mdelay(2);

	lp_Config_Tab = &(gs_SW_Config_Table);
	l_byd_update_flag |= Write_CFG_Para(client,lp_Config_Tab);
	mdelay(2);
	if (1 == l_byd_update_flag)
		{
			WriteRegister(client,BYD_WRITE_CFG_OK,0x00,0x00,1);		//有一个参数需要更新，则写更新OK命令
		}
}


static int ctp_get_system_config(void)
{   
        ctp_print_info(config_info,DEBUG_INIT);
        twi_id = config_info.twi_id;
        SCREEN_MAX_X = config_info.screen_max_x;
        SCREEN_MAX_Y = config_info.screen_max_y;
        REVERT_X_FLAG= config_info.revert_x_flag;
        REVERT_Y_FLAG = config_info.revert_y_flag;
        EXCHANGE_X_Y_FLAG = config_info.exchange_x_y_flag;
        if((twi_id == 0) || (SCREEN_MAX_X == 0) || (SCREEN_MAX_Y == 0)){
                printk("%s:read config error!\n",__func__);
                return 0;
        }
        return 1;
}

static int inet_Judge_Chip_IIC_Address(struct i2c_client *client)
{
	unsigned char Buf[2];
	int ret = 0;

	ret = ReadRegister(client,0x07,Buf,1);		//read only one byte
	if (ret)		return (ret);
		
	ret = ReadRegister(client,0x07,Buf,1);		//read only one byte
	if (ret)		return (ret);

	ret = ReadRegister(client,0x07,Buf,1);		//read only one byte
	if (ret)		return (ret);

	return (ret);
}


int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	
	//pr_info("byd693x_ctp_detect\n");
	 // int ret = 0, i = 0;
       
   if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
       return -ENODEV;
	
   if(twi_id == adapter->nr)
	 {
	    if(inet_Judge_Chip_IIC_Address(client)!=0)
	    {
					printk("%s: Detected chip %s at adapter %d, address 0x%02x\n",
					 __func__, CTP_NAME, i2c_adapter_id(adapter), client->addr);

					m_inet_ctpState=1;
					printk("%s",BYD693X_USED);
	
					strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
					return 0;
	    }
			else
			{
		    return -ENODEV;
		    printk("===byd693x_ts_probe: i2c Address: 0x%2X  CTP is not connect!\n",client->addr);
			}
	}
		   	
		 else
	{
		return -ENODEV;
	}
}

void deviceResume(struct i2c_client *client)
{	
	int i;

	for(i=0;i<sizeof(Resume)/sizeof(Resume[0]);i++)
	{
		WriteRegister(	client,Resume[i].Reg,
				Resume[i].Data1,Resume[i].Data2,
				Resume[i].No);
	}
	mdelay(20);
	//Write Config parameter to CTP
		if (2048 == g_Default_Resolution_X)
	{
		byd693x_Write_All_Config(client);
	}
	/*
	for(i=0;i<sizeof(byd693xcfg_Resolution)/sizeof(byd693xcfg_Resolution[0]);i++)
	{
		WriteRegister(	client,byd693xcfg_Resolution[i].Reg,
				byd693xcfg_Resolution[i].Data1,byd693xcfg_Resolution[i].Data2,
				byd693xcfg_Resolution[i].No);
	}
	mdelay(20);
	*/
}

void deviceSuspend(struct i2c_client *client)
{	
	int i;
	
	for(i=0;i<sizeof(Suspend)/sizeof(Suspend[0]);i++)
	{
		WriteRegister(	client,Suspend[i].Reg,
				Suspend[i].Data1,Suspend[i].Data2,
				Suspend[i].No);
	}
	mdelay(50);
}


static int Read_Resolution(struct i2c_client *client,unsigned char Register_Addr)
{
	unsigned char Buf[2];
	int ret = 0,Temp_resolution = 0;

	ret = ReadRegister(client,Register_Addr,Buf,2);		//read Two byte
	if (ret)
		{
			Temp_resolution = 256 * Buf[0] + Buf[1];
		}
	return Temp_resolution;
}

static void bf693x_ts_send_keyevent(struct byd_ts_priv *BYD_priv,u8 btn_status)
{
	
	switch(btn_status & 0xf0)
	{
		case 0x90:
			BYD_priv->btn_pre_TPKey = TPKey_code[0];
			break;
		case 0xa0:
			BYD_priv->btn_pre_TPKey = TPKey_code[1];
			break;
		case 0xb0:
			BYD_priv->btn_pre_TPKey = TPKey_code[2];
			break;
		case 0xc0:
			BYD_priv->btn_pre_TPKey = TPKey_code[3];
			break;
		case 0xf0:
			input_report_key(BYD_priv->input, BYD_priv->btn_pre_TPKey, REPORT_TPKEY_UP);
			input_sync(BYD_priv->input);
			return;
		default:
			return;
	}
	input_report_key(BYD_priv->input, BYD_priv->btn_pre_TPKey, REPORT_TPKEY_DOWN);
	input_sync(BYD_priv->input);
}
	
static void byd693x_ts_work(struct work_struct *work)
{
	int i;
	unsigned short xpos=0, ypos=0;
	unsigned char Coord_Buf[4*FINGER_NO_MAX +1];		//Define the max finger data
	u8 btn_status;
	u8 Finger_ID,Finger_Status,Report_Status;

	struct byd_ts_priv *BYD_priv = container_of(work,struct byd_ts_priv,BYD_work);

	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	byd693x_ts_work!                  |\n");
	printk("+-----------------------------------------+\n");
	#endif
	//if (BYD_priv->suspend_opend == 1)
		//return ;

	if (!g_Default_Resolution_X)
		{
			g_Default_Resolution_X = Read_Resolution(BYD_priv->client,BYD_RW_RESO_X);		//Read CTP Default Resolution X
			g_Default_Resolution_Y = Read_Resolution(BYD_priv->client,BYD_RW_RESO_Y);		//Read CTP Default Resolution Y
		}
		
	ReadRegister(BYD_priv->client,BYD_COORD_READ_ADDR,Coord_Buf,(4 * Get_Finger_Num +1));		//read only the used finger number data
		
	btn_status = Coord_Buf[0];
	#ifdef CONFIG_TS_COORDIATE_DEBUG
					printk("btn_status is: 0x%x\n",btn_status);
	#endif
	
	if ( 0x00 == (btn_status & 0x80))
	{

		if(0x00 == btn_status)
		Report_Status = 0;
		{
		     for(i=0;i<Get_Finger_Num ;i++)
				 {
						Finger_ID = (Coord_Buf[i*4 + 1]>>4)-1;
						Finger_Status = Coord_Buf[i*4 + 3] & 0xf0;
						if((Finger_ID>=0&&Finger_ID<=4)&&Finger_Status == 0xc0)
						{
								Report_Status = 1;
								input_report_abs(BYD_priv->input, ABS_MT_TRACKING_ID, Finger_ID);
								input_report_abs(BYD_priv->input, ABS_MT_TOUCH_MAJOR, 0);
								input_mt_sync(BYD_priv->input);
									#ifdef CONFIG_TS_COORDIATE_DEBUG
									printk("0x00==(btn_status & 0x80)Finger Touch X = %d , Y = %d, State = 0x%x,Finger_ID=0x%x\n\n",xpos,ypos,Finger_Status,Finger_ID);
								#endif
						}

					}
		         
		     if (Report_Status)
		     {
		        input_sync(BYD_priv->input);
		     }
		}
			

		return;	
	}
	bf693x_ts_send_keyevent(BYD_priv,btn_status);

	BYD_priv->FingerDetect=0;
	Report_Status = 0;
	if ((btn_status & 0x0f))
		{
			for(i=0;i< (btn_status & 0x0f);i++)
			{
				Finger_ID = (Coord_Buf[i*4 + 1]>>4)-1;
				Finger_Status = Coord_Buf[i*4 + 3] & 0xf0;
				xpos = Coord_Buf[i*4 + 1] & 0x0f;
				xpos = (xpos <<8) | Coord_Buf[i*4 + 2];
				
				ypos = Coord_Buf[i*4 + 3] & 0x0f;
				ypos = (ypos <<8) | Coord_Buf[i*4 + 4];

			if(1!=LANDSCAPE_FLAG)
				{
                                        xpos= ypos*SCREEN_MAX_Y/g_Default_Resolution_X;
					ypos= xpos*SCREEN_MAX_X/g_Default_Resolution_Y;					
				
				}
			else
				{
	   				xpos = xpos * SCREEN_MAX_X/ g_Default_Resolution_X;
	   				ypos = ypos * SCREEN_MAX_Y/ g_Default_Resolution_Y;
				}
				
		

				if((0xa0 == Finger_Status) || (0x90 == Finger_Status))		//0xa0:The first Touch;  0x90: Hold Finger Touch
					{
						BYD_priv->FingerDetect++;
						Report_Status = 1;
						input_report_abs(BYD_priv->input, ABS_MT_TRACKING_ID, Finger_ID);  
						input_report_abs(BYD_priv->input, ABS_MT_TOUCH_MAJOR, REPORT_TOUCH_MAJOR);
					if (CONFIG_LANDSCAPE_SCREEN)
						{
						input_report_abs(BYD_priv->input, ABS_MT_POSITION_X, xpos);
						input_report_abs(BYD_priv->input, ABS_MT_POSITION_Y, ypos);
						}
						else
						{
						input_report_abs(BYD_priv->input, ABS_MT_POSITION_X, ((SCREEN_MAX_X- ypos)));
						input_report_abs(BYD_priv->input, ABS_MT_POSITION_Y, (xpos));
						}
			
						input_report_abs(BYD_priv->input, ABS_MT_WIDTH_MAJOR, REPORT_WIDTH_MAJOR);
						input_mt_sync(BYD_priv->input);
			
						#ifdef CONFIG_TS_COORDIATE_DEBUG
							printk("  Finger Touch X = %d , Y = %d, State = 0x%x,Finger_ID=0x%x\n\n",xpos,ypos,Finger_Status,Finger_ID);
						#endif
					}
				else if (Finger_Status == 0xc0)
					{
						Report_Status = 1;
						input_report_abs(BYD_priv->input, ABS_MT_TRACKING_ID, Finger_ID);
						input_report_abs(BYD_priv->input, ABS_MT_TOUCH_MAJOR, 0);
						input_mt_sync(BYD_priv->input);
						#ifdef CONFIG_TS_COORDIATE_DEBUG
							printk("	Touch release  X = %d , Y = %d, State = 0x%x,Finger_ID=0x%x\n\n",xpos,ypos,Finger_Status,Finger_ID);
						#endif
					}
			}
		}
	if (Report_Status)
		{
			input_sync(BYD_priv->input);
		}
}

static u32 byd693x_ts_isr(struct byd_ts_priv *BYD_priv)
{
	
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	byd693x_ts_isr!                   |\n");
	printk("+-----------------------------------------+\n");
	#endif	
	//disable_irq_nosync(BYD_priv->irq);             //
	queue_work(byd693x_wq, &BYD_priv->BYD_work);
	
	return  0;	

}

static int byd693x_ts_probe(struct i2c_client *client,const struct i2c_device_id *idp)
{	
	struct i2c_dev *i2c_dev;
	struct byd_ts_priv *BYD_priv;
	int err = -1;//,i;
  BYD_input = NULL;
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
		printk("+-----------------------------------------+\n");
		printk("|	byd693x_ts_probe!                 |\n");
		printk("+-----------------------------------------+\n");
	#endif
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		byd693x_ts_probe: need I2C_FUNC_I2C\n");
		#endif
		return -ENODEV;
	}
	else
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		byd693x_ts_probe: i2c Check OK!\n");
			printk("		byd693x_ts_probe: i2c_client name : %s\n",client->name);
		#endif
	}

	BYD_priv = kzalloc(sizeof(*BYD_priv), GFP_KERNEL);
	if (!BYD_priv)
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		byd693x_ts_probe: kzalloc err!\n");
		#endif
		err=-ENODEV;
		dev_set_drvdata(&client->dev, NULL);
		return err;
	}
	else
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		byd693x_ts_probe: kzalloc OK!\n");
		#endif
	}

        this_client = client;
       dev_set_drvdata(&client->dev, BYD_priv);
	/*
	gpio_wakeup_hdle = gpio_request_ex("ctp_para", "ctp_wakeup");
	if(!gpio_wakeup_hdle) {
		pr_warning("touch panel tp_wakeup request gpio fail!\n");
		goto err_ioremap_failed;
	}	
	
	
	gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup");
	mdelay(5);
	gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "ctp_wakeup");	
  	mdelay(5);	
	gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup");	
  	mdelay(30);	
  */
  ctp_wakeup_inet(1,5);
  ctp_wakeup_inet(0,5);
  ctp_wakeup_inet(1,60);
	
  
	
	INIT_WORK(&BYD_priv->BYD_work, byd693x_ts_work);
	//byd693x_wq = create_singlethread_workqueue("byd693x_wq");
	//byd693x_wq = create_singlethread_workqueue(dev_name(&client->dev));
	byd693x_wq = create_workqueue(dev_name(&client->dev));
	if (!byd693x_wq)
	{
		#ifdef CONFIG_TS_WORKQUEUE_DEBUG
		printk("		byd693x_ts_init: create_singlethread_workqueue Error!\n");
		#endif
		goto exit_create_singlethread;
	}
	else
	{
		#ifdef CONFIG_TS_WORKQUEUE_DEBUG
		printk("		byd693x_ts_init: create_singlethread_workqueue OK!\n");
		#endif
	}
	
	BYD_input = input_allocate_device();
	if (!BYD_input)
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		byd693x_ts_probe: input_allocate_device err\n");
		#endif
		err=-ENODEV;
		goto	exit_input_dev_alloc_failed;
	}
	else
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		byd693x_ts_probe: input_allocate_device OK\n");
		#endif
	}

	BYD_priv->input = BYD_input;

	BYD_input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) | BIT_MASK(EV_SYN) ;
	//BYD_input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) | BIT_MASK(BTN_2);
	BYD_input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);//gongpiqiang modify,if have BTN_2,some game can not response touch
	BYD_input->name = client->name;
	BYD_input->id.bustype = BUS_I2C;
	BYD_input->id.vendor  = 0x2878; // Modify for Vendor ID
	BYD_input->dev.parent = &client->dev;
//	BYD_input->open = byd693x_ts_open;
//	BYD_input->close = byd693x_ts_close;
	input_set_drvdata(BYD_input, BYD_priv);
	BYD_priv->client = client;
	BYD_priv->irq = CTP_IRQ_NUMBER;
	irq_handler=BYD_priv->irq;
  BYD_priv->FingerNo=Get_Finger_Num;
	suspend_opend = 0;

	printk("BYD Touchscreen I2C Address: 0x%02X\n",client->addr);
//	printk("BYD Touchscreen Device ID  : BF6932\n");
	
	//byd693xdeviceInit(client);
	//ctp_wakeup_inet(0,50);
	//ctp_wakeup_inet(1,20);
	//printk("byd693xdeviceInit(client) ok\n");
	//ctp_wakeup(1,20);

	
	g_Default_Resolution_X = Read_Resolution(client,BYD_RW_RESO_X);		//Read CTP Default Resolution X
	printk("g_Default_Resolution_X=%d\n",g_Default_Resolution_X);
	g_Default_Resolution_Y = Read_Resolution(client,BYD_RW_RESO_Y);		//Read CTP Default Resolution Y
	printk("g_Default_Resolution_Y=%d\n",g_Default_Resolution_Y);
	
	if (2048 == g_Default_Resolution_X)
		{
			ReadRegister(client,BYD_Read_CTP_TYPE,g_Default_CTP_Type,64);		//read 64 byte CTP Infomation
			gp_Default_CTP_sInfo = (struct sTYPEID_Info *) (g_Default_CTP_Type);
			printk("Raysens CTP Type:%s\n",gp_Default_CTP_sInfo->sTYPEID_Type_Str);
			printk("Raysens CTP FPC Type:%s\n",gp_Default_CTP_sInfo->sTYPEID_FPC_Str);
			printk("Raysens CTP Chip Type:%s\n",gp_Default_CTP_sInfo->sTYPEID_Chip_Str);
			printk("Raysens CTP Version:%s\n",gp_Default_CTP_sInfo->sTYPEID_Ver_Str);
			//Write Config parameter to CTP
			byd693x_Write_All_Config(client);
		}
	
	BYD_priv->FingerNo=Get_Finger_Num;
	
	input_set_abs_params(BYD_input, ABS_MT_TRACKING_ID, 0,MAX_TRACKID_ITEM, 0, 0);
	input_set_abs_params(BYD_input, ABS_MT_TOUCH_MAJOR, 0, MAX_TOUCH_MAJOR, 0, 0);
	input_set_abs_params(BYD_input, ABS_MT_WIDTH_MAJOR, 0, MAX_WIDTH_MAJOR, 0, 0);
	input_set_abs_params(BYD_input, ABS_MT_POSITION_X,  0,SCREEN_MAX_X + 1, 0, 0);
	input_set_abs_params(BYD_input, ABS_MT_POSITION_Y,  0,SCREEN_MAX_Y + 1, 0, 0);

#ifdef USE_TOUCH_KEY
	set_bit(KEY_MENU, BYD_input->keybit);
	set_bit(KEY_HOME, BYD_input->keybit);
	set_bit(KEY_BACK, BYD_input->keybit);
	set_bit(KEY_SEARCH, BYD_input->keybit);
#endif
	

	err = input_register_device(BYD_input);
	if(err)
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		byd693x_ts_probe: input_register_device input err!\n");
		#endif
		err=-ENODEV;
		goto	exit_input_register_device_failed;
	}
	else
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		byd693x_ts_probe: input_register_device input OK!\n");
		#endif
	}

	
#ifdef	CONFIG_HAS_EARLYSUSPEND
	BYD_priv->early_suspend.suspend = byd693x_ts_early_suspend;
	BYD_priv->early_suspend.resume  = byd693x_ts_late_resume;
	BYD_priv->early_suspend.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN+1;
	//BYD_priv->early_suspend.level   =  EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	register_early_suspend(&BYD_priv->early_suspend);
#endif 
{
		
		int_handle=sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)byd693x_ts_isr,BYD_priv);
		if(!int_handle)
		{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		byd693x_ts_probe: request_irq err!\n");
		#endif
			err=-ENODEV;
			goto exit_irq_request_failed;
		}
		else
		{
			
			
		
			#ifdef CONFIG_TS_PROBE_DEBUG
				printk("		byd693x_ts_probe: request_irq OK!\n");
			#endif
		}	
	}
	return 0;

exit_irq_request_failed:
	cancel_work_sync(&BYD_priv->BYD_work);
	destroy_workqueue(byd693x_wq);
exit_input_register_device_failed:
	input_free_device(BYD_input);
exit_input_dev_alloc_failed:
	sw_gpio_irq_free(int_handle);
exit_create_singlethread:
	dev_set_drvdata(&client->dev, NULL);
	kfree(BYD_priv);
exit_alloc_data_failed:
exit_check_functionality_failed:
        
	return err;
}

/*
static int byd693x_ts_open(struct input_dev *dev)
{
	struct byd_ts_priv *BYD_priv = input_get_drvdata(dev);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	byd693x_ts_open!                  |\n");
	printk("+-----------------------------------------+\n");
	#endif	
	deviceResume(BYD_priv->client);
	enable_irq(BYD_priv->irq);
	BYD_priv->suspend_opend = 0;
	return 0;
}


static void byd693x_ts_close(struct input_dev *dev)
{
	struct byd_ts_priv *BYD_priv = input_get_drvdata(dev);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	byd693x_ts_close!                 |\n");
	printk("+-----------------------------------------+\n");
	#endif
	deviceSuspend(BYD_priv->client);	
	BYD_priv->suspend_opend = 1;
	disable_irq(BYD_priv->irq);
}
*/
static int byd693x_ts_resume(struct i2c_client *client)
{
	//struct byd_ts_priv *BYD_priv = dev_get_drvdata(&client->dev);
	
	//struct byd_ts_priv *BYD_priv = dev_get_drvdata(&client->dev);
	int l_Finger_i;

	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	byd693x_ts_resume!                |\n");
	printk("+-----------------------------------------+\n");
	#endif
	/*
         	for (l_Finger_i=0;l_Finger_i<Get_Finger_Num;l_Finger_i++)
		{
						input_report_abs(BYD_input, ABS_MT_TRACKING_ID, l_Finger_i);
						input_report_abs(BYD_input, ABS_MT_TOUCH_MAJOR, REPORT_TOUCH_MAJOR);
						input_report_abs(BYD_input, ABS_MT_POSITION_X, (10+5*l_Finger_i));
						input_report_abs(BYD_input, ABS_MT_POSITION_Y, (10+5*l_Finger_i));
						input_report_abs(BYD_input, ABS_MT_WIDTH_MAJOR, REPORT_TOUCH_MAJOR);
						input_mt_sync(BYD_input);
		}
	mdelay(10);
	*/
	//Charles Add, When Resume,some report dot is un-released	; 12-12-10
	for (l_Finger_i=0;l_Finger_i<Get_Finger_Num;l_Finger_i++)
		{
						input_report_abs(BYD_input, ABS_MT_TRACKING_ID, l_Finger_i);
						input_report_abs(BYD_input, ABS_MT_TOUCH_MAJOR, 0);
						input_mt_sync(BYD_input);
		}
		input_sync(BYD_input);

	
	//ctp_wakeup_inet(0,20);
	deviceResume(client);	
	suspend_opend = 0;		
	enable_irq(irq_handler);
	//sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
	
	return 0;
}

static int byd693x_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	
	
	//struct byd_ts_priv *BYD_priv = dev_get_drvdata(this_client);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	byd693x_ts_suspend!               |\n");
	printk("+-----------------------------------------+\n");
	#endif
   suspend_opend = 1;
	disable_irq(irq_handler);
	//sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
	 //cancel_work_sync(&BYD_priv->BYD_work);
	 //flush_workqueue(byd693x_wq);
	deviceSuspend(client);
	//ctp_wakeup_inet(0,50);

	return 0;
}

#ifdef	CONFIG_HAS_EARLYSUSPEND
static void byd693x_ts_late_resume(struct early_suspend *h)
{
	struct byd_ts_priv *BYD_priv = container_of(h, struct byd_ts_priv, early_suspend);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	byd693x_ts_late_resume!           |\n");
	printk("+-----------------------------------------+\n");
	#endif
	byd693x_ts_resume(BYD_priv->client);
	
}
static void byd693x_ts_early_suspend(struct early_suspend *h)
{
	struct byd_ts_priv *BYD_priv = container_of(h, struct byd_ts_priv, early_suspend);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	byd693x_ts_early_suspend!         |\n");
	printk("+-----------------------------------------+\n");
	#endif
	printk("byd693x_ts_early_suspend begin\n");
	byd693x_ts_suspend(BYD_priv->client, PMSG_SUSPEND);
	printk("byd693x_ts_early_suspend end\n");
}

#endif

static int byd693x_ts_remove(struct i2c_client *client)
{
	struct byd_ts_priv *BYD_priv = i2c_get_clientdata(client);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	byd693x_ts_remove !               |\n");
	printk("+-----------------------------------------+\n");
	#endif
	free_irq(BYD_priv->irq, BYD_priv);
	//sw_gpio_irq_free(int_handle);
	input_unregister_device(BYD_input);
	input_free_device(BYD_input);
	cancel_work_sync(&BYD_priv->BYD_work);
	//destroy_workqueue(byd693x_wq);
	kfree(BYD_priv);
	i2c_set_clientdata(client, NULL);
	//ctp_free_platform_resource();
	return 0;
}





static const struct i2c_device_id byd693x_ts_id[] = {
	{ CTP_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, byd693x_ts_id);

static struct i2c_driver byd693x_ts_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = byd693x_ts_probe,
	.remove = byd693x_ts_remove,
#ifndef	CONFIG_HAS_EARLYSUSPEND
	.suspend = byd693x_ts_suspend,
	.resume = byd693x_ts_resume,
#endif

	.driver = {
		.name = CTP_NAME,
		.owner = THIS_MODULE,
	},
	.id_table = byd693x_ts_id,
	.address_list	= u_i2c_addr.normal_i2c,
};

static char banner[] __initdata = KERN_INFO "BYD Touchscreen driver, (c) 2012 BYD Systech Ltd.\n";
static int __init byd693x_ts_init(void)
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
	
	printk("%s\n",BYD693X_IC_INFO);
	
	printk("[FUN]%s\n", __func__);
	
	int ret;

	if(!ctp_get_system_config())
	{
		printk("not used or capture config error");
		return -ENODEV;
	}

	u_i2c_addr.dirty_addr_buf[0] = BYD693X_I2C_ADDR;
	u_i2c_addr.dirty_addr_buf[1] = I2C_CLIENT_END;

	//ctp_wakeup_inet(0,5);
	//ctp_wakeup_inet(1,5);
	
	byd693x_ts_driver.detect = ctp_detect;
	ret=i2c_add_driver(&byd693x_ts_driver);
	#ifdef CONFIG_TS_I2C_TRANSFER_DEBUG
	if(ret) printk("		byd693x_ts_init: i2c_add_driver Error! \n");
	else    printk("		byd693x_ts_init: i2c_add_driver OK! \n");
	#endif
	return ret;
}

static void __exit byd693x_ts_exit(void)
{
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	byd693x_ts_exit!                  |\n");
	printk("+-----------------------------------------+\n");
	#endif
	i2c_del_driver(&byd693x_ts_driver);
	if (byd693x_wq) destroy_workqueue(byd693x_wq);
}

module_init(byd693x_ts_init);
module_exit(byd693x_ts_exit);

MODULE_AUTHOR("BYD Systech Ltd - Raysens Design Technology, Charles Chen.");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("byd693x Touchscreen Driver 1.5_Charles@Raysens@20130122");
