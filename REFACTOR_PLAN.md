# OLED 菜单框架重构计划

本文档记录 OLED 多级菜单项目的重构路线。重构目标不是一次性推翻现有代码，而是在保留当前视觉效果和示例用法的基础上，逐步把 demo 风格代码整理成稳定、可移植、可扩展的小型嵌入式 UI 框架。

## 当前状态

- 阶段 0 已完成：问题基线记录在 `PROJECT_ISSUES.md`。
- 阶段 1 已完成：修复内存扩容、按键越界、控件空指针、滚动条除零和 OLED DMA 发送等待。
- 阶段 2 已完成：新增 `MenuEventTypedef` 和 `Menu_HandleEvent()`，默认按键逻辑已转换为事件驱动。
- 阶段 3 已完成：新增 `MenuContextTypedef`、`defaultMenuContext`、`Menu_SetContext()`，旧全局变量名通过兼容宏映射到默认上下文。
- 阶段 4 已完成：菜单对象、菜单项和控件信息改为固定容量静态池。
- 阶段 5 已部分完成：渲染函数仍保持原函数名，但状态已经通过上下文兼容层访问。
- 阶段 6 已完成：新增 `MenuPlatformTypedef` 和 `Menu_SetPlatform()`，菜单切换延时可替换。
- 阶段 7 已完成：更新 `README.md`，新增 `MIGRATION.md` 和 `LICENSE`。

## 总体目标

- 菜单数据、输入事件、UI 状态、渲染和硬件平台解耦。
- 避免运行期未定义行为和随机 HardFault。
- 减少全局变量之间的隐式耦合。
- 保留现有 `UI_Init()`、`UI_UpDate()`、`UI_Move()`、`UI_Show()` 的兼容入口。
- 后续支持更清晰的静态菜单表、固定容量菜单池、更多控件和不同输入设备。

## 阶段 0：冻结现状

### 目标

记录当前行为和问题，避免重构过程中丢掉已有功能。

### 任务

1. 记录现有 API：
   - `AddMenu()`
   - `AddMenuItem()`
   - `UI_Init()`
   - `UI_UpDate()`
   - `UI_Move()`
   - `UI_Show()`

2. 记录当前示例覆盖的功能：
   - 普通菜单项
   - 开关控件
   - 子菜单跳转

3. 以 `PROJECT_ISSUES.md` 作为问题基线。

## 阶段 1：稳定性修复

### 目标

先修复当前代码中最容易导致崩溃、内存破坏和显示异常的问题，不大改架构。

### 任务

1. 修复 `mainMenuItems` 静态数组被 `realloc()` 的未定义行为。
2. 修复 `UI_UpDate()` 中 `KeyList[keyID - 1]` 越界风险。
3. 修复普通菜单项没有 `control` 时的空指针访问。
4. 修复 `ScrollBar_Update()` 在 `itemCount <= 1` 时的除 0 风险。
5. 修复 `OLED_Send()` 使用 DMA 但未等待传输完成的问题。
6. 给关键绘制函数增加必要的空指针保护。

### 验证

- 示例代码仍可使用原来的 API。
- 普通菜单项、开关控件、子菜单切换路径不再依赖未定义行为。
- OLED buffer 不会在 DMA 传输未完成时被复用。

## 阶段 2：输入事件层

### 目标

将 GPIO 按键状态转换为菜单事件，让菜单核心不直接依赖 `keyID`。

### 计划接口

```c
typedef enum {
  MENU_EVENT_NONE,
  MENU_EVENT_NEXT,
  MENU_EVENT_PREV,
  MENU_EVENT_ENTER,
  MENU_EVENT_BACK,
} MenuEvent;
```

### 任务

1. 中断回调只维护按键状态。
2. 主循环或 `UI_UpDate()` 将按键状态转换为 `MenuEvent`。
3. 新增事件处理函数：

```c
void Menu_HandleEvent(MenuContext *ctx, MenuEvent event);
```

4. 暂时保留旧 API，让旧 API 内部调用新事件处理。

## 阶段 3：引入 `MenuContext`

### 目标

把散落的全局状态收敛到一个上下文对象中。

### 计划结构

```c
typedef struct {
  Menutypedef *currentMenu;
  ScreenIndexTypedef screenIndex;
  uint8_t controlSelectionFlag;
  uint8_t menuSwitchFlag;
} MenuState;

typedef struct {
  UIElemTypedef frameY;
  UIElemTypedef frameWidth;
  UIElemTypedef screenTop;
  UIElemTypedef scrollBarY;
  UIElemTypedef switchCtrlBar;
} MenuAnimState;

typedef struct {
  MenuState state;
  MenuAnimState anim;
} MenuContext;
```

### 任务

1. 先创建默认全局实例 `defaultMenuContext`。
2. 逐步让内部函数接收 `MenuContext *ctx`。
3. 保留旧全局变量一段时间，作为兼容层。
4. 最终让旧 API 变成对默认上下文的包装。

## 阶段 4：菜单数据模型重构

### 目标

减少运行期动态内存，提高嵌入式环境下的可预测性。

### 计划方向

1. 支持静态菜单表。
2. 支持固定容量菜单池。
3. 明确菜单项类型：

```c
typedef enum {
  MENU_ITEM_NORMAL,
  MENU_ITEM_ACTION,
  MENU_ITEM_SUBMENU,
  MENU_ITEM_CONTROL,
} MenuItemType;
```

4. 扩展控件信息：

```c
typedef struct {
  ControlModeTypedef mode;
  int *data;
  int min;
  int max;
  int step;
  void (*onChange)(int value);
} ControlTypedef;
```

## 阶段 5：渲染层独立

### 目标

让菜单渲染只读取菜单状态，不直接改变业务逻辑。

### 任务

1. 区分 OLED 底层绘制和菜单 UI 绘制。
2. 将 `DrawMenuItems()`、`DrawControlSelection()` 等函数改为接收上下文。
3. 统一动画推进入口：

```c
void Menu_Animate(MenuContext *ctx);
```

4. 保留当前视觉效果：
   - 选中框
   - 滚动条
   - 开关滑块
   - 菜单切换消失动画

## 阶段 6：平台层抽象

### 目标

让菜单核心不直接绑定 STM32 HAL 的 GPIO、I2C、DMA 和 Delay。

### 计划接口

```c
typedef struct {
  uint32_t (*getTick)(void);
  void (*delay)(uint32_t ms);
} MenuPlatform;
```

### 任务

1. 封装 `HAL_Delay()`。
2. 封装 tick 获取。
3. 将按键 GPIO 配置和菜单核心拆开。
4. README 中明确 `OLED_Send()` 是平台移植点。

## 阶段 7：文档和示例

### 目标

让项目更像一个可维护的开源库。

### 任务

1. 更新 `README.md`。
2. 修正 `oled_deaw` 拼写。
3. 增加 `LICENSE`。
4. 完善示例：
   - 普通菜单项
   - 子菜单
   - 开关控件
   - 数值显示控件
   - 滑条控件
5. 添加旧 API 到新 API 的迁移说明。

## 推荐执行顺序

1. 第一轮：完成阶段 1，先把当前代码修稳。
2. 第二轮：完成阶段 2 和阶段 3，引入事件层和上下文。
3. 第三轮：完成阶段 4、5、6，重构菜单模型、渲染层和平台层。
4. 第四轮：完成阶段 7，补文档、示例和许可证。
