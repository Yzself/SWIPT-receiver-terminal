/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "queue.h"
#include "OLED.h"
#include "Receiver_ADC.h"
#include "Global_Define.h"
#include "Preprocessing.h"
#include "FrameProcessing.h"
#include "Gardner.h"
#include "PowerManager.h"
#include "AHT20.h"
#include "SubGHz_CC1310.h"
#include "Uart_Printf.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
// 任务相关
BaseType_t judgementTaskCreateReturn;
TaskHandle_t judgementTaskHandle;

BaseType_t messageTaskCreateReturn;
TaskHandle_t messageTaskHandle;

BaseType_t powerTaskCreateReturn;
TaskHandle_t powerTaskHandle;
// 最终拿到的信息
uint8_t g_FinalDisplayMsg[MAX_FRAME_PAYLOAD + 1];
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for judgementQueue */
osMessageQueueId_t judgementQueueHandle;
const osMessageQueueAttr_t judgementQueue_attributes = {
  .name = "judgementQueue"
};
/* Definitions for messageQueue */
osMessageQueueId_t messageQueueHandle;
const osMessageQueueAttr_t messageQueue_attributes = {
  .name = "messageQueue"
};
/* Definitions for subMessageQueue */
osMessageQueueId_t subMessageQueueHandle;
const osMessageQueueAttr_t subMessageQueue_attributes = {
  .name = "subMessageQueue"
};
/* Definitions for fineScanSem */
osSemaphoreId_t fineScanSemHandle;
const osSemaphoreAttr_t fineScanSem_attributes = {
  .name = "fineScanSem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void judgementTask(void *params);
void messageTask(void *params);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	Uart_Printf_Init();
	CC1310_Init();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of fineScanSem */
  fineScanSemHandle = osSemaphoreNew(1, 0, &fineScanSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of judgementQueue */
  judgementQueueHandle = osMessageQueueNew (8, sizeof(uint16_t), &judgementQueue_attributes);

  /* creation of messageQueue */
  messageQueueHandle = osMessageQueueNew (4, sizeof(uint32_t), &messageQueue_attributes);

  /* creation of subMessageQueue */
  subMessageQueueHandle = osMessageQueueNew (4, sizeof(uint32_t), &subMessageQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  CC1310_Queue_Register(subMessageQueueHandle);
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
	OLED_Hardware_PowerOn();			
	OLED_Clear();
	OLED_ShowString(2,4,"PowerManager");
	if(POWER_MANAGER_ENABLE)
	{
		OLED_ShowString(3,4,"Open");
	}else{
		
		OLED_ShowString(3,4,"Close");
	}

	CC1310_Open();
	// 创建判决任务，要求高实时性
	judgementTaskCreateReturn = xTaskCreate(judgementTask, "judgementTask", 256, NULL, osPriorityNormal, &judgementTaskHandle);
	if(judgementTaskCreateReturn != pdPASS) return;
	
	// 创建信息处理任务，实时性要求偏低
	messageTaskCreateReturn = xTaskCreate(messageTask, "messageTask", 256, NULL, osPriorityBelowNormal, &messageTaskHandle);
	if(messageTaskCreateReturn != pdPASS) return;
	
	// 创建电源功耗管理任务，实时性要求偏低
	powerTaskCreateReturn = xTaskCreate(Power_Manager_Task, "powerTask", 128, NULL, osPriorityBelowNormal, &powerTaskHandle);
	if(powerTaskCreateReturn != pdPASS) return;
	
	// 初始化结束，自杀
	osThreadTerminate(osThreadGetId()); 
	
//  for(;;)
//  {
//		osDelay(1000); // 默认任务可以空转或处理其他低频逻辑
//  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
volatile uint8_t scanFlag = 0; // 0：正常判决 1：粗扫描 2：细扫描
/* 判决任务(高优) */
void judgementTask(void *params)
{
	uint16_t adc_int;
	volatile static float adc_float;	          
	osStatus_t status;
	
	// 判决&信息接收相关
	float match_out;															// 匹配滤波输出
	float avg_out;																// 滑动均值滤波输出
	GardnerStruct GStruct;												// Gardner位同步信息
	GStruct.HD_Trigger = false;
	GStruct.JudgeDat = 0;	
	uint8_t reMessage[MAX_FRAME_PAYLOAD + 1];			// 接收信息存储
	const float SCALE_FACTOR = SAMPREF / SAMPRES; // ADC分辨率
	
	// 粗扫描相关
	uint8_t coScanPerCnt = 50;				// 粗扫描每轮累计索引
	uint32_t coScanNew = 0;						// 粗扫描最新累积幅度
	uint16_t coScanCnt = 0;						// 粗扫描最新扫描次数
	uint32_t coScanMax = 0;					  // 粗扫描最大积累幅度
	uint16_t coScanMaxCnt = 0;				// 粗扫描得到最大积累幅度时的扫描次数
	
	// 细扫描相关
	bool fineScanPerFlag = false;			// true:开启一轮细扫描
	uint8_t fineScanPerCnt = 50;			// 细扫描每轮累计索引
	uint32_t fineScanNew = 0;					// 细扫描最新累积幅度
	uint16_t fineScanCnt = 0;					// 细扫描最新扫描次数
	uint32_t fineScanMax = 0;					// 细扫描最大积累幅度
	uint16_t fineScanMaxCnt = 0;			// 细扫描得到最大积累幅度时的扫描次数
	
	// 注册ADC接收通道
	Re_ADC_Queue_Register(judgementQueueHandle);
	// 开启TIM-ADC-DMA
	Re_ADC_Init(); 
	
	while(1)
	{
		// 从队列中获取采样值
		status = osMessageQueueGet(judgementQueueHandle, &adc_int, NULL, osWaitForever);
    
		if (status == osOK)
		{
			// 成功拿到了采样数据 adc_int
			switch(scanFlag){
				case 0:/* 正常判决，不扫描，采样率SAMPFREQ_DATA */
					adc_float = (float)adc_int * SCALE_FACTOR;
					// 匹配滤波
					match_out = Match_Filter(adc_float);
					// 滑动均值滤波(去直流)
					avg_out = Average_Filter(match_out);
					// Gardner位同步
					Gardner(avg_out, &GStruct);
					if(GStruct.HD_Trigger == true)
					{
						// 帧处理 + 解码
						if(FrameProcessing(GStruct.JudgeDat, reMessage))
						{
							Power_RefreshActivity();
							// 1. 停止判决任务，开始全心投入到解析收到的信息
							Re_ADC_Stop();	
							
							// 2. 将解析好的数据拷贝到全局显示缓存
							memcpy(g_FinalDisplayMsg, reMessage, sizeof(g_FinalDisplayMsg));
												
							// 3. 发送指针给显示任务
							// 发送 g_FinalDisplayMsg 的首地址
							uint8_t *pMsg = g_FinalDisplayMsg;
							osMessageQueuePut(messageQueueHandle, &pMsg, 0, 0);
						}
					}
					break;
				case 1:/* 粗扫描,采样率SAMPFREQ_SCAN */
					if(coScanPerCnt > 0)
					{
						coScanNew += adc_int;
						coScanPerCnt --;
					}else{
						coScanPerCnt = 50;
						coScanCnt ++;
						CC1310_SendPrintf("ScanCnt:%d",coScanCnt);
						if(coScanNew > coScanMax)
						{
							coScanMax = coScanNew;
							coScanMaxCnt = coScanCnt;
						}
						coScanNew = 0;
						
						if(coScanCnt == BEAM_COARSE_SCAN_NUM)
						{
							// 粗扫描结束
							CC1310_SendPrintf("CoDone:%d",coScanMaxCnt);
							coScanMax = 0;
							coScanMaxCnt = 0;
							coScanCnt = 0;
							Power_RefreshActivity();
							
							scanFlag = 2; //切换到细扫描
							Mode_Change(scanFlag);
						}
					}
					break;
				case 2:/* 细扫描,采样率SAMPFREQ_SCAN */
					if(fineScanPerFlag)
					{
						if(fineScanPerCnt > 0)
						{
							fineScanNew += adc_int;
							fineScanPerCnt --;
						}else{
							Power_RefreshActivity();
							fineScanPerCnt = 50;
							fineScanPerFlag = false;
							fineScanCnt ++;
							if(fineScanNew > fineScanMax)
							{
								fineScanMax = fineScanNew;
								fineScanMaxCnt = fineScanCnt;
							}
							fineScanNew = 0;
							
							if(fineScanCnt < BEAM_CLOSE_SCAN_NUM)
							{
								CC1310_SendPrintf("GoOn");
							}else{
								// 细扫描结束
								CC1310_SendPrintf("Done:%d",fineScanMaxCnt);
								fineScanMax = 0;
								fineScanMaxCnt = 0;
								fineScanCnt = 0;
								
								// 开始解调信息
								scanFlag = 0;
								Mode_Change(scanFlag);
							}
						}
					}
					
					if(!fineScanPerFlag)
					{
						if(osSemaphoreAcquire(fineScanSemHandle, 0) == osOK)
						{
							fineScanPerFlag = true;
							// 获取瞬间清空一次队列，保证采样新鲜度
							osMessageQueueReset(judgementQueueHandle); 
						}
					}
					break;
				default:
					break;
			}
		}
	}
}
/* 信息处理任务(低优) */
QueueSetHandle_t masterQueueSet; // 消息队列集
void messageTask(void *params)
{
	// 通用指针，用于接收队列中的地址
	uint8_t *pReceivedPtr; 
	// 用于缓存显示接收信息[Re:****][Bt:****]
	char displayBuf[MAX_FRAME_PAYLOAD + 4];
	
	//创建队列集(原生函数，引入头文件queue.h，同时保证configUSE_QUEUE_SETS为1)
	masterQueueSet = xQueueCreateSet(4 + 4);
	xQueueAddToSet((QueueHandle_t)messageQueueHandle, masterQueueSet);
	xQueueAddToSet((QueueHandle_t)subMessageQueueHandle, masterQueueSet);
	while(1)
	{
		// 监听消息队列
    	QueueSetMemberHandle_t activatedMember = xQueueSelectFromSet(masterQueueSet, portMAX_DELAY);
		
		if (activatedMember == (QueueSetMemberHandle_t)subMessageQueueHandle)	// 辅助消息处理
		{
			if (osMessageQueueGet(subMessageQueueHandle, &pReceivedPtr, NULL, 0) == osOK)
      		{
				Power_RefreshActivity();
				// OLED 显示
				char *str = (char*)pReceivedPtr;
				strcpy(displayBuf, "Bt:");
				strcat(displayBuf, str);
				OLED_Clear();
				OLED_ShowString(1, 1, displayBuf);
				//printf("%s\r\n", displayBuf);
				CC1310_SendPrintf("Bt:%s", str);
				
				/* 蓝牙指令解析状态机 */
				// 1. 粗扫描触发指令: "Scan Trigger"
				if (strncmp((char*)pReceivedPtr, "Scan", 4) == 0)
				{
					CC1310_SendString("Ready");
					osDelay(30);  
					scanFlag = 1;
					Mode_Change(scanFlag);
				}
				// 2. 细扫描步进指令: "Close"
				else if (strncmp((char*)pReceivedPtr, "Close", 5) == 0)
				{
					// 释放信号量，通知 judgementTask 里的 Case 2 开始一次 50 点采集
					osSemaphoreRelease(fineScanSemHandle);     
					// 如果当前不是细扫描模式，则切换
					if(scanFlag != 2) {
						scanFlag = 2;
						Mode_Change(scanFlag);
					}
				}    
				// 3. 环境监测指令: "Environment"
				else if (strncmp((char*)pReceivedPtr, "Env", 3) == 0)
				{
					float temp, humi;
					char msg[30];
					
					// 1. 尝试初始化 (包含供电和I2C复位)
					if (AHT20_Init() == 0) {
						// 2. 读取数据
						if (AHT20_Read_Data(&temp, &humi) == 0) {
							// 3. 格式化输出 (绕过浮点数打印限制)
							AHT20_Format_String(msg, temp, humi);
							
							// 4. 交互
							CC1310_SendString(msg);
							OLED_Clear();
							OLED_ShowString(1, 1, msg);
						} else {
							CC1310_SendString("AHT20 Read Error");
						}
					} else {
						CC1310_SendString("AHT20 Init Error");
					}

					// // 3. 彻底断电，进入极低功耗状态
					// AHT20_PowerOff();
				} 
			}
		}
		else if(activatedMember == (QueueSetMemberHandle_t)messageQueueHandle)	// 解调消息处理
		{
			if (osMessageQueueGet(messageQueueHandle, &pReceivedPtr, NULL, 0) == osOK)
			{
				Power_RefreshActivity();
				// OLED 显示 
				char *str = (char*)pReceivedPtr;
				strcpy(displayBuf, "Re:");    
				strcat(displayBuf, str);      
				OLED_Clear();
				OLED_ShowString(1, 1, displayBuf);
				/* 信息处理状态机 */
				// 1.返回环境信息
				if(strncmp((char*)pReceivedPtr, "Env", 3) == 0 && strlen((char*)pReceivedPtr) == 3)
				{
					float temp, humi;
					char msg[30];
					
					// 1. 尝试初始化 (包含供电和I2C复位)
					if (AHT20_Init() == 0) {
						// 2. 读取数据
						if (AHT20_Read_Data(&temp, &humi) == 0) {
							// 3. 格式化输出 (绕过浮点数打印限制)
							AHT20_Format_String(msg, temp, humi);
							
							// 4. 交互
							CC1310_SendString(msg);
							OLED_Clear();
							OLED_ShowString(1, 1, msg);
						} else {
							CC1310_SendString("AHT20 Read Error");
						}
					} else {
						CC1310_SendString("AHT20 Init Error");
					}
				}
				// 2.开启辅助通信模块
				else if(strncmp((char*)pReceivedPtr, "0", 1) == 0 && strlen((char*)pReceivedPtr) == 1)
				{
					CC1310_Open();
				}
				// 3.关闭辅助通信模块
				else if(strncmp((char*)pReceivedPtr, "1", 1) == 0 && strlen((char*)pReceivedPtr) == 1)
				{
					CC1310_Sleep();
				}
				// 信息处理完毕，开始接收下一条信息
				Re_ADC_Start();
			} 
		}
	}
}
/* 辅助函数，用于状态间切换时，清空队列，更改采样率 */
void Mode_Change(uint8_t sFlag)
{
	if(sFlag == 0) // 非扫描模式
	{
		__HAL_TIM_SET_PRESCALER(&htim3, TIM_PRESCARE_DATA);
	}else{				 //  扫描模式
		__HAL_TIM_SET_PRESCALER(&htim3, TIM_PRESCARE_SCAN);
	}
	HAL_TIM_GenerateEvent(&htim3, TIM_EVENTSOURCE_UPDATE);		// 立即触发一次采样
	osMessageQueueReset(judgementQueueHandle);								// 清队列
}
/* USER CODE END Application */

