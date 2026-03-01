#ifndef I2C_BASE_HPP
#define I2C_BASE_HPP

#include "hal_data.h"
#include "bsp_i2c.h"
#include "tx_api.h" // 需要用到 tx_thread_sleep

class I2CDriverBase {
protected:
    bsp_i2c_id_e i2c_id_;
    uint8_t device_addr_;

public:
    /* 构造函数：传入 BSP ID 和 设备地址 */
    I2CDriverBase(bsp_i2c_id_e id, uint8_t addr) 
        : i2c_id_(id), device_addr_(addr) {}

    virtual ~I2CDriverBase() {}

    /* 初始化 */
    virtual bool init() {
        BSP_I2C_Init(i2c_id_);
        return true; 
    }

protected:
    /* 封装 BSP 的写操作 */
    fsp_err_t transmit(uint8_t *p_data, uint32_t len) {
        return BSP_I2C_Write(i2c_id_, device_addr_, p_data, len);
    }

    /* 封装 BSP 的读操作 */
    fsp_err_t receive(uint8_t *p_data, uint32_t len) {
        return BSP_I2C_Read(i2c_id_, device_addr_, p_data, len);
    }
};

#endif // I2C_BASE_HPP