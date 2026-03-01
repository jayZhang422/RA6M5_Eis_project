#ifndef BSP_ADC_H
#define BSP_ADC_H

#include "hal_data.h"




#ifdef __cplusplus
extern "C" {
#endif


#include "tx_api.h"

typedef enum
{
    BSP_ADC_0 = 0,
    BSP_ADC_NUM
} bsp_adc_id_e;

/* API */
void BSP_ADC_Init(bsp_adc_id_e id);

/**
 * @brief 启动 ADC 扫描
 * @note 如果 RASC 中配置了 DMA Trigger 为 ADC Scan End，
 * 调用此函数会自动触发 DMA 搬运。
 */
fsp_err_t BSP_ADC_ScanStart(bsp_adc_id_e id);

/**
 * @brief 等待扫描完成 (阻塞线程)
 */
fsp_err_t BSP_ADC_WaitScanComplete(bsp_adc_id_e id, uint32_t timeout_ticks);

/**
 * @brief 读取单通道值 (适用于未使用 DMA 时)
 */
fsp_err_t BSP_ADC_Read(bsp_adc_id_e id, adc_channel_t channel, uint16_t * const p_data);

/* 回调函数声明 */
void bsp_common_adc(adc_callback_args_t * p_args);



#ifdef __cplusplus
}
#endif




#endif