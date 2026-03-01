#ifndef BSP_DAC_H
#define BSP_DAC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_data.h"
#include "tx_api.h"

/* 1. 逻辑设备 ID */
typedef enum
{
    BSP_DAC_WAVE = 0,    /* 示例：DA0 输出 */
    /* BSP_DAC_1, */
    
    BSP_DAC_NUM       /* 自动计算总数 */
} bsp_dac_id_e;

/* ============================================================== */
/* 辅助宏：应用层用来计算电压 (编译器会自动优化，不占运行时开销) */
/* ============================================================== */
/* 示例：BSP_DAC_VOLTS_TO_RAW(1.65f, 3.3f, 4096) -> 返回 2048 */
#define BSP_DAC_VOLTS_TO_RAW(volts, vref, resolution) \
    ((uint16_t)((volts) * (resolution) / (vref)))

/* ============================================================== */
/* API 接口                                                       */
/* ============================================================== */

/**
 * @brief 初始化 DAC
 * @note  配置硬件，自动创建互斥锁，并开启 DAC 输出
 */
void BSP_DAC_Init(bsp_dac_id_e id);

/**
 * @brief 写入 DAC 数值 (核心函数)
 * @note  1. 线程安全 (带锁)
 * 2. 极速执行 (无浮点运算)
 * @param value: 写入寄存器的原始值 (12-bit: 0~4095)
 */
fsp_err_t BSP_DAC_Write(bsp_dac_id_e id, uint16_t value);

/**
 * @brief 启动 DAC 输出
 * @note  Init 时已默认 Start，一般不需要手动调用，除非你手动 Stop 过
 */
fsp_err_t BSP_DAC_Start(bsp_dac_id_e id);

/**
 * @brief 停止 DAC 输出
 * @note  停止后引脚状态取决于 FSP 配置 (通常为高阻或保持最后电平)
 */
fsp_err_t BSP_DAC_Stop(bsp_dac_id_e id);

#ifdef __cplusplus
}
#endif

#endif /* BSP_DAC_H */