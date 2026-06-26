#ifndef __PREPROCESSING_H
#define __PREPROCESSING_H

/**
 * @file    Preprocessing.h
 * @brief   Matched filter and moving average filter interface
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "Global_Define.h"
#include "main.h"

// Æ¥ÅäÂË²¨Æ÷
float Match_Filter(float Ad_Voltage);
// »¬¶¯Æ½¾ùÂË²¨Æ÷
float Average_Filter(float Matched_Voltage);

#endif
