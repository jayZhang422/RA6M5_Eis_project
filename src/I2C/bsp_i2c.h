#ifndef BSP_I2C_H
#define BSP_I2C_H


#ifdef __cplusplus
extern "C" {
#endif

#include "hal_data.h"
#include "tx_api.h"

/* ============================================================== */
/* 枚举定义                                                      */
/* ============================================================== */

/* 1. 逻辑设备 ID (应用层只认这个) */
/* * 这里的顺序必须和 .c 文件里的 g_i2c_hw_map 表严格对应 
 * 增加新设备时，请在此处添加 ID，并在 .c 中添加对应的硬件配置
 */
typedef enum
{
    BSP_I2C_EEPROM = 0, /* 例如：板载 AT24C02 */
    
    BSP_I2C_TOUCH,      /* 例如：触摸屏 GT911 */
    /* 在这里添加更多设备... */

    BSP_I2C_NUM         /* 自动计算设备总数 */
} bsp_i2c_id_e;

/* 2. 用户回调函数类型定义 */
typedef void (*bsp_i2c_user_cb_t)(i2c_master_event_t event);

/* ============================================================== */
/* API 接口                                                      */
/* ============================================================== */

/**
 * @brief  初始化 I2C 设备
 * @note   会自动创建互斥锁和信号量，并调用 FSP Open
 * @param  id: 逻辑设备 ID
 */
void BSP_I2C_Init(bsp_i2c_id_e id);

/**
 * @brief  注册用户回调 (可选)
 * @note   一般不需要，除非需要处理特定的错误事件
 * @param  id: 逻辑设备 ID
 * @param  cb: 回调函数指针
 */
void BSP_I2C_RegisterCallback(bsp_i2c_id_e id, bsp_i2c_user_cb_t cb);

/**
 * @brief  写数据 (阻塞 + 线程安全 + 自动切地址)
 * @param  id: 逻辑设备 ID
 * @param  slave_addr: 7位从机地址
 * @param  p_data: 数据指针
 * @param  len: 数据长度
 * @return fsp_err_t: FSP_SUCCESS 或 错误码
 */
fsp_err_t BSP_I2C_Write(bsp_i2c_id_e id, uint8_t slave_addr, uint8_t *p_data, uint32_t len);

/**
 * @brief  读数据 (阻塞 + 线程安全 + 自动切地址)
 * @param  id: 逻辑设备 ID
 * @param  slave_addr: 7位从机地址
 * @param  p_buf: 接收缓冲区
 * @param  len: 读取长度
 * @return fsp_err_t: FSP_SUCCESS 或 错误码
 */
fsp_err_t BSP_I2C_Read(bsp_i2c_id_e id, uint8_t slave_addr, uint8_t *p_buf, uint32_t len);

/**
 * @brief  通用中断入口
 * @note   需要在 RASC 配置中将 Callback 设置为此函数名
 * @param  p_args: FSP 回调参数
 */
void bsp_i2c_common_isr(i2c_master_callback_args_t * p_args);


#ifdef __cplusplus
}
#endif

#endif /* BSP_I2C_H */