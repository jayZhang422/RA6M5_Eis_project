# BSP DAC 模块说明文档

## 1. 概述
本模块封装了 Renesas RA 系列的 DAC (Digital to Analog Converter) 驱动。
遵循**高效、原子性**原则，驱动层只负责将 Raw Data 写入寄存器，复杂的波形逻辑交由应用层或 DMAC 处理。

### 核心特性
* **Raw Value 接口**：直接传递 `uint16_t` (0-4095)，避免驱动层进行低效的浮点运算。
* **线程安全**：内置 Mutex，防止多个线程竞争同一 DAC 通道。
* **自动管理**：初始化时自动完成 Open 和 Start。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列 (RA2/RA4/RA6 等支持 DAC 的型号)
* **RTOS**：Azure RTOS ThreadX
* **底层驱动**：`r_dac`

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 Stacks 中添加 `DAC (r_dac)`。
2.  配置 Name (例如 `g_dac0`)。
3.  **Channel**: 选择对应的 DAC 通道 (DA0 或 DA1)。
4.  **Data Format**: 建议选择 `12-bit right-aligned` (右对齐)，这样 0-4095 直接对应 0-Vref。
5.  **Output Amplifier**: 如果需要驱动能力，请开启内部放大器。
6.  **Pins**: 确认引脚分配 (通常是 P014 或 P015)。

### 步骤 2: bsp_config.h 配置
在 `bsp_config.h` 中添加实例定义：
```c
#define DAC0_INSTANCE  &g_dac0