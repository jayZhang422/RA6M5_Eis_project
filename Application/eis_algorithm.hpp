#pragma once 
#include "bsp_algorithm.hpp"
#include "bsp_name.hpp"







namespace EIS {

// =======================================================
// [业务模块] 独立的数字锁相与阻抗解算引擎 (Math Engine)
// =======================================================
class EisMathEngine {
public:
    static void SolveImpedance(const float32_t* p_clean_voltage, 
                               const float32_t* p_clean_current, 
                               uint32_t data_len, 
                               float freq_hz, 
                               float sample_rate_hz) 
    {
        static float32_t ref_sin[2048]; 
        static float32_t ref_cos[2048];

        if (data_len > 2048) return; // 越界保护

        // 【修复2】直接使用外部传入的真实采样率，拒绝内部反推！
        float fs = sample_rate_hz; 

        // 生成本地参考尺子
        EisRefGenerator::Generate(ref_sin, data_len, freq_hz, fs, BSP_ALG_SIN);
        EisRefGenerator::Generate(ref_cos, data_len, freq_hz, fs, BSP_ALG_COS);

        // 【修复3】彻底删除 sim_current_arr 的生成逻辑
        // 直接使用 p_clean_voltage 和 p_clean_current 进行正交解调
        float Vr = DigitalLockIn::Demodulate(p_clean_voltage, ref_sin, data_len);
        float Vi = DigitalLockIn::Demodulate(p_clean_voltage, ref_cos, data_len);
        float Ir = DigitalLockIn::Demodulate(p_clean_current, ref_sin, data_len);
        float Ii = DigitalLockIn::Demodulate(p_clean_current, ref_cos, data_len);

        // 复数除法 Z = V / I
        ImpedanceResult z_result;
        ImpedanceSolver::Calculate(Vr, Vi, Ir, Ii, &z_result);

        // 输出结果
        AppPrint::PrintFloat(">>> [Math] 当前扫频 (Hz)", freq_hz, "");
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
void UpdateDacWaveParameters(float new_center_v, float new_amplitude_v);
void DAC_SweepSignal_OscilloscopeTest(void);
#ifdef __cplusplus
}
#endif