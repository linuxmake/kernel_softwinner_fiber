#include <linux/delay.h>
#include <mach/system.h>

#include <linux/init.h>
#include <linux/module.h>

#include "inet_ctp.h"

int m_inet_ctpState=0;

/*
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

static int getInetCtpId(void)
{
  int i;
  int x1[ID_READ_COUNT],x2[ID_READ_COUNT],y1[ID_READ_COUNT],y2[ID_READ_COUNT];
  int tp_sum[4]={0,0,0,0};
	writel(0x00a303ff,TP_BASSADDRESS + TP_CTRL0);
	writel(0x0000001f,TP_BASSADDRESS + TP_CTRL1);
	writel(0x10310,TP_BASSADDRESS + TP_INT_FIFOC);
	for(i =0 ;i < 400; i ++)
		udelay(1000); 
  //reg_val = readl(TP_BASSADDRESS+TP_CTRL1);
  //printk("TP_CTRL1 0x%08x\n",reg_val);
  for(i=0; i < ID_READ_COUNT; i ++)
  {	
  	x1[i] = readl(TP_BASSADDRESS + TP_DATA);
  	x2[i] = readl(TP_BASSADDRESS + TP_DATA);
		y1[i] = readl(TP_BASSADDRESS + TP_DATA);
  	y2[i] = readl(TP_BASSADDRESS + TP_DATA);
  	printk("\n %d,%d,%d,%d \n",x1[i],x2[i],y1[i],y2[i]);
	}
	touchbubblesort(x1,VALID_COUNT);
	touchbubblesort(x2,VALID_COUNT);
	touchbubblesort(y1,VALID_COUNT);
	touchbubblesort(y2,VALID_COUNT);
	
	for(i=2;i<4;i++) {
		tp_sum[0] += x1[i];
		tp_sum[1] += x2[i];
		tp_sum[2] += y1[i];
		tp_sum[3] += y2[i];
	}
	printk("x1 %d,x2 %d,y1 %d,y2 %d \n",tp_sum[0]/2,tp_sum[1]/2,tp_sum[2]/2,tp_sum[3]/2); 
	for(i = 0; i < 4; i ++)
  {
		device_id <<= 1;
		if(tp_sum[i]/2 >ADC_TO_HIGH)
		{	
			device_id |= 1; 
		}
	}
	
	printk("inet ctp  raw device_id =%d\n",device_id);
		
	if(device_id > MAX_DEVICE_NUM)
		device_id = 1;
	if(device_id)
		device_id -= 1;
	printk("inet ctp device_id =%d\n",device_id);
	
  return device_id;
}
*/

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

int  check_status(void)
{
	__u32  val = 0;    
	val = readl(TP_BASSADDRESS + TP_INT_FIFOS);
	if(val & 0x10000) 
	{  
		writel(0x10000,TP_BASSADDRESS + TP_INT_FIFOS);
		return 1;
	}
	return 0;
}

static int getInetCtpId(void)
{
	int device_id=0;
	
  int i;
  int x1[ID_READ_COUNT],x2[ID_READ_COUNT],y1[ID_READ_COUNT],y2[ID_READ_COUNT];
  int tp_sum[4]={0,0,0,0};
  
  writel(0x000000000,TP_BASSADDRESS + TP_CTRL0);
	writel(0x00000000,TP_BASSADDRESS + TP_CTRL1);
	writel(0x00000,TP_BASSADDRESS + TP_INT_FIFOC);
	for(i =0 ;i < 400; i ++)
		udelay(1000); 
		
	writel(0x00a303ff,TP_BASSADDRESS + TP_CTRL0);
	writel(0x0000001f,TP_BASSADDRESS + TP_CTRL1);
	writel(0x10310,TP_BASSADDRESS + TP_INT_FIFOC);
	for(i =0 ;i < 400; i ++)
		udelay(1000); 
  //reg_val = readl(TP_BASSADDRESS+TP_CTRL1);
  //printk("TP_CTRL1 0x%08x\n",reg_val);
  writel(0x10000,TP_BASSADDRESS + TP_INT_FIFOS);
  while(!check_status()); 
  for(i=0; i < ID_READ_COUNT; i ++)
  {
  	while(!check_status()); 
  	x1[i] = readl(TP_BASSADDRESS + TP_DATA);
  	x2[i] = readl(TP_BASSADDRESS + TP_DATA);
		y1[i] = readl(TP_BASSADDRESS + TP_DATA);
  	y2[i] = readl(TP_BASSADDRESS + TP_DATA);
  	printk("\n %d,%d,%d,%d \n",x1[i],x2[i],y1[i],y2[i]);
	}
	touchbubblesort(x1,VALID_COUNT);
	touchbubblesort(x2,VALID_COUNT);
	touchbubblesort(y1,VALID_COUNT);
	touchbubblesort(y2,VALID_COUNT);
	
	for(i=2;i<4;i++) {
		tp_sum[0] += x1[i];
		tp_sum[1] += x2[i];
		tp_sum[2] += y1[i];
		tp_sum[3] += y2[i];
	}
	printk("x1 %d,x2 %d,y1 %d,y2 %d \n",tp_sum[0]/2,tp_sum[1]/2,tp_sum[2]/2,tp_sum[3]/2); 
	for(i = 0; i < 4; i ++)
  {
		device_id <<= 1;
		if(tp_sum[i]/2 >ADC_TO_HIGH)
		{	
			device_id |= 1; 
		}
	}
	printk("inet get ctp device_id: %d\n",device_id);
	if(device_id > MAX_DEVICE_NUM)
		device_id = 1;
	//if(device_id)
	//	device_id -= 1;
	printk("device_id=%d\n",device_id);
	
  return device_id;
}


static int inet_ctp_init(void)
{
	printk("inet_ctp_init");
	return 0;
}

static void inet_ctp_exit(void)
{
	printk("inet_ctp_exit");
}


module_init(inet_ctp_init);
module_exit(inet_ctp_exit);

EXPORT_SYMBOL(getInetCtpId);
EXPORT_SYMBOL(m_inet_ctpState);
MODULE_AUTHOR("<mbgalex@163.com>");
MODULE_DESCRIPTION("INET CTP CONTROL");
MODULE_LICENSE("Dual BSD/GPL");
