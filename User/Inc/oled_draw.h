#ifndef __OLED_DRAW_H__
#define __OLED_DRAW_H__

#include "font.h"
#include "main.h"
#include "string.h"

typedef enum {
  OLED_COLOR_NORMAL = 0, // 正常模式 黑底白字
  OLED_COLOR_REVERSED    // 反色模式 白底黑字
} OLED_ColorMode;

void OLED_Init();
void OLED_DisPlay_On();
void OLED_DisPlay_Off();

void OLED_NewFrame();
void OLED_ShowFrame();

void OLED_Disappear(void);
void OLED_SetPixel(int16_t x, int16_t y, OLED_ColorMode color);

void OLED_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, OLED_ColorMode color);
void OLED_DrawRectangle(int16_t x, int16_t y, uint8_t w, uint8_t h, OLED_ColorMode color);
void OLED_DrawEmptyRectangle(int16_t x, int16_t y, uint8_t w, uint8_t h);
void OLED_DrawFilledRectangle(int16_t x, int16_t y, uint8_t w, uint8_t h, OLED_ColorMode color);
void OLED_DrawRectangleWithCorners(int16_t x, int16_t y, uint8_t w, uint8_t h, OLED_ColorMode color);
void OLED_DrawFilledRectangleWithCorners(int16_t x, int16_t y, uint8_t w, uint8_t h, OLED_ColorMode color);
void OLED_DrawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, OLED_ColorMode color);
void OLED_DrawFilledTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, OLED_ColorMode color);
void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t r, OLED_ColorMode color);
void OLED_DrawFilledCircle(uint8_t x, uint8_t y, uint8_t r, OLED_ColorMode color);
void OLED_DrawEllipse(uint8_t x, uint8_t y, uint8_t a, uint8_t b, OLED_ColorMode color);
void OLED_DrawImage(uint8_t x, uint8_t y, const Image *img, OLED_ColorMode color);

void OLED_PrintASCIIChar(int16_t x, int16_t y, char ch, const ASCIIFont *font, OLED_ColorMode color);
void OLED_PrintASCIIString(int16_t x, int16_t y, char *str, const ASCIIFont *font, OLED_ColorMode color);
void OLED_PrintString(uint8_t x, uint8_t y, char *str, const Font *font, OLED_ColorMode color);


#endif