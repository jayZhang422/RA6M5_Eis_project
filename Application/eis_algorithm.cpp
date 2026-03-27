    #include "bsp_adc.h"
    #include "bsp_algorithm.hpp"
    #include <cstdint>
    #include <sys/errno.h>
    #include "bsp_name.hpp"
    #include "bsp_dac.h"
    #include "bsp_dmac.h"
    #include "bsp_timer.h"
    #include "bsp_user_elc.h"
    #include "eis_sweep_manager.hpp"
    #include "eis_data_pipeline.hpp"
    #include "dsp_preprocess.hpp"
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




    EIS::ImpedanceResult result ;

    // 数据处理用的缓冲与状态机
    static float32_t s_process_buffer_v[EIS::MAX_DMA_BUFFER_SIZE];


    // =======================================================
    // [2. 底层驱动辅助函数]
    // =======================================================
    // 在 eis_algorithm.cpp 中添加
    static void AdcDma_Callback(transfer_callback_args_t * p_args) {
        (void)p_args;
        // 关键：通知流水线 DMA 搬运已完成，切换 Ping-Pong 状态并唤醒处理线程
        EIS::DataPipeline::NotifyDmaCompleteFromISR(); 
    }
    // 统一的硬件初始化模块，避免重复写代码
    static void HW_Init_Excitation_Path() {
        BSP_DAC_Init(BSP_DAC_WAVE);
        BSP_ADC_Init(BSP_ADC_0);
        BSP_DMAC_Init(BSP_DMAC_ADC_0);
        BSP_DMAC_Init(BSP_DMAC_DAC);
        BSP_DMAC_RegisterCallback(BSP_DMAC_ADC_0, AdcDma_Callback);
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
    extern "C" void ADC_Start_Capture_DMA(uint32_t capture_points) {
        // 1. 停掉接收端 DMA
        BSP_DMAC_Disable(BSP_DMAC_ADC_0); 
        
        // 2. 重新配置 ADC DMA
        // 【核心修改】：源地址固定为 CH5 (ADDR5)
        BSP_DMAC_Reconfig(BSP_DMAC_ADC_0, 
                        BspAnalog::GetAdcReg<0>(5), // <--- 改为 5
                        (void *)EIS::DataPipeline::GetNextWriteBuffer(), 
                        capture_points);
                        
        // 3. 使能 DMA，进入待命状态
        BSP_DMAC_Enable(BSP_DMAC_ADC_0);

        // 4. 武装 ADC
        BSP_ADC_ScanStart(BSP_ADC_0); 
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

    extern "C" void DAC_SingleSignal_131(void) {
        HW_Init_Excitation_Path();
        
        // 设置我们测试内阻的频率参数 (比如 1kHz)
        g_current_excitation_freq_hz = 1000.0f; 
        g_current_sample_rate_hz = 100000.0f;

        // 配置 DAC 波形和定时器
        DAC_Waveform(g_current_excitation_freq_hz, g_current_sample_rate_hz);
        BSP_Timer_SetFreq_Hz(BSP_TIMER_OVERFLOW, g_current_sample_rate_hz);

        // ==========================================
        // 关键修复 1：同时启动 DAC 和 ADC 的 DMA
        // ==========================================
        BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void const *) g_dac_output_buffer, BspAnalog::GetDacReg(0), 0); // 0 代表无限循环发波
        BSP_DMAC_Enable(BSP_DMAC_DAC);
        
        ADC_Start_Capture_DMA(WAVE_POINTS); // 启动 ADC 采集！

        // 启动外设联动
        BSP_DAC_Start(BSP_DAC_WAVE);
        BSP_ELC_Enable(BSP_ELC);
        BSP_Timer_Start(BSP_TIMER_OVERFLOW);

        // ==========================================
        // 关键修复 2：通知处理线程开始算阻抗，并取消 while(1)
        // ==========================================
        g_is_signal_stable = true; 
        
        // 退出函数，让屏幕线程继续刷新！底层 DMA 会自动搬数据！
    }

    extern "C" void EIS_DataProcess_Thread_Entry(void) {
        // 1. 初始化数据流水线事件标志
        EIS::DataPipeline::Init();

        // 定义固定的硬件参数
        const float FIXED_VOLTAGE_GAIN = 1.0f;    // 电池电压通道 (CH5) 运放倍数 

        // 假设每次单通道采集的数据长度为 WAVE_POINTS (200)
        uint32_t single_channel_len = WAVE_POINTS; 

        while (1) {
            const uint16_t* p_raw_data = nullptr;

            // 1. 阻塞等待 DMA 采集完一波数据 
            // 【注意】：因为是 Normal 模式单通道，此时 p_raw_data 里就是纯粹的 200 个 CH5 原始值
            if (EIS::DataPipeline::WaitForDataBlock(&p_raw_data, TX_WAIT_FOREVER) && p_raw_data) {
                
                if (g_is_signal_stable) {
                    
                    // --- A. 直接处理电池电压通道 (CH5) ---
                    // 不再需要 for 循环解交织！直接把 p_raw_data 喂给转换器
                    EIS::DspPreprocess::ConvertRawToVoltage(p_raw_data, s_process_buffer_v, single_channel_len, 3.3f, FIXED_VOLTAGE_GAIN);
                    
                    // 去除直流偏置，提取纯交流电压纹波
                    float32_t dc_bias_v = 0.0f;
                    EIS::DspPreprocess::RemoveDcOffset(s_process_buffer_v, single_channel_len, &dc_bias_v);

                    // --- B. 生成参考波并对电压进行单路锁相解调 ---
                    static float32_t ref_sin[WAVE_POINTS];
                    static float32_t ref_cos[WAVE_POINTS];
                    EIS::EisRefGenerator::Generate(ref_sin, single_channel_len, g_current_excitation_freq_hz, g_current_sample_rate_hz, BSP_ALG_SIN);
                    EIS::EisRefGenerator::Generate(ref_cos, single_channel_len, g_current_excitation_freq_hz, g_current_sample_rate_hz, BSP_ALG_COS);

                    float Vr = EIS::DigitalLockIn::Demodulate(s_process_buffer_v, ref_sin, single_channel_len);
                    float Vi = EIS::DigitalLockIn::Demodulate(s_process_buffer_v, ref_cos, single_channel_len);

                    // --- C. 计算电压纹波的有效幅值 |V| ---
                    float V_mag = 0.0f;
                    arm_sqrt_f32(Vr * Vr + Vi * Vi, &V_mag);

                    // --- D. 基于理论推算内阻 ---
                    // 你的硬件公式：电流(A) = DAC发波振幅(V) / 2.5
                    float I_theoretical_peak = g_dac_amplitude_v / 2.5f; 
                    
                    // 内阻 Rs = 电压纹波幅值 / 理论电流峰值
                    float Rs = V_mag / I_theoretical_peak;

                    // --- E. 将结果塞给全局变量供 UI 提取 ---
                    // 因为只算出了实部 Rs，我们把虚部和相位设为 0
                    result.R_real = Rs;
                    result.R_imag = 0.0f;
                    result.Magnitude = Rs;
                    result.Phase_deg = 0.0f;

                    // 打印结果看看对不对
                    AppPrint::PrintFloat(">>> 理论电流峰值 (A)", I_theoretical_peak, "");
                    AppPrint::PrintFloat(">>> 测得电压纹波 (V)", V_mag, "");
                    AppPrint::PrintFloat(">>> 估算电池内阻 Rs (Ohm)", Rs, "");

                    // --- F. 握手放行 ---
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
extern "C" void DAC_ADC_Loopback_SingleChannel(void) {
    HW_Init_Excitation_Path();
    EIS::DataPipeline::Init();

    DAC_Waveform(1000.0f, 100000.0f);
    BSP_Timer_SetFreq_Hz(BSP_TIMER_OVERFLOW, 100000.0f);

    // 1. 先配 DMA
    BSP_DMAC_Disable(BSP_DMAC_ADC_0); 
    BSP_DMAC_Reconfig(BSP_DMAC_ADC_0, 
                      BspAnalog::GetAdcReg<0>(5), // 锁定 CH5
                      (void *)EIS::DataPipeline::GetNextWriteBuffer(), 
                      WAVE_POINTS);
    BSP_DMAC_Enable(BSP_DMAC_ADC_0);

    // 2. 开启 DAC
    BSP_DAC_Start(BSP_DAC_WAVE);
    BSP_DMAC_Reconfig(BSP_DMAC_DAC, (void const *)g_dac_output_buffer, BspAnalog::GetDacReg(0), 0U); 
    BSP_DMAC_Enable(BSP_DMAC_DAC);

    // 3. 【关键】先武装 ADC，再开定时器
    BSP_ADC_ScanStart(BSP_ADC_0); 
    BSP_ELC_Enable(BSP_ELC);
    BSP_Timer_Start(BSP_TIMER_OVERFLOW);

    AppPrint::PrintLog(">>> [Loopback] Waiting...");

    const uint16_t* p_raw_data = nullptr;
    // 注意：测试时先屏蔽掉 app2_progress() 线程，防止它抢信号导致死机
    if (EIS::DataPipeline::WaitForDataBlock(&p_raw_data, TX_WAIT_FOREVER)) {
        BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
        for (int i = 0; i < 50; i++) {
            BSP_Printf(COM_DEBUG, "Point[%d]: %u\r\n", i, p_raw_data[i]);
        }
    }
}