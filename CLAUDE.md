# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

SWIPT (Simultaneous Wireless Information and Power Transfer) receiver terminal firmware running on STM32F103xB (Cortex-M3, 24MHz) with FreeRTOS. The device samples microwave energy via ADC, performs beam scanning to find the optimal power reception angle, then demodulates data using matched filtering + Gardner timing recovery + Hamming decoding.

## Build system

The active project is `STM32F103/yz_freertos_vscode/`. The old `yz_freertos/` (STM32CubeIDE) has been deleted.

- **Build tool**: CMake 3.22+ with Ninja generator
- **Toolchain**: `arm-none-eabi-gcc` (must be on PATH)
- **Presets**: `Debug` and `Release` defined in `CMakePresets.json`

```bash
cd STM32F103/yz_freertos_vscode
cmake --preset Debug
cmake --build build/Debug -j
```

Output: `build/Debug/yz_freertos.elf` (also produces `.hex`, `.bin`, `.map`).

There is no test suite, linter, or CI pipeline in this project.

## Architecture

```
BSP/                    # Hardware abstraction — peripherals, sensors, display
User/                   # DSP algorithms — filters, timing recovery, decoder
Core/                   # CubeMX-generated HAL init + freertos.c (task bodies)
Drivers/                # STM32 HAL library (vendor, do not edit)
Middlewares/             # FreeRTOS kernel (vendor, do not edit)
```

### Task structure

Defined in `Core/Src/freertos.c`:

| Task | Priority | Role |
|------|----------|------|
| `judgementTask` | Normal (high) | ADC read → matched filter → average filter → Gardner → frame sync → Hamming decode. Also runs coarse/fine beam scanning. |
| `messageTask` | BelowNormal | Processes decoded messages from `judgementTask` and Bluetooth commands. Uses FreeRTOS Queue Set to multiplex two queues. Drives OLED display. |
| `powerTask` | BelowNormal | Idle timeout watchdog. ACTIVE → DIM (5s) → DEEP_SLEEP/STANDBY (30s). PA0 WKUP pin wakes from STANDBY via RF energy detection, causing MCU reset back to ACTIVE. |

`DefaultTask` initializes all three tasks then terminates (`osThreadTerminate`).

### Data flow

```
Energy harvester → ADC1 (TIM3-triggered DMA) → judgementQueue → judgementTask
                                                                   │ frame decoded
                                                             messageQueue → messageTask → OLED / Bluetooth
Bluetooth UART1 RX interrupt → blueToothQueue ────────────────────┘
```

### Mode switching

Global `scanFlag` (declared in `freertos.c`, referenced in `Global_Define.h`) controls operating mode:

| `scanFlag` | Mode | Sample rate |
|------------|------|-------------|
| 0 | Normal demodulation | 2 ksps |
| 1 | Coarse beam scan | 5 ksps |
| 2 | Fine beam scan | 5 ksps |

`Mode_Change()` adjusts TIM3 prescaler and flushes the ADC queue when switching modes.

### Key files to modify

- **Add a BSP driver**: `BSP/` (header + source), then register in `CMakeLists.txt` under `target_sources` and `target_include_directories`
- **Change DSP parameters**: `User/Global_Define.h` (filter lengths, Gardner bandwidth, scan angles, frame header)
- **Change task behavior**: `Core/Src/freertos.c` (task functions are in the USER CODE blocks)
- **Change pinout/peripherals**: `yz_freertos.ioc` in CubeMX, then regenerate `cmake/stm32cubemx/`

### Coding conventions

- `BSP/` files are plain C with no RTOS dependencies (except `Receiver_ADC.c` which uses CMSIS-OS queues).
- `User/` files are pure DSP algorithms, no hardware dependencies.
- `Core/Src/freertos.c` contains all FreeRTOS task bodies in the USER CODE sections. CubeMX re-generation preserves these sections.
- Printf-style debug output goes through Bluetooth DMA: `BT_SendPrintf_DMA()`.

## Git workflow

The branch is `main`. There are no branch protection rules or PR templates. Commit messages use Chinese. The repo is at `github.com:Yzself/SWIPT-receiver-terminal` — use SSH for remote operations.

## PCB

Manufacturing files live in `../PCB/` (relative to the firmware directory). Nine iterations V1.0–V9.0 as Gerber `.zip` archives, plus a V3.0 BOM spreadsheet.
