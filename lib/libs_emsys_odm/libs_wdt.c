/*
 * libs_wdt.c
 *
 *  Created on: 2011-2-16
 *      Author: Administrator
 */

#include "libs_emsys_odm.h"

#ifdef CFG_WDT_LIB

//*------------------------------------------------------------------------------------------------
//* 函数名称 ： Enable_WatchDog
//* 功能描述 ：使能看门狗设备
//* 入口参数 : <dev>[in]  指定看门狗的设备文件名,即"/dev/watchdog"
//*                     : <TimeOut>[in] 指定看门狗的最大喂狗间隔
//* 返回值 ：成功返回设备文件句柄(>=0)；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
Enable_WatchDog(char *dev, int TimeOut)
{
	int fd = -1;

	int cmd = WDIOS_DISABLECARD;

	if (TimeOut <= 0)
		return -1;

	fd = open("/dev/watchdog", O_RDWR);
	if (fd < 0)
		return -1;

	//set timeout
	if (ioctl(fd, WDIOC_SETTIMEOUT, &TimeOut)) {
		close(fd);
		return -1;
	}

	return (fd);
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ： Feed_WatchDog
//* 功能描述 ： 喂狗操作
//* 入口参数 : <fd>[in]  看门狗的设备句柄
//* 返回值 ： 成功返回ERROR_OK(==0)；失败返回ERROR_FAIL(-1)
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int Feed_WatchDog(int fd)
{
	if (ioctl(fd, WDIOC_KEEPALIVE, 0))
		return -1;
	else
		return 0;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ： Disable_WatchDog
//* 功能描述 ： 关闭看门狗
//* 入口参数 : <fd>[in]  看门狗的设备句柄
//* 返回值： 成功返回0；失败返回-1
//* 备  注 ：   内部看门狗不支持关闭功能
//*------------------------------------------------------------------------------------------------
int
Disable_WatchDog(int fd)
{
	int cmd = WDIOS_DISABLECARD;

	if (ioctl(fd, WDIOC_SETOPTIONS, &cmd))
		return -1;

	close(fd);

	return 0;

}

#endif
