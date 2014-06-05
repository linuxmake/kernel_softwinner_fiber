#ifndef __LCD_TC358768_H__
#define __LCD_TC358768_H__
#include "../lcd_panel_cfg.h"

void TC358768_init(__panel_para_t * info);
void TC358768_exit(__panel_para_t * info);

#define TC358768_spi_csx_set(v)	(LCD_GPIO_write(0, 3, v))
#define TC358768_spi_sck_set(v)  (LCD_GPIO_write(0, 0, v))
#define TC358768_spi_sdi_set(v)  (LCD_GPIO_write(0, 1, v))

#define TC358768_spi_sdo_get()		(LCD_GPIO_read(0, 2))//for tc358768,gongpiqiang
#define TC358768_reset(v) (LCD_GPIO_write(0, 4, v))

#endif
