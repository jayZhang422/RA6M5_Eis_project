# BSP DMAC/DTC 模块说明文档

## 1. 概述
本模块封装了 Renesas RA 系列的 DMAC (Direct Memory Access Controller) 或 DTC (Data Transfer Controller) 驱动。
重点提供了 **软件触发** 功能，支持线程安全的重配置与传输，利用 RTOS 信号量实现大数据块传输的同步等待。

### 核心特性
* **Software Trigger 封装**：支持 `SINGLE` (单次) 和 `REPEAT` (连续) 两种软件触发模式。
* **同步阻塞机制**：在 `REPEAT` 模式下，函数会自动挂起等待传输完成中断，无需死循环查询。
* **O(1) 中断效率**：利用 FSP Context 注入，极速定位控制块。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列
* **RTOS**：Azure RTOS ThreadX
* **底层驱动**：`r_dmac` 或 `r_dtc`

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 Stacks 中添加 `Transfer (r_dmac)` 或 `Transfer (r_dtc)`。
2.  配置 Name (例如 `g_transfer0`)。
3.  **Callback**: 设置为 `dmac_common_isr`。
4.  **Mode**: 通常设为 `Normal` (传输完停止)。
5.  **Activation Source**: 如果仅用于软件触发，可选 `None` 或任意未使用源；如果需要硬件触发，按需选择。

### 步骤 2: bsp_config.h 配置
```c
#define DMAC_INSTANCE  &g_transfer0