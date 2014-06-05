#ifndef __SMART_COVER__H__
#define __SMART_COVER__H__

#define MAXNAME 16

struct cover_switch_info{
	int gpio;
	int virp;
	int frags;
	char *name;
	int flags;
	irqreturn_t (*handler)(int, void *);
};

#endif