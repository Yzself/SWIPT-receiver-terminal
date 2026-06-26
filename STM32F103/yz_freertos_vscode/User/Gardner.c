/**
 * @file    Gardner.c
 * @brief   Gardner timing recovery algorithm
 * @author  YuZhang Lin
 * @date    2026-06-26
 */

#include "Gardner.h"
#include <math.h>

//**********************************************************************
/*                      Gardner内插函数
  输入变量：
    GardnerIn_Array[0~3]:经过处理后的采样数组
  输出变量：
    yI0~yI3:内插后的数组，长度是输入数组的两倍
  要求：
    采样率是码元速率的四倍 
  算法原理：
    符号速率为T的输入信号x(t)经过本地固定时钟周期Ts采样后变为离散信号x(mTs)， 
  经过内插滤波器得出的值送入定时误差检测器(TED)得出输入信号与本地时钟的相位
  误差te，再通过一个环路滤波器滤除其中的噪声及高频成分，将得到值送入NCO计算
  出整数采样时刻mk和内插滤波器插值点位置uk从而得到定时输出y(kTi)。 
    NCO对以Ts为采样时钟的输入信号进行抽样，NCO的工作时钟与输入信号的工作时钟
  一致也为Ts，而生成的重采样周期Ti应与输入信号的符号率T同步。每次NCO寄存器溢
  出一次则表示要执行一次重采样操作。每次NCO寄存器过零点的时刻(mk+1)Ts便是内
  插滤波器进行一次运算的时刻。 
  内插公式表示：
    y(kTi) = sigma x(mTs)h(kTi-mTs) = y[(mk+uk)Ts]	
		
	它的工作流程是：
	1.ADC 采样送入 Gardner。
	2.NCO 像个节拍器一样每 2 个 ADC 点敲一下（下溢）。
	3.下溢瞬间，利用窗口里的 4 个点算出那个“不偏不倚”的准点。
	4.每两个准点算一次误差，并微调 NCO 的节奏（w_old）。
	5.每两个准点输出一个 JudgeDat（0 或 1）。		
*/
typedef struct GardnerStatus{
	bool yI_Statement;
	bool JStatement;
}GardnerStatus;

static GardnerStatus GStatus = {
	.yI_Statement = false,	// true:得到一次插值结果
	.JStatement = false			// true:得到一个最佳采样点
};
static float w_old = 0.5;                      //环路滤波器寄存器输出，表示每次递减的步长
static float n_old = 0.7;                      //NCO寄存器，当n_old≤0时发生下溢
static float u = 0.6;                          //NCO输出的定时分数间隔寄存器
static float te_old = 0;                    	 //时钟误差寄存器
static float yI0 = 0.0, yI1 = 0.0, yI2 = 0.0;  //插值结果
static uint8_t strobe = 1, ki = 0, kt = 0, ms = 0;
static float GardnerIn_Array[4] = {0.0};       //Gardner输入 

void Gardner(float voltage_Ts, GardnerStruct *GStruct)
{
	GStruct->HD_Trigger = false;	// 待判决状态
	
	GardnerIn_Array[0] = GardnerIn_Array[1];
	GardnerIn_Array[1] = GardnerIn_Array[2];
	GardnerIn_Array[2] = GardnerIn_Array[3];
	GardnerIn_Array[3] = voltage_Ts;
	
	float n_temp, n_new = 0, w_new = 0, te_new = 0;
  float FI1=0, FI2=0, FI3=0;		//内插系数
  
  n_temp = n_old - w_old;				//当n<w时，表示下一个符号周期即将到来，NCO将产生一次过零 
																//点，寄存器的值模1后的值设为下一个符号周期NCO的初始值
  if(n_temp>0)
  {
    n_new = n_temp;
  }
  else
  {
		// NCO发生下溢, 到插值时刻。
    n_new = 1.0-fabs(n_temp);
    //内插滤波器模块(Farrow结构) 
    FI1 = 0.5*GardnerIn_Array[3]-0.5*GardnerIn_Array[2]-0.5*GardnerIn_Array[1]+0.5*GardnerIn_Array[0];
    FI2 = 1.5*GardnerIn_Array[2]-0.5*GardnerIn_Array[3]-0.5*GardnerIn_Array[1]-0.5*GardnerIn_Array[0];
    FI3 = GardnerIn_Array[1];
    yI0 = yI1; yI1 = yI2;
    yI2 = (FI1*u+FI2)*u+FI3;            //yI2为最新的内插值，由最近的四个采样点拟合而来!
    GStatus.yI_Statement = true;        //状态：得到一次插值结果
    
    strobe = strobe%2;
    if(strobe == 0)	// 每两个内插点调整一次内插时钟
    {
      //每个数据符号计算一次时钟误差
      if(kt==1)
      {
        te_new = yI1*(yI2-yI0);
      }
      else
      {
        te_new = yI1*yI2;
        kt = 1;
      }
      //每个数据符号计算一次环路滤波器输出，采用一阶环
      if(ms==1)
      {
        w_new = w_old+LFC*(te_new-te_old);
      }
      else
      {
        ms = 1;
        w_new = w_old+LFC*te_new;
      }
      
      te_old = te_new;	//迭代储存新的time error
      w_old = w_new;		//迭代储存环路滤波器输出
    }
    
    strobe++;
    ki++;
    if(ki==2)
    {
      GStatus.JStatement = true;  //yI2隔一点取值，此时为最佳采样点
      ki = 0;
    }
    u = n_old/w_old;			//计算分数间隔
  }
  
  n_old = n_new; 	        //迭代NCO寄存器输出 
	
	if(GStatus.yI_Statement)       //插值点
	{
		GStatus.yI_Statement = false;
    
		if(GStatus.JStatement)         //最佳采样点
		{
			GStatus.JStatement = false;
			if(yI2 > 0)                 //判决
			{
				GStruct->JudgeDat = 1;
			}
			else
			{
				GStruct->JudgeDat = 0;
			}
      
			GStruct->HD_Trigger = true;    //判决成功，提取到有效输出
		}
	}
}
