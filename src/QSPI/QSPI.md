# BSP QSPI 模块说明文档

## 1. 概述
本模块封装了 Renesas RA 系列的 QSPI (Quad SPI) 驱动，专为外挂 Flash 存储器设计。
模块结合 ThreadX 实现了**线程安全**的读写操作，并利用 **XIP (Execute In Place)** 技术实现了高效数据读取。

### 核心特性
* **自动分页写入**：`BSP_QSPI_Write` 内部实现了 Page Boundary 检查和分包，用户可随意写入任意长度数据，无需关心 Flash 的 256 字节页限制。
* **XIP 高速读取**：`BSP_QSPI_Read` 默认使用内存映射 (`memcpy`) 方式读取。相比传统的 SPI 指令模式，XIP 利用硬件预取和缓存，速度极快。
* **零轮询开销**：在等待 Flash 擦写（Busy）期间，使用 `tx_thread_sleep` 挂起线程，不占用 CPU 资源。
* **优先级反转保护**：互斥锁采用 `TX_INHERIT` 属性，确保高优先级任务访问总线时的实时性。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列 (RA4/RA6 等支持 QSPI 的型号)
* **FSP 版本**：兼容 FSP 3.x / 4.x / 5.x
* **RTOS**：Azure RTOS ThreadX
* **底层驱动**：`r_qspi`

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 Stacks 中添加 `QSPI` 驱动。
2.  配置 Name (例如 `g_qspi0`)。
3.  **关键配置**：
    * Address Mode: 必须根据 Flash 芯片手册选择 (3-Byte 或 4-Byte)。
    * Data Lines: 通常选择 `Quad` (4线) 以获得最高速度。
4.  确保引脚配置正确。

### 步骤 2: 代码集成
确保 `bsp_qspi.c` 中的 `g_qspi_cfg_map` 数组包含正确的硬件实例：
```c
static const bsp_qspi_static_cfg_t g_qspi_cfg_map[BSP_QSPI_NUM_MAX] = 
{
    /* 这里的 &g_qspi0 需对应 RASC 生成的变量 */
    [BSP_QSPI_FLASH_0] = { .p_hal_instance = &g_qspi0, .mutex_name = "QSPI0_M" },
};