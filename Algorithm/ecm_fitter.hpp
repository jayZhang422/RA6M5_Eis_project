#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "bsp_algorithm.hpp" // 引用你的 ImpedanceResult

namespace EIS {

// 电池健康度 (SOH) 核心参数：Randles 模型
struct EcmParams {
    float Rs;       // 欧姆内阻 (Ohm) - 对应高频截距，直接反映物理老化/失水
    float Rct;      // 极化内阻 (Ohm) - 对应中低频半圆，直接反映化学反应活性
    float Cdl;      // 双电层电容 (F) - 电极/电解液界面的电容效应
    float MSE;      // Mean Square Error (均方误差)，用于评估拟合可信度
};

// ==========================================
// 算子：等效电路模型最小二乘拟合器 (非线性)
// ==========================================
class EcmFitter {
public:
    /**
     * @brief 基于梯度下降的非线性最小二乘法，提取电池内部化学参数
     * @param freqs_hz    [输入] 扫频频率数组
     * @param z_data      [输入] 测得的复数阻抗数组 (需先经过 OSL 校准)
     * @param data_len    [输入] 数据点数量
     * @param max_iters   [输入] 最大迭代次数 (防死机，MCU建议 100-500)
     * @param p_out       [输出] 拟合得到的等效电路参数
     * @return bool       是否拟合成功收敛
     */
    static bool FitRandlesModel(const float* freqs_hz, const ImpedanceResult* z_data, uint16_t data_len, uint16_t max_iters, EcmParams* p_out) {
        if (!freqs_hz || !z_data || !p_out || data_len < 3) return false;

        // 1. 初值猜测 (非常重要！非线性最小二乘法极其依赖初值)
        EstimateInitialGuess(freqs_hz, z_data, data_len, p_out);

        // 学习率配置 (根据实际阻抗量级可能需要微调)
        float lr_Rs = 0.01f;
        float lr_Rct = 0.01f;
        float lr_Cdl = 1e-6f;

        float current_mse = CalculateMSE(freqs_hz, z_data, data_len, p_out);

        // 2. 迭代寻优 (最小化残差平方和)
        for (uint16_t iter = 0; iter < max_iters; iter++) {
            float grad_Rs = 0.0f;
            float grad_Rct = 0.0f;
            float grad_Cdl = 0.0f;

            // 计算所有数据点的梯度累加
            for (uint16_t i = 0; i < data_len; i++) {
                float w = 2.0f * PI * freqs_hz[i];
                float w_Cdl = w * p_out->Cdl;
                float den = 1.0f + (w * p_out->Rct * p_out->Cdl) * (w * p_out->Rct * p_out->Cdl);
                
                // 模型预测值
                float Z_real_pred = p_out->Rs + (p_out->Rct / den);
                float Z_imag_pred = - (w * p_out->Rct * p_out->Rct * p_out->Cdl) / den;

                // 误差 (预测值 - 真实值)
                float err_real = Z_real_pred - z_data[i].R_real;
                float err_imag = Z_imag_pred - z_data[i].R_imag;

                // 偏导数 (Jacobian矩阵的元素，此处做了简化推导)
                // d(Zreal)/d(Rs) = 1
                grad_Rs += 2.0f * err_real * 1.0f;
                // d(Zreal)/d(Rct) 等其他偏导数，这里采用数值微分为主，代码更短且防错
                // 为了极简和MCU运算安全，可以采用微小摄动法求梯度 (Numerical Gradient)
            }

            // 数值微扰法求偏导 (比解析公式更不容易写错，适合嵌入式)
            grad_Rs  = ComputeNumericalGradient(freqs_hz, z_data, data_len, *p_out, 0);
            grad_Rct = ComputeNumericalGradient(freqs_hz, z_data, data_len, *p_out, 1);
            grad_Cdl = ComputeNumericalGradient(freqs_hz, z_data, data_len, *p_out, 2);

            // 3. 参数更新
            EcmParams next_params = *p_out;
            next_params.Rs  -= lr_Rs  * grad_Rs;
            next_params.Rct -= lr_Rct * grad_Rct;
            next_params.Cdl -= lr_Cdl * grad_Cdl;

            // 4. 物理边界约束 (防止拟合出负数电阻或 NaN 导致算法崩溃)
            if (next_params.Rs < 1e-5f)  next_params.Rs = 1e-5f;
            if (next_params.Rct < 1e-5f) next_params.Rct = 1e-5f;
            if (next_params.Cdl < 1e-9f) next_params.Cdl = 1e-9f;

            // 5. 评估是否收敛
            float next_mse = CalculateMSE(freqs_hz, z_data, data_len, &next_params);
            if (next_mse < current_mse) {
                *p_out = next_params;
                
                // 提前停止条件：如果改进微乎其微
                if ((current_mse - next_mse) / current_mse < 1e-4f) {
                    p_out->MSE = next_mse;
                    return true; 
                }
                current_mse = next_mse;
            } else {
                // 如果步子迈太大导致误差上升，降低学习率 (自适应衰减)
                lr_Rs *= 0.5f; lr_Rct *= 0.5f; lr_Cdl *= 0.5f;
            }
        }

        p_out->MSE = current_mse;
        return true; // 达到最大迭代次数退出
    }

private:
    // 启发式估计初始参数 (极度关键)
    static void EstimateInitialGuess(const float* freqs_hz, const ImpedanceResult* z_data, uint16_t data_len, EcmParams* p_out) {
        // Rs 往往是高频区 (频率最大) 与实轴的交点 (Imag 接近 0)
        // 简单起见，取最高频率点的实部作为 Rs 初始值
        p_out->Rs = z_data[data_len - 1].R_real; 

        // Rct 往往是低频区实部减去高频区实部 (半圆的直径)
        float max_real = z_data[0].R_real;
        for(uint16_t i=0; i<data_len; i++) {
            if(z_data[i].R_real > max_real) max_real = z_data[i].R_real;
        }
        p_out->Rct = max_real - p_out->Rs;
        if (p_out->Rct <= 0.0f) p_out->Rct = 0.01f; // 保护

        // Cdl 估算：在半圆顶点 (负虚部最大处)，频率 f_max 满足 2 * PI * f_max * Rct * Cdl = 1
        float min_imag = 0.0f; 
        float f_at_min_imag = freqs_hz[0];
        for(uint16_t i=0; i<data_len; i++) {
            if(z_data[i].R_imag < min_imag) { // 虚部通常在第四象限为负
                min_imag = z_data[i].R_imag;
                f_at_min_imag = freqs_hz[i];
            }
        }
        if (f_at_min_imag > 0.0f) {
            p_out->Cdl = 1.0f / (2.0f * PI * f_at_min_imag * p_out->Rct);
        } else {
            p_out->Cdl = 1.0f; // 兜底
        }
    }

    // 计算均方误差 (MSE)
    static float CalculateMSE(const float* freqs_hz, const ImpedanceResult* z_data, uint16_t data_len, const EcmParams* p) {
        float sum_sq_err = 0.0f;
        for (uint16_t i = 0; i < data_len; i++) {
            float w = 2.0f * PI * freqs_hz[i];
            float den = 1.0f + (w * p->Rct * p->Cdl) * (w * p->Rct * p->Cdl);
            float z_real_pred = p->Rs + (p->Rct / den);
            float z_imag_pred = - (w * p->Rct * p->Rct * p->Cdl) / den;

            float err_r = z_real_pred - z_data[i].R_real;
            float err_i = z_imag_pred - z_data[i].R_imag;
            sum_sq_err += (err_r * err_r + err_i * err_i);
        }
        return sum_sq_err / (float)data_len;
    }

    // 数值微分计算偏导数
    static float ComputeNumericalGradient(const float* freqs_hz, const ImpedanceResult* z_data, uint16_t data_len, EcmParams p, uint8_t param_idx) {
        float epsilon = 1e-6f;
        float base_mse = CalculateMSE(freqs_hz, z_data, data_len, &p);
        
        if (param_idx == 0) { p.Rs += epsilon; }
        else if (param_idx == 1) { p.Rct += epsilon; }
        else if (param_idx == 2) { p.Cdl += epsilon * 1e-3f; } // Cdl量级通常很小
        
        float new_mse = CalculateMSE(freqs_hz, z_data, data_len, &p);
        return (new_mse - base_mse) / epsilon;
    }
};

} // namespace EIS