/*
 * libs_adc.c
 *
 *  Created on: 2011-2-15
 *      Author: Administrator
 */

#include "libs_emsys_odm.h"

#ifdef CFG_ADC_LIB

//通道转换命令
unsigned char __ChanCmd[] = {
	0x8C, 0xCC, 0x9C, 0xDC,
	0xAC, 0xEC, 0xBC, 0xFC,
};
//*------------------------------------------------------------------------------------------------
//* 函数名称 ：   OpenAdc
//* 功能描述 ：   打开ADC设备
//* 入口参数 : <dev>[in]  指定打开的ADC的设备文件名
//*                     : <mode>[in] 指定打开设备时的操作标识
//*                     :               读模式：O_RDONLY
//*                     :                               写模式：O_WRONLY
//*                     :               读写模式：O_RDWR
//* 返回值：   成功返回设备文件句柄(>=0)；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int OpenAdc(char *dev, int mode)
{
	int fd = -1, ret=0;
	unsigned char spimode,bits;

	fd = open("/dev/spidev1.0", O_RDWR);
	if (fd < 0)
	{
		printf("can't open device");
		return -1;
	}
	//set mode
	spimode = SPI_CPHA;
	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &spimode);
	if (ret == -1)
	{
		printf("can't set spi mode");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_MODE, &spimode);
	if (ret == -1)
	{
		printf("can't get spi mode");
		return -1;
	}

	//set bits
	bits = 16;
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		printf("can't set bits per word");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		printf("can't get bits per word");
		return -1;
	}

	return fd;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ：  CloseAdc
//* 功能描述 ：   关闭ADC设备
//* 入口参数 : <fd>[in]  指定ADC设备的句柄(即OpenAdc()的返回值)
//* 返回值：  成功返回0；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int CloseAdc(int fd)
{
	//关闭设备文件；
	close(fd);
	return 0;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ： SetAdcCfg
//* 功能描述 ： 对ADC设备进行配置
//* 入口参数 : <fd>[in]  指定ADC设备的句柄(即OpenAdc()的返回值)
//*                     : <cmd>[in] 指定针对ADC设备的CFG指令
//*                     :               SETADC_START_IOCFG：启动ADC采样
//*                     :                               SETADC_STOP_IOCFG:  停止ADC采样
//*                     :               SETADC_RATE_IOCFG：   设置采样率
//*                     :                               SETADC_MODE_IOCFG:  设置采样模式
//*                     :               SETADC_ENCHAN_IOCFG：      使能指定的采样通道
//*                     :                               SETADC_DISCHAN_IOCFG:  无效指定的采样通道
//*                     : <buffer>[in] ADC配置指令的参数缓冲区
//*                     : <len>[in]    ADC配置指令的参数长度
//* 返回值 ：返回操作状态信息(ERROR_OK/ERROR_FAIL/ERROR_UNSUPPORT/)
//* 备  注 ：   1、采样模式支持如下三种：
//*       :                    TRIGER_ONESHOT：单次触发(即读一次ADC触发一次采样)
//*       :                   TRIGER_SOFTPERIOD：软件定时器触发周期性采样
//*       :                    TRIGER_HARDPERIOD：硬件定时器触发周期性采样
//*       :              2、规范的ADC的设置流程：
//*       :                    (1) 配置ADC的触发采样模式：
//*       :                    (2) 设置ADC的采样参数(包括采样率和使能采样通道)
//*       :                    (3) 启动ADC采样
//*       :                    (4) 读取采样值
//*------------------------------------------------------------------------------------------------
int SetAdcCfg(int fd, unsigned int cmd, char *buffer, unsigned int len)
{
	return 0;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ： GetAdcCfg
//* 功能描述 ： 得到ADC设备的当前配置
//* 入口参数 : <fd>[in]  指定ADC设备的句柄(即OpenAdc()的返回值)
//*                     : <cmd>[in] 针对ADC设备的CFG指令
//*                     :               GETADC_STAT_IOCFG：   采样通道的使能状态
//*                     :                               GETADC_RATE_IOCFG:  采样率
//*                     :               GETADC_MODE_IOCFG：   采样模式
//*                     : <buffer>[in/out] 返回当前配置的数值缓冲区
//*                     : <len>[in/out]    返回当前配置的数值长度
//* 返回值 ： 返回操作状态信息(ERROR_OK/ERROR_FAIL)
//* 备  注 ：   1、采样模式支持如下三种：
//*       :                    TRIGER_ONESHOT：单次触发(即读一次ADC触发一次采样)
//*       :                    TRIGER_SOFTPERIOD：软件定时器触发周期性采样
//*       :                    TRIGER_HARDPERIOD：硬件定时器触发周期性采样
//*------------------------------------------------------------------------------------------------
int GetAdcCfg(int fd, unsigned int cmd, char *buffer, unsigned int *len)
{
	return 0;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ： ReadAdcSampling
//* 功能描述 ： 读ADC的采样值
//* 入口参数 : <fd>[in]  指定ADC设备的句柄(即OpenAdc()的返回值)
//*                     : <channel>[in] 指定需要读取的采样通道号
//*                     :               指定通道号>0: 读取指定的某单一通道
//*                     :                               指定通道号=0: 读取所有通道
//*                     : <buffer>[in/out] 读取的采样值缓冲区
//*                     : <len>[in/out]    指定读取的采样值数，并返回实际读取的采样值数目
//*                     :                      读取单一通道时，len指定读取该通道的总采样值个数
//*                     :                      同时读取所有通道时，len指定针对每一通道的采样值个数
//*                     : <timesout>[in]   目前不支持
//* 返回值 ： 返回操作状态信息(0:成功， -1：失败)
//* 备  注 ：   1、分配采样值存放缓冲区时，一定要足够存放指定的采样值个数：
//*         :            读取单一通道时： 缓冲区大小 >=  len * sizeof(cyg_adc_sample)
//*     :              读取所有通道时： 缓冲区大小 >=  TOTALCHAN * len * sizeof(cyg_adc_sample)
//*------------------------------------------------------------------------------------------------
int ReadAdcSampling(int fd, unsigned int channel,
		cyg_adc_sample * buffer, unsigned int *len, int timesout)
{
	int err = 0;
	unsigned int length = 0, ret = 0, i = 0,j=0;
	struct spi_ioc_transfer	xfer;
	unsigned char	wrbuf[2],rdbuf[2];

	if (!buffer)
		return -1;

	if(channel>TOTALADCCHAN)
		return -1;

	length = (*len) ;
	memset(&xfer, 0, sizeof xfer);
	memset(wrbuf, 0, sizeof wrbuf);
	memset(rdbuf, 0, sizeof rdbuf);

	//采集指定通道
	if(channel)
	{
		wrbuf[0] = 0x0;
		wrbuf[1] = __ChanCmd[channel-1];
		xfer.tx_buf = (unsigned long) wrbuf;
		xfer.rx_buf = (unsigned long) rdbuf;
		xfer.len = 2;

		//first sample ignored
		ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
		if (ret == 1)
		{
			printf("can't send spi message");
			return -1;
		}

		//
		for(i=0;i<(length+1);i++)
		{
			ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
			if (ret == 1)
			{
				printf("can't send spi message");
				return -1;
			}

			if(i>0)
				buffer[i-1] = ((unsigned short)rdbuf[1]<<8 | rdbuf[0]);
		}
	}
	else
	{
		wrbuf[0] = 0x0;
		wrbuf[1] = __ChanCmd[0];
		xfer.tx_buf = (unsigned long) wrbuf;
		xfer.rx_buf = (unsigned long) rdbuf;
		xfer.len = 2;

		//
		for(i=0;i<length;i++)
		{
			for(j=0;j<TOTALADCCHAN;j++)
			{
				wrbuf[1] = __ChanCmd[j];
				//first read
				ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
				if (ret == 1)
				{
					printf("can't send spi message");
					return -1;
				}
				if(j>0)
					buffer[i*TOTALADCCHAN + j-1] = ((unsigned short)rdbuf[1]<<8 | rdbuf[0]);
			}

			//last channel
			ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
			if (ret == 1)
			{
				printf("can't send spi message");
				return -1;
			}

			buffer[i*TOTALADCCHAN + TOTALADCCHAN -1] = ((unsigned short)rdbuf[1]<<8 | rdbuf[0]);
		}
	}

	return err;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ： ConvAdcValue
//* 功能描述 ： 转换采样值(数字量)为对应模拟量
//* 入口参数 : <buffer>[in]  存放采样值的缓冲区
//*                     : <len>[in]     需转换的采样值个数
//*                     : <vlaues>[in/out] 转换模拟量的存放缓冲区
//*                     : <keys>[in]    转换参数表
//* 返回值 ： 0：成功； -1 失败
//* 备  注 ：   1、转换关系表如下所示：
//*                                     struct __sampl_to_float keysvlaue[] = {
//*                                                             {0xfff, 3.32},
//*                                                             {0x12,  0.02} };
//*             上述数组为两组数字量与相对应模拟量数值，需根据实际测量进行设定
//*------------------------------------------------------------------------------------------------
int ConvAdcValue(cyg_adc_sample * buffer, unsigned int len,
	     float *values, struct __sampl_to_float * keys)
{

	int i = 0;
	float kval = 0.0, bval = 0.0;

	if (keys == NULL)
		return -1;

	kval =
	    (keys[0].fvalue - keys[1].fvalue) / (keys[0].ivalue -
						 keys[1].ivalue);

	bval = keys[1].fvalue - kval * keys[1].ivalue;
	for (i = 0; i < len; i++) {
		values[i] = (float) (buffer[i] * kval + bval);
	}
	return 0;
}

#endif
