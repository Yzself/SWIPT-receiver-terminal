#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

void BT_Init(void);
void BT_Open(void);
void BT_Off(void);
void BT_SendChar_DMA(char c);
void BT_SendString_DMA(char *str);
void BT_SendPrintf_DMA(const char *format, ...);

#endif
