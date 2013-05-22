#ifndef __BDK_FONT_H
#define __BDK_FONT_H

#ifdef __cplusplus
 extern "C" {
#endif
// #include <absacc.h>
#define ASCII_BYTES     1632

#define HZ_BYTES     2496

#define FONT_MEM_ADDRESS   0x802D000      //   1030H

// ------------------  ASCII字模的数据表 ------------------------ //
// 码表从0x20~0x7e,
// 0x80:全黑                           //
// 0x81:信号强度0
// 0x82:信号强度1
// 0x83:信号强度2
// 0x84:信号强度3
// 0x85:信号强度4
// 字库: G:\zimo\Asc8X16E.dat 纵向取模上高位                      //
// -------------------------------------------------------------- //
const unsigned char   nAsciiDot[] =              // ASCII	   __at(FONT_MEM_ADDRESS+ASCII_BYTES+HZ_BYTES)
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // - -
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x00,0x00,0x38,0xFC,0xFC,0x38,0x00,0x00,  // -!-
	0x00,0x00,0x00,0x0D,0x0D,0x00,0x00,0x00,

	0x00,0x0E,0x1E,0x00,0x00,0x1E,0x0E,0x00,  // -"-
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x20,0xF8,0xF8,0x20,0xF8,0xF8,0x20,0x00,  // -#-
	0x02,0x0F,0x0F,0x02,0x0F,0x0F,0x02,0x00,

	0x38,0x7C,0x44,0x47,0x47,0xCC,0x98,0x00,  // -$-
	0x03,0x06,0x04,0x1C,0x1C,0x07,0x03,0x00,

	0xF0,0x08,0xF0,0x00,0xE0,0x18,0x00,0x00,// -%-
	0x00,0x21,0x1C,0x03,0x1E,0x21,0x1E,0x00,

	0x80,0xD8,0x7C,0xE4,0xBC,0xD8,0x40,0x00,  // -&-
	0x07,0x0F,0x08,0x08,0x07,0x0F,0x08,0x00,

	0x00,0x10,0x1E,0x0E,0x00,0x00,0x00,0x00,  // -'-
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x00,0x00,0xF0,0xF8,0x0C,0x04,0x00,0x00,  // -(-
	0x00,0x00,0x03,0x07,0x0C,0x08,0x00,0x00,

	0x00,0x00,0x04,0x0C,0xF8,0xF0,0x00,0x00,  // -)-
	0x00,0x00,0x08,0x0C,0x07,0x03,0x00,0x00,

	0x80,0xA0,0xE0,0xC0,0xC0,0xE0,0xA0,0x80,  // -*-
	0x00,0x02,0x03,0x01,0x01,0x03,0x02,0x00,

	0x00,0x80,0x80,0xE0,0xE0,0x80,0x80,0x00,  // -+-
	0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00,

	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // -,-
	0x00,0x00,0x10,0x1E,0x0E,0x00,0x00,0x00,

	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x00,  // ---
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // -.-
	0x00,0x00,0x00,0x0C,0x0C,0x00,0x00,0x00,

	0x00,0x00,0x00,0x80,0xC0,0x60,0x30,0x00,  // -/-
	0x0C,0x06,0x03,0x01,0x00,0x00,0x00,0x00,

	0xF8,0xFC,0x04,0xC4,0x24,0xFC,0xF8,0x00,  // -0-
	0x07,0x0F,0x09,0x08,0x08,0x0F,0x07,0x00,

	0x00,0x10,0x18,0xFC,0xFC,0x00,0x00,0x00,  // -1-
	0x00,0x08,0x08,0x0F,0x0F,0x08,0x08,0x00,

	0x08,0x0C,0x84,0xC4,0x64,0x3C,0x18,0x00,  // -2-
	0x0E,0x0F,0x09,0x08,0x08,0x0C,0x0C,0x00,

	0x08,0x0C,0x44,0x44,0x44,0xFC,0xB8,0x00,  // -3-
	0x04,0x0C,0x08,0x08,0x08,0x0F,0x07,0x00,

	0xC0,0xE0,0xB0,0x98,0xFC,0xFC,0x80,0x00,  // -4-
	0x00,0x00,0x00,0x08,0x0F,0x0F,0x08,0x00,

	0x7C,0x7C,0x44,0x44,0xC4,0xC4,0x84,0x00,  // -5-
	0x04,0x0C,0x08,0x08,0x08,0x0F,0x07,0x00,

	0xF0,0xF8,0x4C,0x44,0x44,0xC0,0x80,0x00,  // -6-
	0x07,0x0F,0x08,0x08,0x08,0x0F,0x07,0x00,

	0x0C,0x0C,0x04,0x84,0xC4,0x7C,0x3C,0x00,  // -7-
	0x00,0x00,0x0F,0x0F,0x00,0x00,0x00,0x00,

	0xB8,0xFC,0x44,0x44,0x44,0xFC,0xB8,0x00,  // -8-
	0x07,0x0F,0x08,0x08,0x08,0x0F,0x07,0x00,

	0x38,0x7C,0x44,0x44,0x44,0xFC,0xF8,0x00,  // -9-
	0x00,0x08,0x08,0x08,0x0C,0x07,0x03,0x00,

	0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,  // -:-
	0x00,0x00,0x00,0x06,0x06,0x00,0x00,0x00,

	0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,  // -;-
	0x00,0x00,0x08,0x0E,0x06,0x00,0x00,0x00,

	0x00,0x80,0xC0,0x60,0x30,0x18,0x08,0x00,  // -<-
	0x00,0x00,0x01,0x03,0x06,0x0C,0x08,0x00,

	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00,  // -=-
	0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x00,

	0x00,0x08,0x18,0x30,0x60,0xC0,0x80,0x00,  // ->-
	0x00,0x08,0x0C,0x06,0x03,0x01,0x00,0x00,

	0x18,0x1C,0x04,0xC4,0xE4,0x3C,0x18,0x00,  // -?-
	0x00,0x00,0x00,0x0D,0x0D,0x00,0x00,0x00,

	0x83,0x86,0xCC,0x78,0xB0,0x60,0xC0,0x80,  // -@-
	0xC1,0x61,0x33,0x1E,0x0D,0x06,0x03,0x01,

	0xE0,0xF0,0x98,0x8C,0x98,0xF0,0xE0,0x00,  // -A-
	0x0F,0x0F,0x00,0x00,0x00,0x0F,0x0F,0x00,

	0x04,0xFC,0xFC,0x44,0x44,0xFC,0xB8,0x00,  // -B-
	0x08,0x0F,0x0F,0x08,0x08,0x0F,0x07,0x00,


	0xF0,0xF8,0x0C,0x04,0x04,0x0C,0x18,0x00,  // -C-
	0x03,0x07,0x0C,0x08,0x08,0x0C,0x06,0x00,

	0x04,0xFC,0xFC,0x04,0x0C,0xF8,0xF0,0x00,  // -D-
	0x08,0x0F,0x0F,0x08,0x0C,0x07,0x03,0x00,

	0x04,0xFC,0xFC,0x44,0xE4,0x0C,0x1C,0x00,  // -E-
	0x08,0x0F,0x0F,0x08,0x08,0x0C,0x0E,0x00,

	0x04,0xFC,0xFC,0x44,0xE4,0x0C,0x1C,0x00,  // -F-
	0x08,0x0F,0x0F,0x08,0x00,0x00,0x00,0x00,

	0xF0,0xF8,0x0C,0x84,0x84,0x8C,0x98,0x00,  // -G-
	0x03,0x07,0x0C,0x08,0x08,0x07,0x0F,0x00,

	0xFC,0xFC,0x40,0x40,0x40,0xFC,0xFC,0x00,  // -H-
	0x0F,0x0F,0x00,0x00,0x00,0x0F,0x0F,0x00,

	0x00,0x00,0x04,0xFC,0xFC,0x04,0x00,0x00,  // -I-
	0x00,0x00,0x08,0x0F,0x0F,0x08,0x00,0x00,

	0x00,0x00,0x00,0x04,0xFC,0xFC,0x04,0x00,  // -J-
	0x07,0x0F,0x08,0x08,0x0F,0x07,0x00,0x00,

 	0x04,0xFC,0xFC,0xC0,0xF0,0x3C,0x0C,0x00,  // -K-
	0x08,0x0F,0x0F,0x00,0x01,0x0F,0x0E,0x00,

	0x04,0xFC,0xFC,0x04,0x00,0x00,0x00,0x00,  // -L-
	0x08,0x0F,0x0F,0x08,0x08,0x0C,0x0E,0x00,

	0xFC,0xFC,0x38,0x70,0x38,0xFC,0xFC,0x00,  // -M-
	0x0F,0x0F,0x00,0x00,0x00,0x0F,0x0F,0x00,

	0xFC,0xFC,0x38,0x70,0xE0,0xFC,0xFC,0x00,  // -N-
	0x0F,0x0F,0x00,0x00,0x00,0x0F,0x0F,0x00,

	0xF0,0xF8,0x0C,0x04,0x0C,0xF8,0xF0,0x00,  // -O-
	0x03,0x07,0x0C,0x08,0x0C,0x07,0x03,0x00,

	0x04,0xFC,0xFC,0x44,0x44,0x7C,0x38,0x00,  // -P-
	0x08,0x0F,0x0F,0x08,0x00,0x00,0x00,0x00,

	0xF8,0xFC,0x04,0x04,0x04,0xFC,0xF8,0x00,  // -Q-
	0x07,0x0F,0x08,0x0E,0x3C,0x3F,0x27,0x00,

	0x04,0xFC,0xFC,0x44,0xC4,0xFC,0x38,0x00,  // -R-
	0x08,0x0F,0x0F,0x00,0x00,0x0F,0x0F,0x00,

	0x18,0x3C,0x64,0x44,0xC4,0x9C,0x18,0x00,  // -S-
	0x06,0x0E,0x08,0x08,0x08,0x0F,0x07,0x00,

	0x00,0x1C,0x0C,0xFC,0xFC,0x0C,0x1C,0x00,  // -T-
	0x00,0x00,0x08,0x0F,0x0F,0x08,0x00,0x00,

	0xFC,0xFC,0x00,0x00,0x00,0xFC,0xFC,0x00,  // -U-
	0x07,0x0F,0x08,0x08,0x08,0x0F,0x07,0x00,

	0xFC,0xFC,0x00,0x00,0x00,0xFC,0xFC,0x00,  // -V-
	0x01,0x03,0x06,0x0C,0x06,0x03,0x01,0x00,

	0xFC,0xFC,0x00,0x80,0x00,0xFC,0xFC,0x00,  // -W-
	0x03,0x0F,0x0E,0x03,0x0E,0x0F,0x03,0x00,

	0x0C,0x3C,0xF0,0xC0,0xF0,0x3C,0x0C,0x00,  // -X-
	0x0C,0x0F,0x03,0x00,0x03,0x0F,0x0C,0x00,

	0x00,0x3C,0x7C,0xC0,0xC0,0x7C,0x3C,0x00,  // -Y-
	0x00,0x00,0x08,0x0F,0x0F,0x08,0x00,0x00,

	0x1C,0x0C,0x84,0xC4,0x64,0x3C,0x1C,0x00,  // -Z-
	0x0E,0x0F,0x09,0x08,0x08,0x0C,0x0E,0x00,

	0x00,0x00,0xFC,0xFC,0x04,0x04,0x00,0x00,  // -[-
	0x00,0x00,0x0F,0x0F,0x08,0x08,0x00,0x00,

	0x38,0x70,0xE0,0xC0,0x80,0x00,0x00,0x00,  // -\-
	0x00,0x00,0x00,0x01,0x03,0x07,0x0E,0x00,

	0x00,0x00,0x04,0x04,0xFC,0xFC,0x00,0x00,  // -]-
	0x00,0x00,0x08,0x08,0x0F,0x0F,0x00,0x00,

	0x08,0x0C,0x06,0x03,0x06,0x0C,0x08,0x00,  // -^-
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // -_-
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,

	0x00,0x00,0x03,0x07,0x04,0x00,0x00,0x00,  // -`-
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x00,0xA0,0xA0,0xA0,0xE0,0xC0,0x00,0x00,  // -a-
	0x07,0x0F,0x08,0x08,0x07,0x0F,0x08,0x00,

	0x04,0xFC,0xFC,0x20,0x60,0xC0,0x80,0x00,  // -b-
	0x08,0x0F,0x07,0x08,0x08,0x0F,0x07,0x00,

	0xC0,0xE0,0x20,0x20,0x20,0x60,0x40,0x00,  // -c-
	0x07,0x0F,0x08,0x08,0x08,0x0C,0x04,0x00,

	0x80,0xC0,0x60,0x24,0xFC,0xFC,0x00,0x00,  // -d-
	0x07,0x0F,0x08,0x08,0x07,0x0F,0x08,0x00,

	0xC0,0xE0,0xA0,0xA0,0xA0,0xE0,0xC0,0x00,  // -e-
	0x07,0x0F,0x08,0x08,0x08,0x0C,0x04,0x00,

	0x40,0xF8,0xFC,0x44,0x0C,0x18,0x00,0x00,  // -f-
	0x08,0x0F,0x0F,0x08,0x00,0x00,0x00,0x00,

	0xC0,0xE0,0x20,0x20,0xC0,0xE0,0x20,0x00,  // -g-
	0x27,0x6F,0x48,0x48,0x7F,0x3F,0x00,0x00,

	0x04,0xFC,0xFC,0x40,0x20,0xE0,0xC0,0x00,  // -h-
	0x08,0x0F,0x0F,0x00,0x00,0x0F,0x0F,0x00,

	0x00,0x00,0x20,0xEC,0xEC,0x00,0x00,0x00,  // -i-
	0x00,0x00,0x08,0x0F,0x0F,0x08,0x00,0x00,

	0x00,0x00,0x00,0x00,0x20,0xEC,0xEC,0x00,  // -j-
	0x00,0x30,0x70,0x40,0x40,0x7F,0x3F,0x00,

	0x04,0xFC,0xFC,0x80,0xC0,0x60,0x20,0x00,  // -k-
	0x08,0x0F,0x0F,0x01,0x03,0x0E,0x0C,0x00,

	0x00,0x00,0x04,0xFC,0xFC,0x00,0x00,0x00,  // -l-
	0x00,0x00,0x08,0x0F,0x0F,0x08,0x00,0x00,

	0xE0,0xE0,0x60,0xC0,0x60,0xE0,0xC0,0x00,  // -m-
	0x0F,0x0F,0x00,0x0F,0x00,0x0F,0x0F,0x00,

	0x20,0xE0,0xC0,0x20,0x20,0xE0,0xC0,0x00,  // -n-
	0x00,0x0F,0x0F,0x00,0x00,0x0F,0x0F,0x00,

	0xC0,0xE0,0x20,0x20,0x20,0xE0,0xC0,0x00,  // -o-
	0x07,0x0F,0x08,0x08,0x08,0x0F,0x07,0x00,

	0x20,0xE0,0xC0,0x20,0x20,0xE0,0xC0,0x00,  // -p-
	0x40,0x7F,0x7F,0x48,0x08,0x0F,0x07,0x00,

	0xC0,0xE0,0x20,0x20,0xC0,0xE0,0x20,0x00,  // -q-
	0x07,0x0F,0x08,0x48,0x7F,0x7F,0x40,0x00,

	0x20,0xE0,0xC0,0x60,0x20,0x60,0xC0,0x00,  // -r-
	0x08,0x0F,0x0F,0x08,0x00,0x00,0x00,0x00,

	0x40,0xE0,0xA0,0x20,0x20,0x60,0x40,0x00,  // -s-
	0x04,0x0C,0x09,0x09,0x0B,0x0E,0x04,0x00,

	0x20,0x20,0xF8,0xFC,0x20,0x20,0x00,0x00,  // -t-
	0x00,0x00,0x07,0x0F,0x08,0x0C,0x04,0x00,

	0xE0,0xE0,0x00,0x00,0xE0,0xE0,0x00,0x00,  // -u-
	0x07,0x0F,0x08,0x08,0x07,0x0F,0x08,0x00,

	0x00,0xE0,0xE0,0x00,0x00,0xE0,0xE0,0x00,  // -v-
	0x00,0x03,0x07,0x0C,0x0C,0x07,0x03,0x00,

	0xE0,0xE0,0x00,0x00,0x00,0xE0,0xE0,0x00,  // -w-
	0x07,0x0F,0x0C,0x07,0x0C,0x0F,0x07,0x00,

	0x20,0x60,0xC0,0x80,0xC0,0x60,0x20,0x00,  // -x-
	0x08,0x0C,0x07,0x03,0x07,0x0C,0x08,0x00,

	0xE0,0xE0,0x00,0x00,0x00,0xE0,0xE0,0x00,  // -y-
	0x47,0x4F,0x48,0x48,0x68,0x3F,0x1F,0x00,

	0x60,0x60,0x20,0xA0,0xE0,0x60,0x20,0x00,  // -z-
	0x0C,0x0E,0x0B,0x09,0x08,0x0C,0x0C,0x00,

	0x00,0x40,0x40,0xF8,0xBC,0x04,0x04,0x00,  // -{-
	0x00,0x00,0x00,0x07,0x0F,0x08,0x08,0x00,

	0x00,0x00,0x00,0xBC,0xBC,0x00,0x00,0x00,  // -|-
	0x00,0x00,0x00,0x0F,0x0F,0x00,0x00,0x00,

	0x00,0x04,0x04,0xBC,0xF8,0x40,0x40,0x00,  // -}-
	0x00,0x08,0x08,0x0F,0x07,0x00,0x00,0x00,

	0x08,0x0C,0x04,0x0C,0x08,0x0C,0x04,0x00,  // -~-
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x80,0xC0,0x60,0x30,0x60,0xC0,0x80,0x00,  // --
	0x07,0x07,0x04,0x04,0x04,0x07,0x07,0x00,

	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  //全黑字符
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,

	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, //信号强度0
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,

	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, //信号强度1
	0x80,0xB8,0xB8,0x80,0x80,0x80,0x80,0x80,

	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, //信号强度2
	0x80,0xB8,0xB8,0x80,0x80,0xBE,0xBE,0x80,

	0x01,0x81,0x81,0x01,0x01,0x01,0x01,0x01, //信号强度3
	0x80,0xBF,0xBF,0x80,0x80,0x80,0x80,0x80,

	0x01,0x81,0x81,0x01,0x01,0xF1,0xF1,0x01, //信号强度4
	0x80,0xBF,0xBF,0x80,0x80,0xBF,0xBF,0x80,

};


// ------------------  汉字字模的数据结构定义 ------------------------ //
typedef struct typFNT_GB16                 // 汉字字模数据结构
{
       signed char Index[4];               // 汉字内码索引
       char Msk[32];                       // 点阵码数据
}aa;

/////////////////////////////////////////////////////////////////////////
// 汉字字模表                                                          //
// 汉字库: 宋体16.dot 纵向取模上高位,数据排列:从左到右从上到下         //
/////////////////////////////////////////////////////////////////////////
const struct typFNT_GB16  GB_16[] =          // 数据表	   __at(FONT_MEM_ADDRESS+HZ_BYTES)
{

"数",   0x90,0x52,0x34,0x10,0xFF,0x10,0x34,0x52,  //0
	0x80,0x70,0x8F,0x08,0x08,0xF8,0x08,0x00,
	0x82,0x9A,0x56,0x63,0x22,0x52,0x8E,0x00,
	0x80,0x40,0x33,0x0C,0x33,0x40,0x80,0x00,

"据",   0x10,0x10,0xFF,0x10,0x90,0x00,0xFE,0x92,  //1
	0x92,0x92,0xF2,0x92,0x92,0x9E,0x80,0x00,
	0x42,0x82,0x7F,0x01,0x80,0x60,0x1F,0x00,
	0xFC,0x44,0x47,0x44,0x44,0xFC,0x00,0x00,

"集",   0x20,0x10,0x08,0xFC,0x57,0x54,0x54,0x55,  //2
        0xFE,0x54,0x54,0x54,0x54,0x04,0x00,0x00,
        0x44,0x44,0x24,0x27,0x15,0x0D,0x05,0xFF,
	0x05,0x0D,0x15,0x25,0x25,0x45,0x44,0x00,

"中",   0x00,0x00,0xF0,0x10,0x10,0x10,0x10,0xFF,  //3
        0x10,0x10,0x10,0x10,0xF0,0x00,0x00,0x00,
        0x00,0x00,0x0F,0x04,0x04,0x04,0x04,0xFF,
	0x04,0x04,0x04,0x04,0x0F,0x00,0x00,0x00,

"器",	0x80,0x80,0x9E,0x92,0x92,0x92,0x9E,0xE0,    //4
        0x80,0x9E,0xB2,0xD2,0x92,0x9E,0x80,0x00,
        0x08,0x08,0xF4,0x94,0x92,0x92,0xF1,0x00,
	0x01,0xF2,0x92,0x94,0x94,0xF8,0x08,0x00,

"转",	0xC8,0xB8,0x8F,0xE8,0x88,0x88,0x40,0x48,    //5
        0x48,0xE8,0x5F,0x48,0x48,0x48,0x40,0x00,
        0x08,0x18,0x08,0xFF,0x04,0x04,0x00,0x02,
	0x0B,0x12,0x22,0xD2,0x0A,0x06,0x00,0x00,
"发",   0x00,0x00,0x18,0x16,0x10,0xD0,0xB8,0x97,  //6
        0x90,0x90,0x90,0x92,0x94,0x10,0x00,0x00,
        0x00,0x20,0x10,0x8C,0x83,0x80,0x41,0x46,
	0x28,0x10,0x28,0x44,0x43,0x80,0x80,0x00,

"热",   0x08,0x08,0x88,0xFF,0x48,0x48,0x00,0x08,  //7
	0x48,0xFF,0x08,0x08,0xF8,0x00,0x00,0x00,
	0x81,0x65,0x08,0x07,0x20,0xC0,0x08,0x04,
	0x23,0xC0,0x03,0x00,0x23,0xC4,0x0F,0x00,

"总",   0x00,0x00,0x00,0xF1,0x12,0x14,0x10,0x10,  //8
        0x10,0x14,0x12,0xF1,0x00,0x00,0x00,0x00,
        0x40,0x30,0x00,0x03,0x39,0x41,0x41,0x45,
	0x59,0x41,0x41,0x73,0x00,0x08,0x30,0x00,

"表",   0x00,0x04,0x24,0x24,0x24,0x24,0x24,0xFF,  //9
	0x24,0x24,0x24,0x24,0x24,0x04,0x00,0x00,
	0x21,0x21,0x11,0x09,0xFD,0x83,0x41,0x23,
	0x05,0x09,0x11,0x29,0x25,0x41,0x41,0x00,

"检",	0x10,0x10,0xD0,0xFF,0x90,0x50,0x20,0x50,    //10
        0x4C,0x43,0x4C,0x50,0x20,0x40,0x40,0x00,
        0x04,0x03,0x00,0xFF,0x00,0x41,0x44,0x58,
	0x41,0x4E,0x60,0x58,0x47,0x40,0x40,0x00,

"测",   0x10,0x60,0x02,0x8C,0x00,0xFE,0x02,0xF2,  //11
	0x02,0xFE,0x00,0xF8,0x00,0xFF,0x00,0x00,
	0x04,0x04,0x7E,0x01,0x80,0x47,0x30,0x0F,
	0x10,0x27,0x00,0x47,0x80,0x7F,0x00,0x00,

"网",   0x00,0xFE,0x02,0x22,0x42,0x82,0x72,0x02,  //12
	0x22,0x42,0x82,0x72,0x02,0xFE,0x00,0x00,
	0x00,0xFF,0x10,0x08,0x06,0x01,0x0E,0x10,
	0x08,0x06,0x01,0x4E,0x80,0x7F,0x00,0x00,

"络",   0x20,0x30,0xAC,0x63,0x30,0x20,0x10,0x18,  //13
	0xA7,0x44,0xA4,0x14,0x0C,0x00,0x00,0x00,
	0x22,0x67,0x22,0x12,0x12,0x04,0x02,0xFD,
	0x44,0x44,0x44,0x45,0xFD,0x02,0x02,0x00,


"服",   0x00,0x00,0xFE,0x22,0x22,0x22,0xFE,0x00,  //14
        0xFE,0x82,0x82,0x92,0xA2,0x9E,0x00,0x00,
        0x80,0x60,0x1F,0x02,0x42,0x82,0x7F,0x00,
	0xFF,0x40,0x2F,0x10,0x2C,0x43,0x80,0x00,

"务",	0x00,0x00,0x90,0x88,0x4C,0x57,0xA4,0x24,    //15
        0x54,0x54,0x8C,0x84,0x00,0x00,0x00,0x00,
        0x01,0x01,0x80,0x42,0x22,0x1A,0x07,0x02,
	0x42,0x82,0x42,0x3E,0x01,0x01,0x01,0x00,

"无",   0x00,0x40,0x42,0x42,0x42,0xC2,0x7E,0x42,  //16
        0xC2,0x42,0x42,0x42,0x40,0x40,0x00,0x00,
        0x80,0x40,0x20,0x10,0x0C,0x03,0x00,0x00,
	0x3F,0x40,0x40,0x40,0x40,0x70,0x00,0x00,

"信",   0x00,0x80,0x60,0xF8,0x07,0x00,0x04,0x24,  //17
        0x24,0x25,0x26,0x24,0x24,0x24,0x04,0x00,
        0x01,0x00,0x00,0xFF,0x00,0x00,0x00,0xF9,
	0x49,0x49,0x49,0x49,0x49,0xF9,0x00,0x00,

"号",   0x80,0x80,0x80,0xBE,0xA2,0xA2,0xA2,0xA2,  //18
        0xA2,0xA2,0xA2,0xBE,0x80,0x80,0x80,0x00,
        0x00,0x00,0x00,0x06,0x05,0x04,0x04,0x04,
	0x44,0x84,0x44,0x3C,0x00,0x00,0x00,0x00,

"强",   0x02,0xE2,0x22,0x22,0x3E,0x00,0x80,0x9E,  //19
        0x92,0x92,0xF2,0x92,0x92,0x9E,0x80,0x00,
        0x00,0x43,0x82,0x42,0x3E,0x40,0x47,0x44,
	0x44,0x44,0x7F,0x44,0x44,0x54,0xE7,0x00,

"远",   0x40,0x42,0x4C,0xC4,0x20,0x22,0x22,0xE2,  //20
	0x22,0x22,0xE2,0x22,0x22,0x20,0x20,0x00,
	0x00,0x40,0x20,0x1F,0x20,0x48,0x44,0x43,
	0x40,0x40,0x47,0x48,0x48,0x48,0x4E,0x00,

"程",   0x10,0x12,0xD2,0xFE,0x91,0x11,0x80,0xBF, //21
	0xA1,0xA1,0xA1,0xA1,0xBF,0x80,0x00,0x00,
	0x04,0x03,0x00,0xFF,0x00,0x41,0x44,0x44,
	0x44,0x7F,0x44,0x44,0x44,0x44,0x40,0x00,


"连",   0x40,0x40,0x42,0xCC,0x00,0x04,0x44,0x64,  //22
        0x5C,0x47,0xF4,0x44,0x44,0x44,0x04,0x00,
        0x00,0x40,0x20,0x1F,0x20,0x44,0x44,0x44,
        0x44,0x44,0x7F,0x44,0x44,0x44,0x44,0x00,

"接",   0x10,0x10,0x10,0xFF,0x10,0x50,0x44,0x54,  //23
        0x65,0xC6,0x44,0x64,0x54,0x44,0x40,0x00,
        0x04,0x44,0x82,0x7F,0x01,0x82,0x82,0x4A,
        0x56,0x23,0x22,0x52,0x4E,0x82,0x02,0x00,

"成",   0x00,0x00,0xF8,0x88,0x88,0x88,0x88,0x08,  //24
	0x08,0xFF,0x08,0x09,0x0A,0xC8,0x08,0x00,
	0x80,0x60,0x1F,0x00,0x10,0x20,0x1F,0x80,
	0x40,0x21,0x16,0x18,0x26,0x41,0xF8,0x00,

"功",   0x08,0x08,0x08,0xF8,0x08,0x08,0x08,0x10,  //25
	0x10,0xFF,0x10,0x10,0x10,0xF0,0x00,0x00,
	0x10,0x30,0x10,0x1F,0x08,0x88,0x48,0x30,
	0x0E,0x01,0x40,0x80,0x40,0x3F,0x00,0x00,

"失",   0x00,0x40,0x30,0x1E,0x10,0x10,0x10,0xFF, //26
	0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x00,
	0x81,0x81,0x41,0x21,0x11,0x0D,0x03,0x01,
	0x03,0x0D,0x11,0x21,0x41,0x81,0x81,0x00,

"败",   0x00,0xFE,0x02,0xFA,0x02,0xFE,0x40,0x20,  //27
	0xD8,0x17,0x10,0x10,0xF0,0x10,0x10,0x00,
	0x80,0x47,0x30,0x0F,0x10,0x67,0x80,0x40,
	0x21,0x16,0x08,0x16,0x21,0x40,0x80,0x00,

"状",   0x00,0x08,0x30,0x00,0xFF,0x20,0x20,0x20,//28
	0x20,0xFF,0x20,0x20,0x22,0x2C,0x20,0x00,
	0x04,0x04,0x02,0x01,0xFF,0x80,0x40,0x30,
	0x0E,0x01,0x06,0x18,0x20,0x40,0x80,0x00,

"态",   0x00,0x04,0x84,0x84,0x44,0x24,0x54,0x8F,//29
	0x14,0x24,0x44,0x84,0x84,0x04,0x00,0x00,
	0x41,0x39,0x00,0x00,0x3C,0x40,0x40,0x42,
	0x4C,0x40,0x40,0x70,0x04,0x09,0x31,0x00,

"已",   0x00,0x00,0xE2,0x82,0x82,0x82,0x82,0x82,//30
        0x82,0x82,0x82,0xFE,0x00,0x00,0x00,0x00,
        0x00,0x00,0x3F,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x78,0x00,0x00,

"未",   0x80,0x80,0x88,0x88,0x88,0x88,0x88,0xFF, //31
        0x88,0x88,0x88,0x88,0x88,0x80,0x80,0x00,
        0x20,0x20,0x10,0x08,0x04,0x02,0x01,0xFF,
	0x01,0x02,0x04,0x08,0x10,0x20,0x20,0x00,

"阀",   0x00,0xF8,0x81,0x42,0xE0,0x9A,0x82,0xFA,//32
        0x42,0x4A,0x52,0x42,0x02,0xFE,0x00,0x00,
        0x00,0xFF,0x00,0x00,0x7F,0x00,0x20,0x13,
	0x0C,0x14,0x23,0x78,0x80,0x7F,0x00,0x00,

"控",   0x10,0x10,0x10,0xFF,0x90,0x20,0x98,0x48,//33
        0x28,0x09,0x0E,0x28,0x48,0xA8,0x18,0x00,
        0x02,0x42,0x81,0x7F,0x00,0x40,0x40,0x42,
	0x42,0x42,0x7E,0x42,0x42,0x42,0x40,0x00,

"量",   0x20,0x20,0x20,0xBE,0xAA,0xAA,0xAA,0xAA,//34
	0xAA,0xAA,0xAA,0xBE,0x20,0x20,0x20,0x00,
	0x00,0x80,0x80,0xAF,0xAA,0xAA,0xAA,0xFF,
	0xAA,0xAA,0xAA,0xAF,0x80,0x80,0x00,0x00,


"报",   0x10,0x10,0x10,0xFF,0x10,0x90,0x00,0xFE,//35
        0x82,0x82,0x82,0x92,0xA2,0x9E,0x00,0x00,
        0x04,0x44,0x82,0x7F,0x01,0x00,0x00,0xFF,
	0x80,0x43,0x2C,0x10,0x2C,0x43,0x80,0x00,

"警",   0x12,0xEA,0xAF,0xAA,0xEA,0x0F,0xFA,0x02,//36
        0x88,0x8C,0x57,0x24,0x54,0x8C,0x84,0x00,
        0x02,0x02,0xEA,0xAA,0xAA,0xAB,0xAA,0xAB,
	0xAA,0xAA,0xAA,0xAA,0xEA,0x02,0x02,0x00,

"息",   0x00,0x00,0x00,0xFC,0x54,0x54,0x56,0x55,//37
	0x54,0x54,0x54,0xFC,0x00,0x00,0x00,0x00,
	0x40,0x30,0x00,0x03,0x39,0x41,0x41,0x45,
	0x59,0x41,0x41,0x73,0x00,0x08,0x30,0x00,

"离",	0x04,0x04,0x04,0xF4,0x84,0xD4,0xA5,0xA6,//38
	0xA4,0xD4,0x84,0xF4,0x04,0x04,0x04,0x00,
	0x00,0xFE,0x02,0x02,0x12,0x3A,0x16,0x13,
	0x12,0x1A,0x32,0x42,0x82,0x7E,0x00,0x00,

"线",	0x20,0x30,0xAC,0x63,0x20,0x18,0x80,0x90,//39
	0x90,0xFF,0x90,0x49,0x4A,0x48,0x40,0x00,
	0x22,0x67,0x22,0x12,0x12,0x12,0x40,0x40,
	0x20,0x13,0x0C,0x14,0x22,0x41,0xF8,0x00,

"故",	0x10,0x10,0x10,0xFF,0x10,0x10,0x50,0x20,//40
	0xD8,0x17,0x10,0x10,0xF0,0x10,0x10,0x00,
	0x00,0x7F,0x21,0x21,0x21,0x7F,0x80,0x40,
	0x21,0x16,0x08,0x16,0x21,0x40,0x80,0x00,

"障",	0x00,0xFE,0x02,0x22,0xDA,0x06,0x10,0xD2,//41
	0x56,0x5A,0x53,0x5A,0x56,0xD2,0x10,0x00,
	0x00,0xFF,0x08,0x10,0x08,0x07,0x10,0x17,
	0x15,0x15,0xFD,0x15,0x15,0x17,0x10,0x00,

"供",	0x00,0x80,0x60,0xF8,0x07,0x00,0x10,0xFF,//42
	0x10,0x10,0x10,0xFF,0x10,0x10,0x00,0x00,
	0x01,0x00,0x00,0xFF,0x80,0x42,0x22,0x1B,
	0x02,0x02,0x02,0x0B,0x32,0xC2,0x02,0x00,

"回",	0x00,0x00,0xFE,0x02,0x02,0xF2,0x12,0x12,//43
	0x12,0xF2,0x02,0x02,0xFE,0x00,0x00,0x00,
	0x00,0x00,0x7F,0x20,0x20,0x27,0x24,0x24,
	0x24,0x27,0x20,0x20,0x7F,0x00,0x00,0x00,

"水",	0x00,0x20,0x20,0x20,0xA0,0x60,0x00,0xFF,//44
	0x60,0x80,0x40,0x20,0x18,0x00,0x00,0x00,
	0x20,0x10,0x08,0x06,0x01,0x40,0x80,0x7F,
	0x00,0x01,0x02,0x04,0x08,0x10,0x10,0x00,

"累",   0x00,0x00,0x3E,0x2A,0x2A,0xAA,0x6A,0x3E,//45
	0x2A,0x2A,0xAA,0x2A,0x3E,0x00,0x00,0x00,
	0x00,0x80,0x48,0x29,0x09,0x4D,0x8D,0x7B,
	0x0B,0x09,0x28,0x4C,0x98,0x00,0x00,0x00,

"计",   0x40,0x40,0x42,0xCC,0x00,0x40,0x40,0x40,//46
	0x40,0xFF,0x40,0x40,0x40,0x40,0x40,0x00,
	0x00,0x00,0x00,0x7F,0x20,0x10,0x00,0x00,
	0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,

"温",   0x10,0x60,0x02,0x8C,0x00,0x00,0xFE,0x92,//47
	0x92,0x92,0x92,0x92,0xFE,0x00,0x00,0x00,
	0x04,0x04,0x7E,0x01,0x40,0x7E,0x42,0x42,
	0x7E,0x42,0x7E,0x42,0x42,0x7E,0x40,0x00,

"度",   0x00,0x00,0xFC,0x24,0x24,0x24,0xFC,0x25,//48
	0x26,0x24,0xFC,0x24,0x24,0x24,0x04,0x00,
	0x40,0x30,0x8F,0x80,0x84,0x4C,0x55,0x25,
	0x25,0x25,0x55,0x4C,0x80,0x80,0x80,0x00,


"地",   0x20,0x20,0x20,0xFF,0x20,0x20,0x80,0xF8,//49
	0x80,0x40,0xFF,0x20,0x10,0xF0,0x00,0x00,
	0x10,0x30,0x10,0x0F,0x08,0x08,0x00,0x3F,
	0x40,0x40,0x5F,0x42,0x44,0x43,0x78,0x00,

"址",   0x20,0x20,0x20,0xFF,0x20,0x20,0x00,0xF8,//50
	0x00,0x00,0xFF,0x40,0x40,0x40,0x00,0x00,
	0x10,0x30,0x10,0x0F,0x08,0x48,0x40,0x7F,
	0x40,0x40,0x7F,0x40,0x40,0x40,0x40,0x00,

"时",   0x00,0xFC,0x84,0x84,0x84,0xFC,0x00,0x10,//51
	0x10,0x10,0x10,0x10,0xFF,0x10,0x10,0x00,
	0x00,0x3F,0x10,0x10,0x10,0x3F,0x00,0x00,
	0x01,0x06,0x40,0x80,0x7F,0x00,0x00,0x00,

"间",   0x00,0xF8,0x01,0x06,0x00,0xF0,0x12,0x12,//52
	0x12,0xF2,0x02,0x02,0x02,0xFE,0x00,0x00,
	0x00,0xFF,0x00,0x00,0x00,0x1F,0x11,0x11,
	0x11,0x1F,0x00,0x40,0x80,0x7F,0x00,0x00,

"结",   0x20,0x30,0xAC,0x63,0x20,0x18,0x08,0x48,//53
	0x48,0x48,0x7F,0x48,0x48,0x48,0x08,0x00,
	0x22,0x67,0x22,0x12,0x12,0x12,0x00,0xFE,
	0x42,0x42,0x42,0x42,0x42,0xFE,0x00,0x00,

"算",   0x08,0x04,0xF3,0x52,0x56,0x52,0x52,0x50,//54
	0x54,0x53,0x52,0x56,0xF2,0x02,0x02,0x00,
	0x10,0x10,0x97,0x55,0x3D,0x15,0x15,0x15,
	0x15,0x15,0xFD,0x15,0x17,0x10,0x10,0x00,

"冷",   0x00,0x02,0x0C,0xC0,0x00,0x40,0x20,0x10,//55
	0x0C,0x23,0xCC,0x10,0x20,0x40,0x40,0x00,
	0x02,0x02,0x7F,0x00,0x00,0x00,0x01,0x09,
	0x11,0x21,0xD1,0x0D,0x03,0x00,0x00,0x00,

"流",   0x10,0x60,0x02,0x8C,0x00,0x44,0x64,0x54,//56
	0x4D,0x46,0x44,0x54,0x64,0xC4,0x04,0x00,
	0x04,0x04,0x7E,0x01,0x80,0x40,0x3E,0x00,
	0x00,0xFE,0x00,0x00,0x7E,0x80,0xE0,0x00,

"瞬",   0xFC,0x24,0x24,0xFC,0x80,0x62,0xAA,0x32,//57
	0x22,0x26,0x29,0x21,0xB1,0x2D,0x60,0x00,
	0x3F,0x11,0x11,0x3F,0x88,0x44,0x2B,0x12,
	0x0E,0x00,0x1A,0x12,0xFF,0x12,0x12,0x00,

"差",  0x00,0x04,0x24,0x24,0x25,0x26,0xE4,0x3C,//58
	0x24,0x26,0x25,0x24,0x24,0x04,0x00,0x00,
	0x41,0x21,0x11,0x89,0x85,0x8B,0x89,0x89,
	0xF9,0x89,0x89,0x89,0x89,0x81,0x01,0x00,

"△",   0x00,0x00,0x00,0x00,0x80,0x60,0x18,0x06,//59
	0x18,0x60,0x80,0x00,0x00,0x00,0x00,0x00,
	0x00,0x20,0x38,0x26,0x21,0x20,0x20,0x20,
	0x20,0x20,0x21,0x26,0x38,0x20,0x00,0x00,

"℃",   0x06,0x09,0x09,0xE6,0xF8,0x0C,0x04,0x02,//60	摄氏度符号
	0x02,0x02,0x02,0x02,0x04,0x1E,0x00,0x00,
	0x00,0x00,0x00,0x07,0x1F,0x30,0x20,0x40,
	0x40,0x40,0x40,0x40,0x20,0x10,0x00,0x00,

"欢", 0x04,0x34,0xC4,0x04,0xC4,0x3C,0x20,0x10, //61
      0x0F,0xE8,0x08,0x08,0x28,0x18,0x00,0x00,
      0x10,0x08,0x06,0x01,0x82,0x8C,0x40,0x30,
      0x0C,0x03,0x0C,0x10,0x60,0xC0,0x40,0x00,

"迎", 0x40,0x42,0x44,0xC8,0x00,0xFC,0x04,0x02,//62
      0x82,0xFC,0x04,0x04,0x04,0xFE,0x04,0x00,
      0x00,0x40,0x20,0x1F,0x20,0x47,0x42,0x41,
      0x40,0x7F,0x40,0x42,0x44,0x63,0x20,0x00,

"使", 0x40,0x20,0xF8,0x07,0x04,0xF4,0x14,0x14,  //63
      0x14,0xFF,0x14,0x14,0x14,0xF6,0x04,0x00,
      0x00,0x00,0xFF,0x00,0x80,0x43,0x45,0x29,
      0x19,0x17,0x21,0x21,0x41,0xC3,0x40,0x00,

"用", 0x00,0x00,0xFE,0x22,0x22,0x22,0x22,0xFE,  //64
      0x22,0x22,0x22,0x22,0xFF,0x02,0x00,0x00,
      0x80,0x60,0x1F,0x02,0x02,0x02,0x02,0x7F,
      0x02,0x02,0x42,0x82,0x7F,0x00,0x00,0x00,

"仪", 0x00,0x80,0x60,0xF8,0x07,0x00,0x1C,0xE0,  //65
      0x01,0x06,0x00,0xE0,0x1E,0x00,0x00,0x00,
      0x01,0x00,0x00,0xFF,0x00,0x80,0x40,0x20,
      0x13,0x0C,0x13,0x20,0x40,0x80,0x80,0x00,

"注", 	0x10,0x60,0x02,0x8C,0x00,0x08,0x08,0x08,  //66
	0x09,0xFA,0x08,0x08,0x08,0x08,0x00,0x00,
	0x04,0x04,0x7E,0x01,0x40,0x40,0x41,0x41,
	0x41,0x7F,0x41,0x41,0x41,0x41,0x40,0x00,

"册", 	0x80,0x80,0x80,0xFE,0x82,0x82,0xFE,0x80, //67
	0x80,0xFE,0x82,0x82,0xFE,0x80,0x80,0x00,
	0x00,0x80,0x40,0x3F,0x00,0x40,0x7F,0x80,
	0x60,0x1F,0x40,0x80,0x7F,0x00,0x00,0x00,

"没",	0x10,0x60,0x02,0xCC,0x80,0x40,0x20,0x1E,  //68
	0x02,0x02,0x02,0x3E,0x40,0x40,0x40,0x00,
	0x04,0x04,0x7E,0x01,0x80,0x80,0x83,0x4D,
	0x51,0x21,0x51,0x49,0x87,0x80,0x80,0x00,

"有",	0x04,0x04,0x04,0x84,0xE4,0x3C,0x27,0x24,  //69
	0x24,0x24,0x24,0xE4,0x04,0x04,0x04,0x00,
	0x04,0x02,0x01,0x00,0xFF,0x09,0x09,0x09,
	0x09,0x49,0x89,0x7F,0x00,0x00,0x00,0x00,

"请",	0x40,0x42,0xCC,0x00,0x00,0x44,0x54,0x54,  //70
	0x54,0x7F,0x54,0x54,0x54,0x44,0x40,0x00,
	0x00,0x00,0x7F,0x20,0x10,0x00,0xFF,0x15,
	0x15,0x15,0x55,0x95,0x7F,0x00,0x00,0x00,

"后",	0x00,0x00,0x00,0xFC,0x24,0x24,0x24,0x24, //71
	0x22,0x22,0x22,0x23,0x22,0x20,0x20,0x00,
	0x40,0x20,0x18,0x07,0x00,0xFE,0x42,0x42,
	0x42,0x42,0x42,0x42,0xFE,0x00,0x00,0x00,

"按",	0x10,0x10,0x10,0xFF,0x90,0x20,0x98,0x88,  //72
	0x88,0xE9,0x8E,0x88,0x88,0xA8,0x98,0x00,
	0x02,0x42,0x81,0x7F,0x00,0x00,0x80,0x84,
	0x4B,0x28,0x10,0x28,0x47,0x80,0x00,0x00,

"键",	0x40,0x30,0xEF,0x24,0x24,0x80,0xE4,0x9C,  //73
	0x10,0x54,0x54,0xFF,0x54,0x7C,0x10,0x00,
	0x01,0x01,0x7F,0x21,0x51,0x26,0x18,0x27,
	0x44,0x45,0x45,0x5F,0x45,0x45,0x44,0x00,

"等",	0x08,0x04,0x23,0x22,0x26,0x2A,0x22,0xFA,  //74
	0x24,0x23,0x22,0x26,0x2A,0x02,0x02,0x00,
	0x01,0x09,0x09,0x09,0x19,0x69,0x09,0x09,
	0x49,0x89,0x7D,0x09,0x09,0x09,0x01,0x00,

"待",	0x00,0x10,0x88,0xC4,0x33,0x40,0x48,0x48,  //75
	0x48,0x7F,0x48,0xC8,0x48,0x48,0x40,0x00,
	0x02,0x01,0x00,0xFF,0x00,0x02,0x0A,0x32,
	0x02,0x42,0x82,0x7F,0x02,0x02,0x02,0x00,

"正", 	0x00,0x02,0x02,0xC2,0x02,0x02,0x02,0x02,  //76
	0xFE,0x82,0x82,0x82,0x82,0x82,0x02,0x00,
	0x20,0x20,0x20,0x3F,0x20,0x20,0x20,0x20,
	0x3F,0x20,0x20,0x20,0x20,0x20,0x20,0x00,

"在", 	0x00,0x04,0x04,0xC4,0x64,0x9C,0x87,0x84,  //77
	0x84,0xE4,0x84,0x84,0x84,0x84,0x04,0x00,
	0x04,0x02,0x01,0x7F,0x00,0x20,0x20,0x20,
	0x20,0x3F,0x20,0x20,0x20,0x20,0x20,0x00,

"设", 	0x40,0x41,0xCE,0x04,0x00,0x80,0x40,0xBE, //78
	0x82,0x82,0x82,0xBE,0xC0,0x40,0x40,0x00,
	0x00,0x00,0x7F,0x20,0x90,0x80,0x40,0x43,
	0x2C,0x10,0x10,0x2C,0x43,0xC0,0x40,0x00,

"置",	0x00,0x20,0x2F,0xA9,0xA9,0xAF,0xE9,0xB9, //79
	0xA9,0xAF,0xA9,0xA9,0x2F,0x20,0x00,0x00,
	0x80,0x80,0x80,0xFF,0xAA,0xAA,0xAA,0xAA,
	0xAA,0xAA,0xAA,0xFF,0x80,0x80,0x80,0x00,

"读", 	0x40,0x42,0xCC,0x04,0x00,0x50,0x94,0x34, //80
	0xD4,0x1F,0xD4,0x14,0x54,0x34,0x10,0x00,
	0x00,0x00,0x7F,0x20,0x10,0x82,0x43,0x22,
	0x12,0x0A,0x07,0x0A,0x12,0xE2,0x42,0x00,

"取", 	0x02,0x02,0xFE,0x92,0x92,0x92,0xFE,0x02, //81
	0x02,0x7C,0x84,0x04,0x84,0x7C,0x04,0x00,
	0x10,0x10,0x0F,0x08,0x08,0x04,0xFF,0x04,
	0x22,0x10,0x09,0x06,0x09,0x30,0x10,0x00,

"编", 	0x20,0x30,0xAC,0x63,0x32,0x00,0xFC,0xA4, //82
	0xA5,0xA6,0xA4,0xA4,0xA4,0xBC,0x00,0x00,
	0x10,0x11,0x09,0x49,0x21,0x1C,0x03,0x7F,
	0x04,0x3F,0x04,0x3F,0x44,0x7F,0x00,0x00,

"号",	0x40,0x40,0x40,0x5F,0xD1,0x51,0x51,0x51, //83
	0x51,0x51,0x51,0x5F,0x40,0x40,0x40,0x00,
	0x00,0x00,0x00,0x02,0x07,0x02,0x02,0x22,
	0x42,0x82,0x42,0x3E,0x00,0x00,0x00,0x00,

"户", 	0x00,0x00,0x00,0xF8,0x88,0x88,0x88,0x89, //84
	0x8A,0x8E,0x88,0x88,0x88,0xF8,0x00,0x00,
	0x80,0x40,0x30,0x0F,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

"设",	0x40,0x41,0xCE,0x04,0x00,0x80,0x40,0xBE, //85
	0x82,0x82,0x82,0xBE,0xC0,0x40,0x40,0x00,
	0x00,0x00,0x7F,0x20,0x90,0x80,0x40,0x43,
	0x2C,0x10,0x10,0x2C,0x43,0xC0,0x40,0x00,

"备",	0x00,0x20,0x10,0x08,0x87,0x8A,0x52,0x22,//86
	0x22,0x52,0x8E,0x82,0x00,0x00,0x00,0x00,
	0x02,0x02,0x01,0xFF,0x4A,0x4A,0x4A,0x7E,
	0x4A,0x4A,0x4A,0xFF,0x01,0x03,0x01,0x00,

"端",	0x50,0x91,0x16,0x10,0xF0,0x10,0x40,0x5E, //87
	0x50,0x50,0xDF,0x50,0x50,0x5E,0x40,0x00,
	0x10,0x13,0x10,0x0F,0x08,0x08,0xFF,0x01,
	0x01,0x3F,0x01,0x3F,0x41,0x81,0x7F,0x00,

"口",	0x00,0x00,0xFC,0x04,0x04,0x04,0x04,0x04, //88
	0x04,0x04,0x04,0x04,0xFC,0x00,0x00,0x00,
	0x00,0x00,0x3F,0x08,0x08,0x08,0x08,0x08,
	0x08,0x08,0x08,0x08,0x3F,0x00,0x00,0x00,
"确",	0x00,0x84,0xE4,0x5C,0x44,0xC4,0x10,0xF8, //89
	0x97,0x92,0xF2,0x9A,0x96,0xF2,0x00,0x00,
	0x01,0x00,0x3F,0x08,0x88,0x4F,0x30,0x0F,
	0x04,0x04,0x3F,0x44,0x84,0x7F,0x00,0x00,

"认",	0x40,0x41,0x42,0xCC,0x04,0x00,0x00,0x00,//90
	0x80,0x7F,0x80,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x7F,0x20,0x90,0x60,0x18,
	0x07,0x00,0x03,0x0C,0x30,0xC0,0x40,0x00,


"删",	0x80,0xFE,0x82,0x82,0xFE,0x80,0xFE,0x82, //91
	0x82,0xFE,0x80,0x00,0xF8,0x00,0xFF,0x00,
	0x40,0x3F,0x00,0x20,0xBF,0x40,0x3F,0x40,
	0x80,0x7F,0x00,0x00,0x4F,0x80,0x7F,0x00,

"除",	0x00,0xFE,0x22,0x5A,0x86,0x20,0x10,0x28,//92
	0x24,0xE3,0x24,0x28,0x10,0x20,0x20,0x00,
	0x00,0xFF,0x04,0x08,0x27,0x11,0x0D,0x41,
	0x81,0x7F,0x01,0x05,0x09,0x31,0x00,0x00,

"取",	0x02,0x02,0xFE,0x92,0x92,0x92,0xFE,0x02,//93
	0x06,0xFC,0x04,0x04,0x04,0xFC,0x00,0x00,
	0x08,0x18,0x0F,0x08,0x08,0x04,0xFF,0x04,
	0x84,0x40,0x27,0x18,0x27,0x40,0x80,0x00,

"消",	0x10,0x60,0x02,0x0C,0xC0,0x00,0xE2,0x2C,//94
	0x20,0x3F,0x20,0x28,0xE6,0x00,0x00,0x00,
	0x04,0x04,0x7C,0x03,0x00,0x00,0xFF,0x09,
	0x09,0x09,0x49,0x89,0x7F,0x00,0x00,0x00,

"第",	0x08,0x04,0x93,0x92,0x96,0x9A,0x92,0xFA, //95
	0x94,0x93,0x92,0x96,0xFA,0x02,0x02,0x00,
	0x40,0x40,0x47,0x24,0x24,0x14,0x0C,0xFF,
	0x04,0x04,0x24,0x44,0x24,0x1C,0x00,0x00,

"显",	0x00,0x00,0x00,0xFE,0x92,0x92,0x92,0x92,//96
	0x92,0x92,0x92,0xFE,0x00,0x00,0x00,0x00,
	0x40,0x42,0x44,0x58,0x40,0x7F,0x40,0x40,
	0x40,0x7F,0x40,0x50,0x48,0x46,0x40,0x00,

"示",	0x40,0x40,0x42,0x42,0x42,0x42,0x42,0xC2,//97
	0x42,0x42,0x42,0x42,0x42,0x40,0x40,0x00,
	0x20,0x10,0x08,0x06,0x00,0x40,0x80,0x7F,
	0x00,0x00,0x00,0x02,0x04,0x08,0x30,0x00,

"退",	0x40,0x40,0x42,0xCC,0x00,0x00,0xFF,0x49,//98
	0x49,0xC9,0x49,0x49,0x7F,0x80,0x00,0x00,
	0x00,0x40,0x20,0x1F,0x20,0x40,0x5F,0x48,
	0x44,0x40,0x41,0x42,0x45,0x58,0x40,0x00,

"出",	0x00,0x00,0x7C,0x40,0x40,0x40,0x40,0xFF,//99
	0x40,0x40,0x40,0x40,0xFC,0x00,0x00,0x00,
	0x00,0x7C,0x40,0x40,0x40,0x40,0x40,0x7F,
	0x40,0x40,0x40,0x40,0x40,0xFC,0x00,0x00,


"输",	0x88,0x68,0x1F,0xC8,0x08,0x10,0xC8,0x54,//100
	0x52,0xD1,0x12,0x94,0x08,0xD0,0x10,0x00,
	0x09,0x19,0x09,0xFF,0x05,0x00,0xFF,0x12,
	0x92,0xFF,0x00,0x5F,0x80,0x7F,0x00,0x00,

"入",	0x00,0x00,0x00,0x00,0x00,0x01,0xE2,0x1C,//101
	0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x80,0x40,0x20,0x10,0x0C,0x03,0x00,0x00,
	0x00,0x03,0x0C,0x30,0x40,0x80,0x80,0x00,

"密",	0x10,0x8C,0x44,0x04,0xE4,0x04,0x95,0xA6,//102
	0x44,0x24,0x14,0x84,0x44,0x94,0x0C,0x00,
	0x02,0x02,0x7A,0x41,0x41,0x43,0x42,0x7E,
	0x42,0x42,0x42,0x43,0xF8,0x00,0x00,0x00,

"码",	0x04,0x84,0xE4,0x5C,0x44,0xC4,0x00,0x02,//103
	0xF2,0x82,0x82,0x82,0xFE,0x80,0x80,0x00,
	0x02,0x01,0x7F,0x10,0x10,0x3F,0x00,0x08,
	0x08,0x08,0x08,0x48,0x88,0x40,0x3F,0x00,

"模",	0x10,0x10,0xD0,0xFF,0x90,0x14,0xE4,0xAF,//104
	0xA4,0xA4,0xA4,0xAF,0xE4,0x04,0x00,0x00,
	0x04,0x03,0x00,0xFF,0x00,0x89,0x4B,0x2A,
	0x1A,0x0E,0x1A,0x2A,0x4B,0x88,0x80,0x00,

"块",	0x20,0x20,0x20,0xFF,0x20,0x20,0x08,0x08,//105
	0x08,0xFF,0x08,0x08,0x08,0xF8,0x00,0x00,
	0x10,0x30,0x10,0x0F,0x88,0x48,0x21,0x11,
	0x0D,0x03,0x0D,0x11,0x21,0x41,0x81,0x00,

"通",	0x40,0x42,0xCC,0x00,0x00,0xE2,0x22,0x2A,//106
	0x2A,0xF2,0x2A,0x26,0x22,0xE0,0x00,0x00,
	0x80,0x40,0x3F,0x40,0x80,0xFF,0x89,0x89,
	0x89,0xBF,0x89,0xA9,0xC9,0xBF,0x80,0x00,

"任",	0x00,0x80,0x60,0xF8,0x87,0x80,0x84,0x84,//107
	0x84,0xFE,0x82,0x83,0x82,0x80,0x80,0x00,
	0x01,0x00,0x00,0xFF,0x00,0x40,0x40,0x40,
	0x40,0x7F,0x40,0x40,0x40,0x40,0x00,0x00,

"意",	0x10,0x10,0x12,0xD2,0x56,0x5A,0x52,0x53,//108
	0x52,0x5A,0x56,0xD2,0x12,0x10,0x10,0x00,
	0x40,0x30,0x00,0x77,0x85,0x85,0x8D,0xB5,
	0x85,0x85,0x85,0xE7,0x00,0x10,0x60,0x00,

"返",	0x40,0x40,0x42,0xCC,0x00,0x00,0xFC,0x24,//109
	0xA4,0x24,0x22,0x22,0xA3,0x62,0x00,0x00,
	0x00,0x40,0x20,0x1F,0x20,0x58,0x47,0x50,
	0x48,0x45,0x42,0x45,0x48,0x50,0x40,0x00,

"回",	0x00,0x00,0xFE,0x02,0x02,0xF2,0x12,0x12,//110
	0x12,0xF2,0x02,0x02,0xFE,0x00,0x00,0x00,
	0x00,0x00,0x7F,0x20,0x20,0x27,0x24,0x24,
	0x24,0x27,0x20,0x20,0x7F,0x00,0x00,0x00,

"‰",   0x00,0x3C,0x42,0x42,0x3C,0x80,0x40,0x20,//111
	0x10,0x08,0x04,0x02,0x01,0x00,0x00,0x00,
	0x10,0x08,0x04,0x02,0x01,0x3C,0x42,0x42,
	0x3C,0x00,0x00,0x3C,0x42,0x42,0x3C,0x00,

"㎥",	0x00,0xF0,0x60,0x80,0x00,0x00,0x00,0x80//112   立方米单位
    , 0x60,0xF0,0x00,0x44,0x82,0x92,0x6C,0x00
    , 0x00,0x7F,0x00,0x03,0x1C,0x60,0x1C,0x03
    , 0x00,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,

"常", 0x20,0x18,0x08,0xEA,0xAC,0xA8,0xA8,0xAF //113
    , 0xA8,0xA8,0xAC,0xEA,0x08,0x28,0x18,0x00
    , 0x00,0x00,0x3E,0x02,0x02,0x02,0x02,0xFF
    ,0x02,0x02,0x12,0x22,0x1E,0x00,0x00,0x00,

"刷", 0x00,0x00,0xFE,0x12,0x12,0x12,0xF2,0x12, //114
	0x12,0x1E,0x00,0xF0,0x00,0x00,0xFF,0x00,
	0x10,0x0E,0x01,0x3F,0x01,0x01,0xFF,0x11,
	0x21,0x1F,0x00,0x0F,0x40,0x80,0x7F,0x00,

"卡", 0x40,0x40,0x40,0x40,0x40,0x40,0xFF,0x44,//115
	0x44,0x44,0x44,0x44,0x44,0x40,0x40,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,
	0x00,0x02,0x04,0x08,0x10,0x00,0x00,0x00,

"到", 0x42,0x62,0x52,0x4A,0xC6,0x42,0x52,0x62,//116
	0xC2,0x00,0xF8,0x00,0x00,0xFF,0x00,0x00,
	0x40,0xC4,0x44,0x44,0x7F,0x24,0x24,0x24,
	0x20,0x00,0x0F,0x40,0x80,0x7F,0x00,0x00,

"授", 0x10,0x10,0x10,0xFF,0x10,0xD0,0x4A,0x72,//117
	0x46,0x5A,0x42,0x61,0x51,0x4D,0xC0,0x00,
	0x04,0x44,0x82,0x7F,0x01,0x80,0x82,0x4E,
	0x52,0x22,0x52,0x4A,0x86,0x80,0x80,0x00,

"权", 0x10,0x10,0xD0,0xFF,0x90,0x10,0x02,0x1E,//118
	0xE2,0x02,0x02,0x02,0xE2,0x1E,0x00,0x00,
	0x04,0x03,0x00,0xFF,0x00,0x83,0x80,0x40,
	0x20,0x13,0x0C,0x13,0x20,0x40,0x80,0x00,

"磁", 0x84,0xE4,0x5C,0xC4,0x00,0x08,0xC8,0x39,//119
	0x8E,0x08,0x08,0xCC,0x3B,0x88,0x08,0x00,
	0x00,0x3F,0x10,0x3F,0x00,0x63,0x5A,0x46,
	0xE1,0x00,0x63,0x5A,0x46,0xE1,0x00,0x00,

"盘", 0x20,0x20,0x20,0xFC,0x24,0x26,0xA5,0x2C,//120
	0x34,0x24,0x24,0xFC,0x20,0x20,0x20,0x00,
	0x40,0x42,0x7D,0x44,0x44,0x7C,0x44,0x45,
	0x44,0x7D,0x46,0x45,0x7C,0x40,0x40,0x00,

"份", 0x00,0x80,0x60,0xF8,0x07,0x80,0x40,0xB0,//121
	0x8E,0x80,0x80,0x87,0x98,0x60,0x80,0x00,
	0x01,0x00,0x00,0xFF,0x00,0x80,0x40,0x30,
	0x0F,0x00,0x40,0x80,0x7F,0x00,0x00,0x00,

"完", 0x10,0x0C,0x04,0x24,0x24,0x24,0x25,0x26,//122
	0x24,0x24,0x24,0x24,0x04,0x14,0x0C,0x00,
	0x00,0x81,0x81,0x41,0x31,0x0F,0x01,0x01,
	0x01,0x7F,0x81,0x81,0x81,0xF1,0x00,0x00,

"拔", 0x10,0x10,0x10,0xFF,0x90,0x40,0x10,0x10,//123
	0xF0,0x9F,0x90,0x91,0x96,0x90,0x10,0x00,
	0x02,0x42,0x81,0x7F,0x00,0x40,0x30,0x8F,
	0x80,0x43,0x2C,0x10,0x2C,0x43,0x80,0x00,

"启", 0x00,0x00,0x00,0xFC,0x44,0x44,0x44,0x45,//124
    0x46,0x44,0x44,0x44,0x44,0x7C,0x00,0x00,
    0x40,0x20,0x18,0x07,0x00,0xFC,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0xFC,0x00,0x00,

"当", 0x00,0x40,0x42,0x44,0x58,0x40,0x40,0x7F,//125
    0x40,0x40,0x50,0x48,0xC6,0x00,0x00,0x00,
    0x00,0x40,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0xFF,0x00,0x00,0x00,

"前", 0x08,0x08,0xE8,0x29,0x2E,0x28,0xE8,0x08,//126
    0x08,0xC8,0x0C,0x0B,0xE8,0x08,0x08,0x00,
    0x00,0x00,0xFF,0x09,0x49,0x89,0x7F,0x00,
    0x00,0x0F,0x40,0x80,0x7F,0x00,0x00,0x00,

"预", 0x42,0x4A,0xD2,0x6A,0x46,0xC0,0x00,0xF2,
    0x12,0x1A,0xD6,0x12,0x12,0xF2,0x02,0x00,
    0x40,0x80,0x7F,0x00,0x01,0x00,0x80,0x4F,
    0x20,0x18,0x07,0x10,0x20,0x4F,0x80,0x00,

"是", 0x00,0x00,0x00,0x7F,0x49,0x49,0x49,0x49,
    0x49,0x49,0x49,0x7F,0x00,0x00,0x00,0x00,
    0x81,0x41,0x21,0x1D,0x21,0x41,0x81,0xFF,
    0x89,0x89,0x89,0x89,0x89,0x81,0x81,0x00,

"否", 0x00,0x02,0x82,0x82,0x42,0x22,0x12,0xFA,
    0x06,0x22,0x22,0x42,0x42,0x82,0x00,0x00,
    0x01,0x01,0x00,0xFC,0x44,0x44,0x44,0x45,
    0x44,0x44,0x44,0xFC,0x00,0x00,0x01,0x00,
};



#ifdef __cplusplus
}
#endif
#endif

//END OF FILE
