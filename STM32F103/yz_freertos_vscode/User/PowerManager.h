#ifndef __POWER_MANAGER_H
#define __POWER_MANAGER_H

#include "main.h"
#include "cmsis_os2.h"

// 电源状态定义
typedef enum {
    POWER_STATE_ACTIVE = 0,  // 全速运行：OLED亮，蓝牙通，高频采样
    POWER_STATE_DIM,         // 节能运行：OLED熄屏，蓝牙关，保持采样解调
    POWER_STATE_DEEP_SLEEP   // 深度睡眠：进入STOP模式，仅保留唤醒
} PowerState_t;

// 配置常量
#define IDLE_TO_DIM_TIME    5000   // 5秒无交互进入DIM状态 (熄屏/关蓝牙)
#define DIM_TO_SLEEP_TIME   30000  // 30秒无数据进入DEEP_SLEEP (停止采样)

// 函数声明
void Power_Init(void);
void Power_RefreshActivity(void);         // 发生交互时调用（如收到指令、判决出数据）
void Power_Manager_Task(void *argument);  // 电源管理任务条目
PowerState_t Power_GetState(void);

#endif
