# BSP I2C 模块说明文档

## 1. 概述
本模块为 Renesas FSP I2C Master 驱动提供了一层基于 ThreadX 的封装。
主要功能是将 FSP 的**异步中断模式**转换为应用层易用的**同步阻塞模式**，并提供了**线程安全**保护。

### 核心特性
* **同步阻塞调用**：调用 Read/Write 函数时，线程会挂起（Suspend），直到传输完成或超时，不占用 CPU 轮询。
* **线程安全**：内置 Mutex（互斥锁），支持多线程访问同一 I2C 总线，防止数据冲突。
* **高效率**：ISR 采用 O(1) 复杂度设计，无遍历开销。
* **逻辑抽象**：应用层只需操作 `BSP_I2C_EEPROM` 等逻辑 ID，无需关心底层 `g_i2c0` 等实例细节。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列 (支持 FSP)
* **RTOS**：Azure RTOS ThreadX
* **底层驱动**：r_iic_master 或 r_sci_i2c

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 Stacks 中添加 `I2C Master` 驱动。
2.  配置 Name (例如 `g_i2c0`)。
3.  **重要**：设置 Callback 为 `bsp_i2c_common_isr`。
4.  确保中断优先级配置正确。

### 步骤 2: bsp_config.h 配置
在 `bsp_config.h` 中定义硬件实例和超时时间（与 `.c` 中的表对应）：
```c
#define EEPROMDEV_INSTANCE  &g_i2c0
#define EEPROMDEV_TIMEOUT   100  // ms