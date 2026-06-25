#ifndef __SUBGHZ_CC1310_H
#define __SUBGHZ_CC1310_H

#include "cmsis_os2.h"
#include "usart.h"
#include "main.h"
#include "PowerManager.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

/* E70-433T CC1310 module: 8 modes selected by M2-M1-M0 (3-bit binary)
   Mode value = (M2<<2) | (M1<<1) | M0 */
typedef enum {
    CC1310_MODE_RSSI         = 0,   /* 000  RSSI, UART open, wireless closed */
    CC1310_MODE_CONTINUOUS   = 1,   /* 001  Transparent continuous transmission */
    CC1310_MODE_SUBPACKAGE   = 2,   /* 010  Transparent sub-packaged transmission */
    CC1310_MODE_CONFIG       = 3,   /* 011  AT config, 9600 8N1 fixed */
    CC1310_MODE_WOR_TX       = 4,   /* 100  WOR transmit (preamble), no RX */
    CC1310_MODE_CONFIG2      = 5,   /* 101  AT config 2, 9600 8N1 fixed */
    CC1310_MODE_WOR_RX       = 6,   /* 110  Power-saving WOR polling receive */
    CC1310_MODE_SLEEP        = 7,   /* 111  Deep sleep, all off */
} CC1310_Mode_t;

void CC1310_Init(void);
void CC1310_Open(void);
void CC1310_Sleep(void);
void CC1310_SetMode(CC1310_Mode_t mode);
void CC1310_SendString(char *str);
void CC1310_SendPrintf(const char *format, ...);
void CC1310_Queue_Register(osMessageQueueId_t queue);

#endif
