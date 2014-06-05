#include <mach/sys_config.h>//gongpiqiang+++
#include "TC358768.h"
extern int lcd_id_used,lcd_id;
//Rocky@20130823+
void SPI_32bit_4wire_write(__u32 tx)
{
	__u8 i;

	TC358768_spi_csx_set(0);

	for(i=0;i<32;i++)
	{
		LCD_delay_us(1);
		TC358768_spi_sck_set(0);
		LCD_delay_us(1);
		if(tx & 0x80000000)
			TC358768_spi_sdi_set(1);
		else
			TC358768_spi_sdi_set(0);
		LCD_delay_us(1);
		TC358768_spi_sck_set(1);
		LCD_delay_us(1);
		tx <<= 1;
	}
	TC358768_spi_sdi_set(1);
	LCD_delay_us(1);
	TC358768_spi_csx_set(1);
	LCD_delay_us(3);
}
__u32 SPI_32bit_4wire_read(__u32 tx)
{
	__u8 i;
	__u32 read_bit,read_data=0;

	tx|=0x10000;

	TC358768_spi_csx_set(0);
  //OSAL_PRINTF("\n readbit :\n");
	for(i=0;i<32;i++)
	{
		read_data <<= 1;
		LCD_delay_us(1);
		TC358768_spi_sck_set(0);
		LCD_delay_us(1);
		if(tx & 0x80000000)
			TC358768_spi_sdi_set(1);
		else
			TC358768_spi_sdi_set(0);
		LCD_delay_us(1);
		read_bit=TC358768_spi_sdo_get();
		//OSAL_PRINTF("%d",read_bit);
		if(read_bit)
			read_data |=  0x00000001;
		else
			read_data &= ~0x00000001;
		TC358768_spi_sck_set(1);
		LCD_delay_us(1);
		tx <<= 1;
	}
	TC358768_spi_sdi_set(1);
	LCD_delay_us(1);
	TC358768_spi_csx_set(1);
	LCD_delay_us(3);
	//OSAL_PRINTF("\n");
	return read_data&0xffff;
	
}
//Rocky@20130823-

void HSD6_98_OTM1283A_init(void)
{	

	printk("\n\nRocky HSD6_98_OTM1283A_init enter\n\n");

// **************************************************
// First initialization Sequence or RESUME Sequence
// **************************************************
// **************************************************
// Power on TC358768XBG according to recommended power-on sequence, if power is cut off
// Assert Reset (RESX="L")
// Deassert Reset (RESX="H")
// Start input REFCK and PCLK
// **************************************************
// **************************************************
// TC358768XBG Software Reset
// **************************************************
SPI_32bit_4wire_write(0x00020001);//SYSctl, S/W Reset
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00020000);//SYSctl, S/W Reset release
// TC358768XBG PLL,Clock Setting
// **************************************************
SPI_32bit_4wire_write(0x00161063);//PLL Control Register 0 (PLL_PRD,PLL_FBD)
SPI_32bit_4wire_write(0x00180603);//PLL_FRS,PLL_LBWS, PLL oscillation enable
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00180613);//PLL_FRS,PLL_LBWS, PLL clock out enable
// **************************************************
// TC358768XBG DPI Input Control
// **************************************************
SPI_32bit_4wire_write(0x00060032);//FIFO Control Register
// **************************************************
// TC358768XBG D-PHY Setting
// **************************************************
SPI_32bit_4wire_write(0x01400000);//D-PHY Clock lane enable
SPI_32bit_4wire_write(0x01420000);//
SPI_32bit_4wire_write(0x01440000);//D-PHY Data lane0 enable
SPI_32bit_4wire_write(0x01460000);//
SPI_32bit_4wire_write(0x01480000);//D-PHY Data lane1 enable
SPI_32bit_4wire_write(0x014A0000);//
SPI_32bit_4wire_write(0x014C0000);//D-PHY Data lane2 enable
SPI_32bit_4wire_write(0x014E0000);//
SPI_32bit_4wire_write(0x01500000);//D-PHY Data lane3 enable
SPI_32bit_4wire_write(0x01520000);//
SPI_32bit_4wire_write(0x01000002);//D-PHY Clock lane control
SPI_32bit_4wire_write(0x01020000);//
SPI_32bit_4wire_write(0x01040002);//D-PHY Data lane0 control
SPI_32bit_4wire_write(0x01060000);//
SPI_32bit_4wire_write(0x01080002);//D-PHY Data lane1 control
SPI_32bit_4wire_write(0x010A0000);//
SPI_32bit_4wire_write(0x010C0002);//D-PHY Data lane2 control
SPI_32bit_4wire_write(0x010E0000);//
SPI_32bit_4wire_write(0x01100002);//D-PHY Data lane3 control
SPI_32bit_4wire_write(0x01120000);//
// **************************************************
// TC358768XBG DSI-TX PPI Control
// **************************************************
SPI_32bit_4wire_write(0x02100FA0);//LINEINITCNT
SPI_32bit_4wire_write(0x02120000);//
SPI_32bit_4wire_write(0x02140002);//LPTXTIMECNT
SPI_32bit_4wire_write(0x02160000);//
SPI_32bit_4wire_write(0x02181202);//TCLK_HEADERCNT
SPI_32bit_4wire_write(0x021A0000);//
SPI_32bit_4wire_write(0x02200002);//THS_HEADERCNT
SPI_32bit_4wire_write(0x02220000);//
SPI_32bit_4wire_write(0x02244650);//TWAKEUPCNT
SPI_32bit_4wire_write(0x02260000);//
SPI_32bit_4wire_write(0x022C0001);//THS_TRAILCNT
SPI_32bit_4wire_write(0x022E0000);//
SPI_32bit_4wire_write(0x02300005);//HSTXVREGCNT
SPI_32bit_4wire_write(0x02320000);//
SPI_32bit_4wire_write(0x0234001F);//HSTXVREGEN enable
SPI_32bit_4wire_write(0x02360000);//
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP
SPI_32bit_4wire_write(0x023A0000);//
SPI_32bit_4wire_write(0x023C0002);//BTACNTRL1
SPI_32bit_4wire_write(0x023E0002);//
SPI_32bit_4wire_write(0x02040001);//STARTCNTRL
SPI_32bit_4wire_write(0x02060000);//
// **************************************************
// TC358768XBG DSI-TX Timing Control
// **************************************************
SPI_32bit_4wire_write(0x06200001);//Sync Pulse/Sync Event mode setting
SPI_32bit_4wire_write(0x06220010);//V Control Register1
SPI_32bit_4wire_write(0x0624000E);//V Control Register2
SPI_32bit_4wire_write(0x06260500);//V Control Register3
SPI_32bit_4wire_write(0x06280045);//H Control Register1
SPI_32bit_4wire_write(0x062A0026);//H Control Register2
SPI_32bit_4wire_write(0x062C0870);//H Control Register3
SPI_32bit_4wire_write(0x05180001);//DSI Start
SPI_32bit_4wire_write(0x051A0000);//
// **************************************************
// LCDD (Peripheral) Setting
// **************************************************
LCD_delay_ms(5);
// **************************************************
// Enable Continuous Clock
// **************************************************
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP
SPI_32bit_4wire_write(0x023A0000);//
LCD_delay_ms(150);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06100000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040004);//
SPI_32bit_4wire_write(0x061012FF);//
SPI_32bit_4wire_write(0x06120183);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06108000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040003);//
SPI_32bit_4wire_write(0x061012FF);//
SPI_32bit_4wire_write(0x06120083);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06108000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000A);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800C0);//
SPI_32bit_4wire_write(0x00E80064);//
SPI_32bit_4wire_write(0x00E81010);//
SPI_32bit_4wire_write(0x00E86400);//
SPI_32bit_4wire_write(0x00E81010);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040007);//
SPI_32bit_4wire_write(0x061000C0);//
SPI_32bit_4wire_write(0x06120056);//
SPI_32bit_4wire_write(0x06140001);//
SPI_32bit_4wire_write(0x06160004);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610A400);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061016C0);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B300);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040003);//
SPI_32bit_4wire_write(0x061000C0);//
SPI_32bit_4wire_write(0x06120050);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06108100);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061066C1);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061049C4);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610A000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E805C4);//
SPI_32bit_4wire_write(0x00E80610);//
SPI_32bit_4wire_write(0x00E80502);//
SPI_32bit_4wire_write(0x00E81015);//
SPI_32bit_4wire_write(0x00E81005);//
SPI_32bit_4wire_write(0x00E80207);//
SPI_32bit_4wire_write(0x00E81505);//
SPI_32bit_4wire_write(0x00E80010);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040003);//
SPI_32bit_4wire_write(0x061000C4);//
SPI_32bit_4wire_write(0x06120000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109100);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040004);//
SPI_32bit_4wire_write(0x061046C5);//
SPI_32bit_4wire_write(0x06120040);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06100000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040003);//
SPI_32bit_4wire_write(0x0610CFD8);//
SPI_32bit_4wire_write(0x061200CF);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06100000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061047d9);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06108100);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061082C4);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040003);//
SPI_32bit_4wire_write(0x061004C5);//
SPI_32bit_4wire_write(0x061200B8);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610BB00);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061080C5);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06108200);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061002C4);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610C600);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061003B0);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06100000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061040D0);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06100000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040003);//
SPI_32bit_4wire_write(0x061000D1);//
SPI_32bit_4wire_write(0x06120000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06108000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000C);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CB);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220010);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CB);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610A000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220010);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CB);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220010);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CB);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610C000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220010);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E805CB);//
SPI_32bit_4wire_write(0x00E80505);//
SPI_32bit_4wire_write(0x00E80505);//
SPI_32bit_4wire_write(0x00E80005);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80500);//
SPI_32bit_4wire_write(0x00E80005);//
SPI_32bit_4wire_write(0x00E80505);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610D000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220010);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CB);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80505);//
SPI_32bit_4wire_write(0x00E80505);//
SPI_32bit_4wire_write(0x00E80505);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610E000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CB);//
SPI_32bit_4wire_write(0x00E80500);//
SPI_32bit_4wire_write(0x00E80005);//
SPI_32bit_4wire_write(0x00E80505);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610F000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000C);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E8ffCB);//
SPI_32bit_4wire_write(0x00E8ffff);//
SPI_32bit_4wire_write(0x00E8ffff);//
SPI_32bit_4wire_write(0x00E8ffff);//
SPI_32bit_4wire_write(0x00E8ffff);//
SPI_32bit_4wire_write(0x00E8ffff);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06108000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220010);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E80ECC);//
SPI_32bit_4wire_write(0x00E80A10);//
SPI_32bit_4wire_write(0x00E8020C);//
SPI_32bit_4wire_write(0x00E80004);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E82E00);//
SPI_32bit_4wire_write(0x00E8002D);//
SPI_32bit_4wire_write(0x00E82A29);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220010);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CC);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80F0D);//
SPI_32bit_4wire_write(0x00E80B09);//
SPI_32bit_4wire_write(0x00E80301);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610A000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CC);//
SPI_32bit_4wire_write(0x00E82E00);//
SPI_32bit_4wire_write(0x00E8002D);//
SPI_32bit_4wire_write(0x00E82A29);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220010);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E80BCC);//
SPI_32bit_4wire_write(0x00E80F09);//
SPI_32bit_4wire_write(0x00E8030D);//
SPI_32bit_4wire_write(0x00E80001);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E82D00);//
SPI_32bit_4wire_write(0x00E8002E);//
SPI_32bit_4wire_write(0x00E82A29);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610C000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220010);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CC);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80A0C);//
SPI_32bit_4wire_write(0x00E80E10);//
SPI_32bit_4wire_write(0x00E80204);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610D000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CC);//
SPI_32bit_4wire_write(0x00E82D00);//
SPI_32bit_4wire_write(0x00E8002E);//
SPI_32bit_4wire_write(0x00E82A29);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06108000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000D);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E88BCE);//
SPI_32bit_4wire_write(0x00E81803);//
SPI_32bit_4wire_write(0x00E8038A);//
SPI_32bit_4wire_write(0x00E88918);//
SPI_32bit_4wire_write(0x00E81803);//
SPI_32bit_4wire_write(0x00E80388);//
SPI_32bit_4wire_write(0x00E80018);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CE);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610A000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E838CE);//
SPI_32bit_4wire_write(0x00E80507);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80018);//
SPI_32bit_4wire_write(0x00E80638);//
SPI_32bit_4wire_write(0x00E80105);//
SPI_32bit_4wire_write(0x00E81800);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E838CE);//
SPI_32bit_4wire_write(0x00E80505);//
SPI_32bit_4wire_write(0x00E80002);//
SPI_32bit_4wire_write(0x00E80018);//
SPI_32bit_4wire_write(0x00E80438);//
SPI_32bit_4wire_write(0x00E80305);//
SPI_32bit_4wire_write(0x00E81800);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610C000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E838CE);//
SPI_32bit_4wire_write(0x00E80503);//
SPI_32bit_4wire_write(0x00E80004);//
SPI_32bit_4wire_write(0x00E80018);//
SPI_32bit_4wire_write(0x00E80238);//
SPI_32bit_4wire_write(0x00E80505);//
SPI_32bit_4wire_write(0x00E81800);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610D000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E838CE);//
SPI_32bit_4wire_write(0x00E80501);//
SPI_32bit_4wire_write(0x00E80006);//
SPI_32bit_4wire_write(0x00E80018);//
SPI_32bit_4wire_write(0x00E80038);//
SPI_32bit_4wire_write(0x00E80705);//
SPI_32bit_4wire_write(0x00E81800);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06108000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CF);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CF);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610A000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CF);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000F);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800CF);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610C000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x0022000C);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E83DCF);//
SPI_32bit_4wire_write(0x00E81502);//
SPI_32bit_4wire_write(0x00E80020);//
SPI_32bit_4wire_write(0x00E80100);//
SPI_32bit_4wire_write(0x00E80081);//
SPI_32bit_4wire_write(0x00E80803);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B500);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040007);//
SPI_32bit_4wire_write(0x061000C5);//
SPI_32bit_4wire_write(0x0612FF6F);//
SPI_32bit_4wire_write(0x06146F00);//
SPI_32bit_4wire_write(0x061600FF);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040005);//
SPI_32bit_4wire_write(0x061002F5);//
SPI_32bit_4wire_write(0x06120211);//
SPI_32bit_4wire_write(0x06140011);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061050C5);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109400);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061066C5);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B200);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040003);//
SPI_32bit_4wire_write(0x061000F5);//
SPI_32bit_4wire_write(0x06120000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B400);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040003);//
SPI_32bit_4wire_write(0x061000F5);//
SPI_32bit_4wire_write(0x06120000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B600);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040003);//
SPI_32bit_4wire_write(0x061000F5);//
SPI_32bit_4wire_write(0x06120000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B800);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040003);//
SPI_32bit_4wire_write(0x061000F5);//
SPI_32bit_4wire_write(0x06120000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06109400);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061002F5);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610BA00);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061003F5);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B400);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610C0C5);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06100000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220011);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800E1);//
SPI_32bit_4wire_write(0x00E8120C);//
SPI_32bit_4wire_write(0x00E8080F);//
SPI_32bit_4wire_write(0x00E80b10);//
SPI_32bit_4wire_write(0x00E8040A);//
SPI_32bit_4wire_write(0x00E80D07);//
SPI_32bit_4wire_write(0x00E80E07);//
SPI_32bit_4wire_write(0x00E81015);//
SPI_32bit_4wire_write(0x00E80003);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06100000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//
SPI_32bit_4wire_write(0x00220011);//
SPI_32bit_4wire_write(0x00E08000);//
SPI_32bit_4wire_write(0x00E800E2);//
SPI_32bit_4wire_write(0x00E8120C);//
SPI_32bit_4wire_write(0x00E8070E);//
SPI_32bit_4wire_write(0x00E80b10);//
SPI_32bit_4wire_write(0x00E80409);//
SPI_32bit_4wire_write(0x00E80D07);//
SPI_32bit_4wire_write(0x00E80E07);//
SPI_32bit_4wire_write(0x00E81015);//
SPI_32bit_4wire_write(0x00E80003);//
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610B400);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061055C0);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x0610A000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061002C1);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06108100);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061007C1);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06021015);//
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x06100000);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x06024039);//
SPI_32bit_4wire_write(0x06040004);//
SPI_32bit_4wire_write(0x0610ffff);//
SPI_32bit_4wire_write(0x0612ffff);//
SPI_32bit_4wire_write(0x06000001);//
LCD_delay_ms(1);
LCD_delay_ms(20);
// 0x00,0x00
SPI_32bit_4wire_write(0x06021015);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write(no parameters)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100000);//bit7:0=DCS Command
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// 0x11,0x00
SPI_32bit_4wire_write(0x06021015);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write(no parameters)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100011);//bit7:0=DCS Command
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(120);
// 0x00,0x00
SPI_32bit_4wire_write(0x06021015);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write(no parameters)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100000);//bit7:0=DCS Command
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(10);
// R29
SPI_32bit_4wire_write(0x06021015);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write(no parameters)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100029);//bit7:0=DCS Command
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(100);
// **************************************************
// Set to HS mode
// **************************************************
SPI_32bit_4wire_write(0x05000086);//
SPI_32bit_4wire_write(0x0502A300);//
SPI_32bit_4wire_write(0x05008000);//
SPI_32bit_4wire_write(0x0502C300);//
// **************************************************
// Host: RGB(DPI) input start
// **************************************************
SPI_32bit_4wire_write(0x00080037);//
SPI_32bit_4wire_write(0x0050003E);//
SPI_32bit_4wire_write(0x00320000);//
SPI_32bit_4wire_write(0x00040044);//




		printk("\n\nRocky HSD6_98_OTM1283A_init leave\n\n");	
}

void HSD6_98_OTM1283A_exit(void)
{
	printk("\n\nRocky HSD6_98_OTM1283A_exit enter\n\n");
// **************************************************
// SUSPEND Sequence  Applicable only for TC358768AXBG
// By this sequence, lowest power state is realized. DSI output will be HiZ. To resume from this state, "TC358768XBG_INIT_OR_RESUME" is needed. 
// **************************************************
// set display off
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100028);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(60);
// Enter LCD sleep mode (How to enter depends on LCD spec. Following setting is a example)
// enter sleep
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100010);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(60);
// RGB Port Disable (from "SetFrmStop to 1" to "Set RstRstPtr to 1")(Only for TC358768AXBG)
SPI_32bit_4wire_write(0x00328000);//Set Frame Stop to 1
LCD_delay_ms(40);
SPI_32bit_4wire_write(0x00040004);//Configuration Control Register Parallel input stop
SPI_32bit_4wire_write(0x0032C000);//Set RstPtr to 1
// Stop DSI continuous clock
SPI_32bit_4wire_write(0x02380000);//
SPI_32bit_4wire_write(0x023A0000);//
// Disable D-PHY  By this setting, DSI line becomes HiZ.  LCD side should permit HiZ input.
SPI_32bit_4wire_write(0x01400001);//
SPI_32bit_4wire_write(0x01420000);//
SPI_32bit_4wire_write(0x01440001);//
SPI_32bit_4wire_write(0x01460000);//
SPI_32bit_4wire_write(0x01480001);//
SPI_32bit_4wire_write(0x014A0000);//
SPI_32bit_4wire_write(0x014C0001);//
SPI_32bit_4wire_write(0x014E0000);//
SPI_32bit_4wire_write(0x01500001);//
SPI_32bit_4wire_write(0x01520000);//
// **************************************************
// Host: Stop RGB input including PCLK.   Stop REFCLK
// **************************************************
// If all the power of TC358768AXBG can be cut off,
// Set all the input signal to TC358768AXBG "L" or "HiZ" including RESX.
// Cut off all the power of TC358768AXBG
// **************************************************


	printk("\n\nRocky HSD6_98_OTM1283A_exit leave\n\n");

	TC358768_spi_csx_set(0);
	TC358768_spi_sck_set(0);
	TC358768_spi_sdi_set(0);
}

void KR070IC4T_init(void)
{	
	printk("\n\nRocky KR070IC4T_init enter\n\n");

// **************************************************
// First initialization Sequence or RESUME Sequence
// **************************************************
// **************************************************
// Power on TC358768XBG according to recommended power-on sequence, if power is cut off
// Assert Reset (RESX="L")
// Deassert Reset (RESX="H")
// Start input REFCK and PCLK
// **************************************************
// **************************************************
// TC358768XBG Software Reset
// **************************************************
SPI_32bit_4wire_write(0x00020001);//SYSctl, S/W Reset
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00020000);//SYSctl, S/W Reset release
// TC358768XBG PLL,Clock Setting
// **************************************************
SPI_32bit_4wire_write(0x00161036);//PLL Control Register 0 (PLL_PRD,PLL_FBD)
SPI_32bit_4wire_write(0x00180203);//PLL_FRS,PLL_LBWS, PLL oscillation enable
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00180213);//PLL_FRS,PLL_LBWS, PLL clock out enable
// **************************************************
// TC358768XBG DPI Input Control
// **************************************************
SPI_32bit_4wire_write(0x000600C8);//FIFO Control Register
// **************************************************
// TC358768XBG D-PHY Setting
// **************************************************
SPI_32bit_4wire_write(0x01400000);//D-PHY Clock lane enable
SPI_32bit_4wire_write(0x01420000);//
SPI_32bit_4wire_write(0x01440000);//D-PHY Data lane0 enable
SPI_32bit_4wire_write(0x01460000);//
SPI_32bit_4wire_write(0x01480000);//D-PHY Data lane1 enable
SPI_32bit_4wire_write(0x014A0000);//
SPI_32bit_4wire_write(0x014C0000);//D-PHY Data lane2 enable
SPI_32bit_4wire_write(0x014E0000);//
SPI_32bit_4wire_write(0x01500000);//D-PHY Data lane3 enable
SPI_32bit_4wire_write(0x01520000);//
SPI_32bit_4wire_write(0x01000002);//D-PHY Clock lane control
SPI_32bit_4wire_write(0x01020000);//
SPI_32bit_4wire_write(0x01040002);//D-PHY Data lane0 control
SPI_32bit_4wire_write(0x01060000);//
SPI_32bit_4wire_write(0x01080002);//D-PHY Data lane1 control
SPI_32bit_4wire_write(0x010A0000);//
SPI_32bit_4wire_write(0x010C0002);//D-PHY Data lane2 control
SPI_32bit_4wire_write(0x010E0000);//
SPI_32bit_4wire_write(0x01100002);//D-PHY Data lane3 control
SPI_32bit_4wire_write(0x01120000);//
// **************************************************
// TC358768XBG DSI-TX PPI Control
// **************************************************
SPI_32bit_4wire_write(0x02100FA0);//LINEINITCNT
SPI_32bit_4wire_write(0x02120000);//
SPI_32bit_4wire_write(0x02140004);//LPTXTIMECNT
SPI_32bit_4wire_write(0x02160000);//
SPI_32bit_4wire_write(0x02181204);//TCLK_HEADERCNT
SPI_32bit_4wire_write(0x021A0000);//
SPI_32bit_4wire_write(0x02200003);//THS_HEADERCNT
SPI_32bit_4wire_write(0x02220000);//
SPI_32bit_4wire_write(0x02244650);//TWAKEUPCNT
SPI_32bit_4wire_write(0x02260000);//
SPI_32bit_4wire_write(0x022C0002);//THS_TRAILCNT
SPI_32bit_4wire_write(0x022E0000);//
SPI_32bit_4wire_write(0x02300005);//HSTXVREGCNT
SPI_32bit_4wire_write(0x02320000);//
SPI_32bit_4wire_write(0x0234001F);//HSTXVREGEN enable
SPI_32bit_4wire_write(0x02360000);//
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP
SPI_32bit_4wire_write(0x023A0000);//
SPI_32bit_4wire_write(0x023C0003);//BTACNTRL1
SPI_32bit_4wire_write(0x023E0004);//
SPI_32bit_4wire_write(0x02040001);//STARTCNTRL
SPI_32bit_4wire_write(0x02060000);//
// **************************************************
// TC358768XBG DSI-TX Timing Control
// **************************************************
SPI_32bit_4wire_write(0x06200001);//Sync Pulse/Sync Event mode setting
SPI_32bit_4wire_write(0x0622000C);//V Control Register1
SPI_32bit_4wire_write(0x06240004);//V Control Register2
SPI_32bit_4wire_write(0x06260500);//V Control Register3
SPI_32bit_4wire_write(0x06280219);//H Control Register1
SPI_32bit_4wire_write(0x062A01E2);//H Control Register2
SPI_32bit_4wire_write(0x062C0960);//H Control Register3
SPI_32bit_4wire_write(0x05180001);//DSI Start
SPI_32bit_4wire_write(0x051A0000);//
// **************************************************
// LCDD (Peripheral) Setting
// **************************************************
LCD_delay_ms(5);
// **************************************************
// Enable Continuous Clock
// **************************************************
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP
SPI_32bit_4wire_write(0x023A0000);//
LCD_delay_ms(150);
// F0,5A,5A
SPI_32bit_4wire_write(0x06024039);//bit15:8=Packet Type:Long Packet, bit5:0=Data Type DCS Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AF0);//bit7:0=DCS Command(xx), bit15:8=Parameter1(gg)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter2(hh), bit15:8=Parameter3(ii)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// F1,5A,5A
SPI_32bit_4wire_write(0x06024039);//bit15:8=Packet Type:Long Packet, bit5:0=Data Type DCS Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AF1);//bit7:0=DCS Command(xx), bit15:8=Parameter1(gg)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter2(hh), bit15:8=Parameter3(ii)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// FC,A5,A5
SPI_32bit_4wire_write(0x06024039);//bit15:8=Packet Type:Long Packet, bit5:0=Data Type DCS Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x0610A5FC);//bit7:0=DCS Command(xx), bit15:8=Parameter1(gg)
SPI_32bit_4wire_write(0x061200A5);//bit7:0=Parameter2(hh), bit15:8=Parameter3(ii)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// D0,00,10
SPI_32bit_4wire_write(0x06024039);//bit15:8=Packet Type:Long Packet, bit5:0=Data Type DCS Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061000D0);//bit7:0=DCS Command(xx), bit15:8=Parameter1(gg)
SPI_32bit_4wire_write(0x06120010);//bit7:0=Parameter2(hh), bit15:8=Parameter3(ii)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// B1,10
SPI_32bit_4wire_write(0x06024039);//bit15:8=Packet Type:Long Packet, bit5:0=Data Type DCS Long Write
SPI_32bit_4wire_write(0x06040002);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061010B1);//bit7:0=DCS Command(xx), bit15:8=Parameter1(gg)
SPI_32bit_4wire_write(0x06120000);//bit7:0=Parameter2(hh), bit15:8=Parameter3(ii)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// B2,14,22,2F,04
SPI_32bit_4wire_write(0x06024039);//bit15:8=Packet Type:Long Packet, bit5:0=Data Type DCS Long Write
SPI_32bit_4wire_write(0x06040005);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061014B2);//bit7:0=DCS Command(xx), bit15:8=Parameter1(gg)
SPI_32bit_4wire_write(0x06122F22);//bit7:0=Parameter2(hh), bit15:8=Parameter3(ii)
SPI_32bit_4wire_write(0x06140004);//
SPI_32bit_4wire_write(0x06160000);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// F2,02,08,08,90,10
SPI_32bit_4wire_write(0x06024039);//bit15:8=Packet Type:Long Packet, bit5:0=Data Type DCS Long Write
SPI_32bit_4wire_write(0x06040006);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061002F2);//bit7:0=DCS Command(xx), bit15:8=Parameter1(gg)
SPI_32bit_4wire_write(0x06120808);//bit7:0=Parameter2(hh), bit15:8=Parameter3(ii)
SPI_32bit_4wire_write(0x06141090);//
SPI_32bit_4wire_write(0x06160000);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// F3,01?-..
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//Data ID setting
SPI_32bit_4wire_write(0x0022000B);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E801F3);//
SPI_32bit_4wire_write(0x00E8E2D7);//
SPI_32bit_4wire_write(0x00E8F462);//
SPI_32bit_4wire_write(0x00E877F7);//
SPI_32bit_4wire_write(0x00E8263C);//If data contents byte is not multiple of 4, add 0x00 to be multiple of 4.
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Keep Mask High to prevent short packets send out
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
// F4,00?-?-
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//Data ID setting
SPI_32bit_4wire_write(0x0022002E);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E800F4);//
SPI_32bit_4wire_write(0x00E80302);//
SPI_32bit_4wire_write(0x00E80326);//
SPI_32bit_4wire_write(0x00E80902);//
SPI_32bit_4wire_write(0x00E80700);//If data contents byte is not multiple of 4, add 0x00 to be multiple of 4.
SPI_32bit_4wire_write(0x00E81616);//
SPI_32bit_4wire_write(0x00E80003);//
SPI_32bit_4wire_write(0x00E80808);//
SPI_32bit_4wire_write(0x00E80E03);//
SPI_32bit_4wire_write(0x00E8120F);//
SPI_32bit_4wire_write(0x00E81D1C);//
SPI_32bit_4wire_write(0x00E80C1E);//
SPI_32bit_4wire_write(0x00E80109);//
SPI_32bit_4wire_write(0x00E80204);//
SPI_32bit_4wire_write(0x00E87461);//
SPI_32bit_4wire_write(0x00E87275);//
SPI_32bit_4wire_write(0x00E88083);//
SPI_32bit_4wire_write(0x00E8B080);//
SPI_32bit_4wire_write(0x00E80100);//
SPI_32bit_4wire_write(0x00E82801);//
SPI_32bit_4wire_write(0x00E80304);//
SPI_32bit_4wire_write(0x00E80128);//
SPI_32bit_4wire_write(0x00E832D1);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Keep Mask High to prevent short packets send out
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
// F5 83,?-..
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//Data ID setting
SPI_32bit_4wire_write(0x0022001B);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E883F5);//
SPI_32bit_4wire_write(0x00E82828);//
SPI_32bit_4wire_write(0x00E8AB5F);//
SPI_32bit_4wire_write(0x00E85298);//
SPI_32bit_4wire_write(0x00E8330F);//If data contents byte is not multiple of 4, add 0x00 to be multiple of 4.
SPI_32bit_4wire_write(0x00E80443);//
SPI_32bit_4wire_write(0x00E85459);//
SPI_32bit_4wire_write(0x00E80552);//
SPI_32bit_4wire_write(0x00E86040);//
SPI_32bit_4wire_write(0x00E8604E);//
SPI_32bit_4wire_write(0x00E82740);//
SPI_32bit_4wire_write(0x00E85226);//
SPI_32bit_4wire_write(0x00E86D25);//
SPI_32bit_4wire_write(0x00E80018);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Keep Mask High to prevent short packets send out
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
// EE,00?-.
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//Data ID setting
SPI_32bit_4wire_write(0x00220009);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E800EE);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//If data contents byte is not multiple of 4, add 0x00 to be multiple of 4.
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Keep Mask High to prevent short packets send out
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
// EF,12?-.
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//Data ID setting
SPI_32bit_4wire_write(0x00220009);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E812EF);//
SPI_32bit_4wire_write(0x00E84312);//
SPI_32bit_4wire_write(0x00E89043);//
SPI_32bit_4wire_write(0x00E82484);//
SPI_32bit_4wire_write(0x00E80081);//If data contents byte is not multiple of 4, add 0x00 to be multiple of 4.
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Keep Mask High to prevent short packets send out
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
// F6,63?-.
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//Data ID setting
SPI_32bit_4wire_write(0x00220007);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E863F6);//
SPI_32bit_4wire_write(0x00E8A625);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80014);//If data contents byte is not multiple of 4, add 0x00 to be multiple of 4.
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
// F7,0A
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//Data ID setting
SPI_32bit_4wire_write(0x00220021);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E80AF7);//
SPI_32bit_4wire_write(0x00E8080A);//
SPI_32bit_4wire_write(0x00E80B08);//
SPI_32bit_4wire_write(0x00E8090B);//If data contents byte is not multiple of 4, add 0x00 to be multiple of 4.
SPI_32bit_4wire_write(0x00E80409);//
SPI_32bit_4wire_write(0x00E80105);//
SPI_32bit_4wire_write(0x00E80101);//
SPI_32bit_4wire_write(0x00E80101);//
SPI_32bit_4wire_write(0x00E80A01);//
SPI_32bit_4wire_write(0x00E8080A);//
SPI_32bit_4wire_write(0x00E80B08);//
SPI_32bit_4wire_write(0x00E8090B);//
SPI_32bit_4wire_write(0x00E80409);//
SPI_32bit_4wire_write(0x00E80501);//
SPI_32bit_4wire_write(0x00E80101);//
SPI_32bit_4wire_write(0x00E80101);//
SPI_32bit_4wire_write(0x00E80001);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Keep Mask High to prevent short packets send out
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
// Fa,00,34?-
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//Data ID setting
SPI_32bit_4wire_write(0x00220012);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E800FA);//
SPI_32bit_4wire_write(0x00E80134);//
SPI_32bit_4wire_write(0x00E80E05);//
SPI_32bit_4wire_write(0x00E80C07);//If data contents byte is not multiple of 4, add 0x00 to be multiple of 4.
SPI_32bit_4wire_write(0x00E81412);//
SPI_32bit_4wire_write(0x00E8231C);//
SPI_32bit_4wire_write(0x00E8342B);//
SPI_32bit_4wire_write(0x00E82E35);//
SPI_32bit_4wire_write(0x00E8302D);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Keep Mask High to prevent short packets send out
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
// Fb,00,34?-
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500039);//Data ID setting
SPI_32bit_4wire_write(0x00220012);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E800FB);//
SPI_32bit_4wire_write(0x00E80134);//
SPI_32bit_4wire_write(0x00E80E05);//
SPI_32bit_4wire_write(0x00E80C07);//If data contents byte is not multiple of 4, add 0x00 to be multiple of 4.
SPI_32bit_4wire_write(0x00E81412);//
SPI_32bit_4wire_write(0x00E8231C);//
SPI_32bit_4wire_write(0x00E8342B);//
SPI_32bit_4wire_write(0x00E82E35);//
SPI_32bit_4wire_write(0x00E8302D);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Keep Mask High to prevent short packets send out
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//
SPI_32bit_4wire_write(0x00E00000);//
LCD_delay_ms(1);
// FD,16?-
SPI_32bit_4wire_write(0x06024039);//bit15:8=Packet Type:Long Packet, bit5:0=Data Type DCS Long Write
SPI_32bit_4wire_write(0x06040006);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061016FD);//bit7:0=DCS Command(xx), bit15:8=Parameter1(gg)
SPI_32bit_4wire_write(0x06121110);//bit7:0=Parameter2(hh), bit15:8=Parameter3(ii)
SPI_32bit_4wire_write(0x06140923);//
SPI_32bit_4wire_write(0x06160000);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// FE,00
SPI_32bit_4wire_write(0x06024039);//bit15:8=Packet Type:Long Packet, bit5:0=Data Type DCS Long Write
SPI_32bit_4wire_write(0x06040007);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061000FE);//bit7:0=DCS Command(xx), bit15:8=Parameter1(gg)
SPI_32bit_4wire_write(0x06120302);//bit7:0=Parameter2(hh), bit15:8=Parameter3(ii)
SPI_32bit_4wire_write(0x06140021);//
SPI_32bit_4wire_write(0x06160078);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// R35
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write(no parameters)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100035);//bit7:0=DCS Command
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// R35
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write(no parameters)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100011);//bit7:0=DCS Command
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(20);
// c3,40,00,28
SPI_32bit_4wire_write(0x06024039);//bit15:8=Packet Type:Long Packet, bit5:0=Data Type DCS Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061040C3);//bit7:0=DCS Command(xx), bit15:8=Parameter1(gg)
SPI_32bit_4wire_write(0x06122800);//bit7:0=Parameter2(hh), bit15:8=Parameter3(ii)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// R29
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write(no parameters)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100029);//bit7:0=DCS Command
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(10);
// **************************************************
// Set to HS mode
// **************************************************
SPI_32bit_4wire_write(0x05000086);//
SPI_32bit_4wire_write(0x0502A300);//
SPI_32bit_4wire_write(0x05008000);//
SPI_32bit_4wire_write(0x0502C300);//
// **************************************************
// Host: RGB(DPI) input start
// **************************************************
SPI_32bit_4wire_write(0x00080037);//
SPI_32bit_4wire_write(0x0050003E);//
SPI_32bit_4wire_write(0x00320000);//
SPI_32bit_4wire_write(0x00040044);//





		printk("\n\nRocky KR070IC4T_init leave\n\n");	
}

void KR070IC4T_exit(void)
{
	printk("\n\nRocky KR070IC4T_exit enter\n\n");
// **************************************************
// SUSPEND Sequence  Applicable only for TC358768AXBG
// By this sequence, lowest power state is realized. DSI output will be HiZ. To resume from this state, "TC358768XBG_INIT_OR_RESUME" is needed. 
// **************************************************
// set display off
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100028);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(60);
// Enter LCD sleep mode (How to enter depends on LCD spec. Following setting is a example)
// enter sleep
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100010);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(60);
// RGB Port Disable (from "SetFrmStop to 1" to "Set RstRstPtr to 1")(Only for TC358768AXBG)
SPI_32bit_4wire_write(0x00328000);//Set Frame Stop to 1
LCD_delay_ms(40);
SPI_32bit_4wire_write(0x00040004);//Configuration Control Register Parallel input stop
SPI_32bit_4wire_write(0x0032C000);//Set RstPtr to 1
// Stop DSI continuous clock
SPI_32bit_4wire_write(0x02380000);//
SPI_32bit_4wire_write(0x023A0000);//
// Disable D-PHY  By this setting, DSI line becomes HiZ.  LCD side should permit HiZ input.
SPI_32bit_4wire_write(0x01400001);//
SPI_32bit_4wire_write(0x01420000);//
SPI_32bit_4wire_write(0x01440001);//
SPI_32bit_4wire_write(0x01460000);//
SPI_32bit_4wire_write(0x01480001);//
SPI_32bit_4wire_write(0x014A0000);//
SPI_32bit_4wire_write(0x014C0001);//
SPI_32bit_4wire_write(0x014E0000);//
SPI_32bit_4wire_write(0x01500001);//
SPI_32bit_4wire_write(0x01520000);//
// **************************************************
// Host: Stop RGB input including PCLK.   Stop REFCLK
// **************************************************
// If all the power of TC358768AXBG can be cut off,
// Set all the input signal to TC358768AXBG "L" or "HiZ" including RESX.
// Cut off all the power of TC358768AXBG
// **************************************************


	printk("\n\nRocky KR070IC4T_exit leave\n\n");

	TC358768_spi_csx_set(0);
	TC358768_spi_sck_set(0);
	TC358768_spi_sdi_set(0);
}

void S6D7AA0X01_BOE_init(void)//xingyuan panel
{	
		printk("\n\nS6D7AA0X01_BOE_init enter\n\n");
		// **************************************************			   
// First initialization Sequence or RESUME Sequence			   
// **************************************************			   
// **************************************************			   
// Power on TC358768XBG according to recommended power-on sequence, if power is cut off			   
// Assert Reset (RESX="L")			   
// Deassert Reset (RESX="H")			   
// Start input REFCK and PCLK			   
// **************************************************			   
// **************************************************			   
// TC358768XBG Software Reset			   
// **************************************************			   
SPI_32bit_4wire_write(0x00020001);//SYSctl, S/W Reset			   
LCD_delay_ms(1);			   
SPI_32bit_4wire_write(0x00020000);//SYSctl, S/W Reset release			   
// TC358768XBG PLL,Clock Setting			   
// **************************************************			   
SPI_32bit_4wire_write(0x00161068);//PLL Control Register 0 (PLL_PRD,PLL_FBD)
SPI_32bit_4wire_write(0x00180603);//PLL_FRS,PLL_LBWS, PLL oscillation enable
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00180613);//PLL_FRS,PLL_LBWS, PLL clock out enable
// **************************************************			   
// TC358768XBG DPI Input Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x00060064);//FIFO Control Register
// **************************************************			   
// TC358768XBG D-PHY Setting			   
// **************************************************			   
SPI_32bit_4wire_write(0x01400000);//D-PHY Clock lane enable			   
SPI_32bit_4wire_write(0x01420000);//			   
SPI_32bit_4wire_write(0x01440000);//D-PHY Data lane0 enable			   
SPI_32bit_4wire_write(0x01460000);//			   
SPI_32bit_4wire_write(0x01480000);//D-PHY Data lane1 enable			   
SPI_32bit_4wire_write(0x014A0000);//			   
SPI_32bit_4wire_write(0x014C0000);//D-PHY Data lane2 enable			   
SPI_32bit_4wire_write(0x014E0000);//			   
SPI_32bit_4wire_write(0x01500000);//D-PHY Data lane3 enable			   
SPI_32bit_4wire_write(0x01520000);//			   
SPI_32bit_4wire_write(0x01000002);//D-PHY Clock lane control			   
SPI_32bit_4wire_write(0x01020000);//			   
SPI_32bit_4wire_write(0x01040002);//D-PHY Data lane0 control			   
SPI_32bit_4wire_write(0x01060000);//			   
SPI_32bit_4wire_write(0x01080002);//D-PHY Data lane1 control			   
SPI_32bit_4wire_write(0x010A0000);//			   
SPI_32bit_4wire_write(0x010C0002);//D-PHY Data lane2 control			   
SPI_32bit_4wire_write(0x010E0000);//			   
SPI_32bit_4wire_write(0x01100002);//D-PHY Data lane3 control			   
SPI_32bit_4wire_write(0x01120000);//			   
// **************************************************			   
// TC358768XBG DSI-TX PPI Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x02101388);//LINEINITCNT			   
SPI_32bit_4wire_write(0x02120000);//			   
SPI_32bit_4wire_write(0x02140003);//LPTXTIMECNT
SPI_32bit_4wire_write(0x02160000);//			   
SPI_32bit_4wire_write(0x02181203);//TCLK_HEADERCNT
SPI_32bit_4wire_write(0x021A0000);//			   
SPI_32bit_4wire_write(0x02200003);//THS_HEADERCNT			   
SPI_32bit_4wire_write(0x02220000);//			   
SPI_32bit_4wire_write(0x02243A98);//TWAKEUPCNT			   
SPI_32bit_4wire_write(0x02260000);//			   
SPI_32bit_4wire_write(0x022C0001);//THS_TRAILCNT
SPI_32bit_4wire_write(0x022E0000);//			   
SPI_32bit_4wire_write(0x02300005);//HSTXVREGCNT			   
SPI_32bit_4wire_write(0x02320000);//			   
SPI_32bit_4wire_write(0x0234001F);//HSTXVREGEN enable			   
SPI_32bit_4wire_write(0x02360000);//			   
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP			   
SPI_32bit_4wire_write(0x023A0000);//			   
SPI_32bit_4wire_write(0x023C0003);//BTACNTRL1			   
SPI_32bit_4wire_write(0x023E0003);//
SPI_32bit_4wire_write(0x02040001);//STARTCNTRL			   
SPI_32bit_4wire_write(0x02060000);//			   
// **************************************************			   
// TC358768XBG DSI-TX Timing Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x06200001);//Sync Pulse/Sync Event mode setting			   
SPI_32bit_4wire_write(0x0622000C);//V Control Register1    8,OLD
SPI_32bit_4wire_write(0x06240008);//V Control Register2    3,OLD
SPI_32bit_4wire_write(0x06260500);//V Control Register3
SPI_32bit_4wire_write(0x06280187);//H Control Register1
SPI_32bit_4wire_write(0x062A0166);//H Control Register2
SPI_32bit_4wire_write(0x062C0960);//H Control Register3
SPI_32bit_4wire_write(0x05180001);//DSI Start
SPI_32bit_4wire_write(0x051A0000);//
// **************************************************
// LCDD (Peripheral) Setting
// **************************************************
LCD_delay_ms(5);
// **************************************************
// Enable Continuous Clock
// **************************************************
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP
SPI_32bit_4wire_write(0x023A0000);//
LCD_delay_ms(150);
// **************************************************
// REM GP_COMMAD_PA(3);
// REM SPI_WriteData(0xF0);
// REM  SPI_WriteData(0x5A);
// REM  SPI_WriteData(0x5A);
// **************************************************
// **************************************************
// Example of Generic Long Write (Up to 8byte parameters)
// **************************************************
// Data Type 29h
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AF0);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// **************************************************
// Example of Generic Long Write (Up to 8byte parameters)
// **************************************************
// Data Type 29h
// SPI_WriteData(0xF1);
// REM     SPI_WriteData(0x5A); 
// REM     SPI_WriteData(0x5A);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AF1);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM SPI_WriteData(0xFC);
// REM SPI_WriteData(0xA5); 
// REM SPI_WriteData(0xA5);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x0610A5FC);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x061200A5);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM SPI_WriteData(0xD0); 
// REM SPI_WriteData(0x00); 
// REM SPI_WriteData(0x10);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061000D0);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06120010);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// **************************************************
// Example of Generic Short Write (2 parameters)
// **************************************************
// Data Type 23h
// Parameter1 xxh
// Parameter2 yyh
// REM     SPI_WriteData(0xB1);
// REM     SPI_WriteData(0x10);
SPI_32bit_4wire_write(0x06021023);//bit15:8=Packet Type:Short, bit5:0=Data Type Generic Short Write(2 parameters)
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061010B1);//bit7:0=Parameter1(xx), bit15:8=Parameter2(yy)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(5);
// REM  SPI_WriteData(0xB2);
// REM  SPI_WriteData(0x14);
// REM  SPI_WriteData(0x22);
// REM  SPI_WriteData(0x2F);
// REM  SPI_WriteData(0x04);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040005);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061014B2);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122F22);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06140004);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160000);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM SPI_WriteData(0xB5);
// REM SPI_WriteData(0x01);
// **************************************************
// Example of Generic Short Write (2 parameters)
// **************************************************
// Data Type 23h
// Parameter1 xxh
// Parameter2 yyh
SPI_32bit_4wire_write(0x06021023);//bit15:8=Packet Type:Short, bit5:0=Data Type Generic Short Write(2 parameters)
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061001B5);//bit7:0=Parameter1(xx), bit15:8=Parameter2(yy)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM     GP_COMMAD_PA(9);
// REM     SPI_WriteData(0xEE);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
// SPI_WriteData(0x1F);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
// SPI_WriteData(0x1F);
// SPI_WriteData(0x00);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220009);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E800EE);//
SPI_32bit_4wire_write(0x00E81F00);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E81F00);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM  GP_COMMAD_PA(14);
// REM    SPI_WriteData(0xEF);
// REM     SPI_WriteData(0x56);
// SPI_WriteData(0x34);
// SPI_WriteData(0x43);
// SPI_WriteData(0x65);
// SPI_WriteData(0x90);
// SPI_WriteData(0x80);
// SPI_WriteData(0x24);
// SPI_WriteData(0x80);
// SPI_WriteData(0x00);
// SPI_WriteData(0x91);
// SPI_WriteData(0x11);
// SPI_WriteData(0x11);
// SPI_WriteData(0x11);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x0022000E);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E856EF);//
SPI_32bit_4wire_write(0x00E84334);//
SPI_32bit_4wire_write(0x00E89065);//
SPI_32bit_4wire_write(0x00E82480);//
SPI_32bit_4wire_write(0x00E80080);//
SPI_32bit_4wire_write(0x00E81191);//
SPI_32bit_4wire_write(0x00E81111);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM    GP_COMMAD_PA(33);
// REM    SPI_WriteData(0xF7);
// REM    SPI_WriteData(0x04);
// SPI_WriteData(0x08);
// SPI_WriteData(0x09);
// SPI_WriteData(0x0A);
// SPI_WriteData(0x0B);
// SPI_WriteData(0x0C);
// SPI_WriteData(0x0D);
// SPI_WriteData(0x0E);
// SPI_WriteData(0x0F);
// SPI_WriteData(0x16);
// SPI_WriteData(0x17);
// SPI_WriteData(0x10);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// SPI_WriteData(0x04);
// SPI_WriteData(0x08);
// SPI_WriteData(0x09);
// SPI_WriteData(0x0A);
// SPI_WriteData(0x0B);
// SPI_WriteData(0x0C);
// SPI_WriteData(0x0D);
// SPI_WriteData(0x0E);
// SPI_WriteData(0x0F);
// SPI_WriteData(0x16);
// SPI_WriteData(0x17);
// SPI_WriteData(0x10);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220021);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E80EF7);//
SPI_32bit_4wire_write(0x00E80A0E);//
SPI_32bit_4wire_write(0x00E80F0A);//
SPI_32bit_4wire_write(0x00E80B0F);//
SPI_32bit_4wire_write(0x00E8050B);//
SPI_32bit_4wire_write(0x00E80107);//
SPI_32bit_4wire_write(0x00E80101);//
SPI_32bit_4wire_write(0x00E80101);//
SPI_32bit_4wire_write(0x00E80C01);//
SPI_32bit_4wire_write(0x00E8080C);//
SPI_32bit_4wire_write(0x00E80D08);//
SPI_32bit_4wire_write(0x00E8090D);//
SPI_32bit_4wire_write(0x00E80409);//
SPI_32bit_4wire_write(0x00E80106);//
SPI_32bit_4wire_write(0x00E80101);//
SPI_32bit_4wire_write(0x00E80101);//
SPI_32bit_4wire_write(0x00E80001);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM     GP_COMMAD_PA(6);
// REM     SPI_WriteData(0xF2);
// REM     SPI_WriteData(0x02);
// SPI_WriteData(0x08);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040006);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061002F2);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06120808);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06141040);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160000);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(6);
// REM     SPI_WriteData(0xF6);
// REM     SPI_WriteData(0x60);
// SPI_WriteData(0x25);
// SPI_WriteData(0x26);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040006);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061063F6);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612A621);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06140000);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160014);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(18);
// REM     SPI_WriteData(0xFA);
// REM     SPI_WriteData(0x04);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// Parameter1 xxh
// Parameter2 ggh
// Parameter3 hhh
// Parameter4 iih
// Parameter5 jjh
// Parameter6 kkh
// Parameter7 llh
// Parameter8 mmh
// |
// ParameterN nnh
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220012);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E804FA);//
SPI_32bit_4wire_write(0x00E80735);//
SPI_32bit_4wire_write(0x00E8120B);//
SPI_32bit_4wire_write(0x00E8100B);//
SPI_32bit_4wire_write(0x00E81A16);//
SPI_32bit_4wire_write(0x00E82C24);//
SPI_32bit_4wire_write(0x00E83B33);//
SPI_32bit_4wire_write(0x00E8333B);//
SPI_32bit_4wire_write(0x00E83334);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(18);
// REM    SPI_WriteData(0xFB);
// REM    SPI_WriteData(0x04);
// SPI_WriteData(0x35);
// SPI_WriteData(0x07);
// SPI_WriteData(0x0B);
// SPI_WriteData(0x12);
// SPI_WriteData(0x0B);
// SPI_WriteData(0x10);
// SPI_WriteData(0x16);
// SPI_WriteData(0x1A);
// SPI_WriteData(0x24);
// SPI_WriteData(0x2C);
// SPI_WriteData(0x33);
// SPI_WriteData(0x3B);
// SPI_WriteData(0x3B);
// SPI_WriteData(0x33);
// SPI_WriteData(0x34);
// SPI_WriteData(0x33);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// Parameter1 xxh
// Parameter2 ggh
// Parameter3 hhh
// Parameter4 iih
// Parameter5 jjh
// Parameter6 kkh
// Parameter7 llh
// Parameter8 mmh
// |
// ParameterN nnh
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220012);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E804FB);//
SPI_32bit_4wire_write(0x00E80735);//
SPI_32bit_4wire_write(0x00E8120B);//
SPI_32bit_4wire_write(0x00E8100B);//
SPI_32bit_4wire_write(0x00E81A16);//
SPI_32bit_4wire_write(0x00E82C24);//
SPI_32bit_4wire_write(0x00E83B33);//
SPI_32bit_4wire_write(0x00E8333B);//
SPI_32bit_4wire_write(0x00E83334);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(11);
// REM    SPI_WriteData(0xF3);
// REM    SPI_WriteData(0x01);
// SPI_WriteData(0xC4);
// SPI_WriteData(0xE0);
// SPI_WriteData(0x62);
// SPI_WriteData(0xD4);
// SPI_WriteData(0x83);
// SPI_WriteData(0x37);
// SPI_WriteData(0x3C);
// SPI_WriteData(0x24);
// SPI_WriteData(0x00);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x0022000B);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E801F3);//
SPI_32bit_4wire_write(0x00E8E0C4);//
SPI_32bit_4wire_write(0x00E8D462);//
SPI_32bit_4wire_write(0x00E83783);//
SPI_32bit_4wire_write(0x00E8243C);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(36);
// REM    SPI_WriteData(0xF4);
// REM    SPI_WriteData(0x00);
// SPI_WriteData(0x02);
// SPI_WriteData(0x03);
// SPI_WriteData(0x26);
// SPI_WriteData(0x03);
// SPI_WriteData(0x02);
// SPI_WriteData(0x09);
// SPI_WriteData(0x00);
// SPI_WriteData(0x07);
// SPI_WriteData(0x16);
// SPI_WriteData(0x16);
// SPI_WriteData(0x03);
// SPI_WriteData(0x00);
// SPI_WriteData(0x08);
// SPI_WriteData(0x08);
// SPI_WriteData(0x03);
// SPI_WriteData(0x19);
// SPI_WriteData(0x1C);
// SPI_WriteData(0x12);
// SPI_WriteData(0x1C);
// SPI_WriteData(0x1D);
// SPI_WriteData(0x1E);
// SPI_WriteData(0x1A);
// SPI_WriteData(0x09);
// SPI_WriteData(0x01);
// SPI_WriteData(0x04);
// SPI_WriteData(0x02);
// SPI_WriteData(0x61);
// SPI_WriteData(0x74);
// SPI_WriteData(0x75);
// SPI_WriteData(0x72);
// SPI_WriteData(0x83);
// SPI_WriteData(0x80);
// SPI_WriteData(0x80);
// SPI_WriteData(0xF0);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220024);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E800F4);//
SPI_32bit_4wire_write(0x00E80302);//
SPI_32bit_4wire_write(0x00E80326);//
SPI_32bit_4wire_write(0x00E80902);//
SPI_32bit_4wire_write(0x00E80700);//
SPI_32bit_4wire_write(0x00E81616);//
SPI_32bit_4wire_write(0x00E80003);//
SPI_32bit_4wire_write(0x00E80808);//
SPI_32bit_4wire_write(0x00E81903);//
SPI_32bit_4wire_write(0x00E8121C);//
SPI_32bit_4wire_write(0x00E81D1C);//
SPI_32bit_4wire_write(0x00E81A1E);//
SPI_32bit_4wire_write(0x00E80109);//
SPI_32bit_4wire_write(0x00E80204);//
SPI_32bit_4wire_write(0x00E87461);//
SPI_32bit_4wire_write(0x00E87275);//
SPI_32bit_4wire_write(0x00E88083);//
SPI_32bit_4wire_write(0x00E8F080);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// **************************************************
// Example of Generic Short Write (2 parameters)
// **************************************************
// Data Type 23h
// Parameter1 xxh
// Parameter2 yyh
SPI_32bit_4wire_write(0x06021023);//bit15:8=Packet Type:Short, bit5:0=Data Type Generic Short Write(2 parameters)
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061001B0);//bit7:0=Parameter1(xx), bit15:8=Parameter2(yy)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(20);
// REM    SPI_WriteData(0xF5);
// REM    SPI_WriteData(0x2F);
// SPI_WriteData(0x2F);
// SPI_WriteData(0x5F);
// SPI_WriteData(0xAB);
// SPI_WriteData(0x98);
// SPI_WriteData(0x52);
// SPI_WriteData(0x0F);
// SPI_WriteData(0x33);
// SPI_WriteData(0x43);
// SPI_WriteData(0x04);
// SPI_WriteData(0x59);
// SPI_WriteData(0x54);
// SPI_WriteData(0x52);
// SPI_WriteData(0x05);
// SPI_WriteData(0x40);
// SPI_WriteData(0x40);
// SPI_WriteData(0x5D);
// SPI_WriteData(0x59);
// SPI_WriteData(0x40);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220014);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E82FF5);//
SPI_32bit_4wire_write(0x00E85F2F);//
SPI_32bit_4wire_write(0x00E898AB);//
SPI_32bit_4wire_write(0x00E80F52);//
SPI_32bit_4wire_write(0x00E84333);//
SPI_32bit_4wire_write(0x00E85904);//
SPI_32bit_4wire_write(0x00E85254);//
SPI_32bit_4wire_write(0x00E84005);//
SPI_32bit_4wire_write(0x00E85D40);//
SPI_32bit_4wire_write(0x00E84059);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(4);
// REM SPI_WriteData(0xBC);
// REM SPI_WriteData(0x01);
// SPI_WriteData(0x4E);
// SPI_WriteData(0x08);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061001BC);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612084E);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM   GP_COMMAD_PA(6);
// REM SPI_WriteData(0xE1);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040006);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061003E1);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06121C10);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x061410A0);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160000);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM  GP_COMMAD_PA(6);
// REM SPI_WriteData(0xFD);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040006);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061016FD);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06121110);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06140920);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160000);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// write 35
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100035);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// write 11
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100011);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(30);
// REM GP_COMMAD_PA(4);
// REM SPI_WriteData(0xC3);
// REM SPI_WriteData(0x40);
// SPI_WriteData(0x00);
// SPI_WriteData(0x28);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061040C3);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122800);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(150);
// display on
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100029);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(200);
// Delay(1);
// **************************************************
// Set to HS mode
// **************************************************
SPI_32bit_4wire_write(0x05000086);//DSI lane setting, DSI mode=HS
SPI_32bit_4wire_write(0x0502A300);//bit set
SPI_32bit_4wire_write(0x05008000);//Switch to DSI mode
SPI_32bit_4wire_write(0x0502C300);//
// **************************************************
// Host: RGB(DPI) input start
// **************************************************
SPI_32bit_4wire_write(0x00080037);//DSI-TX Format setting
SPI_32bit_4wire_write(0x0050003E);//DSI-TX Pixel stream packet Data Type setting
SPI_32bit_4wire_write(0x00320000);//HSYNC Polarity
SPI_32bit_4wire_write(0x00040044);//Configuration Control Register
		
		printk("\n\nS6D7AA0X01_BOE_init leave\n\n");	
}

void S6D7AA0X01_BOE_exit(void)
{
		printk("\n\nS6D7AA0X01_BOE_exit enter\n\n");
	// **************************************************
// SUSPEND Sequence  Applicable only for TC358768AXBG
// By this sequence, lowest power state is realized. DSI output will be HiZ. To resume from this state, "TC358768XBG_INIT_OR_RESUME" is needed. 
// **************************************************
// set display off
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100028);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(40);
// PMIC disable
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061040C3);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122000);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
// Enter LCD sleep mode (How to enter depends on LCD spec. Following setting is a example)
// enter sleep
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100010);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(100);
// RGB Port Disable (from "SetFrmStop to 1" to "Set RstRstPtr to 1")(Only for TC358768AXBG)
SPI_32bit_4wire_write(0x00328000);//Set Frame Stop to 1
LCD_delay_ms(40);
SPI_32bit_4wire_write(0x00040004);//Configuration Control Register Parallel input stop
SPI_32bit_4wire_write(0x0032C000);//Set RstPtr to 1
// Stop DSI continuous clock
SPI_32bit_4wire_write(0x02380000);//
SPI_32bit_4wire_write(0x023A0000);//
// Disable D-PHY  By this setting, DSI line becomes HiZ.  LCD side should permit HiZ input.
SPI_32bit_4wire_write(0x01400001);//
SPI_32bit_4wire_write(0x01420000);//
SPI_32bit_4wire_write(0x01440001);//
SPI_32bit_4wire_write(0x01460000);//
SPI_32bit_4wire_write(0x01480001);//
SPI_32bit_4wire_write(0x014A0000);//
SPI_32bit_4wire_write(0x014C0001);//
SPI_32bit_4wire_write(0x014E0000);//
SPI_32bit_4wire_write(0x01500001);//
SPI_32bit_4wire_write(0x01520000);//
// **************************************************
// Host: Stop RGB input including PCLK.   Stop REFCLK
// **************************************************
// If all the power of TC358768AXBG can be cut off,
// Set all the input signal to TC358768AXBG "L" or "HiZ" including RESX.
// Cut off all the power of TC358768AXBG
// **************************************************

	printk("\n\nS6D7AA0X01_BOE_exit leave\n\n");

	TC358768_spi_csx_set(0);
	TC358768_spi_sck_set(0);
	TC358768_spi_sdi_set(0);
}

void SL008DS21B393_init(void)//sluoling panel SL008DS21B393
{	
		printk("\n\nS6D7AA0X01_BOE_init enter\n\n");
		// **************************************************			   
// First initialization Sequence or RESUME Sequence			   
// **************************************************			   
// **************************************************			   
// Power on TC358768XBG according to recommended power-on sequence, if power is cut off			   
// Assert Reset (RESX="L")			   
// Deassert Reset (RESX="H")			   
// Start input REFCK and PCLK			   
// **************************************************			   
// **************************************************			   
// TC358768XBG Software Reset			   
// **************************************************			   
SPI_32bit_4wire_write(0x00020001);//SYSctl, S/W Reset			   
LCD_delay_ms(1);			   
SPI_32bit_4wire_write(0x00020000);//SYSctl, S/W Reset release			   
// TC358768XBG PLL,Clock Setting			   
// **************************************************			   
SPI_32bit_4wire_write(0x00161068);//PLL Control Register 0 (PLL_PRD,PLL_FBD)
SPI_32bit_4wire_write(0x00180603);//PLL_FRS,PLL_LBWS, PLL oscillation enable
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00180613);//PLL_FRS,PLL_LBWS, PLL clock out enable
// **************************************************			   
// TC358768XBG DPI Input Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x00060064);//FIFO Control Register
// **************************************************			   
// TC358768XBG D-PHY Setting			   
// **************************************************			   
SPI_32bit_4wire_write(0x01400000);//D-PHY Clock lane enable			   
SPI_32bit_4wire_write(0x01420000);//			   
SPI_32bit_4wire_write(0x01440000);//D-PHY Data lane0 enable			   
SPI_32bit_4wire_write(0x01460000);//			   
SPI_32bit_4wire_write(0x01480000);//D-PHY Data lane1 enable			   
SPI_32bit_4wire_write(0x014A0000);//			   
SPI_32bit_4wire_write(0x014C0000);//D-PHY Data lane2 enable			   
SPI_32bit_4wire_write(0x014E0000);//			   
SPI_32bit_4wire_write(0x01500000);//D-PHY Data lane3 enable			   
SPI_32bit_4wire_write(0x01520000);//			   
SPI_32bit_4wire_write(0x01000002);//D-PHY Clock lane control			   
SPI_32bit_4wire_write(0x01020000);//			   
SPI_32bit_4wire_write(0x01040002);//D-PHY Data lane0 control			   
SPI_32bit_4wire_write(0x01060000);//			   
SPI_32bit_4wire_write(0x01080002);//D-PHY Data lane1 control			   
SPI_32bit_4wire_write(0x010A0000);//			   
SPI_32bit_4wire_write(0x010C0002);//D-PHY Data lane2 control			   
SPI_32bit_4wire_write(0x010E0000);//			   
SPI_32bit_4wire_write(0x01100002);//D-PHY Data lane3 control			   
SPI_32bit_4wire_write(0x01120000);//			   
// **************************************************			   
// TC358768XBG DSI-TX PPI Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x02101388);//LINEINITCNT			   
SPI_32bit_4wire_write(0x02120000);//			   
SPI_32bit_4wire_write(0x02140003);//LPTXTIMECNT
SPI_32bit_4wire_write(0x02160000);//			   
SPI_32bit_4wire_write(0x02181203);//TCLK_HEADERCNT
SPI_32bit_4wire_write(0x021A0000);//			   
SPI_32bit_4wire_write(0x02200003);//THS_HEADERCNT			   
SPI_32bit_4wire_write(0x02220000);//			   
SPI_32bit_4wire_write(0x02243A98);//TWAKEUPCNT			   
SPI_32bit_4wire_write(0x02260000);//			   
SPI_32bit_4wire_write(0x022C0001);//THS_TRAILCNT
SPI_32bit_4wire_write(0x022E0000);//			   
SPI_32bit_4wire_write(0x02300005);//HSTXVREGCNT			   
SPI_32bit_4wire_write(0x02320000);//			   
SPI_32bit_4wire_write(0x0234001F);//HSTXVREGEN enable			   
SPI_32bit_4wire_write(0x02360000);//			   
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP			   
SPI_32bit_4wire_write(0x023A0000);//			   
SPI_32bit_4wire_write(0x023C0003);//BTACNTRL1			   
SPI_32bit_4wire_write(0x023E0003);//
SPI_32bit_4wire_write(0x02040001);//STARTCNTRL			   
SPI_32bit_4wire_write(0x02060000);//			   
// **************************************************			   
// TC358768XBG DSI-TX Timing Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x06200001);//Sync Pulse/Sync Event mode setting			   
SPI_32bit_4wire_write(0x06220008);//V Control Register1
SPI_32bit_4wire_write(0x06240003);//V Control Register2
SPI_32bit_4wire_write(0x06260500);//V Control Register3
SPI_32bit_4wire_write(0x06280187);//H Control Register1
SPI_32bit_4wire_write(0x062A0166);//H Control Register2
SPI_32bit_4wire_write(0x062C0960);//H Control Register3
SPI_32bit_4wire_write(0x05180001);//DSI Start
SPI_32bit_4wire_write(0x051A0000);//
// **************************************************
// LCDD (Peripheral) Setting
// **************************************************
LCD_delay_ms(5);
// **************************************************
// Enable Continuous Clock
// **************************************************
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP
SPI_32bit_4wire_write(0x023A0000);//
LCD_delay_ms(150);
// **************************************************
// REM GP_COMMAD_PA(3);
// REM SPI_WriteData(0xF0);
// REM  SPI_WriteData(0x5A);
// REM  SPI_WriteData(0x5A);
// **************************************************
// **************************************************
// Example of Generic Long Write (Up to 8byte parameters)
// **************************************************
// Data Type 29h
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AF0);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// **************************************************
// Example of Generic Long Write (Up to 8byte parameters)
// **************************************************
// Data Type 29h
// SPI_WriteData(0xF1);
// REM     SPI_WriteData(0x5A); 
// REM     SPI_WriteData(0x5A);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AF1);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM SPI_WriteData(0xFC);
// REM SPI_WriteData(0xA5); 
// REM SPI_WriteData(0xA5);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AFC);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM SPI_WriteData(0xD0); 
// REM SPI_WriteData(0x00); 
// REM SPI_WriteData(0x10);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061000D0);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06120010);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(4);
// REM SPI_WriteData(0xC3);
// REM SPI_WriteData(0x40);
// SPI_WriteData(0x00);
// SPI_WriteData(0x28);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061040C3);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122800);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(20);
// REM SPI_WriteData(0x36); 
// REM SPI_WriteData(0x04); 
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040002);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06100436);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06120000);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);

// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(6);
// REM     SPI_WriteData(0xF6);
// REM     SPI_WriteData(0x60);
// SPI_WriteData(0x25);
// SPI_WriteData(0x26);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040007);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061063F6);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06128620);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06140000);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160010);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// write 11
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100011);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(120);
// REM SPI_WriteData(0x36); 
// REM SPI_WriteData(0x00);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040002);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06100036);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06120000);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);

// **************************************************
// REM GP_COMMAD_PA(3);
// REM SPI_WriteData(0xF0);
// REM  SPI_WriteData(0x5A);
// REM  SPI_WriteData(0x5A);
// **************************************************
// **************************************************
// Example of Generic Long Write (Up to 8byte parameters)
// **************************************************
// Data Type 29h
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x0610A5F0);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x061200A5);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// **************************************************
// Example of Generic Long Write (Up to 8byte parameters)
// **************************************************
// Data Type 29h
// SPI_WriteData(0xF1);
// REM     SPI_WriteData(0x5A); 
// REM     SPI_WriteData(0x5A);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AF1);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);

// display on
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100029);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(120);
// Delay(1);
// **************************************************
// Set to HS mode
// **************************************************
SPI_32bit_4wire_write(0x05000086);//DSI lane setting, DSI mode=HS
SPI_32bit_4wire_write(0x0502A300);//bit set
SPI_32bit_4wire_write(0x05008000);//Switch to DSI mode
SPI_32bit_4wire_write(0x0502C300);//
// **************************************************
// Host: RGB(DPI) input start
// **************************************************
SPI_32bit_4wire_write(0x00080037);//DSI-TX Format setting
SPI_32bit_4wire_write(0x0050003E);//DSI-TX Pixel stream packet Data Type setting
SPI_32bit_4wire_write(0x00320000);//HSYNC Polarity
SPI_32bit_4wire_write(0x00040044);//Configuration Control Register
		
		printk("\n\nS6D7AA0X01_BOE_init leave\n\n");	
}

void SL008DS21B393_exit(void)
{
		printk("\n\nS6D7AA0X01_BOE_exit enter\n\n");
	// **************************************************
// SUSPEND Sequence  Applicable only for TC358768AXBG
// By this sequence, lowest power state is realized. DSI output will be HiZ. To resume from this state, "TC358768XBG_INIT_OR_RESUME" is needed. 
// **************************************************
// set display off
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100028);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(40);
// PMIC disable
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061040C3);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122000);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
// Enter LCD sleep mode (How to enter depends on LCD spec. Following setting is a example)
// enter sleep
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100010);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(100);
// RGB Port Disable (from "SetFrmStop to 1" to "Set RstRstPtr to 1")(Only for TC358768AXBG)
SPI_32bit_4wire_write(0x00328000);//Set Frame Stop to 1
LCD_delay_ms(40);
SPI_32bit_4wire_write(0x00040004);//Configuration Control Register Parallel input stop
SPI_32bit_4wire_write(0x0032C000);//Set RstPtr to 1
// Stop DSI continuous clock
SPI_32bit_4wire_write(0x02380000);//
SPI_32bit_4wire_write(0x023A0000);//
// Disable D-PHY  By this setting, DSI line becomes HiZ.  LCD side should permit HiZ input.
SPI_32bit_4wire_write(0x01400001);//
SPI_32bit_4wire_write(0x01420000);//
SPI_32bit_4wire_write(0x01440001);//
SPI_32bit_4wire_write(0x01460000);//
SPI_32bit_4wire_write(0x01480001);//
SPI_32bit_4wire_write(0x014A0000);//
SPI_32bit_4wire_write(0x014C0001);//
SPI_32bit_4wire_write(0x014E0000);//
SPI_32bit_4wire_write(0x01500001);//
SPI_32bit_4wire_write(0x01520000);//
// **************************************************
// Host: Stop RGB input including PCLK.   Stop REFCLK
// **************************************************
// If all the power of TC358768AXBG can be cut off,
// Set all the input signal to TC358768AXBG "L" or "HiZ" including RESX.
// Cut off all the power of TC358768AXBG
// **************************************************

	printk("\n\nS6D7AA0X01_BOE_exit leave\n\n");

	TC358768_spi_csx_set(0);
	TC358768_spi_sck_set(0);
	TC358768_spi_sdi_set(0);
}

void S6D7AA0X04_CPT_init(void)//suoling panel
{	
		printk("\n\nS6D7AA0X04_CPT_init enter\n\n");
		// **************************************************			   
// First initialization Sequence or RESUME Sequence			   
// **************************************************			   
// **************************************************			   
// Power on TC358768XBG according to recommended power-on sequence, if power is cut off			   
// Assert Reset (RESX="L")			   
// Deassert Reset (RESX="H")			   
// Start input REFCK and PCLK			   
// **************************************************			   
// **************************************************			   
// TC358768XBG Software Reset			   
// **************************************************			   
SPI_32bit_4wire_write(0x00020001);//SYSctl, S/W Reset			   
LCD_delay_ms(1);			   
SPI_32bit_4wire_write(0x00020000);//SYSctl, S/W Reset release			   
// TC358768XBG PLL,Clock Setting			   
// **************************************************			   
SPI_32bit_4wire_write(0x00161068);//PLL Control Register 0 (PLL_PRD,PLL_FBD)
SPI_32bit_4wire_write(0x00180603);//PLL_FRS,PLL_LBWS, PLL oscillation enable
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00180613);//PLL_FRS,PLL_LBWS, PLL clock out enable
// **************************************************			   
// TC358768XBG DPI Input Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x00060064);//FIFO Control Register
// **************************************************			   
// TC358768XBG D-PHY Setting			   
// **************************************************			   
SPI_32bit_4wire_write(0x01400000);//D-PHY Clock lane enable			   
SPI_32bit_4wire_write(0x01420000);//			   
SPI_32bit_4wire_write(0x01440000);//D-PHY Data lane0 enable			   
SPI_32bit_4wire_write(0x01460000);//			   
SPI_32bit_4wire_write(0x01480000);//D-PHY Data lane1 enable			   
SPI_32bit_4wire_write(0x014A0000);//			   
SPI_32bit_4wire_write(0x014C0000);//D-PHY Data lane2 enable			   
SPI_32bit_4wire_write(0x014E0000);//			   
SPI_32bit_4wire_write(0x01500000);//D-PHY Data lane3 enable			   
SPI_32bit_4wire_write(0x01520000);//			   
SPI_32bit_4wire_write(0x01000002);//D-PHY Clock lane control			   
SPI_32bit_4wire_write(0x01020000);//			   
SPI_32bit_4wire_write(0x01040002);//D-PHY Data lane0 control			   
SPI_32bit_4wire_write(0x01060000);//			   
SPI_32bit_4wire_write(0x01080002);//D-PHY Data lane1 control			   
SPI_32bit_4wire_write(0x010A0000);//			   
SPI_32bit_4wire_write(0x010C0002);//D-PHY Data lane2 control			   
SPI_32bit_4wire_write(0x010E0000);//			   
SPI_32bit_4wire_write(0x01100002);//D-PHY Data lane3 control			   
SPI_32bit_4wire_write(0x01120000);//			   
// **************************************************			   
// TC358768XBG DSI-TX PPI Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x02101388);//LINEINITCNT			   
SPI_32bit_4wire_write(0x02120000);//			   
SPI_32bit_4wire_write(0x02140003);//LPTXTIMECNT
SPI_32bit_4wire_write(0x02160000);//			   
SPI_32bit_4wire_write(0x02181203);//TCLK_HEADERCNT
SPI_32bit_4wire_write(0x021A0000);//			   
SPI_32bit_4wire_write(0x02200003);//THS_HEADERCNT			   
SPI_32bit_4wire_write(0x02220000);//			   
SPI_32bit_4wire_write(0x02243A98);//TWAKEUPCNT			   
SPI_32bit_4wire_write(0x02260000);//			   
SPI_32bit_4wire_write(0x022C0001);//THS_TRAILCNT
SPI_32bit_4wire_write(0x022E0000);//			   
SPI_32bit_4wire_write(0x02300005);//HSTXVREGCNT			   
SPI_32bit_4wire_write(0x02320000);//			   
SPI_32bit_4wire_write(0x0234001F);//HSTXVREGEN enable			   
SPI_32bit_4wire_write(0x02360000);//			   
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP			   
SPI_32bit_4wire_write(0x023A0000);//			   
SPI_32bit_4wire_write(0x023C0003);//BTACNTRL1			   
SPI_32bit_4wire_write(0x023E0003);//
SPI_32bit_4wire_write(0x02040001);//STARTCNTRL			   
SPI_32bit_4wire_write(0x02060000);//			   
// **************************************************			   
// TC358768XBG DSI-TX Timing Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x06200001);//Sync Pulse/Sync Event mode setting			   
SPI_32bit_4wire_write(0x0622000C);//V Control Register1  8,OLD
SPI_32bit_4wire_write(0x06240008);//V Control Register2  3,OLD
SPI_32bit_4wire_write(0x06260500);//V Control Register3
SPI_32bit_4wire_write(0x06280187);//H Control Register1
SPI_32bit_4wire_write(0x062A0166);//H Control Register2
SPI_32bit_4wire_write(0x062C0960);//H Control Register3
SPI_32bit_4wire_write(0x05180001);//DSI Start
SPI_32bit_4wire_write(0x051A0000);//
// **************************************************
// LCDD (Peripheral) Setting
// **************************************************
LCD_delay_ms(5);
// **************************************************
// Enable Continuous Clock
// **************************************************
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP
SPI_32bit_4wire_write(0x023A0000);//
LCD_delay_ms(150);
// **************************************************
// REM GP_COMMAD_PA(3);
// REM SPI_WriteData(0xF0);
// REM  SPI_WriteData(0x5A);
// REM  SPI_WriteData(0x5A);
// **************************************************
// **************************************************
// Example of Generic Long Write (Up to 8byte parameters)
// **************************************************
// Data Type 29h
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AF0);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// **************************************************
// Example of Generic Long Write (Up to 8byte parameters)
// **************************************************
// Data Type 29h
// SPI_WriteData(0xF1);
// REM     SPI_WriteData(0x5A); 
// REM     SPI_WriteData(0x5A);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AF1);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM SPI_WriteData(0xFC);
// REM SPI_WriteData(0xA5); 
// REM SPI_WriteData(0xA5);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x0610A5FC);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x061200A5);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM SPI_WriteData(0xD0); 
// REM SPI_WriteData(0x00); 
// REM SPI_WriteData(0x10);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061000D0);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06120010);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// **************************************************
// Example of Generic Short Write (2 parameters)
// **************************************************
// Data Type 23h
// Parameter1 xxh
// Parameter2 yyh
// REM     SPI_WriteData(0xB1);
// REM     SPI_WriteData(0x10);
SPI_32bit_4wire_write(0x06021023);//bit15:8=Packet Type:Short, bit5:0=Data Type Generic Short Write(2 parameters)
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061010B1);//bit7:0=Parameter1(xx), bit15:8=Parameter2(yy)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(5);
// REM  SPI_WriteData(0xB2);
// REM  SPI_WriteData(0x14);
// REM  SPI_WriteData(0x22);
// REM  SPI_WriteData(0x2F);
// REM  SPI_WriteData(0x04);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040005);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061014B2);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122F22);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06140004);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160000);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM SPI_WriteData(0xB5);
// REM SPI_WriteData(0x01);
// **************************************************
// Example of Generic Short Write (2 parameters)
// **************************************************
// Data Type 23h
// Parameter1 xxh
// Parameter2 yyh
SPI_32bit_4wire_write(0x06021023);//bit15:8=Packet Type:Short, bit5:0=Data Type Generic Short Write(2 parameters)
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061001B5);//bit7:0=Parameter1(xx), bit15:8=Parameter2(yy)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM     GP_COMMAD_PA(9);
// REM     SPI_WriteData(0xEE);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
// SPI_WriteData(0x1F);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
// SPI_WriteData(0x1F);
// SPI_WriteData(0x00);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220009);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E800EE);//
SPI_32bit_4wire_write(0x00E81F00);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E81F00);//
SPI_32bit_4wire_write(0x00E80000);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM  GP_COMMAD_PA(14);
// REM    SPI_WriteData(0xEF);
// REM     SPI_WriteData(0x56);
// SPI_WriteData(0x34);
// SPI_WriteData(0x43);
// SPI_WriteData(0x65);
// SPI_WriteData(0x90);
// SPI_WriteData(0x80);
// SPI_WriteData(0x24);
// SPI_WriteData(0x80);
// SPI_WriteData(0x00);
// SPI_WriteData(0x91);
// SPI_WriteData(0x11);
// SPI_WriteData(0x11);
// SPI_WriteData(0x11);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x0022000E);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E856EF);//
SPI_32bit_4wire_write(0x00E84334);//
SPI_32bit_4wire_write(0x00E89065);//
SPI_32bit_4wire_write(0x00E82480);//
SPI_32bit_4wire_write(0x00E80080);//
SPI_32bit_4wire_write(0x00E81191);//
SPI_32bit_4wire_write(0x00E81111);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM    GP_COMMAD_PA(33);
// REM    SPI_WriteData(0xF7);
// REM    SPI_WriteData(0x04);
// SPI_WriteData(0x08);
// SPI_WriteData(0x09);
// SPI_WriteData(0x0A);
// SPI_WriteData(0x0B);
// SPI_WriteData(0x0C);
// SPI_WriteData(0x0D);
// SPI_WriteData(0x0E);
// SPI_WriteData(0x0F);
// SPI_WriteData(0x16);
// SPI_WriteData(0x17);
// SPI_WriteData(0x10);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// SPI_WriteData(0x04);
// SPI_WriteData(0x08);
// SPI_WriteData(0x09);
// SPI_WriteData(0x0A);
// SPI_WriteData(0x0B);
// SPI_WriteData(0x0C);
// SPI_WriteData(0x0D);
// SPI_WriteData(0x0E);
// SPI_WriteData(0x0F);
// SPI_WriteData(0x16);
// SPI_WriteData(0x17);
// SPI_WriteData(0x10);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// SPI_WriteData(0x01);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220021);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E804F7);//
SPI_32bit_4wire_write(0x00E80908);//
SPI_32bit_4wire_write(0x00E80B0A);//
SPI_32bit_4wire_write(0x00E80D0C);//
SPI_32bit_4wire_write(0x00E80F0E);//
SPI_32bit_4wire_write(0x00E81716);//
SPI_32bit_4wire_write(0x00E80110);//
SPI_32bit_4wire_write(0x00E80101);//
SPI_32bit_4wire_write(0x00E80401);//
SPI_32bit_4wire_write(0x00E80908);//
SPI_32bit_4wire_write(0x00E80B0A);//
SPI_32bit_4wire_write(0x00E80D0C);//
SPI_32bit_4wire_write(0x00E80F0E);//
SPI_32bit_4wire_write(0x00E81716);//
SPI_32bit_4wire_write(0x00E80110);//
SPI_32bit_4wire_write(0x00E80101);//
SPI_32bit_4wire_write(0x00E80001);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM     GP_COMMAD_PA(6);
// REM     SPI_WriteData(0xF2);
// REM     SPI_WriteData(0x02);
// SPI_WriteData(0x08);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040006);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061002F2);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06120808);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06141040);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160000);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(6);
// REM     SPI_WriteData(0xF6);
// REM     SPI_WriteData(0x60);
// SPI_WriteData(0x25);
// SPI_WriteData(0x26);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
// SPI_WriteData(0x00);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040006);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061060F6);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122625);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06140000);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160000);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(18);
// REM     SPI_WriteData(0xFA);
// REM     SPI_WriteData(0x04);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// Parameter1 xxh
// Parameter2 ggh
// Parameter3 hhh
// Parameter4 iih
// Parameter5 jjh
// Parameter6 kkh
// Parameter7 llh
// Parameter8 mmh
// |
// ParameterN nnh
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220012);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E804FA);//
SPI_32bit_4wire_write(0x00E80735);//
SPI_32bit_4wire_write(0x00E8120B);//
SPI_32bit_4wire_write(0x00E8100B);//
SPI_32bit_4wire_write(0x00E81A16);//
SPI_32bit_4wire_write(0x00E82C24);//
SPI_32bit_4wire_write(0x00E83B33);//
SPI_32bit_4wire_write(0x00E8333B);//
SPI_32bit_4wire_write(0x00E83334);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(18);
// REM    SPI_WriteData(0xFB);
// REM    SPI_WriteData(0x04);
// SPI_WriteData(0x35);
// SPI_WriteData(0x07);
// SPI_WriteData(0x0B);
// SPI_WriteData(0x12);
// SPI_WriteData(0x0B);
// SPI_WriteData(0x10);
// SPI_WriteData(0x16);
// SPI_WriteData(0x1A);
// SPI_WriteData(0x24);
// SPI_WriteData(0x2C);
// SPI_WriteData(0x33);
// SPI_WriteData(0x3B);
// SPI_WriteData(0x3B);
// SPI_WriteData(0x33);
// SPI_WriteData(0x34);
// SPI_WriteData(0x33);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// Parameter1 xxh
// Parameter2 ggh
// Parameter3 hhh
// Parameter4 iih
// Parameter5 jjh
// Parameter6 kkh
// Parameter7 llh
// Parameter8 mmh
// |
// ParameterN nnh
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220012);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E804FB);//
SPI_32bit_4wire_write(0x00E80735);//
SPI_32bit_4wire_write(0x00E8120B);//
SPI_32bit_4wire_write(0x00E8100B);//
SPI_32bit_4wire_write(0x00E81A16);//
SPI_32bit_4wire_write(0x00E82C24);//
SPI_32bit_4wire_write(0x00E83B33);//
SPI_32bit_4wire_write(0x00E8333B);//
SPI_32bit_4wire_write(0x00E83334);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(11);
// REM    SPI_WriteData(0xF3);
// REM    SPI_WriteData(0x01);
// SPI_WriteData(0xC4);
// SPI_WriteData(0xE0);
// SPI_WriteData(0x62);
// SPI_WriteData(0xD4);
// SPI_WriteData(0x83);
// SPI_WriteData(0x37);
// SPI_WriteData(0x3C);
// SPI_WriteData(0x24);
// SPI_WriteData(0x00);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x0022000B);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E801F3);//
SPI_32bit_4wire_write(0x00E8E0C4);//
SPI_32bit_4wire_write(0x00E8D462);//
SPI_32bit_4wire_write(0x00E83783);//
SPI_32bit_4wire_write(0x00E8243C);//
SPI_32bit_4wire_write(0x00E80000);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(36);
// REM    SPI_WriteData(0xF4);
// REM    SPI_WriteData(0x00);
// SPI_WriteData(0x02);
// SPI_WriteData(0x03);
// SPI_WriteData(0x26);
// SPI_WriteData(0x03);
// SPI_WriteData(0x02);
// SPI_WriteData(0x09);
// SPI_WriteData(0x00);
// SPI_WriteData(0x07);
// SPI_WriteData(0x16);
// SPI_WriteData(0x16);
// SPI_WriteData(0x03);
// SPI_WriteData(0x00);
// SPI_WriteData(0x08);
// SPI_WriteData(0x08);
// SPI_WriteData(0x03);
// SPI_WriteData(0x19);
// SPI_WriteData(0x1C);
// SPI_WriteData(0x12);
// SPI_WriteData(0x1C);
// SPI_WriteData(0x1D);
// SPI_WriteData(0x1E);
// SPI_WriteData(0x1A);
// SPI_WriteData(0x09);
// SPI_WriteData(0x01);
// SPI_WriteData(0x04);
// SPI_WriteData(0x02);
// SPI_WriteData(0x61);
// SPI_WriteData(0x74);
// SPI_WriteData(0x75);
// SPI_WriteData(0x72);
// SPI_WriteData(0x83);
// SPI_WriteData(0x80);
// SPI_WriteData(0x80);
// SPI_WriteData(0xF0);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220024);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E800F4);//
SPI_32bit_4wire_write(0x00E80302);//
SPI_32bit_4wire_write(0x00E80326);//
SPI_32bit_4wire_write(0x00E80902);//
SPI_32bit_4wire_write(0x00E80700);//
SPI_32bit_4wire_write(0x00E81616);//
SPI_32bit_4wire_write(0x00E80003);//
SPI_32bit_4wire_write(0x00E80808);//
SPI_32bit_4wire_write(0x00E81903);//
SPI_32bit_4wire_write(0x00E8121C);//
SPI_32bit_4wire_write(0x00E81D1C);//
SPI_32bit_4wire_write(0x00E81A1E);//
SPI_32bit_4wire_write(0x00E80109);//
SPI_32bit_4wire_write(0x00E80204);//
SPI_32bit_4wire_write(0x00E87461);//
SPI_32bit_4wire_write(0x00E87275);//
SPI_32bit_4wire_write(0x00E88083);//
SPI_32bit_4wire_write(0x00E8F080);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// **************************************************
// Example of Generic Short Write (2 parameters)
// **************************************************
// Data Type 23h
// Parameter1 xxh
// Parameter2 yyh
SPI_32bit_4wire_write(0x06021023);//bit15:8=Packet Type:Short, bit5:0=Data Type Generic Short Write(2 parameters)
SPI_32bit_4wire_write(0x06040000);//
SPI_32bit_4wire_write(0x061001B0);//bit7:0=Parameter1(xx), bit15:8=Parameter2(yy)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(20);
// REM    SPI_WriteData(0xF5);
// REM    SPI_WriteData(0x2F);
// SPI_WriteData(0x2F);
// SPI_WriteData(0x5F);
// SPI_WriteData(0xAB);
// SPI_WriteData(0x98);
// SPI_WriteData(0x52);
// SPI_WriteData(0x0F);
// SPI_WriteData(0x33);
// SPI_WriteData(0x43);
// SPI_WriteData(0x04);
// SPI_WriteData(0x59);
// SPI_WriteData(0x54);
// SPI_WriteData(0x52);
// SPI_WriteData(0x05);
// SPI_WriteData(0x40);
// SPI_WriteData(0x40);
// SPI_WriteData(0x5D);
// SPI_WriteData(0x59);
// SPI_WriteData(0x40);
// **************************************************
// Example of Generic Long Write (more than 8byte) Nbyte(Less than 1024) example.  Only for TC358768AXBG!
// **************************************************
// This setting should be put in yellow colored part of "Source" sequence
// Transmission Byte count = ttt
// Data Type 29h
// ----------
// Configuration of command transmission
// ----------
SPI_32bit_4wire_write(0x00080001);//
SPI_32bit_4wire_write(0x00500029);//Data ID setting
SPI_32bit_4wire_write(0x00220014);//Transmission byte count=ttt
SPI_32bit_4wire_write(0x00E08000);//Enable I2C/SPI write to VB
// ----------
// Data contents
// ----------
SPI_32bit_4wire_write(0x00E82FF5);//
SPI_32bit_4wire_write(0x00E85F2F);//
SPI_32bit_4wire_write(0x00E898AB);//
SPI_32bit_4wire_write(0x00E80F52);//
SPI_32bit_4wire_write(0x00E84333);//
SPI_32bit_4wire_write(0x00E85904);//
SPI_32bit_4wire_write(0x00E85254);//
SPI_32bit_4wire_write(0x00E84005);//
SPI_32bit_4wire_write(0x00E85D40);//
SPI_32bit_4wire_write(0x00E84059);//
// ----------
// Command transmission
// ----------
SPI_32bit_4wire_write(0x00E0E000);//Start command transmisison
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00E02000);//Keep Mask High to prevent short packets send out
SPI_32bit_4wire_write(0x00E00000);//Stop DSI-TX command transfer
LCD_delay_ms(1);
// REM GP_COMMAD_PA(4);
// REM SPI_WriteData(0xBC);
// REM SPI_WriteData(0x01);
// SPI_WriteData(0x4E);
// SPI_WriteData(0x08);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061001BC);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612084E);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM   GP_COMMAD_PA(6);
// REM SPI_WriteData(0xE1);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040006);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061003E1);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06121C10);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x061410A0);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160000);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// REM  GP_COMMAD_PA(6);
// REM SPI_WriteData(0xFD);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040006);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061016FD);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06121110);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06140920);//bit7:0=Parameter5(kk), bit15:8=Parameter6(ll)
SPI_32bit_4wire_write(0x06160000);//bit7:0=Parameter7(mm), bit15:8=Parameter8(nn)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// write 35
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100035);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(1);
// write 11
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100011);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(30);
// REM GP_COMMAD_PA(4);
// REM SPI_WriteData(0xC3);
// REM SPI_WriteData(0x40);
// SPI_WriteData(0x00);
// SPI_WriteData(0x28);
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061040C3);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122800);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(150);
// display on
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100029);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(200);
// Delay(1);
// **************************************************
// Set to HS mode
// **************************************************
SPI_32bit_4wire_write(0x05000086);//DSI lane setting, DSI mode=HS
SPI_32bit_4wire_write(0x0502A300);//bit set
SPI_32bit_4wire_write(0x05008000);//Switch to DSI mode
SPI_32bit_4wire_write(0x0502C300);//
// **************************************************
// Host: RGB(DPI) input start
// **************************************************
SPI_32bit_4wire_write(0x00080037);//DSI-TX Format setting
SPI_32bit_4wire_write(0x0050003E);//DSI-TX Pixel stream packet Data Type setting
SPI_32bit_4wire_write(0x00320000);//HSYNC Polarity
SPI_32bit_4wire_write(0x00040044);//Configuration Control Register
		
		printk("\n\nS6D7AA0X04_CPT_init leave\n\n");	
}

void S6D7AA0X04_CPT_exit(void)
{
	printk("\n\nS6D7AA0X04_CPT_exit enter\n\n");
	// **************************************************
// SUSPEND Sequence  Applicable only for TC358768AXBG
// By this sequence, lowest power state is realized. DSI output will be HiZ. To resume from this state, "TC358768XBG_INIT_OR_RESUME" is needed. 
// **************************************************
// set display off
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100028);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(40);
// PMIC disable
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061040C3);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122000);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
// Enter LCD sleep mode (How to enter depends on LCD spec. Following setting is a example)
// enter sleep
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100010);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(100);
// RGB Port Disable (from "SetFrmStop to 1" to "Set RstRstPtr to 1")(Only for TC358768AXBG)
SPI_32bit_4wire_write(0x00328000);//Set Frame Stop to 1
LCD_delay_ms(40);
SPI_32bit_4wire_write(0x00040004);//Configuration Control Register Parallel input stop
SPI_32bit_4wire_write(0x0032C000);//Set RstPtr to 1
// Stop DSI continuous clock
SPI_32bit_4wire_write(0x02380000);//
SPI_32bit_4wire_write(0x023A0000);//
// Disable D-PHY  By this setting, DSI line becomes HiZ.  LCD side should permit HiZ input.
SPI_32bit_4wire_write(0x01400001);//
SPI_32bit_4wire_write(0x01420000);//
SPI_32bit_4wire_write(0x01440001);//
SPI_32bit_4wire_write(0x01460000);//
SPI_32bit_4wire_write(0x01480001);//
SPI_32bit_4wire_write(0x014A0000);//
SPI_32bit_4wire_write(0x014C0001);//
SPI_32bit_4wire_write(0x014E0000);//
SPI_32bit_4wire_write(0x01500001);//
SPI_32bit_4wire_write(0x01520000);//
// **************************************************
// Host: Stop RGB input including PCLK.   Stop REFCLK
// **************************************************
// If all the power of TC358768AXBG can be cut off,
// Set all the input signal to TC358768AXBG "L" or "HiZ" including RESX.
// Cut off all the power of TC358768AXBG
// **************************************************

	printk("\n\nS6D7AA0X04_CPT_exit leave\n\n");

	TC358768_spi_csx_set(0);
	TC358768_spi_sck_set(0);
	TC358768_spi_sdi_set(0);
}

void b079xan01_init(void)
{	
	printk("\n\nRocky b079xan01_init enter\n\n");
		// **************************************************			   
// First initialization Sequence or RESUME Sequence			   
// **************************************************			   
// **************************************************			   
// Power on TC358768XBG according to recommended power-on sequence, if power is cut off			   
// Assert Reset (RESX="L")			   
// Deassert Reset (RESX="H")			   
// Start input REFCK and PCLK			   
// **************************************************			   
// **************************************************			   
// TC358768XBG Software Reset			   
// **************************************************			   
SPI_32bit_4wire_write(0x00020001);//SYSctl, S/W Reset			   
LCD_delay_ms(1);			   
SPI_32bit_4wire_write(0x00020000);//SYSctl, S/W Reset release			   
// TC358768XBG PLL,Clock Setting			   
// **************************************************			   
SPI_32bit_4wire_write(0x00161040);//PLL Control Register 0 (PLL_PRD,PLL_FBD)			   
SPI_32bit_4wire_write(0x00180203);//PLL_FRS,PLL_LBWS, PLL oscillation enable			   
LCD_delay_ms(1);			   
SPI_32bit_4wire_write(0x00180213);//PLL_FRS,PLL_LBWS, PLL clock out enable			   
// **************************************************			   
// TC358768XBG DPI Input Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x000600C8);//FIFO Control Register			   
// **************************************************			   
// TC358768XBG D-PHY Setting			   
// **************************************************			   
SPI_32bit_4wire_write(0x01400000);//D-PHY Clock lane enable			   
SPI_32bit_4wire_write(0x01420000);//			   
SPI_32bit_4wire_write(0x01440000);//D-PHY Data lane0 enable			   
SPI_32bit_4wire_write(0x01460000);//			   
SPI_32bit_4wire_write(0x01480000);//D-PHY Data lane1 enable			   
SPI_32bit_4wire_write(0x014A0000);//			   
SPI_32bit_4wire_write(0x014C0000);//D-PHY Data lane2 enable			   
SPI_32bit_4wire_write(0x014E0000);//			   
SPI_32bit_4wire_write(0x01500000);//D-PHY Data lane3 enable			   
SPI_32bit_4wire_write(0x01520000);//			   
SPI_32bit_4wire_write(0x01000002);//D-PHY Clock lane control			   
SPI_32bit_4wire_write(0x01020000);//			   
SPI_32bit_4wire_write(0x01040002);//D-PHY Data lane0 control			   
SPI_32bit_4wire_write(0x01060000);//			   
SPI_32bit_4wire_write(0x01080002);//D-PHY Data lane1 control			   
SPI_32bit_4wire_write(0x010A0000);//			   
SPI_32bit_4wire_write(0x010C0002);//D-PHY Data lane2 control			   
SPI_32bit_4wire_write(0x010E0000);//			   
SPI_32bit_4wire_write(0x01100002);//D-PHY Data lane3 control			   
SPI_32bit_4wire_write(0x01120000);//			   
// **************************************************			   
// TC358768XBG DSI-TX PPI Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x02101388);//LINEINITCNT			   
SPI_32bit_4wire_write(0x02120000);//			   
SPI_32bit_4wire_write(0x02140004);//LPTXTIMECNT			   
SPI_32bit_4wire_write(0x02160000);//			   
SPI_32bit_4wire_write(0x02181202);//TCLK_HEADERCNT			   
SPI_32bit_4wire_write(0x021A0000);//			   
SPI_32bit_4wire_write(0x02200003);//THS_HEADERCNT			   
SPI_32bit_4wire_write(0x02220000);//			   
SPI_32bit_4wire_write(0x02243A98);//TWAKEUPCNT			   
SPI_32bit_4wire_write(0x02260000);//			   
SPI_32bit_4wire_write(0x022C0002);//THS_TRAILCNT			   
SPI_32bit_4wire_write(0x022E0000);//			   
SPI_32bit_4wire_write(0x02300005);//HSTXVREGCNT			   
SPI_32bit_4wire_write(0x02320000);//			   
SPI_32bit_4wire_write(0x0234001F);//HSTXVREGEN enable			   
SPI_32bit_4wire_write(0x02360000);//			   
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP			   
SPI_32bit_4wire_write(0x023A0000);//			   
SPI_32bit_4wire_write(0x023C0003);//BTACNTRL1			   
SPI_32bit_4wire_write(0x023E0004);//			   
SPI_32bit_4wire_write(0x02040001);//STARTCNTRL			   
SPI_32bit_4wire_write(0x02060000);//			   
// **************************************************			   
// TC358768XBG DSI-TX Timing Control			   
// **************************************************			   
SPI_32bit_4wire_write(0x06200001);//Sync Pulse/Sync Event mode setting			   
SPI_32bit_4wire_write(0x0622005A);//V Control Register1			   
SPI_32bit_4wire_write(0x06240028);//V Control Register2			   
SPI_32bit_4wire_write(0x06260400);//V Control Register3			   
SPI_32bit_4wire_write(0x062801E8);//H Control Register1			   
SPI_32bit_4wire_write(0x062A00E0);//H Control Register2			   
SPI_32bit_4wire_write(0x062C0900);//H Control Register3			   
SPI_32bit_4wire_write(0x05180001);//DSI Start			   
SPI_32bit_4wire_write(0x051A0000);//				   
// **************************************************			   
// LCDD (Peripheral) Setting			   
// **************************************************			   
LCD_delay_ms(5);			   
// exit_sleep			   
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)			   
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)			   
SPI_32bit_4wire_write(0x06100011);//			   
SPI_32bit_4wire_write(0x06000001);//Packet Transfer			   
LCD_delay_ms(1);			   
// **************************************************			   
// Enable Continuous Clock			   
// **************************************************			   
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP			   
SPI_32bit_4wire_write(0x023A0000);//			   
LCD_delay_ms(150);			   
// **************************************************			   
// LCDD (Peripheral) Setting			   
// **************************************************			   
// set display on			   
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)			   
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)			   
SPI_32bit_4wire_write(0x06100029);//			   
SPI_32bit_4wire_write(0x06000001);//Packet Transfer			   
LCD_delay_ms(1);			   
// **************************************************			   
// Set to HS mode			   
// **************************************************			   
SPI_32bit_4wire_write(0x05000086);//DSI lane setting, DSI mode=HS			   
SPI_32bit_4wire_write(0x0502A300);//bit set			   
SPI_32bit_4wire_write(0x05008000);//Switch to DSI mode			   
SPI_32bit_4wire_write(0x0502C300);//			   
// **************************************************			   
// Host: RGB(DPI) input start			   
// **************************************************			   
SPI_32bit_4wire_write(0x00080037);//DSI-TX Format setting			   
SPI_32bit_4wire_write(0x0050003E);//DSI-TX Pixel stream packet Data Type setting			   
SPI_32bit_4wire_write(0x00320000);//HSYNC Polarity			   
SPI_32bit_4wire_write(0x00040044);//Configuration Control Register			  
		
		printk("\n\nRocky b079xan01_init leave\n\n");	
}

void b079xan01_exit(void)
{
	printk("\n\nRocky b079xan01_exit enter\n\n");
	// **************************************************
// SUSPEND Sequence  Applicable only for TC358768AXBG
// By this sequence, lowest power state is realized. DSI output will be HiZ. To resume from this state, "TC358768XBG_INIT_OR_RESUME" is needed. 
// **************************************************
// set display off
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100028);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(40);
// Enter LCD sleep mode (How to enter depends on LCD spec. Following setting is a example)
// enter sleep
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100010);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(100);
// RGB Port Disable (from "SetFrmStop to 1" to "Set RstRstPtr to 1")(Only for TC358768AXBG)
SPI_32bit_4wire_write(0x00328000);//Set Frame Stop to 1
LCD_delay_ms(40);
SPI_32bit_4wire_write(0x00040004);//Configuration Control Register Parallel input stop
SPI_32bit_4wire_write(0x0032C000);//Set RstPtr to 1
// Stop DSI continuous clock
SPI_32bit_4wire_write(0x02380000);//
SPI_32bit_4wire_write(0x023A0000);//
// Disable D-PHY  By this setting, DSI line becomes HiZ.  LCD side should permit HiZ input.
SPI_32bit_4wire_write(0x01400001);//
SPI_32bit_4wire_write(0x01420000);//
SPI_32bit_4wire_write(0x01440001);//
SPI_32bit_4wire_write(0x01460000);//
SPI_32bit_4wire_write(0x01480001);//
SPI_32bit_4wire_write(0x014A0000);//
SPI_32bit_4wire_write(0x014C0001);//
SPI_32bit_4wire_write(0x014E0000);//
SPI_32bit_4wire_write(0x01500001);//
SPI_32bit_4wire_write(0x01520000);//
// **************************************************
// Host: Stop RGB input including PCLK.   Stop REFCLK
// **************************************************
// If all the power of TC358768AXBG can be cut off,
// Set all the input signal to TC358768AXBG "L" or "HiZ" including RESX.
// Cut off all the power of TC358768AXBG
// **************************************************

	printk("\n\nRocky b079xan01_exit leave\n\n");

	TC358768_spi_csx_set(0);
	TC358768_spi_sck_set(0);
	TC358768_spi_sdi_set(0);
}

void KD080D10_init(void)
{	
	printk("\n\nRocky KD080D10_init enter\n\n");
		// **************************************************			   
// **************************************************
// First initialization Sequence or RESUME Sequence
// **************************************************
// **************************************************
// Power on TC358768XBG according to recommended power-on sequence, if power is cut off
// Assert Reset (RESX="L")
// Deassert Reset (RESX="H")
// Start input REFCK and PCLK
// **************************************************
// **************************************************
// TC358768XBG Software Reset
// **************************************************
SPI_32bit_4wire_write(0x00020001);//SYSctl, S/W Reset
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00020000);//SYSctl, S/W Reset release
// TC358768XBG PLL,Clock Setting
// **************************************************
SPI_32bit_4wire_write(0x00161068);//PLL Control Register 0 (PLL_PRD,PLL_FBD)
SPI_32bit_4wire_write(0x00180603);//PLL_FRS,PLL_LBWS, PLL oscillation enable
LCD_delay_ms(1);
SPI_32bit_4wire_write(0x00180613);//PLL_FRS,PLL_LBWS, PLL clock out enable
// **************************************************
// TC358768XBG DPI Input Control
// **************************************************
SPI_32bit_4wire_write(0x00060064);//FIFO Control Register
// **************************************************
// TC358768XBG D-PHY Setting
// **************************************************
SPI_32bit_4wire_write(0x01400000);//D-PHY Clock lane enable
SPI_32bit_4wire_write(0x01420000);//
SPI_32bit_4wire_write(0x01440000);//D-PHY Data lane0 enable
SPI_32bit_4wire_write(0x01460000);//
SPI_32bit_4wire_write(0x01480000);//D-PHY Data lane1 enable
SPI_32bit_4wire_write(0x014A0000);//
SPI_32bit_4wire_write(0x014C0000);//D-PHY Data lane2 enable
SPI_32bit_4wire_write(0x014E0000);//
SPI_32bit_4wire_write(0x01500000);//D-PHY Data lane3 enable
SPI_32bit_4wire_write(0x01520000);//
SPI_32bit_4wire_write(0x01000002);//D-PHY Clock lane control
SPI_32bit_4wire_write(0x01020000);//
SPI_32bit_4wire_write(0x01040002);//D-PHY Data lane0 control
SPI_32bit_4wire_write(0x01060000);//
SPI_32bit_4wire_write(0x01080002);//D-PHY Data lane1 control
SPI_32bit_4wire_write(0x010A0000);//
SPI_32bit_4wire_write(0x010C0002);//D-PHY Data lane2 control
SPI_32bit_4wire_write(0x010E0000);//
SPI_32bit_4wire_write(0x01100002);//D-PHY Data lane3 control
SPI_32bit_4wire_write(0x01120000);//
// **************************************************
// TC358768XBG DSI-TX PPI Control
// **************************************************
SPI_32bit_4wire_write(0x02101388);//LINEINITCNT			   
SPI_32bit_4wire_write(0x02120000);//			   
SPI_32bit_4wire_write(0x02140003);//LPTXTIMECNT
SPI_32bit_4wire_write(0x02160000);//			   
SPI_32bit_4wire_write(0x02181203);//TCLK_HEADERCNT
SPI_32bit_4wire_write(0x021A0000);//			   
SPI_32bit_4wire_write(0x02200003);//THS_HEADERCNT			   
SPI_32bit_4wire_write(0x02220000);//			   
SPI_32bit_4wire_write(0x02243A98);//TWAKEUPCNT			   
SPI_32bit_4wire_write(0x02260000);//			   
SPI_32bit_4wire_write(0x022C0001);//THS_TRAILCNT
SPI_32bit_4wire_write(0x022E0000);//			   
SPI_32bit_4wire_write(0x02300005);//HSTXVREGCNT			   
SPI_32bit_4wire_write(0x02320000);//			   
SPI_32bit_4wire_write(0x0234001F);//HSTXVREGEN enable			   
SPI_32bit_4wire_write(0x02360000);//			   
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP			   
SPI_32bit_4wire_write(0x023A0000);//			   
SPI_32bit_4wire_write(0x023C0003);//BTACNTRL1			   
SPI_32bit_4wire_write(0x023E0003);//
SPI_32bit_4wire_write(0x02040001);//STARTCNTRL
SPI_32bit_4wire_write(0x02060000);//
// **************************************************
// TC358768XBG DSI-TX Timing Control
// **************************************************
SPI_32bit_4wire_write(0x06200001);//Sync Pulse/Sync Event mode setting
SPI_32bit_4wire_write(0x0622000C);//V Control Register1
SPI_32bit_4wire_write(0x06240008);//V Control Register2
SPI_32bit_4wire_write(0x06260500);//V Control Register3
SPI_32bit_4wire_write(0x06280187);//H Control Register1
SPI_32bit_4wire_write(0x062A0166);//H Control Register2
SPI_32bit_4wire_write(0x062C0960);//H Control Register3
SPI_32bit_4wire_write(0x05180001);//DSI Start
SPI_32bit_4wire_write(0x051A0000);//
// **************************************************
// LCDD (Peripheral) Setting
// **************************************************
LCD_delay_ms(5);
// **************************************************
// Enable Continuous Clock
// **************************************************
SPI_32bit_4wire_write(0x02380001);//DSI clock Enable/Disable during LP
SPI_32bit_4wire_write(0x023A0000);//
LCD_delay_ms(150);
// REM SPI_WriteData(0xF0);
// REM  SPI_WriteData(0x5A);
// REM  SPI_WriteData(0x5A);
// **************************************************
// **************************************************
// Example of Generic Long Write (Up to 8byte parameters)
// **************************************************
// Data Type 29h
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040003);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x06105AF0);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x0612005A);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
// **************************************************			   
// LCDD (Peripheral) Setting			   
// **************************************************			   
LCD_delay_ms(5);			   
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100011);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(10);
// REM GP_COMMAD_PA(4);
// REM SPI_WriteData(0xC3);
// REM SPI_WriteData(0x40);
// SPI_WriteData(0x00);
// SPI_WriteData(0x28);

SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061040C3);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122800);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(10);
// display on
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100029);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(200);			   
// **************************************************			   
// Set to HS mode			   
// **************************************************			   
SPI_32bit_4wire_write(0x05000086);//DSI lane setting, DSI mode=HS			   
SPI_32bit_4wire_write(0x0502A300);//bit set			   
SPI_32bit_4wire_write(0x05008000);//Switch to DSI mode			   
SPI_32bit_4wire_write(0x0502C300);//			   
// **************************************************			   
// Host: RGB(DPI) input start			   
// **************************************************			   
SPI_32bit_4wire_write(0x00080037);//DSI-TX Format setting			   
SPI_32bit_4wire_write(0x0050003E);//DSI-TX Pixel stream packet Data Type setting			   
SPI_32bit_4wire_write(0x00320000);//HSYNC Polarity			   
SPI_32bit_4wire_write(0x00040044);//Configuration Control Register			  
		
		printk("\n\nRocky b079xan01_init leave\n\n");	
}

void KD080D10_exit(void)
{
	printk("\n\nRocky KD080D10_exit enter\n\n");
	// **************************************************

// **************************************************
// SUSPEND Sequence  Applicable only for TC358768AXBG
// By this sequence, lowest power state is realized. DSI output will be HiZ. To resume from this state, "TC358768XBG_INIT_OR_RESUME" is needed. 
// **************************************************
// set display off
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100028);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(60);
// REM SPI_WriteData(0xc3,40,00,20);
// **************************************************
// **************************************************
// Example of Generic Long Write (Up to 8byte parameters)
// **************************************************
// Data Type 29h
SPI_32bit_4wire_write(0x06024029);//bit15:8=Packet Type:Long, bit5:0=Data Type Generic Long Write
SPI_32bit_4wire_write(0x06040004);//bit7:0=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)(zz)
SPI_32bit_4wire_write(0x061040C3);//bit7:0=Parameter1(gg), bit15:8=Parameter2(hh)
SPI_32bit_4wire_write(0x06122000);//bit7:0=Parameter3(ii), bit15:8=Parameter4(jj)
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(30);
// Enter LCD sleep mode (How to enter depends on LCD spec. Following setting is a example)
// enter sleep
SPI_32bit_4wire_write(0x06021005);//bit15:8=Packet Type:Short Packet, bit5:0=Data Type DCS Short Write (no parameter)
SPI_32bit_4wire_write(0x06040000);//bit7:6=Word Count(Lower Byte), bit15:8=Word Count(Upper Byte)
SPI_32bit_4wire_write(0x06100010);//
SPI_32bit_4wire_write(0x06000001);//Packet Transfer
LCD_delay_ms(60);
// RGB Port Disable (from "SetFrmStop to 1" to "Set RstRstPtr to 1")(Only for TC358768AXBG)
SPI_32bit_4wire_write(0x00328000);//Set Frame Stop to 1
LCD_delay_ms(40);
SPI_32bit_4wire_write(0x00040004);//Configuration Control Register Parallel input stop
SPI_32bit_4wire_write(0x0032C000);//Set RstPtr to 1
// Stop DSI continuous clock
SPI_32bit_4wire_write(0x02380000);//
SPI_32bit_4wire_write(0x023A0000);//
// Disable D-PHY  By this setting, DSI line becomes HiZ.  LCD side should permit HiZ input.
SPI_32bit_4wire_write(0x01400001);//
SPI_32bit_4wire_write(0x01420000);//
SPI_32bit_4wire_write(0x01440001);//
SPI_32bit_4wire_write(0x01460000);//
SPI_32bit_4wire_write(0x01480001);//
SPI_32bit_4wire_write(0x014A0000);//
SPI_32bit_4wire_write(0x014C0001);//
SPI_32bit_4wire_write(0x014E0000);//
SPI_32bit_4wire_write(0x01500001);//
SPI_32bit_4wire_write(0x01520000);//
// **************************************************
// Host: Stop RGB input including PCLK.   Stop REFCLK
// **************************************************
// If all the power of TC358768AXBG can be cut off,
// Set all the input signal to TC358768AXBG "L" or "HiZ" including RESX.
// Cut off all the power of TC358768AXBG
// **************************************************


	printk("\n\nRocky KD080D10_exit leave\n\n");

	TC358768_spi_csx_set(0);
	TC358768_spi_sck_set(0);
	TC358768_spi_sdi_set(0);
}

void TC358768_init(__panel_para_t * info)
{
		printk("\n\nRocky TC358768_init  enter\n\n");
		printk("Rocky@inet@2013-08-23_20-32\n");
		
		/*
		TC358768_reset(0);
		LCD_delay_ms(20);	
		TC358768_reset(1);
		*/

		printk("Chip1 ID=0x%x\n",SPI_32bit_4wire_read(0x00000000));
		printk("Chip2 ID=0x%x\n",SPI_32bit_4wire_read(0x00000000));
		printk("Chip3 ID=0x%x\n",SPI_32bit_4wire_read(0x00000000));
		
		script_item_u	val;
		script_item_value_type_e	type;
		type = script_get_item("lcd0_para","board_name", &val);
	  if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {  	
	    printk("fetch board_name from sys_config failed\n");
	  } else {
		  printk("board_name = %s\n",val.str);
	  }
		if(lcd_id_used)
			{
				if(!strcmp("Q680",val.str)) {//board Q680
					if(lcd_id == 0)
						SL008DS21B393_init();//suoling samsung lcd
					if(lcd_id == 1)
						S6D7AA0X01_BOE_init();//xingyuan samsung lcd
				} else {//QNV8
					if(lcd_id == 0)
						S6D7AA0X04_CPT_init();
					if(lcd_id == 1)
						S6D7AA0X01_BOE_init();
					if(lcd_id == 2)
						KD080D10_init();//guoxian au panel
				}
			}
		else
			{
				if(!strcmp("S6D7AA0X04_CPT",info->lcd_model_name))
					S6D7AA0X04_CPT_init();
				if(!strcmp("S6D7AA0X01_BOE",info->lcd_model_name))
					S6D7AA0X01_BOE_init();
				if(!strcmp("SL008DS21B393",info->lcd_model_name))
					SL008DS21B393_init();
				if(!strcmp("KD080D10",info->lcd_model_name))
					KD080D10_init();
				if(!strcmp("KR070IC4T",info->lcd_model_name))//xingyuan 7 inch
					KR070IC4T_init();
				if(!strcmp("HSD698",info->lcd_model_name))//suoling 6.98 inch hsd panel
					HSD6_98_OTM1283A_init();
			}
		if(!strcmp("b079xan01",info->lcd_model_name))//board Q790M,Q791M
			b079xan01_init();		
}

void TC358768_exit(__panel_para_t * info)
{
		printk("\n\nRocky TC358768_exit  enter\n\n");
		script_item_u	val;
		script_item_value_type_e	type;
		type = script_get_item("lcd0_para","board_name", &val);
	  if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {  	
	    printk("fetch board_name from sys_config failed\n");
	  } else {
		  printk("board_name = %s\n",val.str);
	  }
		if(lcd_id_used)
			{
				if(!strcmp("Q680",val.str)) {//board Q680
					if(lcd_id == 0)
						SL008DS21B393_exit();//suoling samsung lcd
					if(lcd_id == 1)
						S6D7AA0X01_BOE_exit();//xingyuan samsung lcd					
				} else {//board QNV8
					if(lcd_id == 0)
						S6D7AA0X04_CPT_exit();
					if(lcd_id == 1)
						S6D7AA0X01_BOE_exit();
					if(lcd_id == 2)
						KD080D10_exit();
				}
			}
		else
			{
				if(!strcmp("S6D7AA0X04_CPT",info->lcd_model_name))
					S6D7AA0X04_CPT_exit();
				if(!strcmp("S6D7AA0X01_BOE",info->lcd_model_name))
					S6D7AA0X01_BOE_exit();
				if(!strcmp("SL008DS21B393",info->lcd_model_name))
					SL008DS21B393_exit();
				if(!strcmp("KD080D10",info->lcd_model_name))
					KD080D10_exit();
				if(!strcmp("KR070IC4T",info->lcd_model_name))//xingyuan 7 inch
					KR070IC4T_exit();
				if(!strcmp("HSD698",info->lcd_model_name))//suoling 6.98 inch hsd panel
					HSD6_98_OTM1283A_exit();
			}
		if(!strcmp("b079xan01",info->lcd_model_name))
			b079xan01_exit();
}


