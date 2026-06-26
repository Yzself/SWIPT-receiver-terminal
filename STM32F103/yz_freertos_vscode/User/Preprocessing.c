/**
 * @file    Preprocessing.c
 * @brief   Matched filter and moving average filter
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "Preprocessing.h"


/* 匹配滤波器相关定义 */
static uint8_t MatchedIndex = 0;								// 匹配滤波索引
static float MatchedArray[MATCHED_LEN] = {0.0};	// 匹配滤波Buf
static const float Coef_Matched_Filter[(MATCHED_LEN+1)/2] =
{	-0.01010600, 0.0, 0.01819081, \
	0.0, -0.04244521, 0.0, \
	0.21222607, 0.50004589, 0.63667820 }; 				// 匹配滤波器系数
/* 滑动平均滤波器相关定义 */
static float Average_Array[AVERA_LEN] = {0.0};  // 滑动平均数组
static float sum_average = 0.0;									// 滑动数组总和
static uint8_t Average_Index = AVERA_LEN - 1;             // 滑动平均数组索引


float Match_Filter(float Ad_Voltage)
{
	float MatchedOut = 0.0;
	
	if(MatchedIndex != MATCHED_LEN)
	{
			MatchedArray[MatchedIndex] = Ad_Voltage;
			MatchedIndex ++;
	}else{
			for(uint8_t i = 1; i < MATCHED_LEN;i++)
			{
				MatchedArray[i-1] = MatchedArray[i];
			}
			MatchedArray[MATCHED_LEN - 1] = Ad_Voltage;
	}

	MatchedOut = (MatchedArray[0]+MatchedArray[16])*Coef_Matched_Filter[0]+\
		(MatchedArray[2]+MatchedArray[14])*Coef_Matched_Filter[2]+\
		(MatchedArray[4]+MatchedArray[12])*Coef_Matched_Filter[4]+\
		(MatchedArray[6]+MatchedArray[10])*Coef_Matched_Filter[6]+\
		(MatchedArray[7]+MatchedArray[9])*Coef_Matched_Filter[7]+\
		 MatchedArray[8]*Coef_Matched_Filter[8];

	return MatchedOut;
}

float Average_Filter(float Matched_Voltage)
{
	float AverageOut = 0.0;
	float temp_old = 0.0;		// 最旧值保存
	
	temp_old = Average_Array[Average_Index];
  	Average_Array[Average_Index] = Matched_Voltage;
  	sum_average = sum_average - temp_old + Average_Array[Average_Index];
  	AverageOut = Average_Array[Average_Index] - sum_average/AVERA_LEN;

	if(Average_Index != 0)
	{
		Average_Index--;
	}
	else
	{
		Average_Index = AVERA_LEN - 1; 
	}

	return AverageOut;
}

