#ifndef FT6336U_HPP
#define FT6336U_HPP

#include "touch_base.hpp"

/* 寄存器定义 (保持原逻辑) */
#define FT_DEVIDE_MODE       0x00
#define FT_REG_NUM_FINGER    0x02
#define FT_TP1_REG_XH        0x03
#define FT_ID_G_CIPHER       0xA3
#define FT_ID_G_FT5201ID     0xA8

class FT6336U : public TouchDriverBase {
public:
    /* 构造函数：传入具体的硬件参数 */
    FT6336U(bsp_i2c_id_e i2c_id, 
            bsp_io_port_pin_t rst_pin, 
            bsp_io_port_pin_t int_pin);

    /* 实现父类接口 */
    virtual bool init() override;
    virtual bool readPoints(TouchPoint *points, uint8_t max_points) override;

private:
    /* 私有辅助函数 */
    uint8_t readChipID();
};

#endif // FT6336U_HPP