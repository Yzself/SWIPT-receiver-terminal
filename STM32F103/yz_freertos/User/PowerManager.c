#include "PowerManager.h"
#include "OLED.h"
#include "Bluetooth.h"
#include "Receiver_ADC.h"
#include "Global_Define.h"
#include <string.h>

static uint32_t lastActivityTick = 0;												// 最近一次活跃时间
static PowerState_t currentPowerState = POWER_STATE_ACTIVE;	// 当前节能等级	

/**
 * @brief 初始化电源管理
 */
void Power_Init(void) {
    lastActivityTick = osKernelGetTickCount();
    currentPowerState = POWER_STATE_ACTIVE;
}

/**
 * @brief 刷新活跃时间戳
 * @note 在蓝牙中断、解调成功、按键触发等位置调用
 */
void Power_RefreshActivity(void) {
    lastActivityTick = osKernelGetTickCount();
}

/**
 * @brief 获取当前电源状态
 */
PowerState_t Power_GetState(void) {
    return currentPowerState;
}

/**
 * @brief 执行进入低功耗前的硬件清理
 */
static void Prepare_For_Sleep(void) {
		OLED_Clear();
		OLED_ShowString(1,1,"Time2Sleep");
		osDelay(3000);
	
    Re_ADC_Stop();      				 // 停止ADC DMA
    BT_Off();           				 // 蓝牙断电
    OLED_Hardware_PowerOff();    // OLED掉电
	
		/* --- PA0 唤醒配置开始 --- */
    
    // 2. 确保电源接口时钟开启（使能对电源寄存器的访问）
    __HAL_RCC_PWR_CLK_ENABLE();

    // 3. 清除之前的唤醒标志 (Wakeup Flag)
    // 如果不清除，可能导致系统进入待机后立即被“残留”的中断信号唤醒
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    // 4. 将 PA0 使能为唤醒引脚 (WKUP1)
    // 注意：一旦执行这行，PA0 的 ADC 或普通 GPIO 功能在芯片进入 STANDBY 后会被屏蔽
    // 只有 PA0 上的上升沿信号能唤醒芯片
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1); 
    
    /* --- PA0 唤醒配置结束 --- */
}

/**
 * @brief 电源管理主任务
 */
void Power_Manager_Task(void *argument) {
		Power_Init();
    for(;;) {
        uint32_t elapsed = osKernelGetTickCount() - lastActivityTick;

        switch (currentPowerState) {
            case POWER_STATE_ACTIVE:
                // 如果超过IDLE_TO_DIM_TIM毫秒没有活跃，进入二级节能
                if (elapsed > IDLE_TO_DIM_TIME) {
                    OLED_Poweroff();			// 屏幕指令熄灭
                    currentPowerState = POWER_STATE_DIM;
                }
                break;

            case POWER_STATE_DIM:
                // 如果在DIM期间有了新活跃（比如解调出新数据），切回ACTIVE
                if (elapsed < IDLE_TO_DIM_TIME) {
                    currentPowerState = POWER_STATE_ACTIVE;
                }
                // 如果长时间无数据（DIM_TO_SLEEP_TIME毫秒），并且不处于扫描模式下，进入深度睡眠
                else if (elapsed > DIM_TO_SLEEP_TIME && scanFlag == 0) {
                    currentPowerState = POWER_STATE_DEEP_SLEEP;
                    Prepare_For_Sleep();
                    
                    // 正式进入待机模式
                    HAL_PWR_EnterSTANDBYMode();		  	
                }
                break;

            case POWER_STATE_DEEP_SLEEP:
                // 这种状态理论上直接进入STANDBY内阻塞，不会循环到这里
                break;
						default: 
								break;
        }
        osDelay(500); // 轮询频率不需要太快，节能管理为低实时性任务
    }
}
