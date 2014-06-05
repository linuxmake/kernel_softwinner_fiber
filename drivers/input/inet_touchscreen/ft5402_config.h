#ifndef __FT5402_CONFIG_H__ 
#define __FT5402_CONFIG_H__ 
/*FT5402 config*/

/*
#define FT5402_START_RX			0
#define FT5402_ADC_TARGET			8500
#define FT5402_KX			151
#define FT5402_KY			160
#define FT5402_RESOLUTION_X			600
#define FT5402_RESOLUTION_Y			1024
#define FT5402_LEMDA_X			5
#define FT5402_LEMDA_Y			5
#define FT5402_PWMODE_CTRL			1
#define FT5402_POINTS_SUPPORTED			5
#define FT5402_DRAW_LINE_TH			150
#define FT5402_FACE_DETECT_MODE			0
#define FT5402_FACE_DETECT_STATISTICS_TX_NUM			3
#define FT5402_FACE_DETECT_PRE_VALUE			20
#define FT5402_FACE_DETECT_NUM			10
#define FT5402_THGROUP			22
#define FT5402_THPEAK			60
#define FT5402_BIGAREA_PEAK_VALUE_MIN			100
#define FT5402_BIGAREA_DIFF_VALUE_OVER_NUM			50
#define FT5402_MIN_DELTA_X			2
#define FT5402_MIN_DELTA_Y			2
#define FT5402_MIN_DELTA_STEP			2
#define FT5402_ESD_DIFF_VAL			20
#define FT5402_ESD_NEGTIVE			-50
#define FT5402_ESD_FILTER_FRAME			10
#define FT5402_MAX_TOUCH_VALUE			600
#define FT5402_CUSTOMER_ID			121
#define FT5402_IO_LEVEL_SELECT			0
#define FT5402_DIRECTION			1
#define FT5402_POINTID_DELAY_COUNT			4
#define FT5402_LIFTUP_FILTER_MACRO			0
#define FT5402_POINTS_STABLE_MACRO			1
#define FT5402_ESD_NOISE_MACRO			0
#define FT5402_RV_G_PERIOD_ACTIVE			16
#define FT5402_DIFFDATA_HANDLE			0
#define FT5402_MIN_WATER_VAL			-50
#define FT5402_MAX_NOISE_VAL			10
#define FT5402_WATER_HANDLE_START_RX			0
#define FT5402_WATER_HANDLE_START_TX			0
#define FT5402_HOST_NUMBER_SUPPORTED			1
#define FT5402_RV_G_RAISE_THGROUP			30
#define FT5402_RV_G_CHARGER_STATE			1
#define FT5402_RV_G_FILTERID_START			2
#define FT5402_FRAME_FILTER_EN			0
#define FT5402_FRAME_FILTER_SUB_MAX_TH			2
#define FT5402_FRAME_FILTER_ADD_MAX_TH			2
#define FT5402_FRAME_FILTER_SKIP_START_FRAME			6
#define FT5402_FRAME_FILTER_BAND_EN			1
#define FT5402_FRAME_FILTER_BAND_WIDTH			128
#define FT5402_OTP_PARAM_ID			0


unsigned char g_ft5402_tx_num = 26;
unsigned char g_ft5402_rx_num = 16;
unsigned char g_ft5402_gain = 20;
unsigned char g_ft5402_voltage = 4;
unsigned char g_ft5402_scanselect = 8;
unsigned char g_ft5402_tx_order[] = {0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26};
unsigned char g_ft5402_tx_offset = 2;
unsigned char g_ft5402_tx_cap[] = {50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50};
unsigned char g_ft5402_rx_order[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
unsigned char g_ft5402_rx_offset[] = {52,68,68,52,69,68,68,84};
unsigned char g_ft5402_rx_cap[] = {100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100};
*/


int FT5402_START_RX		=	0;
int FT5402_ADC_TARGET		=	0;
int FT5402_KX		=	0;
int FT5402_KY		=	0;
int FT5402_RESOLUTION_X		=	0;
int FT5402_RESOLUTION_Y		=	0;
int FT5402_LEMDA_X		=	0;
int FT5402_LEMDA_Y		=	0;
int FT5402_PWMODE_CTRL	=		0;
int FT5402_POINTS_SUPPORTED		=	0;
int FT5402_DRAW_LINE_TH		=	0;
int FT5402_FACE_DETECT_MODE		=	0;
int FT5402_FACE_DETECT_STATISTICS_TX_NUM	=		0;
int FT5402_FACE_DETECT_PRE_VALUE	=		0;
int FT5402_FACE_DETECT_NUM		=	0;
int FT5402_THGROUP	=		0;
int FT5402_THPEAK		=	0;
int FT5402_BIGAREA_PEAK_VALUE_MIN		=	0;
int FT5402_BIGAREA_DIFF_VALUE_OVER_NUM		=	0;
int FT5402_MIN_DELTA_X	=		0;
int FT5402_MIN_DELTA_Y		=	0;
int FT5402_MIN_DELTA_STEP	=		0;
int FT5402_ESD_DIFF_VAL		=	0;
int FT5402_ESD_NEGTIVE		=	0;
int FT5402_ESD_FILTER_FRAME		=	0;
int FT5402_MAX_TOUCH_VALUE		=	0;
int FT5402_CUSTOMER_ID		=	0;
int FT5402_IO_LEVEL_SELECT	=		0;
int FT5402_DIRECTION		=	0;
int FT5402_POINTID_DELAY_COUNT		=	0;
int FT5402_LIFTUP_FILTER_MACRO		=	0;
int FT5402_POINTS_STABLE_MACRO		=	0;
int FT5402_ESD_NOISE_MACRO		=	0;
int FT5402_RV_G_PERIOD_ACTIVE		=	0;
int FT5402_DIFFDATA_HANDLE		=	0;
int FT5402_MIN_WATER_VAL		=	0;
int FT5402_MAX_NOISE_VAL		=	0;
int FT5402_WATER_HANDLE_START_RX	=		0;
int FT5402_WATER_HANDLE_START_TX	=		0;
int FT5402_HOST_NUMBER_SUPPORTED	=		0;
int FT5402_RV_G_RAISE_THGROUP		=	0;
int FT5402_RV_G_CHARGER_STATE		=	0;
int FT5402_RV_G_FILTERID_START		=	0;
int FT5402_FRAME_FILTER_EN		=	0;
int FT5402_FRAME_FILTER_SUB_MAX_TH		=	0;
int FT5402_FRAME_FILTER_ADD_MAX_TH		=	0;
int FT5402_FRAME_FILTER_SKIP_START_FRAME	=		0;
int FT5402_FRAME_FILTER_BAND_EN		=	0;
int FT5402_FRAME_FILTER_BAND_WIDTH		=	0;
int FT5402_OTP_PARAM_ID		=	0;


unsigned char g_ft5402_tx_num = 0;
unsigned char g_ft5402_rx_num = 0;
unsigned char g_ft5402_gain = 0;
unsigned char g_ft5402_voltage = 0;
unsigned char g_ft5402_scanselect = 0;
unsigned char g_ft5402_tx_offset = 0;


unsigned char *g_ft5402_tx_order;
unsigned char *g_ft5402_tx_cap;
unsigned char *g_ft5402_rx_order;
unsigned char *g_ft5402_rx_offset;
unsigned char *g_ft5402_rx_cap;


#endif