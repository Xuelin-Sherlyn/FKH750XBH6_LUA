// uart_dyn_rx.h
#ifndef __UART_DYN_RX_H
#define __UART_DYN_RX_H

#include "FreeRTOS.h"
#include "queue.h"
#include "stm32h7xx_hal.h"

// 最简数据包结构
typedef struct {
    void*           data;           // 指向DMA缓冲区的数据（不分配新内存）
    uint16_t        length;         // 数据长度
    uint32_t        start_pos;      // 起始位置（调试用）
    TickType_t      timestamp;      // 时间戳（调试用）
} UART_DataPacket_t;

// 最简接收管理器
typedef struct {
    UART_HandleTypeDef*  huart;             // UART句柄
    QueueHandle_t        packet_queue;      // 目标包队列
    uint8_t*             dma_buffer;        // DMA缓冲区
    uint32_t             dma_buffer_size;   // DMA缓冲区大小
    volatile uint32_t    next_pos;          // 下一个起始位置
} UART_Dyn_Receiver_t;

extern UART_Dyn_Receiver_t UART_Receiver;

void UART_Dynamic_Receive_Init(void);

#endif