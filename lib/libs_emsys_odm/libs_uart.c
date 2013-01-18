/*
 * libs_uart.c
 *
 *  Created on: 2011-2-16
 *      Author: Administrator
 */

#include "libs_emsys_odm.h"

#ifdef CFG_USART_LIB
//*------------------------------------------------------------------------------------------------
//*     函数名称  ：    OpenCom；
//*     功能描述  ：    打开并初始化串口；
//*     入口参数  ：    <dev>[in]  - 串口设备名
//*                     :       <configs>[in] - 串口参数字符串:波特率,数据位，停止位，校验位
//*                                                             如:   "115200,8,1,N"
//*                     :       <mode>[in] - 指定打开设备时的操作标识
//*                     :                 读模式：O_RDONLY
//*                     :                                 写模式：O_WRONLY
//*                     :                 读写模式：O_RDWR
//*     返回值  ：      成功返回设备句柄fd(>=0)， 失败返回-1；
//*     备  注 ：               无
//*------------------------------------------------------------------------------------------------
int
OpenCom(char *dev, char *configs, int mode)
{
	int fd = -1;

	//打开设备文件；
	if ((fd = open(dev, mode | O_NONBLOCK)) < 0) {
		printf("open %s error\n\r", dev);
		return -1;
	}
	//Set the configs
	if (!SetComCfg(fd, configs))
		return fd;
	else
		return -1;

}

//*------------------------------------------------------------------------------------------------
//*     函数名称：      SetComCfg
//*     功能描述：      设置串口参数；
//*     入口参数  ：    <fd>[in]  - 串口设备文件句柄
//*                     :       <configs>[in] - 串口参数字符串:波特率,数据位，停止位，校验位
//*                                                             如:   "115200,8,1,N"
//*     返回值  ：      成功返回0, 失败返回-1；
//*     备   注 ：      无
//*------------------------------------------------------------------------------------------------
int SetComCfg(int fd, char *configs)
{
	int uBaudRate, databits, stopbits;
	char parity;
	char temp[32];
	char *p1 = NULL, *p2 = NULL;
	int n = 0;
	unsigned int flag = 0;
	unsigned short Speed;
	int stats;
	struct termios options;

	/* clear struct for new port settings */
	bzero(&options, sizeof (struct termios));

	//波特率
	p1 = configs;
	p2 = strchr(p1, ',');
	n = p2 - p1;
	if (n) {
		strncpy(temp, p1, n);
		temp[n] = '\0';
		uBaudRate = atoi(temp);
	}
	else
		uBaudRate = COM_BAUDDEF;

	//数据位
	p1 = strchr(p2 + 1, ',');
	n = p1 - (p2 + 1);
	if (n) {
		strncpy(temp, p2 + 1, n);
		temp[n] = '\0';
		databits = atoi(temp);
	}
	else
		databits = COM_DABITSDEF;

	// 停止位
	p2 = strchr(p1 + 1, ',');
	n = p2 - (p1 + 1);
	if (n) {
		strncpy(temp, p1 + 1, n);
		temp[n] = '\0';
		stopbits = atoi(temp);
	}
	else
		stopbits = COM_STBITSDEF;

	//校验位
	if (p2 + 1 != NULL) {
		strcpy(temp, p2 + 1);
		parity = temp[0];
	}
	else {
		parity = COM_PARITYDEF;
	}

	//设置波特率
	switch (uBaudRate) {
	case 110:
		Speed = B110;
		break;

	case 300:
		Speed = B300;
		break;

	case 600:
		Speed = B600;
		break;

	case 1200:
		Speed = B1200;
		break;

	case 2400:
		Speed = B2400;
		break;

	case 4800:
		Speed = B4800;
		break;

	case 9600:
		Speed = B9600;
		break;

	case 19200:
		Speed = B19200;
		break;

	case 38400:
		Speed = B38400;
		break;

	case 57600:
		Speed = B57600;
		break;

	case 115200:
		Speed = B115200;
		break;

	case 230400:
		Speed = B230400;
		break;

	case 460800:
		Speed = B460800;
		break;

	case 921600:
		Speed = B921600;
		break;

	default:
		fprintf(stderr, "Unsupported data size\n");
		return -1;
	}

	flag |= Speed;

	//设置数据位
	switch (databits) {
	case 5:
		flag |= CS5;
		break;

	case 6:
		flag |= CS6;
		break;

	case 7:
		flag |= CS7;
		break;

	case 8:
		flag |= CS8;
		break;

	default:
		fprintf(stderr, "Unsupported data size\n");
		return -1;

	}

	//设置停止位
	switch (stopbits) {
	case 1:
		break;

	case 2:
		flag |= CSTOPB;
		break;

	default:
		fprintf(stderr, "Unsupported stop bits\n");
		return -1;

	}

	//设置奇偶校验位
	switch (parity) {
	case 'n':
	case 'N':
		options.c_iflag = IGNPAR;
		break;

	case 'e':
	case 'E':
		flag |= PARENB;
		options.c_iflag = 0;
		break;

	case 'o':
	case 'O':
		flag |= PARENB | PARODD;
		options.c_iflag = 0;
		break;

	case 's':
	case 'S':
		flag |= PARENB | CMSPAR;
		options.c_iflag = 0;
		break;

	case 'm':
	case 'M':
		flag |= PARENB | PARODD | CMSPAR;
		options.c_iflag = 0;
		break;

	default:
		fprintf(stderr, "Unsupported parity\n");
		return -1;

	}

	options.c_cflag = flag | CREAD | CLOCAL;
	options.c_oflag = 0;
	options.c_lflag = 0;
	options.c_cc[VINTR] = 0;
	options.c_cc[VQUIT] = 0;
	options.c_cc[VERASE] = 0;
	options.c_cc[VKILL] = 0;
	options.c_cc[VEOF] = 4;
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 1;
	options.c_cc[VSWTC] = 0;
	options.c_cc[VSTART] = 0;
	options.c_cc[VSTOP] = 0;
	options.c_cc[VSUSP] = 0;
	options.c_cc[VEOL] = 0;
	options.c_cc[VREPRINT] = 0;
	options.c_cc[VDISCARD] = 0;
	options.c_cc[VWERASE] = 0;
	options.c_cc[VLNEXT] = 0;
	options.c_cc[VEOL2] = 0;

	//刷新输入输出缓冲
	tcflush(fd, TCIOFLUSH);

	//立刻把bote rates设置真正写到串口中去
	stats = tcsetattr(fd, TCSANOW, &options);

	if (stats != 0) {
		perror("tcsetattr fd1");
		return -1;

	}

	return 0;
}


//*------------------------------------------------------------------------------------------------
//*     函数名称：ReadCom；
//*     功能描述：读串口设备数据；
//* 入口参数 : <fd>[in]  指定串口设备的句柄(即OpenCom()的返回值)
//*                     : <buffer>[in/out] 读取的数据存放缓冲区
//*                     : <len>[in/out] 指定读取的数据长度(指针)，并返回实际读取的数据数目
//*                     : <timesout>[in]   指定最大的阻塞读取等待间隔，单位: useconds(微秒)
//*                     :        当timesout<=0，阻塞读模式，即直到读够指定数据个数后函数返回
//*     返回值  ：返回操作状态信息(0:成功返回， -1：失败)
//*     备  注 ：   1、分配采样值存放缓冲区时，一定要足够存放指定的数据个数。
//*             2、当置为阻塞读模式时，指定的读数据个数需要恰当，否则可能会造成进程阻塞，若没有相应
//*                个数数据供读操作的话。
//*				3、实际成功读取数据大小，由参数len返回。
//*------------------------------------------------------------------------------------------------
int ReadCom(int fd, char *buffer, unsigned int *len, int timesout)
{
	int blocking = 1;
	int result=0,readings=0;
	unsigned int length = 0;

	if (!buffer)
		return -1;

	length = (*len);

	//NON_BLOCK
	if (timesout > 0) {
		//set for nonblocking
		if(ioctl(fd, FIONBIO, &blocking)<0)
			return -1;

		struct timeval tv;
		fd_set watchfds;
		int sec = 0, usec = 0;

		sec = timesout / 1000000;
		usec = timesout % 1000000;

		FD_ZERO(&watchfds);
		FD_SET(fd, &watchfds);
		tv.tv_sec = sec;
		tv.tv_usec = usec;

		result = select((1 + fd), &watchfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
		switch(result)
		{
			//timeout or error
			case 0:
			case -1:
				*len = 0;
			return result;

			default:
				if (FD_ISSET(fd, &watchfds))
				{
					ioctl(fd, FIONREAD,&readings);
					//no data for reading or interrupt by signal
					if(readings == 0)
					{
						*len = 0;
						return 0;
					}
					else
					{
						if(readings>length)
							readings = length ;
						readings = read(fd, (char *)buffer,readings);

						*len = readings;
						return 0;
					}
				}
				else
				{
					*len = 0;
					return -1;
				}
			break;
		}
	}
	//Blocking
	else
	{
		//set for blocking
	    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~(O_NONBLOCK)) < 0)
			return -1;

	    result = _Readn(fd, buffer, length);
	    if(result < 0)
	    	return -1;
	    else
	    {
			*len = result;
			return 0;
	    }
	}

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：WriteCom；
//*     功能描述  ：向串口发数据；
//* 入口参数 : <fd>[in]  指定串口设备的句柄(即OpenCom()的返回值)
//*                     : <buffer>[in/out] 读取的采样值缓冲区
//*                     : <len>[in/out] 指定写的数据长度，并返回实际写完的数据长度
//*                     : <timesout>[in]   指定最大的阻塞写等待间隔，单位: useconds(微秒)
//*                     :      当timesout<=0，阻塞写模式，即直到写完指定数据个数后函数返回
//*     返回值  ：返回操作状态信息(0:成功返回， -1：失败)
//*     备  注 :	1、当置为阻塞写模式时，指定的数据个数需要恰当，否则可能会造成进程阻塞，若当前设备不可写的话
//*				2、实际写的数据大小，由参数len返回。

//*------------------------------------------------------------------------------------------------
int WriteCom(int fd, char *buffer, unsigned int *len, int timesout)
{
	int blocking = 1;
	int result=0,writings=0;
	unsigned int length = 0;

	if (!buffer)
		return -1;

	length = (*len);

	//NON_BLOCK
	if (timesout > 0)
	{
		//set for nonblocking
		ioctl(fd, FIONBIO, &blocking);

		struct timeval tv;
		fd_set watchfds;
		int sec = 0, usec = 0;

		sec = timesout / 1000000;
		usec = timesout % 1000000;

		FD_ZERO(&watchfds);
		FD_SET(fd, &watchfds);

		tv.tv_sec = sec;
		tv.tv_usec = usec;

		result = select(1 + fd, NULL, &watchfds, NULL, &tv);
		switch(result)
		{
			//timeout or error
			case 0:
			case -1:
				*len = 0;
			return result;

			default:
				if (FD_ISSET(fd, &watchfds))
				{
					writings = write(fd, (char *) buffer, length);
					*len = writings;
					return 0;
				}
				else
				{
					*len = 0;
					return -1;
				}
			break;
		}

	}
	//Blocking
	else
	{
		//set for blocking
	    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~(O_NONBLOCK)) < 0)
			return -1;

	    result = _Writen(fd, buffer, length);
	    if(result < 0)
	    	return -1;
	    else
	    {
			*len = result;
			return 0;
	    }
	}

	return 0;
}

//*------------------------------------------------------------------------------------------------
//* 函数名称 ：CloseCom
//* 功能描述 ：关闭串口设备
//* 入口参数 : <fd>[in]  指定串口设备的句柄(即OpenCom()的返回值)
//* 返回值 ： 成功返回0；失败返回-1
//* 备  注 ：   无
//*------------------------------------------------------------------------------------------------
int CloseCom(int fd)
{
	tcflush(fd, TCIOFLUSH);

	//关闭设备文件；
	close(fd);
	return 0;
}

#endif
