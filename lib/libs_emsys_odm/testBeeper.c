/*
 * testBeeper.c
 *
 *  Created on: 2011-2-16
 *      Author: casiawu@gmail.com
 *		   www.emsyschina.com
 */

//使用库的IO端口函数，需定义如下
#ifndef CFG_GPIO_LIB
#define CFG_GPIO_LIB 1
#endif

#include "libs_emsys_odm.h"


//BEEP连接的IO端口
#define BEEPIO			PB10

int main(int argc,char **argv)
{
	gpio_attr_t attr;
	int ch=0;

	//IO端口配置为输出模式
	attr.mode = PIO_MODE_OUT;
	SetPIOCfg(BEEPIO,attr);

	printf("Please input enter key to continue\n\r");
	while ((ch = getchar()) != '\n') ;
	while(1)
	{
		//打开
		PIOOutValue(BEEPIO,1);
		sleep(1);
		//关闭
		PIOOutValue(BEEPIO,0);
		sleep(1);
	}

	PIOOutValue(BEEPIO,0);

	return 0;
}
