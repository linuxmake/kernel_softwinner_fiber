/* 
 * sunxi actuator driver
 */

#include "actuator.h"

#define SUNXI_ACT_NAME "ov8825_act"
#define SUNXI_ACT_ID 0x78

#define	TOTAL_STEPS_ACTIVE			64

DEFINE_MUTEX(act_mutex);
//declaration
static struct actuator_ctrl_t act_t;
//static struct v4l2_subdev act_subdev;

//static unsigned short subdev_step_pos_table[2*TOTAL_STEPS_ACTIVE];

/*
 Please implement this for each actuator device!!!
*/
static int subdev_i2c_write(struct actuator_ctrl_t *act_ctrl, unsigned short halfword, void* pt)
{
	int ret=0;
	struct i2c_client *client;
//	struct i2c_msg msg;
	unsigned char data[2];
	
	client = act_ctrl->i2c_client;
	
	data[0] = (unsigned char)(halfword>>8);
	data[1] = (unsigned char)(0xff&halfword);
	
	act_dbg("set client->addr=0x%x [0x%x][0x%x, 0x%x]\n",client->addr, halfword, data[0], data[1]);
	
//	cci_write_a16_d8(act_ctrl->sd_act, 0x3333, data[0]);
//	cci_write_a16_d8(act_ctrl->sd_act, 0x4444, data[1]);
	
	return ret;
}

static int subdev_set_code(struct actuator_ctrl_t *act_ctrl,
													 unsigned short code,
													 unsigned short sr)
{
	int ret=0;
	unsigned short halfword;
	
	halfword = (code&0x3ff)<<4 | (sr&0xf);
	
	act_dbg("subdev_set_code[%x]\n",halfword);
	
	ret=subdev_i2c_write(act_ctrl, halfword, NULL);
	if(ret!=0)
		act_err("subdev set code err!\n");
	else
		act_ctrl->curr_code = code;
	
	return ret;
}

static int subdev_init_table(struct actuator_ctrl_t *act_ctrl,
														 unsigned short ext_tbl_en,
														 unsigned short ext_tbl_steps,
														 unsigned short * ext_tbl)
{
	int ret=0;
	unsigned int i;
	unsigned short *table;
	unsigned short table_size;
	
	act_dbg("subdev_init_table\n");
	
	if(ext_tbl_en==0)
	{
		//table = subdev_step_pos_table;
		//printk("sizeof(subdev_step_pos_table)=%d\n",sizeof(subdev_step_pos_table));
		table_size=2*TOTAL_STEPS_ACTIVE*sizeof(unsigned short);
		table = (unsigned short *)kmalloc(table_size, GFP_KERNEL);
		
		for(i=0;i<TOTAL_STEPS_ACTIVE;i+=1)
		{
			table[2*i]=table[2*i+1]=(unsigned short)(act_ctrl->active_min+
			(unsigned int)(act_ctrl->active_max-act_ctrl->active_min)*i/(TOTAL_STEPS_ACTIVE-1));
			
		}
		act_ctrl->total_steps=TOTAL_STEPS_ACTIVE;
	}
	else
	{
		table = (unsigned short *)kmalloc(2*ext_tbl_steps*sizeof(unsigned short), GFP_KERNEL);
		for(i=0;i<ext_tbl_steps;i+=1)
		{
			table[2*i]=ext_tbl[2*i];
			table[2*i+1]=ext_tbl[2*i+1];
		}
		act_ctrl->total_steps=ext_tbl_steps;
	}
	
	act_ctrl->step_position_table = table;
	for(i=0;i<TOTAL_STEPS_ACTIVE;i+=1)
	{
		act_dbg("TBL[%d]=%d [%d]=%d ",i+0, table[2*i+0], i+1, table[2*i+1]);
	}
	
	return ret;
}

static int subdev_move_pos(struct actuator_ctrl_t *act_ctrl,
													 unsigned short num_steps,
													 unsigned short dir)
{
	int ret = 0;
	char sign_dir = 0;
//	unsigned short index = 0;
	unsigned short target_pos = 0;
//	unsigned short target_code = 0;
	short dest_pos = 0;
	unsigned short curr_code = 0;
	act_dbg("%s called, dir %d, num_steps %d\n",
		__func__,
		dir,
		num_steps);

	/* Determine sign direction */
	if (dir == MOVE_NEAR)
		sign_dir = 1;
	else if (dir == MOVE_FAR)
		sign_dir = -1;
	else {
		act_err("Illegal focus direction\n");
		ret = -EINVAL;
		return ret;
	}

	/* Determine destination step position */
	dest_pos = act_ctrl->curr_pos +
		(sign_dir * num_steps);

	if (dest_pos < 0)
		dest_pos = 0;
	else if (dest_pos > act_ctrl->total_steps-1)
		dest_pos = act_ctrl->total_steps-1;

	if (dest_pos == act_ctrl->curr_pos)
		return ret;
	
	
	act_ctrl->work_status=ACT_STA_BUSY;
	
	curr_code = act_ctrl->step_position_table[dir+2*act_ctrl->curr_pos];
	
	act_dbg("curr_pos =%d dest_pos =%d curr_code=%d\n",
		act_ctrl->curr_pos, dest_pos, curr_code);
	
	target_pos =  act_ctrl->curr_pos;
	while (target_pos != dest_pos) {
		target_pos++;
		ret=subdev_set_code(act_ctrl,act_ctrl->step_position_table[dir+2*target_pos],0);
		if(ret==0)
		{
			usleep_range(1000,1200);;
			act_ctrl->curr_pos = target_pos;
		}
		else
		{
			break;
		}
	}

	act_ctrl->work_status=ACT_STA_IDLE;
	
	return ret;
}

static int subdev_set_pos(struct actuator_ctrl_t *act_ctrl,
													unsigned short pos)
{
	int ret = 0;
	unsigned short target_pos = 0;
	
	/* Determine destination step position */
	if(pos > act_ctrl->total_steps - 1)
		target_pos = act_ctrl->total_steps - 1;
	else 
		target_pos = pos;
	
	//act_dbg("subdev_set_pos[%d]\n",target_pos);
	
	if (target_pos == act_ctrl->curr_pos)
		return ret;
	
	ret=subdev_set_code(act_ctrl,act_ctrl->step_position_table[2*target_pos],0);
	if(ret==0)
	{
		usleep_range(1000,1200);;
		act_ctrl->curr_pos = target_pos;
	}
	else
	{
		act_err("act set pos err!");
	}

	return ret;
}

static int subdev_init(struct actuator_ctrl_t *act_ctrl,
											 struct actuator_para_t *a_para)
{
	int ret=0;
	struct actuator_para_t *para=a_para;
//	struct v4l2_subdev *sdev = &act_ctrl->sdev;
	act_dbg("act subdev_init\n");
  
  if(a_para==NULL)
  {
  	act_err("subdev_init para error\n");
  	ret = -1;
  	goto subdev_init_end;
  }
  
  act_ctrl->active_min=para->active_min;
  act_ctrl->active_max=para->active_max;
  
  //init_table
  subdev_init_table(act_ctrl, para->ext_tbl_en, para->ext_tbl_steps, para->ext_tbl);
	
	act_ctrl->curr_pos = 0;
	
  ret=subdev_i2c_write(act_ctrl,act_ctrl->active_min, NULL);
  
  if(ret==0)
  	act_ctrl->work_status=ACT_STA_IDLE;
	else
  	act_ctrl->work_status=ACT_STA_ERR;
	
subdev_init_end:
	return ret;
}

static int subdev_pwdn(struct actuator_ctrl_t *act_ctrl,
											 unsigned short mode)
{
	int ret=0;
//	struct v4l2_subdev *sdev = &act_ctrl->sdev;
  
  if(mode==1)
  {
		act_dbg("act subdev_pwdn %d\n",mode);
  	ret=subdev_i2c_write(act_ctrl,1<<15|0x7, NULL);
  	if(ret==0)
  		act_ctrl->work_status=ACT_STA_SOFT_PWDN;
  }
  else
  {
		act_dbg("act subdev_pwdn %d\n",mode);
  //if gpio control 
  //if(act_ctrl->af_io!=NULL)
  	//setgpio
  	ret=0;
  	act_ctrl->work_status=ACT_STA_HW_PWDN;
  }
  
	return ret;
}

static int subdev_release(struct actuator_ctrl_t *act_ctrl,
	                           	 struct actuator_ctrl_word_t *ctrlwd)
{
	int ret=0;
  
	act_dbg("act subdev_release to [%d], sr[%d]\n",ctrlwd->code,ctrlwd->sr);
	ret=subdev_i2c_write(act_ctrl,(ctrlwd->code<<4)|(ctrlwd->sr&0x0f), NULL);
	if(ret==0)
		act_ctrl->work_status=ACT_STA_IDLE;
  
	return ret;
}

static const struct i2c_device_id act_i2c_id[] = {
	{SUNXI_ACT_NAME, (kernel_ulong_t)&act_t},
	{ }
};

static const struct v4l2_subdev_core_ops sunxi_act_subdev_core_ops = {
	.ioctl = sunxi_actuator_ioctl,//extracted in actuator.c
};

static struct v4l2_subdev_ops act_subdev_ops = {
	.core = &sunxi_act_subdev_core_ops,
	//no other ops
};


static int act_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	v4l2_i2c_subdev_init(&act_t.sdev, client, &act_subdev_ops);
	
	//correct the fake id
	client->addr=SUNXI_ACT_ID>>1;
	
	act_t.i2c_client=client;
	act_t.work_status=ACT_STA_HW_PWDN;
	//add act other init para
	
	act_dbg("%s probe\n",SUNXI_ACT_NAME);
	return 0;
}

static int act_i2c_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
  v4l2_device_unregister_subdev(sd);
  return 0;
}

static struct i2c_driver act_i2c_driver = {
	.driver = {
	  .owner = THIS_MODULE,
		.name = SUNXI_ACT_NAME,
	},
	.id_table = act_i2c_id,
	.probe  = act_i2c_probe,//actuator_i2c_probe,//,
	.remove = act_i2c_remove,//actuator_i2c_remove,//,
};

static struct actuator_ctrl_t act_t = {
	.i2c_driver = &act_i2c_driver,
	//.i2c_addr = SUNXI_ACT_ID,
	//.sdev
	.sdev_ops = &act_subdev_ops,
	
	.set_info = {
		.total_steps = TOTAL_STEPS_ACTIVE,
	},

	.work_status = ACT_STA_HW_PWDN,
	.active_min = ACT_DEV_MIN_CODE,
	.active_max = ACT_DEV_MAX_CODE,
	
	.curr_pos = 0,
	//.curr_region_index = 0,
	//.initial_code = 0x0,
	.actuator_mutex = &act_mutex,

	.func_tbl = {
		//specific function
		.actuator_init = subdev_init,
		.actuator_pwdn = subdev_pwdn,
		.actuator_i2c_write = subdev_i2c_write,
		.actuator_release = subdev_release,
		.actuator_set_code = subdev_set_code,
		//common function
		.actuator_init_table = subdev_init_table,
		.actuator_move_pos = subdev_move_pos,
		.actuator_set_pos = subdev_set_pos,
	},

	.get_info = {//just for example
		.focal_length_num = 42,
		.focal_length_den = 10,
		.f_number_num = 265,
		.f_number_den = 100,
		.f_pix_num = 14,
		.f_pix_den = 10,
		.total_f_dist_num = 197681,
		.total_f_dist_den = 1000,
	},
};

static int __init act_mod_init(void)
{
  int ret=0;
	ret = i2c_add_driver(&act_i2c_driver);
	act_dbg("act_mod_init[%s]=%d\n", act_i2c_driver.driver.name, ret);
	return ret;
}

static void __exit act_mod_exit(void)
{
	act_dbg("act_mod_exit[%s]\n",act_i2c_driver.driver.name);
	i2c_del_driver(&act_i2c_driver);
}

module_init(act_mod_init);
module_exit(act_mod_exit);

MODULE_DESCRIPTION("ov8825 built-in vcm actuator");
MODULE_LICENSE("GPL v2");
