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
extern MenuContextTypedef defaultMenuContext;
extern MenuContextTypedef *activeMenuContext;

#define currentMenu              (activeMenuContext->state.current_menu)
#define KeyList                  (activeMenuContext->keys)
#define frameY                   (activeMenuContext->anim.frame_y)
#define frameWidth               (activeMenuContext->anim.frame_width)
#define screenTop                (activeMenuContext->anim.screen_top)
#define scrollBarY               (activeMenuContext->anim.scroll_bar_y)
#define switchCtrlBar            (activeMenuContext->anim.switch_ctrl_bar)
#define displayCtrlBar           (activeMenuContext->anim.display_ctrl_bar)
#define screenIndex              (activeMenuContext->state.screen_index)
#define keyID                    (activeMenuContext->key_id)
#define menuSwitchFlag           (activeMenuContext->state.menu_switch_flag)
#define controlSelectionFlag     (activeMenuContext->state.control_selection_flag)
#define moveProcess_FrameY       (activeMenuContext->anim.move_process_frame_y)
#define moveProcess_FrameWidth   (activeMenuContext->anim.move_process_frame_width)
#define moveProcess_Screen       (activeMenuContext->anim.move_process_screen)
#define moveProcess_ScrollBar    (activeMenuContext->anim.move_process_scroll_bar)
#define moveProcess_SwitchCtrlBar (activeMenuContext->anim.move_process_switch_ctrl_bar)

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

void Menu_InitContext(MenuContextTypedef *ctx, Menutypedef *rootMenu);
void Menu_SetContext(MenuContextTypedef *ctx);
void Menu_SetPlatform(MenuContextTypedef *ctx, MenuPlatformTypedef platform);
void Menu_HandleEvent(MenuEventTypedef event);
void Menu_HandleEventFor(MenuContextTypedef *ctx, MenuEventTypedef event);

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

