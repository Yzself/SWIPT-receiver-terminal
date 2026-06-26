#ifndef __OLED_H
#define __OLED_H

/**
 * @file    OLED.h
 * @brief   SSD1306 OLED display interface
 * @author  YuZhang Lin
 * @date    2026-06-26
 */


#include "main.h"
#include "i2c.h"  
#include "cmsis_os2.h"

void OLED_Init(void);
void OLED_Hardware_PowerOn(void);			
void OLED_Hardware_PowerOff(void);
void OLED_Poweroff(void);
void OLED_Poweron(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif
