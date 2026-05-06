#include "Bluetooth.h"
#include "usart.h"
#include "PowerManager.h"
#include "Global_Define.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* 蓝牙相关定义 */
// 接收
#define BT_RX_BUF_SIZE  64
static uint8_t BT_Data[BT_RX_BUF_SIZE];
static uint8_t BT_Data_Index = 0;
static uint8_t bt_rx_byte;  										// 中断接收暂存字节
static uint8_t g_BT_RawCmdBuf[BT_RX_BUF_SIZE]; 				        // 专门用于存放蓝牙原始指令的全局缓存
extern osMessageQueueId_t blueToothQueueHandle;	                    // 蓝牙消息队列

// 发送
#define BT_TX_BUF_SIZE 256
static uint8_t bt_tx_buffer[BT_TX_BUF_SIZE];

// 蓝牙上电
void BT_Init(void) {
    // 启动第一次中断接收 (接收 1 个字节)
    HAL_UART_Receive_IT(&huart1, &bt_rx_byte, 1);
}

void BT_Open(void) {
		// 软件复位：清空之前残留的索引和缓冲区，防止上电杂波误识别
    BT_Data_Index = 0;
    memset(BT_Data, 0, BT_RX_BUF_SIZE);
    
		// PA11 设为输出
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET); 
    HAL_Delay(500); // 等待上电稳定
	
    BT_Init();
}

void BT_Off(void) {
    // 1. 安全检查：如果 DMA 正在发送数据，等待发送完成再断电
    // 防止发送到一半模块掉电，导致接收端收到乱码
    while (huart1.gState != HAL_UART_STATE_READY);
    
    // 2. 停止接收：安全关闭中断接收任务
    // Abort 会清理内部状态机，防止下次开启时逻辑错乱
    HAL_UART_AbortReceive(&huart1);
    
    // 3. 硬件掉电 (PA11)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    
    // 4. 清理缓存状态
    BT_Data_Index = 0;
		memset(BT_Data, 0, BT_RX_BUF_SIZE);
}

void BT_SendChar_DMA(char c) {
    // 等待上一次 DMA 发送完成
    while (huart1.gState != HAL_UART_STATE_READY);

    // 填充数据：字符 + \r + \n
    bt_tx_buffer[0] = (uint8_t)c;
    bt_tx_buffer[1] = '\r';
    bt_tx_buffer[2] = '\n';

    // 开启 DMA 发送
    HAL_UART_Transmit_DMA(&huart1, bt_tx_buffer, 3);
}

void BT_SendString_DMA(char *str) {
    // 等待上一次 DMA 发送完成
    while (huart1.gState != HAL_UART_STATE_READY);

    // 格式化到缓冲区，限制长度防止溢出
    // snprintf 会自动在结尾补 \0，但我们发送时不包括 \0
    int len = snprintf((char*)bt_tx_buffer, BT_TX_BUF_SIZE, "%s\r\n", str);

    if (len > 0) {
        HAL_UART_Transmit_DMA(&huart1, bt_tx_buffer, len);
    }
}

void BT_SendPrintf_DMA(const char *format, ...) {
    // 等待上一次 DMA 发送完成
    while (huart1.gState != HAL_UART_STATE_READY);

    va_list args;
    va_start(args, format);

    // 将格式化内容写入缓冲区
    int len = vsnprintf((char*)bt_tx_buffer, BT_TX_BUF_SIZE, format, args);
    
    va_end(args);

    // 手动追加换行符（如果缓冲区还有空间）
    if (len > 0 && len < BT_TX_BUF_SIZE - 2) {
        bt_tx_buffer[len++] = '\r';
        bt_tx_buffer[len++] = '\n';
        
        HAL_UART_Transmit_DMA(&huart1, bt_tx_buffer, len);
    }
}

// 蓝牙接收中断(用户可在此修改逻辑)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // 1. 将收到的字节存入缓存
        BT_Data[BT_Data_Index] = bt_rx_byte;

        // 2. 检查是否收到指令结束标志 \r\n (索引至少为1才能检查倒数位)
        if (BT_Data_Index >= 1 && BT_Data[BT_Data_Index - 1] == '\r' && BT_Data[BT_Data_Index] == '\n')
        {
			Power_RefreshActivity();
			// 1. 将收到的原始指令拷贝到专用缓存，并补齐结束符
			BT_Data[BT_Data_Index - 1] = '\0';
            memcpy(g_BT_RawCmdBuf, BT_Data, BT_Data_Index);
            // 2. 发送原始指令的指针到蓝牙消息队列
            uint8_t *pCmd = g_BT_RawCmdBuf;
            osMessageQueuePut(blueToothQueueHandle, &pCmd, 0, 0);
            // 3. 关键：处理完指令后，重置索引并清空缓存
            BT_Data_Index = 0;
            memset(BT_Data, 0, BT_RX_BUF_SIZE);
        }
        else
        {
            // 如果没收到 \r\n，则索引累加
            BT_Data_Index++;

            // 缓冲区溢出保护：如果溢出了还没收到结尾，强制重置
            if (BT_Data_Index >= BT_RX_BUF_SIZE)
            {
                BT_Data_Index = 0;
                memset(BT_Data, 0, BT_RX_BUF_SIZE);
            }
        }

        // 重要：重新开启下一次 1 字节中断接收
        HAL_UART_Receive_IT(&huart1, &bt_rx_byte, 1);
    }
}
