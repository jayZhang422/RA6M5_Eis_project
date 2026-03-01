# BSP ELC (Event Link Controller) 模块说明文档

## 1. 概述
ELC (Event Link Controller) 是瑞萨 RA 系列 MCU 的核心外设之一。
它允许将一个外设的事件（如定时器溢出、GPIO 中断、传输完成）直接链接到另一个外设的触发动作（如启动 ADC 采样、启动定时器、改变 GPIO 电平），而**完全不需要 CPU 介入**，也不需要中断 ISR。

### 核心特性
* **零延迟/零 CPU 占用**：纯硬件联动。
* **灵活配置**：支持多对多的事件映射。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列
* **底层驱动**：`r_elc`

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 Stacks 中添加 `Event Link Controller (r_elc)`。
2.  配置 Name (默认 `g_elc`)。
3.  **注意**：你可以在 RASC 的可视化界面中配置链接（Static Configuration），也可以在代码中动态配置（Dynamic Configuration）。本 BSP 封装了动态配置功能。

### 步骤 2: API 调用示例

#### 场景：定时器 0 溢出 -> 触发 ADC 0 采样

```c
/* 1. 初始化 ELC */
BSP_ELC_Init(BSP_ELC_0);

/* 2. 建立链接 
 * 事件源: GPT0 计数器溢出 (ELC_EVENT_GPT0_COUNTER_OVERFLOW)
 * 接收者: ADC0 (ELC_PERIPHERAL_ADC0)
 */
BSP_ELC_LinkSet(BSP_ELC_0, 
                ELC_EVENT_GPT0_COUNTER_OVERFLOW, 
                ELC_PERIPHERAL_ADC0);

/* 3. 启动定时器 */
// 定时器跑起来后，每溢出一次，ADC 就会自动开始转换，无需软件干预