#include <sys/wait.h>
#include <string.h>

#define __LCDDRV_C__
#include "lcddrv.h"
#define code

#include "bdk_font.h"

#ifndef CFG_TIMER_LIB
#define CFG_TIMER_LIB
#endif
#ifndef CFG_GPIO_LIB
#define CFG_GPIO_LIB 1
#endif
#include"libs_emsys_odm.h"

#define LCD_CS_ENABLE()        PIOOutValue(PB31, 0)
#define LCD_CS_DISABLE()    PIOOutValue(PB31, 1)

#define LCD_RESET_IN()        PIOOutValue(PB30, 0)
#define LCD_RESET_OUT()        PIOOutValue(PB30, 1)

#define LCD_LED_ON()           PIOOutValue(PB25, 1)
#define LCD_LED_OFF()        PIOOutValue(PB25, 0)

#define LCD_DOC_D()            PIOOutValue(PB29, 1)
#define LCD_DOC_C()            PIOOutValue(PB29, 0)

#define LCD_MC_HIGH()        PIOOutValue(PB28, 1)
#define LCD_MC_LOW()           PIOOutValue(PB28, 0)

#define LCD_OUT(B)            (B)? PIOOutValue(PB26, 1): PIOOutValue(PB26, 0)
#define LCD_OUT_SET()        PIOOutValue(PB26, 1)
#define LCD_OUT_RESET()        PIOOutValue(PB26, 0)

static uint8 Lcd_Flag_UpData;            //屏幕操作全局互锁标志

#define LCD_SPI_TX_DELAY()        usleep(30*1000)    //20    //2
#define LCD_GPIO_DELAY()        usleep(20*1000)

#define LCD_START_CMD()        LCD_SPI_TX_DELAY(),LCD_DOC_C()
#define LCD_START_DATA()    LCD_SPI_TX_DELAY(),LCD_DOC_D()

#define OS_ENTER_CRITICAL()
#define OS_EXIT_CRITICAL()

unsigned char DATA_D7_D0;

unsigned char LCD_user_part_flag;            //是否使用矩形框填充
#define ASC_TAB_OFFSET 0x20                    //ASC码表偏移量

#define LCD_CMD_SET_START_LINE(s_line)    \
    wdata((s_line&0x3f)|0x40, INVERT_OFF)

#define LCD_CMD_SET_PAGE(page_add)    \
    wdata((page_add&0x0f)|0xb0, INVERT_OFF)

#define INVERT_OFF  0
#define INVERT_ON  1
const char title[]=
{
0xFF,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xFF,
0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x08,0x88,0xFF,0x48,0x48,0x00,0x08,
0x48,0xFF,0x08,0x08,0xF8,0x00,0x00,0x00,0x00,0x10,0x10,0x10,0x10,0x10,0xFF,0x10,
0x10,0x10,0x10,0x10,0xF0,0x00,0x00,0x00,0x20,0x10,0x08,0xFC,0x57,0x54,0x54,0x55,
0xFE,0x54,0x54,0x54,0x54,0x04,0x00,0x00,0x00,0x00,0xF0,0x10,0x10,0x10,0x10,0xFF,
0x10,0x10,0x10,0x10,0xF0,0x00,0x00,0x00,0x80,0x80,0x9E,0x92,0x92,0x92,0x9E,0xE0,
0x80,0x9E,0xB2,0xD2,0x92,0x9E,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x81,0x65,0x08,0x07,0x20,0xC0,0x08,0x04,
0x23,0xC0,0x03,0x00,0x23,0xC4,0x0F,0x00,0x00,0x80,0x40,0x20,0x18,0x06,0x01,0x00,
0x20,0x40,0x80,0x40,0x3F,0x00,0x00,0x00,0x44,0x44,0x24,0x27,0x15,0x0D,0x05,0xFF,
0x05,0x0D,0x15,0x25,0x25,0x45,0x44,0x00,0x00,0x00,0x0F,0x04,0x04,0x04,0x04,0xFF,
0x04,0x04,0x04,0x04,0x0F,0x00,0x00,0x00,0x08,0x08,0xF4,0x94,0x92,0x92,0xF1,0x00,
0x01,0xF2,0x92,0x94,0x94,0xF8,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x10,0x60,0x02,0x8C,0x00,0x0C,0xA4,0xA4,0xA5,0xE6,0xA4,0xA4,0xA4,0x0C,0x00,
0x00,0x40,0x40,0x42,0x42,0x42,0x42,0x42,0xC2,0x42,0x42,0x42,0x42,0x42,0x40,0x40,
0x00,0x00,0x02,0x02,0x12,0x62,0x02,0x02,0xFE,0x02,0x02,0x42,0x32,0x02,0x02,0x00,
0x00,0x00,0x40,0xE0,0x50,0x48,0x44,0x43,0x40,0x40,0x40,0x48,0x50,0x60,0xC0,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x04,0x04,0x7E,0x01,0x00,0x80,0x4F,0x2A,0x0A,0x0F,0x0A,0x2A,0x4F,0x80,0x00,
0x00,0x20,0x10,0x08,0x06,0x00,0x40,0x80,0x7F,0x00,0x00,0x00,0x02,0x04,0x08,0x30,
0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xFF,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
0x00,0x00,0x00,0x00,0xFE,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0xFE,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
0xFF,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xFF,

};

void LcdGpin(void)    //初始化LCD的GPIO
{
    DATA_D7_D0=1;
    LCD_user_part_flag=0;
    LcdClearBuff();
    Lcd_Flag_UpData=0;
};

void LcdLedOn(void)
{
   LCD_LED_ON();
};

void LcdLedOff(void)
{
   LCD_LED_OFF();
};

unsigned char wdata(unsigned char d,unsigned char disp_invert)    //数据输出
{
    unsigned char i;
#if OS_CRITICAL_METHOD == 3
    OS_CPU_SR  cpu_sr = 0;
#endif
    OS_ENTER_CRITICAL();
    LCD_CS_ENABLE();
    for(i=0;i<8;i++)
    {
        LCD_MC_LOW();
        if(disp_invert)
        {
            if(d&0x80)
                LCD_OUT_RESET();
            else
                LCD_OUT_SET();
        }
        else
        {
            if(d&0x80)
                LCD_OUT_SET();
            else
                LCD_OUT_RESET();
        }
        d<<=1;
        LCD_MC_HIGH();
    }
    LCD_CS_DISABLE();
    OS_EXIT_CRITICAL();
    return d;
}

unsigned char wcom(unsigned char d)    //数据输出
{
    unsigned char i;
#if OS_CRITICAL_METHOD == 3
    OS_CPU_SR  cpu_sr = 0;
#endif
    OS_ENTER_CRITICAL();
    LCD_START_CMD();
    LCD_CS_ENABLE();
    for(i=0;i<8;i++)
    {
        LCD_MC_LOW();
        if(d&0x80)
            LCD_OUT_SET();
        else
            LCD_OUT_RESET();
        d<<=1;
        LCD_MC_HIGH();
    }
    LCD_CS_DISABLE();
    LCD_START_DATA();
    OS_EXIT_CRITICAL();
    return d;
}

void DelayUs(uint16 m_Delay)
{
   usleep(m_Delay*12);
}

void DelayMs(uint16 m_Delay)
{
   usleep(m_Delay*1000);
}

void LCD_CMD_SET_COLUMN(unsigned char colu_add)
{
    if(DATA_D7_D0)
    {
        wdata((((colu_add+4)>>4)&0x0f)|0x10, INVERT_OFF);
        wdata((colu_add+4)&0x0f, INVERT_OFF);
    }
    else
    {
        wdata((((colu_add+0)>>4)&0x0f)|0x10, INVERT_OFF);
        wdata((colu_add+0)&0x0f, INVERT_OFF);
    }
}


/*************************************************/
//清屏
/*************************************************/
void LCDClear(void)
{
    unsigned char page;
    unsigned char seg;
    for(page=0x0;page<=0x07;page++)   //写页地址共8页  0xb0----0xb7
    {
//        wcom(page);
//        wcom(0x10);
//        wcom(0x00);
        LCD_DOC_C();
        LCD_CMD_SET_PAGE(page);
        LCD_CMD_SET_COLUMN(0);
        LCD_DOC_D();
        for(seg=0;seg<128;seg++)
        {
            wdata(0x00,INVERT_OFF);
        }
    }
}

/*************************************************/
//写显示字符：8x8 只显示数字
/*************************************************/
void lcdwritechar8x8(char codenum ,char y ,char x,unsigned char disp_invert ,unsigned char mark_L ,unsigned char mark_R, unsigned char mark_T, unsigned char mark_B)
{
    unsigned char seg;
    unsigned char coden;
    unsigned char mark_bb;
    unsigned int x_pos;
    x_pos = x<<3;
    coden=codenum<<3;
    if(mark_B)
        mark_bb = 0x80;
    else
        mark_bb = 0;
    LCD_DOC_C();
    LCD_CMD_SET_PAGE(y);
    LCD_CMD_SET_COLUMN(x_pos);
    LCD_DOC_D();
    if(mark_L)
    {
        wdata(0xFF,INVERT_OFF);
        coden++;
    }
    else
    {
        /* wdata(nNumber8x8[coden++]|(mark_T & 0x01) | mark_bb,disp_invert); */
    }
    for(seg=1;seg<7;seg++)
    {
        /* wdata(nNumber8x8[coden++] |(mark_T & 0x01) | mark_bb, disp_invert); */
    }
    if(mark_R)
    {
        wdata(0xFF,INVERT_OFF);
    }
    else
    {
        /* wdata(nNumber8x8[coden++]|(mark_T & 0x01),disp_invert); */
    }
}

/*************************************************/
//写显示字符：8x16
/*************************************************/
void lcdwritechar(char codenum ,char y ,char x ,unsigned char disp_invert,
                    unsigned char mark_L ,unsigned char mark_R, unsigned char mark_T , unsigned char mark_B)
{
     unsigned char seg;
     unsigned int coden;
     unsigned char mark_bb;
     unsigned int x_pos;
     if(mark_B)
         mark_bb = 0x80;
     else
         mark_bb = 0;
     x_pos = x<<3;
     codenum-=0x20;
     coden=codenum<<4;
//     wcom(0xb0|(y&0x0f));//
//     wcom(0x10|((x>>1)&0x0f));
//     wcom(0x00|((x<<3)&0x0f));
    LCD_DOC_C();
    LCD_CMD_SET_PAGE(y);
    LCD_CMD_SET_COLUMN(x_pos);
    LCD_DOC_D();
     if(mark_L)
     {
         wdata(0xFF,INVERT_OFF);
         coden++;
     }
     else
     {
         wdata(nAsciiDot[coden++]|(mark_T & 0x01),disp_invert);
     }
     for(seg=1;seg<7;seg++)
     {
         wdata(nAsciiDot[coden++]|(mark_T & 0x01),disp_invert);
     }
     if(mark_R)
     {
         wdata(0xFF,INVERT_OFF);
         coden++;
     }
     else
     {
         wdata(nAsciiDot[coden++]|(mark_T & 0x01),disp_invert);
     }
//     wcom(0xb0|((y+1)&0x0f));
//     wcom(0x10|((x>>1)&0x0f));
//     wcom(0x00|((x<<3)&0x0f));
        LCD_DOC_C();
        LCD_CMD_SET_PAGE((y+1));
        LCD_CMD_SET_COLUMN(x_pos);
        LCD_DOC_D();
     if(mark_L)
     {
         wdata(0xFF,INVERT_OFF);
         coden++;
     }
     else
     {
         wdata(nAsciiDot[coden++] | mark_bb ,disp_invert);
     }
     for(seg=1;seg<7;seg++)
     {
         wdata(nAsciiDot[coden++] | mark_bb, disp_invert);
     }
     if(mark_R)
     {
         wdata(0xFF,INVERT_OFF);
         coden++;
     }
     else
     {
         wdata(nAsciiDot[coden++] | mark_bb, disp_invert);
     }

}
/*************************************************/
//写显示汉字 16x16
/*************************************************/
void lcdwritehz(char hznum, char y, char x, unsigned char disp_invert)
{
    unsigned char seg,hz;
    unsigned char coden=0;
    unsigned int x_pos=0;
    hz=hznum;
    x_pos =x<<4;
//    wcom(0xb0|(y&0x0f));
//    wcom(0x10|((x>>1)&0x0f));
//    wcom(0x00|((x<<3)&0x0f));
    LCD_DOC_C();
    LCD_CMD_SET_PAGE(y);
    LCD_CMD_SET_COLUMN(x_pos);
    LCD_DOC_D();
    for (seg = 0; seg < 16; seg ++)    {
        wdata(GB_16[hz].Msk[coden ++],disp_invert);
    }
//    wcom(0xb0|((y+1)&0x0f));
//    wcom(0x10|((x>>1)&0x0f));
//    wcom(0x00|((x<<3)&0x0f));
    LCD_DOC_C();
    LCD_CMD_SET_PAGE((y+1));
    LCD_CMD_SET_COLUMN(x_pos);
    LCD_DOC_D();
    for (seg = 0; seg < 16; seg ++) {
        wdata(GB_16[hz].Msk[coden ++],disp_invert);
    }
}

//特殊区域显示函数
//入口参数：
//        y_pos: 显示起始Y坐标  8x8单元
//        y_len: 共需显示Y长度  8x8单元
//        x_pos: 显示起始X坐标  8x8单元
//        x_len: 共需显示X长度  单点
//        *data：显示数据地址
//        disp_en:为1时显示该区域，为0时清除该区域显示
void display_func(unsigned char y_pos, unsigned char y_len,
                  unsigned char x_pos, unsigned char x_len,
                  const unsigned char * data, unsigned char disp_en)
{
    unsigned int x,y,P,j,i;
    j=0;

    for(y=y_pos;y<y_pos+y_len;y++ )
    {
        LCD_DOC_C();
        LCD_CMD_SET_PAGE(y);
        LCD_CMD_SET_COLUMN(x_pos);
        LCD_DOC_D();
//        wcom(y);//页
//        wcom(0x10|((x_pos>>1)&0x0F));//列
//        wcom(0x00|((x_pos<<3)&0x0F));//行
        for(x=0;x<x_len;)
        {
            if(disp_en)
            {
                P=data[j];
                if((P==0)||(P==0xFF))
                {
                    i=data[j+1];
                    while(i>0)
                    {
                        wdata(P,INVERT_OFF);
                        wdata(P,INVERT_OFF);
                        if(x>=(unsigned char)(x_len-1))
                        {
                            x=0;
                            y++;
//                            wcom(y);//页
//                            wcom(0x10|((x_pos>>1)&0x0F));//列
//                            wcom(0x00|((x_pos<<3)&0x0F));//行
                            LCD_DOC_C();
                            LCD_CMD_SET_PAGE(y);
                            LCD_CMD_SET_COLUMN(x_pos);
                            LCD_DOC_D();
                        }
                        else
                            x++;
                        i--;
                    }
                    j+=2;
                    if(y>=y_pos+y_len) break;
                }
                else
                {
                    wdata(P,INVERT_OFF);
                    wdata(P,INVERT_OFF);
                    j++;
                    x++;
                }
            }
            else
            {
                wdata(0,INVERT_OFF);
                wdata(0,INVERT_OFF);
                x++;
            }
        }
    }
}

/*
void display_full_screen(unsigned char screen_number)
{
*/
    /* unsigned int x,y,P;     */
    /* unsigned int i,j; */
    /* const unsigned char *map; */
    /* map=Full_screen_data[screen_number-1].table; */
    /* LCD_clear();//清屏 */
    /* LCD_CMD_SET_START_LINE(0); */
    /* j=0; */
      /* for(y=0xb0;y<=0xb7;y++) */
    /* { */
  /* //      wcom(y);//页 */
  /* //      wcom(0x10);//列 */
  /* //      wcom(0); */
        /* LCD_DOC_C(); */
        /* LCD_CMD_SET_PAGE(y); */
        /* LCD_CMD_SET_COLUMN(0); */
        /* LCD_DOC_D(); */
        /* for(x=0;x<128;) */
        /* { */
            /* P=map[j]; */
            /* if((P==0)||(P==0xFF)) */
            /* { */
                /* i=map[j+1]; */
                /* while(i>0) */
                /* { */
                    /* wdata(P,INVERT_OFF); */
                    /* if(x==127) */
                    /* {  */
                        /* x=0; */
                        /* y++; */
/* //                        wcom(y);//页 */
/* //                        wcom(0x10);//列 */
/* //                        wcom(0); */
                        /* LCD_DOC_C(); */
                        /* LCD_CMD_SET_PAGE(y); */
                        /* LCD_CMD_SET_COLUMN(0); */
                        /* LCD_DOC_D(); */
                    /* } */
                    /* else  */
                        /* x++; */
                    /* i--; */
                /* } */
                /* j+=2; */
                /* if(y>0xb7) break; */
            /* } */
            /* else */
            /* { */
                /* wdata(P,INVERT_OFF); */
                /* j++; */
                /* x++; */
            /* } */
        /* } */
  /*  } */
/*}*/

void LcdInit(uint8 Flag)    //初始化LCD
{

   gpio_attr_t AttrOut;
   AttrOut.mode = PIO_MODE_OUT;
   AttrOut.resis = PIO_RESISTOR_DOWN;
   AttrOut.filter = PIO_FILTER_NOEFFECT;
   AttrOut.multer = PIO_MULDRIVER_NOEFFECT;
   SetPIOCfg(PB25, AttrOut);
   SetPIOCfg(PB26, AttrOut);
   SetPIOCfg(PB28, AttrOut);
   SetPIOCfg(PB29, AttrOut);
   SetPIOCfg(PB30, AttrOut);
   SetPIOCfg(PB31, AttrOut);


    //#define lcd_wr_cmd(cmd_data)  wdata(cmd_data)
    #define LCD_DISP_OFF     0xae
    #define LCD_DISP_ON         0xaf
    #define lcd_contra         48
    #define LCD_DELAY()        usleep(24*1000)
    #define VC_ON 0x2C         //1010
    #define VR_ON 0x2A
    #define VF_ON 0x29        //1001
    if(Lcd_Flag_UpData!=0)
        return;
    Lcd_Flag_UpData = 1;
    if(Flag == 1)
    {
        LCD_RESET_IN();
        LCD_DELAY();
        LCD_RESET_OUT();
        LCD_LED_ON();
    }
    LCD_START_CMD();
    wcom(LCD_DISP_OFF);       // Display OFF   (BYTE)0xAE;
    //DelayUs(1);
    //LCD_DELAY();
    wcom(0xE2);  //软件复位寄存器
    //DelayUs(1);
    //LCD_DELAY();
    wcom(0xA2);
    if(DATA_D7_D0)
    {
        wcom(0xA1);        //0xa0// Column address is mapped to SEG0
        wcom(0xC8);        //0xc0// Remappled mode COM[N-1] to COM0
    }
    else
    {
        wcom(0xA0);        //0xa0// Column address is mapped to SEG0
        wcom(0xC8);        //0xc0// Remappled mode COM[N-1] to COM0
    }

    wcom(0xA8);
    wcom(0x3F);
    wcom(0x81);
    wcom(lcd_contra);
    wcom(0x2F);        // Turn on OpAmpBuffer,Regulator,Bootser
    wcom(0xA9);        // Extend Command  set bias ratio
    wcom(0x62);        // Bias Ratio : 1/9 or 1/7    17kHz
    wcom(0xAA);        // Extend Command  //USE normal setting(POR)
    wcom(0x40);        // start line
    wcom(0x24);        // in regulator set  0x24 //0x25 //0x27
    wcom(0xA6); //0xA6//wcom(0xA6);      //设置全屏反显状态   0xA6：正常 0xA7：反显
    //DelayMs(1);
    //LCD_DELAY();
    wcom(LCD_DISP_ON);    // Display ON          LCD_DISP_ON     (BYTE)0xAF      // display ON


    Lcd_Flag_UpData = 0;
}



void LCD_DISP_TEST_MSK(unsigned char disp_data)
{
    unsigned char page;
    unsigned char seg;

    LCD_START_CMD();
    LCD_CMD_SET_START_LINE(0);
    for(page=0;page<8;page++)
    {
        LCD_START_CMD();
        LCD_CMD_SET_PAGE(page);
        LCD_CMD_SET_COLUMN(0);
        LCD_START_DATA();

        for(seg=0;seg<128;seg++)
            wdata(disp_data, INVERT_OFF);
    }
}

void LCD_DISP_Buff_All_LCD(void)
{
    unsigned char page;
    unsigned char seg;
    unsigned int j;
    if(Lcd_Flag_UpData!=0)
        return;
    Lcd_Flag_UpData = 1;

    LCD_DOC_C();
    LCD_CMD_SET_START_LINE(0);
    j=0;
    for(page=0;page<8;page++)
    {
        LCD_DOC_C();
        LCD_CMD_SET_PAGE(page);
        LCD_CMD_SET_COLUMN(0);
        LCD_DOC_D();
        for(seg=0;seg<128;seg++)
        {
            //wdata(LCD_Disp_DOT_Buff[page][seg]);
            wdata(title[j], INVERT_OFF);
            j++;
        }
    }
    Lcd_Flag_UpData = 0;
}


void LcdChangDirection(void)
{
    if(DATA_D7_D0)
        DATA_D7_D0=0;
    else
        DATA_D7_D0=1;
    LcdInit(1);
}

void LcdClearBuff(void)
{
    memset(LCD_Disp_DOT_Buff,0,sizeof(LCD_Disp_DOT_Buff));
}

void Lcd_test(void)
{
    uint8 Count=0;
    uint8 x=0;
    uint8 c='A';
    LCDClear();
    LCD_DISP_Buff_All_LCD();
    //display_full_screen(1);
    LCDClear();
    for(Count=0;Count <=15;Count++)
        lcdwritechar(c++,0,x++,0,0,0,0,0);
    x=0;
    for(Count='0';Count <='9';Count++)
        lcdwritechar(Count,2,x++,0,0,0,0,0);
    x=0;
    for(Count=0;Count <=7;Count++)
        lcdwritehz(Count,4,x++,0);
    x=0;
    for(Count=0;Count <=9;Count++)
        lcdwritechar8x8(Count,6,x++,1,0,0,0,0 );
    x=0;
    for(Count=0;Count <=9;Count++)
        lcdwritechar8x8(Count,7,x++,0,0,0,0,0 );
}

void Lcd_TestBuf(void)
{
    unsigned char *p = (unsigned char *)&LCD_Disp_DOT_Buff;
    unsigned short i;
    for(i = 0;i<sizeof(LCD_Disp_DOT_Buff);i++)
        p[i] = title[i];
}



//end file
