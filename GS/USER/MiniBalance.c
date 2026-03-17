/***********************************************
项目：电机正反转四步循环控制
功能：
1. 按下KEY2 -> 正转 (BIN1=1, BIN2=0)
2. 再按一次 -> 停止 (BIN1=0, BIN2=0)
3. 再按一次 -> 反转 (BIN1=0, BIN2=1)
4. 再按一次 -> 停止 (BIN1=0, BIN2=0)
硬件：STM32F103 + TB6612 + 电机(带编码器)
接线：PC13-KEY2, PB13-BIN1, PB14-BIN2, PA7-STBY, PB1-PWMB(TIM3_CH4),
PB6/7-Encoder(TIM4)
***********************************************/
#include "delay.h"
#include "encoder.h"
#include "key.h"
#include "motor.h"
#include "stm32f10x.h"
#include "sys.h"
#include "usart.h"
#include "sync_comm.h"

// ==========================================
// 核心配置：切换主从板只需要修改这个宏定义！
// 1 = 这块板子是【主板】 (挂接KEY2按键，主动发指令)
// 0 = 这块板子是【从板】 (只听主机指挥，自己堵转报警)
// ==========================================
#define IS_MASTER_BOARD 0

// 电机状态枚举 (全局可见，方便中断修改)
typedef enum {
  MOTOR_STATE_STOP1 = 0,
  MOTOR_STATE_FORWARD,
  MOTOR_STATE_STOP2,
  MOTOR_STATE_BACKWARD
} MotorState_t;

volatile MotorState_t current_state = MOTOR_STATE_STOP1;

// ========================================================
// 串口 2 收到完整的同步通信帧后，中断自动调用此函数
// ========================================================
void Process_Sync_Command(u8 cmd, u8 param)
{
    if(cmd == CMD_EMERGENCY)
    {
        // 收到急停警报 (对面的板子发生堵转了)
        printf("\r\n[SYNC] EMERGENCY STOP from another board!\r\n");
        current_state = MOTOR_STATE_STOP1; // 强制切换到停止状态
        BIN1 = 0;
        BIN2 = 0;
        PWMB = 0;
    }
    
#if (IS_MASTER_BOARD == 0) // 只有从板才处理主机的运动指令
    else if(cmd == CMD_MOTOR_CTRL)
    {
        printf("\r\n[SYNC] Master CMD Received: %d\r\n", param);
        if(param == PARAM_FORWARD) 
        {
            current_state = MOTOR_STATE_FORWARD;
            BIN1 = 1; BIN2 = 0; PWMB = 2000;
        }
        else if(param == PARAM_BACKWARD) 
        {
            current_state = MOTOR_STATE_BACKWARD;
            BIN1 = 0; BIN2 = 1; PWMB = 2000;
        }
        else if(param == PARAM_STOP) 
        {
            current_state = MOTOR_STATE_STOP1;
            BIN1 = 0; BIN2 = 0; PWMB = 0;
        }
    }
#endif
}

int main(void) {
	
  int encoder_increment = 0;
  int stall_counter = 0;
  u32 timestamp = 0;

  // 1. 系统初始化
  MY_NVIC_PriorityGroupConfig(2);
  delay_init();
  uart_init(115200);   // printf 用
  SyncComm_Init(115200); // 新增加的串口2双板通信

  // 2. 硬件驱动初始化
  MiniBalance_PWM_Init(7199, 0); // 10kHz PWM
  Encoder_Init_TIM4();           // TIM4 编码器模式
  
#if IS_MASTER_BOARD
  KEY_Init();                    // 只有主机需要初始化 KEY2 (PC13)
#endif

  printf("\r\n========================================\r\n");
#if IS_MASTER_BOARD
  printf("  [MASTER BOARD] Sync System Started\r\n");
#else
  printf("  [SLAVE BOARD] Sync System Started\r\n");
#endif
  printf("========================================\r\n");

  while (1) {
      
#if IS_MASTER_BOARD
    // 3. 【主板专属逻辑】按键扫描与状态机切换 (按下KEY2触发)
    if (Key_Scan()) {
      current_state = (MotorState_t)((current_state + 1) % 4);

      // 执行相应动作，【并且立刻发串口指令告诉从机】
      switch (current_state) {
      case MOTOR_STATE_FORWARD:
        BIN1 = 1; BIN2 = 0; PWMB = 2000; // 正转
        printf(">> Master changed: FORWARD\r\n");
        SyncComm_SendPacket(CMD_MOTOR_CTRL, PARAM_FORWARD);
        break;
      case MOTOR_STATE_BACKWARD:
        BIN1 = 0; BIN2 = 1; PWMB = 2000; // 反转
        printf(">> Master changed: BACKWARD\r\n");
        SyncComm_SendPacket(CMD_MOTOR_CTRL, PARAM_BACKWARD);
        break;
      case MOTOR_STATE_STOP1:
      case MOTOR_STATE_STOP2:
        BIN1 = 0; BIN2 = 0; PWMB = 0; // 停止
        printf(">> Master changed: STOP\r\n");
        SyncComm_SendPacket(CMD_MOTOR_CTRL, PARAM_STOP);
        break;
      }
    }
#endif

    // 4. 定时任务：100ms周期任务 (读取编码器 + 双板各自独立的堵转监测)
    timestamp++;
    if (timestamp >= 10) // 10 * 10ms = 100ms
    {
      timestamp = 0;

      // 读取本次100ms内的增量 (Read_Encoder会自动清零计数器)
      encoder_increment = Read_Encoder(4);

      // 堵转监测逻辑 (仅在电机当前处于运动状态下判断)
      if (current_state == MOTOR_STATE_FORWARD ||
          current_state == MOTOR_STATE_BACKWARD) {
              
        // 计算绝对值 (获取当前速度)
        int abs_speed = (encoder_increment < 0) ? -encoder_increment : encoder_increment;

        // --- 增强型判定逻辑 (速度低于30阈值累计3次) ---
        if (abs_speed < 30) {
          stall_counter++;
        } else {
          if (stall_counter > 0) stall_counter--; // 容错机制
        }

        if (stall_counter >= 3) {
          printf("\r\n!!! STALL DETECTED on THIS BOARD !!!\r\n");
          
          // 自己立刻停机
          current_state = MOTOR_STATE_STOP1; 
          BIN1 = 0;
          BIN2 = 0;
          PWMB = 0;
          stall_counter = 0;
          
          // 【核心】立刻发送急停警报给另一块板子！ (主从机都适用)
          SyncComm_SendPacket(CMD_EMERGENCY, 0xFF); 
          printf(">> Sent Emergency Stop to another board.\r\n");
        }
      }

      // 为了看清楚，每隔一段时间打印一下状态
      // printf("State: %d | Speed: %d | StallCnt: %d\r\n", current_state, encoder_increment, stall_counter);
    } // end of 100ms timer

    delay_ms(10); // 主循环节拍
  }
}
