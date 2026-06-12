# Migration Notes

本文档记录菜单框架重构后的主要变化，以及旧代码迁移建议。

## 1. 静态内存池

菜单对象、菜单项和控件信息现在使用静态池，不再依赖运行期 `malloc()` / `realloc()` 扩容。

默认容量在 `User/Inc/oled_menu_types.h` 中配置：

```c
#define MENU_MAX_ITEMS 16
#define MENU_MAX_MENUS 8
```

如果 `AddMenu()` 或 `AddMenuItem()` 返回 `NULL`，通常表示对应静态池已满。

## 2. 字符串生命周期

`AddMenu()` 和 `AddMenuItem()` 不再复制名称字符串，只保存传入指针。

推荐使用字符串字面量：

```c
AddMenuItem(&mainMenu, "switch", FunctionForCtrl, NULL, SWITCH_CTRL, &switchData);
```

如果传入动态或局部缓冲区，必须保证该字符串在菜单生命周期内一直有效。

## 3. 输入事件层

新增菜单事件：

```c
MENU_EVENT_NEXT
MENU_EVENT_PREV
MENU_EVENT_ENTER
MENU_EVENT_BACK
```

默认 `UI_UpDate()` 仍会把左右键短按/长按转换为事件。

也可以绕过默认按键逻辑，直接调用：

```c
Menu_HandleEvent(MENU_EVENT_NEXT);
Menu_HandleEvent(MENU_EVENT_ENTER);
```

这让后续接入编码器、五向按键、触摸输入更简单。

## 4. MenuContext

新增默认上下文：

```c
extern MenuContextTypedef defaultMenuContext;
```

旧的全局变量名仍然保留为兼容宏，例如：

```c
currentMenu
frameY
screenIndex
controlSelectionFlag
```

新代码可以显式切换上下文：

```c
MenuContextTypedef ctx;
Menu_InitContext(&ctx, &mainMenu);
Menu_SetContext(&ctx);
```

## 5. 平台延时替换

`InterfaceSwitch()` 不再直接调用 `HAL_Delay()`，而是通过上下文平台接口延时。

裸机默认仍使用 `HAL_Delay()`。

FreeRTOS 中可以替换：

```c
static void MenuOsDelay(uint32_t ms)
{
  osDelay(ms);
}

MenuPlatformTypedef platform = {
  .get_tick = HAL_GetTick,
  .delay = MenuOsDelay,
};

Menu_SetPlatform(&defaultMenuContext, platform);
```

## 6. 仍保留的兼容 API

以下接口仍然可用：

```c
AddMenu()
AddMenuItem()
UI_Init()
UI_UpDate()
UI_Move()
UI_Show()
FunctionForCtrl()
FunctionForNextMenu()
```

旧示例整体不需要大改，但如果菜单数量或单个菜单项数量超过默认静态池容量，需要调大宏。
