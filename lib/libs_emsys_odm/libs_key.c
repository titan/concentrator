/*
 * libs_key.c
 *
 *  Created on: 2011-2-17
 *      Author: casiawu@gmail.com
 *		   www.emsyschina.com
 */

#include "libs_emsys_odm.h"

#ifdef CFG_KBD_LIB

//*------------------------------------------------------------------------------------------------
//* 函数名称 ： OpenKbd
//* 功能描述 ： 打开键盘设备
//* 入口参数 : <dev>[in]  指定打开的键盘的设备文件名
//*                     : <mode>[in] 指定打开设备时的操作标识
//*                     :               读模式：O_RDONLY
//*                     :                               写模式：O_WRONLY
//*                     :               读写模式：O_RDWR
//* 返回值 ：成功返回设备文件句柄(>=0)；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
OpenKbd(char *dev, int mode)
{

	int fd = -1;

	//打开设备文件；
	if ((fd = open(dev, mode)) < 0) {
		printf("open KBD error\n\r");
		return -1;
	}

	return fd;

}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ： FlushKbd
//* 功能描述 ： 清空键盘缓冲区
//* 入口参数 : <fd>[in]  指定KBD设备的句柄(即OpenKbd()的返回值)
//* 返回值：成功返回0；无按键值或者失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
FlushKbd(int fd)
{

	int err = 0;

	unsigned char keys = 0;

	while (read(fd, &keys, sizeof (unsigned char)) > 0) ;

	return err;

}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ： GetKeyValue
//* 功能描述 ： 读取键盘值
//* 入口参数 : <fd>[in]  指定KBD设备的句柄(即OpenKbd()的返回值)
//*                     : <keyValue>[in/out] 返回按键值
//*                     : <status>[in/out]   被返回按键的操作状态(UP/DOWN)
//* 返回值 ： 成功返回(==0)；无按键值或者失败返回(-1)
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
GetKeyValue(int fd, unsigned short *keyValue, unsigned char *status)
{

	int err = 0;

	unsigned char keys = 0;

	if (read(fd, &keys, sizeof (unsigned char)) < 0)
		return -1;

	if (keys > UP_THRESHOLD_DOWN) {

		*keyValue = keys - UP_THRESHOLD_DOWN;

		*status = UP_KEY;

	}

	else {

		*keyValue = keys;

		*status = DOWN_KEY;

	}

	return err;

}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ：CloseKbd
//* 功能描述 ：关闭键盘设备
//* 入口参数 : <fd>[in]  指定KBD设备的句柄(即OpenKbd()的返回值)
//* 返回值 ： 成功返回0；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
CloseKbd(int fd)
{
	//刷新缓冲区键值
	FlushKbd(fd);

	//关闭设备文件；
	close(fd);
	return 0;

}

#endif
