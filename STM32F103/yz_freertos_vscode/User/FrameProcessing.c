#include "FrameProcessing.h"
#include "Decoder.h"
#include "PowerManager.h"
#include <string.h>

static uint8_t FrameStatus = 0;         				 // 状态机：0寻找帧头，1解析帧长，2数据解调   
static uint32_t FrameHead = 0;									 // 缓存待检测的帧头(32bits) 
static uint8_t FLHam_Index = HANMING_LEN;        // 解析帧长索引
static uint16_t FLHamming = 0;     							 // 缓存待计算的帧长(16bits) 
static uint8_t FrameLength = 0;            			 // 解析出的帧长
static uint8_t IM_Buffer[MAX_FRAME_PAYLOAD + 1]; // 信息缓存
static uint8_t IM_Index = HANMING_LEN;    			 // 每一信息符号的汉明码bits索引          
static uint16_t IM_In = 0;            					 // 每一信息符号的汉明码bits缓存
static uint8_t IM_temp;                    			 // 汉明解码缓存
static uint8_t IM_Dat_Index = 0;           			 // 信息位索引

/**
 * @brief 帧处理函数
 * @param bit Gardner输出的有效位
 * @param reMessage 外部传入的接收缓存（确保大小足够）
 * @return bool 是否成功接收一帧
 */
bool FrameProcessing(uint8_t bit, uint8_t *reMessage)
{
    if(FrameStatus == 0)    // 1.寻找帧头
    {
        // 高效移位：假设最新位进 MSB
        FrameHead = (FrameHead >> 1) | ((uint32_t)bit << 31);
        
        // 计算汉明距离（异或后计算1的个数）
        uint32_t diff = FrameHead ^ FRAME_HEADER;
        
        // 使用编译器内置函数（硬件指令）快速计算1的个数，比while循环快几十倍
        if (__builtin_popcount(diff) <= 2) 
        {
            FrameStatus = 1;
            FLHam_Index = HANMING_LEN;	
            FLHamming = 0;
						FrameLength = 0;
            IM_Dat_Index = 0; // 关键：在这里复位索引
					
						Power_RefreshActivity();// 成功找到帧头，活跃
        }
    }
    else if(FrameStatus == 1)  // 2.解析帧长 (HANMING_LEN位汉明码)
    {
        FLHamming = (FLHamming >> 1) | ((uint16_t)bit << 15);
        FLHam_Index--;

        if(FLHam_Index == 0)
        {
            // 提取 14 位有效码
            uint16_t code = FLHamming >> 2; // 原逻辑保持不变
            
            HammingDecoder(code, &FrameLength);
            if(FrameLength > MAX_FRAME_PAYLOAD) 
						{
							FrameLength = MAX_FRAME_PAYLOAD;	// 粗暴强制转换，可以有更好的逻辑
						}else if(FrameLength == 0)
						{
							FrameStatus = 0;
							FrameHead = 0;
						}else{
							FrameStatus = 2;
							IM_Index = HANMING_LEN;
							IM_In = 0;
						}
        }
    }
    else if(FrameStatus == 2)  // 3.数据解调
    {
        IM_In = (IM_In >> 1) | ((uint16_t)bit << 15);
        IM_Index--;

        if(IM_Index == 0)
        {
            IM_Index = HANMING_LEN;
            uint16_t code = IM_In >> 2;
            
            HammingDecoder(code, &IM_temp);
            IM_Buffer[IM_Dat_Index++] = IM_temp;
            
            // 使用计数器控制接收，不改变 FrameLength 本身的值，方便后续处理
            if(IM_Dat_Index >= FrameLength)
            {
                IM_Buffer[IM_Dat_Index] = '\0';
                
                // 关键修正：将数据拷贝给外部，而不是改指针地址
                memcpy(reMessage, IM_Buffer, IM_Dat_Index + 1);
                
								FrameHead = 0;
                FrameStatus = 0; // 复位状态机
                return true;     // 成功触发
            }
        }
    }
    return false;
}
  
