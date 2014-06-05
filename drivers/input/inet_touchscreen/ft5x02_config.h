#ifndef __FT5X02_CONFIG_H__ 
#define __FT5X02_CONFIG_H__ 
/*FT5X02 config*/


unsigned char g_ft5x02_tx_num = 0;
unsigned char g_ft5x02_rx_num = 0;
unsigned char g_ft5x02_gain = 0;
unsigned char g_ft5x02_voltage = 0;
unsigned char g_ft5x02_scanselect = 0;

unsigned char g_ft5x02_tx_offset = 0;

unsigned char *g_ft5x02_tx_order;
unsigned char *g_ft5x02_tx_cap;
unsigned char *g_ft5x02_rx_order;
unsigned char *g_ft5x02_rx_offset;
unsigned char *g_ft5x02_rx_cap;


#endif