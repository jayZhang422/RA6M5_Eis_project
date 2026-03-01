# BSP ADC 驱动模块说明 (BSP_ADC)

## 1. 模块概述
本模块是基于 Renesas FSP `r_adc` 驱动的轻量级封装，专为 **ThreadX RTOS** 环境设计。

**核心特性：**
* **RTOS 深度集成**：使用 `tx_semaphore` 实现零 CPU 占用等待（Blocking Wait），拒绝死循环空转。
* **配置分离**：硬件参数（分辨率、通道、对齐方式）由 RASC 图形化配置，本驱动仅负责运行时的控制（启动、等待、读取）。
* **DMA 友好**：既支持 CPU 轮询读取，也完美支持配合 `bsp_dmac` 进行自动搬运。

---

## 2. 依赖与 FSP 配置 (RASC)

在使用本模块前，**必须**在 FSP Configuration (RASC) 中正确配置 ADC 堆栈。

### 2.1 添加 Stack
* **Path**: `New Stack` -> `Analog` -> `ADC (r_adc)`
* **Name**: 建议保持默认或自定义，例如 `g_adc0`。

### 2.2 关键属性设置 (Properties)

| 属性项 (Property) | 设置建议 (Recommendation) | 说明 |
| :--- | :--- | :--- |
| **Unit** | 0 / 1 ... | 根据原理图选择 ADC 单元 |
| **Resolution** | 12-Bit | 根据需求选择精度 |
| **Alignment** | Right | 通常选择右对齐以便直接读取 `uint16_t` |
| **Mode** | Single Scan / Continuous | 单次扫描或连续扫描 |
| **Interrupt Priority** | **Enabled (e.g., Priority 12)** | **必须开启！** 否则信号量无法释放，线程会死锁 |
| **Callback** | **`bsp_common_adc`** | **必须完全匹配**，驱动依赖此回调释放信号量 |

> **⚠️ 注意**：如果您计划使用 **DMA 搬运**，请参阅本文档第 5 节的特殊配置说明。

---

## 3. 软件架构

### 3.1 文件结构
* `bsp_adc.h`: 对外接口声明，枚举定义。
* `bsp_adc.c`: 驱动实现，中断回调处理，硬件实例映射表。

### 3.2 硬件映射 (Mapping)
在 `bsp_adc.c` 中，通过静态数组 `g_adc_hw_map` 将逻辑 ID 映射到 FSP 生成的硬件实例：

```c
static const bsp_adc_static_cfg_t g_adc_hw_map[BSP_ADC_NUM] = 
{
    /* 逻辑ID: BSP_ADC_0  <--> 硬件实例: &g_adc0 */
    [BSP_ADC_0] = { .p_hal_instance = &g_adc0 },
};