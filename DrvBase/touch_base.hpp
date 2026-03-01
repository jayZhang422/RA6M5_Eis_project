#ifndef TOUCH_BASE_HPP
#define TOUCH_BASE_HPP

#include "hal_data.h"
#include "bsp_i2c.h"
#include "bsp_gpio.h"
#include "bsp_usart.h" // 用于打印调试
#include "tx_api.h"
#include <string.h>// for memcpy

/* 通用触摸点结构 */
struct TouchPoint {
    uint16_t x;
    uint16_t y;
    uint8_t  id;
    bool     pressed;
};

class TouchDriverBase {
protected:
    /* 硬件配置参数 */
    bsp_i2c_id_e i2c_id_;
    uint8_t      device_addr_;
    bsp_io_port_pin_t rst_pin_;
    bsp_io_port_pin_t int_pin_;
    
    /* 屏幕属性 */
    uint16_t width_;
    uint16_t height_;

public:
    TouchDriverBase(bsp_i2c_id_e i2c_id, uint8_t addr, 
                    bsp_io_port_pin_t rst, bsp_io_port_pin_t irq,
                    uint16_t w, uint16_t h)
        : i2c_id_(i2c_id), device_addr_(addr), 
          rst_pin_(rst), int_pin_(irq), 
          width_(w), height_(h) {}

    virtual ~TouchDriverBase() {}

    /* 纯虚函数：子类必须实现 */
    virtual bool init() = 0;
    virtual bool readPoints(TouchPoint *points, uint8_t max_points) = 0;

protected:
    /* 通用硬件复位逻辑 (逻辑复刻自原驱动) */
    void hwReset() {
        /* 配置引脚 */
        R_IOPORT_PinCfg(&g_ioport_ctrl, rst_pin_, IOPORT_CFG_PORT_DIRECTION_OUTPUT);
        R_IOPORT_PinCfg(&g_ioport_ctrl, int_pin_, IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_PULLUP_ENABLE);

        BSP_Printf(COM_DEBUG, "[TouchBase] Resetting...\r\n");
        
        /* 复位时序: Low > 5ms, Wait > 300ms */
        BSP_GPIOWrite(rst_pin_, BSP_IO_LEVEL_LOW);
        tx_thread_sleep(20); 

        BSP_GPIOWrite(rst_pin_, BSP_IO_LEVEL_HIGH);
        tx_thread_sleep(300); 
    }

    /* I2C 写寄存器辅助函数 */
   bool writeReg(uint8_t reg, uint8_t *data, uint8_t len) { // 参数是小写 len
    uint8_t temp_buf[32];
    
    // 修正1：把 Len 改为 len
    if (len > 30) return false;

    temp_buf[0] = reg;
    
    // 修正2：把 Len 改为 len
    if (len > 0 && data != nullptr) {
        // 修正3：去掉 std::，直接用 memcpy
        memcpy(&temp_buf[1], data, len); 
    }

    // 修正4：底部的 Len 也要改 (虽然截图没截全，但通常这里也会报错)
    // return (BSP_I2C_Write(..., len + 1) == FSP_SUCCESS);
    return (BSP_I2C_Write(i2c_id_, device_addr_, temp_buf, len + 1) == FSP_SUCCESS);
}

    /* I2C 读寄存器辅助函数 */
    bool readReg(uint8_t reg, uint8_t *buf, uint8_t len) {
        if (BSP_I2C_Write(i2c_id_, device_addr_, &reg, 1) != FSP_SUCCESS) {
            return false;
        }
        if (BSP_I2C_Read(i2c_id_, device_addr_, buf, len) != FSP_SUCCESS) {
            return false;
        }
        return true;
    }
};

#endif // TOUCH_BASE_HPP