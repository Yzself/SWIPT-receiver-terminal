#include "DS18B20.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Global_Define.h"
#include <stdio.h>

/* 
 * 强制要求：
 * 1. PB0 必须外接 4.7k 欧姆上拉电阻。
 * 2. 系统主频 SYSFREQ 必须准确（24000000）。
 */

// --- 极速寄存器控制 (针对 F103 PB0) ---
static inline void DQ_Mode_Out(void) {
    GPIOB->CRL &= ~(0x0F << (0 * 4)); 
    GPIOB->CRL |=  (0x03 << (0 * 4)); // 推挽输出 50MHz
}

static inline void DQ_Mode_In(void) {
    GPIOB->CRL &= ~(0x0F << (0 * 4));
    GPIOB->CRL |=  (0x08 << (0 * 4)); // 上拉输入
    GPIOB->ODR |= GPIO_PIN_0; 
}

#define DQ_Write(x)  (x ? (GPIOB->BSRR = GPIO_PIN_0) : (GPIOB->BRR = GPIO_PIN_0))
#define DQ_Read()    ((GPIOB->IDR & GPIO_PIN_0) ? 1 : 0)

// --- 精确延时 (24MHz 下已考虑函数开销) ---
static void Delay_us(uint32_t us) {
    if (us == 0) return;
    uint32_t ticks = us * (SYSFREQ / 1000000);
    uint32_t tcnt = 0;
    uint32_t told = SysTick->VAL;
    uint32_t tnow;
    uint32_t reload = SysTick->LOAD;
    while (1) {
        tnow = SysTick->VAL;
        if (tnow != told) {
            if (tnow < told) tcnt += told - tnow;
            else tcnt += reload - tnow + told;
            told = tnow;
            if (tcnt >= ticks) break;
        }
    }
}

// --- 1-Wire 协议底层 ---

uint8_t DS18B20_Init(void) {
    uint8_t response = 1;
    
    HAL_GPIO_WritePin(GPIOB, DS_VCC_PIN, GPIO_PIN_SET); // 供电
    // 增加上电延时
    if (osKernelGetState() == osKernelRunning) osDelay(200); else HAL_Delay(200);

    taskENTER_CRITICAL(); // 开启临界区

    DQ_Mode_Out();
    DQ_Write(0);     
    Delay_us(500);   // 复位脉冲
    
    DQ_Write(1);     
    Delay_us(60);    // 释放后等待 60us

    DQ_Mode_In();    
    // 传感器响应 60-240us
    if(DQ_Read() == 0) {
        response = 0; 
    }
    
    taskEXIT_CRITICAL(); // 退出临界区

    Delay_us(400);   
    return response;
}

static void DS18B20_Write_Bit(uint8_t bit) {
    taskENTER_CRITICAL();
    DQ_Mode_Out();
    if (bit) {
        DQ_Write(0); Delay_us(2);   // 写1: 拉低 2us
        DQ_Write(1); Delay_us(60);
    } else {
        DQ_Write(0); Delay_us(65);  // 写0: 拉低 65us
        DQ_Write(1); Delay_us(2);
    }
    taskEXIT_CRITICAL();
}

static uint8_t DS18B20_Read_Bit(void) {
    uint8_t bit;
    taskENTER_CRITICAL();
    
    DQ_Mode_Out();
    DQ_Write(0); 
    Delay_us(2);      // 拉低 2us
    DQ_Write(1);      // 释放总线
    
    DQ_Mode_In();     // 在 24MHz 下，执行这行函数和寄存器切换大约需要 5-8us
    
    // 【关键】在 24MHz 下不额外 Delay，此时刚好处于拉低后的 10-12us
    bit = DQ_Read();  
    
    taskEXIT_CRITICAL();
    Delay_us(50);     // 完成 60us 周期
    return bit;
}

static void DS18B20_Write_Byte(uint8_t dat) {
    for (uint8_t i = 0; i < 8; i++) {
        DS18B20_Write_Bit(dat & 0x01);
        dat >>= 1;
    }
}

static uint8_t DS18B20_Read_Byte(void) {
    uint8_t dat = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (DS18B20_Read_Bit()) dat |= (1 << i);
    }
    return dat;
}

// --- 高级应用 ---

short DS18B20_Get_Temp(void) {
    uint8_t TL, TH;
    int16_t temp_raw;
    
    // 1. 发起转换
    if(DS18B20_Init() != 0) return -999;
    DS18B20_Write_Byte(0xcc); 
    DS18B20_Write_Byte(0x44); 
    
    // 2. 转换等待：12位需要 750ms。确保电源引脚在延时期间一直为高
    HAL_GPIO_WritePin(GPIOB, DS_VCC_PIN, GPIO_PIN_SET); 
    vTaskDelay(pdMS_TO_TICKS(750)); 
    
    // 3. 读取 RAM
    if(DS18B20_Init() != 0) return -999;
    DS18B20_Write_Byte(0xcc);
    DS18B20_Write_Byte(0xbe); 
    
    TL = DS18B20_Read_Byte();
    TH = DS18B20_Read_Byte();
    
    temp_raw = (TH << 8) | TL;
    
    // 正常返回 10 倍值
    return (short)((float)temp_raw * 0.625f);
}

void TEM_Num_to_char(short Temp, char *Tem) {
    uint8_t is_neg = 0;
    if (Temp < 0) { Temp = -Temp; is_neg = 1; }
    sprintf(Tem, "%c%d.%d", is_neg ? '-' : '+', Temp / 10, Temp % 10);
}

