# BSP GPIO 模块说明文档

## 1. 概述
本模块是对 Renesas FSP `IOPORT` 驱动的超轻量级封装。
所有函数均采用 `static inline` 定义，编译时直接展开，**不占用额外的 Flash 空间，也没有函数调用开销**，实现了最高的执行效率。

### 核心特性
* **零开销 (Zero Overhead)**：性能等同于直接调用底层驱动。
* **通用性**：统一了 Write/Read/Toggle 接口风格。
* **安全性**：包含完整的 FSP 错误码传递。

## 2. 依赖环境
* **硬件平台**：Renesas RA 系列
* **底层驱动**：`r_ioport` (Port)

## 3. 使用步骤

### 步骤 1: RASC 配置
1.  在 **Pins** 标签页中配置引脚。
2.  找到目标引脚 (例如 P106)，设置 Mode 为 `Output mode (Initial Low/High)` 或 `Input mode`。
3.  Generate Project Content。
4.  **注意**：`g_ioport_ctrl` 是 FSP 自动生成的全局变量，无需手动初始化，系统启动时会自动调用 `R_IOPORT_Open`。

### 步骤 2: API 调用示例

#### 4.1 输出控制 (点灯)
```c
/* 定义引脚别名 (可选) */
#define LED_PIN  BSP_IO_PORT_01_PIN_06

/* 拉高 */
BSP_GPIOWrite(LED_PIN, GPIO_HIGH);

/* 拉低 */
BSP_GPIOWrite(LED_PIN, GPIO_LOW);

/* 翻转 */
BSP_GPIOToggle(LED_PIN);