#include "bsp_adc.h"
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
#include <cstring>
#include <math.h>
#include "eis_algorithm.hpp"
// =======================================================
// [1. 全局与共享变量区]
// =======================================================
#define WAVE_POINTS 200
volatile  float g_dac_center_v = 1.65f  ;
volatile  float g_dac_amplitude_v =1.0f;
volatile bool g_is_signal_stable = false;
// 发波用的缓冲
static float sin_voltage[WAVE_POINTS];
uint16_t g_dac_output_buffer[WAVE_POINTS];

// 采集用的共享参数 (用于让采集线程知道当前的激励状态)
volatile float g_current_excitation_freq_hz = 1000.0f; // 当前发波频率
volatile float g_current_excitation_current = 0.00f;  
volatile float g_current_sample_rate_hz = 100000.0f;

// 数据处理用的缓冲与状态机
static float32_t s_process_buffer_i[EIS::MAX_DMA_BUFFER_SIZE];
static float32_t s_process_buffer_v[EIS::MAX_DMA_BUFFER_SIZE];


// =======================================================
// [2. 底层驱动辅助函数]
// =======================================================

// 统一的硬件初始化模块，避免重复写代码
static void HW_Init_Excitation_Path() {
    BSP_DAC_Init(BSP_DAC_WAVE);
    BSP_ADC_Init(BSP_ADC_0);
    BSP_DMAC_Init(BSP_DMAC_ADC_0);
    BSP_DMAC_Init(BSP_DMAC_DAC);
    BSP_Timer_Init(BSP_TIMER_OVERFLOW);
    BSP_ELC_Init(BSP_ELC);
}

// 动态更新 DAC 输出参数
void UpdateDacWaveParameters(float new_center_v, float new_amplitude_v) {
    // 可以在这里加一些安全限制，防止乱填参数把硬件烧了
    if (new_center_v > 3.1f || new_center_v < 0.0f) return;
    if (new_amplitude_v > 1.5f || new_amplitude_v < 0.0f) return;

    g_dac_center_v = new_center_v;
    g_dac_amplitude_v = new_amplitude_v;
}

// DAC 波形生成器 (生成一周期存入 g_dac_output_buffer)
static void DAC_Waveform(float target_freq_hz, float sample_rate_hz) {
    EIS::EisRefGenerator::Generate(sin_voltage, WAVE_POINTS, target_freq_hz, sample_rate_hz, BSP_ALG_SIN);
    EIS::DacWaveConfig config = EIS::DacConfigHelper::Calculate(g_dac_center_v*1000, g_dac_amplitude_v*1000);

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
  
    HW_Init_Excitation_Path();
    DAC_Waveform(1000.0f, 100000.0f);
    BSP_Timer_SetFreq_Hz(BSP_TIMER_OVERFLOW, 100000.0f);

    
    BSP_DAC_Start(BSP_DAC_WAVE);
    BSP_ELC_Enable(BSP_ELC);
    BSP_Timer_Start(BSP_TIMER_OVERFLOW);

    while (1) {
        
    BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void const *) g_dac_output_buffer, BspAnalog::GetDacReg(0), 0);
    BSP_DMAC_Enable(BSP_DMAC_DAC);
    BSP_DMAC_WaitComplete(BSP_DMAC_DAC, TX_WAIT_FOREVER);    
        
         }
}

extern "C" void DAC_SweepSignal(void) { 
    HW_Init_Excitation_Path();
    BSP_DAC_Start(BSP_DAC_WAVE);
    BSP_ELC_Enable(BSP_ELC);

    EIS::SweepGenerator sweep;
    sweep.Init(10.0f, 10000.0f, 31); 

    float target_freq_hz = 0.0f;
    uint16_t step = 0U;

    while (sweep.GetNext(&target_freq_hz)) {
        step++;
        uint32_t cycles_m = Sweep_SelectIntegerCycles(target_freq_hz, 100000.0f);
        float sample_rate = (target_freq_hz * (float)WAVE_POINTS) / (float)cycles_m;
        uint32_t sample_rate_hz = (uint32_t)(sample_rate + 0.5f);
        float actual_freq_hz = ((float)sample_rate_hz * (float)cycles_m) / (float)WAVE_POINTS;
        
        // 【共享数据】给 RX 线程准备好基准频率和采样率
        g_current_excitation_freq_hz = actual_freq_hz;
        g_current_sample_rate_hz = (float)sample_rate_hz; 

        // ==========================================
        // 1. 【防越界核心修复】彻底关闭并销毁上一个频点的 DMA 状态
        // ==========================================
        BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
        g_dma_dac.p_api->disable(g_dma_dac.p_ctrl);
        g_dma_dac.p_api->close(g_dma_dac.p_ctrl);
        
        // 2. 重新配置定时器和新的正弦波形
        BSP_Timer_SetFreq_Hz(BSP_TIMER_OVERFLOW, sample_rate_hz);
        DAC_Waveform(actual_freq_hz, (float)sample_rate_hz);

        // ==========================================
        // 3. 【防 0V 核心修复】按出厂状态重启 DMA，并重新绑定数组地址
        // ==========================================
        g_dma_dac.p_api->open(g_dma_dac.p_ctrl, g_dma_dac.p_cfg);
        BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void const *) g_dac_output_buffer, BspAnalog::GetDacReg(0), 0U);
        
        // 4. 启动发波
        g_dma_dac.p_api->enable(g_dma_dac.p_ctrl);
        BSP_Timer_Start(BSP_TIMER_OVERFLOW);

        // 5. 动态稳态等待 (硬件 RC 充放电延迟，保证进入 ADC 前没有毛刺)
        uint32_t delay_ms = EIS::SettlingTimeCalculator::CalculateDelayMs(actual_freq_hz, 5.0f, 50U);
        ULONG delay_ticks = (ULONG)((((uint64_t)delay_ms) * TX_TIMER_TICKS_PER_SECOND + 999ULL) / 1000ULL);
        tx_thread_sleep(delay_ticks);

        // ==========================================
        // 6. 握手阶段：此时波形已经极度平滑，通知 RX 开始干活！
        // ==========================================
        g_is_signal_stable = true;

        // 7. 阻塞等待 RX 线程把 DMA 的数据解算完毕，并把标志位拉低
        while(g_is_signal_stable) {
            tx_thread_sleep(1); 
        }
    }

    // 扫频彻底结束，打扫战场
    BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
    g_dma_dac.p_api->disable(g_dma_dac.p_ctrl);
    g_dma_dac.p_api->close(g_dma_dac.p_ctrl);
    while (1) { tx_thread_sleep(1000); }
}

// 替换 eis_algorithm.cpp 中的 EIS_DataProcess_Thread_Entry
extern "C" void EIS_DataProcess_Thread_Entry(void) {
    EIS::DataPipeline::Init();
    

    // 定义固定的硬件参数
    const float FIXED_VOLTAGE_GAIN = 1.0f;    // 电池电压通道的固定硬件运放倍数 (无放大则为1.0)
    const float FIXED_SHUNT_GAIN   = 1.0f;    // 电流采样电阻通道的固定硬件运放倍数
    const float SHUNT_RESISTOR_OHM = 0.01f;   // 10 毫欧采样电阻

    // 假设每次 DMA 搬运的数据是一周期，长度为 WAVE_POINTS
    uint32_t current_dma_len = WAVE_POINTS; 

    while (1) {
        const uint16_t* p_raw_data = nullptr;

        // 1. 阻塞等待 DMA 采集完一波数据
        if (EIS::DataPipeline::WaitForDataBlock(&p_raw_data, TX_WAIT_FOREVER) && p_raw_data) {
            
            // 2. 只有当发波线程宣告“信号已经稳定”，我们才去处理和解算
            if (g_is_signal_stable) {
                
                // --- A. 数据分离 (前半段电压，后半段电流) ---
                const uint16_t* p_raw_v = p_raw_data; 
                const uint16_t* p_raw_i = p_raw_data + current_dma_len; 
                
                // --- B. 电压通道处理 ---
                // ADC 值转物理电压，使用固定增益
                EIS::DspPreprocess::ConvertRawToVoltage(p_raw_v, s_process_buffer_v, current_dma_len, 3.3f, FIXED_VOLTAGE_GAIN);
                // 去除直流偏置，变成纯交流电压纹波
                float32_t dc_bias_v = 0.0f;
                EIS::DspPreprocess::RemoveDcOffset(s_process_buffer_v, current_dma_len, &dc_bias_v);

                // --- C. 电流通道处理 ---
                // ADC 值转采样电阻上的物理电压，使用固定增益
                EIS::DspPreprocess::ConvertRawToVoltage(p_raw_i, s_process_buffer_i, current_dma_len, 3.3f, FIXED_SHUNT_GAIN);
                // 去除直流偏置
                float32_t dc_bias_i = 0.0f;
                EIS::DspPreprocess::RemoveDcOffset(s_process_buffer_i, current_dma_len, &dc_bias_i);
                
                // 【核心转换】：把采样电阻的电压，除以 10 毫欧，变成真实的交流电流 (安培)
                for (uint32_t k = 0; k < current_dma_len; k++) {
                    s_process_buffer_i[k] = s_process_buffer_i[k] / SHUNT_RESISTOR_OHM;
                }

                // --- D. 调用锁相放大器解算阻抗 ---
                EIS::EisMathEngine::SolveImpedance(
                    s_process_buffer_v,            // 洗干净的纯交流电压 (V)
                    s_process_buffer_i,            // 洗干净的纯交流电流 (A)
                    current_dma_len,               
                    g_current_excitation_freq_hz,  // TX 线程告诉我们的当前频率
                    g_current_sample_rate_hz       // TX 线程告诉我们的当前采样率
                );

                // --- E. 握手放行 ---
                // 解算完毕，把标志位拉低，TX 线程就会解除阻塞，去发下一个频率
                g_is_signal_stable = false;
            }
        }
    }
}

extern "C" void DAC_SweepSignal_OscilloscopeTest(void) { 
    HW_Init_Excitation_Path();
    BSP_DAC_Start(BSP_DAC_WAVE);
    BSP_ELC_Enable(BSP_ELC);

    EIS::SweepGenerator sweep;
    sweep.Init(10.0f, 10000.0f, 31); 

    float target_freq_hz = 0.0f;
    uint16_t step = 0U;

    while (sweep.GetNext(&target_freq_hz)) {
        step++;
        uint32_t cycles_m = Sweep_SelectIntegerCycles(target_freq_hz, 100000.0f);
        float sample_rate = (target_freq_hz * (float)WAVE_POINTS) / (float)cycles_m;
        uint32_t sample_rate_hz = (uint32_t)(sample_rate + 0.5f);
        float actual_freq_hz = ((float)sample_rate_hz * (float)cycles_m) / (float)WAVE_POINTS;
        
        // 1. 彻底销毁上一个 DMA 状态，清空被腰斩的内部计数器 (解决 3/4 波形问题)
        BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
        g_dma_dac.p_api->disable(g_dma_dac.p_ctrl);
        g_dma_dac.p_api->close(g_dma_dac.p_ctrl);
        
        // 2. 重新计算并生成这 200 个点的完美正弦波
        BSP_Timer_SetFreq_Hz(BSP_TIMER_OVERFLOW, sample_rate_hz);
        DAC_Waveform(actual_freq_hz, (float)sample_rate_hz);

        // 3. 按照出厂状态重新打开 DMA
        g_dma_dac.p_api->open(g_dma_dac.p_ctrl, g_dma_dac.p_cfg);
        
        // ==========================================
        // 【最致命的修复】：刚刚就是漏了这一行！
        // 必须重新把数组地址绑给 DMA，否则它会去读 NULL (0V)
        // 传入 0U 代表让硬件自动无限循环 (131秒续航)
        // ==========================================
        BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void const *) g_dac_output_buffer, BspAnalog::GetDacReg(0), 0U);
        
        // 4. 发射！全自动无缝发波开始
        g_dma_dac.p_api->enable(g_dma_dac.p_ctrl);
        BSP_Timer_Start(BSP_TIMER_OVERFLOW);

        AppPrint::PrintFloat(">>> Scope Test: Playing Freq (Hz)", actual_freq_hz, "");

        // 5. 让 CPU 睡 2 秒。此时 DMA 正在疯狂自动搬运，波形绝对完美无缝隙
        ULONG delay_ticks = (ULONG)((2000ULL * TX_TIMER_TICKS_PER_SECOND) / 1000ULL);
        tx_thread_sleep(delay_ticks);
    }

    BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
    g_dma_dac.p_api->disable(g_dma_dac.p_ctrl);
    g_dma_dac.p_api->close(g_dma_dac.p_ctrl);
    AppPrint::PrintLog("Scope Sweep finished.");
    while (1) { tx_thread_sleep(1000); }
}