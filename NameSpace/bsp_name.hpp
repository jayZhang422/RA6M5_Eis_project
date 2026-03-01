#ifndef BSP_NAME_HPP
#define BSP_NAME_HPP

#include "hal_data.h" // 包含瑞萨底层头文件
#include <cmath>
#include "bsp_usart.h"
#ifdef __cplusplus
/* 使用命名空间防止污染全局空间 */
namespace BspAnalog {

    /**
     * @brief 获取 ADC 数据寄存器地址
     */
    template <int UnitIndex>
    static inline void* GetAdcReg(int channel)
    {
        // 这一行依然在编译期起作用，写错 UnitIndex 直接报错，安全！
        static_assert(UnitIndex == 0 || UnitIndex == 1, "Error: Invalid ADC Unit Index!");

        if (UnitIndex == 0) {
            return (void*)&R_ADC0->ADDR[channel];
        } else {
            return (void*)&R_ADC1->ADDR[channel];
        }
    }

    // 同样改为 inline
    static inline void* GetDacReg(int channel)
    {
        return (void*)&R_DAC->DADR[channel];
    }
}


/*


* @brief 打印数据的名字空间

* 由于增加浮点数打印会导致flash开销增大而封装，同时也增加可读性


*/

    namespace AppPrint 
{
    /* * 通用浮点打印工具 (省 RAM 版)
     * @param label: 前缀标签 (支持中文, e.g., "电池电压")
     * @param val:   浮点数值
     * @param unit:  单位后缀 (e.g., "V", "mA", "Ohm")
     */
    inline void PrintFloat(const char* label, float val, const char* unit)
    {
        // 1. 处理符号位 (解决 -0.5V 显示为 0.5V 的 bug)
        char sign = '+';
        if (val < 0.0f) {
            sign = '-';
            val = -val; // 转为正数处理
        }
        
        // 2. 拆分整数部分
        int i_part = (int)val;

        // 3. 拆分小数部分 (这里保留 3 位小数，即乘 1000)
        // 使用 fabsf 确保计算安全，虽然上面已经转正了
        int d_part = (int)((val - (float)i_part) * 1000.0f);

        // 4. 打印
        // 注意逻辑：如果是负数，我们手动打印 '-' 号
        // %03d 保证小数部分如 .005 不会显示成 .5
        if (sign == '-') {
            BSP_Printf(COM_DEBUG, "%s: -%d.%03d %s\r\n", label, i_part, d_part, unit);
        } else {
            BSP_Printf(COM_DEBUG, "%s: %d.%03d %s\r\n", label, i_part, d_part, unit);
        }
    }
     /*
     
     *用于打印字符数据
     
     */
    inline void PrintLog(const char *str)
    {
    
    BSP_Printf(COM_DEBUG, "%s\r\n", str); 
    }
}

#endif

#endif // BSP_ADC_HPP