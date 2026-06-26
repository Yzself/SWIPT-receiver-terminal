#ifndef __FRAMEPROCESSING_H
#define __FRAMEPROCESSING_H

/**
 * @file    FrameProcessing.h
 * @brief   Frame synchronization and data extraction interface
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "Global_Define.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

bool FrameProcessing(uint8_t bit, uint8_t *reMessage);

#endif
