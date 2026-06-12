#ifndef __OLED_MENU_TYPES_H__
#define __OLED_MENU_TYPES_H__

#include <stdint.h>

struct Menutypedef;
struct ItemTypedef;
struct ControlTypedef;

#define MENU_MAX_ITEMS 16
#define MENU_MAX_MENUS 8

/**
 * @brief 控件模式类型枚举。
 * 
 * 此枚举定义了菜单项中控件的类型，例如开关控件、显示控件和滑动条控件。
 * NONE_CTRL表示无控件。
 */
typedef enum 
{
  NONE_CTRL    =  0,        /**< 无控件 */
  SWITCH_CTRL  =  1,        /**< 开关控件 */
  DISPLAY_CTRL =  2,        /**< 显示控件 */
  SLIDER_CTRL  =  3         /**< 滑动条控件 */
} ControlModeTypedef;

/**
 * @brief 按键模式类型枚举。
 * 
 * 此枚举定义了按键的操作类型，例如长按和短按。
 */
typedef enum
{
  LONG_PRESS   =  1,        /**< 长按 */
  SHORT_PRESS  =  2         /**< 短按 */
} PressModeTypedef;

/**
 * @brief 菜单事件类型枚举。
 *
 * 此枚举将底层按键输入转换为菜单可以理解的抽象事件。
 */
typedef enum
{
  MENU_EVENT_NONE  = 0,     /**< 无事件 */
  MENU_EVENT_NEXT  = 1,     /**< 下一项或增加值 */
  MENU_EVENT_PREV  = 2,     /**< 上一项或减少值 */
  MENU_EVENT_ENTER = 3,     /**< 进入/执行 */
  MENU_EVENT_BACK  = 4      /**< 返回/退出 */
} MenuEventTypedef;
/**
 * @brief 控件结构体。
 *
 * 定义了控件的属性，包括绑定的数据指针和控件模式。
 */
typedef struct ControlTypedef
{
  int *data;                      /**< 控件绑定的数据指针 */
  ControlModeTypedef mode;        /**< 控件模式 */

} ControlTypedef;

/**
 * @brief 菜单项结构体。
 *
 * 定义了菜单项的属性，包括名称、子菜单、控件信息等。
 */
typedef struct ItemTypedef
{
  char *str;                      /**< 菜单项名称 */
  uint8_t len;                    /**< 菜单项名称长度 */
  void (*Function)(void);         /**< 菜单项执行的函数指针 */
  struct Menutypedef *subMenu;    /**< 子级菜单指针 */
  struct ControlTypedef *control; /**< 控件信息指针 */
  ControlTypedef controlData;     /**< 静态控件存储 */

} ItemTypedef;

/**
 * @brief 菜单结构体。
 *
 * 定义了一个菜单的属性，包括名称、菜单项、父菜单、子菜单等。
 */
typedef struct Menutypedef
{
  char *menuName;                 /**< 菜单名称 */
  ItemTypedef *items;             /**< 菜单项指针 */
  ItemTypedef itemPool[MENU_MAX_ITEMS]; /**< 静态菜单项存储池 */
  struct Menutypedef *parentMenu; /**< 父级菜单指针 */
  uint16_t itemCount;             /**< 菜单项数量 */
  int currentItemIndex;           /**< 当前菜单项索引 */

} Menutypedef;

/**
 * @brief UI 元素结构体。
 * 
 * 定义了 UI 元素的值、目标值和上一次的值。
 */
typedef struct 
{
  int16_t val;                    /**< 当前值 */
  int16_t targetVal;              /**< 目标值 */
  int16_t lastVal;                /**< 上一次值 */

} UIElemTypedef;

/**
 * @brief 按键结构体。
 * 
 * 定义了按键的当前值、上一次值、长按和短按标志以及时间信息。
 */
typedef struct 
{
  uint8_t val;                    /**< 当前值 */
  uint8_t lastVal;                /**< 上一次值 */
  uint8_t updateFlag;             /**< 更新标志 */
  uint8_t longPress;              /**< 长按标志 */
  uint32_t pressTime;             /**< 按下时间 */
  uint32_t releaseTime;           /**< 抬起时间 */
  
} KeyTypeDef;

/**
 * @brief 屏幕索引结构体。
 * 
 * 定义了屏幕的顶部和底部索引，用于分页显示菜单。
 */
typedef struct
{
  int16_t topIndex;               /**< 屏幕顶部索引 */
  int16_t bottomIndex;            /**< 屏幕底部索引 */

} ScreenIndexTypedef;

typedef struct
{
  Menutypedef *current_menu;
  ScreenIndexTypedef screen_index;
  uint8_t menu_switch_flag;
  uint8_t control_selection_flag;
} MenuStateTypedef;

typedef struct
{
  UIElemTypedef frame_y;
  UIElemTypedef frame_width;
  UIElemTypedef screen_top;
  UIElemTypedef scroll_bar_y;
  UIElemTypedef switch_ctrl_bar;
  UIElemTypedef display_ctrl_bar;
  float move_process_frame_y;
  float move_process_frame_width;
  float move_process_screen;
  float move_process_scroll_bar;
  float move_process_switch_ctrl_bar;
} MenuAnimStateTypedef;

typedef struct
{
  uint32_t (*get_tick)(void);
  void (*delay)(uint32_t ms);
} MenuPlatformTypedef;

typedef struct
{
  MenuStateTypedef state;
  MenuAnimStateTypedef anim;
  MenuPlatformTypedef platform;
  KeyTypeDef keys[2];
  uint8_t key_id;
} MenuContextTypedef;



#endif
