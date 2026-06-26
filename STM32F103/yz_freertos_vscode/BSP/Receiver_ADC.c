/**
 * @file    Receiver_ADC.c
 * @brief   ADC receiver with TIM-triggered DMA and queue
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "Receiver_ADC.h"

volatile static uint16_t adc_buf;
volatile static struct ADC_Queue adcQueue = {0};
void Re_ADC_Start(void)
{
	HAL_TIM_Base_Start(&htim3);
	for(uint8_t i = 0;i < adcQueue.QueueNum;i++)
	{
		/* 清空队列 */
		osMessageQueueReset(adcQueue.QueueHandle[i]);
	}
}

void Re_ADC_Stop(void)
{
	HAL_TIM_Base_Stop(&htim3);
	for(uint8_t i = 0;i < adcQueue.QueueNum;i++)
	{
		/* 清空队列 */
		osMessageQueueReset(adcQueue.QueueHandle[i]);
	}
}

void Re_ADC_Init(void)
{
	HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_buf, ADC_BUF_LENGTH);
	Re_ADC_Start();
}

void Re_ADC_Queue_Register(osMessageQueueId_t QueueHandle)
{
	if(adcQueue.QueueNum < ADC_QUEUE_LENGTH_MAX)
	{
		adcQueue.QueueHandle[adcQueue.QueueNum] = QueueHandle;
		adcQueue.QueueNum ++;
	}
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if(hadc->Instance == ADC1)
	{
		if(adcQueue.QueueNum > 0)
		{
			uint16_t val = adc_buf;
			for(uint8_t i = 0;i < adcQueue.QueueNum;i++)
			{
				/* 写入队列 */
        		osMessageQueuePut(adcQueue.QueueHandle[i], &val, 0, 0);
			}
		}
	}
}
