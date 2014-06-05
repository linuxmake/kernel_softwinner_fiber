#include "lcd_dp501.h"



__u8 WriteByteP0(__u8 sub_addr, __u8 d)
{
	lcd_iic_write(0x10, sub_addr, d);
	return 0;
}

__u8 WriteByteP2(__u8 sub_addr, __u8 d)
{
	lcd_iic_write(0x14, sub_addr, d);
	return 0;
}

__u8 ReadByteP0(__u8 sub_addr, __u8  *d)
{
	lcd_iic_read(0x10, sub_addr, d);
	return 0;
}
__u8 ReadByteP2(__u8 sub_addr, __u8  *d)
{
	lcd_iic_read(0x14, sub_addr, d);
	return 0;
}

__u8 GetChipID(__u8 sub_addr)
{
	__u8 val1,val2,val3,ret1,ret2;
	val1=0xff;
	val2=0xff;
	val3=0xff;
	ret1=ret2=0;
	
	ret1=lcd_iic_read(sub_addr, 0x80, &val1);
	
	ret2=lcd_iic_read(sub_addr, 0x81, &val2);
	
	lcd_iic_read(sub_addr, 0x82, &val3);
	

	
	printk("chip id i2c addr=0x%x 0x80=0x%x\n",sub_addr,val1);
	printk("chip id i2c addr=0x%x 0x81=0x%x\n",sub_addr,val2);
	printk("chip id i2c addr=0x%x 0x82=0x%x\n",sub_addr,val3);
	
	return 0;
}




void dp501_init(__panel_para_t * info)
{  
		__u8 val2,val3;
		int j=0;

		val2=2;
		val3=3;
		printk("**dp501@Rocky@20130506_1011**\n");
		
		//Read chip ic
		GetChipID(0x14);	
		
		
		//first,dp501 initialization
		WriteByteP2(0x24,0x02);
		WriteByteP2(0x25,0x04);
		WriteByteP2(0x26,0x10); //PIO setting
		WriteByteP0(0x0a,0x0c); //block 74 & 76
		WriteByteP0(0x27,0x30); //auto detect CRTC 
		WriteByteP0(0x2f,0x82); //reset tpfifo at v blank 
		WriteByteP0(0x24,0xc4); //DVO mapping
		WriteByteP0(0x20,0x00); //Rocky 6bit pannel
		WriteByteP0(0x28,0x07); //crtc follow mode
		WriteByteP0(0x87,0x7f); //aux retry
		WriteByteP0(0x88,0x1e); //aux retry
		WriteByteP0(0xbb,0x06); //aux retry
		WriteByteP0(0x72,0xa9); //DPCD readable
		
		ReadByteP0(0x20, &val2);
		printk("0x20=%x\n",val2); //0x0a or 0x06
//		ReadByteP0(0x52, &val2);
//		printk("0x52=%x\n",val2);//0x82
		
		
		WriteByteP0(0x60,0x00); //Scramble on
		WriteByteP0(0x8f,0x02); //debug select, read P0.0x8d[2] can check HPD
		WriteByteP2(0x24,0x22);
		WriteByteP2(0x00,0x6C);
		WriteByteP2(0x01,0x68);
		WriteByteP2(0x02,0x28);
		WriteByteP2(0x03,0x2A);
		WriteByteP2(0x16,0x50);// code for power optimization
		
		//then,start training
		WriteByteP0(0x5d,0x0a); //training link rate(2.7Gbps)
		WriteByteP0(0x5e,0x81); //training lane count(4Lanes),
		WriteByteP0(0x74,0x00); //idle pattern
		WriteByteP0(0x5f,0x0d); //trigger training
		mdelay(100); //delay 100ms
		
		
		// check training result

		ReadByteP0(0x63, &val2); //0x77
		printk("val2=0x%x\n",val2);
		
		
//		for(j=0x10;j<=0x1f;j++)
//		{
//			ReadByteP0(j,&val2);
//			printk("0x%x=0x%x\n",j,val2);
//		}
		
		//reg15 14
		//reg1b 1a
		
		//ReadByteP0(0x64, &val3); //Each 4bits stand for one lane, 0x77/0x77 means training succeed with 4Lane
		
		
		//printk("val3=%0x%x\n",val3);
		
		printk("dp501_init end\n");
}

void dp501_exit(void)
{
	printk("dp501_exit \n");
}
