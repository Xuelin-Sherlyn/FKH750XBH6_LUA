// terminal.h
#ifndef __TERMINAL_H
#define __TERMINAL_H

#include "FreeRTOS.h"
#include "queue.h"
#include <stdbool.h>

#define MAX_CMD_LENGTH   256
#define MAX_HISTORY      10

// 使用 aligned 属性确保结构体8字节对齐
typedef struct {
    void* data;                   // 当前行缓冲区指针
    uint16_t cursor_pos;          // 光标位置（也是缓冲区索引）
    uint16_t length;              // 当前行长度
    bool     echo_enabled;        // 是否回显（用于密码输入）
    
    // 历史记录
    char history[MAX_HISTORY][MAX_CMD_LENGTH];
    uint8_t history_index;
    uint8_t history_count;
    
    QueueHandle_t cmd_queue;      // 完成命令队列
} Terminal_t;

extern Terminal_t g_terminal;

// 初始化终端
void Terminal_Init(void);

#endif