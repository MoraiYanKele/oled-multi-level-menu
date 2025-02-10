#ifndef __OLED_MENU_H__
#define __OLED_MENU_H__

#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#include "oled_menu_types.h"
#include "oled_draw.h"

#define LIMIT_MAGNITUDE(value, low, high) \
        ((value) < (low) ? (low) : ((value) > (high) ? (high) : (value))) // 限幅函数

#define OLED_SCREEN_WIDTH   128  // 屏幕宽度
#define OLED_SCREEN_HEIGHT  64   // 屏幕高度
#define MENU_ITEM_HEIGHT    16   // 每一个菜单项的高度

#define SCROLLBAR_WIDTH    2
#define SCROLLBAR_MARGIN   3

#define LONG_PRESS_THRESHOLD 600 // 长按判定时间(ms)

// 设置左右键对应的GPIO，默认高电平为按下
#define RIGHT_KEY_GPIOX       GPIOC
#define RIGHT_KEY_GPIO_PIN    GPIO_PIN_8
#define LEFT_KEY_GPIOX        GPIOC
#define LEFT_KEY_GPIO_PIN     GPIO_PIN_9

extern Menutypedef mainMenu;
extern Menutypedef *currentMenu;
extern KeyTypeDef KeyList[2];

extern UIElemTypedef frameY;
extern UIElemTypedef frameWidth;
extern UIElemTypedef screenTop;
extern UIElemTypedef scrollBarY;
extern UIElemTypedef switchCtrlBar;
extern UIElemTypedef displayCtrlBar;

extern ScreenIndexTypedef screenIndex;
extern uint8_t keyID;
extern uint8_t menuSwitchFlag;
extern uint8_t controlSelectionFlag;

extern float moveProcess_FrameY;
extern float moveProcess_FrameWidth;
extern float moveProcess_Screen;
extern float moveProcess_ScrollBar;
extern float moveProcess_SwitchCtrlBar;

Menutypedef *AddMenu(const char *name, ItemTypedef *items, uint16_t itemCount, Menutypedef *parentMenu);

ItemTypedef *AddMenuItem(Menutypedef *menu, 
                         const char *name, 
                         void (*funtion)(void), 
                         Menutypedef *subMenu, 
                         ControlModeTypedef ctrlMode, 
                         int *ctrlData);

void FunctionForCtrl(void);
void FunctionForNextMenu(void);

void UI_Init(void);
void UI_UpDate(void);
void UI_Move(void);
void UI_Show(void);


void KeyShortPress(void);
void KeyLongPress(void);
void Frame_Update(void);
void Screen_Update(void);
void ScrollBar_Update(void);
void switchCtrlBar_Update(void);
uint8_t UI_SmoothTransition(UIElemTypedef *elem, float *moveProcess, float moveSpeed);
void InterfaceSwitch(void);
void DrawMenuItems(void);
void DrawControlSelection(void);
void DrawItemName(char *str, int xPos, int yPos);
void DrawControlInformation(ControlTypedef *control, int yPos);
void DrawSelectionFrame(void);
void DrawScrollBar(void);
void DrawSwitchControl(ControlTypedef *control);
void DrawDisplayControl(ControlTypedef *control);
void DrawSliderControl(ControlTypedef *control);
float easeInOut(float t);



#endif

