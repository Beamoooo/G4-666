#include "sync_comm.h"

// ============================================
// Initialize USART2 (PA2 = TX, PA3 = RX)
// ============================================
void SyncComm_Init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 1. Enable Clocks: GPIOA (Pins) and USART2 (Peripheral)
    // Note: USART2 is on APB1!
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // 2. Configure PA2 (USART2_TX) as Alternate Function Push-Pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. Configure PA3 (USART2_RX) as Floating Input
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 4. Configure USART2 Parameters
    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    // 5. Configure NVIC for USART2 Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // Preemption Priority 1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        // Sub Priority 0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 6. Enable USART2 Receive Interrupt (RXNE)
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    // 7. Enable USART2
    USART_Cmd(USART2, ENABLE);
}

// ============================================
// Internal helper: Send 1 Byte
// ============================================
static void SyncComm_SendByte(u8 byte)
{
    USART_SendData(USART2, byte);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET); 
}

// ============================================
// External Function: Send a Command Packet
// Format: [0xAA] [CMD] [PARAM] [CHECKSUM]
// ============================================
void SyncComm_SendPacket(u8 cmd, u8 param)
{
    u8 checksum = (cmd + param) & 0xFF; 
    
    SyncComm_SendByte(0xAA);      // Header
    SyncComm_SendByte(cmd);       // Command
    SyncComm_SendByte(param);     // Parameter
    SyncComm_SendByte(checksum);  // Checksum
}

// ============================================
// USART2 Interrupt Service Routine
// Handle byte reception and packet parsing
// ============================================
#define RX_STATE_WAIT_HEADER  0
#define RX_STATE_WAIT_CMD     1
#define RX_STATE_WAIT_PARAM   2
#define RX_STATE_WAIT_CHECK   3

static u8 rx_state = RX_STATE_WAIT_HEADER;
static u8 rx_cmd = 0;
static u8 rx_param = 0;

void USART2_IRQHandler(void)
{
    u8 rx_data;
    u8 expected_checksum;

    // Check if it's the Receive Interrupt flag
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) 
    {
        rx_data = USART_ReceiveData(USART2); 

        switch(rx_state)
        {
            case RX_STATE_WAIT_HEADER:
                if(rx_data == 0xAA) {
                    rx_state = RX_STATE_WAIT_CMD; 
                }
                break;
                
            case RX_STATE_WAIT_CMD:
                rx_cmd = rx_data;
                rx_state = RX_STATE_WAIT_PARAM;
                break;
                
            case RX_STATE_WAIT_PARAM:
                rx_param = rx_data;
                rx_state = RX_STATE_WAIT_CHECK;
                break;
                
            case RX_STATE_WAIT_CHECK:
                expected_checksum = (rx_cmd + rx_param) & 0xFF;
                // Checksum passed, a full valid packet received
                if(rx_data == expected_checksum) 
                {
                    // Pass it up to the application level logic in main.c
                    Process_Sync_Command(rx_cmd, rx_param); 
                }
                // Reset to wait for the next packet header
                rx_state = RX_STATE_WAIT_HEADER;
                break;
                
            default:
                rx_state = RX_STATE_WAIT_HEADER;
                break;
        }
        
        // Clear the Interrupt Pending Bit
        USART_ClearITPendingBit(USART2, USART_IT_RXNE); 
    }
}
