/*
mbgalex@163.com 2013-05-30_16:2
修复部分APK 不能用的功能

2013-06-11_14:18
fex add 1050 for RS10F118_V1.0

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
//#include "ctp_platform_ops.h"

#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/ctp.h>

#define MISC_DEV

#ifdef MISC_DEV
#include <linux/fs.h>
#include <linux/uaccess.h> 
#include <linux/delay.h>  
#include <linux/miscdevice.h>   

#define TP_CHR "tp_chr"

static long tp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int tp_open(struct inode *inode, struct file *file);
static int tp_release(struct inode *inode, struct file *file);
static ssize_t tp_read(struct file *file, char __user *buf, size_t count,
		loff_t *offset);
static ssize_t tp_write(struct file *file, const char __user *buf,
		size_t count, loff_t *offset);

struct i2c_client *g_tp_client;
#endif

//#define	CONFIG_TS_FUNCTION_CALLED_DEBUG			//Display the debug information whitch function is called
//#define CONFIG_TS_PROBE_DEBUG		//Display the debug information in ssd253x_ts_probe function
//#define CONFIG_TS_I2C_TRANSFER_DEBUG		//Display the debug information of IIC transfer
//#define CONFIG_TPKEY_STATUS_DEBUG			//Display the debug information of Touch Key status
//#define CONFIG_TS_WORKQUEUE_DEBUG		//Display the debug ihnformation of creating work queue
//#define CONFIG_TS_COORDIATE_DEBUG		//
//#define CONFIG_TS_CUTEDGE_DEBUG			//

#define DEVICE_ID_REG                    2
#define VERSION_ID_REG                 3
#define AUTO_INIT_RST_REG             68
#define EVENT_STATUS                   121
#define EVENT_MSK_REG                 122
#define IRQ_MSK_REG                     123
#define FINGER01_REG                    124
#define EVENT_STACK                   	 128
#define EVENT_FIFO_SCLR               135
#define TIMESTAMP_REG                 136
#define SELFCAP_STATUS_REG         185		





static struct i2c_client *this_client;
extern struct ctp_config_info config_info;       


#define CTP_IRQ_NUMBER                  (config_info.irq_gpio_number)
#define CTP_IRQ_MODE					(TRIG_EDGE_NEGATIVE)

static void* __iomem gpio_addr = NULL;
static int gpio_int_hdle = 0;
static int gpio_wakeup_hdle = 0;
//----------------------------------------//
struct ChipSetting {
	char No;
	char Reg;
	char Data1;
	char Data2;
};

#include "ssd253x-ts.h"

extern struct ctp_config_info config_info;

#define SSD253X_USED     "\n \
												 \n+++++++++++++++++++++++++++++++++ \
                         \n++++++   SSD253X used   +++++++++ \
                         \n+++++++++++++++++++++++++++++++++ \
                         \n"
                               
#define SSD253X_IC_INFO  "\n============================================================== \
											   \nIC     :SSD253X \
                         \nAUTHOR :mbgalex@163.com \
                         \nVERSION:2013-06-11_14:18\n"
                         

extern int m_inet_ctpState;
#define CTP_NAME	"sis925x_ts"

/* Addresses to scan */
static union{
	unsigned short dirty_addr_buf[2];
	const unsigned short normal_i2c[2];
}u_i2c_addr = {{0x00},};


//#ifdef USE_TOUCH_KEY
static u8 btn_status_last = 0;
//#endif

static u8 SSD_Touch_Key_Flag = 0;
static u8 SSD_Cut_Edge_Flag = 1;
static unsigned short XPOS_MAX	= 896;		//(15-1)*64
static unsigned short YPOS_MAX = 576;		//(9-1)*64

static u8 CONFIG_LANDSCAPE_SCREEN = 1;

static int device_id = 0;
static int mSSD253xDriverType=0; 
static __u32 twi_id = 0;
static int screen_max_x = 800;
static int screen_max_y = 480;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;

static u8 Get_Finger_Num = 5;

void deviceReset(struct i2c_client *client);
void deviceResume(struct i2c_client *client);
void deviceSuspend(struct i2c_client *client);
void SSD253xdeviceInit(struct i2c_client *client); 

static int ssd253x_ts_open(struct input_dev *dev);
static void ssd253x_ts_close(struct input_dev *dev);
static int ssd253x_ts_suspend(struct i2c_client *client, pm_message_t mesg);
static int ssd253x_ts_resume(struct i2c_client *client);
#ifdef CONFIG_HAS_EARLYSUSPEND
static void ssd253x_ts_early_suspend(struct early_suspend *h);
static void ssd253x_ts_late_resume(struct early_suspend *h);
#endif /* CONFIG_HAS_EARLYSUSPEND */

static enum hrtimer_restart ssd253x_ts_timer(struct hrtimer *timer);
//static irqreturn_t ssd253x_ts_isr(int irq, void *dev_id);


static struct workqueue_struct *ssd253x_wq;

int Ssd_record,Ssd_current,Ssd_Timer_flag;		//Change by Charles		//120613

struct ssl_ts_priv {
	struct i2c_client *client;
	struct input_dev *input;
	struct hrtimer timer;
	struct work_struct  ssl_work;
//#ifdef USE_CALKEY_PROC
	struct work_struct calkey_work;
	struct timer_list calkey_timer;
//#endif
#ifdef	CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif 

	int irq;
	int use_irq;
	int FingerNo;
	int FingerX[FINGER_NO_MAX];
	int FingerY[FINGER_NO_MAX];
	int FingerP[FINGER_NO_MAX];

	int Resolution;
	int EventStatus;
	int FingerDetect;

	int sFingerX[FINGER_NO_MAX];
	int sFingerY[FINGER_NO_MAX];
	int pFingerX[FINGER_NO_MAX];
	int pFingerY[FINGER_NO_MAX];
	int suspend_opend;
};

static struct ssl_ts_priv *ssl_priv;
static struct input_dev *ssl_input=NULL;
static int irq_handler;

static u32 ssd253x_ts_isr(struct ssl_ts_priv *ssl_priv);

//#ifdef USE_CALKEY_PROC
//===============key CAL define ==============================//
static struct workqueue_struct *calkey_workq;
//static struct work_struct calkey_work;
static unsigned short base_1,base_2,base_3,base_4;
static int timer_status = 0;
//=================================================//
#define CAL_RANGE (0x50)
//#endif

/***********************************************************
Read Data from TP through IIC
***********************************************************/
int ReadRegister(struct i2c_client *client,uint8_t reg,int ByteNo)
{
	unsigned char buf[4];
	struct i2c_msg msg[2];
	int ret;

	memset(buf, 0xFF, sizeof(buf));
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &reg;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = ByteNo;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);

	#ifdef CONFIG_TS_I2C_TRANSFER_DEBUG
	if(ret<0)	printk("		ReadRegister: i2c_transfer Error !\n");
	else		printk("		ReadRegister: i2c_transfer OK !\n");
	#endif

	if(ByteNo==1) return (int)((unsigned int)buf[0]<<0);
	if(ByteNo==2) return (int)((unsigned int)buf[1]<<0)|((unsigned int)buf[0]<<8);
	if(ByteNo==3) return (int)((unsigned int)buf[2]<<0)|((unsigned int)buf[1]<<8)|((unsigned int)buf[0]<<16);
	if(ByteNo==4) return (int)((unsigned int)buf[3]<<0)|((unsigned int)buf[2]<<8)|((unsigned int)buf[1]<<16)|(buf[0]<<24);
	return 0;
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

//#ifdef USE_CALKEY_PROC
static int read_key_base(struct i2c_client *client,unsigned char fir_nor,unsigned short buf[4])
{
	int ret = 1;
	WriteRegister(client,0xBD,fir_nor,0x00,1);

	buf[0] = ReadRegister(client,0xB5,2);
	buf[1] = ReadRegister(client,0xB6,2);
	buf[2] = ReadRegister(client,0xB7,2);
	buf[3] = ReadRegister(client,0xB8,2);

	return ret;		
}
//#endif



void SSD253xdeviceInit(struct i2c_client *client)
{	
int i;


switch(mSSD253xDriverType)
{
	case 1050:    //10.1 Inch RS10M256G G+G平贴TP，不带触摸按键
		{
				printk("ssd 2533QN10 For Raysens 256G 10.1 Inch driver\n");
				for(i=0;i<sizeof(ssd2533QN10_RS256G_M10F_cfg)/sizeof(ssd2533QN10_RS256G_M10F_cfg[0]);i++)
				{
					WriteRegister(	client,ssd2533QN10_RS256G_M10F_cfg[i].Reg,ssd2533QN10_RS256G_M10F_cfg[i].Data1,ssd2533QN10_RS256G_M10F_cfg[i].Data2,ssd2533QN10_RS256G_M10F_cfg[i].No);
				}
				SSD_Touch_Key_Flag = 0;		//TP按键
				SSD_Cut_Edge_Flag = 0;		//10.1 inch 23x38，不需要做cut edge
				XPOS_MAX	= 1408;		//(23-1)*64
				YPOS_MAX = 2368;		//(38-1)*64
				CONFIG_LANDSCAPE_SCREEN = 1;
				Get_Finger_Num = 10;
		}
		break;
		
	case 1051:    //10.1 Inch RS10M256G G+G平贴TP，不带触摸按键,专门给国城兴客户使用。
		{
				printk("ssd 2533QN10 For Raysens 256G 10.1 Inch driver\n");
				for(i=0;i<sizeof(ssd2533QN10_RS256G_M10F_gcx_cfg)/sizeof(ssd2533QN10_RS256G_M10F_gcx_cfg[0]);i++)
				{
					WriteRegister(	client,ssd2533QN10_RS256G_M10F_gcx_cfg[i].Reg,ssd2533QN10_RS256G_M10F_gcx_cfg[i].Data1,ssd2533QN10_RS256G_M10F_gcx_cfg[i].Data2,ssd2533QN10_RS256G_M10F_gcx_cfg[i].No);
				}
				SSD_Touch_Key_Flag = 0;		//TP按键
				SSD_Cut_Edge_Flag = 0;		//10.1 inch 23x38，不需要做cut edge
				XPOS_MAX	= 1408;		//(23-1)*64
				YPOS_MAX = 2368;		//(38-1)*64
				CONFIG_LANDSCAPE_SCREEN = 1;
				Get_Finger_Num = 10;
		}
		break;	
	case 1005:

		printk("ssd 2533QN10 For Raysens 099/177 10.1 Inch driver\n");
						for(i=0;i<sizeof(ssd2532QN10_RS99_M1F_cfg)/sizeof(ssd2532QN10_RS99_M1F_cfg[0]);i++)
						{
							WriteRegister(	client,ssd2532QN10_RS99_M1F_cfg[i].Reg,ssd2532QN10_RS99_M1F_cfg[i].Data1,ssd2532QN10_RS99_M1F_cfg[i].Data2,ssd2532QN10_RS99_M1F_cfg[i].No);
						}
						SSD_Touch_Key_Flag = 0;		//TP按键
						SSD_Cut_Edge_Flag = 0;		//10.1 inch 23x38，不需要做cut edge
						XPOS_MAX	= 1408;		//(23-1)*64
						YPOS_MAX = 2368;		//(38-1)*64
						CONFIG_LANDSCAPE_SCREEN = 1;
						Get_Finger_Num = 10;
		break;

	case 9701:
		SSD_Touch_Key_Flag = 0;		//TP按键
				SSD_Cut_Edge_Flag = 0;		//9.7 inch 23x38，不需要做cut edge
				XPOS_MAX	= 1408;		//(23-1)*64
				YPOS_MAX = 2368;		//(38-1)*64
				CONFIG_LANDSCAPE_SCREEN = 1;
				Get_Finger_Num = 10;
				
		printk("ssd2533QN10_M3F_cfg2 \n");
		
							for(i=0;i<sizeof(ssd2533QN10_M3F_cfg2)/sizeof(ssd2533QN10_M3F_cfg2[0]);i++)
							{
								WriteRegister(	client,ssd2533QN10_M3F_cfg2[i].Reg,
								ssd2533QN10_M3F_cfg2[i].Data1,ssd2533QN10_M3F_cfg2[i].Data2,
								ssd2533QN10_M3F_cfg2[i].No);
							}
							

			
	break;
	
	default:
		SSD_Touch_Key_Flag = 0;		//TP按键
				SSD_Cut_Edge_Flag = 0;		//9.7 inch 23x38，不需要做cut edge
				XPOS_MAX	= 1408;		//(23-1)*64
				YPOS_MAX = 2368;		//(38-1)*64
				CONFIG_LANDSCAPE_SCREEN = 1;
				Get_Finger_Num = 10;
				
		printk("ssd2533QN10_M3F_cfg2 \n");
		
							for(i=0;i<sizeof(ssd2533QN10_M3F_cfg2)/sizeof(ssd2533QN10_M3F_cfg2[0]);i++)
							{
								WriteRegister(	client,ssd2533QN10_M3F_cfg2[i].Reg,
								ssd2533QN10_M3F_cfg2[i].Data1,ssd2533QN10_M3F_cfg2[i].Data2,
								ssd2533QN10_M3F_cfg2[i].No);
							}

		break;
		
	
}


mdelay(10);
//Rocky -	
}

void deviceReset(struct i2c_client *client)
{	
	int i;
	for(i=0;i<sizeof(Reset)/sizeof(Reset[0]);i++)
	{
		WriteRegister(	client,Reset[i].Reg,
				Reset[i].Data1,Reset[i].Data2,
				Reset[i].No);
	}
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
	
	//pr_info("ssd253x_ctp_detect\n");
	
	if(twi_id == adapter->nr)
	{
		pr_info("%s: Detected chip %s at adapter %d, address 0x%02x\n",
			 __func__, CTP_NAME, i2c_adapter_id(adapter), client->addr);

		strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
		return 0;
	}else{
		return -ENODEV;
	}
}




static int ctp_get_system_config(void)
{   
		script_item_u   val;
        
       
		if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "mSSD253xDriverType", &val))
		{
			mSSD253xDriverType=0;
		}
		else
		{
			mSSD253xDriverType = val.val;
		}
		//mSSD253xDriverType = 9701;
		printk("mSSD253xDriverType=%d\n",mSSD253xDriverType);
        
        ctp_print_info(config_info,DEBUG_INIT);
        twi_id = config_info.twi_id;
        screen_max_x= config_info.screen_max_x;
        screen_max_y= config_info.screen_max_y;
        revert_x_flag= config_info.revert_x_flag;
        revert_y_flag= config_info.revert_y_flag;
        exchange_x_y_flag= config_info.exchange_x_y_flag;
        
   
        if((twi_id == 0) || (screen_max_x == 0) || (screen_max_y == 0)){
                printk("%s:read config error!\n",__func__);
                return 0;
        }
        return 1;
}


void deviceResume(struct i2c_client *client)
{	
  ctp_wakeup_inet(0,5);
  ctp_wakeup_inet(1,2);
	deviceReset(client);
	mdelay(50);
	SSD253xdeviceInit(client);
}

void deviceSuspend(struct i2c_client *client)
{		
  ctp_wakeup_inet(0,5);
  ctp_wakeup_inet(1,2);
}

#define Mode RunningAverageMode
#define Dist RunningAverageDist
void RunningAverage(unsigned short *xpos,unsigned short *ypos,int No,struct ssl_ts_priv *ssl_priv)
{	
	int FilterMode[4][2]={{0,8},{5,3},{6,2},{7,1}};
	int dx,dy;
	int X,Y;

	X=*xpos;
	Y=*ypos;
	if((ssl_priv->pFingerX[No]!=0x0FFF)&&(X!=0x0FFF))
	{
		dx=abs(ssl_priv->pFingerX[No]-X);
		dy=abs(ssl_priv->pFingerY[No]-Y);
		if(dx+dy<Dist*64)
		{
			ssl_priv->pFingerX[No]=(FilterMode[Mode][0]*ssl_priv->pFingerX[No]+FilterMode[Mode][1]*X)/8;
			ssl_priv->pFingerY[No]=(FilterMode[Mode][0]*ssl_priv->pFingerY[No]+FilterMode[Mode][1]*Y)/8;
		}
		else
		{
			ssl_priv->pFingerX[No]=X;
			ssl_priv->pFingerY[No]=Y;
		}
	}
	else
	{
		ssl_priv->pFingerX[No]=X;
		ssl_priv->pFingerY[No]=Y;
	}
	*xpos=ssl_priv->pFingerX[No];
	*ypos=ssl_priv->pFingerY[No];
}

void FingerCheckSwap(int *FingerX,int *FingerY,int *FingerP,int FingerNo,int *sFingerX,int *sFingerY)
{
  	int i,j;
  	int index1,index2;
  	int Vx,Vy;
  	int Ux,Uy;
  	int R1x,R1y;
  	int R2x,R2y;
	for(i=0;i<FingerNo;i++)
  	{
 		index1=i;
	    	if( FingerX[index1]!=0xFFF)
		if(sFingerX[index1]!=0xFFF) 
		{
			for(j=i+1;j<FingerNo+3;j++)
			{
				index2=j%FingerNo;
	    			if( FingerX[index2]!=0xFFF)
				if(sFingerX[index2]!=0xFFF) 
		    		{
					Ux=sFingerX[index1]-sFingerX[index2];
					Uy=sFingerY[index1]-sFingerY[index2];      
					Vx= FingerX[index1]- FingerX[index2];
					Vy= FingerY[index1]- FingerY[index2];					

					R1x=Ux-Vx;
					R1y=Uy-Vy;
					R2x=Ux+Vx;
					R2y=Uy+Vy;
							
					R1x=R1x*R1x;
					R1y=R1y*R1y; 
					R2x=R2x*R2x;
					R2y=R2y*R2y;

					if(R1x+R1y>R2x+R2y)
				    	{
				    		Ux=FingerX[index1];
						Uy=FingerY[index1];
						Vx=FingerP[index1];
							          
						FingerX[index1]=FingerX[index2];
						FingerY[index1]=FingerY[index2];
						FingerP[index1]=FingerP[index2];
							
						FingerX[index2]=Ux;
						FingerY[index2]=Uy;
						FingerP[index2]=Vx;
					}
					break;
			    	}
			}
		}
  	}        
  	for(i=0;i<FingerNo;i++)
  	{
    		sFingerX[i]=FingerX[i];
    		sFingerY[i]=FingerY[i];
  	}
}

//#ifdef USE_TOUCH_KEY
static void ssd2533_ts_send_keyevent(struct ssl_ts_priv *ssl_priv,u8 btn_status, int downup)
{
	
	switch(btn_status & 0x0f)
	{
		case 0x01:
			input_report_key(ssl_priv->input, TPKey_code[0], downup);
			input_sync(ssl_priv->input);		
			break;
		case 0x02:
			input_report_key(ssl_priv->input, TPKey_code[1], downup);
			input_sync(ssl_priv->input);
			break;
		case 0x04:
			input_report_key(ssl_priv->input, TPKey_code[2], downup);
			input_sync(ssl_priv->input);
			break;
		case 0x08:
			input_report_key(ssl_priv->input, TPKey_code[3], downup);
			input_sync(ssl_priv->input);
			break;
		default:
			break;
	}
	#ifdef CONFIG_TPKEY_STATUS_DEBUG
	printk("send %x %x\n", btn_status, downup);
	#endif
}
//#endif

//#ifdef USE_CUT_EDGE
static int ssd253x_ts_cut_edge(unsigned short pos,unsigned short x_y)
{
	u8 cut_value = 20; //cut_value < 32
	if(pos == 0xfff)
	{
		return pos;
	}
	if(x_y) //xpos
	{
		#ifdef CONFIG_TS_CUTEDGE_DEBUG
			printk("X: Raw data %d\n",pos);
		#endif
		if (pos >=XPOS_MAX)
			{
				pos = XPOS_MAX;
			}
		if (pos<64)
			{
				pos = Cut_Edge_XLeft[pos];		//Left cut edge
			}
		else
		if ((XPOS_MAX - pos) <64)
			{
				pos = Cut_Edge_XRight[XPOS_MAX - pos];		//Right cut edge
			}
		else
			{
				pos = pos + cut_value;		//
				pos = screen_max_x * pos / (XPOS_MAX + cut_value*2);//screen_max_x|ì???¨2¨?¨°?à??|ì
			}
		#ifdef CONFIG_TS_CUTEDGE_DEBUG
			printk("X: Cut edge data %d\n",pos);
		#endif
		return pos;
	}
	else    //ypos
	{

		#ifdef CONFIG_TS_CUTEDGE_DEBUG
			printk("Y: Raw data %d\n",pos);
		#endif
		if (pos >=YPOS_MAX)
			{
				pos = YPOS_MAX;
			}
		if (pos<64)
			{
				pos = Cut_Edge_YUp[pos];		//Up cut edge
			}
		else
		if ((YPOS_MAX - pos) <64)
			{
				pos = Cut_Edge_YDown[YPOS_MAX - pos];		//Down cut edge
			}
		else
			{
				pos = pos + cut_value;		//
				pos = screen_max_y * pos / (YPOS_MAX + cut_value*2);//screen_max_x|ì???¨2¨?¨°?à??|ì
			}
		#ifdef CONFIG_TS_CUTEDGE_DEBUG
			printk("X: Cut edge data %d\n",pos);
		#endif
		return pos;
	}
}

static int ssd253x_ts_HLT_cut_edge(unsigned short pos,unsigned short x_y)
{
	u8 cut_value = 15; //cut_value < 32
	if(pos == 0xfff)
	{
		return pos;
	}
	if(x_y) //xpos
	{
		#ifdef CONFIG_TS_CUTEDGE_DEBUG
			printk("X: Raw data %d\n",pos);
		#endif
		pos = pos + cut_value;//μ÷?ú×ó±??μ
		pos = screen_max_x * pos / (XPOS_MAX + cut_value*2);//screen_max_xμ÷?úóò±??μ

		#ifdef CONFIG_TS_CUTEDGE_DEBUG
			printk("X: Cut edge data %d\n",pos);
		#endif
		return pos;
	}
	else    //ypos
	{

		#ifdef CONFIG_TS_CUTEDGE_DEBUG
			printk("Y: Raw data %d\n",pos);
		#endif
		pos = pos + cut_value;//μ÷?úé?±??μ
		pos = screen_max_y* pos / (YPOS_MAX + cut_value*2);//screen_max_yμ÷?ú??±??μ
		#ifdef CONFIG_TS_CUTEDGE_DEBUG
			printk("X: Cut edge data %d\n",pos);
		#endif
		return pos;
	}
}

//#endif
	
static void ssd253x_ts_work(struct work_struct *work)
{
	int i;
	unsigned short xpos=0, ypos=0, width=0;
	int FingerInfo;
	int EventStatus;
	int FingerX[FINGER_NO_MAX];
	int FingerY[FINGER_NO_MAX];
	int FingerP[FINGER_NO_MAX];
	int clrFlag=0;
	int Ssd_Timer;
//	#ifdef USE_TOUCH_KEY
	u8 btn_status;
//	#endif
	

	struct ssl_ts_priv *ssl_priv = container_of(work,struct ssl_ts_priv,ssl_work);

	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_work!                  |\n");
	printk("+-----------------------------------------+\n");
	#endif

	if (ssl_priv->suspend_opend == 1)
		return ;
	//#ifdef USE_TOUCH_KEY
	if (SSD_Touch_Key_Flag)
	{
			btn_status = ReadRegister(ssl_priv->client,SELFCAP_STATUS_REG, 1);
			btn_status &= 0x0f;
	#ifdef CONFIG_TPKEY_STATUS_DEBUG
			printk("btn pressed:%x btn_status_last%d\n", btn_status & 0x0f ,btn_status_last);
	#endif
		if((ssl_priv->use_irq==HYBRID_INT) && (btn_status!=0))
		{
				hrtimer_start(&ssl_priv->timer, ktime_set(0, TimeoutInterupt), HRTIMER_MODE_REL);		//start the timeout interrupt
		}
		if (btn_status_last != btn_status)
		{
			if(btn_status)
			{
				btn_status_last = btn_status;
				ssd2533_ts_send_keyevent(ssl_priv,btn_status, 1);
//				#ifdef USE_CALKEY_PROC
					timer_status = 1;	//Charles added for touch-key check 2012-03-12
//				#endif
			#ifdef CONFIG_TPKEY_STATUS_DEBUG
				printk("send %x btn_status_last%d \n", btn_status,btn_status_last);
			#endif
			}
			else
			{
				ssd2533_ts_send_keyevent(ssl_priv,btn_status_last, 0);
				btn_status_last = 0;
				#ifdef CONFIG_TPKEY_STATUS_DEBUG
						printk("btn_status_last %x \n", btn_status_last);
				#endif
			}
			return ;
		}
	}
	//#endif
	if(!Ssd_Timer_flag)
	{
		Ssd_Timer = ReadRegister(ssl_priv->client,TIMESTAMP_REG,2);
		if(!Ssd_record)                                      
		{
				Ssd_record = Ssd_Timer/1000;   			
		}
		
		Ssd_current = Ssd_Timer/1000;               
		
		if(Ssd_current < Ssd_record)
       {
       		Ssd_current += 0xffff/1000;
       }
		
		if((Ssd_current - Ssd_record) > 5)
		{
		WriteRegister(ssl_priv->client,AUTO_INIT_RST_REG,0x00,0x00,1);
		Ssd_Timer_flag = 1;
		Ssd_record = 0;
		}
	 }

	EventStatus = ReadRegister(ssl_priv->client,EVENT_STATUS,2)>>4;
	ssl_priv->FingerDetect=0;
	for(i=0;i<ssl_priv->FingerNo;i++)
	{
		if((EventStatus>>i)&0x1)
		{
			FingerInfo=ReadRegister(ssl_priv->client,FINGER01_REG+i,4);
			xpos = ((FingerInfo>>4)&0xF00)|((FingerInfo>>24)&0xFF);
			ypos = ((FingerInfo>>0)&0xF00)|((FingerInfo>>16)&0xFF);
			width= ((FingerInfo>>4)&0x00F);	

			if(xpos!=0xFFF)
			{
				ssl_priv->FingerDetect++;

				if (1 == SSD_Cut_Edge_Flag)
				{
					xpos = ssd253x_ts_cut_edge(xpos, 1);
					ypos = ssd253x_ts_cut_edge(ypos, 0);
				}
				if (2 == SSD_Cut_Edge_Flag)
					{
						xpos = ssd253x_ts_HLT_cut_edge(xpos,1);
						ypos = ssd253x_ts_HLT_cut_edge(ypos,0);
					}
			}
			else 
			{
				// This part is to avoid asyn problem when the finger leaves
				EventStatus=EventStatus&~(1<<i);
				clrFlag=1;
			}
		}
		else
		{
			xpos=ypos=0xFFF;
			width=0;
			clrFlag=1;
		}
		FingerX[i]=xpos;
		FingerY[i]=ypos;
		FingerP[i]=width;
	}
	if((ssl_priv->use_irq==HYBRID_INT) && (ssl_priv->FingerDetect!=0))
	{
			hrtimer_start(&ssl_priv->timer, ktime_set(0, TimeoutInterupt), HRTIMER_MODE_REL);		//start the timeout interrupt
	}
	if(clrFlag) WriteRegister(ssl_priv->client,EVENT_FIFO_SCLR,0x01,0x00,1);

	if(ssl_priv->input->id.product==0x2533)
	if(ssl_priv->input->id.version==0x0101) 
		FingerCheckSwap(FingerX,FingerY,FingerP,ssl_priv->FingerNo,ssl_priv->sFingerX,ssl_priv->sFingerY);

	for(i=0;i<ssl_priv->FingerNo;i++)
	{
		xpos=FingerX[i];
		ypos=FingerY[i];
		width=FingerP[i];
		if(ssl_priv->input->id.product==0x2533)
		{
			if(ssl_priv->input->id.version==0x0101) RunningAverage(&xpos,&ypos,i,ssl_priv);
			if(ssl_priv->input->id.version==0x0102) RunningAverage(&xpos,&ypos,i,ssl_priv);
		}

		if(xpos!=0xFFF)
		{
			input_report_abs(ssl_priv->input, ABS_MT_TRACKING_ID, i);  
			input_report_abs(ssl_priv->input, ABS_MT_TOUCH_MAJOR, REPORT_TOUCH_MAJOR);

			if (CONFIG_LANDSCAPE_SCREEN)
				{
					input_report_abs(ssl_priv->input, ABS_MT_POSITION_X, xpos);
					input_report_abs(ssl_priv->input, ABS_MT_POSITION_Y, ypos);
				}
			else
				{
					input_report_abs(ssl_priv->input, ABS_MT_POSITION_X, ((screen_max_x- ypos)));
					input_report_abs(ssl_priv->input, ABS_MT_POSITION_Y, (xpos));
				}
			input_report_abs(ssl_priv->input, ABS_MT_WIDTH_MAJOR, width);		//REPORT_WIDTH_MAJOR);//width);
			input_mt_sync(ssl_priv->input);

			#ifdef CONFIG_TS_COORDIATE_DEBUG
			if(i==0)
				printk("		ssd253x_ts_work: X = %d , Y = %d, W = 0x%x\n",xpos,ypos,width);
			#endif
		}
		else if(ssl_priv->FingerX[i]!=0xFFF)
		{
			input_report_abs(ssl_priv->input, ABS_MT_TRACKING_ID, i);
			input_report_abs(ssl_priv->input, ABS_MT_TOUCH_MAJOR, 0);
			input_mt_sync(ssl_priv->input);
			#ifdef CONFIG_TS_COORDIATE_DEBUG
			if(i==0) printk("	release	ssd253x_ts_work: X = 0x%x , Y = 0x%x, W = 0x%x\n",xpos,ypos,width);
			#endif
		}
		ssl_priv->FingerX[i]=FingerX[i];
		ssl_priv->FingerY[i]=FingerY[i];
		//ssl_priv->FingerP[i]=width;
	}		
	ssl_priv->EventStatus=EventStatus;	
	input_sync(ssl_priv->input);

}
static void touchbubblesort(unsigned int Number[],unsigned int num)
{
	int i,j;
	unsigned int temp;
	for(i=0 ; i<(int)(num-1) ; i++)
	{
		for(j=i+1;j<(int)num;j++)
		{
			if(Number[j] != 0 &&  Number[i]>Number[j])
			{
				temp   = Number[i];
				Number[i] = Number[j];
				Number[j] = temp;
			}
		}
	}
}

//#ifdef USE_CALKEY_PROC
static void calkey_do_timer(unsigned long data)
{
	struct ssl_ts_priv *ssl_priv = (struct ssl_ts_priv *)data;		
	queue_work(calkey_workq, &ssl_priv->calkey_work);
}

static void calkey_do_work(struct work_struct *work)
{
	unsigned short data[4];
	int ret = 0;
	char buf[2];
	struct ssl_ts_priv *ssl_priv = container_of(work,struct ssl_ts_priv,calkey_work);
		if (ssl_priv->suspend_opend == 1)
		return ;		
	ret = read_key_base(ssl_priv->client,0x00,data);
	//printk("\n key base : %d %d %d %d\n",base_1,base_2,base_3,base_4);
  //printk("\n key data : %d %d %d %d\n",data[0],data[1],data[2],data[3]);  
  if(data[0] < base_1)
  {
		if((base_1 - data[0]) > CAL_RANGE)
    {
			printk("\n cal key base_1 \n");
			buf[1] = (char)(0xff & data[0]);
			buf[0] = (char)(data[0] >> 8);
			WriteRegister(ssl_priv->client,0xb1,buf[0],buf[1],2);
			if(ret < 0)
			 {
					printk("write b1 error!\n");
					goto quit;
				}
          base_1 = data[0];
          timer_status = 1;	//Charles added for touch-key check 2012-03-12
      }
	}
  if(data[1] < base_2)
  {
      if((base_2 - data[1]) > CAL_RANGE) 
      {
          printk("cal key base_2 \n");
          buf[1] = (char)(0xff & data[1]);
          buf[0] = (char)(data[1] >> 8);
					WriteRegister(ssl_priv->client,0xb2,buf[0],buf[1],2);
          if(ret < 0) 
          {
              printk("write b2 error!\n");
              goto quit;
          }
          base_2 = data[1];
          timer_status = 1;	//Charles added for touch-key check 2012-03-12
      }
	}
  if(data[2] < base_3)
  {
		if((base_3 - data[2]) > CAL_RANGE) 
		{
			printk("cal key base_3 \n");
			buf[1] = (char)(0xff & data[2]);
			buf[0] = (char)(data[2] >> 8);
			WriteRegister(ssl_priv->client,0xb3,buf[0],buf[1],2);
			if(ret < 0)
			{
				printk("write b3 error!\n");
				goto quit;
			}
				base_3 = data[2];
				timer_status = 1;	//Charles added for touch-key check 2012-03-12
		}
	}

  if(data[3] < base_4)
  {
      if((base_4 - data[3]) > CAL_RANGE)
      {
          printk("cal key base_4 \n");
          buf[1] = (char)(0xff & data[3]);
          buf[0] = (char)(data[3] >> 8);
					WriteRegister(ssl_priv->client,0xb4,buf[0],buf[1],2);
          if(ret < 0)
          {
              printk("write b4 error!\n");
          }
          base_4 = data[3];
          timer_status = 1;	//Charles added for touch-key check 2012-03-12
      }
		}
quit:
		if(timer_status == 1)//Paul added for touch-key check 2012-01-12
			return;
    mod_timer(&ssl_priv->calkey_timer,jiffies + 200);		//Charles change to 200ms,120313
}
//#endif

static int ssd253x_ts_probe(struct i2c_client *client,const struct i2c_device_id *idp)
{
	//struct ssl_ts_priv *ssl_priv;
	//struct input_dev *ssl_input = NULL;
//#ifdef USE_CALKEY_PROC	
	unsigned short data[4];
//#endif	
	int error = -1;
	int i;

	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
		printk("+-----------------------------------------+\n");
		printk("|	ssd253x_ts_probe!                 |\n");
		printk("+-----------------------------------------+\n");
	#endif

#ifdef MISC_DEV
	g_tp_client = client;
#endif
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: need I2C_FUNC_I2C\n");
		#endif
		return -ENODEV;
	}
	else
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: i2c Check OK!\n");
			printk("		ssd253x_ts_probe: i2c_client name : %s\n",client->name);
		#endif
	}
this_client=client;
	ssl_priv = kzalloc(sizeof(*ssl_priv), GFP_KERNEL);
	if (!ssl_priv)
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: kzalloc Error!\n");
		#endif
		error=-ENODEV;
		goto	err0;
	}
	else
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: kzalloc OK!\n");
		#endif
	}

  
  
	dev_set_drvdata(&client->dev, ssl_priv);
	ssl_input = input_allocate_device();
	if (!ssl_input)
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: input_allocate_device Error\n");
		#endif
		error=-ENODEV;
		goto	err1;
	}
	else
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: input_allocate_device OK\n");
		#endif
	}

	ssl_input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) | BIT_MASK(EV_SYN)|BIT_MASK(EV_REP) ;
	ssl_input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);// | BIT_MASK(BTN_2);
	ssl_input->name = client->name;
	ssl_input->id.bustype = BUS_I2C;
	ssl_input->id.vendor  = 0x2878; // Modify for Vendor ID
	ssl_input->dev.parent = &client->dev;
	ssl_input->open = ssd253x_ts_open;
	ssl_input->close = ssd253x_ts_close;
	input_set_drvdata(ssl_input, ssl_priv);
	ssl_priv->client = client;
	ssl_priv->input = ssl_input;
	ssl_priv->use_irq = ENABLE_INT;
	//ssl_priv->irq = TOUCH_INT_NO;
	ssl_priv->irq = CTP_IRQ_NUMBER;
	irq_handler=ssl_priv->irq;
//	ssl_priv->FingerNo=Get_Finger_Num;		//FINGER_NO_MAX;
	ssl_priv->Resolution=64;

	for(i=0;i<FINGER_NO_MAX;i++)
	{
		//ssl_priv->FingerP[i]=0;
		// For Finger Check Swap
		ssl_priv->sFingerX[i]=0xFFF;
		ssl_priv->sFingerY[i]=0xFFF;

		// For Adaptive Running Average
		ssl_priv->pFingerX[i]=0xFFF;
		ssl_priv->pFingerY[i]=0xFFF;
	}

ctp_wakeup_inet(1,10);
 ctp_wakeup_inet(0,10);
 ctp_wakeup_inet(1,60);
  
	deviceReset(client);
	mdelay(200);
	printk("SSL Touchscreen I2C Address: 0x%02X\n",client->addr);
	ssl_input->id.product = ReadRegister(client, DEVICE_ID_REG,2);
	ssl_input->id.version = ReadRegister(client,VERSION_ID_REG,2);
	ssl_input->id.product = ReadRegister(client, DEVICE_ID_REG,2);
	ssl_input->id.version = ReadRegister(client,VERSION_ID_REG,2);
	printk("SSL Touchscreen Device ID  : 0x%04X\n",ssl_input->id.product);
	printk("SSL Touchscreen Version ID : 0x%04X\n",ssl_input->id.version);
	
	if(ssl_priv->input->id.product==0x2531)		ssl_priv->Resolution=32;
	else if(ssl_priv->input->id.product==0x2533)	ssl_priv->Resolution=64;
	else
	{
  ctp_wakeup_inet(0,10);
  ctp_wakeup_inet(1,100);
	      deviceReset(client);
	      mdelay(200);
				ssl_input->id.product = ReadRegister(client, DEVICE_ID_REG,2);
				ssl_input->id.version = ReadRegister(client,VERSION_ID_REG,2);
				ssl_input->id.product = ReadRegister(client, DEVICE_ID_REG,2);
				ssl_input->id.version = ReadRegister(client,VERSION_ID_REG,2);

	   		if(ssl_priv->input->id.product==0x2531)		ssl_priv->Resolution=32;
	   			else if(ssl_priv->input->id.product==0x2533)	ssl_priv->Resolution=64;
	   		else
				{
            m_inet_ctpState=0;
						error=-ENODEV;
            goto	err1;
         }
	}	
	
	SSD253xdeviceInit(client);
	ssl_priv->FingerNo=Get_Finger_Num;		//FINGER_NO_MAX;
	WriteRegister(client,EVENT_FIFO_SCLR,0x01,0x00,1); // clear Event FiFo
	#ifdef CONFIG_TS_PROBE_DEBUG
	printk("		ssd253X_ts_probe: %04XdeviceInit OK!\n",ssl_input->id.product);
	#endif
	m_inet_ctpState=1;
	
	ssd253x_wq = create_singlethread_workqueue("ssd253x_wq");
	if (!ssd253x_wq)
	{
		#ifdef CONFIG_TS_WORKQUEUE_DEBUG
		printk("		ssd253x_ts_init: create_singlethread_workqueue Error!\n");
		#endif
		return -ENOMEM;
	}
	else
	{
		#ifdef CONFIG_TS_WORKQUEUE_DEBUG
		printk("		ssd253x_ts_init: create_singlethread_workqueue OK!\n");
		#endif
	}
 
	input_set_abs_params(ssl_input, ABS_MT_TRACKING_ID, 0,MAX_TRACKID_ITEM, 0, 0);
	input_set_abs_params(ssl_input, ABS_MT_TOUCH_MAJOR, 0, MAX_TOUCH_MAJOR, 0, 0);
	input_set_abs_params(ssl_input, ABS_MT_WIDTH_MAJOR, 0, MAX_WIDTH_MAJOR, 0, 0);
	input_set_abs_params(ssl_input, ABS_MT_POSITION_X,  0,screen_max_x + 1, 0, 0);
	input_set_abs_params(ssl_input, ABS_MT_POSITION_Y,  0,screen_max_y + 1, 0, 0);

	//#ifdef USE_TOUCH_KEY
	if (SSD_Touch_Key_Flag)
	{
		set_bit(KEY_MENU, ssl_input->keybit);
		set_bit(KEY_HOME, ssl_input->keybit);
		set_bit(KEY_BACK, ssl_input->keybit);
		set_bit(KEY_SEARCH, ssl_input->keybit);
	}
	//#endif
	
	INIT_WORK(&ssl_priv->ssl_work, ssd253x_ts_work);
	error = input_register_device(ssl_input);

	if(error)
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: input_register_device input Error!\n");
		#endif
		error=-ENODEV;
		goto	err1;
	}
	else
	{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: input_register_device input OK!\n");
		#endif
	}

	if (ssl_priv->use_irq==HYBRID_INT)
	{
	
		if(!(sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)ssd253x_ts_isr,ssl_priv)))
		{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: request_irq Error!\n");
		#endif
			error=-ENODEV;
			goto err2;
		}
		else
		{
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: request_irq OK!\n");
			#endif
		}	
	}
//	if (1)
	{
		hrtimer_init(&ssl_priv->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ssl_priv->timer.function = ssd253x_ts_timer;
		#ifdef CONFIG_TS_PROBE_DEBUG
			printk("		ssd253x_ts_probe: timer_init OK!\n");
		#endif
	}

#ifdef	CONFIG_HAS_EARLYSUSPEND
	ssl_priv->early_suspend.suspend = ssd253x_ts_early_suspend;
	ssl_priv->early_suspend.resume  = ssd253x_ts_late_resume;
	//ssl_priv->early_suspend.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN+1;
	ssl_priv->early_suspend.level   =  EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	register_early_suspend(&ssl_priv->early_suspend);
#endif 

//#ifdef USE_CALKEY_PROC
	if (SSD_Touch_Key_Flag)
	{
//======================== touch key Cal ==============================//
    calkey_workq = create_workqueue ("calkey_workq");
    if (!calkey_workq) 
    {
        error = -ENOMEM;
        goto err2;
    }
    INIT_WORK(&ssl_priv->calkey_work,calkey_do_work);
    msleep(100);
		read_key_base(ssl_priv->client,0x01,data);
		base_1 = data[0];
		base_2 = data[1];
		base_3 = data[2];
		base_4 = data[3];
    init_timer(&ssl_priv->calkey_timer);
    ssl_priv->calkey_timer.function = calkey_do_timer;
    ssl_priv->calkey_timer.data = (unsigned long)ssl_priv;
    ssl_priv->calkey_timer.expires = 0;
    mod_timer(&ssl_priv->calkey_timer,jiffies + 200);
  }
//=======================================================================//    
//#endif
	return 0;

exit_set_irq_mode:
err2:	input_unregister_device(ssl_input);	
err1:	input_free_device(ssl_input);
err_wakeup_request:
err_ioremap_failed:
    if(gpio_addr){
        iounmap(gpio_addr);
    }	
	kfree(ssl_priv);
err0:	dev_set_drvdata(&client->dev, NULL);
	return error;
}

static int ssd253x_ts_open(struct input_dev *dev)
{
	struct ssl_ts_priv *ssl_priv = input_get_drvdata(dev);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_open!                  |\n");
	printk("+-----------------------------------------+\n");
	#endif	
	deviceResume(ssl_priv->client);
	if(ssl_priv->use_irq!=POLLING_INT)
		{
			enable_irq(ssl_priv->irq);
		}
	else
		{
			hrtimer_start(&ssl_priv->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
		}
		ssl_priv->suspend_opend = 0;
	return 0;
}

static void ssd253x_ts_close(struct input_dev *dev)
{
	struct ssl_ts_priv *ssl_priv = input_get_drvdata(dev);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_close!                 |\n");
	printk("+-----------------------------------------+\n");
	#endif
	deviceSuspend(ssl_priv->client);
	
	ssl_priv->suspend_opend = 1;
	hrtimer_cancel(&ssl_priv->timer);
	if(ssl_priv->use_irq==HYBRID_INT)
		{
			disable_irq(irq_handler);
		}
}

static int ssd253x_ts_resume(struct i2c_client *client)
{
	//struct ssl_ts_priv *ssl_priv = dev_get_drvdata(&client->dev);
	//struct ssl_ts_priv *ssl_priv = i2c_get_clientdata(this_client);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_resume!                |\n");
	printk("+-----------------------------------------+\n");
	#endif
	Ssd_Timer_flag = 0;
	Ssd_record = 0;
	deviceResume(client);
	ssl_priv->suspend_opend = 0;		
	if(ssl_priv->use_irq!=POLLING_INT) 
		{
			enable_irq(irq_handler);
		}
	else 
		{
			hrtimer_start(&ssl_priv->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
		}
	return 0;
}

static int ssd253x_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	//struct ssl_ts_priv *ssl_priv = dev_get_drvdata(&client->dev);
	//struct ssl_ts_priv *ssl_priv = i2c_get_clientdata(this_client);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_suspend!               |\n");
	printk("+-----------------------------------------+\n");
	#endif
//#ifdef USE_CALKEY_PROC
	timer_status = 1;	//Paul added for touch-key check 2012-01-12	
//#endif
	ssl_priv->suspend_opend = 1;
	hrtimer_cancel(&ssl_priv->timer);	
	if (ssl_priv->use_irq==HYBRID_INT)
		{
			disable_irq(irq_handler);
		}	
	Ssd_Timer_flag = 0;
	Ssd_record = 0;
	deviceSuspend(client);

	return 0;
}

#ifdef	CONFIG_HAS_EARLYSUSPEND
static void ssd253x_ts_late_resume(struct early_suspend *h)
{
	struct ssl_ts_priv *ssl_priv = container_of(h, struct ssl_ts_priv, early_suspend);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_late_resume!           |\n");
	printk("+-----------------------------------------+\n");
	#endif
	//ssd253x_ts_resume(ssl_priv->client);
}
static void ssd253x_ts_early_suspend(struct early_suspend *h)
{
	struct ssl_ts_priv *ssl_priv = container_of(h, struct ssl_ts_priv, early_suspend);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_early_suspend!         |\n");
	printk("+-----------------------------------------+\n");
	#endif
	//ssd253x_ts_suspend(ssl_priv->client, PMSG_SUSPEND);
}
#endif

static int ssd253x_ts_remove(struct i2c_client *client)
{
	//struct ssl_ts_priv *ssl_priv = dev_get_drvdata(&client->dev);
	//struct ssl_ts_priv *ssl_priv = i2c_get_clientdata(this_client);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_remove !               |\n");
	printk("+-----------------------------------------+\n");
	#endif
	//if((ssl_priv->use_irq==POLLING_INT)||(ssl_priv->use_irq==HYBRID_INT)) 
	hrtimer_cancel(&ssl_priv->timer);
	if (ssl_priv->use_irq==HYBRID_INT) 
		{
			free_irq(ssl_priv->irq, ssl_priv);
		}
	input_unregister_device(ssl_priv->input);
	input_free_device(ssl_priv->input);
	kfree(ssl_priv);
	dev_set_drvdata(&client->dev, NULL);
	return 0;
}

static u32 ssd253x_ts_isr(struct ssl_ts_priv *ssl_priv)
{
	//int reg_val;
	//struct ssl_ts_priv *ssl_priv = dev_id;
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_isr!                   |\n");
	printk("+-----------------------------------------+\n");
	#endif	

     queue_work(ssd253x_wq, &ssl_priv->ssl_work);
	return  0;
}

//Rocky@20120112-
static enum hrtimer_restart ssd253x_ts_timer(struct hrtimer *timer)
{
	struct ssl_ts_priv *ssl_priv = container_of(timer, struct ssl_ts_priv, timer);
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_timer!                 |\n");
	printk("+-----------------------------------------+\n");
	#endif
	queue_work(ssd253x_wq, &ssl_priv->ssl_work);
	if(ssl_priv->use_irq==POLLING_INT) 
		{
			hrtimer_start(&ssl_priv->timer, ktime_set(0, MicroTimeTInterupt), HRTIMER_MODE_REL);
		}
	return HRTIMER_NORESTART;
}

static const struct i2c_device_id ssd253x_ts_id[] = {
	{ CTP_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ssd253x_ts_id);

static struct i2c_driver ssd253x_ts_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = ssd253x_ts_probe,
	.remove = ssd253x_ts_remove,
//#ifndef	CONFIG_HAS_EARLYSUSPEND
	.suspend = ssd253x_ts_suspend,
	.resume = ssd253x_ts_resume,
//#endif
	.driver = {
		.name = CTP_NAME,
		.owner = THIS_MODULE,
	},
	.id_table = ssd253x_ts_id,
	.address_list	= u_i2c_addr.normal_i2c,
};

#ifdef MISC_DEV
static const struct file_operations tp_fops = {
	.owner		= THIS_MODULE,
	.read		= tp_read,
	.write		= tp_write,
	.unlocked_ioctl	= tp_ioctl,
	.open		= tp_open,
	.release	= tp_release,
};
static struct miscdevice misc = {  
    .minor = MISC_DYNAMIC_MINOR,  
    .name  = TP_CHR, 
    .fops  = &tp_fops,  
};  
#endif

static char banner[] __initdata = KERN_INFO "SSL Touchscreen driver, (c) 2011 Solomon Systech Ltd.\n";

static int __init ssd253x_ts_init(void)
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
	
	printk("%s\n",SSD253X_IC_INFO);		
	printk("[FUN]%s\n", __func__);
	
	
	if(!ctp_get_system_config())
	{
		printk("not used or capture config error");
		return -ENODEV;
	}
		
	u_i2c_addr.dirty_addr_buf[0] = 0x48;
	u_i2c_addr.dirty_addr_buf[1] = I2C_CLIENT_END;
	
	
	

	ssd253x_ts_driver.detect = ctp_detect;

	ret=i2c_add_driver(&ssd253x_ts_driver);
	#ifdef CONFIG_TS_I2C_TRANSFER_DEBUG
	if(ret) printk("		ssd253x_ts_init: i2c_add_driver Error! \n");
	else    printk("		ssd253x_ts_init: i2c_add_driver OK! \n");
	#endif

#ifdef MISC_DEV
  ret = misc_register(&misc);  
  printk(TP_CHR "\tinitialized\n");  
#endif

	return ret;
}

static void __exit ssd253x_ts_exit(void)
{
	#ifdef CONFIG_TS_FUNCTION_CALLED_DEBUG
	printk("+-----------------------------------------+\n");
	printk("|	ssd253x_ts_exit!                  |\n");
	printk("+-----------------------------------------+\n");
	#endif
	i2c_del_driver(&ssd253x_ts_driver);
	if (ssd253x_wq) destroy_workqueue(ssd253x_wq);

#ifdef MISC_DEV
    misc_deregister(&misc);  
#endif    
}

#ifdef MISC_DEV
static long tp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	
	return 0;
}

static int tp_open(struct inode *inode, struct file *file)
{
			printk("[%s]---------+++++---------------\n",__func__);


	return 0;
}
static int tp_release(struct inode *inode, struct file *file)
{
			printk("[%s]---------+++++---------------\n",__func__);
	return 0;
}

static ssize_t tp_read(struct file *file, char __user *buf, size_t count,
		loff_t *offset)
{
    char *kbuf;  
    uint8_t reg;
    int  ByteNo;
    int readValue;
    int i;

    kbuf = kmalloc(count,GFP_KERNEL);  
    
    if(copy_from_user(kbuf,buf,1))  
    {  
        printk("no enough memory!\n");  
        return -1;  
    }  

		reg = (uint8_t)kbuf[0];
		ByteNo = count;
	
//		printk("[%s]===ReadRegister:reg=0x%02x,ByteNo=%d============\n",__func__,reg,ByteNo);
		readValue = ReadRegister(g_tp_client, reg, ByteNo);
//		printk("[%s]===readValue:0x%04x============\n",__func__,readValue);
		for(i = 0;i < ByteNo;i++)
		{
			kbuf[i] = (readValue>>(8*i)) & 0xff;
//			printk("[%s]===read:0x%02x============\n",__func__,kbuf[i]);
		}

    if(copy_to_user(buf,kbuf,count))  
    {  
        printk("no enough memory!\n");  
        return -1;  
    }  

		kfree(kbuf);

	return count;
}

static ssize_t tp_write(struct file *file, const char __user *buf,
		size_t count, loff_t *offset)
{
		int i;
    char *kbuf;   
    int data;

    kbuf = kmalloc(count,GFP_KERNEL);  
      
    if(copy_from_user(kbuf,buf,count))  
    {  
        printk("no enough memory!\n");  
        return -1;  
    }  
//    printk("<1>spi write!,count=%d,buf=0x%02x,0x%02x,0x%02x,0x%02x\n",count,kbuf[0],kbuf[1],kbuf[2],kbuf[3]);  

		//gpio reset
		if(kbuf[1] == 0x04)
		{
	  	ctp_wakeup_inet(0,10);
  		ctp_wakeup_inet(1,100);
		}
		
		WriteRegister(	g_tp_client,kbuf[1],kbuf[2],kbuf[3],kbuf[0]);

		if(kbuf[1] == 0x04)
		{
			msleep(100);
		}
		
		kfree(kbuf);

	return count;
}
#endif

module_init(ssd253x_ts_init);
module_exit(ssd253x_ts_exit);

MODULE_AUTHOR("Solomon Systech Ltd - Raysens Technology, Richard Liu");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ssd253x Touchscreen Driver 1.6_Richard@Raysens@20130502");


