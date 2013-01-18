/*
 * libs_io.c
 *
 *  Created on: 2011-2-15
 *      Author: Administrator
 */

#include "libs_emsys_odm.h"

#ifdef CFG_GPIO_LIB

#ifdef EMSYS_AT91RM9200
#define PIO_BASE_ADDR (0xFFFFF400)
#elif defined(EMSYS_AT91SAM9260)
#define PIO_BASE_ADDR (0xFFFFF400)
#elif defined(EMSYS_AT91SAM9263)
#define PIO_BASE_ADDR (0xFFFFF200)
#endif

static int _gfilpfd = -1;
static unsigned long _gmap_base = 0;

//GPIO寄存器偏移
#define PER_OFFSET	(0x0)
#define PDR_OFFSET	(0x4)
#define OER_OFFSET	(0x10)
#define ODR_OFFSET	(0x14)
#define OSR_OFFSET	(0x18)
#define IFER_OFFSET	(0x20)
#define IFDR_OFFSET	(0x24)
#define SODR_OFFSET	(0x30)
#define CODR_OFFSET	(0x34)
#define ODSR_OFFSET (0x38)
#define PDSR_OFFSET (0x3c)
#define MDER_OFFSET (0x50)
#define MDDR_OFFSET (0x54)
#define PPUER_OFFSET (0x64)
#define PPUDR_OFFSET (0x60)
#define ASR_OFFSET 	(0x70)
#define BSR_OFFSET 	(0x74)

//*------------------------------------------------------------------------------------------------
//*     函数名称：SetPIOCfg；
//*     功能描述：设置GPIO的端口属性；
//* 入口参数 : < attr >[in]  指定GPIO的属性（模式、内部电阻状态、滤波器等等）
//*     返回值  ：返回操作状态信息(0:成功；-1:失败)
//*     备  注 ：  无
//*------------------------------------------------------------------------------------------------
int SetPIOCfg(gpio_name_t pio, gpio_attr_t attr)
{
	unsigned char bank=0,bits=0;
	unsigned char*start=NULL;
	off_t addr, page;
	unsigned long map_ioaddr = 0;

	bank = pio / 32;
	bits = pio % 32;

#ifdef EMSYS_AT91RM9200
	if(bank > 3)
		return -1;
#elif defined(EMSYS_AT91SAM9260)
	if(bank > 2)
		return -1;
#elif defined(EMSYS_AT91SAM9263)
	if(bank > 4)
		return -1;
#endif

	//
	if(_gfilpfd <0)
	{
		_gfilpfd = open("/dev/mem", O_RDWR|O_SYNC);
		if (_gfilpfd < 0) {
			printf("open /dev/mem error");
			return -1;
		}

		addr = PIO_BASE_ADDR;
		page = addr & 0xfffff000;
		start = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, _gfilpfd, page);
		if (start == MAP_FAILED) {
			perror("mmap:");
			return -1;
		}

		_gmap_base = (unsigned long)start + (addr & 0xfff);
	}

	map_ioaddr = _gmap_base + bank * 0x200;


	//设置端口
	switch(attr.mode)
	{
		case PIO_MODE_IN:
			_outl((map_ioaddr+PER_OFFSET),(0x1<<bits));
			_outl((map_ioaddr+ODR_OFFSET),(0x1<<bits));
		break;

		case PIO_MODE_A:
			_outl((map_ioaddr+PDR_OFFSET),(0x1<<bits));
			_outl((map_ioaddr+ASR_OFFSET),(0x1<<bits));
		break;

		case PIO_MODE_B:
			_outl((map_ioaddr+PDR_OFFSET),(0x1<<bits));
			_outl((map_ioaddr+BSR_OFFSET),(0x1<<bits));
		break;

		default:
		case PIO_MODE_OUT:
			_outl((map_ioaddr+PER_OFFSET),(0x1<<bits));
			_outl((map_ioaddr+OER_OFFSET),(0x1<<bits));
		break;
	}

	switch(attr.resis)
	{
		case PIO_RESISTOR_PULLUP:
			_outl((map_ioaddr+PPUER_OFFSET),(0x1<<bits));
		break;

		case PIO_RESISTOR_DOWN:
			_outl((map_ioaddr+PPUDR_OFFSET),(0x1<<bits));
		break;

		default:
		break;
	}

	switch(attr.filter)
	{
		case PIO_FILTER_DISABLED:
			_outl((map_ioaddr+IFDR_OFFSET),(0x1<<bits));
		break;

		case PIO_FILTER_ENABLED:
			_outl((map_ioaddr+IFER_OFFSET),(0x1<<bits));
		break;

		default:
		break;
	}


	switch(attr.multer)
	{
		case PIO_MULDRIVER_DISABLED:
			_outl((map_ioaddr+MDDR_OFFSET),(0x1<<bits));
		break;

		case PIO_MULDRIVER_ENABLED:
			_outl((map_ioaddr+MDER_OFFSET),(0x1<<bits));
		break;

		default:
		break;
	}

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称：PIOOutValue；
//*     功能描述：GPIO端口输出；
//* 入口参数 : <pio>[in]  指定GPIO端口
//*                     : <val>[in] 指定输出状态
//*     返回值  ：操作状态(0:成功，-1：失败)
//*------------------------------------------------------------------------------------------------
int PIOOutValue(gpio_name_t pio, char val)
{
	unsigned char bank=0,bits=0;
	unsigned long map_ioaddr = 0;

	bank = pio / 32;
	bits = pio % 32;

#ifdef EMSYS_AT91RM9200
	if(bank > 3)
		return -1;
#elif defined(EMSYS_AT91SAM9260)
	if(bank > 2)
		return -1;
#elif defined(EMSYS_AT91SAM9263)
	if(bank > 4)
		return -1;
#endif

	if(_gfilpfd <0)
		return -1;

	map_ioaddr = _gmap_base + bank * 0x200;


	if(val)
		_outl((map_ioaddr+SODR_OFFSET),(0x1<<bits));
	else
		_outl((map_ioaddr+CODR_OFFSET),(0x1<<bits));

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称：PIOInValue；
//*     功能描述：读GPIO端口输入；
//* 入口参数 : <pio>[in]  指定GPIO端口
//*     返回值  ：返回GPIO端口状态（0/1：成功，-1：失败）
//*------------------------------------------------------------------------------------------------
int PIOInValue(gpio_name_t pio)
{
	unsigned char bank=0,bits=0;
	unsigned long map_ioaddr = 0;
	int res=0;

	bank = pio / 32;
	bits = pio % 32;

#ifdef EMSYS_AT91RM9200
	if(bank > 3)
		return -1;
#elif defined(EMSYS_AT91SAM9260)
	if(bank > 2)
		return -1;
#elif defined(EMSYS_AT91SAM9263)
	if(bank > 4)
		return -1;
#endif

	if(_gfilpfd <0)
		return -1;

	map_ioaddr = _gmap_base + bank * 0x200;
	//输出IO
	if(_inl(map_ioaddr+OSR_OFFSET) & (0x1<<bits))
		res = (_inl(map_ioaddr+ODSR_OFFSET) & (0x1<<bits)) >> bits ;
	//输入
	else
		res = (_inl(map_ioaddr+PDSR_OFFSET) & (0x1<<bits)) >> bits ;

	return res;
}

#endif
