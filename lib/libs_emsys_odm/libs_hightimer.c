/*
 * libs_hightimer.c
 *
 *  Created on: 2011-2-16
 *      Author: Administrator
 */

#include "libs_emsys_odm.h"

#ifdef CFG_HIGHTIMER_LIB

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：GreateHighResTimer；
//*     功能描述  ：创建定时器
//* 入口参数 : <handler>[in] 定时器的句柄函数(即定时器执行函数)
//*                     : <timerid>[in]    高精度定时器句柄指针（供删除定时器时使用）
//*                     : <nsecs>[in]   指定定时间隔，单位: neconds(纳秒)
//*                     : <mode>[in]    指定定时器的触发模式
//*             :                                HIGHSINSHOT_MODE   :    单次触发
//*             :                                HIGHSPERSHOT_MODE   :    周期性触发
//* 返回值 : 返回操作状态信息(成功0/失败-1)
//*     备  注 ：   无
//*------------------------------------------------------------------------------------------------
int GreateHighResTimer(HFUNC handler, timer_t *timerid, unsigned int nsecs,unsigned char mode)
{
    struct sigevent seg;
    struct itimerspec its;
    sigset_t mask;
    struct sigaction act;
    struct timespec ts;
	int secs = 0, nsec = 0;

	if (nsecs <= 0)
		return -1;
	secs = nsecs / 1000000000;
	nsec = nsecs % 1000000000;

	//检测系统的时钟分辨率
	if (clock_getres(CLOCK_MONOTONIC, &ts))
		return -1;

	//如果不支持高分辨定时器，则返回
	if(ts.tv_sec != 0 || ts.tv_nsec != 1)
		return -1;

	//
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = handler;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGRTMIN, &act, NULL) == -1)
    	return -1;
    //
    sigemptyset(&mask);
    sigaddset(&mask, SIGRTMIN);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
    	return -1;

	//
	seg.sigev_notify = SIGEV_SIGNAL;
	seg.sigev_signo = SIGRTMIN;
	seg.sigev_value.sival_ptr = (void *) timerid;
	if(timer_create(CLOCK_MONOTONIC, &seg, timerid))
		return -1;

	//启动时钟
    its.it_value.tv_sec = secs;
    its.it_value.tv_nsec = nsec;
	if (mode == HIGHSPERSHOT_MODE)
	{
	    its.it_interval.tv_sec = its.it_value.tv_sec;
	    its.it_interval.tv_nsec = its.it_value.tv_nsec;
	}
	else
	{
	    its.it_interval.tv_sec = 0;
	    its.it_interval.tv_nsec = 0;
	}

    if (timer_settime((*timerid), 0, &its, NULL) == -1)
    	return -1;

	if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
		return -1;

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：DelHighResTimer；
//*     功能描述  ：用户层延时函数（相对精确）
//* 入口参数 : <timerid>[in]    高精度定时器句柄指针（由GreateHighResTimer返回）
//*
//* 返回值 : 返回操作状态信息(成功0/失败-1)
//*     备  注 ：   参数一定是GreateHighResTimer返回的定时器句柄
//*------------------------------------------------------------------------------------------------
int DelHighResTimer(timer_t timerid)
{
	if(timerid == NULL)
		return -1;

	return timer_delete(timerid);
}


//*------------------------------------------------------------------------------------------------
//*     函数名称  ：Userusdelay；
//*     功能描述  ：用户层延时函数（相对精确）
//*     入口参数 : <usecs>[in] 延时大小(单位：微秒us)
//* 	返回值 : 延时时刻到成功返回0，否则-1
//*     备  注 ：   无
//*------------------------------------------------------------------------------------------------
int Userusdelay(unsigned int usecs)
{
	struct timespec tv;

	tv.tv_sec = (time_t)(usecs / 1000000);
	tv.tv_nsec = (long)((usecs % 1000000)*1000);

	while (1)
	{
		int rval = nanosleep (&tv, &tv);
		if (rval == 0)
	      return 0;
		else if (errno == EINTR)
			continue;
	    else
	      return -1;
	}

	return 0;
}

#endif
