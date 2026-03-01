#include "bsp_algorithm.hpp"
#include <sys/errno.h>
#include "bsp_name.hpp"
#include "bsp_adc.h"
#include "bsp_dac.h"
#include "bsp_dmac.h"
#include "bsp_timer.h"
#include "bsp_usart.h"
#include "bsp_user_elc.h"
#include "fsp_common_api.h"
#include "m-profile/armv8m_mpu.h"

#define WAVE_POINTS 1024
#define R_REF_OHM 4700.0f
float sin_voltage[WAVE_POINTS];
float sin_current[WAVE_POINTS];
float stand_sin[WAVE_POINTS];
float stand_cos[WAVE_POINTS];

#define STANDBY_RAM_BASE  0x28000000

/* 手动分配内存布局 */
/* 0x28000000 开始放 DAC 数据 */
#define DAC_BUF_ADDR      (STANDBY_RAM_BASE)
/* 0x28000000 + (1024 * 2字节) = 0x28000800 开始放 ADC 数据 */
#define ADC_BUF_ADDR      (STANDBY_RAM_BASE + (WAVE_POINTS * sizeof(uint16_t)))

// /* 使用指针强转 */
// uint16_t *g_dac_output_buffer = (uint16_t *)DAC_BUF_ADDR;
// uint16_t *g_adc_raw_buffer    = (uint16_t *)ADC_BUF_ADDR;

uint16_t g_dac_output_buffer[WAVE_POINTS];
uint16_t g_adc_raw_buffer [WAVE_POINTS];
ImpedanceResult g_phase1_result ;




extern "C" void Run_phase1_test(void) ;
extern "C" void Run_phase2_test(void) ;
extern "C" void Run_phase25_test(void);


void Run_phase1_test(void)
{
    float target_freq = 1000.0f;   // 1kHz 信号
    float sample_rate = 100000.0f; // 100kHz 采样率
    float battery_res = 0.5f;      // 假设电池内阻 0.5 欧姆


    EisRefGenerator::Generate(sin_current, WAVE_POINTS,target_freq,sample_rate,BSP_ALG_SIN);
    for(int i=0; i<WAVE_POINTS; i++) {
        sin_voltage[i] = battery_res * sin_current[i]; 
        sin_voltage[i] += 0.002f;
    }
    EisRefGenerator::Generate(stand_sin, WAVE_POINTS, target_freq, sample_rate, BSP_ALG_SIN);
    EisRefGenerator::Generate(stand_cos, WAVE_POINTS, target_freq, sample_rate, BSP_ALG_COS);

    // 4. 运行核心算法 (The Core Algorithm)
    // ---------------------------------------------------------
    
    // 解算电压的 实部(Vr) 和 虚部(Vi)
    float Vr = DigitalLockIn::Demodulate(sin_voltage, stand_sin, WAVE_POINTS);
    float Vi = DigitalLockIn::Demodulate(sin_voltage, stand_cos, WAVE_POINTS);

    // 解算电流的 实部(Ir) 和 虚部(Ii)
    float Ir = DigitalLockIn::Demodulate(sin_current, stand_sin, WAVE_POINTS);
    float Ii = DigitalLockIn::Demodulate(sin_current, stand_cos, WAVE_POINTS);

    // 计算最终阻抗 Z
   ImpedanceSolver::Calculate(Vr, Vi, Ir, Ii,&g_phase1_result);

    AppPrint::PrintFloat("阻抗实部",g_phase1_result.R_real,"omega");
    

}

static void DAC_Waveform(void)
{
    // 1. 借用 sin_voltage 数组生成标准正弦波 (-1.0 ~ 1.0)
    EisRefGenerator::Generate(sin_voltage, WAVE_POINTS, 1000.0f, 100000.0f, BSP_ALG_SIN);

    const float offset = 2048.0f;
    const float amplitude = 1241.0f;

    for(int i=0; i<WAVE_POINTS; i++) {
        float val = sin_voltage[i];
        
        // 变换: 抬升 + 缩放
        val = offset + (val * amplitude);

        // 钳位
        if (val > 4095.0f) val = 4095.0f;
        if (val < 0.0f)    val = 0.0f;

        g_dac_output_buffer[i] = (uint16_t)val;
    }
    
    // AppPrint::PrintFloat("DEBUG: Buffer[500]", (float)g_dac_output_buffer[525], "code");
    // 清理借用的数组，准备给后面计算用
    memset(sin_voltage, 0, sizeof(sin_voltage));
}

extern "C" void Run_phase2_test(void)
{
    AppPrint::PrintLog("=== Phase 2 Diagnostic Start ===");

    // 1. 初始化
    BSP_DAC_Init(BSP_DAC_WAVE);
    BSP_ADC_Init(BSP_ADC_0);
    BSP_DMAC_Init(BSP_DMAC_DAC);
    BSP_DMAC_Init(BSP_DMAC_ADC_0);
    BSP_Timer_Init(BSP_TIMER_OVERFLOW);
    BSP_ELC_Init(BSP_ELC);

    // 2. 准备数据
    DAC_Waveform();

    // 3. 配置 DMA
    BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void*)g_dac_output_buffer ,BspAnalog::GetDacReg( 0), WAVE_POINTS);
    
   
    BSP_DMAC_Reconfig(BSP_DMAC_ADC_0, BspAnalog::GetAdcReg<0>(5), (void*)g_adc_raw_buffer, WAVE_POINTS);

    // 4. 启动
    BSP_DMAC_Enable(BSP_DMAC_DAC);    
    BSP_DMAC_Enable(BSP_DMAC_ADC_0);  
    BSP_DAC_Start(BSP_DAC_WAVE);      
    BSP_ADC_ScanStart(BSP_ADC_0);     
    BSP_ELC_Enable(BSP_ELC);
    // 5. 启动 Timer
    BSP_Timer_Start(BSP_TIMER_OVERFLOW); 
    AppPrint::PrintLog("Timer Started. Waiting...");

    // 6. 等待 (200 ticks)
    fsp_err_t err = BSP_DMAC_WaitComplete(BSP_DMAC_ADC_0, 2000);

    if (FSP_SUCCESS == err) {
        // 采集成功，立即停止 Timer 防止数据覆写
        BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
        for(int i = 0 ; i<50 ; i++)
        {

            BSP_Printf(COM_DEBUG, "%d\r\n", g_adc_raw_buffer[i]); 

        }
        AppPrint::PrintLog("SUCCESS: Transfer Done! Calculating...");

        // ------------------------------------------------------
        // Step 5: 数据处理与阻抗解算
        // ------------------------------------------------------
        
        // A. 计算直流偏置 (DC Bias)
        long sum = 0;
        for(int i=0; i<WAVE_POINTS; i++) sum += g_adc_raw_buffer[i];
        float dc_bias = (float)sum / WAVE_POINTS;
        
        AppPrint::PrintFloat("1. DC Bias (Target: ~2048)", dc_bias, "code");

        // B. 去直流 (DC Removal) -> 得到纯交流信号
        // 将 ADC 原始整数 (LSB) 转换为浮点，并减去偏置
        // 结果存入 sin_voltage 数组
        for(int i=0; i<WAVE_POINTS; i++) {
            sin_voltage[i] = (float)g_adc_raw_buffer[i] - dc_bias;
        }

        // C. 生成标准参考波 (Reference Signals)
        // 生成标准正弦和余弦波 (幅度 1.0)，用于解调
        EisRefGenerator::Generate(stand_sin, WAVE_POINTS, 1000.0f, 100000.0f, BSP_ALG_SIN);
        EisRefGenerator::Generate(stand_cos, WAVE_POINTS, 1000.0f, 100000.0f, BSP_ALG_COS);

        // D. 数字锁相解算 (Demodulation)
        // 计算实部 (In-Phase) 和 虚部 (Quadrature)
        float Vr = DigitalLockIn::Demodulate(sin_voltage, stand_sin, WAVE_POINTS);
        float Vi = DigitalLockIn::Demodulate(sin_voltage, stand_cos, WAVE_POINTS);

        // E. 计算最终阻抗 (Impedance Calculation)
        // 关键：硬件回环测试中，V_out 接 V_in，所以我们假设 Current = Voltage
        // 因此 R = V / I = 1.0
        ImpedanceResult res;
        ImpedanceSolver::Calculate(Vr, Vi, Vr, Vi, &res); 

        // ------------------------------------------------------
        // Step 6: 打印最终结果
        // ------------------------------------------------------
        AppPrint::PrintFloat("2. Meas Mag (Target: ~1241)", res.Magnitude, "LSB");
        AppPrint::PrintFloat("3. Meas Res (Target: 1.000)", res.R_real, "Ohm");

        // 简单的自动判定
        if (res.R_real > 0.95f && res.R_real < 1.05f) {
             AppPrint::PrintLog(">>> RESULT: PASS (Perfect Loopback) <<<");
        } else {
             AppPrint::PrintLog(">>> RESULT: WARNING (Check Connections) <<<");
        }

    } else {
        // ------------------------------------------------------
        // Step 7: 错误诊断
        // ------------------------------------------------------
        BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
        AppPrint::PrintLog("FAILURE: Timeout occurred!");
        
        
    }
}

extern "C" void Run_phase25_test(void)
{ 

    AppPrint::PrintLog("=== Phase 2.5: Resistor Divider Test ===");

    // 1. 初始化
    BSP_DAC_Init(BSP_DAC_WAVE);
    BSP_ADC_Init(BSP_ADC_0);
    BSP_DMAC_Init(BSP_DMAC_DAC);
    BSP_DMAC_Init(BSP_DMAC_ADC_0);
    BSP_Timer_Init(BSP_TIMER_OVERFLOW);
    BSP_ELC_Init(BSP_ELC);

    // 2. 准备数据
    DAC_Waveform();

    // 3. 配置 DMA
    BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void*)g_dac_output_buffer ,BspAnalog::GetDacReg( 0), WAVE_POINTS);
    BSP_DMAC_Reconfig(BSP_DMAC_ADC_0, BspAnalog::GetAdcReg<0>(5), (void*)g_adc_raw_buffer, WAVE_POINTS);

    // 4. 启动
    BSP_DMAC_Enable(BSP_DMAC_DAC);    
    BSP_DMAC_Enable(BSP_DMAC_ADC_0);  
    BSP_DAC_Start(BSP_DAC_WAVE);      
    BSP_ADC_ScanStart(BSP_ADC_0);     
    BSP_ELC_Enable(BSP_ELC);
    
    // 5. 启动 Timer
    BSP_Timer_Start(BSP_TIMER_OVERFLOW); 
    AppPrint::PrintLog("Timer Started. Waiting...");

    // 6. 等待 (2000 ticks)
    fsp_err_t err = BSP_DMAC_WaitComplete(BSP_DMAC_ADC_0, 2000);

    if (FSP_SUCCESS == err) {
        // 采集成功，立即停止 Timer 防止数据覆写
        BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
        
        // 打印前 50 个点用于观察波形是否正常生成和采集
        for(int i = 0 ; i<50 ; i++)
        {
            BSP_Printf(COM_DEBUG, "%d\r\n", g_adc_raw_buffer[i]); 
        }
     
        // ======================================================
        // 以下是补充的 Phase 2.5 核心解算部分
        // ======================================================
        AppPrint::PrintLog("Done. Calculating...");

        // A. 计算直流偏置 (DC Bias)
        long sum = 0;
        for(int i = 0; i < WAVE_POINTS; i++) {
            sum += g_adc_raw_buffer[i];
        }
        float dc_bias = (float)sum / WAVE_POINTS;
        AppPrint::PrintFloat("1. ADC DC Bias (Target: ~1024)", dc_bias, "LSB");

        // B. 去直流 (DC Removal) -> 得到纯交流信号，存入 sin_voltage
        for(int i = 0; i < WAVE_POINTS; i++) {
            sin_voltage[i] = (float)g_adc_raw_buffer[i] - dc_bias;
        }

        // C. 生成标准参考波 (两把尺子：正弦和余弦)
        EisRefGenerator::Generate(stand_sin, WAVE_POINTS, 1000.0f, 100000.0f, BSP_ALG_SIN);
        EisRefGenerator::Generate(stand_cos, WAVE_POINTS, 1000.0f, 100000.0f, BSP_ALG_COS);

        // D. 正交解调 (Demodulate) -> 提取分压点 (V_mid) 的实部和虚部
        float V_mid_r = DigitalLockIn::Demodulate(sin_voltage, stand_sin, WAVE_POINTS);
        float V_mid_i = DigitalLockIn::Demodulate(sin_voltage, stand_cos, WAVE_POINTS);

        // E. 设定源头电压 (V_source)
        // 我们没有用额外的 ADC 通道去测 DAC 的输出，直接假定它完美输出了我们在 DAC_Waveform 中设定的交流幅度 (1241)
        float V_src_r = 1241.0f; 
        float V_src_i = 0.0f;

        // F. 矢量计算电流 -> I = (V_source - V_mid) / R_ref
        float I_r = (V_src_r - V_mid_r) / R_REF_OHM;
        float I_i = (V_src_i - V_mid_i) / R_REF_OHM;

        // G. 最终求阻抗 -> Z_dut = V_mid / I
        ImpedanceResult res;
        ImpedanceSolver::Calculate(V_mid_r, V_mid_i, I_r, I_i, &res);

        // H. 打印最终解算结果
        AppPrint::PrintFloat("2. Measured R (Target: ~4700)", res.R_real, "Ohm");
        
        // 判断测试是否通过
        if (res.R_real > 4000.0f && res.R_real < 5500.0f) {
            AppPrint::PrintLog(">>> SUCCESS: Resistor Measured Correctly! <<<");
        } else {
            AppPrint::PrintLog(">>> FAIL: Value off. Check Wiring/Resistor Value. <<<");
        }

    } else {
        // 采集超时处理
        BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
        AppPrint::PrintLog("FAILURE: Timeout occurred!");
    }
}