#ifndef __RECEIVER_ADC_H
#define __RECEIVER_ADC_H

#include "adc.h"  
#include "tim.h"
#include "cmsis_os2.h"

#define ADC_BUF_LENGTH 1
#define ADC_FREQ 24000000/200/60
#define ADC_QUEUE_LENGTH_MAX 10

struct ADC_Queue{
	osMessageQueueId_t QueueHandle[ADC_QUEUE_LENGTH_MAX];
	uint8_t QueueNum;
};
void Re_ADC_Init(void);
void Re_ADC_Start(void);
void Re_ADC_Stop(void);
void Re_ADC_Queue_Register(osMessageQueueId_t QueueHandle);

#endif
