# BSP USART (Serial) 模块说明文档

## 1. 概述
本模块封装了 Renesas RA 系列的 UART 驱动，结合 ThreadX 实现了**线程安全**的阻塞式发送与接收。
特别优化了 `BSP_Printf` 函数，使其在多线程环境下能安全地输出调试信息，不会发生字符乱序或覆盖。

### 核心特性
* **Context 注入 (O(1) ISR)**：通过 FSP 的 Context 机制，中断服务程序直接获取当前设备控制块，无需遍历数组，极大降低了高波特率下的 CPU 占用。
* **重入安全 Printf**：`BSP_Printf` 使用栈变量作为缓冲区，而非全局静态变量，支持多个线程同时向不同的串口打印数据而互不干扰。
* **优先级反转保护**：互斥锁使用 `TX_INHERIT` 属性，防止低优先级打印任务阻塞高优先级控制任务。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列
* **RTOS**：Azure RTOS ThreadX
* **底层驱动**：`r_sci_uart`

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 Stacks 中添加 `UART (r_sci_uart)`。
2.  配置 Name (例如 `g_uart0`)。
3.  **Callback**: 设置为 `usart_common_callback`。
4.  **Interrupt Priority**: 确保配置了合适的中断优先级。
5.  **DTC/DMA**: 如果数据量大，建议开启 TX/RX 的 DTC 支持。

### 步骤 2: bsp_config.h 配置
```c
#define DEBUGDEV_INSTANCE  &g_uart0