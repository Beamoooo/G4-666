#include "key.h"
#include "delay.h"

void KEY_Init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/**************************************************************************
函数功用：按键扫描（确认按下）
返回值：1-按下，0-无动作
**************************************************************************/
u8 Key_Scan(void) {
  static u8 key_up = 1; // 按键松开标志
  if (key_up && KEY2 == 0) {
    delay_ms(20); // 消抖
    key_up = 0;
    if (KEY2 == 0)
      return 1;
  } else if (KEY2 == 1)
    key_up = 1;
  return 0;
}
