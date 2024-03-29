/*
 * A V4L2 driver for GalaxyCore SP0718 cameras.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>


#include "camera.h"


MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for Superpix SP0718 sensors");
MODULE_LICENSE("GPL");



//for internel driver debug
#define DEV_DBG_EN   		0
#if(DEV_DBG_EN == 1)		
#define vfe_dev_dbg(x,arg...) printk("[CSI_DEBUG][SP0718]"x,##arg)
#else
#define vfe_dev_dbg(x,arg...) 
#endif
#define vfe_dev_err(x,arg...) printk("[CSI_ERR][SP0718]"x,##arg)
#define vfe_dev_print(x,arg...) printk("[CSI][SP0718]"x,##arg)

#define LOG_ERR_RET(x)  { \
                          int ret;  \
                          ret = x; \
                          if(ret < 0) {\
                            vfe_dev_err("error at %s\n",__func__);  \
                            return ret; \
                          } \
                        }

//define module timing
#define MCLK (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x0718

//define the voltage level of control signal
#define CSI_STBY_ON			1
#define CSI_STBY_OFF 		0
#define CSI_RST_ON			0
#define CSI_RST_OFF			1
#define CSI_PWR_ON			1
#define CSI_PWR_OFF			0

#define regval_list reg_list_a8_d8

#define REG_DLY  0xff

#define SP0718_AWB_EN   (1<<4)

#define SP0718_AWB_DIS	(~(1<<4))
#define SP0718_AE_EN   (1<<0)
#define SP0718_AE_DIS	(~(1<<0))

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 20

/*
 * The sp0718 sits on i2c with ID 0x42
 */
#define I2C_ADDR 0x42

#define SP0718_NORMAL_Y0ffset  0x20
#define SP0718_LOWLIGHT_Y0ffset  0x25
//AE target
#define  SP0718_P1_0xeb  0x78	//0x78	//modify by sy_yjp,20121210 +8
#define  SP0718_P1_0xec  0x6c	//0x78	//70_20130817														+8
#define  SP0718_P1_0xee  0x0a
#define  SP0718_P1_0xed  0x10
#define  SP0718_P1_0x10  0x00//outdoor
#define  SP0718_P1_0x14  0x20
#define  SP0718_P1_0x11  0x00//nr
#define  SP0718_P1_0x15  0x18
#define  SP0718_P1_0x12  0x00//dummy
#define  SP0718_P1_0x16  0x10
#define  SP0718_P1_0x13  0x00//low
#define  SP0718_P1_0x17  0x00

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;  /* coming later */

struct cfg_array { /* coming later */
	struct regval_list * regs;
	int size;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
  return container_of(sd, struct sensor_info, sd);
}
 
static struct regval_list sensor_default_regs[] = {
	{0xfd,0x00},
	{0x1C,0x3c},
	{0x31,0x00}, //flip
	{0x27,0xb3},///0xb3	///2x gain
	{0x1b,0x17},
	{0x26,0xaa},
	{0x37,0x02},
	{0x28,0x8f},
	{0x1a,0x73},
	{0x1e,0x1b},
	{0x21,0x06},  ///blackout voltage
	{0x22,0x2a},  ///colbias
	{0x0f,0x3f},
	{0x10,0x3e},
	{0x11,0x00},
	{0x12,0x01},
	{0x13,0x3f},
	{0x14,0x04},
	{0x15,0x30},
	{0x16,0x31},
	{0x17,0x01},
	{0x69,0x31},
	{0x6a,0x2a},
	{0x6b,0x33},
	{0x6c,0x1a},
	{0x6d,0x32},
	{0x6e,0x28},
	{0x6f,0x29},
	{0x70,0x34},
	{0x71,0x18},
	{0x36,0x02},///delete badframe
	{0xfd,0x01},
	{0x5d,0x51},///position
	
	///Blacklevel
	{0x1f,0x10},
	{0x20,0x1f},
	
	//SI15_SP0718 24M 50Hz 18-10fps
	///ae setting
	{0xfd,0x00},
	{0x03,0x02},
	{0x04,0x88},
	{0x06,0x5a},
	{0x09,0x01},
	{0x0a,0x05},
	{0xfd,0x01},
	{0xef,0x6c},
	{0xf0,0x00},
	{0x02,0x0a},
	{0x03,0x01},
	{0x06,0x66},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	///Status
	{0xfd,0x02},
	{0xbe,0x38},
	{0xbf,0x04},
	{0xd0,0x38},
	{0xd1,0x04},
	{0xfd,0x01},
	{0x5b,0x04},
	{0x5c,0x38},

	
	///rpc
	{0xfd,0x01},
	{0xe0,0x40},//////24///4c///48///4c///44///4c///3e///3c///3a///38///rpc_1base_max
	{0xe1,0x30},//////24///3c///38///3c///36///3c///30///2e///2c///2a///rpc_2base_max
	{0xe2,0x2e},//////24///34///30///34///2e///34///2a///28///26///26///rpc_3base_max
	{0xe3,0x2a},//////24///2a///2e///2c///2e///2a///2e///26///24///22///rpc_4base_max
	{0xe4,0x2a},//////24///2a///2e///2c///2e///2a///2e///26///24///22///rpc_5base_max
	{0xe5,0x28},//////24///2c///2a///2c///28///2c///24///22///20///rpc_6base_max
	{0xe6,0x28},//////24///2c///2a///2c///28///2c///24///22///20///rpc_7base_max
	{0xe7,0x26},//////24///2a///28///2a///26///2a///22///20///20///1e///rpc_8base_max
	{0xe8,0x26},//////24///2a///28///2a///26///2a///22///20///20///1e///rpc_9base_max
	{0xe9,0x26},//////24///2a///28///2a///26///2a///22///20///20///1e///rpc_10base_max
	{0xea,0x26},//////24///28///26///28///24///28///20///1f///1e///1d///rpc_11base_max
	{0xf3,0x26},//////24///28///26///28///24///28///20///1f///1e///1d///rpc_12base_max
	{0xf4,0x26},//////24///28///26///28///24///28///20///1f///1e///1d///rpc_13base_max
	///ae gain &status
	{0xfd,0x01},
	{0x04,0xa0},///rpc_max_indr
	{0x05,0x26},///1e///rpc_min_indr 
	{0x0a,0xa0},///rpc_max_outdr
	{0x0b,0x26},///rpc_min_outdr
	{0x5a,0x40},///dp rpc   
	{0xfd,0x02}, 
	{0xbc,0xa0},///rpc_heq_low
	{0xbd,0x80},///rpc_heq_dummy
	{0xb8,0x80},///mean_normal_dummy
	{0xb9,0x90},///mean_dummy_normal
	
	///ae target
	{0xfd,0x01}, 
	{0xeb,SP0718_P1_0xeb},///78  modify by sp_yjp,20121210
	{0xec,SP0718_P1_0xec},///78  modify by sp_yjp,20121210
	{0xed,SP0718_P1_0xed},	
	{0xee,SP0718_P1_0xee},
	
	///lsc
	{0xfd,0x01},
	{0x26,0x30},
	{0x27,0x2c},
	{0x28,0x07},
	{0x29,0x08},
	{0x2a,0x00},
	{0x2b,0x03},
	{0x2c,0x00},
	{0x2d,0x00},
	///same lens as sp0718
	///RGain
	{0xfd,0x01},
	{0xa1,0x27},
	{0xa2,0x20},
	{0xa3,0x2d},
	{0xa4,0x24},
	{0xad,0x0a},///lu
	{0xae,0x08},///ru
	{0xaf,0x0a},///ld
	{0xb0,0x03},///rd
	///GGain
	{0xa5,0x25},
	{0xa6,0x20},
	{0xa7,0x25},
	{0xa8,0x20},
	{0xb1,0x05},
	{0xb2,0x00},
	{0xb3,0x00},
	{0xb4,0x00},
	///BGain
	{0xa9,0x22},
	{0xaa,0x20},
	{0xab,0x28},
	{0xac,0x1f},
	{0xb5,0x00},
	{0xb6,0x00},
	{0xb7,0x00},
	{0xb8,0x00},
	
	///xy-034
	///RGain
	{0xfd,0x01},
	{0xa1,0x2a},
	{0xa2,0x24},
	{0xa3,0x2d},
	{0xa4,0x24},
	{0xad,0x0d},///lu
	{0xae,0x08},///ru
	{0xaf,0x0a},///ld
	{0xb0,0x03},///rd
	///GGain
	{0xa5,0x25},
	{0xa6,0x20},
	{0xa7,0x25},
	{0xa8,0x20},
	{0xb1,0x02},
	{0xb2,0x00},
	{0xb3,0x00},
	{0xb4,0x00},
	///BGain
	{0xa9,0x22},
	{0xaa,0x20},
	{0xab,0x28},
	{0xac,0x1f},
	{0xb5,0x00},
	{0xb6,0x00},
	{0xb7,0x00},
	{0xb8,0x00},
	
	///蓝光
	///RGain
	{0xfd,0x01},
	{0xa1,0x2a},
	{0xa2,0x26},
	{0xa3,0x2d},
	{0xa4,0x24},
	{0xad,0x0d},///lu
	{0xae,0x08},///ru
	{0xaf,0x0a},///ld
	{0xb0,0x03},///rd
	///GGain
	{0xa5,0x25},
	{0xa6,0x1d},
	{0xa7,0x25},
	{0xa8,0x20},
	{0xb1,0x02},
	{0xb2,0x00},
	{0xb3,0x00},
	{0xb4,0x00},
	///BGain
	{0xa9,0x1c},
	{0xaa,0x1a},
	{0xab,0x1f},
	{0xac,0x1c},
	{0xb5,0x00},
	{0xb6,0x00},
	{0xb7,0x00},
	{0xb8,0x00},
	///DP
	{0xfd,0x01},
	{0x48,0x09},
	{0x49,0x99},
	  
	///awb
	{0xfd,0x01},
	{0x32,0x15},
	{0xfd,0x02},
	{0x26,0xc9},
	{0x27,0x8b},
	{0x1b,0x80},
	{0x1a,0x80},
	{0x18,0x27},
	{0x19,0x26},
	{0xfd,0x02},
	{0x2a,0x01},
	{0x2b,0x10},
	{0x28,0xef},///0xa0///f8
	{0x29,0x08},
	
	///d65 90
	{0x66,0x4e},///0x48
	{0x67,0x65},///0x69
	{0x68,0xcb},///0xaa
	{0x69,0xed},
	{0x6a,0xa5},
	///indoor 91
	{0x7c,0x41},///0x2f///0x44
	{0x7d,0x55},///0x4b///0x6f
	{0x7e,0x0a},///0xed
	{0x7f,0x23},///0x28
	{0x80,0xaa},///0xa6
	///cwf   92
	{0x70,0x2e},///0x3b
	{0x71,0x3f},///0x55
	{0x72,0x22},///0x28
	{0x73,0x35},///0x45
	{0x74,0xaa},
	///tl84  93
	{0x6b,0x11},
	{0x6c,0x25},///0x2f
	{0x6d,0x35},
	{0x6e,0x46},///0x52
	{0x6f,0xaa},
	///f    94
	{0x61,0xf4},///0xed
	{0x62,0x0f},///0f
	{0x63,0x52},///0x5d
	{0x64,0x70},///0x75///0x8f
	{0x65,0x6a},
	
	{0x75,0x80},
	{0x76,0x09},
	{0x77,0x02},
	{0x24,0x25},
	///outdoor r\b range
	/// 0x78,0xc0
	/// 0x79,0xa0
	/// 0x7a,0xa0
	/// 0x7b,0x80
	///skin 
	{0x0e,0x30},
	{0x09,0x07},
	///gw
	{0x31,0x60},
	{0x32,0x60},
	{0x33,0xc0},
	{0x35,0x6f},
	{0x3b,0x09},   
	
	/// sharp
	{0xfd,0x02},
	{0xde,0x0f},
	{0xd2,0x00},///控制黑白边；0-边粗，f-变细
	{0xd3,0x00},
	{0xd4,0x04},
	{0xd5,0x06},
	{0xd7,0x14},///轮廓判断//0x10 modify by sp_yjp,20121210
	{0xd8,0x1e},	//0x1a modify by sp_yjp,20121210
	{0xd9,0x28},	//0x24 modify by sp_yjp,20121210
	{0xda,0x2c},	//0x28 modify by sp_yjp,20121210
	{0xdb,0x08},
	{0xe8,0x48},///轮廓强度
	{0xe9,0x38},	//0x38 0x48 modify by sp_yjp,20121210
	{0xea,0x30},	//0x30 modify by sp_yjp,20121210
	{0xeb,0x20},	//0x20 modify by sp_yjp,20121210
	{0xec,0x70},	//0x80 modify by sp_yjp,20121210
	{0xed,0x60},	//0x60 modify by sp_yjp,20121210
	{0xee,0x40},	//0x40 modify by sp_yjp,20121210
	{0xef,0x40},	//0x20 modify by sp_yjp,20121210
	///平坦区域锐化力度
	{0xf7,0x00},
	{0xf8,0x08},
	{0xf9,0x00},
	{0xfa,0x00},
	///dns
	{0xfd,0x01},
	{0x64,0x44}, ///沿方向边缘平滑力度  ///0-最强，8-最弱
	{0x65,0x22},
	{0x6d,0x08},///强平滑（平坦）区域平滑阈值
	{0x6e,0x08},
	{0x6f,0x10},
	{0x70,0x10},
	{0x71,0x0d},///弱平滑（非平坦）区域平滑阈值	
	{0x72,0x1b},
	{0x73,0x20},
	{0x74,0x24},
	//{0x75,0x46}, ///[7:4]平坦区域强度，[3:0]非平坦区域强度；0-最强，8-最弱?
	{0x75,0x44},		//0x46 modify by sp_yjp,20121210
	{0x76,0x44},	//0x34  0x46	//modify by sp_yjp,20121210
	
	{0x77,0x33},	//0x33 modify by sp_yjp,20121210
	{0x78,0x22}, //0x02 modify by sp_yjp,20121210
	
	{0x81,0x18},///2x///根据增益判定区域阈值
	{0x82,0x30},///4x
	{0x83,0x40},///8x
	{0x84,0x50},///16x
	{0x85,0x0c},///12/8+reg0x81 第二阈值，在平坦和非平坦区域做连接
	{0xfd,0x02},
	{0xdc,0x0f},
	
	///gamma  蓝光镜头对比强，18阶高亮分不出来；sp0718镜头；
	{0xfd,0x01},
	{0x8b,0x00},///00///00///00///00///00     
	{0x8c,0x08},///05///02///0b///0b///10     
	{0x8d,0x10},///0d///0a///19///17///20     
	{0x8e,0x19},///15///13///2a///27///31     
	{0x8f,0x26},///1e///1d///37///35///3f     
	{0x90,0x3b},///37///30///4b///51///53     
	{0x91,0x4d},///4a///40///5e///64///64     
	{0x92,0x5f},///5b///4e///6c///74///74     
	{0x93,0x71},///6d///5a///78///80///80     
	{0x94,0x8b},///88///71///92///92///92     
	{0x95,0xa1},///9c///85///a6///a2///a2     
	{0x96,0xb3},///af///96///b5///af///af     
	{0x97,0xbf},///bc///a6///bf///bb///bb     
	{0x98,0xca},///ca///b3///ca///c6///c6     
	{0x99,0xd2},///d2///c0///d2///d0///d0     
	{0x9a,0xda},///da///cb///d9///d9///d9     
	{0x9b,0xe1},///e1///d5///e1///e0///e0     
	{0x9c,0xe7},///e7///df///e8///e8///e8     
	{0x9d,0xec},///ec///e9///ee///ee///ee     
	{0x9e,0xf3},///f3///f2///f4///f4///f4     
	{0x9f,0xf9},///f9///fa///fa///fa///fa     
	{0xa0,0xff},///ff///ff///ff///ff///ff     
	///CCM
	{0xfd,0x02},
	{0x15,0xd0},///b>th
	{0x16,0xa0},///r<th
	///gc镜头照人脸偏黄
	{0xa0,0x80},///a6///a6///a6///8c///80///
	{0xa1,0x00},///da///da///da///fa///00///
	{0xa2,0x00},///00///00///00///fa///00///
	{0xa3,0xda},///da///e7///da///da///e7///
	{0xa4,0xe6},///c0///c0///c0///c0///a6///
	{0xa5,0xc0},///e7///da///e7///e7///f4///
	{0xa6,0xf4},///f4///00///00///00///00///
	{0xa7,0xc0},///c0///b4///a7///cd///da///
	{0xa8,0xcc},///cc///cc///d9///b3///a6///
	{0xa9,0x00},///0c///0c///0c///3c///00///
	{0xaa,0x33},///33///33///33///33///33///
	{0xab,0x0f},///0f///0c///0c///0c///0c///
	{0xac,0xa6},///a2///b3///8c///
	{0xad,0xda},///04///0c///0c///
	{0xae,0x00},///da///c0///e7///
	{0xaf,0xda},///cd///cd///b4///
	{0xb0,0xcc},///d9///e6///e6///
	{0xb1,0xda},///da///cd///e7///
	{0xb2,0xe7},///f6///e7///e7///
	{0xb3,0x5a},///98///9a///9a///
	{0xb4,0x40},///f3///00///00///
	{0xb5,0x0c},///30///30///30///
	{0xb6,0x33},///33///33///33///
	{0xb7,0x1f},///0f///1f///1f/// 
	
	///sat u 
	{0xfd,0x01},
	{0xd3,0x4E},
	{0xd4,0x4A},
	{0xd5,0x40},
	{0xd6,0x40},
	///sat v     
	{0xd7,0x4E},
	{0xd8,0x4A},
	{0xd9,0x40},
	{0xda,0x40},
	///auto_sat
	{0xdd,0x30},
	{0xde,0x10},
	{0xd2,0x01},///autosa_en	
	{0xdf,0xff},///a0///y_mean_th
	
	///uv_th
	{0xfd,0x01},
	{0xc2,0xaa},
	{0xc3,0xaa},
	{0xc4,0x66},
	{0xc5,0x22}, 
	
	///heq
	{0xfd,0x01},
	{0x10,0x00}, 
	{0x11,0x00}, 
	{0x12,0x00}, 
	{0x13,0x00}, 
	
	{0x14,0x14},  	//0x00 modify by sp_yjp,20121210	
	{0x15,0x14},  	//0x00 modify by sp_yjp,20121210
	{0x16,0x14},   	//0x00 modify by sp_yjp,20121210
	{0x17,0x14},   	//0x00 modify by sp_yjp,20121210


	///{0xfd,0x01},	//add by sp_yjp,20121211
	///{0xd2,0x03},	///autosa_en autocontrast_en add by sp_yjp,20121211

	
	///auto 
	{0xfd,0x01},
	{0xfb,0x33},
	{0x32,0x15},
	{0x33,0xff},
	{0x34,0xe7},
	
	{0x35,0x00},	  	//add by sp_yjp,20121210
	{0xff,0xff}	
};

//20130817
static struct regval_list sensor_Q7011_regs[] = 
{
		{0xfd,0x00},
		{0x1C,0x3c},
		{0x31,0x00},
		{0x27,0xb3},///0xb3	///2x gain
		{0x1b,0x17},
		{0x26,0xaa},
		{0x37,0x02},
		{0x28,0x8f},
		{0x1a,0x73},
		{0x1e,0x1b},
		{0x21,0x06},  ///blackout voltage
		{0x22,0x2a},  ///colbias
		{0x0f,0x3f},
		{0x10,0x3e},
		{0x11,0x00},
		{0x12,0x01},
		{0x13,0x3f},
		{0x14,0x04},
		{0x15,0x30},
		{0x16,0x31},
		{0x17,0x01},
		{0x69,0x31},
		{0x6a,0x2a},
		{0x6b,0x33},
		{0x6c,0x1a},
		{0x6d,0x32},
		{0x6e,0x28},
		{0x6f,0x29},
		{0x70,0x34},
		{0x71,0x18},
		{0x36,0x00},//02 delete badframe
		{0xfd,0x01},
		{0x5d,0x51},//position
		{0xf2,0x19},

		//Blacklevel
		{0x1f,0x10},
		{0x20,0x1f},
		//pregain 
		{0xfd,0x02},
		{0x00,0x88},
		{0x01,0x88},
		//SI15_SP0718 24M 50Hz 15-8fps 
		//ae setting
		{0xfd,0x00},
		{0x03,0x01},
		{0x04,0xce},
		{0x06,0x00},
		{0x09,0x02},
		{0x0a,0xc4},
		{0xfd,0x01},
		{0xef,0x4d},
		{0xf0,0x00},
		{0x02,0x0c},
		{0x03,0x01},
		{0x06,0x47},
		{0x07,0x00},
		{0x08,0x01},
		{0x09,0x00},
		//Status   
		{0xfd,0x02},
		{0xbe,0x9c},
		{0xbf,0x03},
		{0xd0,0x9c},
		{0xd1,0x03},
		{0xfd,0x01},
		{0x5b,0x03},
		{0x5c,0x9c},

		//rpc
		{0xfd,0x01},
		{0xe0,0x40},////24//4c//48//4c//44//4c//3e//3c//3a//38//rpc_1base_max
		{0xe1,0x30},////24//3c//38//3c//36//3c//30//2e//2c//2a//rpc_2base_max
		{0xe2,0x2e},////24//34//30//34//2e//34//2a//28//26//26//rpc_3base_max
		{0xe3,0x2a},////24//2a//2e//2c//2e//2a//2e//26//24//22//rpc_4base_max
		{0xe4,0x2a},////24//2a//2e//2c//2e//2a//2e//26//24//22//rpc_5base_max
		{0xe5,0x28},////24//2c//2a//2c//28//2c//24//22//20//rpc_6base_max
		{0xe6,0x28},////24//2c//2a//2c//28//2c//24//22//20//rpc_7base_max
		{0xe7,0x26},////24//2a//28//2a//26//2a//22//20//20//1e//rpc_8base_max
		{0xe8,0x26},////24//2a//28//2a//26//2a//22//20//20//1e//rpc_9base_max
		{0xe9,0x26},////24//2a//28//2a//26//2a//22//20//20//1e//rpc_10base_max
		{0xea,0x26},////24//28//26//28//24//28//20//1f//1e//1d//rpc_11base_max
		{0xf3,0x26},////24//28//26//28//24//28//20//1f//1e//1d//rpc_12base_max
		{0xf4,0x26},////24//28//26//28//24//28//20//1f//1e//1d//rpc_13base_max
		//ae gain &status
		{0xfd,0x01},
		{0x04,0xe0},//rpc_max_indr
		{0x05,0x26},//1e//rpc_min_indr 
		{0x0a,0xa0},//rpc_max_outdr
		{0x0b,0x26},//rpc_min_outdr
		{0x5a,0x40},//dp rpc   
		{0xfd,0x02}, 
		{0xbc,0xa0},//rpc_heq_low
		{0xbd,0x80},//rpc_heq_dummy
		{0xb8,0x80},//mean_normal_dummy
		{0xb9,0x90},//mean_dummy_normal

		//ae target
		{0xfd,0x01}, 
		{0xeb,SP0718_P1_0xeb},//78 
		{0xec,SP0718_P1_0xec},//78
		{0xed,0x0a},	
		{0xee,0x10},

		//lsc       
		{0xfd,0x01},
		{0x26,0x30},
		{0x27,0x2c},
		{0x28,0x07},
		{0x29,0x08},
		{0x2a,0x40},
		{0x2b,0x03},
		{0x2c,0x00},
		{0x2d,0x00},
         
		{0xa1,0x24},
		{0xa2,0x27},
		{0xa3,0x27},
		{0xa4,0x2b},
		{0xa5,0x1c},
		{0xa6,0x1a},
		{0xa7,0x1a},
		{0xa8,0x1a},
		{0xa9,0x18},
		{0xaa,0x1c},
		{0xab,0x17},
		{0xac,0x17},
		{0xad,0x08},
		{0xae,0x08},
		{0xaf,0x08},
		{0xb0,0x00},
		{0xb1,0x00},
		{0xb2,0x00},
		{0xb3,0x00},
		{0xb4,0x00},
		{0xb5,0x02},
		{0xb6,0x06},
		{0xb7,0x00},
		{0xb8,0x00},


		//DP       
		{0xfd,0x01},
		{0x48,0x09},
		{0x49,0x99},

		//awb       
		{0xfd,0x01},
		{0x32,0x05},
		{0xfd,0x00},
		{0xe7,0x03},
		{0xfd,0x02},
		{0x26,0xc8},
		{0x27,0xb6},
		{0xfd,0x00},
		{0xe7,0x00},
		{0xfd,0x02},
		{0x1b,0x80},
		{0x1a,0x80},
		{0x18,0x26},
		{0x19,0x28},
		{0xfd,0x02},
		{0x2a,0x00},
		{0x2b,0x08},
		{0x28,0xef},//0xa0//f8
		{0x29,0x20},

		//d65 90  e2 93
		{0x66,0x42},//0x59//0x60////0x58//4e//0x48
		{0x67,0x62},//0x74//0x70//0x78//6b//0x69
		{0x68,0xee},//0xd6//0xe3//0xd5//cb//0xaa
		{0x69,0x18},//0xf4//0xf3//0xf8//ed
		{0x6a,0xa6},//0xa5
		//indoor 91
		{0x7c,0x3b},//0x45//30//41//0x2f//0x44
		{0x7d,0x5b},//0x70//60//55//0x4b//0x6f
		{0x7e,0x15},//0a//0xed
		{0x7f,0x39},//23//0x28
		{0x80,0xaa},//0xa6
		//cwf   92 
		{0x70,0x3e},//0x38//41//0x3b
		{0x71,0x59},//0x5b//5f//0x55
		{0x72,0x31},//0x30//22//0x28
		{0x73,0x4f},//0x54//44//0x45
		{0x74,0xaa},
		//tl84  93 
		{0x6b,0x1b},//0x18//11
		{0x6c,0x3a},//0x3c//25//0x2f
		{0x6d,0x3e},//0x3a//35
		{0x6e,0x59},//0x5c//46//0x52
		{0x6f,0xaa},
		//f    94
		{0x61,0xea},//0x03//0x00//f4//0xed
		{0x62,0x03},//0x1a//0x25//0f//0f
		{0x63,0x6a},//0x62//0x60//52//0x5d
		{0x64,0x8a},//0x7d//0x85//70//0x75//0x8f
		{0x65,0x6a},//0xaa//6a
         
		{0x75,0x80},
		{0x76,0x20},
		{0x77,0x00},
		{0x24,0x25},

		//针对室内调偏不过灯箱测试使用//针对人脸调偏
		{0x20,0xd8},
		{0x21,0xa3},//82//a8偏暗照度还有调偏
		{0x22,0xd0},//e3//bc
		{0x23,0x86},

		//outdoor r\b range
		{0x78,0xc3},//d8
		{0x79,0xba},//82
		{0x7a,0xa6},//e3
		{0x7b,0x99},//86


		//skin 
		{0x08,0x15},//
		{0x09,0x04},//
		{0x0a,0x20},//
		{0x0b,0x12},//
		{0x0c,0x27},//
		{0x0d,0x06},//
		{0x0e,0x63},//

		//wt th
		{0x3b},{0x10},
		//gw
		{0x31,0x60},
		{0x32,0x60},
		{0x33,0xc0},
		{0x35,0x6f},
         
		// sharp
		{0xfd,0x02},
		{0xde,0x0f},
		{0xd2,0x02},//6//控制黑白边；0-边粗，f-变细
		{0xd3,0x06},
		{0xd4,0x06},
		{0xd5,0x06},
		{0xd7,0x20},//10//2x根据增益判断轮廓阈值
		{0xd8,0x30},//24//1A//4x
		{0xd9,0x38},//28//8x
		{0xda,0x38},//16x
		{0xdb,0x08},//
		{0xe8,0x58},//48//轮廓强度
		{0xe9,0x48},
		{0xea,0x30},
		{0xeb,0x20},
		{0xec,0x48},//60//80
		{0xed,0x48},//50//60
		{0xee,0x30},
		{0xef,0x20},
		//平坦区域锐化力度
		{0xf3,0x50},
		{0xf4,0x10},
		{0xf5,0x10},
		{0xf6,0x10},
		//dns       
		{0xfd,0x01},
		{0x64,0x44},//沿方向边缘平滑力度  //0-最强，8-最弱
		{0x65,0x22},
		{0x6d,0x04},//8//强平滑（平坦）区域平滑阈值
		{0x6e,0x06},//8
		{0x6f,0x10},
		{0x70,0x10},
		{0x71,0x08},//0d//弱平滑（非平坦）区域平滑阈值	
		{0x72,0x12},//1b
		{0x73,0x1c},//20
		{0x74,0x24},
		{0x75,0x44},//[7:4]平坦区域强度，[3:0]非平坦区域强度；0-最强，8-最弱；
		{0x76,0x02},//46
		{0x77,0x02},//33
		{0x78,0x02},
		{0x81,0x10},//18//2x//根据增益判定区域阈值，低于这个做强平滑、大于这个做弱平滑；
		{0x82,0x20},//30//4x
		{0x83,0x30},//40//8x
		{0x84,0x48},//50//16x
		{0x85,0x0c},//12/8+reg0x81 第二阈值，在平坦和非平坦区域做连接
		{0xfd,0x02},
		{0xdc,0x0f},

		//gamma    
		{0xfd,0x01},
		{0x8b,0x00},//00//00     
		{0x8c,0x0a},//0c//09     
		{0x8d,0x16},//19//17     
		{0x8e,0x1f},//25//24     
		{0x8f,0x2a},//30//33     
		{0x90,0x3c},//44//47     
		{0x91,0x4e},//54//58     
		{0x92,0x5f},//61//64     
		{0x93,0x6c},//6d//70     
		{0x94,0x82},//80//81     
		{0x95,0x94},//92//8f     
		{0x96,0xa6},//a1//9b     
		{0x97,0xb2},//ad//a5     
		{0x98,0xbf},//ba//b0     
		{0x99,0xc9},//c4//ba     
		{0x9a,0xd1},//cf//c4     
		{0x9b,0xd8},//d7//ce     
		{0x9c,0xe0},//e0//d7     
		{0x9d,0xe8},//e8//e1     
		{0x9e,0xef},//ef//ea     
		{0x9f,0xf8},//f7//f5     
		{0xa0,0xff},//ff//ff     
		//CCM      
		{0xfd,0x02},
		{0x15,0xd0},//b>th
		{0x16,0x95},//r<th
		//gc镜头照人脸偏黄
		//!F        
		{0xa0,0x80},//80
		{0xa1,0x00},//00
		{0xa2,0x00},//00
		{0xa3,0x00},//06
		{0xa4,0x8c},//8c
		{0xa5,0xf4},//ed
		{0xa6,0x0c},//0c
		{0xa7,0xf4},//f4
		{0xa8,0x80},//80
		{0xa9,0x00},//00
		{0xaa,0x30},//30
		{0xab,0x0c},//0c 
		//F  ,   
		{0xac,0x8c},
		{0xad,0xf4},
		{0xae,0x00},
		{0xaf,0xed},
		{0xb0,0x8c},
		{0xb1,0x06},
		{0xb2,0xf4},
		{0xb3,0xf4},
		{0xb4,0x99},
		{0xb5,0x0c},
		{0xb6,0x03},
		{0xb7,0x0f},

		//sat u     
		{0xfd,0x01},
		{0xd3,0x9c},//0x88//50
		{0xd4,0x98},//0x88//50
		{0xd5,0x8c},//50
		{0xd6,0x84},//50
		//sat v   
		{0xd7,0x9c},//0x88//50
		{0xd8,0x98},//0x88//50
		{0xd9,0x8c},//50
		{0xda,0x84},//50
		//auto_sat  
		{0xdd,0x30},
		{0xde,0x10},
		{0xd2,0x01},//autosa_en
		{0xdf,0xff},//a0//y_mean_th

		//uv_th     
		{0xfd,0x01},
		{0xc2,0xaa},
		{0xc3,0xaa},
		{0xc4,0x66},
		{0xc5,0x66}, 

		//heq
		{0xfd,0x01},
		{0x0f,0xff},
		{0x10,SP0718_P1_0x10}, //out
		{0x14,SP0718_P1_0x14}, 
		{0x11,SP0718_P1_0x11}, //nr
		{0x15,SP0718_P1_0x15},  
		{0x12,SP0718_P1_0x12}, //dummy
		{0x16,SP0718_P1_0x16}, 
		{0x13,SP0718_P1_0x13}, //low 	
		{0x17,SP0718_P1_0x17},   	
      
		{0xfd,0x01},
		{0xcd,0x20},
		{0xce,0x1f},
		{0xcf,0x20},
		{0xd0,0x55},  
		//aut,
		{0xfd,0x01},
		{0xfb,0x33},
		{0x32,0x15},
		{0x33,0xff},
		{0x34,0xe7},
       
		{0x35,0x00},	  	//add by sp_yjp,20121210
		{0xff,0xff}	

};
/*
 * The white balance settings
 * Here only tune the R G B channel gain. 
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */
static struct regval_list sensor_wb_manual[] = { 
//null
};

static struct regval_list sensor_wb_auto_regs[] = {
	{0xfd,0x02},                      
	{0x26,0xc8},		                  
	{0x27,0xb6},                      
	{0xfd,0x01}, 		
	{0x32,0x15},   //awb & ae  opened
	{0xfd,0x00},
};

static struct regval_list sensor_wb_incandescence_regs[] = {
	//bai re guang
	{0xfd,0x01}, 
	{0x32,0x05},                 
	{0xfd,0x02},                 
	{0x26,0x75},		             
	{0x27,0xe2},		             
	{0xfd,0x00}, 
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	//ri guang deng
	{0xfd,0x01},  
	{0x32,0x05},                  
	{0xfd,0x02},                  
	{0x26,0x91},		              
	{0x27,0xc8},		              
	{0xfd,0x00},		
};

static struct regval_list sensor_wb_tungsten_regs[] = {
	//wu si deng
	{0xfd,0x01}, 
	{0x32,0x05},                 
	{0xfd,0x02},                 
	{0x26,0x75},		             
	{0x27,0xe2},		             
	{0xfd,0x00}, 
};

static struct regval_list sensor_wb_horizon[] = { 
//null
};

static struct regval_list sensor_wb_daylight_regs[] = {
	//tai yang guang
	{0xfd,0x01}, 
	{0x32,0x05},           
	{0xfd,0x02},           
	{0x26,0xaa},	         
	{0x27,0xce},	         
	{0xfd,0x00}, 
};

static struct regval_list sensor_wb_flash[] = { 
//null
};


static struct regval_list sensor_wb_cloud_regs[] = {
	{0xfd,0x01}, 
	{0x32,0x05},          
	{0xfd,0x02},          
	{0x26,0xc8},	        
	{0x27,0x89},	        
	{0xfd,0x00},
};

static struct regval_list sensor_wb_shade[] = { 
//null
};

static struct cfg_array sensor_wb[] = {
  { 
  	.regs = sensor_wb_manual,             //V4L2_WHITE_BALANCE_MANUAL       
    .size = ARRAY_SIZE(sensor_wb_manual),
  },
  {
  	.regs = sensor_wb_auto_regs,          //V4L2_WHITE_BALANCE_AUTO      
    .size = ARRAY_SIZE(sensor_wb_auto_regs),
  },
  {
  	.regs = sensor_wb_incandescence_regs, //V4L2_WHITE_BALANCE_INCANDESCENT 
    .size = ARRAY_SIZE(sensor_wb_incandescence_regs),
  },
  {
  	.regs = sensor_wb_fluorescent_regs,   //V4L2_WHITE_BALANCE_FLUORESCENT  
    .size = ARRAY_SIZE(sensor_wb_fluorescent_regs),
  },
  {
  	.regs = sensor_wb_tungsten_regs,      //V4L2_WHITE_BALANCE_FLUORESCENT_H
    .size = ARRAY_SIZE(sensor_wb_tungsten_regs),
  },
  {
  	.regs = sensor_wb_horizon,            //V4L2_WHITE_BALANCE_HORIZON    
    .size = ARRAY_SIZE(sensor_wb_horizon),
  },  
  {
  	.regs = sensor_wb_daylight_regs,      //V4L2_WHITE_BALANCE_DAYLIGHT     
    .size = ARRAY_SIZE(sensor_wb_daylight_regs),
  },
  {
  	.regs = sensor_wb_flash,              //V4L2_WHITE_BALANCE_FLASH        
    .size = ARRAY_SIZE(sensor_wb_flash),
  },
  {
  	.regs = sensor_wb_cloud_regs,         //V4L2_WHITE_BALANCE_CLOUDY       
    .size = ARRAY_SIZE(sensor_wb_cloud_regs),
  },
  {
  	.regs = sensor_wb_shade,              //V4L2_WHITE_BALANCE_SHADE  
    .size = ARRAY_SIZE(sensor_wb_shade),
  },
//  {
//  	.regs = NULL,
//    .size = 0,
//  },
};
                                          

/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {
	{0xfd,0x01},
	{0x66,0x00},
	{0x67,0x80},
	{0x68,0x80},
};

static struct regval_list sensor_colorfx_bw_regs[] = {
	{0xfd,0x01},
	{0x66,0x20},
	{0x67,0x80},
	{0x68,0x80},
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
	{0xfd,0x01},
	{0x66,0x10},
	{0x67,0xc0},
	{0x68,0x20},
};

static struct regval_list sensor_colorfx_negative_regs[] = {
	{0xfd,0x01},
	{0x66,0x04},
	{0x67,0x80},
	{0x68,0x80},
};

static struct regval_list sensor_colorfx_emboss_regs[] = {
	{0xfd,0x01},                                     
	{0x66,0x01},                                                          
	{0x67,0x80},                                                          
	{0x68,0x80}
};

static struct regval_list sensor_colorfx_sketch_regs[] = {
	{0xfd,0x01},                                     
	{0x66,0x02},                                                          
	{0x67,0x80},                                                          
	{0x68,0x80}
};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
	{0xfd,0x01},                                     
	{0x66,0x10},                                                          
	{0x67,0x20},                                                          
	{0x68,0xf0}
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
	{0xfd,0x01},                                     
	{0x66,0x10},                                                          
	{0x67,0x20},                                                          
	{0x68,0x20}
};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
//NULL
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
//NULL
};

static struct regval_list sensor_colorfx_aqua_regs[] = {
//null
};

static struct regval_list sensor_colorfx_art_freeze_regs[] = {
//null
};

static struct regval_list sensor_colorfx_silhouette_regs[] = {
//null
};

static struct regval_list sensor_colorfx_solarization_regs[] = {
//null
};

static struct regval_list sensor_colorfx_antique_regs[] = {
//null
};

static struct regval_list sensor_colorfx_set_cbcr_regs[] = {
//null
};

static struct cfg_array sensor_colorfx[] = {
  {
  	.regs = sensor_colorfx_none_regs,         //V4L2_COLORFX_NONE = 0,         
    .size = ARRAY_SIZE(sensor_colorfx_none_regs),
  },
  {
  	.regs = sensor_colorfx_bw_regs,           //V4L2_COLORFX_BW   = 1,  
    .size = ARRAY_SIZE(sensor_colorfx_bw_regs),
  },
  {
  	.regs = sensor_colorfx_sepia_regs,        //V4L2_COLORFX_SEPIA  = 2,   
    .size = ARRAY_SIZE(sensor_colorfx_sepia_regs),
  },
  {
  	.regs = sensor_colorfx_negative_regs,     //V4L2_COLORFX_NEGATIVE = 3,     
    .size = ARRAY_SIZE(sensor_colorfx_negative_regs),
  },
  {
  	.regs = sensor_colorfx_emboss_regs,       //V4L2_COLORFX_EMBOSS = 4,       
    .size = ARRAY_SIZE(sensor_colorfx_emboss_regs),
  },
  {
  	.regs = sensor_colorfx_sketch_regs,       //V4L2_COLORFX_SKETCH = 5,       
    .size = ARRAY_SIZE(sensor_colorfx_sketch_regs),
  },
  {
  	.regs = sensor_colorfx_sky_blue_regs,     //V4L2_COLORFX_SKY_BLUE = 6,     
    .size = ARRAY_SIZE(sensor_colorfx_sky_blue_regs),
  },
  {
  	.regs = sensor_colorfx_grass_green_regs,  //V4L2_COLORFX_GRASS_GREEN = 7,  
    .size = ARRAY_SIZE(sensor_colorfx_grass_green_regs),
  },
  {
  	.regs = sensor_colorfx_skin_whiten_regs,  //V4L2_COLORFX_SKIN_WHITEN = 8,  
    .size = ARRAY_SIZE(sensor_colorfx_skin_whiten_regs),
  },
  {
  	.regs = sensor_colorfx_vivid_regs,        //V4L2_COLORFX_VIVID = 9,        
    .size = ARRAY_SIZE(sensor_colorfx_vivid_regs),
  },
  {
  	.regs = sensor_colorfx_aqua_regs,         //V4L2_COLORFX_AQUA = 10,        
    .size = ARRAY_SIZE(sensor_colorfx_aqua_regs),
  },
  {
  	.regs = sensor_colorfx_art_freeze_regs,   //V4L2_COLORFX_ART_FREEZE = 11,  
    .size = ARRAY_SIZE(sensor_colorfx_art_freeze_regs),
  },
  {
  	.regs = sensor_colorfx_silhouette_regs,   //V4L2_COLORFX_SILHOUETTE = 12,  
    .size = ARRAY_SIZE(sensor_colorfx_silhouette_regs),
  },
  {
  	.regs = sensor_colorfx_solarization_regs, //V4L2_COLORFX_SOLARIZATION = 13,
    .size = ARRAY_SIZE(sensor_colorfx_solarization_regs),
  },
  {
  	.regs = sensor_colorfx_antique_regs,      //V4L2_COLORFX_ANTIQUE = 14,     
    .size = ARRAY_SIZE(sensor_colorfx_antique_regs),
  },
  {
  	.regs = sensor_colorfx_set_cbcr_regs,     //V4L2_COLORFX_SET_CBCR = 15, 
    .size = ARRAY_SIZE(sensor_colorfx_set_cbcr_regs),
  },
};



/*
 * The brightness setttings
 */
static struct regval_list sensor_brightness_neg4_regs[] = {
	{0xfd,0x01},                                     
	{0xdb,0xc0}
};

static struct regval_list sensor_brightness_neg3_regs[] = {
	{0xfd,0x01},                                     
	{0xdb,0xd0}
};

static struct regval_list sensor_brightness_neg2_regs[] = {
	{0xfd,0x01},                                     
	{0xdb,0xe0}
};

static struct regval_list sensor_brightness_neg1_regs[] = {
	{0xfd,0x01},                                     
	{0xdb,0xf0}
};

static struct regval_list sensor_brightness_zero_regs[] = {
	{0xfd,0x01},                                     
	{0xdb,0x00}
};

static struct regval_list sensor_brightness_pos1_regs[] = {
	{0xfd,0x01},                                     
	{0xdb,0x10}
};

static struct regval_list sensor_brightness_pos2_regs[] = {
	{0xfd,0x01},                                     
	{0xdb,0x20}
};

static struct regval_list sensor_brightness_pos3_regs[] = {
	{0xfd,0x01},                                     
	{0xdb,0x30}
};

static struct regval_list sensor_brightness_pos4_regs[] = {
	{0xfd,0x01},                                     
	{0xdb,0x40}
};

static struct cfg_array sensor_brightness[] = {
  {
  	.regs = sensor_brightness_neg4_regs,
  	.size = ARRAY_SIZE(sensor_brightness_neg4_regs),
  },
  {
  	.regs = sensor_brightness_neg3_regs,
  	.size = ARRAY_SIZE(sensor_brightness_neg3_regs),
  },
  {
  	.regs = sensor_brightness_neg2_regs,
  	.size = ARRAY_SIZE(sensor_brightness_neg2_regs),
  },
  {
  	.regs = sensor_brightness_neg1_regs,
  	.size = ARRAY_SIZE(sensor_brightness_neg1_regs),
  },
  {
  	.regs = sensor_brightness_zero_regs,
  	.size = ARRAY_SIZE(sensor_brightness_zero_regs),
  },
  {
  	.regs = sensor_brightness_pos1_regs,
  	.size = ARRAY_SIZE(sensor_brightness_pos1_regs),
  },
  {
  	.regs = sensor_brightness_pos2_regs,
  	.size = ARRAY_SIZE(sensor_brightness_pos2_regs),
  },
  {
  	.regs = sensor_brightness_pos3_regs,
  	.size = ARRAY_SIZE(sensor_brightness_pos3_regs),
  },
  {
  	.regs = sensor_brightness_pos4_regs,
  	.size = ARRAY_SIZE(sensor_brightness_pos4_regs),
  },
};

/*
 * The contrast setttings
 */
static struct regval_list sensor_contrast_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_zero_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos4_regs[] = {
};

static struct cfg_array sensor_contrast[] = {
  {
  	.regs = sensor_contrast_neg4_regs,
  	.size = ARRAY_SIZE(sensor_contrast_neg4_regs),
  },
  {
  	.regs = sensor_contrast_neg3_regs,
  	.size = ARRAY_SIZE(sensor_contrast_neg3_regs),
  },
  {
  	.regs = sensor_contrast_neg2_regs,
  	.size = ARRAY_SIZE(sensor_contrast_neg2_regs),
  },
  {
  	.regs = sensor_contrast_neg1_regs,
  	.size = ARRAY_SIZE(sensor_contrast_neg1_regs),
  },
  {
  	.regs = sensor_contrast_zero_regs,
  	.size = ARRAY_SIZE(sensor_contrast_zero_regs),
  },
  {
  	.regs = sensor_contrast_pos1_regs,
  	.size = ARRAY_SIZE(sensor_contrast_pos1_regs),
  },
  {
  	.regs = sensor_contrast_pos2_regs,
  	.size = ARRAY_SIZE(sensor_contrast_pos2_regs),
  },
  {
  	.regs = sensor_contrast_pos3_regs,
  	.size = ARRAY_SIZE(sensor_contrast_pos3_regs),
  },
  {
  	.regs = sensor_contrast_pos4_regs,
  	.size = ARRAY_SIZE(sensor_contrast_pos4_regs),
  },
};

/*
 * The saturation setttings
 */
static struct regval_list sensor_saturation_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_zero_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos4_regs[] = {
//NULL
};

static struct cfg_array sensor_saturation[] = {
  {
  	.regs = sensor_saturation_neg4_regs,
  	.size = ARRAY_SIZE(sensor_saturation_neg4_regs),
  },
  {
  	.regs = sensor_saturation_neg3_regs,
  	.size = ARRAY_SIZE(sensor_saturation_neg3_regs),
  },
  {
  	.regs = sensor_saturation_neg2_regs,
  	.size = ARRAY_SIZE(sensor_saturation_neg2_regs),
  },
  {
  	.regs = sensor_saturation_neg1_regs,
  	.size = ARRAY_SIZE(sensor_saturation_neg1_regs),
  },
  {
  	.regs = sensor_saturation_zero_regs,
  	.size = ARRAY_SIZE(sensor_saturation_zero_regs),
  },
  {
  	.regs = sensor_saturation_pos1_regs,
  	.size = ARRAY_SIZE(sensor_saturation_pos1_regs),
  },
  {
  	.regs = sensor_saturation_pos2_regs,
  	.size = ARRAY_SIZE(sensor_saturation_pos2_regs),
  },
  {
  	.regs = sensor_saturation_pos3_regs,
  	.size = ARRAY_SIZE(sensor_saturation_pos3_regs),
  },
  {
  	.regs = sensor_saturation_pos4_regs,
  	.size = ARRAY_SIZE(sensor_saturation_pos4_regs),
  },
};

/*
 * The exposure target setttings
 */
static struct regval_list sensor_ev_neg4_regs[] = {
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb-0x20},
	{0xec,SP0718_P1_0xec-0x20},
	{0xdb,0x00}
};

static struct regval_list sensor_ev_neg3_regs[] = {
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb-0x18},
	{0xec,SP0718_P1_0xec-0x18},
	{0xdb,0x00}
};

static struct regval_list sensor_ev_neg2_regs[] = {
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb-0x10},
	{0xec,SP0718_P1_0xec-0x10},
	{0xdb,0x00}
};

static struct regval_list sensor_ev_neg1_regs[] = {
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb-0x08},
	{0xec,SP0718_P1_0xec-0x08},
	{0xdb,0x00}
};

static struct regval_list sensor_ev_zero_regs[] = {
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb},
	{0xec,SP0718_P1_0xec},
	{0xdb,0x00}
};

static struct regval_list sensor_ev_pos1_regs[] = {
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb+0x08},
	{0xec,SP0718_P1_0xec+0x08},
	{0xdb,0x00}
};

static struct regval_list sensor_ev_pos2_regs[] = {
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb+0x10},
	{0xec,SP0718_P1_0xec+0x10},
	{0xdb,0x00}
};

static struct regval_list sensor_ev_pos3_regs[] = {
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb+0x18},
	{0xec,SP0718_P1_0xec+0x18},
	{0xdb,0x00}
};

static struct regval_list sensor_ev_pos4_regs[] = {
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb+0x20},
	{0xec,SP0718_P1_0xec+0x20},
	{0xdb,0x00}
};

static struct cfg_array sensor_ev[] = {
  {
  	.regs = sensor_ev_neg4_regs,
  	.size = ARRAY_SIZE(sensor_ev_neg4_regs),
  },
  {
  	.regs = sensor_ev_neg3_regs,
  	.size = ARRAY_SIZE(sensor_ev_neg3_regs),
  },
  {
  	.regs = sensor_ev_neg2_regs,
  	.size = ARRAY_SIZE(sensor_ev_neg2_regs),
  },
  {
  	.regs = sensor_ev_neg1_regs,
  	.size = ARRAY_SIZE(sensor_ev_neg1_regs),
  },
  {
  	.regs = sensor_ev_zero_regs,
  	.size = ARRAY_SIZE(sensor_ev_zero_regs),
  },
  {
  	.regs = sensor_ev_pos1_regs,
  	.size = ARRAY_SIZE(sensor_ev_pos1_regs),
  },
  {
  	.regs = sensor_ev_pos2_regs,
  	.size = ARRAY_SIZE(sensor_ev_pos2_regs),
  },
  {
  	.regs = sensor_ev_pos3_regs,
  	.size = ARRAY_SIZE(sensor_ev_pos3_regs),
  },
  {
  	.regs = sensor_ev_pos4_regs,
  	.size = ARRAY_SIZE(sensor_ev_pos4_regs),
  },
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 * 
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	{0xfd,0x01},//Page 0
//	{0x35,0x40}	//YCbYCr
};

static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{0xfd,0x01},//Page 0
//	{0x35,0x41}	//YCrYCb
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{0xfd,0x01},//Page 0
//	{0x35,0x01}	//CrYCbY
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{0xfd,0x01},//Page 0
//	{0x35,0x00}	//CbYCrY
};

//static struct regval_list sensor_fmt_raw[] = {
//	//raw
//};



/*
 * Low-level register I/O.
 *
 */


/*
 * On most platforms, we'd rather do straight i2c I/O.
 */
static int sensor_read(struct v4l2_subdev *sd, unsigned char reg,
    unsigned char *value) //!!!!be careful of the para type!!!
{
	int ret=0;
	int cnt=0;
	
  struct i2c_client *client = v4l2_get_subdevdata(sd);
  ret = cci_read_a8_d8(client,reg,value);
  while(ret!=0&&cnt<2)
  {
  	ret = cci_read_a8_d8(client,reg,value);
  	cnt++;
  }
  if(cnt>0)
  	vfe_dev_dbg("sensor read retry=%d\n",cnt);
  
  return ret;
}

static int sensor_write(struct v4l2_subdev *sd, unsigned char reg,
    unsigned char value)
{
	int ret=0;
	int cnt=0;
	
  struct i2c_client *client = v4l2_get_subdevdata(sd);
  
  ret = cci_write_a8_d8(client,reg,value);
  while(ret!=0&&cnt<2)
  {
  	ret = cci_write_a8_d8(client,reg,value);
  	cnt++;
  }
  if(cnt>0)
  	vfe_dev_dbg("sensor write retry=%d\n",cnt);
  
  return ret;
}

/*
 * Write a list of register settings;
 */
static int sensor_write_array(struct v4l2_subdev *sd, struct regval_list *regs, int array_size)
{
	int i=0;
	
  if(!regs)
  	return 0;
  	//return -EINVAL;
  
  while(i<array_size)
  {
    if(regs->addr == REG_DLY) {
      msleep(regs->data);
    } 
    else {
    	//printk("write 0x%x=0x%x\n", regs->addr, regs->data);
      LOG_ERR_RET(sensor_write(sd, regs->addr, regs->data))
    }
    i++;
    regs++;
  }
  return 0;
}

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  LOG_ERR_RET(sensor_write(sd, 0xfd, 0x00)) //page 0
  LOG_ERR_RET(sensor_read(sd, 0x31, &rdval))
  
  rdval &= (1<<5);  
  *value = (rdval>>5);
  
  info->vflip = *value;
  return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  if(info->hflip == value)
    return 0;
    
  LOG_ERR_RET(sensor_write(sd, 0xfd, 0x00)) //page 0 
  LOG_ERR_RET(sensor_read(sd, 0x31, &rdval))
  
  switch (value) {
    case 0:
      rdval &= 0xdf;
      break;
    case 1:
      rdval |= 0x20;
      break;
    default:
      return -EINVAL;
  }
  
  LOG_ERR_RET(sensor_write(sd, 0x31, rdval))
  
  usleep_range(10000,12000);
  info->hflip = value;
  return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  LOG_ERR_RET(sensor_write(sd, 0xfd, 0x00)) //page 0
  LOG_ERR_RET(sensor_read(sd, 0x31, &rdval))
  
  rdval &= (1<<6);  
  *value = (rdval>>6);
  
  info->vflip = *value;
  return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  if(info->vflip == value)
    return 0;
    
  LOG_ERR_RET(sensor_write(sd, 0xfd, 0x00)) //page 0
  LOG_ERR_RET(sensor_read(sd, 0x31, &rdval))

  switch (value) {
    case 0:
      rdval &= 0xbf;
      break;
    case 1:
      rdval |= 0x40;
      break;
    default:
      return -EINVAL;
  }

  LOG_ERR_RET(sensor_write(sd, 0x31, rdval))
  
  usleep_range(10000,12000);
  info->vflip = value;
  return 0;
}

static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;
	
	ret = sensor_write(sd, 0xfd, 0x01);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_g_autoexp!\n");
		return ret;
	}
	
	ret = sensor_read(sd, 0x32, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_g_autoexp!\n");
		return ret;
	}

	val &= SP0718_AE_EN;
	if (val == SP0718_AE_EN) {
		*value = V4L2_EXPOSURE_AUTO;
	}
	else
	{
		*value = V4L2_EXPOSURE_MANUAL;
	}
	
	info->autoexp = *value;
	return 0;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
		enum v4l2_exposure_auto_type value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;
	
	ret = sensor_write(sd, 0xfd, 0x01);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_autoexp!\n");
		return ret;
	}
	
	ret = sensor_read(sd, 0x32, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_s_autoexp!\n");
		return ret;
	}

	switch (value) {
		case V4L2_EXPOSURE_AUTO:
		  val |= SP0718_AE_EN;
			break;
		case V4L2_EXPOSURE_MANUAL:
			val &= SP0718_AE_DIS;
			break;
		case V4L2_EXPOSURE_SHUTTER_PRIORITY:
			return -EINVAL;    
		case V4L2_EXPOSURE_APERTURE_PRIORITY:
			return -EINVAL;
		default:
			return -EINVAL;
	}
		
	ret = sensor_write(sd, 0x32, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_autoexp!\n");
		return ret;
	}
	
	usleep_range(10000,12000);
	
	info->autoexp = value;
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;
	
	ret = sensor_write(sd, 0xfd, 0x01);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_g_autowb!\n");
		return ret;
	}
	
	ret = sensor_read(sd, 0x32, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_g_autowb!\n");
		return ret;
	}

	val &= (1<<4);
	val = val>>4;		//0x32 bit4 is awb enable
		
	*value = val;
	info->autowb = *value;
	
	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;
	
	ret = sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs));
	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_autowb!\n");
		return ret;
	}
	
	ret = sensor_write(sd, 0xfd, 0x01);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_autowb!\n");
		return ret;
	}
	ret = sensor_read(sd, 0x32, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_s_autowb!\n");
		return ret;
	}

	switch(value) {
	case 0:
		val &= SP0718_AWB_DIS;
		break;
	case 1:
		val |= SP0718_AWB_EN;
		break;
	default:
		break;
	}	
	ret = sensor_write(sd, 0x32, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_autowb!\n");
		return ret;
	}
	
	usleep_range(10000,12000);
	
	info->autowb = value;
	return 0;
}

static int sensor_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_hue(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}
/* *********************************************end of ******************************************** */

static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  
  *value = info->brightness;
  return 0;
}

static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
  struct sensor_info *info = to_state(sd);
  
  if(info->brightness == value)
    return 0;
  
  if(value < -4 || value > 4)
    return -ERANGE;
  
  LOG_ERR_RET(sensor_write_array(sd, sensor_brightness[value+4].regs, sensor_brightness[value+4].size))

  info->brightness = value;
  return 0;
}

static int sensor_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  
  *value = info->contrast;
  return 0;
}

static int sensor_s_contrast(struct v4l2_subdev *sd, int value)
{
  struct sensor_info *info = to_state(sd);
  
  if(info->contrast == value)
    return 0;
  
  if(value < -4 || value > 4)
    return -ERANGE;
    
  LOG_ERR_RET(sensor_write_array(sd, sensor_contrast[value+4].regs, sensor_contrast[value+4].size))
  
  info->contrast = value;
  return 0;
}

static int sensor_g_saturation(struct v4l2_subdev *sd, __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  
  *value = info->saturation;
  return 0;
}

static int sensor_s_saturation(struct v4l2_subdev *sd, int value)
{
  struct sensor_info *info = to_state(sd);
  
  if(info->saturation == value)
    return 0;

  if(value < -4 || value > 4)
    return -ERANGE;
      
  LOG_ERR_RET(sensor_write_array(sd, sensor_saturation[value+4].regs, sensor_saturation[value+4].size))

  info->saturation = value;
  return 0;
}

static int sensor_g_exp_bias(struct v4l2_subdev *sd, __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  
  *value = info->exp_bias;
  return 0;
}

static int sensor_s_exp_bias(struct v4l2_subdev *sd, int value)
{
  struct sensor_info *info = to_state(sd);

  if(info->exp_bias == value)
    return 0;

  if(value < -4 || value > 4)
    return -ERANGE;
      
  LOG_ERR_RET(sensor_write_array(sd, sensor_ev[value+4].regs, sensor_ev[value+4].size))

  info->exp_bias = value;
  return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
  struct sensor_info *info = to_state(sd);
  enum v4l2_auto_n_preset_white_balance *wb_type = (enum v4l2_auto_n_preset_white_balance*)value;
  
  *wb_type = info->wb;
  
  return 0;
}

static int sensor_s_wb(struct v4l2_subdev *sd,
    enum v4l2_auto_n_preset_white_balance value)
{
  struct sensor_info *info = to_state(sd);
  
  if(info->capture_mode == V4L2_MODE_IMAGE)
    return 0;
  
  if(info->wb == value)
    return 0;
  
  LOG_ERR_RET(sensor_write_array(sd, sensor_wb[value].regs ,sensor_wb[value].size) )
  
  if (value == V4L2_WHITE_BALANCE_AUTO) 
    info->autowb = 1;
  else
    info->autowb = 0;
	
	info->wb = value;
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd,
		__s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx*)value;
	
	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd,
    enum v4l2_colorfx value)
{
  struct sensor_info *info = to_state(sd);

  if(info->clrfx == value)
    return 0;
  
  LOG_ERR_RET(sensor_write_array(sd, sensor_colorfx[value].regs, sensor_colorfx[value].size))

  info->clrfx = value;
  return 0;
}

static int sensor_g_flash_mode(struct v4l2_subdev *sd,
    __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  enum v4l2_flash_led_mode *flash_mode = (enum v4l2_flash_led_mode*)value;
  
  *flash_mode = info->flash_mode;
  return 0;
}

static int sensor_s_flash_mode(struct v4l2_subdev *sd,
    enum v4l2_flash_led_mode value)
{
  struct sensor_info *info = to_state(sd);
//  struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
//  int flash_on,flash_off;
//  
//  flash_on = (dev->flash_pol!=0)?1:0;
//  flash_off = (flash_on==1)?0:1;
//  
//  switch (value) {
//  case V4L2_FLASH_MODE_OFF:
//    os_gpio_write(&dev->flash_io,flash_off);
//    break;
//  case V4L2_FLASH_MODE_AUTO:
//    return -EINVAL;
//    break;  
//  case V4L2_FLASH_MODE_ON:
//    os_gpio_write(&dev->flash_io,flash_on);
//    break;   
//  case V4L2_FLASH_MODE_TORCH:
//    return -EINVAL;
//    break;
//  case V4L2_FLASH_MODE_RED_EYE:   
//    return -EINVAL;
//    break;
//  default:
//    return -EINVAL;
//  }
  
  info->flash_mode = value;
  return 0;
}

//static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
//{
//	int ret=0;
////	unsigned char rdval;
////	
////	ret=sensor_read(sd, 0x00, &rdval);
////	if(ret!=0)
////		return ret;
////	
////	if(on_off==CSI_STBY_ON)//sw stby on
////	{
////		ret=sensor_write(sd, 0x00, rdval&0x7f);
////	}
////	else//sw stby off
////	{
////		ret=sensor_write(sd, 0x00, rdval|0x80);
////	}
//	return ret;
//}

/*
 * Stuff that knows about the sensor.
 */
 
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
  //make sure that no device can access i2c bus during sensor initial or power down
  //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
  i2c_lock_adapter(client->adapter);
  
  //insure that clk_disable() and clk_enable() are called in pair 
  //when calling CSI_SUBDEV_STBY_ON/OFF and CSI_SUBDEV_PWR_ON/OFF  
  switch(on)
  {
    case CSI_SUBDEV_STBY_ON:
      vfe_dev_dbg("CSI_SUBDEV_STBY_ON\n");
      //standby on io
      vfe_gpio_write(sd,PWDN,CSI_STBY_ON);
      usleep_range(30000,31000);
      //inactive mclk after stadby in
      vfe_set_mclk(sd,OFF);
      break;
    case CSI_SUBDEV_STBY_OFF:
      vfe_dev_dbg("CSI_SUBDEV_STBY_OFF\n");
      //active mclk before stadby out
      vfe_set_mclk_freq(sd,MCLK);
      vfe_set_mclk(sd,ON);
      usleep_range(30000,31000);
      //standby off io
      vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
      usleep_range(10000,12000);
//			//reset off io
//			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
//			usleep_range(30000,31000);
      break;
    case CSI_SUBDEV_PWR_ON:
      vfe_dev_dbg("CSI_SUBDEV_PWR_ON\n");
      //power on reset
      vfe_gpio_set_status(sd,PWDN,1);//set the gpio to output
      vfe_gpio_set_status(sd,RESET,1);//set the gpio to output
      usleep_range(10000,12000);
      //standby off io
      vfe_gpio_write(sd,PWDN,CSI_STBY_ON);
			//reset on io
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
      //power supply
      vfe_gpio_write(sd,POWER_EN,CSI_PWR_ON);
      vfe_set_pmu_channel(sd,AVDD,ON);
      vfe_set_pmu_channel(sd,IOVDD,ON);

      vfe_set_pmu_channel(sd,DVDD,ON);
      vfe_set_pmu_channel(sd,AFVDD,ON);
      usleep_range(20000,22000);
      //standby off io
      vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
      usleep_range(10000,12000);
			//active mclk
      vfe_set_mclk_freq(sd,MCLK);
      vfe_set_mclk(sd,ON);
      usleep_range(10000,12000);
			//reset on io
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
			usleep_range(30000,31000);
			//reset off io
      vfe_gpio_write(sd,RESET,CSI_RST_OFF);
			usleep_range(30000,31000);
			break;
		case CSI_SUBDEV_PWR_OFF:
      vfe_dev_dbg("CSI_SUBDEV_PWR_OFF\n");
      //reset io
      usleep_range(10000,12000);
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
			usleep_range(10000,12000);
			//inactive mclk after power off
      vfe_set_mclk(sd,OFF);
      //power supply off
      vfe_gpio_write(sd,POWER_EN,CSI_PWR_OFF);
      vfe_set_pmu_channel(sd,AFVDD,OFF);
      vfe_set_pmu_channel(sd,DVDD,OFF);
      vfe_set_pmu_channel(sd,AVDD,OFF);
      vfe_set_pmu_channel(sd,IOVDD,OFF);  
      usleep_range(10000,12000);
      //standby and reset io
			//standby of io
      vfe_gpio_write(sd,PWDN,CSI_STBY_ON);
      usleep_range(10000,12000);
      //set the io to hi-z
      vfe_gpio_set_status(sd,RESET,0);//set the gpio to input
      vfe_gpio_set_status(sd,PWDN,0);//set the gpio to input
			break;
		default:
			return -EINVAL;
	}		

	//remember to unlock i2c adapter, so the device can access the i2c bus again
	i2c_unlock_adapter(client->adapter);	
	return 0;
}
 
static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
  switch(val)
  {
    case 0:
      vfe_gpio_write(sd,RESET,CSI_RST_OFF);
      usleep_range(10000,12000);
      break;
    case 1:
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
      usleep_range(10000,12000);
      break;
    default:
      return -EINVAL;
  }
    
  return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret;
	unsigned char val;
	
	ret = sensor_write(sd, 0xfd, 0x00);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_detect!\n");
		return ret;
	}
	
	ret = sensor_read(sd, 0x02, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_detect!\n");
		return ret;
	}

	if(val != 0x71)
		return -ENODEV;
	
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	vfe_dev_dbg("sensor_init\n");
	
	/*Make sure it is a target sensor*/
	ret = sensor_detect(sd);
	if (ret) {
		vfe_dev_err("chip found is not an target chip.\n");
		return ret;
	}              
	usleep_range(200000,250000);
	return sensor_write_array(sd, sensor_Q7011_regs , ARRAY_SIZE(sensor_Q7011_regs));
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret=0;
		return ret;
}


/*
 * Store information about the video data format. 
 */
static struct sensor_format_struct {
	__u8 *desc;
	//__u32 pixelformat;
	enum v4l2_mbus_pixelcode mbus_code;//linux-3.0
	struct regval_list *regs;
	int	regs_size;
	int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,//linux-3.0
		.regs 		= sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp		= 2,
	},
	{
		.desc		= "YVYU 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_YVYU8_2X8,//linux-3.0
		.regs 		= sensor_fmt_yuv422_yvyu,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
		.bpp		= 2,
	},
	{
		.desc		= "UYVY 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_UYVY8_2X8,//linux-3.0
		.regs 		= sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp		= 2,
	},
	{
		.desc		= "VYUY 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_VYUY8_2X8,//linux-3.0
		.regs 		= sensor_fmt_yuv422_vyuy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
		.bpp		= 2,
	},
//	{
//		.desc		= "Raw RGB Bayer",
//		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,//linux-3.0
//		.regs 		= sensor_fmt_raw,
//		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
//		.bpp		= 1
//	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)


/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */


static struct sensor_win_size 
sensor_win_sizes[] = {
  /* VGA */
  {
    .width      = VGA_WIDTH,
    .height     = VGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = NULL,
    .regs_size  = 0,
    .set_size   = NULL,
  },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
                 enum v4l2_mbus_pixelcode *code)
{
  if (index >= N_FMTS)
    return -EINVAL;

  *code = sensor_formats[index].mbus_code;
  return 0;
}

static int sensor_enum_size(struct v4l2_subdev *sd,
                            struct v4l2_frmsizeenum *fsize)
{
  if(fsize->index > N_WIN_SIZES-1)
  	return -EINVAL;
  
  fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
  fsize->discrete.width = sensor_win_sizes[fsize->index].width;
  fsize->discrete.height = sensor_win_sizes[fsize->index].height;
  
  return 0;
}


static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
    struct v4l2_mbus_framefmt *fmt,
    struct sensor_format_struct **ret_fmt,
    struct sensor_win_size **ret_wsize)
{
  int index;
  struct sensor_win_size *wsize;

  for (index = 0; index < N_FMTS; index++)
    if (sensor_formats[index].mbus_code == fmt->code)
      break;

  if (index >= N_FMTS) 
    return -EINVAL;
  
  if (ret_fmt != NULL)
    *ret_fmt = sensor_formats + index;
    
  /*
   * Fields: the sensor devices claim to be progressive.
   */
  
  fmt->field = V4L2_FIELD_NONE;
  
  /*
   * Round requested image size down to the nearest
   * we support, but not below the smallest.
   */
  for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
       wsize++)
    if (fmt->width >= wsize->width && fmt->height >= wsize->height)
      break;
    
  if (wsize >= sensor_win_sizes + N_WIN_SIZES)
    wsize--;   /* Take the smallest one */
  if (ret_wsize != NULL)
    *ret_wsize = wsize;
  /*
   * Note the size we'll actually handle.
   */
  fmt->width = wsize->width;
  fmt->height = wsize->height;
  //pix->bytesperline = pix->width*sensor_formats[index].bpp;
  //pix->sizeimage = pix->height*pix->bytesperline;

  return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd, 
             struct v4l2_mbus_framefmt *fmt)//linux-3.0
{
	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
           struct v4l2_mbus_config *cfg)
{
  cfg->type = V4L2_MBUS_PARALLEL;
  cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL ;
  
  return 0;
}

/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd, 
             struct v4l2_mbus_framefmt *fmt)//linux-3.0
{
	int ret;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);
	vfe_dev_dbg("sensor_s_fmt\n");
	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;
	
		
	sensor_write_array(sd, sensor_fmt->regs , sensor_fmt->regs_size);
	
	ret = 0;
	if (wsize->regs)
	{
		ret = sensor_write_array(sd, wsize->regs , wsize->regs_size);
		if (ret < 0)
			return ret;
	}
	
	if (wsize->set_size)
	{
		ret = wsize->set_size(sd);
		if (ret < 0)
			return ret;
	}
	
	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	
	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	//struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	cp->timeperframe.denominator = SENSOR_FRAME_RATE;
	
	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
//	struct v4l2_captureparm *cp = &parms->parm.capture;
	//struct v4l2_fract *tpf = &cp->timeperframe;
	//struct sensor_info *info = to_state(sd);
	//int div;

//	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
//		return -EINVAL;
//	if (cp->extendedmode != 0)
//		return -EINVAL;

//	if (tpf->numerator == 0 || tpf->denominator == 0)
//		div = 1;  /* Reset to full rate */
//	else
//		div = (tpf->numerator*SENSOR_FRAME_RATE)/tpf->denominator;
//		
//	if (div == 0)
//		div = 1;
//	else if (div > CLK_SCALE)
//		div = CLK_SCALE;
//	info->clkrc = (info->clkrc & 0x80) | div;
//	tpf->numerator = 1;
//	tpf->denominator = sensor_FRAME_RATE/div;
//sensor_write(sd, REG_CLKRC, info->clkrc);
	return 0;
}


/* 
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */
static int sensor_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */
	/* see sensor_s_parm and sensor_g_parm for the meaning of value */
	
	switch (qc->id) {
//	case V4L2_CID_BRIGHTNESS:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_CONTRAST:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_SATURATION:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_HUE:
//		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
//	case V4L2_CID_GAIN:
//		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
//	case V4L2_CID_AUTOGAIN:
//		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_EXPOSURE:
  case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
  case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
    return v4l2_ctrl_query_fill(qc, 0, 9, 1, 1);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_COLORFX:
    return v4l2_ctrl_query_fill(qc, 0, 15, 1, 0);
  case V4L2_CID_FLASH_LED_MODE:
	  return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);	
	}
	return -EINVAL;
}


static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_g_brightness(sd, &ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_g_contrast(sd, &ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_g_saturation(sd, &ctrl->value);
	case V4L2_CID_HUE:
		return sensor_g_hue(sd, &ctrl->value);	
	case V4L2_CID_VFLIP:
		return sensor_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_g_hflip(sd, &ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_g_autogain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
  case V4L2_CID_AUTO_EXPOSURE_BIAS:
    return sensor_g_exp_bias(sd, &ctrl->value);
  case V4L2_CID_EXPOSURE_AUTO:
    return sensor_g_autoexp(sd, &ctrl->value);
  case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
    return sensor_g_wb(sd, &ctrl->value);
  case V4L2_CID_AUTO_WHITE_BALANCE:
    return sensor_g_autowb(sd, &ctrl->value);
  case V4L2_CID_COLORFX:
    return sensor_g_colorfx(sd, &ctrl->value);
  case V4L2_CID_FLASH_LED_MODE:
    return sensor_g_flash_mode(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
  struct v4l2_queryctrl qc;
  int ret;
  
//  vfe_dev_dbg("sensor_s_ctrl ctrl->id=0x%8x\n", ctrl->id);
  qc.id = ctrl->id;
  ret = sensor_queryctrl(sd, &qc);
  if (ret < 0) {
    return ret;
  }

	if (qc.type == V4L2_CTRL_TYPE_MENU ||
		qc.type == V4L2_CTRL_TYPE_INTEGER ||
		qc.type == V4L2_CTRL_TYPE_BOOLEAN)
	{
	  if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
	    return -ERANGE;
	  }
	}
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_s_brightness(sd, ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_s_contrast(sd, ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_s_saturation(sd, ctrl->value);
	case V4L2_CID_HUE:
		return sensor_s_hue(sd, ctrl->value);		
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
    case V4L2_CID_AUTO_EXPOSURE_BIAS:
      return sensor_s_exp_bias(sd, ctrl->value);
    case V4L2_CID_EXPOSURE_AUTO:
      return sensor_s_autoexp(sd,
          (enum v4l2_exposure_auto_type) ctrl->value);
    case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
  		return sensor_s_wb(sd,
          (enum v4l2_auto_n_preset_white_balance) ctrl->value); 
    case V4L2_CID_AUTO_WHITE_BALANCE:
      return sensor_s_autowb(sd, ctrl->value);
    case V4L2_CID_COLORFX:
      return sensor_s_colorfx(sd,
          (enum v4l2_colorfx) ctrl->value);
    case V4L2_CID_FLASH_LED_MODE:
      return sensor_s_flash_mode(sd,
          (enum v4l2_flash_led_mode) ctrl->value);
	}
	return -EINVAL;
}


static int sensor_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}


/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_chip_ident = sensor_g_chip_ident,
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
  .enum_mbus_fmt = sensor_enum_fmt,
  .enum_framesizes = sensor_enum_size,
  .try_mbus_fmt = sensor_try_fmt,
  .s_mbus_fmt = sensor_s_fmt,
  .s_parm = sensor_s_parm,
  .g_parm = sensor_g_parm,
  .g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
};

/* ----------------------------------------------------------------------- */

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
//	int ret;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &sensor_ops);

	//client->addr=0x42>>1;

	info->fmt = &sensor_formats[0];
	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp = 0;
	info->autoexp = 0;
	info->autowb = 1;
	info->wb = 0;
	info->clrfx = 0;
	
	return 0;
}


static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{ "sp0718", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

//linux-3.0
static struct i2c_driver sensor_driver = {
	.driver = {
		.owner = THIS_MODULE,
	  .name = "sp0718",
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static __init int init_sensor(void)
{
	return i2c_add_driver(&sensor_driver);
}

static __exit void exit_sensor(void)
{
  i2c_del_driver(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
