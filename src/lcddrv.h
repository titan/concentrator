

#ifndef __LCDDRV_H__
#define __LCDDRV_H__
#include"sysdefs.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __LCDDRV_C__
#define GLOBAL
#else
#define GLOBAL extern
#endif

#define LCD_DOT_Y	128
#define LCD_DOT_X	64


GLOBAL unsigned char LCD_Disp_DOT_Buff[8][128];	//显存


GLOBAL void LcdGpin(void);
GLOBAL void LcdInit(uint8 Flag);
GLOBAL void LcdLedOn(void);
GLOBAL void LcdLedOff(void);
GLOBAL void LCD_DISP_TEST_MSK(unsigned char disp_data);
GLOBAL void LCD_DISP_Buff_All_LCD(void);
GLOBAL void LCD_DIP_Buff_Rectangle_LCD(void);
GLOBAL void LcdChangDirection(void);
GLOBAL void LcdClearBuff(void);
GLOBAL void Lcd_test(void);
void Lcd_TestBuf(void);
//void display_full_screen(unsigned char screen_number);
void display_func(unsigned char y_pos, unsigned char y_len,
                  unsigned char x_pos, unsigned char x_len,
                  const unsigned char * data, unsigned char disp_en);
void lcdwritehz(char hznum ,char y ,char x , unsigned char  disp_invert);
void lcdwritechar(char codenum ,char y ,char x ,unsigned char disp_invert,
                  unsigned char mark_L ,unsigned char mark_R, unsigned char mark_T , unsigned char mark_B);
void lcdwritechar8x8(char codenum ,char y ,char x,unsigned char disp_invert ,unsigned char mark_L ,unsigned char mark_R, unsigned char mark_T, unsigned char mark_B);
void LCDClear(void);

#undef GLOBAL
#ifdef __cplusplus
}
#endif
#endif //__LCDDRV_H__
