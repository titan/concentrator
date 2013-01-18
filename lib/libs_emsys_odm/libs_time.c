/*
 * libs_time.c
 *
 *  Created on: 2011-2-15
 *      Author: Administrator
 */


#include "libs_emsys_odm.h"

#ifdef CFG_DATETIM_LIB
//自定义的BEEP I/O Control 指令
#define I2C_SLAVE		(0x0706)	/* Change slave address			*/
//DS1307的地址
#define I2C_DS1307_ADDR	(0x68)
//设备名
#define I2CNAME0		"/dev/i2c-0"
#define I2CNAME1		"/dev/i2c-1"
#define BCD2BIN(val)	(((val) & 0x0f) + ((val)>>4)*10)
#define BIN2BCD(val)	((((val)/10)<<4) + (val)%10)

//
static int fd_rtc = -1;

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：GetDateTime
//*     功能描述  ：获取当前日期与时间
//*     入口参数 : <data>[in/out] 返回当前日期与时间数值
//*                     :                data[0] --- year
//*                     :                       data[1] --- month
//*                     :                data[2] --- day
//*                     :                data[3] --- hour
//*                     :                        data[4] --- minute
//*                     :                data[5] --- second
//*                     :                data[6] --- wday
//*						:<from>[in], 指定时间出处(0:系统时间， 1：RTC时间（墙钟）)
//*     返回值： 成功返回ERROR_OK(==0)；失败返回-1
//*     备     注 ：        无
//*------------------------------------------------------------------------------------------------

int GetDateTime(unsigned int* data, char from)
{
	struct tm *tm_ptr = NULL;
	time_t timep;
	unsigned char buffer[12];
	struct rtc_time t;
	int tmp=0,retval=0;

	//系统时钟
	if(!from)
	{
		time(&timep);
		tm_ptr = localtime(&timep);
		data[0] = 1900 + tm_ptr->tm_year;
		data[1] = 1 + tm_ptr->tm_mon;
		data[2] = tm_ptr->tm_mday;
		data[3] = tm_ptr->tm_hour;
		data[4] = tm_ptr->tm_min;
		data[5] = tm_ptr->tm_sec;
		data[6] = tm_ptr->tm_wday;
	}
	//RTC
	else
	{
		if(fd_rtc<0)
		{
			fd_rtc = open(I2CNAME0,O_RDWR);
			if (fd_rtc <0)
			{
				fd_rtc = open(I2CNAME1,O_RDWR);
				if (fd_rtc <0)
				{
					printf("open device i2c error!\n");
					return -1;
				}
			}

			//设置DS1307的从机地址
			retval = ioctl(fd_rtc,I2C_SLAVE,I2C_DS1307_ADDR);
			if(retval<0)
			{
				printf("I2C_SLAVE_FORCE error!\n");
				return -1;
			}
		}

		//发送DS1307内部读地址
		buffer[0] = 0x0;
		if((retval = write(fd_rtc,buffer,1)) != 1)
		if(retval<0)
		{
			printf("write addr error!\n");
			return -1;
		}

		//读时间
		if((retval = read(fd_rtc,buffer,7)) != 7)
		{
			printf("read error! %d != 7\n", retval);
			return -1;
		}

		//根据时钟格式进行修订
		data[5] = BCD2BIN(buffer[0] & 0x7f);
		data[4] = BCD2BIN(buffer[1] & 0x7f);
		tmp = buffer[2] & 0x3f;
		data[3] = BCD2BIN(tmp);
		data[6] = BCD2BIN(buffer[3] & 0x07)-1;
		data[2] = BCD2BIN(buffer[4] & 0x3f);
		tmp = buffer[5] & 0x1f;
		data[1] = BCD2BIN(tmp);
		/* assume 20YY not 19YY, and ignore DS1337_BIT_CENTURY */
		data[0] = BCD2BIN(buffer[6]) + 2000;
	}

	return 0;

}

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：GetDate
//*     功能描述  ：获取当前日期
//*        入口参数 : <data>[in/out] 返回当前日期
//*                     :                data[0] --- year
//*                     :                       data[1] --- month
//*                     :                data[2] --- day
//*                     :                data[3] --- wday
//*						:<from>[in], 指定时间出处(0:系统时间， 1：RTC时间（墙钟）)
//*     返回值 ： 成功返回ERROR_OK(==0)；失败返回-1
//*     备  注 ：        无
//*------------------------------------------------------------------------------------------------

int GetDate(unsigned int* data, char from)
{
	struct tm *tm_ptr = NULL;
	time_t timep;
	unsigned char buffer[12];
	struct rtc_time t;
	int tmp=0,retval=0;

	//系统时钟
	if(!from)
	{
		time(&timep);
		tm_ptr = localtime(&timep);
		data[0] = tm_ptr->tm_year + 1900;
		data[1] = 1 + tm_ptr->tm_mon;
		data[2] = tm_ptr->tm_mday;
		data[3] = tm_ptr->tm_wday;
	}
	else
	{
		if(fd_rtc<0)
		{
			fd_rtc = open(I2CNAME0,O_RDWR);
			if (fd_rtc <0)
			{
				fd_rtc = open(I2CNAME1,O_RDWR);
				if (fd_rtc <0)
				{
					printf("open device i2c error!\n");
					return -1;
				}
			}

			//设置DS1307的从机地址
			retval = ioctl(fd_rtc,I2C_SLAVE,I2C_DS1307_ADDR);
			if(retval<0)
			{
				printf("I2C_SLAVE_FORCE error!\n");
				return -1;
			}
		}

		//发送DS1307内部读地址
		buffer[0] = 0x0;
		if((retval = write(fd_rtc,buffer,1)) != 1)
		if(retval<0)
		{
			printf("write addr error!\n");
			return -1;
		}

		//读时间
		if((retval = read(fd_rtc,buffer,7)) != 7)
		{
			printf("read error! %d != 7\n", retval);
			return -1;
		}

		//根据时钟格式进行修订
		data[3] = BCD2BIN(buffer[3] & 0x07)-1;
		data[2] = BCD2BIN(buffer[4] & 0x3f);
		tmp = buffer[5] & 0x1f;
		data[1] = BCD2BIN(tmp);

		/* assume 20YY not 19YY, and ignore DS1337_BIT_CENTURY */
		data[0] = BCD2BIN(buffer[6]) + 2000;

	}

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：GetTime
//*     功能描述  ：获取当前时间；
//*     入口参数 : <data>[in/out] 返回当前时间数值
//*                     :                data[0] --- hour
//*                     :                        data[1] --- minute
//*                     :                data[2] --- second
//*						:<from>[in], 指定时间出处(0:系统时间， 1：RTC时间（墙钟）)
//*     返回值：成功返回ERROR_OK(==0)；失败返回-1
//*     备  注 ：        无
//*------------------------------------------------------------------------------------------------
int GetTime(unsigned int* data, char from)
{
	struct tm *tm_ptr;
	time_t timep;
	unsigned char buffer[12];
	struct rtc_time t;
	int tmp=0,retval=0;

	//系统时钟
	if(!from)
	{
		time(&timep);
		tm_ptr = localtime(&timep);
		data[0] = tm_ptr->tm_hour;
		data[1] = tm_ptr->tm_min;
		data[2] = tm_ptr->tm_sec;
	}
	else
	{
		if(fd_rtc<0)
		{
			fd_rtc = open(I2CNAME0,O_RDWR);
			if (fd_rtc <0)
			{
				fd_rtc = open(I2CNAME1,O_RDWR);
				if (fd_rtc <0)
				{
					printf("open device i2c error!\n");
					return -1;
				}
			}

			//设置DS1307的从机地址
			retval = ioctl(fd_rtc,I2C_SLAVE,I2C_DS1307_ADDR);
			if(retval<0)
			{
				printf("I2C_SLAVE_FORCE error!\n");
				return -1;
			}
		}

		//发送DS1307内部读地址
		buffer[0] = 0x0;
		if((retval = write(fd_rtc,buffer,1)) != 1)
		if(retval<0)
		{
			printf("write addr error!\n");
			return -1;
		}

		//读时间
		if((retval = read(fd_rtc,buffer,7)) != 7)
		{
			printf("read error! %d != 7\n", retval);
			return -1;
		}

		//根据时钟格式进行修订
		data[2] = BCD2BIN(buffer[0] & 0x7f);
		data[1] = BCD2BIN(buffer[1] & 0x7f);
		tmp = buffer[2] & 0x3f;
		data[0] = BCD2BIN(tmp);
	}

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：SetDateTime
//*     功能描述  ：设置当前日期与时间
//*     入口参数 : <data>[in/out] 包含日期与时间的预设定数值
//*                     :                data[0] --- year
//*                     :                        data[1] --- month
//*                     :                data[2] --- day
//*                     :                data[3] --- hour
//*                     :                        data[4] --- minute
//*                     :                data[5] --- second
//*						:<to>[in], 时间类型(2:重置RTC时间(墙钟)， 1：重置系统时间, 0:全部重置)
//*    返回值  ：  成功返回ERROR_OK(==0)；失败返回-1
//*    备      注 ：        无
//*------------------------------------------------------------------------------------------------
int SetDateTime(unsigned int* data, char to)
{
	char __chrDateTime[40];
	unsigned char buffer[12];
	int tmp=0,retval=0;

	//系统时间
	if(!(to & 0x1<<1))
	{
		//设置系统时钟
		sprintf(__chrDateTime, "/bin/date  %02d%02d%02d%02d%04d.%d",
			data[1], data[2], data[3], data[4], data[0], data[5]);

		if (system(__chrDateTime))
			return -1;
	}

	//设置RTC时钟
	if(!(to & 0x1<<0))
	{
		//
		if(fd_rtc<0)
		{
			fd_rtc = open(I2CNAME0,O_RDWR);
			if (fd_rtc <0)
			{
				fd_rtc = open(I2CNAME1,O_RDWR);
				if (fd_rtc <0)
				{
					printf("open device i2c error!\n");
					return -1;
				}
			}

			//设置DS1307的从机地址
			retval = ioctl(fd_rtc,I2C_SLAVE,I2C_DS1307_ADDR);
			if(retval<0)
			{
				printf("I2C_SLAVE_FORCE error!\n");
				return -1;
			}
		}

		buffer[0] = 0;		/* first register addr */
		buffer[1] = BIN2BCD(data[5]);
		buffer[2] = BIN2BCD(data[4]);
		buffer[3] = BIN2BCD(data[3]);
		buffer[4] = 0;
		buffer[5] = BIN2BCD(data[2]);
		buffer[6] = BIN2BCD(data[1]);

		/* assume 20YY not 19YY */
		tmp = data[0] - 2000;
		buffer[7] = BIN2BCD(tmp);

		//写时间
		if((retval = write(fd_rtc,buffer,8)) != 8)
		{
			printf("write error! %d != 7\n", retval);
			return -1;
		}

	}

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：SetTime
//*     功能描述  ：设置当前时间
//*    入口参数 : <data>[in/out] 包含时间的预设定数值
//*                     :                data[0] --- hour
//*                     :                data[1] --- minute
//*                     :                data[2] --- second
//*						:<to>[in], 时间类型(2:重置RTC时间(墙钟)， 1：重置系统时间, 0:全部重置)
//*    返回值： 成功返回ERROR_OK(==0)；失败返回-1
//*    备     注 ：        无
//*------------------------------------------------------------------------------------------------
int SetTime(unsigned int* data, char to)
{
	char __chrDateTime[40];
	unsigned int temp[8];
	unsigned char buffer[12];
	int tmp=0,retval=0;

	//读取RTC
	GetDateTime(temp,1);

	//系统时间
	if(!(to & 0x1<<1))
	{
		temp[3] = data[0];
		temp[4] = data[1];
		temp[5] = data[2];
		//设置系统时钟
		sprintf(__chrDateTime, "/bin/date  %02d%02d%02d%02d%04d.%d",
		temp[1], temp[2], temp[3], temp[4], temp[0], temp[5]);

		if (system(__chrDateTime))
			return -1;
	}

	//设置RTC时钟钟
	if(!(to & 0x1<<0))
	{
		//
		if(fd_rtc<0)
		{
			fd_rtc = open(I2CNAME0,O_RDWR);
			if (fd_rtc <0)
			{
				fd_rtc = open(I2CNAME1,O_RDWR);
				if (fd_rtc <0)
				{
					printf("open device i2c error!\n");
					return -1;
				}
			}

			//设置DS1307的从机地址
			retval = ioctl(fd_rtc,I2C_SLAVE,I2C_DS1307_ADDR);
			if(retval<0)
			{
				printf("I2C_SLAVE_FORCE error!\n");
				return -1;
			}
		}

		buffer[0] = 0;		/* first register addr */
		buffer[1] = BIN2BCD(data[2]);
		buffer[2] = BIN2BCD(data[1]);
		buffer[3] = BIN2BCD(data[0]);
		buffer[4] = 0;
		buffer[5] = BIN2BCD(temp[2]);
		buffer[6] = BIN2BCD(temp[1]);

		/* assume 20YY not 19YY */
		tmp = temp[0] - 2000;
		buffer[7] = BIN2BCD(tmp);

		//写时间
		if((retval = write(fd_rtc,buffer,8)) != 8)
		{
			printf("write error! %d != 7\n", retval);
			return -1;
		}

	}

	return 0;
}

//*------------------------------------------------------------------------------------------------
//*     函数名称  ：SetDate
//*     功能描述  ：设置当前时间
//*     入口参数 : <data>[in/out] 包含时间的预设定数值
//*                     :                data[0] --- year
//*                     :                        data[1] --- month
//*                     :                data[2] --- day
//*						:<from>[in], 时间类型(2:重置RTC时间(墙钟)， 1：重置系统时间, 0:全部重置)
//*     返回值：成功返回ERROR_OK(==0)；失败返回-1
//*     备  注 ：        无
//*------------------------------------------------------------------------------------------------
int SetDate(unsigned int* data, char to)
{
	char __chrDateTime[40];
	unsigned int temp[8];
	unsigned char buffer[12];
	int tmp=0,retval=0;

	//读取RTC
	GetDateTime(temp,1);

	//系统时间
	if(!(to & 0x1<<1))
	{
		//设置时间值
		temp[0] = data[0];
		temp[1] = data[1];
		temp[2] = data[2];

		//设置系统时钟
		sprintf(__chrDateTime, "/bin/date  %02d%02d%02d%02d%04d.%d",
			temp[1], temp[2], temp[3], temp[4], temp[0], temp[5]);

		if (system(__chrDateTime))
			return -1;
	}

	//设置RTC  时钟钟
	if(!(to & 0x1<<0))
	{
		//
		if(fd_rtc<0)
		{
			fd_rtc = open(I2CNAME0,O_RDWR);
			if (fd_rtc <0)
			{
				fd_rtc = open(I2CNAME1,O_RDWR);
				if (fd_rtc <0)
				{
					printf("open device i2c error!\n");
					return -1;
				}
			}

			//设置DS1307的从机地址
			retval = ioctl(fd_rtc,I2C_SLAVE,I2C_DS1307_ADDR);
			if(retval<0)
			{
				printf("I2C_SLAVE_FORCE error!\n");
				return -1;
			}
		}

		buffer[0] = 0;		/* first register addr */
		buffer[1] = BIN2BCD(temp[5]);
		buffer[2] = BIN2BCD(temp[4]);
		buffer[3] = BIN2BCD(temp[3]);
		buffer[4] = 0;
		buffer[5] = BIN2BCD(data[2]);
		buffer[6] = BIN2BCD(data[1]);

		/* assume 20YY not 19YY */
		tmp = data[0] - 2000;
		buffer[7] = BIN2BCD(tmp);

		//写时间
		if((retval = write(fd_rtc,buffer,8)) != 8)
		{
			printf("write error! %d != 7\n", retval);
			return -1;
		}

	}

	return 0;
}
#endif
