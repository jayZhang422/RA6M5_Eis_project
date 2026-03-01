# BSP SPI 模块说明文档

## 1. 概述
本模块封装了 Renesas RA 系列的 SPI 驱动。
通过 ThreadX 的 **Semaphore** 实现异步转同步，通过 **Mutex** 实现线程安全，支持多线程共享同一 SPI 总线（如同时连接 Flash 和 LCD）。

### 核心特性
* **原生 API 优化**：`Write`、`Read`、`Transfer` 分别调用底层的 `write`、`read`、`writeRead`，避免了不必要的缓冲区拷贝和 Dummy 数据处理，实现最高效率。
* **零延迟上下文**：ISR 使用 Context 指针直接定位设备，无需遍历数组，适合高频 SPI 通信。
* **自动片选管理**：在每次传输前后自动拉低/拉高 CS 引脚。
* **优先级反转保护**：使用 `TX_INHERIT` 属性的互斥锁。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列
* **RTOS**：Azure RTOS ThreadX
* **底层驱动**：`r_spi`

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 Stacks 中添加 `SPI (r_spi)`。
2.  配置 Name (例如 `g_spi0`)。
3.  **Callback**: 设置为 `spi_common_callback`。
4.  **Bit Width**: 通常设为 8 Bits。
5.  **Pins**: 配置 SCK, MOSI, MISO。**CS 引脚**配置为 GPIO Output 模式 (不使用 SPI 硬件 SSL，而是软件控制 GPIO)。

### 步骤 2: bsp_config.h 配置
```c
#define LCDDEV_INSTANCE  &g_spi0