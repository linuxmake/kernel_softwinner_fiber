/*
 * header file
 * set/get config from sysconfig and ini/bin file
 * Author: raymonxiu
 * 
 */
 
#ifndef __CONFIG__H__
#define __CONFIG__H__

#include "vfe.h"

int fetch_config(struct vfe_dev *dev);
int read_ini_info(struct vfe_dev *dev);
int fetch_inet_config(struct vfe_dev *dev);//gongpiqiang+++
#endif //__CONFIG__H__