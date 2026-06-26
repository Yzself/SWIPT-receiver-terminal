/**
 * @file    SubGHz_CC1310.c
 * @brief   E70-433T CC1310 Sub-GHz module driver
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "SubGHz_CC1310.h"

#define CC1310_AUX_READ() HAL_GPIO_ReadPin(AUX_GPIO_Port, AUX_Pin)

/* RX buffer */
#define CC1310_RX_BUF_SIZE  64
static uint8_t  cc1310_rx_buf[CC1310_RX_BUF_SIZE];
static uint8_t  cc1310_rx_idx;
static uint8_t  cc1310_rx_byte;

/* Registered message queue */
static osMessageQueueId_t cc1310_queue;
static osMutexId_t        cc1310_mutex;

/* Module open state */
static bool CC1310_OPEN = false;

/**
*   @brief Wait for AUX pin to go high, with timeout
*   @param timeout_ms Timeout in milliseconds
*   @return true if AUX went high, false if timeout occurred
*/
static bool CC1310_WaitAux(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while (CC1310_AUX_READ() == GPIO_PIN_RESET) {
        if (HAL_GetTick() - start >= timeout_ms)
            return false;
        osDelay(1);
    }
    return true;
}

/**
*   @brief Set the mode of the CC1310 module
*   @param mode The mode to set (CC1310_Mode_t)
*/
void CC1310_SetMode(CC1310_Mode_t mode)
{
    /* Per manual: mode switch valid only when AUX=1 */
    CC1310_WaitAux(200);

    HAL_GPIO_WritePin(M0_GPIO_Port, M0_Pin,
        (mode & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(M1_GPIO_Port, M1_Pin,
        (mode & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(M2_GPIO_Port, M2_Pin,
        (mode & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* Per manual: after AUX rising edge, wait 2ms for mode to take effect */
    CC1310_WaitAux(200);
    osDelay(2);
}

/**
*   @brief Initialize the CC1310 module
*/
void CC1310_Init(void)
{
    cc1310_mutex = osMutexNew(NULL);
}

/**
*   @brief Register a message queue for receiving messages from the CC1310 module
*   @param queue The message queue to register
*/
void CC1310_Queue_Register(osMessageQueueId_t queue)
{
    cc1310_queue = queue;
}

/**
*   @brief Put the CC1310 module to run mode (continuous transparent mode)
*/
void CC1310_Open(void)
{
    if(CC1310_OPEN)
        return;

    /* Create mutex if not already created */
    if (cc1310_mutex == NULL)
        cc1310_mutex = osMutexNew(NULL);

    /* Enter continuous transparent mode */
    CC1310_SetMode(CC1310_MODE_CONTINUOUS);

    /* Flush RX state and start interrupt receive */
    cc1310_rx_idx = 0;
    HAL_UART_Receive_IT(&huart1, &cc1310_rx_byte, 1);

    CC1310_OPEN = true;
}

/**
*   @brief Put the CC1310 module to sleep mode (low power)
*/
void CC1310_Sleep(void)
{
    if(!CC1310_OPEN)
        return;

    /* Abort any ongoing UART transfer/receive */
    while (huart1.gState != HAL_UART_STATE_READY);
    HAL_UART_AbortReceive(&huart1);

    /* Enter deep sleep (Mode 7) */
    CC1310_SetMode(CC1310_MODE_SLEEP);

    /* Flush local state */
    cc1310_rx_idx = 0;
    if (cc1310_mutex != NULL) {
        osMutexRelease(cc1310_mutex);
    }
    
    CC1310_OPEN = false;
}

/**
*   @brief Send a string through the CC1310 module
*   @param str The string to send
*/
void CC1310_SendString(char *str)
{
    if (!CC1310_WaitAux(100)) return;

    osMutexAcquire(cc1310_mutex, osWaitForever);
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), 100);
    HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", 2, 100);
    osMutexRelease(cc1310_mutex);
}

/**
*   @brief Send a formatted string through the CC1310 module
*   @param format The format string
*   @param ... The arguments for the format string
*/
void CC1310_SendPrintf(const char *format, ...)
{
    if (!CC1310_WaitAux(100)) return;

    va_list args;
    va_start(args, format);

    osMutexAcquire(cc1310_mutex, osWaitForever);
    char buf[128];
    int len = vsnprintf(buf, sizeof(buf), format, args);
    if (len > 0 && len < (int)sizeof(buf) - 2) {
        buf[len++] = '\r';
        buf[len++] = '\n';
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100);
    }
    osMutexRelease(cc1310_mutex);

    va_end(args);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1)
        return;

    cc1310_rx_buf[cc1310_rx_idx] = cc1310_rx_byte;

    if (cc1310_rx_idx >= 1
        && cc1310_rx_buf[cc1310_rx_idx - 1] == '\r'
        && cc1310_rx_buf[cc1310_rx_idx] == '\n')
    {
        Power_RefreshActivity();
        cc1310_rx_buf[cc1310_rx_idx - 1] = '\0';

        uint8_t *pCmd = cc1310_rx_buf;
        if (cc1310_queue != NULL)
            osMessageQueuePut(cc1310_queue, &pCmd, 0, 0);

        cc1310_rx_idx = 0;
    }
    else
    {
        cc1310_rx_idx++;
        if (cc1310_rx_idx >= CC1310_RX_BUF_SIZE) {
            cc1310_rx_idx = 0;
        }
    }

    HAL_UART_Receive_IT(&huart1, &cc1310_rx_byte, 1);
}
