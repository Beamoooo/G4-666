#ifndef __KEY_H
#define __KEY_H
#include "sys.h"

#define KEY2 PCin(13)

void KEY_Init(void);
u8 Key_Scan(void); // 返回1表示确认按下一次

#endif
