# ⚡ SWIPT 接收终端

基于 STM32F103 的无线携能通信（SWIPT）接收终端固件，实现微波能量波束扫描、信号解调与环境感知功能。

## 硬件平台

| 模块 | 型号 | 接口 |
|------|------|------|
| 🧠 MCU | STM32F103xB | Cortex-M3, HSE 8MHz → PLL 24MHz |
| 📺 显示 | 0.96" OLED | I2C1 |
| 📡 通信 | 蓝牙透传模块 | USART1 |
| ⚡ 采样 | 能量检测电路 | ADC1 + DMA (TIM3 触发) |
| 🌡️ 温湿度 | AHT20 | I2C2 |

## 🏗️ 软件架构

基于 FreeRTOS (CMSIS-OS v2)，采用 CMSIS-RTOS 队列通信：

```
DefaultTask (初始化后自毁)
    ├─ 🔴 judgementTask  高优先级  — ADC 采样、信号处理、波束扫描
    ├─ 🟡 messageTask    低优先级  — 消息解析、蓝牙指令响应、OLED 显示
    └─ 🟢 powerTask      低优先级  — 三级功耗管理
```

### 📨 任务间通信

```
[ADC DMA]  ──→  judgementQueue  ──→  judgementTask
                                         │ (解调成功)
                                   messageQueue  ──→  messageTask  ──→  OLED
[蓝牙 UART] ──→  blueToothQueue  ────────────────────→  ↑
[细扫描步进] ──→  fineScanSem    ──→  judgementTask
```

messageTask 使用 FreeRTOS Queue Set 同时监听 `messageQueue` 和 `blueToothQueue`，避免了轮询开销。

## 📶 信号处理链

```
ADC 采样 → 匹配滤波 (17阶) → 滑动均值滤波 (256点去直流) → Gardner 位同步 → 帧同步 (帧头 0xFDC8E92E) → 汉明译码 (14位)
```

### 🔧 关键参数

| 参数 | 数值 | 说明 |
|------|------|------|
| 系统时钟 | 24 MHz | HSE × 3 PLL |
| ADC 参考电压 | 3.3V / 12bit | 分辨率约 0.8mV |
| 采样率 (数据模式) | 2 ksps | `SAMPFREQ_DATA` |
| 采样率 (扫描模式) | 5 ksps | `SAMPFREQ_SCAN` |
| TIM 自动重装值 | 59 | |
| 匹配滤波器长度 | 17 | `MATCHED_LEN` |
| 滑动均值长度 | 256 | `AVERA_LEN` |
| Gardner 环路带宽 | 0.0267 | `LFC` |
| 帧同步头 | `0xFDC8E92E` | 32 位固定帧头 |
| 最大帧载荷 | 64 字节 | `MAX_FRAME_PAYLOAD` |
| 汉明码字长度 | 14 位 | `HANMING_LEN` |

## 🎯 波束扫描

系统通过微控制器控制天线阵列的波束方向，依次在离散角度上采集信号能量，找到能量最大的方向进行能量传输和信息解调。

### 🔭 粗扫描

| 参数 | 范围 | 步进 | 说明 |
|------|------|------|------|
| θ (俯仰) | 0° ~ 45° | 1° | 46 个离散点 |
| φ (方位) | 0° ~ 360° | 3° | 120 个离散点 |

每轮累积 50 个 ADC 采样点，共扫描 **5520 轮**，取能量最大轮次作为最优方向。

### 🔬 细扫描

在粗扫描确定的最优方向附近进行精确搜索：

| 参数 | 范围 | 步进 | 说明 |
|------|------|------|------|
| θ | best_θ ± 3° | 1° | 3 个离散点 |
| φ | 0° ~ 360° | 1° | 360 个离散点 |

共扫描 **1080 轮**，得到精确波束对准角度后自动切回正常判决模式。

## 🔄 工作模式

| 模式 | `scanFlag` | 采样率 | 功能 |
|------|-----------|--------|------|
| 📻 正常判决 | `0` | 2 ksps | 实时解调解码 |
| 🔍 粗扫描 | `1` | 5 ksps | 波束方向粗搜索 |
| 🎯 细扫描 | `2` | 5 ksps | 波束方向精对准 |

模式切换时自动调整 TIM 预分频器并清空 ADC 消息队列，保证采样数据新鲜度。

## 📡 蓝牙通信协议

蓝牙串口作为上位机控制与遥测回传通道。

### 📤 上行指令 (上位机 → 终端)

| 指令 | 功能 |
|------|------|
| `Scan Trigger` | 启动波束粗扫描流程 |
| `Close` | 触发一次细扫描步进（释放 fineScanSem） |
| `Environment` | 读取 AHT20 温湿度数据 |

### 📥 下行回传 (终端 → 上位机)

| 消息 | 说明 |
|------|------|
| `Ready` | 扫描准备就绪 |
| `ScanCnt:N` | 粗扫描当前轮次 |
| `CoDone:N` | 粗扫描完成，最优轮次为 N |
| `GoOn` | 细扫描通知继续下一步 |
| `Done:N` | 细扫描完成，最优轮次为 N |
| `Temp:xx.xC Humi:xx.x%` | 温湿度回传 |

## 📬 解调消息处理

解调后的消息在 messageTask 中按首字段分发：

| 消息内容 | 处理动作 |
|----------|----------|
| `Env` | 读取 AHT20 温湿度，通过蓝牙回传 |
| `0` | 波束已对准，开启蓝牙 |
| `1` | 关闭蓝牙 |

## 🔋 功耗管理

三级功耗策略，由 `PowerManager` 任务管理：

```
ACTIVE ──(5s 无活动)──→ DIM ──(30s)──→ DEEP_SLEEP
  ↑                      ↑
  └── 收到指令/判决数据 ──┘
```

| 状态 | OLED | 蓝牙 | 采样 |
|------|------|------|------|
| ⚡ ACTIVE | 常亮 | 正常收发 | 持续 |
| 💤 DIM | 熄灭 | 待机 | 间歇 |
| 😴 DEEP_SLEEP | 熄灭 | 关闭 | STOP 模式 |

## 📂 目录结构

```
STM32F103/yz_freertos_vscode/
├── BSP/                          # 板级驱动层
│   ├── AHT20.c/h                 # 🌡️  AHT20 温湿度传感器 (I2C2)
│   ├── Bluetooth.c/h             # 📡 蓝牙透传 (USART1 DMA)
│   ├── OLED.c/h                  # 📺 OLED 显示屏 (I2C1)
│   ├── OLED_Font.h               # 🔤 字库
│   └── Receiver_ADC.c/h          # ⚡ 能量检测 ADC (TIM3 触发 DMA)
├── User/                         # 上层算法层
│   ├── Decoder.c/h               # 🧩 汉明译码器
│   ├── FrameProcessing.c/h       # 📦 帧同步与帧载荷提取
│   ├── Gardner.c/h               # 🎛️  Gardner 位同步
│   ├── Global_Define.h           # ⚙️  全局参数定义
│   ├── PowerManager.c/h          # 🔋 功耗管理状态机
│   └── Preprocessing.c/h         # 🔢 匹配滤波 + 滑动均值滤波
├── Core/                         # STM32CubeMX 生成
│   ├── Src/                      # HAL 外设初始化和 ISR
│   └── Inc/                      # 外设头文件、FreeRTOSConfig
├── Drivers/                      # HAL 库 + CMSIS
├── Middlewares/                  # FreeRTOS 内核
├── cmake/                        # CMake 工具链
│   ├── gcc-arm-none-eabi.cmake   # 🔧 ARM GCC 工具链
│   ├── starm-clang.cmake         # 🔧 ARM Clang 工具链
│   └── stm32cubemx/              # CubeMX 生成的 CMake 子项目
├── CMakeLists.txt                # 顶层 CMake
├── CMakePresets.json             # CMake 预设
├── startup_stm32f103xb.s         # 🚀 启动文件
├── STM32F103XX_FLASH.ld          # 🗺️  链接脚本
└── yz_freertos.ioc               # 📐 CubeMX 项目配置
```

## 🛠️ 开发环境

- **构建系统**：CMake 3.22+
- **工具链**：ARM GNU GCC (`arm-none-eabi-gcc`) 或 ARM Clang
- **IDE**：VS Code + CMake Tools 插件
- **配置工具**：STM32CubeMX（修改 IOC 后重新生成 `cmake/stm32cubemx/` 下的 HAL 代码）
- **调试器**：ST-Link / J-Link（SWD 接口）

## 🚀 构建与烧录

```bash
# 配置 (根据工具链选择 preset)
cmake --preset gcc-arm-none-eabi

# 编译
cmake --build build/Debug -j

# 烧录 (ST-Link)
STM32_Programmer_CLI -c port=SWD -w build/Debug/yz_freertos.elf
```

## 📐 PCB 版本

Gerber 文件位于 `../PCB/` 目录，共 V1.0 ~ V9.0 九个迭代版本：

| 版本 | 文件 |
|------|------|
| 📐 V1.0 ~ V9.0 | `Gerber_SWIPT_Yzself_V*.zip` |
| 📋 V3.0 BOM | `BOM_SWIPT_V3.0_2026-03-11.xlsx` |
