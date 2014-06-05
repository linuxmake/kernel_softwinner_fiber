#
# Makefile for the touchscreen drivers.
#

# Each configuration option enables a list of files.


obj-$(CONFIG_TOUCHSCREEN_SUN6I_TS)      += sun6i-ts.o



##########################inet_ctp#######################
obj-m					+= inet_ctp.o
obj-$(CONFIG_TOUCHSCREEN_FT5X_TS)       += ft5x06_ts.o

obj-m      += ft5x02.o
#ft5x02-objs := ft5x02_ts.o ft5x02_config.o ini.o

obj-m       += ft5402.o
ft5402-objs := ft5402_ts.o ft5402_config.o ft5402_ini.o

obj-m      += sis925x.o
#sis925x-objs := ssd253x-ts.o

obj-m      += gt801_2plus1.o
obj-m      += byd693x-ts.o
obj-m      += ct36x_ts.o
obj-m      += zet622x.o
obj-m      += gslX680.o
obj-m      += gt9xx.o
gt9xx-objs := gt9xx_ts.o gt9xx_tool.o gt9xx_update.o

obj-m       += gslX680OGS.o
gslX680OGS-objs := gslX680_OGS.o gsl_point_id_20130415 gsl_point_id_20131106


obj-m       += elan_ts.o

obj-m       += elan_ts_32XX.o


