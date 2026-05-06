# SWIPT 接收终端

基于 STM32F103 的无线携能通信（SWIPT）接收终端，实现微波能量接收、信号解调与波束对准功能。

## 硬件平台

| 模块 | 型号/接口 |
|------|-----------|
| MCU | STM32F103 (Cortex-M3, 24MHz) |
| 显示 | 0.96" OLED (I2C) |
| 通信 | 蓝牙透传模块 (UART) |
| 采样 | ADC + DMA (TIM 触发) |
| 测温 | DS18B20 |
| 定位 | 波束扫描 (粗扫 + 细扫) |

## 软件架构

基于 FreeRTOS (CMSIS-OS v2)，包含三个核心任务：

```
DefaultTask (初始化)
    ├─ judgementTask  (高优先级) — ADC 采样、信号处理、波束扫描
    ├─ messageTask    (低优先级) — 消息解析、蓝牙指令响应、OLED 显示
    └─ powerTask      (低优先级) — 三级功耗管理
```

### 任务间通信

```
[ADC DMA]  →  judgementQueue  →  judgementTask
                                       ↓ (解调成功)
                                messageQueue  →  messageTask  →  OLED
[蓝牙 UART] →  blueToothQueue  ──────────↑
[波束扫描] ←  fineScanSem     ←  messageTask  (细扫描步进触发)
```

## 信号处理链

```
ADC 采样 → 匹配滤波 → 滑动均值滤波(去直流) → Gardner 位同步 → 帧同步(0xFDC8E92E) → 汉明译码
```

### 关键参数

| 参数 | 值 |
|------|-----|
| 采样率 (数据模式) | 2 ksps |
| 采样率 (扫描模式) | 5 ksps |
| 匹配滤波器阶数 | 17 |
| 均值滤波长度 | 256 |
| Gardner 环路带宽 | 0.0267 |
| 帧头 | `0xFDC8E92E` |
| 最大帧载荷 | 64 bytes |
| 汉明码长度 | 14 |

## 工作模式

| 模式 | `scanFlag` | 说明 |
|------|-----------|------|
| 正常判决 | `0` | 实时 ADC 采样，连续解调解码 |
| 粗扫描 | `1` | θ: 0~45° / φ: 0~360°，搜索最佳波束方向 |
| 细扫描 | `2` | 粗扫最优方向 ±3°，精确定位 |

粗扫描完成自动切换细扫描，细扫描完成回到正常判决模式。蓝牙串口发送 `"Scan Trigger"` 触发扫描流程。

## 功耗管理

三级功耗策略，由 `PowerManager` 任务管理：

1. **ACTIVE** — 全速运行，OLED 常亮，蓝牙收发
2. **DIM** — 5 秒无活动触发，OLED 熄灭，蓝牙待机
3. **DEEP SLEEP** — 30 秒后进入 STOP 模式，等待外部唤醒

## 目录结构

```
SWIPT/
├── PCB/                          # 硬件设计
│   ├── Gerber_*_V1.0~V9.0.zip   # 各版本 Gerber 文件
│   └── BOM_SWIPT_V3.0.xlsx      # V3.0 物料清单
├── STM32F103/
│   ├── yz_freertos/              # STM32CubeIDE 工程
│   │   ├── BSP/                  # 板级驱动 (OLED/蓝牙/DS18B20/ADC)
│   │   ├── Core/                 # HAL 外设初始化
│   │   ├── User/                 # 算法层 (解码/帧处理/Gardner/滤波/功耗)
│   │   └── MDK-ARM/              # Keil MDK 支持
│   └── yz_freertos_vscode/       # VS Code + CMake 工程 (替代 IDE)
│       └── AHT20 替代 DS18B20
└── README.md
```

## 开发环境

- **STM32CubeIDE** — 标准 CubeMX 生成工程 (FreeRTOS + HAL)
- **VS Code + CMake + GCC** — 轻量替代方案
- **Keil MDK** — 兼容 ARM Compiler 5/6

## 蓝牙指令

| 指令 | 功能 |
|------|------|
| `Scan Trigger` | 启动波束扫描 |
| `Close` | 触发一次细扫描步进 |
| `TEM` | 读取 DS18B20 温度 |

## PCB 版本演进

- V1.0 ~ V3.0: 原型验证，分立元件方案
- V4.0 ~ V9.0: 迭代优化，集成度提升

最新版本 BOM 见 `PCB/BOM_SWIPT_V3.0_2026-03-11.xlsx`。
