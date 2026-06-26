#ifndef __GARDNER_H
#define __GARDNER_H

/**
 * @file    Gardner.h
 * @brief   Gardner timing recovery interface
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "main.h"
#include "Global_Define.h"

typedef struct Gardner_Struct                             //布尔控制状态定义
{
	bool  HD_Trigger;      // HeadDetect Trigger:true代表成功判决，可以开启后续帧工作.
	uint8_t JudgeDat;			 // 判决结果:0或1.
}GardnerStruct;

void Gardner(float voltage_Ts, GardnerStruct *GStruct);

#endif
