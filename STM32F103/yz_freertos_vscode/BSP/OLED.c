/**
 * @file    OLED.c
 * @brief   SSD1306 OLED display driver
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "OLED.h"
#include "OLED_Font.h"
#include "gpio.h"
#include "stdbool.h"
/* 
 * 外部声明 hi2c1 
 * 如果你的 CubeMX 生成设置没有生成 i2c.h，
 * 或者 hi2c1 定义在 main.c 中，请确保能引用到它。
 */
extern I2C_HandleTypeDef hi2c1; 

/* OLED I2C 互斥量句柄 */
osMutexId_t oledMutexHandle;

// 内部宏：锁定与解锁 (为了代码整洁)
#define OLED_LOCK()    osMutexAcquire(oledMutexHandle, osWaitForever)
#define OLED_UNLOCK()  osMutexRelease(oledMutexHandle)

/* OLED I2C 地址 (写地址) */
#define OLED_I2C_ADDR  0x78

static bool OLED_OPEN = false; // 熄屏指示

/* OLED写命令 */
void OLED_WriteCommand(uint8_t Command)
{
    /* 
     * 使用 HAL_I2C_Mem_Write 直接写入
     * 0x00 是控制字节，表示接下来是命令
     */
    HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, &Command, 1, 100);
}

/* OLED写数据 */
void OLED_WriteData(uint8_t Data)
{
    /* 
     * 0x40 是控制字节，表示接下来是数据
     */
    HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, &Data, 1, 100);
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);                    //设置Y位置
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));    //设置X位置低4位
    OLED_WriteCommand(0x00 | (X & 0x0F));           //设置X位置高4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
		OLED_LOCK();
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for(i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
		OLED_UNLOCK();
}

/**
  * @brief  OLED部分清屏
  * @param  Line 行位置，范围：1~4
  * @param  start 列开始位置，范围：1~16
  * @param  end 列开始位置，范围：1~16
  * @retval 无
  */
void OLED_Clear_Part(uint8_t Line, uint8_t start, uint8_t end)
{  
		OLED_LOCK();
    uint8_t i,Column;
    for(Column = start; Column <= end; Column++)
    {
        OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);       //设置光标位置在上半部分
        for (i = 0; i < 8; i++)
        {
            OLED_WriteData(0x00);           //显示上半部分内容
        }
        OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);   //设置光标位置在下半部分
        for (i = 0; i < 8; i++)
        {
            OLED_WriteData(0x00);       //显示下半部分内容
        }
    }
		OLED_UNLOCK();
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
		OLED_LOCK();	
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);       //设置光标位置在上半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);          //显示上半部分内容
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);   //设置光标位置在下半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);      //显示下半部分内容
    }
		OLED_UNLOCK();
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
		OLED_Poweron();			// 亮屏
		OLED_LOCK();
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
		OLED_UNLOCK();
}

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
		OLED_Poweron();
    uint8_t i;
    for (i = 0; i < Length; i++)                            
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
		OLED_Poweron();
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++)                            
    {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
		OLED_Poweron();
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)                            
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10)
        {
            OLED_ShowChar(Line, Column + i, SingleNumber + '0');
        }
        else
        {
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
        }
    }
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
		OLED_Poweron();
    uint8_t i;
    for (i = 0; i < Length; i++)                            
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

/**
  * @brief  OLED初始化
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{
		/* 1. 创建递归互斥量 */
    const osMutexAttr_t Thread_Mutex_attr = {
				"oledMutex",                          // 名称
				osMutexRecursive | osMutexPrioInherit, // 递归模式 + 优先级继承
				NULL, 0
    };
		if(!oledMutexHandle) oledMutexHandle = osMutexNew(&Thread_Mutex_attr);
    
    /* 2. 写入初始化命令序列 */
		OLED_LOCK();
		
    OLED_WriteCommand(0xAE);    // 关闭显示
    OLED_WriteCommand(0xD5);    // 设置时钟分频
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8);    // 多路复用
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3);    // 偏移
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40);    // 开始行
    OLED_WriteCommand(0xA1);    // 左右方向
    OLED_WriteCommand(0xC8);    // 上下方向
    OLED_WriteCommand(0xDA);    // 硬件配置
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81);    // 对比度
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9);    // 预充电
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB);    // VCOMH
    OLED_WriteCommand(0x30);
    OLED_WriteCommand(0xA4);    // 全显开关
    OLED_WriteCommand(0xA6);    // 反显
		OLED_Poweroff();						// 按需打开屏幕
        
    /* 3. 清屏 (如果这一步成功，屏幕就会变黑，不再显示旧数字) */
    OLED_Clear(); 
		OLED_UNLOCK();
}

/**
  * @brief  OLED进入休眠模式 (省电)
  * @note   此模式下电荷泵关闭，屏幕完全熄灭，功耗极低
  */
void OLED_Poweroff(void)
{
	if(OLED_OPEN)
	{
		OLED_LOCK();
    
    OLED_WriteCommand(0xAE);    // 1. 关闭显示 (Set Display OFF)
    
    // 2. 关闭电荷泵 (Charge Pump Control)
    // 这是最省电的关键一步
    OLED_WriteCommand(0x8D);    
    OLED_WriteCommand(0x10);    // 0x10 为关闭，0x14 为开启
    
		OLED_OPEN = false;
    OLED_UNLOCK();
	}
}

/**
  * @brief  OLED从休眠中唤醒
  */
void OLED_Poweron(void)
{
	if(!OLED_OPEN)
	{
		OLED_LOCK();
    
    // 1. 开启电荷泵
    OLED_WriteCommand(0x8D);    
    OLED_WriteCommand(0x14);    
    
    // 2. 开启显示
    OLED_WriteCommand(0xAF);    
    
		OLED_OPEN = true;
    OLED_UNLOCK();
	}
}

/**
  * @brief  OLED物理掉电
  * @note   直接切断OLED供电，极致省电
  */
void OLED_Hardware_PowerOff(void) {
    // 1. 先发指令让屏幕休息（优雅关断）
    OLED_Poweroff();
    
    // 2. 将所有引脚（VCC, SCL, SDA）强制配置为推挽输出低电平
    // 这能把 OLED 的电容余电迅速放掉
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // 3. 真正拉低电平
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
    
    // 4. 稍微延时，确保放电完成
    osDelay(100); 
    
    // 6. 进入 STOP 模式前，将它们全部设为模拟输入以防漏电
//    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
//    GPIO_InitStruct.Pull = GPIO_NOPULL;
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
	* @brief  OLED物理重新上电
  */
void OLED_Hardware_PowerOn(void) {
    // 1. 恢复 PB5 (VCC) 为高电平
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    
    // 2. 延时给电容充电，等待屏幕稳定
    HAL_Delay(50); 
    
    // 3. 重新恢复 I2C 引脚功能（调用之前的 I2C 初始化）
    MX_I2C1_Init(); 
    
    // 4. 彻底重新初始化 OLED（因为硬件断电后寄存器全丢了）
    OLED_Init(); 
}
