// uart_protocol.h
#include "FreeRTOS.h"
#include "queue.h"

// 数据包结构 - 用于携带动态数据
typedef struct {
    uint8_t *data;      // 指向动态分配的数据
    size_t   length;    // 数据长度
    TickType_t tick;    // 时间戳（调试用）
} uart_packet_t;

// 全局队列句柄声明
extern osMessageQueueId_t xLUAQueueHandle;

// 初始化函数
void uart_protocol_init(void);