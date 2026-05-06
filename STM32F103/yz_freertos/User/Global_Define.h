#ifndef __GLOBAL_DEFINE_H
#define __GLOBAL_DEFINE_H

#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os.h"

#define 				SYSFREQ									24000000			 //主时钟
// 采样相关
#define         SAMPREF                 3.3            //采样参考电压
#define 				SAMPRES									4095					 //采样分辨率
#define 				SAMPFREQ_DATA						(500 * 4)			 //在接收模式下的采样率
#define 				SAMPFREQ_SCAN						(100 * 50)		 //在扫描模式下的采样率
#define 				TIM_AUTORELOAD					(60 - 1)			 //保持自动重装值不变
#define 				TIM_PRESCARE_DATA				(SYSFREQ/(TIM_AUTORELOAD + 1)/SAMPFREQ_DATA - 1)		 
#define 				TIM_PRESCARE_SCAN				(SYSFREQ/(TIM_AUTORELOAD + 1)/SAMPFREQ_SCAN - 1)	
// 预处理相关
#define         MATCHED_LEN      				17             //匹配滤波器系数长度
#define         AVERA_LEN               256            //滑动平均数组长度
// Gardner相关
#define         LFC                     0.0267         //环路增益系数，决定了锁定速度。
// Frame相关
#define					FRAME_HEADER						0xFDC8E92E		 //固定帧头
#define 				MAX_FRAME_PAYLOAD 			64 						 //最大帧长
#define  				HANMING_LEN							14						 //汉明码长度
//波束粗扫描角度定义
#define         THETA_COARSE_MAX        45
#define         PHI_COARSE_MAX          360
#define         THETA_COARSE_GAP        1
#define         PHI_COARSE_GAP          3
#define         BEAM_COARSE_SCAN_NUM    ((THETA_COARSE_MAX+1)*(PHI_COARSE_MAX)/(THETA_COARSE_GAP)/(PHI_COARSE_GAP))
#define         THETA_COARSE_NUM        ((THETA_COARSE_MAX)/(THETA_COARSE_GAP)+1)
#define         PHI_COARSE_NUM          ((PHI_COARSE_MAX)/(PHI_COARSE_GAP))
//波束细扫描角度定义
#define         THETA_CLOSE_RANGE       3
#define         PHI_CLOSE_RANGE         360
#define         THETA_CLOSE_GAP         1
#define         PHI_CLOSE_GAP           1
#define         BEAM_CLOSE_SCAN_NUM     ((THETA_CLOSE_RANGE)*(PHI_CLOSE_RANGE)/(THETA_CLOSE_GAP)/(PHI_CLOSE_GAP))
#define         THETA_CLOSE_NUM         ((THETA_CLOSE_RANGE)/(THETA_CLOSE_GAP))
#define         PHI_CLOSE_NUM           ((PHI_CLOSE_RANGE)/(PHI_CLOSE_GAP))

extern volatile uint8_t scanFlag;					// 0：正常判决 1：粗扫描 2：细扫描
extern void Mode_Change(uint8_t sFlag);		// 模式切换时调用

#endif
