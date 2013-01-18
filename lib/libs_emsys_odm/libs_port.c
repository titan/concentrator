/*
 * libs_port.c
 *
 *  Created on: 2011-2-15
 *      Author: Administrator
 */

#include "libs_emsys_odm.h"

#ifdef CFG_PORT_LIB
//*------------------------------------------------------------------------------------------------
//*     函数名称  ：    OpenPortDev；
//*     功能描述  ：    打开端口设备；
//*     入口参数  ：    <dev>[in]  - 端口设备名
//*     返回值  ：      成功返回设备句柄fd(>=0)， 失败返回-1；
//*     备  注 ：               无
//*------------------------------------------------------------------------------------------------
int OpenPortDev(char *dev)
{
	int fd = -1;

	//打开设备文件
	if ((fd = open(dev, O_RDWR)) < 0) {
		printf("open %s error\n\r", dev);
		return -1;
	}

	return fd;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ：ClosePortDev
//* 功能描述 ：关闭总线端口设备
//* 入口参数 : <fd>[in]  指定总线端口设备的句柄(即ClosePortDev()的返回值)
//* 返回值 ： 成功返回0；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
ClosePortDev(int fd)
{
	//关闭设备文件；
	close(fd);
	return 0;

}

//*------------------------------------------------------------------------------------------------
//*     函数名称：SetPortCfg；
//*     功能描述：设置端口属性；
//* 入口参数 : <fd>[in]  指定端口设备的句柄(即OpenPortDev()的返回值)
//*                     : <bank>[in] 指定端口Bank配置
//*                     : <width>[in] 指定端口读写宽度(8/16/32)
//*     返回值  ：返回操作状态信息(0:成功；-1:失败)
//*     备  注 ：   端口的Bank由硬件配置决定
//*------------------------------------------------------------------------------------------------
int SetPortCfg(int fd, unsigned char bank, unsigned char width)
{
	port_attr_t pattr;

	switch(bank)
	{
		case 2:
			pattr.bank = PORT_BANK_2;
		break;

		case 4:
			pattr.bank = PORT_BANK_4;
		break;

		case 5:
			pattr.bank = PORT_BANK_5;
		break;

		case 6:
			pattr.bank = PORT_BANK_6;
		break;

		default:
		case 7:
			pattr.bank = PORT_BANK_7;
		break;
	}

	switch(width)
	{
		case 8:
			pattr.width = PORT_WIDTH_8;
		break;

		case 32:
			pattr.width = PORT_WIDTH_32;
		break;

		case 16:
			pattr.width = PORT_WIDTH_16;
		default:
		break;
	}

	if(ioctl(fd,IOCTL_PORT_SET_ATTR,&pattr))
		return -1;

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称：GetPortCfg；
//*     功能描述：获取端口属性；
//* 入口参数 : <fd>[in]  指定端口设备的句柄(即OpenPortDev()的返回值)
//*                     : <bank>[in] 指定端口Bank配置
//*                     : <width>[in] 指定端口读写宽度(8/16/32)
//*     返回值  ：返回操作状态信息(0:成功；-1:失败)
//*     备  注 ：   端口的Bank由硬件配置决定
//*------------------------------------------------------------------------------------------------
int GetPortCfg(int fd, unsigned char *bank, unsigned char *width)
{
	port_attr_t pattr;

	if(ioctl(fd,IOCTL_PORT_GET_ATTR,&pattr))
		return -1;

	switch(pattr.bank)
	{
		case PORT_BANK_2:
			*bank = 2;
		break;

		case PORT_BANK_4:
			*bank = 4;
		break;

		case PORT_BANK_5:
			*bank = 5;
		break;

		case PORT_BANK_6:
			*bank = 6;
		break;

		default:
		case PORT_BANK_7:
			*bank = 7;
		break;
	}

	switch(pattr.width)
	{
		case 8:
			*width = 8;
		break;

		case 32:
			*width = 32;
		break;

		case 16:
			*width = 16;
		default:
		break;
	}

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称：InPortw；
//*     功能描述：16-bit读端口；
//* 入口参数 : <fd>[in]  指定端口设备的句柄(即OpenPortDev()的返回值)
//*                     : <addr>[in] 指定读取的端口地址
//*     返回值  ：返回端口当前值(16-bit),(-1:失败)
//*------------------------------------------------------------------------------------------------
inline short InPortw(int fd, unsigned long addr)
{
	port_data_t pdata;

    pdata.regAddr = addr & 0xFFFFFFFE;
	if(ioctl(fd,IOCTL_PORT_READ,&pdata))
		return -1;
    return (pdata.regData&0xFFFF);
}

//*------------------------------------------------------------------------------------------------
//*     函数名称：OutPortw；
//*     功能描述：16-bit写端口；
//* 入口参数 : <fd>[in]  指定端口设备的句柄(即OpenPortDev()的返回值)
//*                     : <addr>[in] 指定读取的端口地址
//*                     : <data>[in] 写数据
//*     返回值  ：返回状态(0：成功，-1:失败)
//*------------------------------------------------------------------------------------------------
inline int OutPortw(int fd, unsigned long addr, short data)
{
	port_data_t pdata;

    pdata.regAddr = addr & 0xFFFFFFFE;
    pdata.regData = data;

	if(ioctl(fd,IOCTL_PORT_WRITE,&pdata))
		return -1;

    return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称：InPortl；
//*     功能描述：32-bit读端口；
//* 入口参数 : <fd>[in]  指定端口设备的句柄(即OpenPortDev()的返回值)
//*                     : <addr>[in] 指定读取的端口地址
//*     返回值  ：返回端口当前值(16-bit),(-1:失败)
//*------------------------------------------------------------------------------------------------
inline int InPortl(int fd, unsigned long addr)
{
	port_data_t pdata;

    pdata.regAddr = addr & 0xFFFFFFFF;

	if(ioctl(fd,IOCTL_PORT_READ,&pdata))
		return -1;

    return (pdata.regData&0xFFFFFFFF);
}

//*------------------------------------------------------------------------------------------------
//*     函数名称：OutPortw；
//*     功能描述：16-bit写端口；
//* 入口参数 : <fd>[in]  指定端口设备的句柄(即OpenPortDev()的返回值)
//*                     : <addr>[in] 指定读取的端口地址
//*                     : <data>[in] 写数据
//*     返回值  ：返回状态(0：成功，-1:失败)
//*------------------------------------------------------------------------------------------------
inline int OutPortl(int fd, unsigned long addr, int data)
{
	port_data_t pdata;

    pdata.regAddr = addr & 0xFFFFFFFF;
    pdata.regData = data;

	if(ioctl(fd,IOCTL_PORT_WRITE,&pdata))
		return -1;

    return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称：InPortw；
//*     功能描述：16-bit读端口；
//* 入口参数 : <fd>[in]  指定端口设备的句柄(即OpenPortDev()的返回值)
//*                     : <addr>[in] 指定读取的端口地址
//*     返回值  ：返回端口当前值(16-bit),(-1:失败)
//*------------------------------------------------------------------------------------------------
inline char InPortb(int fd, unsigned long addr)
{
	port_data_t pdata;

    pdata.regAddr = addr & 0xFFFFFFFC;
	if(ioctl(fd,IOCTL_PORT_READ,&pdata))
		return -1;
    return (pdata.regData&0xFF);
}

//*------------------------------------------------------------------------------------------------
//*     函数名称：OutPortw；
//*     功能描述：16-bit写端口；
//* 入口参数 : <fd>[in]  指定端口设备的句柄(即OpenPortDev()的返回值)
//*                     : <addr>[in] 指定读取的端口地址
//*                     : <data>[in] 写数据
//*     返回值  ：返回状态(0：成功，-1:失败)
//*------------------------------------------------------------------------------------------------
inline int OutPortb(int fd, unsigned long addr, char data)
{
	port_data_t pdata;

    pdata.regAddr = addr & 0xFFFFFFFC;
    pdata.regData = data;

	if(ioctl(fd,IOCTL_PORT_WRITE,&pdata))
		return -1;

    return 0;
}

#endif
