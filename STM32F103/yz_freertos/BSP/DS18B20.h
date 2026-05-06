#ifndef __DS18B20_H
#define __DS18B20_H

#include "main.h"

/* 引脚定义：请确保是 GPIOB 和 PIN 0 */
#define DS_PORT       GPIOB
#define DS_DQ_PIN     GPIO_PIN_0
#define DS_VCC_PIN    GPIO_PIN_1

/* 函数声明 */
uint8_t DS18B20_Init(void);
short   DS18B20_Get_Temp(void);
void    TEM_Num_to_char(short Temp, char *Tem);

#endif
