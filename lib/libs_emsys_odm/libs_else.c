/*
 * libs_else.c
 *
 *  Created on: 2011-2-15
 *      Author: Administrator
 */

#include "libs_emsys_odm.h"

inline unsigned short _inw(unsigned long addr) {
        unsigned short ret;

        asm volatile (
                "ldrh %0, [ %1 ]\n"
                : "=r" (ret)
                : "r" (addr)
                : "memory"
        );
        return ret;
}

inline void _outw(unsigned long addr, unsigned short dat) {
        asm volatile (
                "strh %1, [ %0 ]\n"
                :
                : "r" (addr), "r" (dat)
                : "memory"
        );
}

inline unsigned long _inl(unsigned long addr) {
        unsigned long ret;

        asm volatile (
                "ldr %0, [ %1 ]\n"
                : "=r" (ret)
                : "r" (addr)
                : "memory"
        );
        return ret;
}

inline void _outl(unsigned long addr, unsigned long dat) {
        asm volatile (
                "str %1, [ %0 ]\n"
                :
                : "r" (addr), "r" (dat)
                : "memory"
        );
}


inline unsigned char _inb(unsigned long addr) {
        unsigned char ret;

        asm volatile (
                "ldrb %0, [ %1 ]\n"
                : "=r" (ret)
                : "r" (addr)
                : "memory"
        );
        return ret;
}

inline void _outb(unsigned long addr, unsigned char dat) {
        asm volatile (
                "strb %1, [ %0 ]\n"
                :
                : "r" (addr), "r" (dat)
                : "memory"
        );
}

inline void _nops(unsigned long nums) {
       __asm__ __volatile__(
        		"__loops__:\n\t"
        		"nop \n\t"
                "subs %0, %0, #1 \n\t"
                "bne __loops__ \n\t"
                :
                : "r" (nums)
                : "memory"
        );
}

//*------------------------------------------------------------------------------------------------
//*     函数名称：_Readn；
//*     功能描述：流式读函数；
//*     入口参数  ：<fd>[in]  - 文件句柄
//*                     : <buf>[in] - 数据存放缓冲区
//*                     : <n>[in]   - 指定打开设备时的操作标识
//*     返回值 ：返回成功已读字符数 ； 失败返回-1 ；
//*     备  注  ：无
//*------------------------------------------------------------------------------------------------
int _Readn(int fd, void *buf, int n)
{
	int nleft;
	int nread;
	char *ptr;

	if ((fd <= 0) || (NULL == buf) || (0 == n))
		return -1;

	ptr = buf;
	nleft = n;

	while (nleft > 0) {
		nread = read(fd, ptr, nleft);
		if (nread < 0)
		{
			if (errno == EINTR) {
				nread = 0;
			}
			else
				return (-1);
		}//
		else if (nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}

	return (n - nleft);
}

//*------------------------------------------------------------------------------------------------
//*      函数名称：_Writen；
//*      功能描述：流式写函数
//*      入口参数  ：<fd>[in]  - 文件句柄
//*                     : <buf>[in] - 数据存放缓冲区
//*                     : <n>[in]   - 指定打开设备时的操作标识
//*     返回值 ：返回已写字符数；失败返回-1 ；
//* 备  注 ：无
//*------------------------------------------------------------------------------------------------
int _Writen(int fd, const void *buf, int n)
{
	int nleft;
	int nwrite;
	const char *ptr;

	if ((fd <= 0) || (NULL == buf) || (0 == n))
		return -1;

	ptr = buf;
	nleft = n;
	while (nleft > 0)
	{
		if ((nwrite = write(fd, ptr, nleft)) < 0) {
			if (errno == EINTR) {
				nwrite = 0;
				break;
			}
			else {
				return (-1);
			}
		}

		nleft -= nwrite;
		ptr += nwrite;
	}

	return (n - nleft);
}
