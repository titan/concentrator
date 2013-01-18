/*
 * libs_dac.c
 *
 *  Created on: 2011-2-15
 *      Author: Administrator
 */

#include "libs_emsys_odm.h"

#ifdef CFG_DAC_LIB

#if 1
//DA输出的LED指示
#define	LEDDA0		PA16
#define	LEDDA1		PA17
#define	LEDDA2		PA18
#define	LEDDA3		PA19
static unsigned int _gLedDAS[TOTALDACCHAN];
#endif

//DA的PIN
#define CSPIN		PD6
#define SCKPIN		PD7
#define SDIPIN		PD8
#define SDOPIN		PD9


//DA初始化指令
#define ENABLEEXTREF			(0x7F<<16 | 0x0000)
//
#define WRUPDATA0				(0x30<<16)
#define WRUPDATA1				(0x31<<16)
#define WRUPDATA2				(0x32<<16)
#define WRUPDATA3				(0x33<<16)


//内部函数
static void WriteCmdtoDAC(int Cmd)
{
	int i=0;

	PIOOutValue(CSPIN,0);
	PIOOutValue(SCKPIN,0);

	_nops(20);

	for(i=24;i>0;i--)
	{
		if(Cmd & 0x800000)
			PIOOutValue(SDIPIN,1);
		else
			PIOOutValue(SDIPIN,0);

		Cmd<<=1;
		_nops(5);
		PIOOutValue(SCKPIN,1);
		_nops(100);
		PIOOutValue(SCKPIN,0);
		_nops(100);
	}

	PIOOutValue(CSPIN,1);
}
//*------------------------------------------------------------------------------------------------
//* 函数名称 ： OpenDac
//* 功能描述 ：打开DAC设备
//* 入口参数 : <dev>[in]  指定打开的DAC的设备文件名，可置为NULL
//*                     : <mode>[in] 指定打开设备时的操作标识
//*                     :               读模式：O_RDONLY
//*                     :               写模式：O_WRONLY
//*                     :               读写模式：O_RDWR
//* 返回值 ： 成功返回设备文件句柄(>=0)；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
OpenDac(char *dev, int mode)
{
	gpio_attr_t attr;

	//IO端口配置
	//输出
	attr.mode = PIO_MODE_OUT;
	SetPIOCfg(CSPIN,attr);
	SetPIOCfg(SCKPIN,attr);
	SetPIOCfg(SDIPIN,attr);
	//输入
	attr.mode = PIO_MODE_IN;
	SetPIOCfg(SDOPIN,attr);
	//初始状态
	PIOOutValue(CSPIN,1);
	PIOOutValue(SCKPIN,0);
	PIOOutValue(SDIPIN,0);

#if 1
	//LED配置
	attr.mode = PIO_MODE_OUT;
	SetPIOCfg(LEDDA0,attr);
	SetPIOCfg(LEDDA1,attr);
	SetPIOCfg(LEDDA2,attr);
	SetPIOCfg(LEDDA3,attr);
	PIOOutValue(LEDDA0,0);
	PIOOutValue(LEDDA1,0);
	PIOOutValue(LEDDA2,0);
	PIOOutValue(LEDDA3,0);

	_gLedDAS[0] = LEDDA0;
	_gLedDAS[1] = LEDDA1;
	_gLedDAS[2] = LEDDA2;
	_gLedDAS[3] = LEDDA3;
#endif

	//初始化
	WriteCmdtoDAC(ENABLEEXTREF);

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：WriteDacVal；
//*     功能描述  ：向DAC输出数据；
//* 入口参数 : <fd>[in]  指定DAC的句柄(为保持兼容而保留，可直接置为0)
//*                     : <chan>[in] 指定输出的DA通道
//*                     : <value>[in] 指定输出的电压值(浮点数)
//*                     : <keys>[in]  转换参数表(如果keys==NULL,则直接将value值作为转换后的数字量输出)
//* 返回值 ：返回操作状态信息(0：成功， -1：失败)
//* 备  注 ：   1、转换关系表如下所示：
//*                                     struct __sampl_to_float keysvlaue[] = {
//*                                                             {3.32, 0xfff},
//*                                                             {0.02, 0x12} };
//*             上述数组为两组模拟量与相对应数字量数值，
//*                             根据芯片参数或者实际测量进行设定。
//*------------------------------------------------------------------------------------------------
int
WriteDacVal(int fd, unsigned char chan, float value,
	    struct __float_to_dacout * keys)
{
	cyg_dac_out daValue;
	float kval = 0.0, bval = 0.0, temp = 0.0;
	unsigned int __data=0;

	if(chan >= TOTALDACCHAN)
		return -1;

	if (keys == NULL) {
		daValue = (cyg_dac_out) value;
	} else {
		kval =
		    (keys[0].ivalue - keys[1].ivalue) / (keys[0].fvalue -
							 keys[1].fvalue);

		bval = keys[1].ivalue - kval * keys[1].fvalue;
		temp = kval * value + bval;
		daValue = (cyg_dac_out) temp;
	}

	switch(chan)
	{
		case 1:
			__data = WRUPDATA1 | (daValue & 0xFFFF);
		break;

		case 2:
			__data = WRUPDATA2 | (daValue & 0xFFFF) ;
		break;

		case 3:
			__data = WRUPDATA3 | (daValue & 0xFFFF) ;
		break;

		case 0:
			__data = WRUPDATA0 | (daValue & 0xFFFF) ;
		default:
		break;
	}

	WriteCmdtoDAC(__data);

#if 1
	//setup LED
	if(daValue>=0x2000)
		PIOOutValue(_gLedDAS[chan],1);
	else
		PIOOutValue(_gLedDAS[chan],0);
#endif

	return 0;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ：CloseDac
//* 功能描述 ：关闭DAC设备
//* 入口参数 : <fd>[in]  指定DAC的句柄(为保持兼容而保留，可直接置为0)
//* 返回值 ： 成功返回0；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int
CloseDac(int fd)
{
	gpio_attr_t attr;

	//置为输入状态
	attr.mode = PIO_MODE_IN;
	SetPIOCfg(CSPIN,attr);
	SetPIOCfg(SCKPIN,attr);
	SetPIOCfg(SDIPIN,attr);
	SetPIOCfg(SDOPIN,attr);

	return 0;
}

#endif
