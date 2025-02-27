# OLED Menu System for STM32

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

OLED多级菜单系统，支持动态动画效果和多种交互控件，适用于嵌入式设备的用户界面开发

（暂时只支持英文，后续有时间会增加中文支持）

视频介绍：

https://www.bilibili.com/video/BV1y7NYeMELy/?spm_id_from=333.1387.homepage.video_card.click&vd_source=db23359a52cb1ccca808fd2611658f09

问题整理:
  - 两个按键gpio的中断需要上升沿和下降沿都触发中断！
  - 如果创建的内容较多需要再cubemx的Project Manager中的Linker Settings将堆栈内存设置大一些
    - 如果在freertos中使用直接将堆栈分配大一些
  - 在freertos中使用时需要在`oled_draw.c`中`InterfaceSwitch()`将延时函数改为非阻塞的


---

## 功能特性

- **多级菜单支持**：灵活嵌套的树形菜单结构
- **平滑动画过渡**：菜单切换和控件操作均支持缓动动画
- **丰富控件类型**：
  - 开关控件（ON/OFF）
  - 数值显示控件
  - 滑动条控件
- **智能滚动条**：根据菜单项数量自适应调整
- **按键响应优化**：支持长短按区分处理
- **低内存占用**：显存动态管理，适配资源受限设备

---

## 硬件依赖

- MCU：STM32（需支持HAL库）
- 显示屏：128x64 OLED（SSD1306驱动）
- 输入：两个物理按键（左/右方向）

---

## 快速开始

### 1. 移植驱动
将以下文件添加到工程：

`oled_menu.h` `oled_deaw.h` `oled_menu_types.h` `font.h`移植到Inc文件夹

`oled_menu.c` `oled_deaw.c` `font.c`移植到Src文件夹

### 2. CubeMX配置
![GPIO配置](images/2025-02-28012933.png)

### 3. 按键配置
在`oled_menu.h`中修改GPIO定义：
```c
#define RIGHT_KEY_GPIOX    GPIOC
#define RIGHT_KEY_GPIO_PIN GPIO_PIN_8
#define LEFT_KEY_GPIOX     GPIOC
#define LEFT_KEY_GPIO_PIN  GPIO_PIN_9
```
并且需要开启两个按键gpio的中断
且需要上升沿和下降沿都触发中断！

### 4. OLED_Send()函数配置
在`oled_draw.c`中的OLED_Send是移植本项目时的重要函数

```c
void OLED_Send(uint8_t *data, uint8_t len)
{
  while(HAL_I2C_Master_Transmit_DMA(&hi2c1, OLED_ADDRESS, data, len) != HAL_OK);
}
```

默认使用stm32 i2c+DMA（建议在cubemx中配置i2c为高速模式）, 如有其他i2c或spi需要请自行更改


### 5. api调用

`AddMenu()`添加新菜单   `AddMenuItem()`添加菜单项

在初始化中调用   `UI_Init()`

在主循环中调用   `UI_UpDate()` `UI_Move()` `UI_Show()` 

具体使用见例程和视频
