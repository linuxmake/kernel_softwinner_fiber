/*
 * drivers\staging\android\switch\switch_headset.c
 * (C) Copyright 2010-2016
 * reuuimllatech Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "switch.h"
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/scenelock.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/irqs-sun6i.h>
#include <mach/sys_config.h>
#include <mach/system.h>
#include <mach/gpio.h>

#undef SWITCH_DBG

#if (0)
    #define SWITCH_DBG(format,args...)  printk("[SWITCH] "format,##args)    
#else
    #define SWITCH_DBG(...)    
#endif

#define VIR_CODEC_BASSADDRESS      (0xf1c22c00)
#define SUN6I_PA_CTRL			   (0x24)
#define SUN6I_MIC_CTRL			   (0x28)
#define SUN6I_ADDAC_TUNE		   (0x30)
#define SUN6I_HMIC_CTL 	           (0x50)
#define SUN6I_HMIC_DATA	           (0x54)

/*0x24*/
#define HPPAEN					  (31)
#define HPCOM_CTL				  (29)
#define LTRNMUTE				  (25)
#define RTLNMUTE				  (24)

/*0x28 */
#define HBIASEN					  (31)
#define HBIASADCEN				  (29)
#define MIC2AMPEN				  (24)
#define LINEOUTL_EN			  	  (19)
#define LINEOUTR_EN			  	  (18)

/* 0x30 */
#define PA_SLOPE_SECECT			  (30)

/*HMIC Control Register
*codecbase+0x50
*/
#define HMIC_M					  (28)
#define HMIC_N					  (24)
#define HMIC_DIRQ				  (23)
#define HMIC_TH1_HYS			  (21)
#define	HMIC_EARPHONE_OUT_IRQ_EN  (20)
#define HMIC_EARPHONE_IN_IRQ_EN	  (19)
#define HMIC_KEY_UP_IRQ_EN		  (18)
#define HMIC_KEY_DOWN_IRQ_EN	  (17)
#define HMIC_DATA_IRQ_EN		  (16)
#define HMIC_DS_SAMP			  (14)
#define HMIC_TH2_HYS			  (13)
#define HMIC_TH2_KEY		      (8)
#define HMIC_SF_SMOOTH_FIL		  (6)
#define KEY_UP_IRQ_PEND			  (5)
#define HMIC_TH1_EARPHONE		  (0)

/*HMIC Data Register
* codecbase+0x54
*/
#define HMIC_EARPHONE_OUT_IRQ_PEND  (20)
#define HMIC_EARPHONE_IN_IRQ_PEND   (19)
#define HMIC_KEY_UP_IRQ_PEND 	    (18)
#define HMIC_KEY_DOWN_IRQ_PEND 		(17)
#define HMIC_DATA_IRQ_PEND			(16)
#define HMIC_ADC_DATA				(0)

#define FUNCTION_NAME "h2w"

#define hmic_rdreg(reg)	    readl((hmic_base+(reg)))
#define hmic_wrreg(reg,val)  writel((val),(hmic_base+(reg)))

static void __iomem *hmic_base;
static int headphone_direct_used = 0;
/*1=headphone in slot, else 0*/
static int headphone_state = 0;
/* key define */
#define KEY_HEADSETHOOK         226

/*
* 	CIRCLE_COUNT == 0, check the earphone state three times(audio_hmic_irq:one time and earphone_switch_timer_poll:two times)
*	CIRCLE_COUNT == 1, check the earphone state four times(audio_hmic_irq:one time and earphone_switch_timer_poll:three times)
*	CIRCLE_COUNT == 2, check the earphone state five times(audio_hmic_irq:one time and earphone_switch_timer_poll:four times)
*/
#define CIRCLE_COUNT			1
#define ERROR_COUNT             3//gongpiqiang+++
static void switch_resume_events(struct work_struct *work);
static struct workqueue_struct *resume_switch_work_queue = NULL;

static void codec_resume_events(struct work_struct *work);
struct workqueue_struct *resume_work_queue;
static DECLARE_WORK(codec_resume_work, codec_resume_events);

static int req_mute_status;
static script_item_u item_mute;
//static script_item_value_type_e  type;

enum headphone_mode_u {
	HEADPHONE_IDLE,
	FOUR_HEADPHONE_PLUGIN,
	THREE_HEADPHONE_PLUGIN,
};

struct gpio_switch_data {
	struct switch_dev sdev;
	int state;
	int check_three_count;
	int check_four_count;

	enum headphone_mode_u mode;		/* mode for three/four sector headphone */
	struct work_struct work;
	struct semaphore sem;
	struct timer_list timer;

	struct work_struct resume_work;
	struct input_dev *key;
};

/**
* codec_wrreg_bits - update codec register bits
* @reg: codec register
* @mask: register mask
* @value: new value
*
* Writes new register value.
* Return 1 for change else 0.
*/
int hmic_wrreg_bits(unsigned short reg, unsigned int	mask,	unsigned int value)
{
	unsigned int old, new;

	old	=	hmic_rdreg(reg);
	new	=	(old & ~mask) | value;

	hmic_wrreg(reg,new);

	return 0;
}

int hmic_wr_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	hmic_wrreg_bits(reg, mask, reg_val);
	return 0;
}

static void earphone_switch_timer_poll(unsigned long data)
{
	int tmp = 0;
	struct gpio_switch_data	*switch_data =(struct gpio_switch_data *)data;
	static int error_count = 0;//gongpiqiang+++
	tmp = hmic_rdreg(SUN6I_HMIC_DATA);
	tmp &= 0x1f;
	SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, tmp);

	if ((tmp >= 0xb) && (switch_data->mode != FOUR_HEADPHONE_PLUGIN) && (switch_data->state != 2)) {
		SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, tmp);
		if ((switch_data->check_three_count > CIRCLE_COUNT) && (switch_data->state != 2)) {
			/*it means the three sections earphone has plun in*/
			switch_data->mode = THREE_HEADPHONE_PLUGIN;
			switch_data->state 		= 2;
			schedule_work(&switch_data->work);

			SWITCH_DBG("%s,line:%d\n", __func__, __LINE__);
			switch_data->check_three_count = 0;
			error_count  = 0;
		}
		/*check again to reduce the disturb from earphone plug in unstable*/
		switch_data->check_three_count++;
		if (((&switch_data->timer) != NULL)) {
			SWITCH_DBG("%s,line:%d\n", __func__, __LINE__);
			mod_timer(&switch_data->timer, jiffies +  HZ/8);
		}
	} else if ((tmp>=0x1 && tmp<0xb) && (switch_data->mode != THREE_HEADPHONE_PLUGIN) && (switch_data->state != 1)) {
		SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, tmp);
		if ((switch_data->check_four_count > CIRCLE_COUNT) && (switch_data->state != 1)) {
			/*it means the four sections earphone has plun in*/
			switch_data->mode = FOUR_HEADPHONE_PLUGIN;
			switch_data->state 		= 1;
			schedule_work(&switch_data->work);

			SWITCH_DBG("%s,line:%d\n", __func__, __LINE__);
			switch_data->check_four_count = 0;
			error_count  = 0;
		}
		switch_data->check_four_count++;
		switch_data->check_three_count = 0;
		if (((&switch_data->timer) != NULL)) {
			mod_timer(&switch_data->timer, jiffies +  HZ/8);
		}
		SWITCH_DBG("%s,line:%d\n", __func__, __LINE__);
	} else if ((tmp>=0xb) && (switch_data->mode == FOUR_HEADPHONE_PLUGIN) && (switch_data->state == 1)) {
		SWITCH_DBG("%s,line:%d\n", __func__, __LINE__);
		/*hook down*/
		input_report_key(switch_data->key, KEY_HEADSETHOOK, 1);
		input_sync(switch_data->key);
		error_count  = 0;
		if (((&switch_data->timer) != NULL)) {
			mod_timer(&switch_data->timer, jiffies +  HZ/4);
		}
	} else if ((tmp<0xb) && (switch_data->mode == FOUR_HEADPHONE_PLUGIN) && (switch_data->state == 1)) {
		SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, tmp);
		/*hook up*/
		input_report_key(switch_data->key, KEY_HEADSETHOOK, 0);
		input_sync(switch_data->key);
		error_count  = 0;
		if (((&switch_data->timer) != NULL)) {
			mod_timer(&switch_data->timer, jiffies +  HZ/8);
		}
	} else {
		SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, tmp);
		//gongpiqiang+++ for headphone plugin det error
		error_count ++;
		SWITCH_DBG("%s,line:%d,error_count=%d\n", __func__, __LINE__,error_count);
		if(error_count > ERROR_COUNT) {
			error_count = 0;
			return ;
		}//gongpiqiang--- for headphone plugin det error
		switch_data->mode = HEADPHONE_IDLE;
		switch_data->state 		= 0;
		if (((&switch_data->timer) != NULL)) {
			mod_timer(&switch_data->timer, jiffies +  HZ/8);
		}
	}
}

static void earphone_switch_work(struct work_struct *work)
{
	struct gpio_switch_data	*switch_data =
		container_of(work, struct gpio_switch_data, work);

	SWITCH_DBG("%s,line:%d, data->state:%d\n", __func__, __LINE__, switch_data->state);
	down(&switch_data->sem);
	switch_set_state(&switch_data->sdev, switch_data->state);
	up(&switch_data->sem);
	if (((&switch_data->timer) != NULL)&&(switch_data->state==2)) {
		SWITCH_DBG("%s,line:%d\n", __func__, __LINE__);
		del_timer(&switch_data->timer);
	}
	hmic_wr_control(SUN6I_PA_CTRL, 0x1, HPPAEN, 0x0);
	usleep_range(50,100);
	hmic_wr_control(SUN6I_PA_CTRL, 0x1, HPPAEN, 0x1);
}

static irqreturn_t audio_hmic_irq(int irq, void *dev_id)
{
	int tmp = 0;
	struct gpio_switch_data *switch_data = (struct gpio_switch_data *)dev_id;

	if (switch_data == NULL) {
		return IRQ_NONE;
	}

	hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_KEY_DOWN_IRQ_PEND, 0x1);
	hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_EARPHONE_IN_IRQ_PEND, 0x1);
	hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_KEY_UP_IRQ_PEND, 0x1);
	hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_EARPHONE_OUT_IRQ_PEND, 0x1);
	hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_DATA_IRQ_PEND, 0x1);
	switch_data->mode = HEADPHONE_IDLE;
	switch_data->check_three_count = 0;
	switch_data->check_four_count = 0;
	tmp = hmic_rdreg(SUN6I_HMIC_DATA);
	tmp &= 0x1f;
SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, tmp);

	if (((&switch_data->timer) != NULL)) {
		del_timer(&switch_data->timer);
	}

	if (!headphone_direct_used) {
		if (tmp > 2) {//allwinner modify for headphone plugout det eroor		
			SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, tmp);
			init_timer(&switch_data->timer);
			switch_data->timer.function = earphone_switch_timer_poll;
			switch_data->timer.data = (unsigned long)switch_data;
			mod_timer(&switch_data->timer, jiffies +  HZ/8 );
			headphone_state = 1;
			SWITCH_DBG("%s,line:%d,headphone_state:%d\n", __func__, __LINE__, headphone_state);
			return IRQ_HANDLED;
		} else {
			SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, (tmp&0x1f));
			/*if the irq is hmic earphone pull out, when the irq coming, clean the pending bit*/
			headphone_state 	= 0;
			switch_data->state 	= 0;
		}
	} else {
		/*headphone_direct_used == 1*/
		if (tmp > 0) {
			SWITCH_DBG("headphone three or four HP,HMIC_DAT= %d\n",(tmp&0x1f));
			switch_data->state 	= 2;
			headphone_state 	= 1;
		} else {
			SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, (tmp&0x1f));
			/*if the irq is hmic earphone pull out, when the irq coming, clean the pending bit*/
			headphone_state 	= 0;
			switch_data->state 	= 0;
		}
	}
	schedule_work(&switch_data->work);
	return IRQ_HANDLED;
}

static void codec_resume_events(struct work_struct *work)
{
	int headphone_mute_used = 0;
	script_item_u val;
	script_item_value_type_e  type;
   	/*fix the resume blaze blaze noise*/
	hmic_wr_control(SUN6I_ADDAC_TUNE, 0x1, PA_SLOPE_SECECT, 0x1);
	hmic_wr_control(SUN6I_PA_CTRL, 0x1, HPPAEN, 0x1);
	msleep(450);
	type = script_get_item("audio_para", "headphone_mute_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_mute_used type err!\n");
    }
	headphone_mute_used = val.val;
	if (headphone_mute_used) {
		item_mute.gpio.data = 1;
		/*config gpio info of headphone_mute_used, the default pa config is close(check sys_config.fex).*/
		if (0 != sw_gpio_setall_range(&item_mute.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
	}
	msleep(200);
	/*audio codec hardware bug. the HBIASADCEN bit must be enable in init*/
	hmic_wr_control(SUN6I_MIC_CTRL, 0x1, HBIASADCEN, 0x1);
	hmic_wr_control(SUN6I_MIC_CTRL, 0x1, HBIASEN, 0x1);

	printk("====codec_resume_events===\n");
}

static void switch_resume_events(struct work_struct *work)
{
	int tmp = 0,tmp1 = 0;
	int headphone_mute_used = 0;
	script_item_u val;
	script_item_value_type_e  type;

	struct gpio_switch_data *switch_data = container_of(work,
				struct gpio_switch_data, resume_work);

	if (switch_data == NULL) {
		SWITCH_DBG("%s, %d, switch_data is NULL\n", __func__, __LINE__);
		return;
	}
   	/*fix the resume blaze blaze noise*/
	hmic_wr_control(SUN6I_ADDAC_TUNE, 0x1, PA_SLOPE_SECECT, 0x1);
	hmic_wr_control(SUN6I_PA_CTRL, 0x1, HPPAEN, 0x1);
	msleep(450);
	type = script_get_item("audio_para", "headphone_mute_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_mute_used type err!\n");
    }
	headphone_mute_used = val.val;
	if (headphone_mute_used) {
		item_mute.gpio.data = 1;
		/*config gpio info of headphone_mute_used, the default pa config is close(check sys_config.fex).*/
		if (0 != sw_gpio_setall_range(&item_mute.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
	}
	msleep(200);

	/*audio codec hardware bug. the HBIASADCEN bit must be enable in init*/
	hmic_wr_control(SUN6I_MIC_CTRL, 0x1, HBIASADCEN, 0x1);
	hmic_wr_control(SUN6I_MIC_CTRL, 0x1, HBIASEN, 0x1);
	SWITCH_DBG("%s,line:%d\n", __func__, __LINE__);
	msleep(200);
	tmp = hmic_rdreg(SUN6I_HMIC_DATA);
	tmp1 =(tmp&0x1f);
	switch_data->mode = HEADPHONE_IDLE;
	switch_data->check_three_count = 0;
	switch_data->check_four_count = 0;
	SWITCH_DBG("%s,line:%d,headphone_state:%d, tmp:%x\n", __func__, __LINE__, headphone_state, tmp);
	if ( (tmp & (0x1<<20)) || (headphone_state ==1 && tmp == 0) )  { //plug out
		SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, (tmp&0x1f));
		/*if the irq is hmic earphone pull out, when the irq coming, clean the pending bit*/
		hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_EARPHONE_OUT_IRQ_PEND, 0x1);
		switch_data->state = 0;
		headphone_state = 0;
		// schedule_work(&switch_data->work);
		down(&switch_data->sem);
		switch_set_state(&switch_data->sdev, switch_data->state);
		up(&switch_data->sem);
	} else if ((tmp1>0x0) && (tmp1<0xb)) {
		switch_data->mode = FOUR_HEADPHONE_PLUGIN;
		if (((&switch_data->timer) != NULL)) {
			del_timer(&switch_data->timer);
		}
		init_timer(&switch_data->timer);
		switch_data->timer.function = earphone_switch_timer_poll;
		switch_data->timer.data = (unsigned long)switch_data;
		mod_timer(&switch_data->timer, jiffies +  HZ/8 );
		SWITCH_DBG("%s,line:%d,headphone_state:%d, tmp1:%x\n", __func__, __LINE__, headphone_state, tmp1);
	} else if (tmp1>=0xb) {
		switch_data->mode = THREE_HEADPHONE_PLUGIN;
		SWITCH_DBG("%s,line:%d,headphone_state:%d, tmp1:%x\n", __func__, __LINE__, headphone_state, tmp1);
	}
}

static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);

	return sprintf(buf, "%d\n", switch_data->state);
}

static ssize_t print_headset_name(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);

	return sprintf(buf, "%s\n", switch_data->sdev.name);
}

static int gpio_switch_probe(struct platform_device *pdev)
{
	int headphone_mute_used = 0;
	struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_switch_data *switch_data;
	int ret = 0;
	script_item_u val;
	script_item_value_type_e  type;

	if (!pdata) {
		return -EBUSY;
	}

	type = script_get_item("audio_para", "headphone_direct_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("[audiocodec] type err!\n");
	}
	headphone_direct_used = val.val;

	hmic_base = (void __iomem *)VIR_CODEC_BASSADDRESS;
	hmic_wr_control(SUN6I_HMIC_CTL, 0xf, HMIC_M, 0xf);						/*0xf should be get from hw_debug 28*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0xf, HMIC_N, 0x1);						/*0xf should be get from hw_debug 24 0xf*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1, HMIC_EARPHONE_OUT_IRQ_EN, 0x1); 	/*20*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1, HMIC_EARPHONE_IN_IRQ_EN, 0x1); 	/*19*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0x3, HMIC_DS_SAMP, 0x3);//0x1 				/*14 */  //adc freq,,gongpiqiang
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1f, HMIC_TH2_KEY, 0x8);				/*0xf should be get from hw_debug 8*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1f, HMIC_TH1_EARPHONE, 0x1);			/*0x1 should be get from hw_debug 0*/

	switch_data = kzalloc(sizeof(struct gpio_switch_data), GFP_KERNEL);
	if (!switch_data) {
		printk("%s,line:%d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, (void *)switch_data);

	switch_data->sdev.state 		= 0;
	switch_data->state				= -1;
	switch_data->sdev.name 			= pdata->name;
	switch_data->sdev.print_name 	= print_headset_name;
	switch_data->sdev.print_state 	= switch_gpio_print_state;
	INIT_WORK(&switch_data->work, earphone_switch_work);
	INIT_WORK(&switch_data->resume_work, switch_resume_events);
 	/* create input device */
    switch_data->key = input_allocate_device();
    if (!switch_data->key) {
        printk(KERN_ERR "gpio_switch_probe: not enough memory for input device\n");
        ret = -ENOMEM;
        goto err_input_allocate_device;
    }

    switch_data->key->name          = "headset";
    switch_data->key->phys          = "headset/input0";
    switch_data->key->id.bustype    = BUS_HOST;
    switch_data->key->id.vendor     = 0x0001;
    switch_data->key->id.product    = 0xffff;
    switch_data->key->id.version    = 0x0100;

    switch_data->key->evbit[0] = BIT_MASK(EV_KEY);

    set_bit(KEY_HEADSETHOOK, switch_data->key->keybit);

    ret = input_register_device(switch_data->key);
    if (ret) {
        printk(KERN_ERR "gpio_switch_probe: input_register_device failed\n");
        goto err_input_register_device;
    }

	headphone_state = 0;
	sema_init(&switch_data->sem, 1);

	ret = switch_dev_register(&switch_data->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}

	ret = request_irq(AW_IRQ_CODEC, audio_hmic_irq, 0, "audio_hmic_irq", switch_data);
	if (ret < 0) {
		printk("request irq err\n");
		ret = -EINVAL;
		goto err_request_irq;
	}
	
	resume_switch_work_queue = create_singlethread_workqueue("switch_resume");
	if (resume_switch_work_queue == NULL) {
		printk("[switch_headset] try to create workqueue for codec failed!\n");
		ret = -ENOMEM;
		goto err_switch_work_queue;
	}

	resume_work_queue = create_singlethread_workqueue("codec_resume");
	if (resume_work_queue == NULL) {
		printk("[sun4i-codec] try to create workqueue for codec failed!\n");
		ret = -ENOMEM;
		goto err_switch_work_queue;
	}

	type = script_get_item("audio_para", "headphone_mute_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_mute_used type err!\n");
    }
	headphone_mute_used = val.val;
	if (headphone_mute_used) {
		/*get the default headphone mute val(close)*/
		type = script_get_item("audio_para", "audio_mute_ctrl", &item_mute);
		if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
			printk("script_get_item return type err\n");
			return -EFAULT;
		}
		/*request gpio*/
		req_mute_status = gpio_request(item_mute.gpio.gpio, NULL);
		if (0 != req_mute_status) {
			printk("request gpio headphone mute failed!\n");
		}
		/*config gpio info of headphone_mute_used, the default pa config is close(check sys_config.fex).*/
		if (0 != sw_gpio_setall_range(&item_mute.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
	}
	queue_work(resume_work_queue, &codec_resume_work);
	return 0;

err_switch_work_queue:
	free_irq(AW_IRQ_CODEC, switch_data);
err_request_irq:
	switch_dev_unregister(&switch_data->sdev);
err_input_register_device:
    if(switch_data->key){
        input_free_device(switch_data->key);
    }

err_input_allocate_device:
    switch_dev_unregister(&switch_data->sdev);

err_switch_dev_register:
	kfree(switch_data);

	return ret;
}

static int switch_suspend(struct platform_device *pdev,pm_message_t state)
{
	int headphone_mute_used = 0;
	script_item_u val;
	script_item_value_type_e  type;

	/* check if called in talking standby */
	if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
		printk("In talking standby, do not suspend!!\n");
		return 0;
	}
	type = script_get_item("audio_para", "headphone_mute_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_mute_used type err!\n");
    }
	headphone_mute_used = val.val;
	if (headphone_mute_used) {
		item_mute.gpio.data = 0;
		/*config gpio info of headphone_mute_used, the default pa config is close(check sys_config.fex).*/
		if (0 != sw_gpio_setall_range(&item_mute.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
	}
	msleep(150);
	/*fix the resume blaze blaze noise*/
	hmic_wr_control(SUN6I_ADDAC_TUNE, 0x1, PA_SLOPE_SECECT, 0x1);
	/*disable pa*/
	hmic_wr_control(SUN6I_PA_CTRL, 0x1, HPPAEN, 0x0);
	msleep(350);

	return 0;
}

static int switch_resume(struct platform_device *pdev)
{
	struct gpio_switch_data *switch_data;

	if (check_scene_locked(SCENE_TALKING_STANDBY) != 0) {
		hmic_wr_control(SUN6I_HMIC_CTL, 0xf, HMIC_M, 0xf);						/*0xf should be get from hw_debug 28*/
		hmic_wr_control(SUN6I_HMIC_CTL, 0xf, HMIC_N, 0x1);						/*0xf should be get from hw_debug 24*/
		hmic_wr_control(SUN6I_HMIC_CTL, 0x1, HMIC_EARPHONE_OUT_IRQ_EN, 0x1); 	/*20*/
		hmic_wr_control(SUN6I_HMIC_CTL, 0x1, HMIC_EARPHONE_IN_IRQ_EN, 0x1); 	/*19*/
		hmic_wr_control(SUN6I_HMIC_CTL, 0x3, HMIC_DS_SAMP, 0x3); //0x1				/*14*/ //adc freq,,gongpiqiang
		hmic_wr_control(SUN6I_HMIC_CTL, 0x1f, HMIC_TH2_KEY, 0x8);				/*0xf should be get from hw_debug 8*/
		hmic_wr_control(SUN6I_HMIC_CTL, 0x1f, HMIC_TH1_EARPHONE, 0x1);			/*0x1 should be get from hw_debug 0*/
	}
	switch_data = (struct gpio_switch_data *)platform_get_drvdata(pdev);

	if (switch_data != NULL) {
		queue_work(resume_switch_work_queue, &switch_data->resume_work);
	}
	return 0;
}

static void switch_shutdown(struct platform_device *devptr)
{
	int headphone_mute_used = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "headphone_mute_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_mute_used type err!\n");
    }
	headphone_mute_used = val.val;
	if (headphone_mute_used) {
		item_mute.gpio.data = 0;
		/*config gpio info of headphone_mute_used, the default pa config is close(check sys_config.fex).*/
		if (0 != sw_gpio_setall_range(&item_mute.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
	}
	msleep(150);
	/*fix the resume blaze blaze noise*/
	hmic_wr_control(SUN6I_ADDAC_TUNE, 0x1, PA_SLOPE_SECECT, 0x1);
	/*disable pa*/
	hmic_wr_control(SUN6I_PA_CTRL, 0x1, HPPAEN, 0x0);
	msleep(450);
}

static int __devexit gpio_switch_remove(struct platform_device *pdev)
{
	struct gpio_switch_data *switch_data = platform_get_drvdata(pdev);
	
	if (resume_switch_work_queue != NULL) {
		flush_workqueue(resume_switch_work_queue);
		destroy_workqueue(resume_switch_work_queue);
	}
	
    if (switch_data->key) {
        input_unregister_device(switch_data->key);
        input_free_device(switch_data->key);
    }

    switch_dev_unregister(&switch_data->sdev);

	kfree(switch_data);	

	return 0;
}

static struct platform_driver gpio_switch_driver = {
	.probe		= gpio_switch_probe,
	.remove		= __devexit_p(gpio_switch_remove),
	.driver		= {
		.name	= "switch-gpio",
		.owner	= THIS_MODULE,
	},
	.suspend	= switch_suspend,
	.resume		= switch_resume,
	.shutdown   = switch_shutdown,
};

static struct gpio_switch_platform_data headset_switch_data = { 
    .name = "h2w",
};

static struct platform_device gpio_switch_device = { 
    .name = "switch-gpio",
    .dev = { 
    	.platform_data = &headset_switch_data,
    }   
};

static int __init gpio_switch_init(void)
{
	int ret = 0;
    
	ret = platform_device_register(&gpio_switch_device);
	if (ret == 0) {
		ret = platform_driver_register(&gpio_switch_driver);
	}

	return ret;
}

static void __exit gpio_switch_exit(void)
{
	platform_driver_unregister(&gpio_switch_driver);
	platform_device_unregister(&gpio_switch_device);

}
module_init(gpio_switch_init);
module_exit(gpio_switch_exit);

MODULE_AUTHOR("huanxin<huanxin@reuuimllatech.com>");
MODULE_DESCRIPTION("GPIO Switch driver");
MODULE_LICENSE("GPL");
