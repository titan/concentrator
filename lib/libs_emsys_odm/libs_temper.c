/*
 * libs_temper.c
 *
 *  Created on: 2011-2-16
 *      Author: Administrator
 */

#include "libs_emsys_odm.h"

#ifdef CFG_TEMPER_LIB

//*------------------------------------------------------------------------------------------------
//* 函数名称 : OpenThermometer
//* 功能描述 : 打开串行传感器设备
//* 入口参数 : <dev>[in]  指定串行ID(DI)的设备文件名
//*                     : <mode>[in] 指定打开设备时的操作标识
//*                     :               读模式：O_RDONLY
//*                     :               写模式：O_WRONLY
//*                     :               读写模式：O_RDWR
//* 返回值 : 成功返回设备文件句柄(>=0)；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
OpenThermometer(char *dev, int mode)
{

	int fd = -1;

	fd = open(dev, mode);
	if (fd < 0)
		return -1;

	return fd;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 : SetTemperatureCfg
//* 功能描述 : 配置温度传感器的设置
//* 入口参数 : <fd>[in]  指定传感器设备的句柄(即OpenThermometer()的返回值)
//*                     : <cmd>[in] 指定针对传感器设备的CFG指令
//*                     :               TM_SET_RESOLUTION：设定分辨率
//*                     :               TM_SET_MAXLIM:  设定报警最高限值
//*                     :               TM_SET_MINLIM：   设定报警最低限值
//*                     : <buf>[in] 传感器设备配置指令的参数缓冲区
//*                     : <len>[in]    传感器设备配置指令的参数长度
//* 返回值 : 返回操作状态信息(0:成功, -1:失败)
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int SetTemperatureCfg(int fd, unsigned int cmd, char *buf,
		   unsigned int *len)
{
	int err = 0;
	unsigned char value=0,res=0;

	switch (cmd) {

	case TM_SET_RESOLUTION:
		value = (*(unsigned char *) buf);
		switch(value)
		{
			case 9:
				res = 0x0;
			break;

			case 10:
				res = 0x1;
			break;

			case 11:
				res = 0x2;
			break;

			case 12:
				res = 0x3;
			break;

			default:
				return -1;
		}
		err = ioctl(fd, TM_SET_RESOLUTION, res);
		*len = 1;
		break;

	//unsupported now
	case TM_SET_MAXLIM:
	case TM_SET_MINLIM:
		return -1;

	default:
		return -1;

	}

	return err;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 : ReadTemperature
//* 功能描述 : 读取传感器的温度值
//* 入口参数 : <fd>[in]  <fd>[in]  指定DO设备的句柄(即OpenThermometer()的返回值)
//*                     : <value>[in/out]  返回温度值
//* 返回值 : 成功返回0；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int ReadTemperature(int fd, float* value)
{
	int ret = 0;
	unsigned int cmd = TM_GET_RESOLUTION;
	unsigned char res=0;
	unsigned short val;
	float data=0.0;

	res = ioctl(fd, cmd, 0);

	if(read(fd,&val,sizeof(unsigned short))<0)
	{
		printf("Here is %s:%d\n\r",__FUNCTION__,__LINE__);
		return -1;
	}

	switch(res)
	{
		case 3:
			data += ((val & (0x1<<0)) * 0.0625);

		case 2:
			data += (((val & (0x1<<1))>>1) * 0.125);

		case 1:
			data += (((val & (0x1<<2)) >> 2) * 0.25);

		case 0:
			data += (((val & (0x1<<10)) >> 10) * 64.0) + (((val & (0x1<<9))>>9) * 32.0) + (((val & (0x1<<8))>>8) * 16.0)
			+ (((val & (0x1<<7))>>7) * 8.0) + (((val & (0x1<<6))>>6) * 4.0) +(((val & (0x1<<5))>>5) * 2.0) +
			(((val & (0x1<<4))>>4) * 1.0) +(((val & (0x1<<3))>>3) * 0.5);
		break;
	}


	*value = data;
	return ret;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 : CloseThermometer
//* 功能描述 : 关闭串行传感器设备
//* 入口参数 : <fd>[in]  指定传感器的句柄(即OpenThermometer()的返回值)
//* 返回值 : 成功返回0；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
CloseThermometer(int fd)
{
	if (close(fd) < 0)
		return -1;
	else
		return 0;
}

#endif
