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
                               float sample_rate_hz,
                               ImpedanceResult* p_out_result) // <--- 【修改：新增外部输出指针】
    {
        static float32_t ref_sin[2048]; 
        static float32_t ref_cos[2048];

        if (data_len > 2048) return; 

        float fs = sample_rate_hz; 
        EisRefGenerator::Generate(ref_sin, data_len, freq_hz, fs, BSP_ALG_SIN);
        EisRefGenerator::Generate(ref_cos, data_len, freq_hz, fs, BSP_ALG_COS);

        float Vr = DigitalLockIn::Demodulate(p_clean_voltage, ref_sin, data_len);
        float Vi = DigitalLockIn::Demodulate(p_clean_voltage, ref_cos, data_len);
        float Ir = DigitalLockIn::Demodulate(p_clean_current, ref_sin, data_len);
        float Ii = DigitalLockIn::Demodulate(p_clean_current, ref_cos, data_len);

        // 【修改：将算出的阻抗存入外部传入的结构体中】
        if (p_out_result != nullptr) {
            ImpedanceSolver::Calculate(Vr, Vi, Ir, Ii, p_out_result);

            AppPrint::PrintFloat(">>> [Math] 当前扫频 (Hz)", freq_hz, "");
            AppPrint::PrintFloat(">>> [Math] 测得阻抗模值 |Z| (Ohm)", p_out_result->Magnitude, "");
            // AppPrint::PrintFloat(">>> [Math] 测得阻抗相角 Ph (Deg)", p_out_result->Phase_deg, "");
        }
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
void ADC_Start_Capture_DMA(uint32_t capture_points) ;
void DAC_SingleSignal_131(void);
void DAC_ADC_Loopback_SingleChannel(void);

#ifdef __cplusplus
}
#endif