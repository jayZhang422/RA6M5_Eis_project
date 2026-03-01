#include "ft6336u.hpp"

/* 构造函数初始化父类 */
FT6336U::FT6336U(bsp_i2c_id_e i2c_id, bsp_io_port_pin_t rst_pin, bsp_io_port_pin_t int_pin)
    : TouchDriverBase(i2c_id, 0x38, rst_pin, int_pin, 320, 480) // 默认地址 0x38，尺寸可根据需要传参
{
}

uint8_t FT6336U::readChipID() {
    uint8_t vendor_id = 0;
    uint8_t chip_id = 0;

    readReg(FT_ID_G_FT5201ID, &vendor_id, 1);
    readReg(FT_ID_G_CIPHER, &chip_id, 1);

    BSP_Printf(COM_DEBUG, "[FT6336U] Vendor ID: 0x%02X, Chip ID: 0x%02X\r\n", vendor_id, chip_id);

    if (vendor_id == 0x00 && chip_id == 0x00) return 0;
    return vendor_id;
}

bool FT6336U::init() {
    BSP_Printf(COM_DEBUG, "[FT6336U] Start Init...\r\n");

    /* 1. 初始化 I2C (BSP层面) */
    BSP_I2C_Init(i2c_id_);

    /* 2. 硬件复位 (调用父类通用方法) */
    hwReset();

    /* 3. 验证 ID */
    if (readChipID() == 0) {
        BSP_Printf(COM_DEBUG, "[FT6336U] I2C Error! Check wiring.\r\n");
        return false;
    }

    /* 4. 设定工作模式为 Normal (0x00) */
    uint8_t mode = 0x00;
    writeReg(FT_DEVIDE_MODE, &mode, 1);

    BSP_Printf(COM_DEBUG, "[FT6336U] Init Success.\r\n");
    return true;
}

bool FT6336U::readPoints(TouchPoint *points, uint8_t max_points) {
    if (points == nullptr || max_points == 0) return false;

    uint8_t status;
    uint8_t buf[4]; // 用于存储 XH, XL, YH, YL

    /* 1. 读取状态寄存器 (0x02) 获取触摸点数 */
    if (!readReg(FT_REG_NUM_FINGER, &status, 1)) return false;

    /* TD_STATUS[3:0] 是触摸点数量 */
    uint8_t touch_cnt = status & 0x0F;

    if (touch_cnt == 0 || touch_cnt > 2) {
        return false; 
    }

    /* 2. 读取第一个点的坐标数据 (寄存器 0x03 ~ 0x06) */
    if (!readReg(FT_TP1_REG_XH, buf, 4)) return false;

    /* 3. 解析坐标 (保持原 C 代码的位操作逻辑) */
    uint16_t x_raw = (uint16_t)(((buf[0] & 0x0F) << 8) | buf[1]);
    uint16_t y_raw = (uint16_t)(((buf[2] & 0x0F) << 8) | buf[3]);

    /* 填入结果 */
    points[0].x = x_raw;
    points[0].y = y_raw;
    points[0].id = 0; 
    points[0].pressed = true;

    /* 如果你需要支持多点，可以在这里扩展读取 0x09 之后的寄存器，逻辑同上 */

    return true;
}