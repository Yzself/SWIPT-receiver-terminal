#ifndef __GLOBAL_DEFINE_H
#define __GLOBAL_DEFINE_H
/**
 * @file    Global_Define.h
 * @brief   Global constants, parameters and shared declarations
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os.h"

#define POWER_MANAGER_ENABLE    0               // 1: enable, 0: disable (debug)

/* System */
#define SYSFREQ                 24000000        // system clock (Hz)

/* ADC / Sampling */
#define SAMPREF                 3.3f            // ADC reference voltage (V)
#define SAMPRES                 4095            // ADC resolution (12-bit)
#define SAMPFREQ_DATA           (500 * 4)       // sampling rate in demod mode (sps)
#define SAMPFREQ_SCAN           (100 * 50)      // sampling rate in scan mode (sps)
#define TIM_AUTORELOAD          (60 - 1)        // timer auto-reload value
#define TIM_PRESCARE_DATA       (SYSFREQ / (TIM_AUTORELOAD + 1) / SAMPFREQ_DATA - 1)
#define TIM_PRESCARE_SCAN       (SYSFREQ / (TIM_AUTORELOAD + 1) / SAMPFREQ_SCAN - 1)

/* Preprocessing (matched filter + moving average) */
#define MATCHED_LEN             17              // matched filter coefficient count
#define AVERA_LEN               256             // moving average window size

/* Gardner timing recovery */
#define LFC                     0.0267f         // loop filter coefficient

/* Frame */
#define FRAME_HEADER            0xFDC8E92E      // fixed frame sync word
#define MAX_FRAME_PAYLOAD       64              // max payload length (bytes)
#define HANMING_LEN             14              // Hamming code word length (bits)

/* Coarse beam scan */
#define THETA_COARSE_MAX        45
#define PHI_COARSE_MAX          360
#define THETA_COARSE_GAP        1
#define PHI_COARSE_GAP          3
#define BEAM_COARSE_SCAN_NUM    ((THETA_COARSE_MAX + 1) * (PHI_COARSE_MAX) / (THETA_COARSE_GAP) / (PHI_COARSE_GAP))
#define THETA_COARSE_NUM        ((THETA_COARSE_MAX) / (THETA_COARSE_GAP) + 1)
#define PHI_COARSE_NUM          ((PHI_COARSE_MAX) / (PHI_COARSE_GAP))

/* Fine beam scan */
#define THETA_CLOSE_RANGE       3
#define PHI_CLOSE_RANGE         360
#define THETA_CLOSE_GAP         1
#define PHI_CLOSE_GAP           1
#define BEAM_CLOSE_SCAN_NUM     ((THETA_CLOSE_RANGE) * (PHI_CLOSE_RANGE) / (THETA_CLOSE_GAP) / (PHI_CLOSE_GAP))
#define THETA_CLOSE_NUM         ((THETA_CLOSE_RANGE) / (THETA_CLOSE_GAP))
#define PHI_CLOSE_NUM           ((PHI_CLOSE_RANGE) / (PHI_CLOSE_GAP))

extern volatile uint8_t scanFlag;               // 0: demod, 1: coarse scan, 2: fine scan
extern void Mode_Change(uint8_t sFlag);         // switch sampling rate and flush ADC queue

#endif
