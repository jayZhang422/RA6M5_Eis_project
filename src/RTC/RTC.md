# BSP RTC 模块说明文档

## 1. 概述
本模块封装了 Renesas RA 系列的 RTC (Real-Time Clock) 驱动。
提供了基于 **Unix Time 风格 (tm)** 到 **人类易读时间 (datetime)** 的自动转换，并加入了 **ThreadX 互斥锁** 以保障多线程环境下的安全性。

### 核心特性
* **自动格式转换**：API 层直接使用 `year=2026`, `mon=1` 格式，驱动层自动处理 `tm_year=126`, `tm_mon=0` 的偏移。
* **掉电保持逻辑**：初始化时会自动检查 RTC 是否处于 STOP 状态，仅在首次上电或电池耗尽时重置时间，防止复位导致时间丢失。
* **线程安全**：所有读写操作均受 Mutex 保护，防止多任务竞争（如一个任务读时间，另一个任务正在校时）。
* **O(1) 中断分发**：利用 Context 指针直接定位设备实例，无需遍历查找。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列
* **RTOS**：Azure RTOS ThreadX
* **底层驱动**：`r_rtc`

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 Stacks 中添加 `RTC` 驱动。
2.  配置 Name (例如 `g_rtc0`)。
3.  **Callback**: 设置为 `rtc_common_isr`。
4.  **Clock Source**: 确保选择了正确的子时钟（Sub-Clock）或 LOCO，并在 `Clocks` 选项卡中启用了该时钟。

### 步骤 2: bsp_config.h 配置
```c
#define RTCDEV_INSTANCE  &g_rtc0