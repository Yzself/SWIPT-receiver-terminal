#ifndef __AHT20_H
#define __AHT20_H

/**
 * @file    AHT20.h
 * @brief   AHT20 temperature and humidity sensor interface
 * @author  YuZhang Lin
 * @date    2026-06-26
 */


#include "main.h"

// AHT20 I2C 7位地址为 0x38，HAL库需要左移一位
#define AHT20_ADDR        (0x38 << 1)

// 指令集
#define AHT20_CMD_INIT    0xBE
#define AHT20_CMD_MEASURE 0xAC
#define AHT20_CMD_RESET   0xBA

// 引脚配置 (请确保在CubeMX中 PB1 为 GPIO_Output)
#define AHT_VCC_PORT      GPIOB
#define AHT_VCC_PIN       GPIO_PIN_1

// 函数声明
uint8_t AHT20_Init(void);
uint8_t AHT20_Read_Data(float *Temperature, float *Humidity);
void    AHT20_PowerOff(void);
void    AHT20_Format_String(char *buf, float temp, float humi);

#endif
