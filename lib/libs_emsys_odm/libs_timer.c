/*
 * libs_timer.c
 *
 *  Created on: 2011-2-16
 *      Author: Administrator
 */

#include "libs_emsys_odm.h"

#ifdef CFG_TIMER_LIB

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：GreateTimer；
//*     功能描述  ：创建定时器
//* 入口参数 : <handler>[in] 定时器的句柄函数(即周期性执行函数)
//*                     : <args>[in]    传递给句柄函数的参数（不可用，为兼容而保留）
//*                     : <usecs>[in]   指定定时间隔，单位: useconds(微秒)
//*                     : <mode>[in]    指定定时器的触发模式
//*             :                                SINSHOT_MODE   :    单次触发
//*             :                                PERSHOT_MODE   :    周期性触发
//* 返回值 : 返回操作状态信息(0:成功， -1：失败)
//*     备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
CreateTimer(FUNC handler, void *args, unsigned int usecs, unsigned char mode)
{
	struct sigaction act;
	struct itimerval value;

	int secs = 0, usec = 0;
	if (usecs <= 0)
		return -1;

	secs = usecs / 1000000;
	usec = usecs % 1000000;
	act.sa_handler = handler;
	act.sa_flags = 0;

	sigemptyset(&act.sa_mask);
	sigaction(SIGALRM, &act, NULL);
	value.it_value.tv_sec = secs;
	value.it_value.tv_usec = usec;

	if (mode == PERSHOT_MODE)
		value.it_interval = value.it_value;
	else {
		value.it_interval.tv_sec = 0;
		value.it_value.tv_usec = 0;
	}

	//启动定时器
	setitimer(ITIMER_REAL, &value, NULL);

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：Usermsdelay；
//*     功能描述  ：用户层延时函数（相对精确）
//*     入口参数 : <millsecs>[in] 延时大小(单位：毫秒ms)
//* 	返回值 : 延时时刻到成功返回0，否则-1
//*     备  注 ：   无
//*------------------------------------------------------------------------------------------------
int Usermsdelay(unsigned int millsecs)
{
	struct timeval tv;

	tv.tv_sec = millsecs/1000;
	tv.tv_usec = (millsecs % 1000) * 1000;

	if(select (0, NULL, NULL, NULL, &tv) == -1)
	{
		return -1;
	}

	return 0;
}

#endif
