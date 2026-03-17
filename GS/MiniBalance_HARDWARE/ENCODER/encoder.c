#include "encoder.h"

/**************************************************************************
函数功用：把TIM4初始化为编码器模式
**************************************************************************/
void Encoder_Init_TIM4(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_ICInitTypeDef TIM_ICInitStructure;

  // 1. 使能GPIOB和TIM4时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

  // 2. 配置PB6, PB7为浮空输入
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // 3. 配置定时器
  TIM_TimeBaseStructure.TIM_Period = 65535; // 满量程计数
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

  // 4. 配置编码器模式 (双相计数)
  TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising,
                             TIM_ICPolarity_Rising);

  // 5. 配置输入捕获滤波器
  TIM_ICStructInit(&TIM_ICInitStructure);
  TIM_ICInitStructure.TIM_ICFilter = 10;
  TIM_ICInit(TIM4, &TIM_ICInitStructure);

  // 6. 清零并开启定时器
  TIM_SetCounter(TIM4, 0);
  TIM_Cmd(TIM4, ENABLE);
}

/**************************************************************************
函数功用：单位时间读取编码器计数
**************************************************************************/
int Read_Encoder(u8 TIMX) {
  int Encoder_TIM;
  switch (TIMX) {
  case 2:
    Encoder_TIM = (short)TIM_GetCounter(TIM2);
    TIM_SetCounter(TIM2, 0);
    break;
  case 3:
    Encoder_TIM = (short)TIM_GetCounter(TIM3);
    TIM_SetCounter(TIM3, 0);
    break;
  case 4:
    Encoder_TIM = (short)TIM_GetCounter(TIM4);
    TIM_SetCounter(TIM4, 0);
    break;
  default:
    Encoder_TIM = 0;
  }
  return Encoder_TIM;
}
