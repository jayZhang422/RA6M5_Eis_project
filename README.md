# cmake_test (Renesas RA6M5 + ThreadX + EIS)

本项目基于 Renesas RA6M5，使用 FSP/RASC 生成底层工程，采用 CMake + ARM GCC 构建。  
当前代码已具备 EIS（电化学阻抗）核心链路与验证模块，可作为电池内阻、转移阻抗和 SOH 工程的基础版本。

## 项目概览

- MCU/平台：Renesas RA6M5 + ThreadX
- 构建系统：CMake + Ninja + `arm-none-eabi-gcc/g++`
- 主要能力：
- ADC/DAC/DMAC/ELC 定时触发与采样链路
- 参考波生成、数字锁相解调、阻抗求解
- FFT/信号质量评估/AGC/安全监测/OSL 校准模块
- 串口验证入口与屏幕 UI 入口

## 目录结构（核心）

- `src/`：BSP 封装与线程入口（ADC/DMAC/UART/TIMER 等）
- `Algorithm/`：EIS 算法组件（解调、阻抗、校准、安全、数据流水线）
- `Application/`：测试流程与业务验证（`Phase_test`、`eis_uart_verify`）
- `User_app/`：屏幕业务侧逻辑（Nyquist/Bode/结果页交互）
- `Driver/`：外设驱动封装
- `ra_gen/`、`ra_cfg/`：FSP/RASC 生成代码与配置（不建议手改）

## 构建方式（CLI）

配置：

```bash
cmake -DARM_TOOLCHAIN_PATH="/your/toolchain/path" -DCMAKE_TOOLCHAIN_FILE=cmake/gcc.cmake -G Ninja -B build/Debug
```

示例：

```bash
cmake -DARM_TOOLCHAIN_PATH="C:/12_2_mpacbti_rel1/bin" -DCMAKE_TOOLCHAIN_FILE=cmake/gcc.cmake -G Ninja -B build/Debug
cmake -DARM_TOOLCHAIN_PATH="C:/12_2_mpacbti_rel1/bin" -DCMAKE_TOOLCHAIN_FILE=cmake/gcc.cmake -DCMAKE_BUILD_TYPE=Release -G Ninja -B build/Release
```

编译：

```bash
cmake --build build/Debug
```

烧录（需安装 pyOCD，目标在 `CMakeLists.txt` 中已定义）：

```bash
cmake --build build/Debug --target flash
```

## VS Code 配置

- 启动 VS Code 前设置环境变量 `ARM_GCC_TOOLCHAIN_PATH`
- 或在 `.vscode/cmake-kits.json` 中设置 `ARM_TOOLCHAIN_PATH`
- 状态栏选择 `ARM GCC kit with toolchainFile`
- 建议工具链路径和工程路径避免空格

示例：

```bash
set ARM_GCC_TOOLCHAIN_PATH=C:/12_2_mpacbti_rel1/bin
cd "D:/REANSA/cmake_test" && code .
```

## 运行入口说明

- App 线程入口：`src/App_Thread_entry.cpp`
- 屏幕线程入口：`src/Screen_Thread_entry.cpp`
- 当前可选测试入口（按需在 `App_Thread_entry` 中启用）：
- `Run_eis_uart_verify_server()`：串口喂包验证 EIS 算法链路
- `Run_phase2_test()` / `Run_phase25_test()`：分阶段联调流程

## EIS 与 SOH 开发建议

当前工程已具备 SOH 初版所需的大部分基础能力，后续可优先推进：

1. 固化特征：`R0`、特征频点转移阻抗、相位特征。
2. 建立映射：特征到 SOH 的分段经验模型。
3. 加入补偿：温度/SOC 的一阶补偿。
4. 完成闭环：测量、显示、记录、阈值告警。

---

如果你准备开始 SOH 初版，可直接在现有 EIS 结果输出后增加特征归一化与健康度评分模块，先实现“可用版”，再逐步迭代精度。
