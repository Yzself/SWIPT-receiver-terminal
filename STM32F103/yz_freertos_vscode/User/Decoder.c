#include "Decoder.h"

//**********************************************************************
/*                         汉明码解码函数
  输入变量：
    HammingIn: 16位汉明码输入(实际有效位13~0，14、15位置零)，值传递
  输出值：
    *HammingOut: 8位汉明码输出，地址传递
*/
//**********************************************************************
void HammingDecoder(uint16_t const HammingIn, uint8_t *HammingOut)
{
  *HammingOut = 0x00;           //清零
  uint8_t verify_low=0x00, verify_high=0x00;
  uint8_t S1=0, S2=0, S3=0;
  
  /* 低4位数据解调 */
  //监督位S1、S2、S3
  S1 = ((HammingIn&0x08)>>3) ^ ((HammingIn&0x10)>>4) ^ ((HammingIn&0x20)>>5) ^ ((HammingIn&0x40)>>6);
  S2 = ((HammingIn&0x02)>>1) ^ ((HammingIn&0x04)>>2) ^ ((HammingIn&0x20)>>5) ^ ((HammingIn&0x40)>>6);
  S3 = (HammingIn&0x01) ^ ((HammingIn&0x04)>>2) ^ ((HammingIn&0x10)>>4) ^ ((HammingIn&0x40)>>6);
  verify_low = (S1<<2) | (S2<<1) | S3;
  //低4位数据 
  *HammingOut |= ((HammingIn&0x04)>>2) | ((HammingIn&0x10)>>3) | ((HammingIn&0x20)>>3) | ((HammingIn&0x40)>>3);
  
  if(verify_low == 3)
    *HammingOut ^= 1;           //纠正a2
  else if(verify_low == 5)
    *HammingOut ^= (1<<1);      //纠正a4
  else if(verify_low == 6)
    *HammingOut ^= (1<<2);      //纠正a5
  else if(verify_low == 7)
    *HammingOut ^= (1<<3);      //纠正a6

  /* 高4位数据解调 */
  //监督位S1、S2、S3
  S1 = ((HammingIn&0x0400)>>10) ^ ((HammingIn&0x0800)>>11) ^ ((HammingIn&0x1000)>>12) ^ ((HammingIn&0x2000)>>13);
  S2 = ((HammingIn&0x0100)>>8) ^ ((HammingIn&0x0200)>>9) ^ ((HammingIn&0x1000)>>12) ^ ((HammingIn&0x2000)>>13);
  S3 = ((HammingIn&0x0080)>>7) ^ ((HammingIn&0x0200)>>9) ^ ((HammingIn&0x0800)>>11) ^ ((HammingIn&0x2000)>>13);
  verify_high = (S1<<2) | (S2<<1) | S3;
  //高4位数据 
  *HammingOut |= ((HammingIn&0x0200)>>5) | ((HammingIn&0x0800)>>6) | ((HammingIn&0x1000)>>6) | ((HammingIn&0x2000)>>6);
  
  if(verify_high == 3)
    *HammingOut ^= (1<<4);      //纠正a9
  else if(verify_high == 5)
    *HammingOut ^= (1<<5);      //纠正a11
  else if(verify_high == 6)
    *HammingOut ^= (1<<6);      //纠正a12
  else if(verify_high == 7)
    *HammingOut ^= (1<<7);      //纠正a13
  
}
