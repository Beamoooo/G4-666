# 工程功能与调用关系深度审查报告

## 1. 总体结构与主流程

### 1.1 入口文件与主函数
- 入口：[USER/MiniBalance.c](../USER/MiniBalance.c)
- 主函数：main()
- 主要流程：
  1. 系统与外设初始化（中断优先级、延时、串口、PWM、编码器、按键等）
  2. 主/从板逻辑分支（主板处理按键与指令下发，从板响应指令）
  3. 100ms周期任务：编码器测速、堵转检测、急停联动

### 1.2 主要外部信号/数据流
- 按键输入（KEY2，主板）
- 编码器输入（TIM4，速度反馈）
- 串口通信（USART2，主从同步/急停）
- 电机驱动输出（TB6612，BIN1/BIN2/PWMB）

## 2. 主函数调用链与功能关联

### 2.1 main()初始化流程
- MY_NVIC_PriorityGroupConfig(2)：中断优先级分组
- delay_init()：延时系统初始化
- uart_init(115200)：调试串口初始化
- SyncComm_Init(115200)：双板通信串口初始化
- MiniBalance_PWM_Init(7199, 0)：PWM初始化
- Encoder_Init_TIM4()：编码器初始化
- KEY_Init()：按键初始化（主板）

### 2.2 主循环与状态机
- 主板：
  - Key_Scan() 检测按键，切换 current_state
  - 根据状态切换，控制 BIN1/BIN2/PWMB 并通过 SyncComm_SendPacket() 通知从板
- 从板：
  - 通过 Process_Sync_Command() 响应主板指令，切换 current_state 并控制输出

### 2.3 100ms周期任务
- Read_Encoder(4)：读取编码器增量
- 堵转检测：速度低于阈值累计，触发急停
- SyncComm_SendPacket(CMD_EMERGENCY, 0xFF)：急停联动

## 3. 中断与外部事件响应
- 串口2接收帧后自动调用 Process_Sync_Command()（中断服务）
- 编码器、按键等外设通过定时/中断方式驱动
- NVIC/EXTI相关配置见 SYSTEM/sys/sys.c

## 4. 主要功能模块与文件关联
- USER/MiniBalance.c：主流程、状态机、堵转保护、主从切换
- SYSTEM/delay/delay.c：延时与定时
- SYSTEM/sys/sys.c：中断优先级、外部中断配置
- SYSTEM/usart/usart.c：串口初始化与printf重定向
- MiniBalance_HARDWARE/ENCODER/encoder.c：编码器驱动
- MiniBalance_HARDWARE/KEY/key.c：按键驱动
- MiniBalance_HARDWARE/MOTOR/motor.c：电机驱动
- MiniBalance_HARDWARE/SYNC_COMM/sync_comm.c：主从通信协议

## 5. 关键数据流与信号链路
- 按键(KEY2)→主板main→状态机→电机控制/指令下发
- 编码器(TIM4)→Read_Encoder→堵转检测→急停
- 串口2(USART2)→Process_Sync_Command→状态同步/急停
- 电机输出(BIN1/BIN2/PWMB)→TB6612→电机

## 6. 中断与定时机制
- NVIC/EXTI配置：见sys.c，支持外部中断与优先级管理
- 串口2接收中断：自动解析帧并调用Process_Sync_Command
- 定时任务：主循环delay_ms(10)节拍+100ms周期任务

## 7. PID 速度闭环控制功能

为提升双电机转速一致性，系统建议引入基于编码器反馈的PID闭环调速：

### 7.1 原理与公式
- 设定目标速度 `target_speed`（如每100ms编码器增量目标值）；
- 实时采样当前速度 `current_speed = abs(Read_Encoder(4))`；
- 计算误差 `error = target_speed - current_speed`；
- 通过PID输出增量调节PWM，形成“慢则加、大则减”的自适应控制。

PID公式：

  PWM_Output = Kp * error + Ki * error_sum + Kd * (error - last_error)

其中：
- Kp：比例系数，决定响应快慢；
- Ki：积分系数，消除稳态误差；
- Kd：微分系数，抑制超调和振荡。

### 7.2 工程实现建议
可在主循环100ms任务中集成如下代码片段：

```c
// 目标速度（100ms增量）
int target_speed = 300;
// PID参数（需实机整定）
float Kp = 3.0f, Ki = 0.2f, Kd = 0.5f;
static float error_sum = 0;
static float last_error = 0;
static int pwm_base = 2000;

int current_speed = (encoder_increment < 0) ? -encoder_increment : encoder_increment;
float error = (float)(target_speed - current_speed);
error_sum += error;
// 抗积分饱和
if (error_sum > 2000) error_sum = 2000;
if (error_sum < -2000) error_sum = -2000;
float pid_out = Kp * error + Ki * error_sum + Kd * (error - last_error);
last_error = error;
int pwm_cmd = pwm_base + (int)pid_out;
// PWM限幅
if (pwm_cmd > 7199) pwm_cmd = 7199;
if (pwm_cmd < 0) pwm_cmd = 0;
PWMB = pwm_cmd;
```

### 7.3 与现有系统结合方式
1. 电机进入运行状态（正转/反转）后，启用PID闭环更新PWMB；
2. 停止状态时清零积分项，避免再次启动时突变；
3. 堵转或急停触发时，立即将PWMB=0并冻结PID状态；
4. 双板可先分别本地闭环，再通过上层同步策略做速度一致性补偿。

---

## 8. 参考文档
- [doc/智能座椅技术文档.md](智能座椅技术文档.md)：系统方案、硬件接口、功能说明

---

> 本报告基于源代码与文档自动分析生成，详细函数调用与信号链路可结合各.c/.h文件进一步追溯。