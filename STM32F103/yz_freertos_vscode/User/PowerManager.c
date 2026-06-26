/**
 * @file    PowerManager.c
 * @brief   Power management state machine (ACTIVE/DIM/SLEEP)
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "PowerManager.h"
#include "OLED.h"
#include "SubGHz_CC1310.h"
#include "Receiver_ADC.h"
#include "Global_Define.h"
#include <string.h>

static uint32_t lastActivityTick = 0;
static PowerState_t currentPowerState = POWER_STATE_ACTIVE;

void Power_Init(void)
{
    lastActivityTick = osKernelGetTickCount();
    currentPowerState = POWER_STATE_ACTIVE;
}

void Power_RefreshActivity(void)
{
    lastActivityTick = osKernelGetTickCount();
}

PowerState_t Power_GetState(void)
{
    return currentPowerState;
}

static void Prepare_For_Sleep(void)
{
    OLED_Clear();
    OLED_ShowString(2, 5, "Standby!");
    osDelay(3000);

    Re_ADC_Stop();
    OLED_Hardware_PowerOff();

    /* Configure PA0 as wake-up pin */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
}

void Power_Manager_Task(void *argument)
{
    Power_Init();
#if POWER_MANAGER_ENABLE
    for (;;) {
        uint32_t elapsed = osKernelGetTickCount() - lastActivityTick;

        switch (currentPowerState) {
        case POWER_STATE_ACTIVE:
            if (elapsed > IDLE_TO_DIM_TIME) {
                OLED_Poweroff();
                currentPowerState = POWER_STATE_DIM;
            }
            break;

        case POWER_STATE_DIM:
            if (elapsed < IDLE_TO_DIM_TIME) {
                currentPowerState = POWER_STATE_ACTIVE;
            } else if (elapsed > DIM_TO_SLEEP_TIME && scanFlag == 0) {
                currentPowerState = POWER_STATE_DEEP_SLEEP;
                Prepare_For_Sleep();
                HAL_PWR_EnterSTANDBYMode();
            }
            break;

        case POWER_STATE_DEEP_SLEEP:
            break;
        }
        osDelay(500);
    }
#else
    for (;;) {
        osDelay(5000);
    }
#endif
}
