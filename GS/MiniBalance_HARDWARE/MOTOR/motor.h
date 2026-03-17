#ifndef __MOTOR_H
#define __MOTOR_H
#include <sys.h>

// TB6612FNG电机管理 (根据用户指定接线)
#define PWMB TIM3->CCR4 // PB1 - 电机PWM (TIM3_CH4)
#define BIN1 PBout(13)  // PB13 - 方向1
#define BIN2 PBout(14)  // PB14 - 方向2
#define STBY PAout(7)   // PA7 - 使能(高电平工作)

void MiniBalance_PWM_Init(u16 arr, u16 psc);
void MiniBalance_Motor_Init(void);
#endif
