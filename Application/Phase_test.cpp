#include "Phase_test.hpp"
#include "bsp_algorithm.hpp"
#include <sys/errno.h>
#include "bsp_name.hpp"
#include "bsp_dac.h"
#include "bsp_dmac.h"
#include "bsp_timer.h"
#include "bsp_user_elc.h"
#include "eis_sweep_manager.hpp"
#include "eis_data_pipeline.hpp"
#include "dsp_preprocess.hpp"
#include "afe_control.hpp"
#include <cstring>
#include <math.h>

// =======================================================
// [1. 全局与共享变量区]
// =======================================================
#define WAVE_POINTS 200

// 发波用的缓冲
static float sin_voltage[WAVE_POINTS];
uint16_t g_dac_output_buffer[WAVE_POINTS];

// 采集用的共享参数 (用于让采集线程知道当前的激励状态)
volatile float g_current_excitation_freq_hz = 1000.0f; // 当前发波频率
volatile float g_current_excitation_current = 0.00f;  

// 数据处理用的缓冲与状态机
static float32_t s_process_buffer[EIS::MAX_DMA_BUFFER_SIZE];
static EIS::AFE_Controller  afe_hardware;
static EIS::AgcStateMachine agc_machine;
static const float PGA_GAIN_FLOAT_TABLE[11] = {
    0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f
};

// =======================================================
// [2. 底层驱动辅助函数]
// =======================================================

// 统一的硬件初始化模块，避免重复写代码
static void HW_Init_Excitation_Path() {
    BSP_DAC_Init(BSP_DAC_WAVE);
    BSP_DMAC_Init(BSP_DMAC_DAC);
    BSP_Timer_Init(BSP_TIMER_OVERFLOW);
    BSP_ELC_Init(BSP_ELC);
}

// DAC 波形生成器 (生成一周期存入 g_dac_output_buffer)
static void DAC_Waveform(float target_freq_hz, float sample_rate_hz) {
    EIS::EisRefGenerator::Generate(sin_voltage, WAVE_POINTS, target_freq_hz, sample_rate_hz, BSP_ALG_SIN);
    EIS::DacWaveConfig config = EIS::DacConfigHelper::Calculate(1650.0f, 1000.0f);

    float offset = config.offset;
    float amplitude = config.amplitude;

    // 钳位保护
    if (!(offset >= 0.0f && offset <= 4095.0f)) offset = 2048.0f;
    if (!(amplitude >= 0.0f && amplitude <= 4095.0f)) amplitude = 0.0f;
    if (offset + amplitude > 4095.0f) amplitude = 4095.0f - offset;

    for(int i = 0; i < WAVE_POINTS; i++) {
        float val = offset + (sin_voltage[i] * amplitude);
        if (val > 4095.0f) val = 4095.0f;
        else if (val < 0.0f) val = 0.0f;
        
        g_dac_output_buffer[i] = (uint16_t)(val + 0.5f);
    }
}

// 计算最优 DMA 周期
static uint32_t Sweep_SelectIntegerCycles(float target_freq_hz, float max_sample_rate_hz) {
    float m = ceilf((target_freq_hz * (float) WAVE_POINTS) / max_sample_rate_hz);
    uint32_t cycles = (uint32_t) m;
    if (cycles < 1U) cycles = 1U;
    
    uint32_t max_cycles = (uint32_t) (WAVE_POINTS / 4U);
    if (cycles > max_cycles) cycles = max_cycles;
    return cycles;
}


// =======================================================
// [3. 硬件发波测试入口 (给 main 调用的)]
// =======================================================

extern "C" void DAC_SingleSignal(void) {
    AppPrint::PrintLog("=== DAC 单频点(1kHz)循环输出 ===");
    
    HW_Init_Excitation_Path();
    DAC_Waveform(1000.0f, 100000.0f);

    BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void const *) g_dac_output_buffer, BspAnalog::GetDacReg(0), 0);
    BSP_DMAC_Enable(BSP_DMAC_DAC);
    BSP_DAC_Start(BSP_DAC_WAVE);
    BSP_ELC_Enable(BSP_ELC);
    BSP_Timer_Start(BSP_TIMER_OVERFLOW);

    while (1) { tx_thread_sleep(1000); }
}

extern "C" void DAC_SweepSignal(void) { 
    AppPrint::PrintLog("=== DAC 10Hz~10kHz 扫频输出 ===");

    HW_Init_Excitation_Path();
    BSP_DAC_Start(BSP_DAC_WAVE);
    BSP_ELC_Enable(BSP_ELC);

    EIS::SweepGenerator sweep;
    sweep.Init(10.0f, 10000.0f, 31); // 10Hz 到 10kHz 扫 31 个点

    float target_freq_hz = 0.0f;
    uint16_t step = 0U;

    while (sweep.GetNext(&target_freq_hz)) {
        step++;
        uint32_t cycles_m = Sweep_SelectIntegerCycles(target_freq_hz, 100000.0f);
        float sample_rate = (target_freq_hz * (float)WAVE_POINTS) / (float)cycles_m;
        uint32_t sample_rate_hz = (uint32_t)(sample_rate + 0.5f);
        float actual_freq_hz = ((float)sample_rate_hz * (float)cycles_m) / (float)WAVE_POINTS;

        float dac_ac_amplitude_v = 1.0f;
        
        float actual_current_peak_a = dac_ac_amplitude_v / 2.5f;
        // 【关键新增】将真实的发波频率同步给采集处理线程！
        g_current_excitation_freq_hz = actual_freq_hz;
        g_current_excitation_current = actual_current_peak_a;

        BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
        BSP_Timer_SetFreq_Hz(BSP_TIMER_OVERFLOW, sample_rate_hz);
        DAC_Waveform(actual_freq_hz, (float)sample_rate_hz);

        BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void const *) g_dac_output_buffer, BspAnalog::GetDacReg(0), 0U);
        BSP_DMAC_Enable(BSP_DMAC_DAC);
        BSP_Timer_Start(BSP_TIMER_OVERFLOW);

        // 动态稳态等待
        uint32_t delay_ms = EIS::SettlingTimeCalculator::CalculateDelayMs(actual_freq_hz, 5.0f, 50U);
        ULONG delay_ticks = (ULONG)((((uint64_t)delay_ms) * TX_TIMER_TICKS_PER_SECOND + 999ULL) / 1000ULL);
        
       
        tx_thread_sleep(delay_ticks);
    }

    BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
    AppPrint::PrintLog("Sweep finished.");
    while (1) { tx_thread_sleep(1000); }
}


// =======================================================
// [RTOS 任务] 前端数据清洗与自动增益控制 (AGC) 线程
// =======================================================
extern "C" void EIS_DataProcess_Thread_Entry(void) {
    EIS::DataPipeline::Init();
    afe_hardware.init(); 
    agc_machine.Init(0, 10, 3, 2);
    
    uint8_t current_gain_idx = 3;
    float current_pga_gain = PGA_GAIN_FLOAT_TABLE[current_gain_idx];
    uint32_t current_dma_len = WAVE_POINTS;

    while (1) {
        const uint16_t* p_raw_data = nullptr;

        // 1. 同步等待硬件 DMA 采完数据
        if (EIS::DataPipeline::WaitForDataBlock(&p_raw_data, TX_WAIT_FOREVER) && p_raw_data) {
            
            // 2. 物理量转换与预处理 (转电压 -> 去直流 -> 算RMS)
            bool is_clipped = EIS::DspPreprocess::ConvertRawToVoltage(
                p_raw_data, s_process_buffer, current_dma_len, 3.3f, current_pga_gain);
            float32_t dc_bias = 0.0f;
            EIS::DspPreprocess::RemoveDcOffset(s_process_buffer, current_dma_len, &dc_bias);
            float32_t ac_rms = EIS::DspPreprocess::CalculateRMS(s_process_buffer, current_dma_len);

            // 3. 评估信号质量并喂给 AGC 状态机
            EIS::SignalQuality_e quality = EIS::DspPreprocess::EvaluateSignalQuality(is_clipped, ac_rms, current_pga_gain);
            uint8_t target_gain_idx = current_gain_idx;
            EIS::AgcState_e agc_state = agc_machine.Process(quality, &target_gain_idx);

            // 4. 根据 AGC 指令执行动作
            if (agc_state == EIS::AGC_STATE_JUST_CHANGED) {
                // 动作 A：驱动硬件切档，并丢弃当前不稳定的数据
                afe_hardware.set_pga_gain((EIS::pga281_gain_t)target_gain_idx);
                current_gain_idx = target_gain_idx;
                current_pga_gain = PGA_GAIN_FLOAT_TABLE[current_gain_idx];
                continue; 
            }
            else if (agc_state == EIS::AGC_STATE_BLANKING) {
                // 动作 B：处于盲区，丢弃当前数据
                continue; 
            }
           EIS::EisMathEngine::SolveImpedance(
                s_process_buffer,              // 洗干净的电压数据
                current_dma_len,               // 数组长度
                g_current_excitation_freq_hz,  // 当前扫频的频率
                g_current_excitation_current   // 当前注入的恒流大小
            );
           
        }
    }
}


