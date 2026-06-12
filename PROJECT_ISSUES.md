# OLED 多级菜单项目问题整理

本文档整理当前项目中已经发现的主要稳定性、移植性和维护性问题。项目整体结构清晰，菜单树、控件、动画和 OLED 绘制已经有较完整的雏形；下面的问题主要集中在 C 内存管理、按键状态机、DMA 传输生命周期和边界条件保护。

## 1. `mainMenu` 静态数组被 `realloc`

### 位置

- `User/Src/oled_menu.c`
- `Example/main.c`

### 现象

`mainMenuItems` 是全局静态数组：

```c
ItemTypedef mainMenuItems[] =
{
  {"example", 7, NULL, NULL, NULL},
};

Menutypedef mainMenu = {"mainMenu", mainMenuItems, NULL, sizeof(mainMenuItems) / sizeof(ItemTypedef), 0};
```

但是 `AddMenuItem(&mainMenu, ...)` 会在内部执行：

```c
realloc(menu->items, (menu->itemCount + 1) * sizeof(ItemTypedef));
```

`realloc()` 只能处理由 `malloc()` / `calloc()` / `realloc()` 分配出来的堆内存，不能处理全局静态数组。

### 可能影响

- STM32 上直接 HardFault。
- 堆管理器元数据被破坏。
- 菜单项文字乱码、控件指针异常。
- Debug 正常、Release 异常，或者换编译优化等级后随机出问题。

### 建议修法

优先方案之一：

```c
Menutypedef mainMenu = {"mainMenu", NULL, NULL, 0, 0};
```

然后初始化时全部通过 `AddMenuItem()` 添加菜单项。

或者改成固定容量数组，避免运行期 `realloc()`：

```c
#define MENU_MAX_ITEMS 16
ItemTypedef mainMenuItems[MENU_MAX_ITEMS];
```

嵌入式项目更推荐固定容量，或者只在初始化阶段分配，运行期不再动态扩容。

## 2. `UI_UpDate()` 使用 `KeyList[keyID - 1]` 有越界风险

### 位置

- `User/Src/oled_menu.c`

### 现象

`keyID` 初始值是 0：

```c
uint8_t keyID = 0;
```

但 `UI_UpDate()` 中直接使用：

```c
KeyList[keyID - 1]
```

当 `keyID == 0` 时，会访问：

```c
KeyList[-1]
```

这是数组越界访问。

### 可能影响

- 读到随机值，导致未按键时也触发短按或长按。
- 写坏 `KeyList` 前面的全局变量。
- 菜单乱跳、显示异常、状态机异常。
- STM32 上可能 HardFault。

### 建议修法

最小修法是在使用前检查 `keyID`：

```c
if (keyID == 0 || keyID > 2)
{
  return;
}
```

更推荐的写法是遍历按键状态，不依赖全局 `keyID` 做数组索引：

```c
void UI_UpDate(void)
{
  for (uint8_t i = 0; i < 2; i++)
  {
    if (KeyList[i].updateFlag)
    {
      KeyList[i].updateFlag = 0;
      keyID = i + 1;

      if (KeyList[i].longPress == SHORT_PRESS)
      {
        KeyShortPress();
      }
      else if (KeyList[i].longPress == LONG_PRESS)
      {
        KeyLongPress();
      }

      Frame_Update();
      Screen_Update();
      ScrollBar_Update();
      switchCtrlBar_Update();

      keyID = 0;
    }
  }
}
```

后续可以进一步把 `KeyShortPress()` 和 `KeyLongPress()` 改成接收 `keyID` 参数，减少全局状态依赖。

## 3. `switchCtrlBar_Update()` 无条件解引用 `control`

### 位置

- `User/Src/oled_menu.c`

### 现象

当前 `UI_UpDate()` 每次处理按键后都会调用：

```c
switchCtrlBar_Update();
```

但 `switchCtrlBar_Update()` 内部直接访问：

```c
currentMenu->items[currentMenu->currentItemIndex].control->data
```

普通菜单项没有控件，`control` 可能是 `NULL`。

### 可能影响

- 选中普通菜单项后按键，可能空指针访问。
- STM32 上可能 HardFault。
- 菜单项既有普通项又有控件项时，问题更容易出现。

### 建议修法

进入函数后先判断当前 item 是否存在控件，且控件类型是否是 `SWITCH_CTRL`：

```c
void switchCtrlBar_Update(void)
{
  ControlTypedef *control = currentMenu->items[currentMenu->currentItemIndex].control;

  if (!control || control->mode != SWITCH_CTRL || !control->data)
  {
    return;
  }

  moveProcess_SwitchCtrlBar = 0.0;
  switchCtrlBar.lastVal = switchCtrlBar.val;

  if (*(control->data) == 0)
  {
    switchCtrlBar.targetVal = 64 + 2;
  }
  else
  {
    switchCtrlBar.targetVal = 64 - 30 + 2;
  }
}
```

也可以在 `UI_UpDate()` 中只对开关控件调用该函数。

## 4. `ScrollBar_Update()` 在 `itemCount == 1` 时可能除以 0

### 位置

- `User/Src/oled_menu.c`

### 现象

当前代码先计算：

```c
float moveRangef = (float)((OLED_SCREEN_HEIGHT - 4) - (OLED_SCREEN_HEIGHT * 2) / currentMenu->itemCount) / (currentMenu->itemCount - 1);
```

然后才判断：

```c
if (currentMenu->itemCount == 2 || currentMenu->itemCount == 1)
```

当 `itemCount == 1` 时，`currentMenu->itemCount - 1` 为 0。

### 可能影响

- 除 0 导致运行异常。
- 单项子菜单进入后可能触发问题。
- 滚动条位置计算不稳定。

### 建议修法

先处理 `itemCount <= 1` 的情况：

```c
void ScrollBar_Update(void)
{
  moveProcess_ScrollBar = 0.0;
  scrollBarY.lastVal = scrollBarY.val;

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
```

## 5. `OLED_Send()` 启动 DMA 后没有等待传输完成

### 位置

- `User/Src/oled_draw.c`

### 现象

当前发送函数：

```c
void OLED_Send(uint8_t *data, uint8_t len)
{
  while(HAL_I2C_Master_Transmit_DMA(&hi2c1, OLED_ADDRESS, data, len) != HAL_OK);
}
```

`HAL_I2C_Master_Transmit_DMA()` 返回 `HAL_OK` 只表示 DMA 传输成功启动，不表示数据已经传输完成。

同时 `OLED_ShowFrame()` 复用静态 buffer：

```c
static uint8_t sendBuffer[OLED_COLUMN + 1];
```

如果上一页 DMA 还没发完，下一页已经改写 `sendBuffer`，屏幕数据可能被覆盖。

### 可能影响

- OLED 显示花屏。
- 页面数据错乱。
- 偶发性显示异常，尤其 I2C 速度慢或主循环很快时。

### 建议修法

第一阶段建议保留 DMA，但让 `OLED_Send()` 等待传输完成后再返回：

```c
#define OLED_I2C_TIMEOUT_MS 100

void OLED_Send(uint8_t *data, uint8_t len)
{
  uint32_t tickStart = HAL_GetTick();

  while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
  {
    if (HAL_GetTick() - tickStart > OLED_I2C_TIMEOUT_MS)
    {
      return;
    }
  }

  tickStart = HAL_GetTick();

  while (HAL_I2C_Master_Transmit_DMA(&hi2c1, OLED_ADDRESS, data, len) != HAL_OK)
  {
    if (HAL_GetTick() - tickStart > OLED_I2C_TIMEOUT_MS)
    {
      return;
    }
  }

  tickStart = HAL_GetTick();

  while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
  {
    if (HAL_GetTick() - tickStart > OLED_I2C_TIMEOUT_MS)
    {
      return;
    }
  }
}
```

更简单的稳定方案是直接改成阻塞发送：

```c
void OLED_Send(uint8_t *data, uint8_t len)
{
  HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, data, len, 100);
}
```

真正异步 DMA 需要双缓冲或状态机配合 `HAL_I2C_MasterTxCpltCallback()`，改动会明显更大。

## 6. ISR 和主循环共享变量未使用 `volatile`

### 位置

- `User/Src/oled_menu.c`
- `User/Inc/oled_menu.h`

### 现象

`HAL_GPIO_EXTI_Callback()` 在中断中修改：

```c
KeyList
keyID
```

主循环中读取这些变量。对于中断和主循环共享的状态，通常应该使用 `volatile`，避免编译器优化导致主循环看不到最新值。

### 可能影响

- 按键事件偶发丢失。
- 优化等级变化后行为不一致。
- Debug 和 Release 表现不同。

### 建议修法

可以将中断共享状态声明为 `volatile`：

```c
volatile KeyTypeDef KeyList[2];
volatile uint8_t keyID;
```

如果结构体成员较多，也可以只让事件标志和按键 ID 使用 `volatile`，并在主循环中尽快复制到局部变量处理。

## 7. `InterfaceSwitch()` 使用阻塞延时

### 位置

- `User/Src/oled_menu.c`

### 现象

菜单切换动画中使用：

```c
HAL_Delay(50);
```

循环 3 次，总共阻塞约 150 ms。

### 可能影响

- FreeRTOS 中阻塞任务。
- 切换动画期间按键响应变差。
- 后续如果有传感器、通信任务，会影响实时性。

### 建议修法

裸机 demo 可以保留。若作为库发布，建议提供可替换延时接口，或者将切换动画改成非阻塞状态机。

例如提供弱函数：

```c
__weak void OLED_MenuDelay(uint32_t ms)
{
  HAL_Delay(ms);
}
```

FreeRTOS 用户可以重写为 `osDelay()`。

## 8. 控件数据缺少范围和空指针保护

### 位置

- `User/Src/oled_menu.c`

### 现象

`SLIDER_CTRL` 修改数据时直接自增自减：

```c
*(currentMenu->items[currentMenu->currentItemIndex].control->data) += 1;
```

绘制时才限制到 0 到 100：

```c
LIMIT_MAGNITUDE(*(control->data), 0, 100)
```

此外，部分路径没有检查 `control->data` 是否为空。

### 可能影响

- 滑条内部值可能超出显示范围。
- 用户数据被改成负数或超过 100。
- 如果 `ctrlData == NULL`，会空指针访问。

### 建议修法

在修改数据时就限制范围，并检查指针：

```c
if (control && control->data)
{
  *(control->data) = LIMIT_MAGNITUDE(*(control->data), 0, 100);
}
```

后续可以把控件扩展为带 `min`、`max`、`step` 字段，而不是固定 0 到 100。

## 9. README 和仓库元信息有小问题

### 位置

- `README.md`

### 现象

- README 中写了 `oled_deaw.c` / `oled_deaw.h`，应该是 `oled_draw.c` / `oled_draw.h`。
- README badge 标注 MIT，但仓库中当前没有看到 `LICENSE` 文件。
- README 中说暂时只支持英文，但代码中已有 `OLED_PrintString()` 和 `font16x16` 相关中文显示逻辑，描述可以更新。

### 可能影响

- 新用户移植时复制文件名出错。
- 开源协议不够明确。
- README 和代码能力不一致。

### 建议修法

- 修正拼写。
- 添加 `LICENSE` 文件。
- 更新 README 的中文显示说明，明确“菜单项默认使用 ASCII 绘制，底层绘制函数已有 UTF-8 字库接口”。

## 建议修复优先级

### P0：容易导致崩溃或内存破坏

1. 修复 `mainMenuItems` 静态数组被 `realloc`。
2. 修复 `KeyList[keyID - 1]` 越界。
3. 修复 `switchCtrlBar_Update()` 空指针访问。
4. 修复 `ScrollBar_Update()` 除 0。

### P1：容易导致显示异常或运行不稳定

1. 修复 `OLED_Send()` DMA 未等待完成。
2. 给中断和主循环共享变量增加 `volatile` 或重构事件传递。
3. 控件数据增加空指针和范围保护。

### P2：提升库化质量和移植体验

1. 将 `HAL_Delay()` 封装为可替换接口。
2. 减少运行期动态内存，或提供固定容量配置。
3. 整理 README、LICENSE 和示例代码。
4. 给菜单、控件 API 增加更明确的初始化和错误返回说明。

## 推荐第一轮修复路线

第一轮建议尽量不改公开 API，只做稳定性补丁：

1. `mainMenu` 改为动态初始化或固定容量。
2. `UI_UpDate()` 改为遍历 `KeyList`。
3. 所有控件访问前判断 `control` 和 `control->data`。
4. `ScrollBar_Update()` 先处理 `itemCount <= 1`。
5. `OLED_Send()` 等 DMA 完成，或先改为阻塞发送。

这样可以在保持现有项目结构的情况下，先把最危险的问题压下去。
