#ifndef __SYNC_COMM_H
#define __SYNC_COMM_H

#include "stm32f10x.h"

// ============================================
// Communication Protocol Commands
// ============================================
#define CMD_MOTOR_CTRL   0x01 // Motor Movement Control Command
#define CMD_EMERGENCY    0x0E // Emergency Stop Alarm Command (Both ways)

// Motor Movement Parameters
#define PARAM_STOP       0x00
#define PARAM_FORWARD    0x01
#define PARAM_BACKWARD   0x02

// ============================================
// Function Prototypes
// ============================================
void SyncComm_Init(u32 bound);
void SyncComm_SendPacket(u8 cmd, u8 param);

// This function must be implemented in main.c (or similar application file)
// and handles the application-level response to incoming packets.
void Process_Sync_Command(u8 cmd, u8 param); 

#endif
