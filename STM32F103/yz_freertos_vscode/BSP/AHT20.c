#include "AHT20.h"
#include "i2c.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c2;

/**
 * @brief  AHT20 强力初始化
 * @note   解决了 STM32F1 硬件 I2C 容易锁死在 Busy 状态的问题
 */
uint8_t AHT20_Init(void) {
    uint8_t status_byte;
    uint8_t cmd[3];

    // 1. 供电：PB1 拉高
    HAL_GPIO_WritePin(AHT_VCC_PORT, AHT_VCC_PIN, GPIO_PIN_SET);
    // 给 C15 电容充电并等待 AHT20 内部固件启动 (说明书要求 >40ms)
    HAL_Delay(100); 

    // 2. 【核心补丁】彻底重置 I2C 硬件模块，消除 BUSY 位死锁
    __HAL_RCC_I2C2_FORCE_RESET();
    HAL_Delay(5);
    __HAL_RCC_I2C2_RELEASE_RESET();

    // 3. 重新配置硬件 I2C2 寄存器 (由 CubeMX 生成的代码)
    MX_I2C2_Init();

    // 4. 检查设备是否存在 (尝试握手)
    if (HAL_I2C_IsDeviceReady(&hi2c2, AHT20_ADDR, 5, 100) != HAL_OK) {
        return 1; 
    }

    // 5. 读取状态字，检查校准位 (Bit[3])
    if (HAL_I2C_Master_Receive(&hi2c2, AHT20_ADDR, &status_byte, 1, 100) != HAL_OK) {
        return 1;
    }

    // 说明书第11页：如果状态字 Bit[3] 为 0，说明未校准，需要初始化
    if ((status_byte & 0x08) == 0x00) {
        cmd[0] = AHT20_CMD_INIT;
        cmd[1] = 0x08;
        cmd[2] = 0x00;
        if (HAL_I2C_Master_Transmit(&hi2c2, AHT20_ADDR, cmd, 3, 100) != HAL_OK) {
            return 1;
        }
        HAL_Delay(10);
    }
    
    return 0; // 初始化成功
}

/**
 * @brief  获取温湿度
 * @return 0: 成功, 1: 失败
 */
uint8_t AHT20_Read_Data(float *Temperature, float *Humidity) {
    uint8_t cmd[3] = {AHT20_CMD_MEASURE, 0x33, 0x00};
    uint8_t data[6];
    uint8_t retry = 10;

    // 1. 发送触发测量命令
    if (HAL_I2C_Master_Transmit(&hi2c2, AHT20_ADDR, cmd, 3, 100) != HAL_OK) {
        return 1;
    }

    // 2. 等待测量完成 (说明书要求 80ms)
    HAL_Delay(80);

    // 3. 等待忙标志消失 Bit[7]
    while (retry--) {
        if (HAL_I2C_Master_Receive(&hi2c2, AHT20_ADDR, data, 1, 100) != HAL_OK) return 1;
        if ((data[0] & 0x80) == 0) break;
        HAL_Delay(10);
    }
    if (retry == 0) return 1;

    // 4. 读取完整 6 字节数据
    if (HAL_I2C_Master_Receive(&hi2c2, AHT20_ADDR, data, 6, 100) != HAL_OK) {
        return 1;
    }

    // 5. 数据换算 (20位数据)
    uint32_t humi_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

    *Humidity = (float)humi_raw / 1048576.0f * 100.0f;
    *Temperature = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;

    return 0;
}

/**
 * @brief  休眠断电，并防止 I2C 引脚倒灌漏电
 */
void AHT20_PowerOff(void) {
    // 1. 切断 VCC
    HAL_GPIO_WritePin(AHT_VCC_PORT, AHT_VCC_PIN, GPIO_PIN_RESET);

    // 2. 强行将 I2C 引脚设为模拟模式 (关闭上拉驱动)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // 3. 关闭 I2C 外设时钟以进一步省电
    __HAL_RCC_I2C2_CLK_DISABLE();
}

/**
 * @brief  由于 STM32 默认不支持 sprintf 打印浮点数，此函数手动实现整数化转换
 */
void AHT20_Format_String(char *buf, float temp, float humi) {
    // 处理温度 T:25.6 C
    int t_int = (int)temp;
    int t_dec = (int)(temp * 10) % 10;
    if (t_dec < 0) t_dec = -t_dec;

    // 处理湿度 H:45%
    int h_int = (int)humi;

    sprintf(buf, "T:%d.%d C, H:%d%%", t_int, t_dec, h_int);
}