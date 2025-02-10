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
 * @brief 主菜单项。
 * 
 * 定义了主菜单的静态项。
 */
ItemTypedef mainMenuItems[] = 
{
  {"example", 7, NULL, NULL, NULL}, /**< 示例菜单项 */

};

/**
 * @brief 主菜单。
 * 
 * 包含主菜单的名称、菜单项、父菜单以及菜单项数量。
 */
Menutypedef mainMenu = {"mainMenu", mainMenuItems, NULL, sizeof(mainMenuItems) / sizeof(ItemTypedef), 0};

Menutypedef *currentMenu = &mainMenu;

/**
 * @brief 按键列表。
 * 
 * 存储所有按键的状态信息。
 */
KeyTypeDef KeyList[2] = {0};

/**
 * @brief 菜单框的 Y 坐标。
 */
UIElemTypedef frameY = 
{
  .val = 0,         /**< 当前值 */
  .targetVal = 0,   /**< 目标值 */
  .lastVal = 0      /**< 上一次值 */
};

/**
 * @brief 菜单框的宽度。
 */
UIElemTypedef frameWidth = 
{
  .val = 0,         /**< 当前值 */
  .targetVal = 0,   /**< 目标值 */
  .lastVal = 0      /**< 上一次值 */
};

/**
 * @brief 屏幕顶部的 Y 坐标。
 */
UIElemTypedef screenTop = 
{
  .val = 0,         /**< 当前值 */
  .targetVal = 0,   /**< 目标值 */
  .lastVal = 0      /**< 上一次值 */
};

/**
 * @brief 滚动条的 Y 坐标。
 */
UIElemTypedef scrollBarY = 
{
  .val = 2,         /**< 当前值 */
  .targetVal = 2,   /**< 目标值 */
  .lastVal = 2      /**< 上一次值 */
};

/**
 * @brief 开关控件条的 Y 坐标。
 */
UIElemTypedef switchCtrlBar = 
{
  .val = 64 + 2,    /**< 当前值 */
  .targetVal = 64 + 2, /**< 目标值 */
  .lastVal = 64 + 2 /**< 上一次值 */
};

/**
 * @brief 显示控件条的 Y 坐标。
 */
UIElemTypedef displayCtrlBar = 
{
  .val = 0,         /**< 当前值 */
  .targetVal = 0,   /**< 目标值 */
  .lastVal = 0      /**< 上一次值 */
};

/**
 * @brief 屏幕索引。
 * 
 * 包含屏幕顶部和底部菜单项的索引。
 */
ScreenIndexTypedef screenIndex = 
{
  .topIndex = 0,    /**< 顶部索引 */
  .bottomIndex = 3  /**< 底部索引 */
};

/**
 * @brief 当前按键 ID。
 * 
 * 0 表示没有按键响应，1 表示 KEY1，2 表示 KEY2。
 */
uint8_t keyID = 0;

/**
 * @brief 菜单切换标志。
 * 
 * 0 表示没有切换菜单，1 表示切换菜单。
 */
uint8_t menuSwitchFlag = 0;

/**
 * @brief 控件选择标志。
 * 
 * 0 表示没有选择控件，1 表示选择控件。
 */
uint8_t controlSelectionFlag = 0;

/**
 * @brief 动画进度变量，用于平滑过渡 UI 组件的移动效果。
 * 
 * 这些变量的值范围通常为 0.0 到 1.0，表示当前动画的进度。
 */

/** @brief 框架 Y 方向的移动进度 */
float moveProcess_FrameY = 0.0;

/** @brief 框架宽度变化的移动进度 */
float moveProcess_FrameWidth = 0.0;

/** @brief 屏幕滚动的移动进度 */
float moveProcess_Screen = 0.0;

/** @brief 滚动条的移动进度 */
float moveProcess_ScrollBar = 0.0;

/** @brief 开关控件滑块的移动进度 */
float moveProcess_SwitchCtrlBar = 0.0;


// ------------函数定义------------// 
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
 * @param name       菜单名称。
 * @param items      菜单项数组（可为 NULL）。
 * @param itemCount  菜单项数量（如果 items 为 NULL，传 0）。
 * @param parentMenu 父级菜单（可为 NULL）。
 * @return Menutypedef* 返回新创建的菜单指针，失败时返回 NULL。
 */
Menutypedef *AddMenu(const char *name, ItemTypedef *items, uint16_t itemCount, Menutypedef *parentMenu)
{
  if (!name)
    return NULL;

  // 分配菜单结构体
  Menutypedef *newMenu = (Menutypedef *)malloc(sizeof(Menutypedef));
  if (!newMenu)
    return NULL;

  // 分配菜单名称
  newMenu->menuName = (char *)malloc(strlen(name) + 1);
  if (!newMenu->menuName)
  {
    free(newMenu);
    return NULL;
  }
  strcpy(newMenu->menuName, name);

  // 初始化菜单项
  if (items && itemCount > 0)
  {
    newMenu->items = items;          // 指向现有菜单项数组
    newMenu->itemCount = itemCount; // 设置菜单项数量
  }
  else
  {
    newMenu->items = NULL;          // 无菜单项时，初始化为空
    newMenu->itemCount = 0;
  }

  // 初始化其他属性
  newMenu->currentItemIndex = 0;
  newMenu->parentMenu = parentMenu;

  return newMenu;
}


/**
 * @brief 向指定菜单添加一个新的菜单项。
 * 
 * 该函数动态创建并初始化菜单项，并将其添加到 `menu->items` 数组中。
 * 如果 `menu->items` 为空，则先分配内存。如果已有菜单项，则使用 `realloc` 进行扩展。
 * 
 * @param menu     指向目标菜单的指针。
 * @param name     菜单项的名称（字符串）。
 * @param funtion  菜单项对应的回调函数（可为 NULL）。
 * @param subMenu  该菜单项的子菜单（可为 NULL）。
 * @param ctrlMode 控件模式（如 SWITCH_CTRL、DISPLAY_CTRL 等，可为 NULL）。
 * @param ctrlData 控件数据指针（用于存储控件的值，可为 NULL）。
 * @return ItemTypedef* 返回新创建的菜单项指针，若失败则返回 NULL。
 * 
 * @note 
 * - 该函数会为 `name` 和 `control` 结构体分配内存，确保数据的独立性。
 * - 如果 `menu->items` 为空，会先分配空间，否则使用 `realloc` 扩展。
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

  ItemTypedef *newItem = (ItemTypedef *)malloc(sizeof(ItemTypedef));
  if (!newItem)
    return NULL;
  
  newItem->str = (char *)malloc(strlen(name) + 1);
  if (!newItem->str)
  {
    free(newItem);
    return NULL;
  }
  strcpy(newItem->str, name);
  newItem->len = strlen(newItem->str);
  newItem->subMenu = subMenu;
  newItem->Function = funtion;
  
  if (ctrlMode != 0)
  {
    newItem->control = (ControlTypedef *)malloc(sizeof(ControlTypedef));
    if (!newItem->control)
    {
      free(newItem->str);
      free(newItem);
      return NULL;
    }

    newItem->control->mode = ctrlMode;
    newItem->control->data = ctrlData;
  }
  else 
    newItem->control = NULL;
  
  if (!menu->items)
  {

    menu->items = (ItemTypedef *)malloc(sizeof(ItemTypedef));
    if (!menu->items) 
    {
      if (newItem->control)
        free(newItem->control);
      free(newItem->str);
      free(newItem);
      return NULL;
    }
    menu->itemCount = 0; 
  }
  else 
  {
    ItemTypedef *temp = (ItemTypedef *)realloc(menu->items, (menu->itemCount + 1) * sizeof(ItemTypedef));
    if (!temp)
    {
      if (newItem->control)
        free(newItem->control);
      free(newItem->str);
      free(newItem);
      return NULL;
    }
    menu->items = temp;
  }

  menu->items[menu->itemCount] = *newItem;
  menu->itemCount++;

  free(newItem);
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
  OLED_Init();
  memset(KeyList, 0, sizeof(KeyList));
  frameWidth.targetVal = currentMenu->items[currentMenu->currentItemIndex].len * 6 + 4;
  HAL_Delay(20);
}


/**
 * @brief 更新 UI 状态。
 * 
 * 根据按键状态标志更新 UI，并触发对应的短按或长按逻辑。
 * 
 * - 如果按键为短按，调用 `KeyShortPress`。
 * - 如果按键为长按，调用 `KeyLongPress`。
 * - 更新框架、屏幕、滚动条和控件条的显示。
 */
void UI_UpDate()
{
  if (KeyList[keyID - 1].updateFlag)
  {
    KeyList[keyID - 1].updateFlag = 0;

    if (KeyList[keyID - 1].longPress == 2)
    {
      KeyShortPress(); /**< 处理短按逻辑 */
    }
    else if (KeyList[keyID - 1].longPress == 1)
    {
      KeyLongPress(); /**< 处理长按逻辑 */
    }

    Frame_Update();        /**< 更新框架显示 */
    Screen_Update();       /**< 更新屏幕显示 */
    ScrollBar_Update();    /**< 更新滚动条显示 */
    switchCtrlBar_Update();/**< 更新开关控件条显示 */
  }
}

/**
 * @brief 处理按键长按逻辑。
 * 
 * 根据当前的控件选择状态和按键 ID，执行不同的操作：
 */
void KeyLongPress()
{
  if (controlSelectionFlag == 0)
  {
    if (keyID == 1)
    {
      // 执行当前菜单项的功能函数
      if (currentMenu->items[currentMenu->currentItemIndex].Function)
      {
        currentMenu->items[currentMenu->currentItemIndex].Function();
      }
    }
    else if (keyID == 2)
    {
      // 返回父菜单
      if (currentMenu->parentMenu)
      {
        currentMenu = currentMenu->parentMenu;
        menuSwitchFlag = 1; // 设置菜单切换标志
      }
    }
  }
  else if (controlSelectionFlag == 1)
  {
    if (keyID == 1)
    {
      // 按键 1 暂未实现功能
    }
    else if (keyID == 2)
    {
      // 退出控件选择状态
      controlSelectionFlag = 0;
      menuSwitchFlag = 1; // 设置菜单切换标志
    }
  }
}

/**
 * @brief 处理按键短按逻辑。
 * 
 * 根据控件选择状态和按键 ID，执行不同的操作
 */
void KeyShortPress()
{
  if (controlSelectionFlag == 0)
  {
    if (keyID == 1)
    {
      currentMenu->currentItemIndex++; /**< 移动到下一个菜单项 */
    }
    else if (keyID == 2)
    {
      currentMenu->currentItemIndex--; /**< 移动到上一个菜单项 */
    }

    // 循环索引
    if (currentMenu->currentItemIndex == currentMenu->itemCount)
    {
      currentMenu->currentItemIndex = 0; /**< 回到菜单开头 */
    }
    else if (currentMenu->currentItemIndex == -1)
    {
      currentMenu->currentItemIndex = currentMenu->itemCount - 1; /**< 回到菜单结尾 */
    }
  }
  else if (controlSelectionFlag == 1)
  {
    // 控件操作逻辑
    switch (currentMenu->items[currentMenu->currentItemIndex].control->mode)
    {
    case SWITCH_CTRL:
      // 切换开关状态
      if (keyID == 1 || keyID == 2)
      {
        if (*(currentMenu->items[currentMenu->currentItemIndex].control->data) == 0)
        {
          *(currentMenu->items[currentMenu->currentItemIndex].control->data) = 1; /**< 设置为开启 */
        }
        else
        {
          *(currentMenu->items[currentMenu->currentItemIndex].control->data) = 0; /**< 设置为关闭 */
        }
      }
      break;
    
    case DISPLAY_CTRL:
      // 显示控件，暂不处理
      break;
    
    case SLIDER_CTRL:
      // 调整滑动条值
      if (keyID == 1)
      {
        *(currentMenu->items[currentMenu->currentItemIndex].control->data) += 1; /**< 增加值 */
      }
      else if (keyID == 2)
      {
        *(currentMenu->items[currentMenu->currentItemIndex].control->data) -= 1; /**< 减小值 */
      }
      break;
    
    default:
      break;
    }
  }
}

/**
 * @brief 更新框架显示目标值。
 * 
 * 根据当前菜单项索引，更新框架的 Y 坐标和宽度的目标值。
 */
void Frame_Update()
{
  moveProcess_FrameY = 0.0;
  moveProcess_FrameWidth = 0.0;

  frameY.lastVal = frameY.targetVal;
  frameWidth.lastVal = frameWidth.targetVal;

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

  float moveRangef = (float)((OLED_SCREEN_HEIGHT - 4) - (OLED_SCREEN_HEIGHT * 2) / currentMenu->itemCount) / (currentMenu->itemCount - 1);
  int moveRange = (int)(moveRangef + 0.5f);
  if (currentMenu->itemCount == 2 || currentMenu->itemCount == 1)
  {
    scrollBarY.targetVal = (currentMenu->currentItemIndex * 10) + 2;
  }
  else
  {
    if (currentMenu->currentItemIndex == currentMenu->itemCount - 1)
    {
      scrollBarY.targetVal = OLED_SCREEN_HEIGHT - 2 - (OLED_SCREEN_HEIGHT * 2) / currentMenu->itemCount;
    }
    else
    {
      scrollBarY.targetVal = (currentMenu->currentItemIndex * moveRange) + 2;
    }
  }
}

/**
 * @brief 更新开关控件条位置。
 * 
 * 根据控件数据状态（0 或 1）调整开关控件条的目标位置。
 */
void switchCtrlBar_Update(void)
{
  moveProcess_SwitchCtrlBar = 0.0;
  switchCtrlBar.lastVal = switchCtrlBar.val;

  if (*(currentMenu->items[currentMenu->currentItemIndex].control->data) == 0)
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
    HAL_Delay(50); /**< 在 FreeRTOS 中使用非阻塞式延时 */
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
  int16_t showItemEndNum = 0;
  int16_t showItemStartNum = 0;
  if (currentMenu->itemCount > 0) 
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
  OLED_DrawEmptyRectangle(16, 8, 96, 48);

  DrawItemName(currentMenu->items[currentMenu->currentItemIndex].str, 16 + 2, 8 + 2);

  switch (currentMenu->items[currentMenu->currentItemIndex].control->mode)
  {
    case SWITCH_CTRL:
    {
      DrawSwitchControl(currentMenu->items[currentMenu->currentItemIndex].control);
      break;
    }
    case DISPLAY_CTRL:
    {
      DrawDisplayControl(currentMenu->items[currentMenu->currentItemIndex].control);
      break;
    }
    case SLIDER_CTRL:
    {
      DrawSliderControl(currentMenu->items[currentMenu->currentItemIndex].control);
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
  if (control)
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
  if (currentMenu->items[currentMenu->currentItemIndex].subMenu)
  {
    currentMenu = currentMenu->items[currentMenu->currentItemIndex].subMenu;
    menuSwitchFlag = 1;
  }
}
