#ifndef __DECODER_H
#define __DECODER_H

/**
 * @file    Decoder.h
 * @brief   Hamming(7,4) decoder interface
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "Global_Define.h"
#include <stdint.h>

void HammingDecoder(uint16_t const HammingIn, uint8_t *HammingOut);

#endif
