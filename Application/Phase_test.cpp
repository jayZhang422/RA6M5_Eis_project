#include "bsp_algorithm.hpp"
#include <sys/errno.h>
#include "bsp_name.hpp"
#include "bsp_adc.h"
#include "bsp_dac.h"
#include "bsp_dmac.h"
#include "bsp_timer.h"
#include "bsp_usart.h"
#include "bsp_user_elc.h"
#include "eis_sweep_manager.hpp"

#include <math.h>
#define WAVE_POINTS 200
#define R_REF_OHM 4700.0f
float sin_voltage[WAVE_POINTS];
float sin_current[WAVE_POINTS];
float stand_sin[WAVE_POINTS];
float stand_cos[WAVE_POINTS];
uint16_t g_dac_output_buffer[WAVE_POINTS];
uint16_t g_adc_raw_buffer [WAVE_POINTS];




ImpedanceResult g_phase1_result ;




extern "C" void Run_phase1_test(void) ;
extern "C" void DAC_Sin_normal(void);
extern "C" void Run_phase2_test(void);
extern "C" void Run_phase25_test(void);
extern "C" void DAC_set_hz(void);

void Run_phase1_test(void)
{
    float target_freq = 1000.0f;   // 1kHz �ź�
    float sample_rate = 100000.0f; // 100kHz ����??
    float battery_res = 0.5f;      // ���������� 0.5 ŷķ


    EisRefGenerator::Generate(sin_current, WAVE_POINTS,target_freq,sample_rate,BSP_ALG_SIN);
    for(int i=0; i<WAVE_POINTS; i++) {
        sin_voltage[i] = battery_res * sin_current[i]; 
        sin_voltage[i] += 0.002f;
    }
    EisRefGenerator::Generate(stand_sin, WAVE_POINTS, target_freq, sample_rate, BSP_ALG_SIN);
    EisRefGenerator::Generate(stand_cos, WAVE_POINTS, target_freq, sample_rate, BSP_ALG_COS);

    // 4. ���к����㷨 (The Core Algorithm)
    // ---------------------------------------------------------
    
    // ������??ʵ��(Vr) ??�鲿(Vi)
    float Vr = DigitalLockIn::Demodulate(sin_voltage, stand_sin, WAVE_POINTS);
    float Vi = DigitalLockIn::Demodulate(sin_voltage, stand_cos, WAVE_POINTS);

    // ��������??ʵ��(Ir) ??�鲿(Ii)
    float Ir = DigitalLockIn::Demodulate(sin_current, stand_sin, WAVE_POINTS);
    float Ii = DigitalLockIn::Demodulate(sin_current, stand_cos, WAVE_POINTS);

    // ����������??Z
   ImpedanceSolver::Calculate(Vr, Vi, Ir, Ii,&g_phase1_result);

    AppPrint::PrintFloat("R_real",g_phase1_result.R_real,"omega");
    

}
    
static void DAC_Waveform(float target_freq_hz = 1000.0f, float sample_rate_hz = 100000.0f)
{
    // 1. 产生标准正弦波 (-1.0 ~ 1.0)
    EisRefGenerator::Generate(sin_voltage, WAVE_POINTS, target_freq_hz, sample_rate_hz, BSP_ALG_SIN);

    // ==========================================
    // 2. 人性化修改振幅：直接传入你想要的电压！
    // 示例：中心电压 1650mV (1.65V)，振幅 1000mV (波峰到2.65V，波谷到0.65V)
    // ==========================================
    DacWaveConfig config = DacConfigHelper::Calculate(1650.0f, 1000.0f);

    float offset = config.offset;
    float amplitude = config.amplitude;

    // 防御式兜底：捕获非有限值，确保后续 DMA 数据安全。
    if (!(offset >= 0.0f && offset <= 4095.0f)) {
        offset = 2048.0f;
    }
    if (!(amplitude >= 0.0f && amplitude <= 4095.0f)) {
        amplitude = 0.0f;
    }
    if (offset + amplitude > 4095.0f) {
        amplitude = 4095.0f - offset;
    }

    // 3. 叠加并输出到 DMA 数组
    for(int i=0; i<WAVE_POINTS; i++) {
        float val = sin_voltage[i];
        
        // 变换: 抬升 + 放大
        val = offset + (val * amplitude);

        // 最终钳位：同时处理越界和 NaN，避免异常值进入 DAC 缓冲。
        if (!(val >= 0.0f && val <= 4095.0f)) {
            if (val > 4095.0f) {
                val = 4095.0f;
            } else if (val < 0.0f) {
                val = 0.0f;
            } else {
                val = offset; // NaN 情况回退到安全直流偏置
            }
        }

        // 使用四舍五入，减小截断带来的量化偏差。
        g_dac_output_buffer[i] = (uint16_t)(val + 0.5f);
    }
    
    memset(sin_voltage, 0, sizeof(sin_voltage));
}

static uint32_t Sweep_SelectIntegerCycles(float target_freq_hz, float max_sample_rate_hz)
{
    float m = ceilf((target_freq_hz * (float) WAVE_POINTS) / max_sample_rate_hz);
    uint32_t cycles = (uint32_t) m;
    if (cycles < 1U)
    {
        cycles = 1U;
    }

    // �����Ƶÿ�����ٲ��� 4 ������, ��������ݺ�Ѩ��
    uint32_t max_cycles = (uint32_t) (WAVE_POINTS / 4U);
    if (cycles > max_cycles)
    {
        cycles = max_cycles;
    }
    return cycles;
}

extern "C" void DAC_Sin_normal(void)
{
    fsp_err_t err;

    AppPrint::PrintLog("=== Phase 2 DAC Repeat Start ===");

    BSP_DAC_Init(BSP_DAC_WAVE);
    BSP_DMAC_Init(BSP_DMAC_DAC);
    BSP_Timer_Init(BSP_TIMER_OVERFLOW);
    BSP_ELC_Init(BSP_ELC);

    DAC_Waveform();

    err = BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void const *) g_dac_output_buffer, BspAnalog::GetDacReg(0), 0);
    if (FSP_SUCCESS != err)
    {
        BSP_Printf(COM_DEBUG, "DAC DMAC Reconfig err=%d\r\n", (int) err);
        while (1)
        {
            tx_thread_sleep(100);
        }
    }

    err = BSP_DMAC_Enable(BSP_DMAC_DAC);
    if (FSP_SUCCESS != err)
    {
        BSP_Printf(COM_DEBUG, "DAC DMAC Enable err=%d\r\n", (int) err);
        while (1)
        {
            tx_thread_sleep(100);
        }
    }

    err = BSP_DAC_Start(BSP_DAC_WAVE);
    if (FSP_SUCCESS != err)
    {
        BSP_Printf(COM_DEBUG, "DAC Start err=%d\r\n", (int) err);
        while (1)
        {
            tx_thread_sleep(100);
        }
    }

    BSP_ELC_Enable(BSP_ELC);

    err = BSP_Timer_Start(BSP_TIMER_OVERFLOW);
    if (FSP_SUCCESS != err)
    {
        BSP_Printf(COM_DEBUG, "Timer Start err=%d\r\n", (int) err);
        while (1)
        {
            tx_thread_sleep(100);
        }
    }

    BSP_Printf(COM_DEBUG, "DAC repeat output running.\r\n");

    while (1)
    {
        tx_thread_sleep(1000);
    }
}

extern "C" void Run_phase2_test(void)
{
    // 兼容旧命名入口
    DAC_Sin_normal();
}

extern "C" void Run_phase25_test(void)
{ 
    static constexpr float kSweepStartHz     = 10.0f;
    static constexpr float kSweepEndHz       = 10000.0f;
    static constexpr uint16_t kPtsPerDecade  = 10U;
    static constexpr uint16_t kTotalSteps    = (3U * kPtsPerDecade) + 1U; // 10~10k: 3 decades
    static constexpr float kMaxSampleRateHz  = 100000.0f;                  // GPT6/ADC/DAC shared trigger cap

    AppPrint::PrintLog("=== Phase 2.5: 10Hz~10kHz Sweep Output ===");

    // 1. 初始化激励链路 (DAC->DMAC->ELC->GPT)
    BSP_DAC_Init(BSP_DAC_WAVE);
    BSP_DMAC_Init(BSP_DMAC_DAC);
    BSP_Timer_Init(BSP_TIMER_OVERFLOW);
    BSP_ELC_Init(BSP_ELC);

    fsp_err_t err = BSP_DAC_Start(BSP_DAC_WAVE);
    if (FSP_SUCCESS != err)
    {
        BSP_Printf(COM_DEBUG, "Sweep DAC Start err=%d\r\n", (int) err);
        while (1) { tx_thread_sleep(100); }
    }

    BSP_ELC_Enable(BSP_ELC);

    EIS::SweepGenerator sweep;
    sweep.Init(kSweepStartHz, kSweepEndHz, kTotalSteps);

    float target_freq_hz = 0.0f;
    uint16_t step = 0U;

    while (sweep.GetNext(&target_freq_hz))
    {
        step++;

        // 2. 为当前频点挑选整数周期数 m，并反推采样触发频率 fs
        uint32_t cycles_m = Sweep_SelectIntegerCycles(target_freq_hz, kMaxSampleRateHz);
        float sample_rate = (target_freq_hz * (float) WAVE_POINTS) / (float) cycles_m;
        uint32_t sample_rate_hz = (uint32_t) (sample_rate + 0.5f);
        float actual_freq_hz = ((float) sample_rate_hz * (float) cycles_m) / (float) WAVE_POINTS;

        // 3. 调整定时器触发频率，并重建当前频点 DAC 波表
        BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
        err = BSP_Timer_SetFreq_Hz(BSP_TIMER_OVERFLOW, sample_rate_hz);
        if (FSP_SUCCESS != err)
        {
            BSP_Printf(COM_DEBUG, "Sweep Timer SetFreq err=%d\r\n", (int) err);
            break;
        }

        DAC_Waveform(actual_freq_hz, (float) sample_rate_hz);

        // 4. 重配 DMAC 源缓冲后继续输出
        err = BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void const *) g_dac_output_buffer, BspAnalog::GetDacReg(0), 0U);
        if (FSP_SUCCESS != err)
        {
            BSP_Printf(COM_DEBUG, "Sweep DMAC Reconfig err=%d\r\n", (int) err);
            break;
        }

        err = BSP_DMAC_Enable(BSP_DMAC_DAC);
        if (FSP_SUCCESS != err)
        {
            BSP_Printf(COM_DEBUG, "Sweep DMAC Enable err=%d\r\n", (int) err);
            break;
        }

        err = BSP_Timer_Start(BSP_TIMER_OVERFLOW);
        if (FSP_SUCCESS != err)
        {
            BSP_Printf(COM_DEBUG, "Sweep Timer Start err=%d\r\n", (int) err);
            break;
        }

        // 动态稳态等待: 等 5 个周期，且最少等待 50ms
        uint32_t delay_ms = EIS::SettlingTimeCalculator::CalculateDelayMs(actual_freq_hz, 5.0f, 50U);
        ULONG delay_ticks = (ULONG) ((((uint64_t) delay_ms) * TX_TIMER_TICKS_PER_SECOND + 999ULL) / 1000ULL);
        if (delay_ticks == 0U)
        {
            delay_ticks = 1U;
        }

        BSP_Printf(COM_DEBUG,
                   "SWEEP,%u/%u,Ft=%lu mHz,Fa=%lu mHz,Fs=%lu Hz,m=%lu,Td=%lu ms\r\n",
                   (unsigned) step,
                   (unsigned) kTotalSteps,
                   (unsigned long) (target_freq_hz * 1000.0f + 0.5f),
                   (unsigned long) (actual_freq_hz * 1000.0f + 0.5f),
                   (unsigned long) sample_rate_hz,
                   (unsigned long) cycles_m,
                   (unsigned long) delay_ms);

        tx_thread_sleep(delay_ticks);
    }

    BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
    AppPrint::PrintLog("Sweep finished.");

    while (1)
    {
        tx_thread_sleep(1000);
    }
}

extern "C" void DAC_set_hz(void)
{
    // 兼容旧调用入口，内部复用 Run_phase25_test 扫频实现
    Run_phase25_test();
}
