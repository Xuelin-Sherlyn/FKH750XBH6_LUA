// uart_dyn_rx.c
#include "uart_dyn_rx.h"
#include "usart.h"
#include <string.h>

// DMA缓冲区
__attribute__((section(".ram_d1"), aligned(32)))
static uint8_t s_dma_rx_buffer[1024];
static uint8_t safe_buffer[512];

// 全局接收器实例
UART_Dyn_Receiver_t UART_Receiver = {
    .huart = &huart1,
    .dma_buffer = s_dma_rx_buffer,
    .dma_buffer_size = sizeof(s_dma_rx_buffer),
    .next_pos = 0,
    .packet_queue = NULL
};
// 全局数据包实例
UART_DataPacket_t DataPacket;

// 初始化
void UART_Dynamic_Receive_Init(void) {
    // 创建队列（10个包足够）
    UART_Receiver.packet_queue = xQueueCreate(10, sizeof(UART_DataPacket_t));
    
    // 启动不定长接收
    HAL_UARTEx_ReceiveToIdle_DMA(
        UART_Receiver.huart,
        UART_Receiver.dma_buffer,
        UART_Receiver.dma_buffer_size
    );
    
    // 启用空闲中断
    __HAL_UART_ENABLE_IT(UART_Receiver.huart, UART_IT_IDLE);
}

// UART接收事件回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint16_t copy_len = 0;
    uint32_t src_pos = 0;
    uint32_t bytes_until_wrap = 0;
    
    if (huart->Instance == USART1 && Size > 0) {
        // 1. 立即复制数据到安全位置
        copy_len = (Size < sizeof(safe_buffer)) ? Size : sizeof(safe_buffer)-1;
        
        // 计算源地址（考虑环形缓冲区回绕）
        src_pos = UART_Receiver.next_pos;
        bytes_until_wrap = UART_Receiver.dma_buffer_size - src_pos;
        
        if (copy_len <= bytes_until_wrap) {
            // 线性复制
            memcpy(safe_buffer, &UART_Receiver.dma_buffer[src_pos], copy_len);
        } else {
            // 回绕复制
            memcpy(safe_buffer, &UART_Receiver.dma_buffer[src_pos], bytes_until_wrap);
            memcpy(&safe_buffer[bytes_until_wrap], 
                   UART_Receiver.dma_buffer, 
                   copy_len - bytes_until_wrap);
        }
        
        // 添加字符串终止符
        safe_buffer[copy_len] = '\0';
        
        // 2. 填充数据包
        DataPacket.data = safe_buffer;
        DataPacket.length = copy_len;
        DataPacket.start_pos = src_pos;
        
        // 3. 发送到队列
        if (xQueueSendFromISR(UART_Receiver.packet_queue, 
                             &DataPacket, 
                             &xHigherPriorityTaskWoken) == pdPASS) {
            // 4. 清理刚刚使用过的DMA缓冲区区域
            if (copy_len <= bytes_until_wrap) {
                // 线性清理
                memset(&UART_Receiver.dma_buffer[src_pos], 0, copy_len);
            } else {
                // 回绕清理
                memset(&UART_Receiver.dma_buffer[src_pos], 0, bytes_until_wrap);
                memset(UART_Receiver.dma_buffer, 0, copy_len - bytes_until_wrap);
            }
            
            // 5. 更新位置
            // UART_Receiver.next_pos = (src_pos + Size) % UART_Receiver.dma_buffer_size;
        }
    }
    // 可选：清理整个缓冲区（更彻底）
    // memset(UART_Receiver.dma_buffer, 0, UART_Receiver.dma_buffer_size);
    // 重新启动不定长接收
    HAL_UARTEx_ReceiveToIdle_DMA(
            UART_Receiver.huart,
            UART_Receiver.dma_buffer,
            UART_Receiver.dma_buffer_size
        );
    // 重新启用空闲中断
    __HAL_UART_ENABLE_IT(UART_Receiver.huart, UART_IT_IDLE);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}