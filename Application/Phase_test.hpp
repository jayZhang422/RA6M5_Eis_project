#pragma once 
#include "bsp_algorithm.hpp"
#include "bsp_name.hpp"







namespace EIS {

// =======================================================
// [业务模块] 独立的数字锁相与阻抗解算引擎 (Math Engine)
// =======================================================
class EisMathEngine {
public:
    // 算法执行入口 (注意：WAVE_POINTS 最好作为参数传进来，以实现彻底解耦)
    static void SolveImpedance(const float32_t* p_clean_voltage, uint32_t data_len, 
                               float freq_hz, float current_amps) 
    {
        // 1. 动态兼容不同长度 (利用堆栈外内存，防止大数组爆栈)
        // 注意：如果你系统的 data_len 可能很大，建议像之前一样用 static，
        // 如果 data_len 最大就是 200，可以直接用 static 或者在外部申请好传进来。
        static float32_t ref_sin[2048]; // 假设系统最大支持 2048 点
        static float32_t ref_cos[2048];
        static float32_t sim_current_arr[2048]; 

        if (data_len > 2048) return; // 越界保护

        float fs = freq_hz * (float)data_len; 

        // 2. 生成本地参考尺子
        EisRefGenerator::Generate(ref_sin, data_len, freq_hz, fs, BSP_ALG_SIN);
        EisRefGenerator::Generate(ref_cos, data_len, freq_hz, fs, BSP_ALG_COS);

        // 3. 构建理论激励电流波形
        EisRefGenerator::Generate(sim_current_arr, data_len, freq_hz, fs, BSP_ALG_SIN);
        float I_amplitude = current_amps * 1.41421356f; 
        for(uint32_t i = 0; i < data_len; i++) {
            sim_current_arr[i] *= I_amplitude;
        }

        // 4. 锁相放大器解调 
        float Vr = DigitalLockIn::Demodulate(p_clean_voltage, ref_sin, data_len);
        float Vi = DigitalLockIn::Demodulate(p_clean_voltage, ref_cos, data_len);
        float Ir = DigitalLockIn::Demodulate(sim_current_arr, ref_sin, data_len);
        float Ii = DigitalLockIn::Demodulate(sim_current_arr, ref_cos, data_len);

        // 5. 复数除法
        ImpedanceResult z_result;
        ImpedanceSolver::Calculate(Vr, Vi, Ir, Ii, &z_result);

        // 6. 输出结果
        AppPrint::PrintFloat(">>> [Math] 测得阻抗模值 |Z| (Ohm)", z_result.Magnitude, "");
        AppPrint::PrintFloat(">>> [Math] 测得阻抗相角 Ph (Deg)", z_result.Phase_deg, "");
    }
};

} // namespace EIS



#ifdef __cplusplus
extern "C" {
#endif

void DAC_SingleSignal(void);  // 发送单频点
void DAC_SweepSignal(void);   // 发送扫频序列
void EIS_DataProcess_Thread_Entry(void); // RTOS 任务入口

#ifdef __cplusplus
}
#endif