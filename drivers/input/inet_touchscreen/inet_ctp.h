#define TP_BASSADDRESS         (0xf1c25000)
#define TP_CTRL0               (0x00)
#define TP_CTRL1               (0x04)
#define TP_CTRL2               (0x08)
#define TP_CTRL3               (0x0c)
#define TP_INT_FIFOC           (0x10)
#define TP_INT_FIFOS           (0x14)
#define TP_TPR                 (0x18)
#define TP_CDAT                (0x1c)
#define TEMP_DATA              (0x20)
#define TP_DATA                (0x24)

#define MAX_DEVICE_NUM 16
#define ID_READ_COUNT 6
#define VALID_COUNT	2
#define ADC_TO_HIGH 2605 //2.1V VDD = 3.3V
static int device_id = 0;

//static int mSSD253xDriverType=0;  //inet ssd253x driver type ,will be valued by different modules.