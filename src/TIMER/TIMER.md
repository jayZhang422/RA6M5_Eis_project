# BSP Timer (GPT) 模块说明文档

## 1. 概述
本模块封装了 Renesas RA 系列的 GPT (General PWM Timer) 驱动。
提供了通用定时器功能（定时中断、计数）以及 PWM 生成功能（频率设置、占空比设置）。

### 核心特性
* **O(1) 高效中断**：利用 FSP 的 `p_context` 机制，在中断服务程序中直接定位控制块，避免遍历数组，适合高频 PWM 或定时中断。
* **单位自动换算**：提供了 Hz 到 Ticks 的自动换算接口 `BSP_Timer_SetFreq_Hz`，用户无需手动计算时钟分频。
* **安全的 PWM 接口**：`BSP_Timer_SetDuty` 使用千分比（0-1000）作为参数，内部自动处理计数转换和溢出保护。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列 (RA2/RA4/RA6)
* **RTOS**：Azure RTOS ThreadX (本模块主要使用 RTOS 宏，无复杂 IPC 依赖)
* **底层驱动**：`r_gpt`

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 Stacks 中添加 `Timer, General PWM (r_gpt)`。
2.  配置 Name (例如 `g_timer0`)。
3.  **Callback**: 设置为 `gpt_common_isr`。
4.  **Pins**: 如果用作 PWM，需在 Pins 标签页配置 GTIOCA/B 引脚。
5.  **Mode**: 如果是 PWM，Mode 设为 PWM；如果是纯定时器，Mode 设为 Periodic。

### 步骤 2: bsp_config.h 配置
```c
#define LEDPWM_INSTANCE  &g_timer0