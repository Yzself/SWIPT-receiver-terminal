/**
 * @file    Uart_Printf.c
 * @brief   UART2 printf redirection with FreeRTOS mutex
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "Uart_Printf.h"
#include "usart.h"
#include "cmsis_os2.h"
#include <errno.h>

static osMutexId_t printf_mutex;

int _write(int file, char *ptr, int len)
{
    (void)file;
    osMutexAcquire(printf_mutex, osWaitForever);
    if (HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 100) != HAL_OK) {
        osMutexRelease(printf_mutex);
        errno = EIO;
        return -1;
    }
    osMutexRelease(printf_mutex);
    return len;
}

void Uart_Printf_Init(void)
{
    printf_mutex = osMutexNew(NULL);
}
