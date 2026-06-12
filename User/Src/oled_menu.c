/**
 * @file oled_menu.c
 * @brief OLED菜单系统实现，包含菜单项管理、按键响应及UI动态更新
 * @author [Ykl]
 * @version 1.0
 * @date 2025-2-8
 * @license MIT
 * 
 * ____    ____  __  ___  __      
 * \   \  /   / |  |/  / |  |     
 *  \   \/   /  |  '  /  |  |     
 *   \_    _/   |    <   |  |     
 *     |  |     |  .  \  |  `----.
 *     |__|     |__|\__\ |_______|                               
 * 
 * @note 功能特性:
 * - 多级菜单系统支持
 * - 平滑动画过渡效果
 * - 支持三种控件类型(开关/数值显示/滑动条)
 * - 长短按按键区分处理
 * - 自适应滚动条显示
 * 
 * @warning 硬件依赖:
 * - STM32 HAL库
 * - 128x64 OLED显示屏(SSD1306)
 * - 两个物理按键(左/右)
 * 
 * @acknowledgment oled硬件驱动借鉴参考波特律动(keysking)波特律动OLED驱动(SSD1306)
 */

#include "oled_menu.h"

/**
 * @brief 主菜单。
 *
 * 包含主菜单的名称、菜单项、父菜单以及菜单项数量。
 */
Menutypedef mainMenu =
{
  .menuName = "mainMenu",
  .items = NULL,
  .itemPool =
  {
    {"example", 7, NULL, NULL, NULL, {NULL, NONE_CTRL}},
  },
  .parentMenu = NULL,
  .itemCount = 1,
  .currentItemIndex = 0,
};
static Menutypedef menuPool[MENU_MAX_MENUS] = {0};
static uint16_t menuPoolCount = 0;

MenuContextTypedef defaultMenuContext =
{
  .state =
  {
    .current_menu = &mainMenu,
    .screen_index = {.topIndex = 0, .bottomIndex = 3},
    .menu_switch_flag = 0,
    .control_selection_flag = 0,
  },
  .anim =
  {
    .frame_y = {.val = 0, .targetVal = 0, .lastVal = 0},
    .frame_width = {.val = 0, .targetVal = 0, .lastVal = 0},
    .screen_top = {.val = 0, .targetVal = 0, .lastVal = 0},
    .scroll_bar_y = {.val = 2, .targetVal = 2, .lastVal = 2},
    .switch_ctrl_bar = {.val = 64 + 2, .targetVal = 64 + 2, .lastVal = 64 + 2},
    .display_ctrl_bar = {.val = 0, .targetVal = 0, .lastVal = 0},
    .move_process_frame_y = 0.0,
    .move_process_frame_width = 0.0,
    .move_process_screen = 0.0,
    .move_process_scroll_bar = 0.0,
    .move_process_switch_ctrl_bar = 0.0,
  },
  .keys = {0},
  .key_id = 0,
};

MenuContextTypedef *activeMenuContext = &defaultMenuContext;

static uint32_t Menu_DefaultGetTick(void)
{
  return HAL_GetTick();
}

static void Menu_DefaultDelay(uint32_t ms)
{
  HAL_Delay(ms);
}

// ------------函数定义------------//
static void Menu_EnsureStorage(Menutypedef *menu)
{
  if (menu && !menu->items)
  {
    menu->items = menu->itemPool;
  }
}

static void Menu_RefreshTargets(void)
{
  Frame_Update();
  Screen_Update();
  ScrollBar_Update();
  switchCtrlBar_Update();
}

void Menu_InitContext(MenuContextTypedef *ctx, Menutypedef *rootMenu)
{
  if (!ctx)
  {
    return;
  }

  memset(ctx, 0, sizeof(MenuContextTypedef));
  ctx->state.current_menu = rootMenu ? rootMenu : &mainMenu;
  ctx->state.screen_index.topIndex = 0;
  ctx->state.screen_index.bottomIndex = 3;
  ctx->anim.scroll_bar_y.val = 2;
  ctx->anim.scroll_bar_y.targetVal = 2;
  ctx->anim.scroll_bar_y.lastVal = 2;
  ctx->anim.switch_ctrl_bar.val = 64 + 2;
  ctx->anim.switch_ctrl_bar.targetVal = 64 + 2;
  ctx->anim.switch_ctrl_bar.lastVal = 64 + 2;
  ctx->platform.get_tick = Menu_DefaultGetTick;
  ctx->platform.delay = Menu_DefaultDelay;
}

void Menu_SetContext(MenuContextTypedef *ctx)
{
  activeMenuContext = ctx ? ctx : &defaultMenuContext;
}

void Menu_SetPlatform(MenuContextTypedef *ctx, MenuPlatformTypedef platform)
{
  if (!ctx)
  {
    ctx = &defaultMenuContext;
  }

  ctx->platform.get_tick = platform.get_tick ? platform.get_tick : Menu_DefaultGetTick;
  ctx->platform.delay = platform.delay ? platform.delay : Menu_DefaultDelay;
}

static void Menu_EnsurePlatform(MenuContextTypedef *ctx)
{
  if (!ctx)
  {
    return;
  }

  if (!ctx->platform.get_tick)
  {
    ctx->platform.get_tick = Menu_DefaultGetTick;
  }

  if (!ctx->platform.delay)
  {
    ctx->platform.delay = Menu_DefaultDelay;
  }
}

static void Menu_Delay(uint32_t ms)
{
  Menu_EnsurePlatform(activeMenuContext);
  activeMenuContext->platform.delay(ms);
}

/**
 * @brief 按键中断回调函数。
 * 
 * 根据按键引脚处理按下、释放和长按短按事件。
 * 
 * @param GPIO_Pin 按键引脚号。
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t currentTime = HAL_GetTick();

  if (GPIO_Pin == RIGHT_KEY_GPIO_PIN)
  {
    if (HAL_GPIO_ReadPin(RIGHT_KEY_GPIOX, RIGHT_KEY_GPIO_PIN) == GPIO_PIN_SET) // 默认高电平按下，可以更改
    {
      KeyList[0].val = 1;
      KeyList[0].pressTime = currentTime;
      keyID = 1;
    }
    else 
    {
      KeyList[0].val = 0;
      KeyList[0].releaseTime = currentTime;

      uint32_t pressDuration = KeyList[0].releaseTime - KeyList[0].pressTime;
      if (pressDuration >= LONG_PRESS_THRESHOLD)
      {
        KeyList[0].longPress = LONG_PRESS;  /**< 长按标志 */
      }
      else
      {
        KeyList[0].longPress = SHORT_PRESS; /**< 短按标志 */
      }
    }

    if (!KeyList[0].val && KeyList[0].longPress)
    {
      KeyList[0].updateFlag = 1;
    }

  }
  else if (GPIO_Pin == LEFT_KEY_GPIO_PIN)
  {
    if (HAL_GPIO_ReadPin(LEFT_KEY_GPIOX, LEFT_KEY_GPIO_PIN) == GPIO_PIN_SET)
    {
      KeyList[1].val = 1;
      KeyList[1].pressTime = currentTime;
      keyID = 2;
    }
    else 
    {
      KeyList[1].val = 0;
      KeyList[1].releaseTime = currentTime;

      uint32_t pressDuration = KeyList[1].releaseTime - KeyList[1].pressTime;
      if (pressDuration >= LONG_PRESS_THRESHOLD)
      {
        KeyList[1].longPress = LONG_PRESS;  /**< 长按标志 */
      }
      else
      {
        KeyList[1].longPress = SHORT_PRESS; /**< 短按标志 */
      }
    }

    if (!KeyList[1].val && KeyList[1].longPress)
    {
      KeyList[1].updateFlag = 1;
    }
  }
}


/**
 * @brief 创建并添加一个菜单。
 * 
 * @param name       菜单名称，需在菜单生命周期内保持有效。
 * @param items      菜单项数组（可为 NULL）。
 * @param itemCount  菜单项数量（如果 items 为 NULL，传 0）。
 * @param parentMenu 父级菜单（可为 NULL）。
 * @return Menutypedef* 返回新创建的菜单指针，失败时返回 NULL。
 */
Menutypedef *AddMenu(const char *name, ItemTypedef *items, uint16_t itemCount, Menutypedef *parentMenu)
{
  if (!name)
    return NULL;

  if (menuPoolCount >= MENU_MAX_MENUS)
  {
    return NULL;
  }

  Menutypedef *newMenu = &menuPool[menuPoolCount++];
  memset(newMenu, 0, sizeof(Menutypedef));

  newMenu->menuName = (char *)name;
  newMenu->items = newMenu->itemPool;
  newMenu->itemCount = 0;

  if (items && itemCount > 0)
  {
    uint16_t copyCount = itemCount > MENU_MAX_ITEMS ? MENU_MAX_ITEMS : itemCount;
    memcpy(newMenu->itemPool, items, copyCount * sizeof(ItemTypedef));
    for (uint16_t i = 0; i < copyCount; i++)
    {
      if (newMenu->itemPool[i].control)
      {
        newMenu->itemPool[i].controlData = *newMenu->itemPool[i].control;
        newMenu->itemPool[i].control = &newMenu->itemPool[i].controlData;
      }
    }
    newMenu->itemCount = copyCount;
  }

  // 初始化其他属性
  newMenu->currentItemIndex = 0;
  newMenu->parentMenu = parentMenu;

  return newMenu;
}


/**
 * @brief 向指定菜单添加一个新的菜单项。
 * 
 * 该函数初始化一个菜单项，并将其写入菜单内置的静态菜单项池中。
 * 如果菜单项数量达到 MENU_MAX_ITEMS，则添加失败并返回 NULL。
 * 
 * @param menu     指向目标菜单的指针。
 * @param name     菜单项的名称（字符串）。
 * @param funtion  菜单项对应的回调函数（可为 NULL）。
 * @param subMenu  该菜单项的子菜单（可为 NULL）。
 * @param ctrlMode 控件模式（如 SWITCH_CTRL、DISPLAY_CTRL 等，无控件时为NONE_CTRL）。
 * @param ctrlData 控件数据指针（用于存储控件的值，可为 NULL）。
 * @return ItemTypedef* 返回新创建的菜单项指针，若失败则返回 NULL。
 * 
 * @note 
 * - 该函数不为 `name` 分配内存，调用方需保证字符串在菜单生命周期内有效。
 * - 控件信息存储在菜单项内置的 `controlData` 中，不使用动态分配。
 * - 确保 `menu->itemCount` 及时更新，以便正确管理菜单项。
 * 
 * @warning 
 * - `name` 需要是有效的字符串指针，否则会导致崩溃。
 * - 调用该函数后，需确保 `menu` 在程序运行期间不会被提前释放，否则会导致内存访问错误。
 */
ItemTypedef *AddMenuItem(Menutypedef *menu, 
                         const char *name, 
                         void (*funtion)(void), 
                         Menutypedef *subMenu, 
                         ControlModeTypedef ctrlMode, 
                         int *ctrlData)
{
  if (!menu || !name)
    return NULL;

  Menu_EnsureStorage(menu);

  if (menu->itemCount >= MENU_MAX_ITEMS)
  {
    return NULL;
  }

  ItemTypedef *newItem = &menu->items[menu->itemCount];
  memset(newItem, 0, sizeof(ItemTypedef));

  newItem->str = (char *)name;
  newItem->len = strlen(name);
  newItem->subMenu = subMenu;
  newItem->Function = funtion;

  if (ctrlMode != 0)
  {
    newItem->control = &newItem->controlData;
    newItem->control->mode = ctrlMode;
    newItem->control->data = ctrlData;
  }
  else
    newItem->control = NONE_CTRL;

  menu->itemCount++;

  return &menu->items[menu->itemCount - 1];
}


/**
 * @brief 初始化 UI 相关参数。
 * 
 * 该函数用于初始化按键状态和 UI 元素的初始值，确保界面在启动时处于正确状态。
 * 
 * @note
 * - 使用 `memset` 将 `KeyList` 清零，防止按键状态异常。
 * - 计算并设置 `frameWidth.targetVal`，确保选中菜单项的框架宽度正确。
 */
void UI_Init()
{
  Menu_EnsurePlatform(activeMenuContext);
  OLED_Init();
  memset(KeyList, 0, sizeof(KeyList));
  Menu_EnsureStorage(currentMenu);
  if (currentMenu && currentMenu->items && currentMenu->itemCount > 0)
  {
    frameWidth.targetVal = currentMenu->items[currentMenu->currentItemIndex].len * 6 + 4;
  }
  else
  {
    frameWidth.targetVal = 0;
  }
  Menu_Delay(20);
}

static void Menu_HandleActiveEvent(MenuEventTypedef event)
{
  Menu_EnsureStorage(currentMenu);

  if (event == MENU_EVENT_NONE || !currentMenu || !currentMenu->items || currentMenu->itemCount == 0)
  {
    return;
  }

  ItemTypedef *currentItem = &currentMenu->items[currentMenu->currentItemIndex];

  if (controlSelectionFlag == 0)
  {
    switch (event)
    {
      case MENU_EVENT_NEXT:
        currentMenu->currentItemIndex++;
        if (currentMenu->currentItemIndex >= currentMenu->itemCount)
        {
          currentMenu->currentItemIndex = 0;
        }
        break;

      case MENU_EVENT_PREV:
        currentMenu->currentItemIndex--;
        if (currentMenu->currentItemIndex < 0)
        {
          currentMenu->currentItemIndex = currentMenu->itemCount - 1;
        }
        break;

      case MENU_EVENT_ENTER:
        if (currentItem->Function)
        {
          currentItem->Function();
        }
        break;

      case MENU_EVENT_BACK:
        if (currentMenu->parentMenu)
        {
          currentMenu = currentMenu->parentMenu;
          Menu_EnsureStorage(currentMenu);
          menuSwitchFlag = 1;
        }
        break;

      default:
        break;
    }
  }
  else
  {
    ControlTypedef *control = currentItem->control;

    switch (event)
    {
      case MENU_EVENT_NEXT:
      case MENU_EVENT_PREV:
        if (!control || !control->data)
        {
          break;
        }

        if (control->mode == SWITCH_CTRL)
        {
          *(control->data) = *(control->data) ? 0 : 1;
        }
        else if (control->mode == SLIDER_CTRL)
        {
          if (event == MENU_EVENT_NEXT)
          {
            *(control->data) += 1;
          }
          else
          {
            *(control->data) -= 1;
          }
          *(control->data) = LIMIT_MAGNITUDE(*(control->data), 0, 100);
        }
        break;

      case MENU_EVENT_BACK:
        controlSelectionFlag = 0;
        menuSwitchFlag = 1;
        break;

      default:
        break;
    }
  }

  Menu_RefreshTargets();
}

void Menu_HandleEventFor(MenuContextTypedef *ctx, MenuEventTypedef event)
{
  MenuContextTypedef *lastContext = activeMenuContext;

  Menu_SetContext(ctx);
  Menu_HandleActiveEvent(event);
  Menu_SetContext(lastContext);
}

void Menu_HandleEvent(MenuEventTypedef event)
{
  Menu_HandleActiveEvent(event);
}


/**
 * @brief 更新 UI 状态。
 *
 * 根据按键状态生成菜单事件，并交给 `Menu_HandleEvent()` 统一处理。
 */
void UI_UpDate()
{
  for (uint8_t i = 0; i < 2; i++)
  {
    MenuEventTypedef event = MENU_EVENT_NONE;

    if (!KeyList[i].updateFlag)
    {
      continue;
    }

    KeyList[i].updateFlag = 0;
    keyID = i + 1;

    if (KeyList[i].longPress == SHORT_PRESS)
    {
      event = (keyID == 1) ? MENU_EVENT_NEXT : MENU_EVENT_PREV;
    }
    else if (KeyList[i].longPress == LONG_PRESS)
    {
      event = (keyID == 1) ? MENU_EVENT_ENTER : MENU_EVENT_BACK;
    }

    Menu_HandleEvent(event);

    KeyList[i].longPress = 0;
    keyID = 0;
  }
}

/**
 * @brief 处理按键长按逻辑。
 * 
 * 根据当前的控件选择状态和按键 ID，执行不同的操作：
 */
void KeyLongPress()
{
  if (keyID == 1)
  {
    Menu_HandleEvent(MENU_EVENT_ENTER);
  }
  else if (keyID == 2)
  {
    Menu_HandleEvent(MENU_EVENT_BACK);
  }
}

/**
 * @brief 处理按键短按逻辑。
 * 
 * 根据控件选择状态和按键 ID，执行不同的操作
 */
void KeyShortPress()
{
  if (keyID == 1)
  {
    Menu_HandleEvent(MENU_EVENT_NEXT);
  }
  else if (keyID == 2)
  {
    Menu_HandleEvent(MENU_EVENT_PREV);
  }
}

/**
 * @brief 更新框架显示目标值。
 * 
 * 根据当前菜单项索引，更新框架的 Y 坐标和宽度的目标值。
 */
void Frame_Update()
{
  Menu_EnsureStorage(currentMenu);

  moveProcess_FrameY = 0.0;
  moveProcess_FrameWidth = 0.0;

  frameY.lastVal = frameY.targetVal;
  frameWidth.lastVal = frameWidth.targetVal;

  if (!currentMenu || !currentMenu->items || currentMenu->itemCount == 0)
  {
    frameY.targetVal = 0;
    frameWidth.targetVal = 0;
    return;
  }

  frameY.targetVal = currentMenu->currentItemIndex * MENU_ITEM_HEIGHT; /**< 更新 Y 目标值 */
  frameWidth.targetVal = currentMenu->items[currentMenu->currentItemIndex].len * 6 + 4; /**< 更新宽度目标值 */
}

/**
 * @brief 更新屏幕显示范围。
 * 
 * 根据当前菜单项索引调整屏幕的顶部和底部索引，并更新屏幕顶部的目标值。
 */
void Screen_Update()
{
  Menu_EnsureStorage(currentMenu);

  if (!currentMenu || currentMenu->itemCount == 0)
  {
    screenIndex.topIndex = 0;
    screenIndex.bottomIndex = 0;
    screenTop.lastVal = screenTop.targetVal;
    screenTop.targetVal = 0;
    return;
  }

  if (currentMenu->currentItemIndex > screenIndex.bottomIndex)
  {
    moveProcess_Screen = 0.0;
    screenIndex.bottomIndex = currentMenu->currentItemIndex;
    screenIndex.topIndex = screenIndex.bottomIndex - 3; /**< 调整顶部索引 */
  }
  else if (currentMenu->currentItemIndex < screenIndex.topIndex)
  {
    moveProcess_Screen = 0.0;
    screenIndex.topIndex = currentMenu->currentItemIndex;
    screenIndex.bottomIndex = screenIndex.topIndex + 3; /**< 调整底部索引 */
  }

  screenTop.lastVal = screenTop.targetVal;
  screenTop.targetVal = screenIndex.topIndex * MENU_ITEM_HEIGHT; /**< 更新屏幕顶部目标值 */
}

/**
 * @brief 更新滚动条位置。
 * 
 * 根据当前菜单项索引，计算滚动条的目标位置，支持不同菜单项数量的情况。
 */
void ScrollBar_Update(void)
{
  moveProcess_ScrollBar = 0.0;
  scrollBarY.lastVal = scrollBarY.val;

  if (!currentMenu)
  {
    scrollBarY.targetVal = 2;
    return;
  }

  if (currentMenu->itemCount <= 1)
  {
    scrollBarY.targetVal = 2;
    return;
  }

  if (currentMenu->itemCount == 2)
  {
    scrollBarY.targetVal = (currentMenu->currentItemIndex * 10) + 2;
    return;
  }

  float moveRangef = (float)((OLED_SCREEN_HEIGHT - 4) - (OLED_SCREEN_HEIGHT * 2) / currentMenu->itemCount) / (currentMenu->itemCount - 1);
  int moveRange = (int)(moveRangef + 0.5f);

  if (currentMenu->currentItemIndex == currentMenu->itemCount - 1)
  {
    scrollBarY.targetVal = OLED_SCREEN_HEIGHT - 2 - (OLED_SCREEN_HEIGHT * 2) / currentMenu->itemCount;
  }
  else
  {
    scrollBarY.targetVal = (currentMenu->currentItemIndex * moveRange) + 2;
  }
}

/**
 * @brief 更新开关控件条位置。
 * 
 * 根据控件数据状态（0 或 1）调整开关控件条的目标位置。
 */
void switchCtrlBar_Update(void)
{
  Menu_EnsureStorage(currentMenu);

  if (!currentMenu || !currentMenu->items || currentMenu->itemCount == 0)
  {
    return;
  }

  ControlTypedef *control = currentMenu->items[currentMenu->currentItemIndex].control;
  if (!control || control->mode != SWITCH_CTRL || !control->data)
  {
    return;
  }

  moveProcess_SwitchCtrlBar = 0.0;
  switchCtrlBar.lastVal = switchCtrlBar.val;

  if (*(control->data) == 0)
  {
    switchCtrlBar.targetVal = 64 + 2; /**< 开关关闭位置 */
  }
  else
  {
    switchCtrlBar.targetVal = 64 - 30 + 2; /**< 开关开启位置 */
  }
}

// /**
//  * @brief 更新显示控件条位置。
//  * 
//  * 将控件数据值映射到显示控件条的位置，并限制值的范围为 0 到 100。
//  */
// void DisplayCtrlBar_Update(void)
// {
//   displayCtrlBar.lastVal = displayCtrlBar.val;

//   float val = LIMIT_MAGNITUDE(*(currentMenu->items[currentMenu->currentItemIndex].control->data), 0, 100);
//   val = (val * 76) / 100; 
//   int displayVal = (int)(val + 0.5f);

//   displayCtrlBar.targetVal = displayVal; /**< 显示控件条目标值 */
// }


/**
 * @brief 更新 UI 元素的动画过渡效果。
 * 
 * 该函数通过 `UI_SmoothTransition` 让多个 UI 组件（如框架、滚动条、开关控件等）平滑移动到目标位置，增强界面动画效果。
 * 
 * @note 
 * - 每个 UI 元素都有一个对应的移动进度变量 (`moveProcess_*`)，用于控制过渡进度。
 * - `UI_SmoothTransition` 采用缓动算法，使动画更加自然。
 * - 该函数应在主循环中定期调用，以维持 UI 的平滑动画。
 */
void UI_Move(void)
{
  UI_SmoothTransition(&frameY, &moveProcess_FrameY, 0.1);
  UI_SmoothTransition(&frameWidth, &moveProcess_FrameWidth, 0.1);
  UI_SmoothTransition(&screenTop, &moveProcess_Screen, 0.1);
  UI_SmoothTransition(&scrollBarY, &moveProcess_ScrollBar, 0.1);
  UI_SmoothTransition(&switchCtrlBar, &moveProcess_SwitchCtrlBar, 0.1);
}


/**
 * @brief 更新 UI 元素的移动状态。
 * 
 * 根据当前进度和移动速度，逐步将 UI 元素的值从 `lastVal` 平滑过渡到 `targetVal`。
 * 
 * @param elem         指向需要更新的 UI 元素。
 * @param moveProcess  移动进度指针（0.0 到 1.0）。
 * @param moveSpeed    移动速度，每次调用增加的进度量。
 * @return uint8_t     返回 1 表示移动完成，0 表示未完成。
 */
uint8_t UI_SmoothTransition(UIElemTypedef *elem, float *moveProcess, float moveSpeed)
{
  if (elem->val == elem->targetVal)
  {
    return 1;
  }
  if (*moveProcess < 1.0)
  {
    *moveProcess += moveSpeed;
  }
  if (*moveProcess > 1.0)
  {
    *moveProcess = 1.0;
  } 

  float easedProcess = easeInOut(*moveProcess);

  elem->val = (int16_t)((1.0 - easedProcess) * elem->lastVal + easedProcess * elem->targetVal);

  if (*moveProcess == 1.0)
  {
    elem->val = elem->targetVal;
    return 1;
  }
  else
  {
    return 0;
  }
}

/**
 * @brief 显示当前 UI。
 * 
 * 根据菜单切换标志和控件选择标志，调用对应的绘制函数：
 * - `InterfaceSwitch`：处理菜单切换时的过渡效果。
 * - `DrawMenuItems`：绘制菜单项。
 * - `DrawControlSelection`：绘制控件选择界面。
 */
void UI_Show()
{
  if (menuSwitchFlag == 1)
  {
    InterfaceSwitch();
  }

  if (controlSelectionFlag == 0)
  {
    DrawMenuItems();
  }
  else if (controlSelectionFlag == 1)
  {
    DrawControlSelection();
  }
}

/**
 * @brief 菜单切换过渡效果。
 * 
 * 显示菜单切换动画，通过三次快速消失与重绘实现。
 */
void InterfaceSwitch()
{
  menuSwitchFlag = 0;
  for (uint8_t i = 0; i < 3; i++)
  {
    OLED_Disappear();
    OLED_ShowFrame();
    Menu_Delay(50); /**< 在 FreeRTOS 中可通过 Menu_SetPlatform 替换延时 */
  }
}

/**
 * @brief 绘制当前菜单项。
 * 
 * 根据当前菜单项索引和屏幕显示范围，绘制菜单项名称和控件信息，防止数组越界。
 * - 调用 `DrawItemName` 绘制菜单项名称。
 * - 调用 `DrawControlInformation` 绘制控件状态。
 * - 调用 `DrawSelectionFrame` 和 `DrawScrollBar` 完成其他显示元素。
 */
void DrawMenuItems()
{
  Menu_EnsureStorage(currentMenu);

  int16_t showItemEndNum = 0;
  int16_t showItemStartNum = 0;
  if (currentMenu && currentMenu->itemCount > 0)
  {
    showItemEndNum = (currentMenu->itemCount > 4) ? ((currentMenu->currentItemIndex) < currentMenu->itemCount - 4 ? 5 : 4) : currentMenu->itemCount;
    showItemStartNum = (currentMenu->currentItemIndex > 3) ? -1 : 0;
  } 
  else 
  {
    showItemEndNum = 0; 
  }

  OLED_NewFrame();

  for (int i = showItemStartNum; i < showItemEndNum; i++)
  {
    int itemIndex = screenIndex.topIndex + i;
    if (itemIndex < 0 || itemIndex >= currentMenu->itemCount) 
    {
      continue; /**< 防止越界 */
    }
    int yPos = screenTop.targetVal - screenTop.val + (i * MENU_ITEM_HEIGHT) + 4;

    DrawItemName(currentMenu->items[itemIndex].str, 2, yPos);
    DrawControlInformation(currentMenu->items[itemIndex].control, yPos);
  }

  DrawSelectionFrame();
  DrawScrollBar();

  OLED_ShowFrame();
}

/**
 * @brief 绘制控件选择界面。
 * 
 * 显示当前选中的控件及其状态：
 * - 根据控件模式调用对应的绘制函数，如 `DrawSwitchControl`、`DrawDisplayControl` 和 `DrawSliderControl`。
 * - 绘制控件的外框和名称。
 */
void DrawControlSelection()
{
  Menu_EnsureStorage(currentMenu);

  if (!currentMenu || !currentMenu->items || currentMenu->itemCount == 0)
  {
    controlSelectionFlag = 0;
    return;
  }

  ControlTypedef *control = currentMenu->items[currentMenu->currentItemIndex].control;
  if (!control)
  {
    controlSelectionFlag = 0;
    return;
  }

  OLED_NewFrame();
  OLED_DrawEmptyRectangle(16, 8, 96, 48);

  DrawItemName(currentMenu->items[currentMenu->currentItemIndex].str, 16 + 2, 8 + 2);

  switch (control->mode)
  {
    case SWITCH_CTRL:
    {
      DrawSwitchControl(control);
      break;
    }
    case DISPLAY_CTRL:
    {
      DrawDisplayControl(control);
      break;
    }
    case SLIDER_CTRL:
    {
      DrawSliderControl(control);
      break;
    }
    default:
      break;  
  }

  OLED_ShowFrame();
}

/**
 * @brief 绘制菜单项名称。
 * 
 * 在指定位置显示菜单项的文本。
 * 
 * @param str   指向菜单项名称的字符串。
 * @param xPos  绘制文本的 X 坐标。
 * @param yPos  绘制文本的 Y 坐标。
 */
void DrawItemName(char *str, int xPos, int yPos)
{
  OLED_PrintASCIIString(xPos, yPos, str, &afont8x6, OLED_COLOR_NORMAL);
}

/**
 * @brief 绘制控件信息。
 * 
 * 根据控件模式（开关、显示、滑条）显示控件的当前状态或值。
 * 
 * @param control 指向控件信息的指针。
 * @param yPos    绘制控件信息的 Y 坐标。
 */
void DrawControlInformation(ControlTypedef *control, int yPos)
{
  if (control && control->data)
  {
    switch (control->mode)
    {
      case SWITCH_CTRL:
      {
        // 绘制开关控件状态（ON 或 OFF）
        if (*(control->data) == 0)
        {
          OLED_PrintASCIIString(100, yPos, "OFF", &afont8x6, OLED_COLOR_NORMAL);
        }
        else
        {
          OLED_PrintASCIIString(100, yPos, "ON", &afont8x6, OLED_COLOR_NORMAL);
        }
        break;
      }

      case DISPLAY_CTRL:
      case SLIDER_CTRL:
      {
        // 绘制显示控件或滑条控件的数值
        char str[10];
        snprintf(str, sizeof(str), "%d", *(control->data));
        OLED_PrintASCIIString(100, yPos, str, &afont8x6, OLED_COLOR_NORMAL);
        break;
      }

      default:
        break;
    }
  }
}

/**
 * @brief 绘制当前选中项的高亮框。
 * 
 * 根据当前选中项的位置绘制反色矩形，突出显示当前项。
 */
void DrawSelectionFrame()
{
  if (!currentMenu || currentMenu->itemCount == 0)
  {
    return;
  }

  OLED_DrawFilledRectangleWithCorners(0, frameY.val + 1 - (screenIndex.topIndex * MENU_ITEM_HEIGHT), frameWidth.val, MENU_ITEM_HEIGHT - 2, OLED_COLOR_REVERSED);
}

/**
 * @brief 绘制滚动条。
 * 
 * 根据当前菜单项数量和选中项位置绘制滚动条，并调整滚动条高度以适配菜单。
 */
void DrawScrollBar()
{
  OLED_DrawRectangle(OLED_SCREEN_WIDTH - SCROLLBAR_WIDTH - SCROLLBAR_MARGIN - 2, 0, 6, OLED_SCREEN_HEIGHT - 1, OLED_COLOR_NORMAL);
  if (!currentMenu || currentMenu->itemCount == 0)
  {
    return;
  }

  if (currentMenu->itemCount == 2 || currentMenu->itemCount == 1)
  {
    OLED_DrawFilledRectangle(OLED_SCREEN_WIDTH - SCROLLBAR_WIDTH - SCROLLBAR_MARGIN, scrollBarY.val, SCROLLBAR_WIDTH, currentMenu->itemCount == 1 ? 60 : 50, OLED_COLOR_NORMAL);
  }
  else 
  {
    OLED_DrawFilledRectangle(OLED_SCREEN_WIDTH - SCROLLBAR_WIDTH - SCROLLBAR_MARGIN, scrollBarY.val, SCROLLBAR_WIDTH, (OLED_SCREEN_HEIGHT * 2) / currentMenu->itemCount, OLED_COLOR_NORMAL);
  }
}

/**
 * @brief 绘制开关控件的状态。
 * 
 * 在控件区域显示 ON/OFF 的文本及其状态指示框。
 * 
 * @param control 指向开关控件的结构体指针。
 */
void DrawSwitchControl(ControlTypedef *control)
{
  if (!control || !control->data)
  {
    return;
  }

  OLED_DrawRectangleWithCorners(64 - 30, 28, 60, 20, OLED_COLOR_NORMAL);
  OLED_PrintASCIIString(64 - 20, 28 + 5, "ON", &afont12x6, OLED_COLOR_NORMAL);
  OLED_PrintASCIIString(64 + 7, 28 + 5, "OFF", &afont12x6, OLED_COLOR_NORMAL);
  OLED_DrawFilledRectangleWithCorners(switchCtrlBar.val, 28 + 2, 26, 17, OLED_COLOR_REVERSED);
}

/**
 * @brief 绘制显示控件的状态。
 * 
 * 根据控件的值绘制进度条，并显示当前值的文本。
 * 
 * @param control 指向显示控件的结构体指针。
 */
void DrawDisplayControl(ControlTypedef *control)
{
  if (!control || !control->data)
  {
    return;
  }

  OLED_DrawRectangle(64 - 40, 40, 80, 10, OLED_COLOR_NORMAL);
  char str[10];
  sprintf(str, "%d", *(control->data));
  OLED_PrintASCIIString(64 - 6, 25, str, &afont12x6, OLED_COLOR_NORMAL);

  float val = LIMIT_MAGNITUDE(*(control->data), 0, 100);
  val = (val * 76) / 100; 
  int displayVal = (int)(val + 0.5f);
  OLED_DrawFilledRectangle(64 - 40 + 2, 40 + 2, displayVal, 7, OLED_COLOR_NORMAL);
}

/**
 * @brief 绘制滑动条控件的状态。
 * 
 * 根据控件的值绘制滑动条的进度，并显示当前值的文本。
 * 
 * @param control 指向滑动条控件的结构体指针。
 */
void DrawSliderControl(ControlTypedef *control)
{
  if (!control || !control->data)
  {
    return;
  }

  OLED_DrawRectangle(64 - 40, 40, 80, 10, OLED_COLOR_NORMAL);
  char str[10];
  sprintf(str, "%d", *(control->data));
  OLED_PrintASCIIString(64 - 6, 25, str, &afont12x6, OLED_COLOR_NORMAL);

  float val = LIMIT_MAGNITUDE(*(control->data), 0, 100);
  val = (val * 76) / 100; 
  int displayVal = (int)(val + 0.5f);
  OLED_DrawFilledRectangle(64 - 40 + 2, 40 + 2, displayVal, 7, OLED_COLOR_NORMAL);
}

/**
 * @brief 缓动函数（Ease-In-Out）。
 * 
 * 用于计算缓动效果的进度值，使移动过程更加平滑。
 * 
 * @param t 输入的进度值，范围为 0.0 到 1.0。
 * @return float 返回缓动后的进度值。
 */
float easeInOut(float t) 
{
  if (t < 0.5)
    return 2 * t * t;
  else
    return -1 + (4 - 2 * t) * t;
}

/**
 * @brief 进入控件选择模式。
 * 
 * 该函数用于切换到控件选择界面，允许用户对菜单项的控件进行操作。
 * 
 * @note 
 * - `controlSelectionFlag = 1`：表示进入控件选择模式。
 * - `menuSwitchFlag = 1`：触发菜单界面刷新，使 UI 适应新的模式。
 */
void FunctionForCtrl(void)
{
  Menu_EnsureStorage(currentMenu);

  if (!currentMenu || !currentMenu->items || currentMenu->itemCount == 0)
  {
    return;
  }

  ControlTypedef *control = currentMenu->items[currentMenu->currentItemIndex].control;
  if (!control || !control->data)
  {
    return;
  }

  controlSelectionFlag = 1;
  menuSwitchFlag = 1;
}

/**
 * @brief 进入当前选中菜单项的子菜单。
 * 
 * 如果当前选中的菜单项包含子菜单，则切换 `currentMenu` 到对应的子菜单，并触发菜单切换标志。
 * 
 * @note 
 * - `currentMenu` 会被更新为选中菜单项的 `subMenu`。
 * - `menuSwitchFlag = 1` 触发 UI 重新绘制，以显示新的子菜单。
 * - 如果当前菜单项没有子菜单，则不执行任何操作。
 */
void FunctionForNextMenu(void)
{
  Menu_EnsureStorage(currentMenu);

  if (!currentMenu || !currentMenu->items || currentMenu->itemCount == 0)
  {
    return;
  }

  if (currentMenu->items[currentMenu->currentItemIndex].subMenu)
  {
    currentMenu = currentMenu->items[currentMenu->currentItemIndex].subMenu;
    menuSwitchFlag = 1;
  }
}
