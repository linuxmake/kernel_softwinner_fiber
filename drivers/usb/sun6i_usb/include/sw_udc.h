/*
*************************************************************************************
*                         			      Linux
*					           USB Device Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_udc.h
*
* Author 		: javen
*
* Description 	: USB Device 控制器驱动
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-3-3            1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_UDC_H__
#define  __SW_UDC_H__

#include <linux/usb.h>
#include <linux/usb/gadget.h>
//#include <mach/dma.h>
#include <linux/dma-mapping.h>

/*  */
typedef struct sw_udc_ep {
	struct list_head		queue;
	unsigned long			last_io;	/* jiffies timestamp */
	struct usb_gadget		*gadget;
	struct sw_udc		    *dev;
	const struct usb_endpoint_descriptor *desc;
	struct usb_ep			ep;
	u8				        num;

	unsigned short			fifo_size;
	u8				        bEndpointAddress;
	u8				        bmAttributes;

	unsigned			    halted : 1;
	unsigned			    already_seen : 1;
	unsigned			    setup_stage : 1;

	__u32					dma_working;		/* flag. is dma busy? 		*/
	__u32 					dma_transfer_len;	/* dma want transfer length */
}sw_udc_ep_t;


/* Warning : ep0 has a fifo of 16 bytes */
/* Don't try to set 32 or 64            */
/* also testusb 14 fails  wit 16 but is */
/* fine with 8                          */
//#define  EP0_FIFO_SIZE		    8
#define  EP0_FIFO_SIZE		    64

#define  SW_UDC_EP_FIFO_SIZE	    512

#define	 SW_UDC_EP_CTRL_INDEX			0x00
#define  SW_UDC_EP_BULK_IN_INDEX		0x01
#define  SW_UDC_EP_BULK_OUT_INDEX		0x02

#ifdef  SW_UDC_DOUBLE_FIFO
#define  SW_UDC_FIFO_NUM			1
#else
#define  SW_UDC_FIFO_NUM			0
#endif

#define	SW_UDC_TEST_J		0x0100
#define	SW_UDC_TEST_K		0x0200
#define	SW_UDC_TEST_SE0_NAK	0x0300
#define	SW_UDC_TEST_PACKET	0x0400


static const char ep0name [] = "ep0";

static const char *const ep_name[] = {
	ep0name,	/* everyone has ep0 */

	/* sw_udc four bidirectional bulk endpoints */
	"ep1-bulk",
	"ep2-bulk",
	"ep3-bulk",
	"ep4-bulk",
	"ep5-int"
};

#define SW_UDC_ENDPOINTS       ARRAY_SIZE(ep_name)

#define  is_tx_ep(ep)		((ep->bEndpointAddress) & USB_DIR_IN)

enum sw_buffer_map_state {
	UN_MAPPED = 0,
	PRE_MAPPED,
	SW_UDC_USB_MAPPED
};

struct sw_udc_request {
	struct list_head		queue;		/* ep's requests */
	struct usb_request		req;

	__u32 is_queue;  /* flag. 是否已经压入队列? */
	enum sw_buffer_map_state map_state;
};

enum ep0_state {
        EP0_IDLE,
        EP0_IN_DATA_PHASE,
        EP0_OUT_DATA_PHASE,
        EP0_END_XFER,
        EP0_STALL,
};

/*
static const char *ep0states[]= {
        "EP0_IDLE",
        "EP0_IN_DATA_PHASE",
        "EP0_OUT_DATA_PHASE",
        "EP0_END_XFER",
        "EP0_STALL",
};
*/
#if 1
//---------------------------------------------------------------
//  DMA
//---------------------------------------------------------------
typedef struct sw_udc_dma{
	char name[32];
	//struct sw_dma_client dma_client;
	int is_start;
	int dma_hdle;	/* dma 句柄 */
}sw_udc_dma_t;

/* dma 传输参数 */
typedef struct sw_udc_dma_parg{
	struct sw_udc *dev;
	struct sw_udc_ep *ep;
	struct sw_udc_request *req;
}sw_udc_dma_parg_t;
#endif


/* i/o 信息 */
typedef struct sw_udc_io{
	struct resource	*usb_base_res;   	/* USB  resources 		*/
	struct resource	*usb_base_req;   	/* USB  resources 		*/
	void __iomem	*usb_vbase;			/* USB  base address 	*/

	struct resource	*sram_base_res;   	/* SRAM resources 		*/
	struct resource	*sram_base_req;   	/* SRAM resources 		*/
	void __iomem	*sram_vbase;		/* SRAM base address 	*/

	struct resource	*clock_base_res;   	/* clock resources 		*/
	struct resource	*clock_base_req;   	/* clock resources 		*/
	void __iomem	*clock_vbase;		/* clock base address 	*/

	bsp_usbc_t usbc;					/* usb bsp config 		*/
	__hdle usb_bsp_hdle;				/* usb bsp handle 		*/

	__u32 clk_is_open;					/* is usb clock open? 	*/
	struct clk	*ahb_otg;				/* ahb clock handle 	*/
	struct clk	*mod_usbotg;			/* mod_usb otg clock handle 	*/
	struct clk	*mod_usbphy;			/* PHY0 clock handle 	*/
}sw_udc_io_t;

//---------------------------------------------------------------
//
//---------------------------------------------------------------
typedef struct sw_udc {
	spinlock_t			        lock;
    struct platform_device      *pdev;
	struct device		        *controller;

	struct sw_udc_ep		    ep[SW_UDC_ENDPOINTS];
	int				            address;
	struct usb_gadget		    gadget;
	struct usb_gadget_driver	*driver;
	struct sw_udc_request		fifo_req;
	u8				            fifo_buf[SW_UDC_EP_FIFO_SIZE];
	u16				            devstatus;

	u32				            port_status;
	int				            ep0state;

	unsigned			        got_irq : 1;

	unsigned			        req_std : 1;
	unsigned			        req_config : 1;
	unsigned			        req_pending : 1;
	u8				            vbus;
	struct dentry			    *regs_info;

	sw_udc_io_t					*sw_udc_io;
	char 						driver_name[32];
	__u32 						usbc_no;	/* 控制器端口号 	*/
	sw_udc_dma_t 			    sw_udc_dma[6];

	u32							stoped;		/* 控制器停止工作 	*/
	u32 						irq_no;		/* USB 中断号 		*/
}sw_udc_t;

enum sw_udc_cmd_e {
	SW_UDC_P_ENABLE	= 1,	/* Pull-up enable        */
	SW_UDC_P_DISABLE = 2,	/* Pull-up disable       */
	SW_UDC_P_RESET	= 3,	/* UDC reset, in case of */
};

typedef struct sw_udc_mach_info {
	struct usb_port_info *port_info;
	unsigned int usbc_base;
}sw_udc_mach_info_t;


int sw_usb_device_enable(void);
int sw_usb_device_disable(void);

#endif   //__SW_UDC_H__

