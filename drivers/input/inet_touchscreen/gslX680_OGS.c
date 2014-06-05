/*
 * drivers/input/touchscreen/gslX680.c
 *
 * Copyright (c) 2012 Shanghai Basewin
 *	Guan Yuwei<guanyuwei@basewin.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
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

#include "gslX680_OGS.h" //resolution:1024*768

#include "GslChipAjust.h"


//Q71
#include "Q71_GSL2682B_1280800_OGS_DZ.h"


//Q790
//OGS
#include "Q790_GSL2682B_1024768_OGS_DZ_79A1.h"
#include "Q790_GSL2682B_1024768_OGS_DZ_79F1.h"

//GG
#include "Q790_GSL2682B_1024768_GG_QSD.h"
#include "Q790_GSL2682B_1024768_PG_INET.h"
#include "Q790_GSL2682B_1024768_PG_YLD.h"

//Q72
//OGS
#include "Q72_GSL1688E_1024600_OGS_DZ_70L1.h"
#include "Q72_GSL1688E_1024600_OGS_DZ_70E2.h"

//PG
#include "Q72_GSL1688E_1024600_PG_LHJ.h"
#include "Q72_GSL1688E_800480_PG_LHJ.h"


//Q7011
#include "Q7011_GSL1680E_1024600_PG.h"
#include "Q7011_GSL1680E_1024600_PG_DPT.h"

//Q791
//OGS
#include "Q791_GSL915_1024768_OGS_DZ_79D2.h"
#include "Q791_GSL915_1024768_OGS_DZ_79D4.h"
#include "Q791_GSL915_1024768_OGS_SG5908A.h"
#include "Q791_GSL915_1024768_OGS_JND.h"

//GF
#include "Q791_GSL915_1024768_GF_QSD.h"

//GG
#include "Q791_GSL915_1024768_GG_DPT.h"

//PG
#include "Q791_GSL915_1024768_PG_QSD.h"
#include "Q791_GSL915_1024768_PG_XLL.h"




// Q102
#include "Q102_GSL3675B_1024600_GG_QSD.h"
#include "Q102_GSL3675B_1024600_GG_DH.h"
#include "Q102_GSL3675B_1024600_PG_QSD.h"
#include "Q102_GSL3675B_PG_DH_1024600.h"
#include "Q102_GSL3675B_PG_DPT_1024600.h"
#include "Q102_GSL3675B_PG_LHJ_1024600.h"
#include "Q102_GSL3675B_1024600_GG_HT_JH.h"
#include "Q102_GSL3675B_1024600_PG_GG.h"
#include "Q102_GSL3675B_1024600_GG_QLT.h"


//Q92
#include "Q92_GSL3675B_GG_800480_QSD.h"
#include "Q92_GSL3675B_PG_800480_DH.h"
#include "Q92_GSL3675B_GG_800480_DH.h"
#include "Q92_GSL3675B_PG_800480_QSD.h"
#include "Q92_GSL3675B_PG_800480_LHJ.h"

#include "Q92_GSL3675B_PG_1024600_QSD.h"
#include "Q92_GSL3675B_GG_1024600_QSD.h"
#include "Q92_GSL3675B_GG_1024600_DH.h"
#include "Q92_GSL3675B_PG_1024600_DH.h"
#include "Q92_GSL3675B_PG_LHJ_1024600.h"

//Q7012
#include "Q7012_GSL1680E_1024600_PG.h"

//Q101
#include "Q101_GSL3675B_1024600_GG.h"

//Q680
#include "Q680_GSL3676B_1280800_OGS_DZ.h"

//Q680_7inch
#include "Q680_7inch_GSL3676B_1024600_OGS_DZ.h"

//Q108
#include "Q108_GSL3675B_1280800_GG_CTD.h"
#include "Q108_GSL3675B_1280800_PG_CTD.h"
#include "Q108_GSL3675B_1280800_PG_QLT.h"



//===================Customer=================
#include "Customer_info.h"
//#include "Customer_Q70_GSL1680E_1024600_GG_YJ_JIUZHOU.h"
//#include "Customer_Q70_GSL1680E_1024600_PG_HONGSHAN.h"
//#include "Customer_Q72_GSL1688E_1024600_OGS_CTD.h"
//#include "Customer_Q72_GSL1688E_1024600_OGS_DZ_70E2_20131204_FOR_XIANGCHENG.h"
//#include "Customer_Q72_GSL1688E_1024600_OGS_DZ_70E2_20131221_FOR_XIANGCHENG.h"
//#include "Customer_Q72_GSL1688E_1024600_OGS_HS.h"
//#include "Customer_Q72_GSL1688E_1024600_OGS_JND_S1214.h"
//#include "Customer_Q72_GSL1688E_1024600_OGS_XCL_GERUIBANG.h"
//#include "Customer_Q72_GSL1688E_1024600_PG_DX"
//#include "Customer_Q72_GSL1688E_1024600_PG_MX_DX.h"
//#include "Customer_Q72_GSL1688E_1024600_PG_SHX.h"

//#include "Customer_Q790_GSL2682B_1024768_GG_DH.h"
//#include "Customer_Q790_GSL2682B_1024768_GG_DPT.h"
//#include "Customer_Q790_GSL2682B_1024768_GG_DZH.h"
//#include "Customer_Q790_GSL2682B_1024768_GG_SG.h"

//#include "Customer_Q791_GSL915_1024768_G1F_HS.h"
//#include "Customer_Q791_GSL915_1024768_GG_CTD.h"
//#include "Customer_Q791_GSL915_1024768_OGS_JND_MINGZHI.h"
//#include "Customer_Q791_GSL915_1024768_OGS_SG5908A_JIUZHOU.h"
//#include "Customer_Q791_GSL915_1024768_OGS_SG5908A_PUCHENG.h"
//#include "Customer_Q791_GSL915_1024768_PG_CTD.h"


//#include "Customer_Q102_GSL3675B_1024600_GG_GG_RUIFULIANKE.h" 


//mbg ++ 20131230
//#define GSL_MONITOR //This is for ESD

#ifdef GSL_MONITOR
static struct delayed_work gsl_monitor_work;
static struct workqueue_struct *gsl_monitor_workqueue = NULL;
static char int_1st[4] = {0};
static char int_2nd[4] = {0};
static char dac_counter = 0;
static struct i2c_client *gsl_client = NULL;
#endif 

//mbg --
//#define FOR_TSLIB_TEST
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
#define MAX_FINGERS 		10//5 //最大手指个数
#define MAX_CONTACTS 		10
#define DMA_TRANS_LEN		0x20

#define PHO_CFG2_OFFSET	(0X104)
#define PHO_DAT_OFFSET		(0X10C)
#define PHO_PULL1_OFFSET	(0X11C)
#define GPIOF_CON			0x7f0080a0
#define GPIOF_DAT			0x7f0080a4
#define GPIOF_PUD			0x7f0080a8

#define GSL_NOID_VERSION
static char cfg_adjust_flag = 0;
static int cfg_adjust_used_id = 0;

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
												  \n++++++ GSLX680 new used +++++++++ \
												  \n+++++++++++++++++++++++++++++++++ \
												  \n"
													 
#define GSLX680_IC_INFO  "\n============================================================== \
												  \nIC	 :GSLX680 \
												  \nAUTHOR :Gavin\
												  \nVERSION:2014_05_15_20:34\n"
												  

#ifdef GSL_ID_VERSION_INFO
extern unsigned int gsl_mask_tiaoping_1106(void);
extern unsigned int gsl_version_id_1106(void);
extern void gsl_alg_id_main_1106(struct gsl_touch_info *cinfo);
extern void gsl_DataInit_1106(int *ret);

#define GSL_DATAINIT			gsl_DataInit_1106
#define GSL_VERSION_ID			gsl_version_id_1106
#define GSL_ALG_ID_MAIN			gsl_alg_id_main_1106
#define GSL_MASK_TIAOPING		gsl_mask_tiaoping_1106
#else
extern unsigned int gsl_mask_tiaoping(void);
extern unsigned int gsl_version_id(void);
extern void gsl_alg_id_main(struct gsl_touch_info *cinfo);
extern void gsl_DataInit(int *ret);

#define GSL_DATAINIT			gsl_DataInit
#define GSL_VERSION_ID			gsl_version_id
#define GSL_ALG_ID_MAIN			gsl_alg_id_main
#define GSL_MASK_TIAOPING		gsl_mask_tiaoping

#endif

											  
extern int m_inet_ctpState;

static int gsl_chipType_new = 0;
static int ctp_cob_gslX680 = 0;
static int ctp_ft5402_num = 0;
static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;
//mbg ++ 20131126
static int fw_index = -1;
struct gslX680_fw_array {
	unsigned int size;
	const struct fw_data *fw;
} gslx680_fw_grp[] = {
	{ARRAY_SIZE(Inet_Customer),Inet_Customer}, // 0

	{ARRAY_SIZE(FW_Q71_GSL2682B_1280800_OGS_DZ),FW_Q71_GSL2682B_1280800_OGS_DZ}, // 1

	{ARRAY_SIZE(FW_Q790_GSL2682B_1024768_PG_INET),FW_Q790_GSL2682B_1024768_PG_INET},// 2

	
	{ARRAY_SIZE(FW_Q790_GSL2682B_1024768_OGS_DZ_79A1),FW_Q790_GSL2682B_1024768_OGS_DZ_79A1},// 3
	{ARRAY_SIZE(FW_Q790_GSL2682B_1024768_OGS_DZ_79F1),FW_Q790_GSL2682B_1024768_OGS_DZ_79F1},// 4
	
	{ARRAY_SIZE(FW_Q790_GSL2682B_1024768_GG_QSD),FW_Q790_GSL2682B_1024768_GG_QSD},// 5
	
	{ARRAY_SIZE(FW_Q72_GSL1688E_1024600_OGS_DZ_70L1),FW_Q72_GSL1688E_1024600_OGS_DZ_70L1},// 6
	{ARRAY_SIZE(FW_Q72_GSL1688E_1024600_OGS_DZ_70E2),FW_Q72_GSL1688E_1024600_OGS_DZ_70E2},// 7
	
	
	{ARRAY_SIZE(FW_Q72_GSL1688E_1024600_PG_LHJ),FW_Q72_GSL1688E_1024600_PG_LHJ},// 8
	{ARRAY_SIZE(FW_Q72_GSL1688E_800480_PG_LHJ),FW_Q72_GSL1688E_800480_PG_LHJ},// 9
	
	{ARRAY_SIZE(FW_Q7011_GSL1680E_1024600_PG),FW_Q7011_GSL1680E_1024600_PG},// 10
	
	{ARRAY_SIZE(FW_Q791_GSL915_1024768_OGS_DZ_79D2),FW_Q791_GSL915_1024768_OGS_DZ_79D2},// 11
	{ARRAY_SIZE(FW_Q791_GSL915_1024768_OGS_DZ_79D4),FW_Q791_GSL915_1024768_OGS_DZ_79D4},// 12
	{ARRAY_SIZE(FW_Q791_GSL915_1024768_OGS_SG5908A),FW_Q791_GSL915_1024768_OGS_SG5908A},// 13
	{ARRAY_SIZE(FW_Q791_GSL915_1024768_OGS_JND),FW_Q791_GSL915_1024768_OGS_JND},// 14
	{ARRAY_SIZE(Q791_GSL915_1024768_GF_QSD),Q791_GSL915_1024768_GF_QSD},// 15
	
	{ARRAY_SIZE(FW_Q791_GSL915_1024768_GG_DPT),FW_Q791_GSL915_1024768_GG_DPT},// 16
	
	{ARRAY_SIZE(FW_Q791_GSL915_1024768_PG_QSD),FW_Q791_GSL915_1024768_PG_QSD},// 17
	{ARRAY_SIZE(Q791_GSL915_1024768_PG_XLL),Q791_GSL915_1024768_PG_XLL},// 17

	{ARRAY_SIZE(FW_Q102_GSL3675B_1024600_GG_QSD),FW_Q102_GSL3675B_1024600_GG_QSD},// 19
	{ARRAY_SIZE(Q102_GSL3675B_1024600_GG_DH),Q102_GSL3675B_1024600_GG_DH},// 20
	{ARRAY_SIZE(Q102_GSL3675B_1024600_PG_QSD),Q102_GSL3675B_1024600_PG_QSD},// 21
	{ARRAY_SIZE(Q102_GSL3675B_PG_DH_1024600),Q102_GSL3675B_PG_DH_1024600},// 22
	{ARRAY_SIZE(Q102_GSL3675B_PG_DPT_1024600),Q102_GSL3675B_PG_DPT_1024600},// 23
	{ARRAY_SIZE(Q102_GSL3675B_PG_LHJ_1024600),Q102_GSL3675B_PG_LHJ_1024600},// 24
	{ARRAY_SIZE(FW_Q102_GSL3675B_1024600_GG_HT_JH),FW_Q102_GSL3675B_1024600_GG_HT_JH},// 25
	{ARRAY_SIZE(FW_Q102_GSL3675B_1024600_PG_GG),FW_Q102_GSL3675B_1024600_PG_GG},// 26
	{ARRAY_SIZE(FW_Q102_GSL3675B_1024600_GG_QLT),FW_Q102_GSL3675B_1024600_GG_QLT},// 27


	{ARRAY_SIZE(FW_Q92_GSL3675B_GG_800480_QSD),FW_Q92_GSL3675B_GG_800480_QSD},// 28
	{ARRAY_SIZE(Q92_GSL3675B_PG_800480_DH),Q92_GSL3675B_PG_800480_DH},// 29
	{ARRAY_SIZE(FW_Q92_GSL3675B_GG_800480_DH),FW_Q92_GSL3675B_GG_800480_DH},// 30
	{ARRAY_SIZE(FW_Q92_GSL3675B_PG_800480_QSD),FW_Q92_GSL3675B_PG_800480_QSD},// 31
	{ARRAY_SIZE(FW_Q92_GSL3675B_PG_800480_LHJ),FW_Q92_GSL3675B_PG_800480_LHJ},// 32


	
	{ARRAY_SIZE(FW_Q92_GSL3675B_GG_1024600_DH),FW_Q92_GSL3675B_GG_1024600_DH},// 33
	{ARRAY_SIZE(Q92_GSL3675B_PG_1024600_DH),Q92_GSL3675B_PG_1024600_DH},// 34
	{ARRAY_SIZE(FW_Q92_GSL3675B_GG_1024600_QSD),FW_Q92_GSL3675B_GG_1024600_QSD},// 35
	{ARRAY_SIZE(FW_Q92_GSL3675B_PG_1024600_QSD),FW_Q92_GSL3675B_PG_1024600_QSD},// 36
	{ARRAY_SIZE(Q92_GSL3675B_PG_LHJ_1024600),Q92_GSL3675B_PG_LHJ_1024600},// 37

	
	{ARRAY_SIZE(FW_Q7012_GSL1680E_1024600_PG),FW_Q7012_GSL1680E_1024600_PG},// 38

	{ARRAY_SIZE(Q101_GSL3675B_1024600_GG),Q101_GSL3675B_1024600_GG},// 39

	{ARRAY_SIZE(Q108_GSL3675B_1280800_GG_CTD),Q108_GSL3675B_1280800_GG_CTD},// 40
	{ARRAY_SIZE(Q108_GSL3675B_1280800_PG_CTD),Q108_GSL3675B_1280800_PG_CTD},// 41
	{ARRAY_SIZE(Q108_GSL3675B_1280800_PG_QLT),Q108_GSL3675B_1280800_PG_QLT},// 42
	
	
	{ARRAY_SIZE(FW_Q680_GSL3676B_1280800_OGS_DZ),FW_Q680_GSL3676B_1280800_OGS_DZ},// 43

	{ARRAY_SIZE(FW_Q680_7inch_GSL3676B_1024600_OGS_DZ),FW_Q680_7inch_GSL3676B_1024600_OGS_DZ},// 44

	{ARRAY_SIZE(FW_Q7011_GSL1680E_1024600_PG_DPT),FW_Q7011_GSL1680E_1024600_PG_DPT},// 45
};

unsigned int *gslX680_config_data[] = {
	gsl_config_data_id_Customer,// 0
	
	gsl_config_data_id_Q71_GSL2682B_1280800_OGS_DZ,// 1
	
	gsl_config_data_id_Q790_GSL2682B_1024768_PG_INET, // 2


	gsl_config_data_id_Q790_GSL2682B_1024768_OGS_DZ_79A1, // 3
	gsl_config_data_id_Q790_GSL2682B_1024768_OGS_DZ_79F1,// 4
	
	gsl_config_data_id_Q790_GSL2682B_1024768_GG_QSD,//  5
	
	gsl_config_data_id_Q72_GSL1688E_1024600_OGS_DZ_70L1, // 6
	gsl_config_data_id_Q72_GSL1688E_1024600_OGS_DZ_70E2, //7
	
	gsl_config_data_id_Q72_GSL1688E_1024600_PG_LHJ, // 8
	gsl_config_data_id_Q72_GSL1688E_800480_PG_LHJ, //9
	
	gsl_config_data_id_Q7011_GSL1680E_1024600_PG, //10
	
	gsl_config_data_id_Q791_GSL915_1024768_OGS_DZ_79D2, //11
	gsl_config_data_id_Q791_GSL915_1024768_OGS_DZ_79D4,//12
	gsl_config_data_id_Q791_GSL915_1024768_OGS_SG5908A, //13
	gsl_config_data_id_JND_OGS_Q791_GSL915_1024768_OGS_JND, // 14
	gsl_config_data_id_Q791_GSL915_1024768_GF_QSD, //15
	
	gsl_config_data_id_Q791_GSL915_1024768_GG_DPT,//16
	
	gsl_config_data_id_Q791_GSL915_1024768_PG_QSD,//17
	gsl_config_data_id_Q791_GSL915_1024768_PG_XLL, //18

	gsl_config_data_id_Q102_GSL3675B_1024600_GG_QSD, //19
	gsl_config_data_id_Q102_GSL3675B_1024600_GG_DH, //20
	gsl_config_data_id_Q102_GSL3675B_1024600_PG_QSD, //21
	gsl_config_data_id_Q102_GSL3675B_PG_DH_1024600, //22
	gsl_config_data_id_Q102_GSL3675B_PG_DPT_1024600, //23
	gsl_config_data_id_Q102_GSL3675B_PG_LHJ_1024600, //24
	gsl_config_data_id_Q102_GSL3675B_1024600_GG_HT_JH, //25
	gsl_config_data_id_Q102_GSL3675B_1024600_PG_GG, //26
	gsl_config_data_id_Q102_GSL3675B_1024600_GG_QLT, //27


	gsl_config_data_id_Q92_GSL3675B_GG_800480_QSD, //28
	gsl_config_data_id_Q92_GSL3675B_PG_800480_DH, //29
	gsl_config_data_id_Q92_GSL3675B_GG_800480_DH, //30
	gsl_config_data_id_Q92_GSL3675B_PG_800480_QSD, //31
	gsl_config_data_id_Q92_GSL3675B_PG_800480_LHJ, //32
	

	gsl_config_data_id_Q92_GSL3675B_GG_1024600_DH, //33
	gsl_config_data_id_Q92_GSL3675B_PG_1024600_DH, //34
	gsl_config_data_id_Q92_GSL3675B_GG_1024600_QSD, //35
	gsl_config_data_id_Q92_GSL3675B_PG_1024600_QSD, //36
	gsl_config_data_id_Q92_GSL3675B_PG_LHJ_1024600, //37
	
	gsl_config_data_id_Q7012_GSL1680E_1024600_PG, //38

	
	gsl_config_data_id_Q101_GSL3675B_1024600_GG, //39

	gsl_config_data_id_Q108_GSL3675B_1280800_GG_CTD, //40
	gsl_config_data_id_Q108_GSL3675B_1280800_PG_CTD, //41
	gsl_config_data_id_Q108_GSL3675B_1280800_PG_QLT, //42
	
	gsl_config_data_id_Q680_GSL3676B_1280800_OGS_DZ, //43

	gsl_config_data_id_Q680_7inch_GSL3676B_1024600_OGS_DZ, //44
	
	gsl_config_data_id_Q7011_GSL1680E_1024600_PG_DPT, //45

};

static int getIndex(int num)
{
	int index = 0;
	printk("++++++++++++++++++++++++\n");
 	switch(num)
 	{
 		case 999:
			index = 0;
			printk("Inet_Customer\n");
			printk("%s\n",CUSTOMER_INFO);
			break;
 		case 1:
			index = 1;
			printk("FW_Q71_GSL2682B_1280800_OGS_DZ\n");
			break;
		case 5:
			index = 2;
			printk("FW_Q790_GSL2682B_1024768_PG_INET\n");
			break;
		case 6:
			index = 3;
			printk("FW_Q790_GSL2682B_1024768_OGS_DZ_79A1\n");
			break;
		case 62:
			index = 4;
			printk("FW_Q790_GSL2682B_1024768_OGS_DZ_79F1\n");
			break;
		case 603:
			index = 5;
			printk("FW_Q790_GSL2682B_1024768_GG_QSD\n");
			break;
		case 72:
			index = 6;
			printk("FW_Q72_GSL1688E_1024600_OGS_DZ_70L1\n");
			break;
		case 73:
			index = 7;
			printk("FW_Q72_GSL1688E_1024600_OGS_DZ_70E2\n");
			break;
		case 7202:
			if(screen_max_x == 1024 && screen_max_y == 600)
			{
				index = 8;
				printk("FW_Q72_GSL1688E_1024600_PG_LHJ\n");
			}
			else
			{
				index = 9;
				printk("FW_Q72_GSL1688E_800480_PG_LHJ\n");
			}
			break;
		case 7011:
		case 701102:
			index = 10;
			printk("FW_Q7011_GSL1680E_1024600_PG\n");
			break;
		case 790101:
			//index = 10;
			printk("FW_Q791_GSL915_1024768_OGS TP ajust\n");
			break;
		case 790105:
			//index = 11;
			printk("FW_Q791_GSL915_1024768_OGS TP ajust\n");
			break;
		case 7911:
			index = 16;
			printk("FW_Q791_GSL915_1024768_GG_DPT\n");
			break;
		case 7912:
			//index = 16;
			printk("Q791_GSL915_1024768_PG TP adjust\n");
			break;
		case 102:
			//index = 18;
			printk("Q102 tp ajust\n");
			break;
		case 921:
			//index = 27;
			printk("Q92_GSL3675B 800480 TP ajust \n");
			break;
		case 922:
			//index = 32;
			printk("FW_Q92_GSL3675B TP ajust\n");
			break;
		case 7012:
			index = 38;
			printk("FW_Q7012_GSL1680E_1024600_PG\n");
			break;
		case 101:
			index = 39;
			printk("Q101_GSL3675B_1024600_GG\n");
			break;
		case 108:
			//index = 39;
			printk("Q108_GSL3675B_1280800 TP ajust\n");
			break;
		case 680:
			//index = 42;
			printk("Q680_GSL3676B_1280800 TP ajust\n");
			break;	
		case 68070:
			//index = 43;
			printk("Q680_7inch_GSL3676B_1024600 TP ajust\n");
			break;	
		case 701103:
			index = 45;
			printk("FW_Q7011_GSL1680E_1024600_PG_DPT\n");
			break;
		default:
			index = 1;
			printk("FW_Q71_GSL2682B_1280800_OGS_DZ\n");
			break;
 	}
	printk("++++++++++++++++++++++++\n");
	return index;
}
//--
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
	//printk("[FUN]%s\n", __func__);
	
	u8 buf[DMA_TRANS_LEN*4 + 1] = {0};
	u8 send_flag = 1;
	u8 *cur = buf + 1;
	u32 source_line = 0;
	u32 source_len;
	static const struct fw_data *ptr_fw;
	
	u8 read_buf[4] = {0};

	gsl_ts_read(client, 0xfc, read_buf, 4);
	printk("read 0xfc = %x %x %x %x\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	if(read_buf[2] != 0x82 && read_buf[2] != 0x88 &&  read_buf[2] != 0x91 && read_buf[2] != 0x36)
	{
		msleep(100);
		gsl_ts_read(client, 0xfc, read_buf, 4);
		printk("read 0xfc = %x %x %x %x\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	}
	
	
	printk("=============gsl_load_fw start==============\n");

	switch(ctp_cob_gslX680)
	{
		case 790101:
			if(cfg_adjust_flag)
			{
				switch(cfg_adjust_used_id)
				{
					case 1:
						fw_index = 11;
						printk("FW_Q791_GSL915_1024768_OGS_DZ_79D2\n");
						break;
					case 2:
						fw_index = 13;
						printk("FW_Q791_GSL915_1024768_OGS_SG5908A\n");
						break;
					case 3:
						fw_index = 14;
						printk("FW_Q791_GSL915_1024768_OGS_JND\n");
						break;
					case 4:
						fw_index = 15;
						printk("Q791_GSL915_1024768_GF_QSD\n");
						break;
					default:
						fw_index = 11;
						printk("FW_Q791_GSL915_1024768_OGS_DZ_79D2\n");
						break;
						
				}
				ptr_fw = gslx680_fw_grp[fw_index].fw;
				source_len = gslx680_fw_grp[fw_index].size;	
				break;
				
			}
			else
			{
				cfg_adjust_flag = 1 ;
				ptr_fw = GSLX680_FW_TEST;
				source_len = ARRAY_SIZE(GSLX680_FW_TEST);
				break;
			}
		case 790105:
			if(cfg_adjust_flag)
			{
				switch(cfg_adjust_used_id)
				{
					case 1:
						fw_index = 12;
						printk("FW_Q791_GSL915_1024768_OGS_DZ_79D4\n");
						break;
					case 2:
						fw_index = 13;
						printk("FW_Q791_GSL915_1024768_OGS_SG5908A\n");
						break;
					case 3:
						fw_index = 14;
						printk("FW_Q791_GSL915_1024768_OGS_JND\n");
						break;
					case 4:
						fw_index = 15;
						printk("Q791_GSL915_1024768_GF_QSD\n");
						break;
					default:
						fw_index = 12;
						printk("FW_Q791_GSL915_1024768_OGS_DZ_79D4\n");
						break;
						
				}
				ptr_fw = gslx680_fw_grp[fw_index].fw;
				source_len = gslx680_fw_grp[fw_index].size;	
				break;
				
			}
			else
			{
				cfg_adjust_flag = 1 ;
				ptr_fw = GSLX680_FW_TEST;
				source_len = ARRAY_SIZE(GSLX680_FW_TEST);
				break;
			}
		case 921:
			if(cfg_adjust_flag)
			{
				switch(cfg_adjust_used_id)
				{
					case 1:
						fw_index = 28;
						printk("FW_Q92_GSL3675B_GG_800480_QSD\n");
						break;
					case 2:
						fw_index = 29;
						printk("Q92_GSL3675B_PG_800480_DH\n");
						break;
					case 3:
						fw_index = 30;
						printk("FW_Q92_GSL3675B_GG_800480_DH\n");
						break;
					case 4:
						fw_index = 31;
						printk("FW_Q92_GSL3675B_PG_800480_QSD\n");
						break;
					case 5:
						fw_index = 32;
						printk("FW_Q92_GSL3675B_PG_800480_LHJ\n");
						break;
					default:
						fw_index = 28;
						printk("FW_Q92_GSL3675B_GG_800480_QSD\n");
						break;
						
				}
				ptr_fw = gslx680_fw_grp[fw_index].fw;
				source_len = gslx680_fw_grp[fw_index].size;	
				break;
			}
			else
			{
				cfg_adjust_flag = 1 ;
				ptr_fw = GSLX680_FW_TEST;
				source_len = ARRAY_SIZE(GSLX680_FW_TEST);
				break;
			}
			
	
		case 922:
			if(cfg_adjust_flag)
			{
				switch(cfg_adjust_used_id)
				{
					case 1:
						fw_index = 33;
						printk("FW_Q92_GSL3675B_GG_1024600_DH\n");
						break;
					case 2:
						fw_index = 34;
						printk("Q92_GSL3675B_PG_1024600_DH\n");
						break;
					case 3:
						fw_index = 35;
						printk("Q92_GSL3675B_GG_QSD_1024600\n");
						break;
					case 4:
						fw_index = 36;
						printk("FW_Q92_GSL3675B_PG_1024600_QSD\n");
						break;
					case 5:
						fw_index = 37;
						printk("Q92_GSL3675B_PG_LHJ_1024600\n");
						break;
					default:
						fw_index = 33;
						printk("FW_Q92_GSL3675B_GG_1024600_DH\n");
						break;
						
				}
				ptr_fw = gslx680_fw_grp[fw_index].fw;
				source_len = gslx680_fw_grp[fw_index].size;	
				break;
				
			}
			else
			{
				cfg_adjust_flag = 1 ;
				ptr_fw = GSLX680_FW_TEST;
				source_len = ARRAY_SIZE(GSLX680_FW_TEST);
				break;
			}
		case 102:
			if(cfg_adjust_flag)
			{
				switch(cfg_adjust_used_id)
				{
					case 1:
						fw_index = 19;
						printk("Q102_GSL3675B_1024600_GG_QSD\n");
						break;
					case 2:
						fw_index = 20;
						printk("Q102_GSL3675B_1024600_GG_DH\n");
						break;
					case 3:
						fw_index = 21;
						printk("Q102_GSL3675B_1024600_PG_QSD\n");
						break;
					case 4:
						fw_index = 22;
						printk("Q102_GSL3675B_PG_DH_1024600\n");
						break;
					case 5:
						fw_index = 23;
						printk("Q102_GSL3675B_PG_DPT_1024600\n");
						break;
					case 6:
						fw_index = 24;
						printk("Q102_GSL3675B_PG_LHJ_1024600\n");
						break;
					case 7:
						fw_index = 25;
						printk("Q102_GSL3675B_1024600_GG_HT_JH\n");
						break;
					case 8:
						fw_index =26;
						printk("FW_Q102_GSL3675B_1024600_PG_GG\n");
						break;
					case 9:
						fw_index =27;
						printk("FW_Q102_GSL3675B_1024600_GG_QLT\n");
						break;	
					default:
						fw_index = 19;
						printk("Q102_GSL3675B_1024600_GG_QSD\n");
						break;
						
				}
				ptr_fw = gslx680_fw_grp[fw_index].fw;
				source_len = gslx680_fw_grp[fw_index].size;	
				break;
				
			}
			else
			{
				cfg_adjust_flag = 1 ;
				ptr_fw = GSLX680_FW_TEST;
				source_len = ARRAY_SIZE(GSLX680_FW_TEST);
				break;
			}
		case 680:
			if(cfg_adjust_flag)
			{
				switch(cfg_adjust_used_id)
				{
					case 1:
						fw_index = 43;
						printk("FW_Q680_GSL3676B_1280800_OGS_DZ\n");
						break;
					default:
						fw_index = 43;
						printk("FW_Q680_GSL3676B_1280800_OGS_DZ\n");
						break;
						
				}
				ptr_fw = gslx680_fw_grp[fw_index].fw;
				source_len = gslx680_fw_grp[fw_index].size;	
				break;
				
			}
			else
			{
				cfg_adjust_flag = 1 ;
				ptr_fw = GSLX680_FW_TEST;
				source_len = ARRAY_SIZE(GSLX680_FW_TEST);
				break;
			}	
		case 68070:
			if(cfg_adjust_flag)
			{
				switch(cfg_adjust_used_id)
				{
					case 1:
						fw_index = 52;
						printk("FW_Q680_7inch_GSL3676B_1024600_OGS_DZ\n");
						break;
					default:
						fw_index = 52;
						printk("FW_Q680_7inch_GSL3676B_1024600_OGS_DZ\n");
						break;
						
				}
				ptr_fw = gslx680_fw_grp[fw_index].fw;
				source_len = gslx680_fw_grp[fw_index].size;	
				break;
				
			}
			else
			{
				cfg_adjust_flag = 1 ;
				ptr_fw = GSLX680_FW_TEST;
				source_len = ARRAY_SIZE(GSLX680_FW_TEST);
				break;
			}
		case 108:
			if(cfg_adjust_flag)
			{
				switch(cfg_adjust_used_id)
				{
					case 1:
						fw_index = 40;
						printk("Q108_GSL3675B_1280800_GG_CTD\n");
						break;
					case 2:
						fw_index = 41;
						printk("Q108_GSL3675B_1280800_PG_CTD\n");
						break;
					case 3:
						fw_index = 42;
						printk("Q108_GSL3675B_1280800_PG_QLT\n");
						break;
					default:
						fw_index = 40;
						printk("Q108_GSL3675B_1280800_GG_CTD\n");
						break;
						
				}
				ptr_fw = gslx680_fw_grp[fw_index].fw;
				source_len = gslx680_fw_grp[fw_index].size;	
				break;
				
			}
			else
			{
				cfg_adjust_flag = 1 ;
				ptr_fw = GSLX680_FW_TEST;
				source_len = ARRAY_SIZE(GSLX680_FW_TEST);
				break;
			}
		case 7912:
			if(cfg_adjust_flag)
			{
				switch(cfg_adjust_used_id)
				{
					case 1:
						fw_index = 17;
						printk("FW_Q791_GSL915_1024768_PG_QSD\n");
						break;
					case 2:
						fw_index = 18;
						printk("Q791_GSL915_1024768_PG_XLL\n");
						break;
					default:
						fw_index = 17;
						printk("FW_Q791_GSL915_1024768_PG_QSD\n");
						break;
						
				}
				ptr_fw = gslx680_fw_grp[fw_index].fw;
				source_len = gslx680_fw_grp[fw_index].size;	
				break;
				
			}
			else
			{
				cfg_adjust_flag = 1 ;
				ptr_fw = GSLX680_FW_TEST;
				source_len = ARRAY_SIZE(GSLX680_FW_TEST);
				break;
			}
		default:
			ptr_fw = gslx680_fw_grp[fw_index].fw;
			source_len = gslx680_fw_grp[fw_index].size;	
			break;
	}
	
	//mbg --

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
#if 0
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



static int cfg_adjust(struct i2c_client *client)
{
	
	int i,j,ret;
	u8 read_buf[4]  = {0};
	u8 temp[4]={0};
      u32 GSL_TP_ID_TEMP=0;

	switch(ctp_cob_gslX680)
	{
		case 790101:
		case 790105:
		case 921:
		case 922:
		case 102:
		case 680:
		case 68070:
		case 108:
		case 7912:
			ret = 1;
			break;
		default:
			ret = 0;
			break;
			
	 }

	if(ret==0)
	{
		return 0;
	}
	
	printk("=============gsl_load_cfg_adjust check start==============\n");
	msleep(500);
	/*gsl_ts_read(client,0xb4,  read_buf,sizeof(read_buf));
	printk("fuc:cfg_adjust, b4: %x %x %x %x\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	msleep(100);
	gsl_ts_read(client,0xb4,  read_buf,sizeof(read_buf));
	printk("fuc:cfg_adjust, b4: %x %x %x %x\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);*/
      gsl_ts_read(client,0xb8,  read_buf,sizeof(read_buf));
	//i2c_smbus_read_i2c_block_data(client,0xb8, sizeof(read_buf), read_buf);
    printk("fuc:cfg_adjust, b8: %x %x %x %x\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	//GSL_TP_ID_TEMP=(u32)(read_buf[0]<<24+read_buf[1]<<16+read_buf[2]<<8+read_buf[3]);
	//GSL_TP_ID_TEMP=GSL_MX790_TP_ID[0];
	printk("fuc:cfg_adjust, GSL_TP_ID_TEMP: %x \n", GSL_TP_ID_TEMP);

	switch(ctp_cob_gslX680)
	{
		case 790101:
		case 790105:
			for(i=0;i<Q791_TP_NUM;i++)
			  {
				for(j=0;j<4;j++)
				{
				temp[j]=Q791_OGS_TP[i][j];
				}
				printk("temp[] %x %x %x %x\n", temp[0],temp[1],temp[2],temp[3]);
				if((read_buf[3]==temp[0])&&(read_buf[2]==temp[1])&&(read_buf[1]==temp[2])&&(read_buf[0]==temp[3]))
					{
					cfg_adjust_used_id=i+1;
					break;
					}
			  }
			break;
		case 921:
			for(i=0;i<Q92_800480_TP_NUM;i++)
			  {
				for(j=0;j<4;j++)
				{
				temp[j]=Q92_800480_TP[i][j];
				}
				printk("temp[] %x %x %x %x\n", temp[0],temp[1],temp[2],temp[3]);
				if((read_buf[3]==temp[0])&&(read_buf[2]==temp[1])&&(read_buf[1]==temp[2])&&(read_buf[0]==temp[3]))
					{
					cfg_adjust_used_id=i+1;
					break;
					}
			  }
			break;
		case 922:
			for(i=0;i<Q92_1024600_TP_NUM;i++)
			  {
				for(j=0;j<4;j++)
				{
				temp[j]=Q92_1024600_TP[i][j];
				}
				printk("temp[] %x %x %x %x\n", temp[0],temp[1],temp[2],temp[3]);
				if((read_buf[3]==temp[0])&&(read_buf[2]==temp[1])&&(read_buf[1]==temp[2])&&(read_buf[0]==temp[3]))
					{
					cfg_adjust_used_id=i+1;
					break;
					}
			  }
			break;
		case 102:
			for(i=0;i<Q102_1024600_TP_NUM;i++)
			  {
				for(j=0;j<4;j++)
				{
				temp[j]=Q102_1024600_TP[i][j];
				}
				printk("temp[] %x %x %x %x\n", temp[0],temp[1],temp[2],temp[3]);
				if((read_buf[3]==temp[0])&&(read_buf[2]==temp[1])&&(read_buf[1]==temp[2])&&(read_buf[0]==temp[3]))
					{
					cfg_adjust_used_id=i+1;
					break;
					}
			  }
			break;
		case 680:
			for(i=0;i<Q680_TP_NUM;i++)
			  {
				for(j=0;j<4;j++)
				{
				temp[j]=Q680_OGS_TP[i][j];
				}
				printk("temp[] %x %x %x %x\n", temp[0],temp[1],temp[2],temp[3]);
				if((read_buf[3]==temp[0])&&(read_buf[2]==temp[1])&&(read_buf[1]==temp[2])&&(read_buf[0]==temp[3]))
					{
					cfg_adjust_used_id=i+1;
					break;
					}
			  }
			break;	
		case 68070:
			for(i=0;i<Q680_7inch_TP_NUM;i++)
			  {
				for(j=0;j<4;j++)
				{
				temp[j]=Q680_7inch_OGS_TP[i][j];
				}
				printk("temp[] %x %x %x %x\n", temp[0],temp[1],temp[2],temp[3]);
				if((read_buf[3]==temp[0])&&(read_buf[2]==temp[1])&&(read_buf[1]==temp[2])&&(read_buf[0]==temp[3]))
					{
					cfg_adjust_used_id=i+1;
					break;
					}
			  }
			break;			
		case 108:
			for(i=0;i<Q108_TP_NUM;i++)
			  {
				for(j=0;j<4;j++)
				{
				temp[j]=Q108_1280800_TP[i][j];
				}
				printk("temp[] %x %x %x %x\n", temp[0],temp[1],temp[2],temp[3]);
				if((read_buf[3]==temp[0])&&(read_buf[2]==temp[1])&&(read_buf[1]==temp[2])&&(read_buf[0]==temp[3]))
					{
					cfg_adjust_used_id=i+1;
					break;
					}
			  }
			break;
		case 7912:
			for(i=0;i<Q791_PG_TP_NUM;i++)
			  {
				for(j=0;j<4;j++)
				{
				temp[j]=Q791_PG_TP[i][j];
				}
				printk("temp[] %x %x %x %x\n", temp[0],temp[1],temp[2],temp[3]);
				if((read_buf[3]==temp[0])&&(read_buf[2]==temp[1])&&(read_buf[1]==temp[2])&&(read_buf[0]==temp[3]))
					{
					cfg_adjust_used_id=i+1;
					break;
					}
			  }
			break;	
	}
	printk("fuc:cfg_adjust, cfg_adjust_used_id: %x \n", cfg_adjust_used_id);
	printk("=============gsl_load_cfg_adjust check end==============\n");
		reset_chip(client);
		gsl_load_fw(client);
		startup_chip(client);
		reset_chip(client);
		startup_chip(client);	

	return 1;
}
//#endif

static void clr_reg(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};

	write_buf[0] = 0x88;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);
	write_buf[0] = 0x03;//mbg modify for GSL3675 ic ajust 20140107
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

	//if(gsl_chipType_new)	
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
		tmp=GSL_VERSION_ID();
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
				ptr +=sprintf(ptr,"%d\n",gslX680_config_data[fw_index][tmp]); 
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
	//mbg ++ 20131130
	GSL_DATAINIT(gslX680_config_data[fw_index]);	
	
	//mbg --
	
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
			//gsl_config_data_id[tmp1] = tmp;
		//mbg ++ 20131130
		gslX680_config_data[fw_index][tmp1] = tmp;
		
		//mbg --
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
	switch(ctp_cob_gslX680)
	{
		case 7202:
		case 7203:
		case 7204:
		case 7012:
		case 701102:
			break;
		default:
			x = SCREEN_MAX_X - x;
			y = SCREEN_MAX_Y - y;
			break;
	}
	/*
	if(ctp_cob_gslX680 != 7202 && ctp_cob_gslX680 != 7012)
	{
		x = SCREEN_MAX_X - x;
		y = SCREEN_MAX_Y - y;
	}		
	*/
	dprintk(DEBUG_X_Y_INFO,"#####id=%d,x=%d,y=%d######\n",id,x,y);
	//printk("#####id=%d,x=%d,y=%d######\n",id,x,y);

	if(x>SCREEN_MAX_X||y>SCREEN_MAX_Y)
	{
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
	int tmp1 = 0;
	u8 buf[4]={0};
	touches = ts->touch_data[ts->dd->touch_index];
#ifdef GSL_NOID_VERSION
    struct gsl_touch_info cinfo;
	cinfo.finger_num = touches;
	//printk("tp-gsl  finger_num = %d\n",cinfo.finger_num);
	for(i = 0; i < (touches < MAX_CONTACTS ? touches : MAX_CONTACTS); i ++)
	{
		cinfo.x[i] = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		cinfo.y[i] = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i ]);
		//printk("tp-gsl  x = %d y = %d \n",cinfo.x[i],cinfo.y[i]);
	}
	cinfo.finger_num = ts->touch_data[0] | (ts->touch_data[1]<<8)|(ts->touch_data[2]<<16)|
		(ts->touch_data[3]<<24);
	GSL_ALG_ID_MAIN(&cinfo);
	//printk("tp-gsl  finger_num = %d\n",cinfo.finger_num);
	tmp1=GSL_MASK_TIAOPING();
	//printk("[tp-gsl] tmp1=%x\n",tmp1);
	if(tmp1>0&&tmp1<0xffffffff)
	{
		buf[0]=0xa;
		buf[1]=0;
		buf[2]=0;
		buf[3]=0;
		gsl_ts_write(ts->client,0xf0,buf,4);
		buf[0]=(u8)(tmp1 & 0xff);
		buf[1]=(u8)((tmp1>>8) & 0xff);
		buf[2]=(u8)((tmp1>>16) & 0xff);
		buf[3]=(u8)((tmp1>>24) & 0xff);
		printk("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n",
			tmp1,buf[0],buf[1],buf[2],buf[3]);
		gsl_ts_write(ts->client,0x8,buf,4);
	}
	touches = cinfo.finger_num;

#endif
	for(i=1;i<=MAX_CONTACTS;i++){
		if(touches == 0)
			id_sign[i] = 0;	
		id_state_flag[i] = 0;
	}
	for(i= 0;i < (touches > MAX_FINGERS ? MAX_FINGERS : touches);i++)
	{
	#ifdef GSL_NOID_VERSION
		id = cinfo.id[i];
		x =  cinfo.x[i];
		y =  cinfo.y[i];	
	#else
		x = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		y = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i ]);
		id = ts->touch_data[ts->dd->id_index + 4 * i] >> 4;
    #endif
		if(1 <=id && id <= MAX_CONTACTS)
		{

		#ifdef STRETCH_FRAME
			stretch_frame(&x, &y);
		
		#endif
		#ifdef FILTER_POINT
			filter_point(x, y ,id);
		#else
			record_point(x, y , id);
		#endif
			report_data(ts, x_new, y_new, 10, id);		
			id_state_flag[id] = 1;
		}
	}
	
	
	for(i = 1;i <= MAX_CONTACTS ; i++)
	{
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
	input_sync(ts->input);
	ts->prev_touches = touches;
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

#ifdef GSL_MONITOR
static void gsl_monitor_worker(void)
{
	char write_buf[4] = {0};
	char read_buf[4]  = {0};
	
	printk("----------------gsl_monitor_worker-----------------\n");	

	gsl_ts_read(gsl_client, 0xb4, read_buf, 4);	
	int_2nd[3] = int_1st[3];
	int_2nd[2] = int_1st[2];
	int_2nd[1] = int_1st[1];
	int_2nd[0] = int_1st[0];
	int_1st[3] = read_buf[3];
	int_1st[2] = read_buf[2];
	int_1st[1] = read_buf[1];
	int_1st[0] = read_buf[0];

	if (int_1st[3] == int_2nd[3] && int_1st[2] == int_2nd[2] &&int_1st[1] == int_2nd[1] && int_1st[0] == int_2nd[0]) 
	{
		printk("======int_1st: %x %x %x %x , int_2nd: %x %x %x %x ======\n",int_1st[3], int_1st[2], int_1st[1], int_1st[0], int_2nd[3], int_2nd[2],int_2nd[1],int_2nd[0]);
		init_chip(gsl_client);
	}

	write_buf[3] = 0x01;
	write_buf[2] = 0xfe;
	write_buf[1] = 0x10;
	write_buf[0] = 0x00;
	gsl_ts_write(gsl_client, 0xf0, write_buf, 4);
	gsl_ts_read(gsl_client, 0x10, read_buf, 4);
	gsl_ts_read(gsl_client, 0x10, read_buf, 4);	
	//printk("======read DAC1_0: %x %x %x %x ======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	
	if(read_buf[3] < 10 && read_buf[2] < 10 && read_buf[1] < 10 && read_buf[0] < 10)
		dac_counter ++;
	else
		dac_counter = 0;

	if(dac_counter > 2) 
	{
		printk("======read DAC1_0: %x %x %x %x ======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip(gsl_client);
		dac_counter = 0;
	}
	
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 100);
}
#endif
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

#ifdef GSL_MONITOR
		printk( "gsl_ts_suspend () : cancel gsl_monitor_work\n");
		cancel_delayed_work_sync(&gsl_monitor_work);
#endif
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
#ifdef GSL_MONITOR
	gsl_client = client;
#endif
	gslX680_chip_init();    	
	init_chip(ts->client);
	cfg_adjust(ts->client);
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
	ts->gsl_timer.expires = jiffies + 3 * HZ;	//定时3  秒钟
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
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 201;
	ts->early_suspend.suspend = gsl_ts_early_suspend;
	ts->early_suspend.resume = gsl_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
#ifdef GSL_NOID_VERSION
	//mbg ++ 20131130
	GSL_DATAINIT(gslX680_config_data[fw_index]);

#endif
	
#ifdef GSL_MONITOR
		printk( "gsl_ts_probe () : queue gsl_monitor_workqueue\n");
	
		INIT_DELAYED_WORK(&gsl_monitor_work, gsl_monitor_worker);
		gsl_monitor_workqueue = create_singlethread_workqueue("gsl_monitor_workqueue");
		queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 1000);
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

#ifdef GSL_MONITOR
		cancel_delayed_work_sync(&gsl_monitor_work);
		destroy_workqueue(gsl_monitor_workqueue);
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
        
       
		if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_cob_gslX680", &val))
		{
			ctp_cob_gslX680=0;
		}
		else
		{
			ctp_cob_gslX680 = val.val;
		}

		#ifdef CUSTOMER_USED
		ctp_cob_gslX680 = 999;
		#endif
		printk("ctp_cob_gslX680=%d\n",ctp_cob_gslX680);

        ctp_print_info(config_info,DEBUG_INIT);
 
        twi_id = config_info.twi_id;
        screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
        revert_x_flag = config_info.revert_x_flag;
        revert_y_flag = config_info.revert_y_flag;
        exchange_x_y_flag = config_info.exchange_x_y_flag;
		
	fw_index = getIndex(ctp_cob_gslX680);
	printk("fw_index=%d\n",fw_index);
	if (fw_index == -1) {
		printk("gslx680: no matched TP firmware!\n");
		return 0;
	}
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

