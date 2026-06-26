#ifndef __POWER_MANAGER_H
#define __POWER_MANAGER_H

/**
 * @file    PowerManager.h
 * @brief   Power management state machine interface
 * @author  YuZhang Lin
 * @date    2026-06-26
 */


#include "main.h"
#include "cmsis_os2.h"

/* Power states */
typedef enum {
    POWER_STATE_ACTIVE = 0,  /* Fully on: OLED, communication, RF sampling */
    POWER_STATE_DIM,         /* Dimmed: OLED off, peripherals off, sampling continues */
    POWER_STATE_DEEP_SLEEP   /* Deep sleep: STOP mode, all stopped */
} PowerState_t;

/* Timeout constants (FreeRTOS ticks @ 1kHz) */
#define IDLE_TO_DIM_TIME    5000    /* 5s  no activity → DIM */
#define DIM_TO_SLEEP_TIME   30000   /* 30s no activity → DEEP_SLEEP */

/* API */
void Power_Init(void);
void Power_RefreshActivity(void);
void Power_Manager_Task(void *argument);
PowerState_t Power_GetState(void);

#endif
