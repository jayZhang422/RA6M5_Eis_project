# BSP External IRQ (ICU) 模块说明文档

## 1. 概述
本模块封装了 Renesas RA 系列的 ICU (Interrupt Controller Unit) 中的外部中断功能 (`r_icu`)。
主要用于处理 GPIO 外部中断，如按键按下、传感器 Data Ready 信号等。

### 核心特性
* **O(1) 中断分发**：利用 FSP 上下文机制，在 ISR 中直接定位回调函数，避免了多中断源时的轮询开销，实现极低延迟响应。
* **简单易用**：统一了 Init、RegisterCallback、Enable/Disable 流程。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列
* **底层驱动**：`r_icu`

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 Stacks 中添加 `External IRQ (r_icu)`。
2.  配置 Name (例如 `g_external_irq0`)。
3.  **Callback**: 设置为 `bsp_irq_common_isr`。
4.  **Channel**: 选择对应的 IRQ 通道（需查看原理图）。
5.  **Trigger**: 设置触发方式（Rising/Falling/Both/Low Level）。
6.  **Digital Filter**: 建议对按键开启滤波，对高速信号关闭滤波。

### 步骤 2: bsp_config.h 配置
```c
#define KEYDEV_INSTANCE  &g_external_irq0