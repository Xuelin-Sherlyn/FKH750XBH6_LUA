// terminal.c
#include "terminal.h"
#include "portable.h"
#include "usart.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 全局数据变量 - 8字节对齐
static char RxData[MAX_CMD_LENGTH] __attribute__((aligned(8)));
static char rx_char __attribute__((aligned(4)));

// 全局终端实例 - 确保在BSS段对齐
Terminal_t g_terminal __attribute__((aligned(8)));

// ANSI转义码
#define ANSI_CLEAR_LINE   "\033[2K"
#define ANSI_CURSOR_HOME  "\033[G"
#define ANSI_CURSOR_LEFT  "\033[D"
#define ANSI_CURSOR_RIGHT "\033[C"
#define ANSI_SAVE_CURSOR  "\033[s"
#define ANSI_RESTORE_CURSOR "\033[u"

void Terminal_Init(void) {
    // 清零结构体
    memset(&g_terminal, 0, sizeof(Terminal_t));
    
    // 分配对齐的缓冲区
    g_terminal.data = RxData;  // 指向静态对齐缓冲区
    g_terminal.echo_enabled = true;
    
    // 创建队列 - 队列中存储字符串指针
    g_terminal.cmd_queue = xQueueCreate(5, sizeof(char*));
    
    if (g_terminal.cmd_queue == NULL) {
        safe_printf("\r\033[31mERROR: Failed to create command queue!\033[0m\r\n");
        // 可以考虑进入错误处理
    }
    
    // 清零缓冲区
    memset(RxData, 0, MAX_CMD_LENGTH);
    
    // 启动字符中断接收
    HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_char, 1);
    
    // 显示提示符
    safe_printf("\033[36m[Lua Shell]>\033[0m ");
}

// 辅助函数：复制字符串到对齐内存
static char* copy_string_aligned(const char* src) {
    if (src == NULL) return NULL;
    
    size_t len = strlen(src) + 1;
    
    // 分配对齐内存（8字节对齐）
    char* dst = (char*)malloc(len);
    if (dst == NULL) {
        return NULL;
    }
    
    // 复制字符串
    strcpy(dst, src);
    return dst;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    char* buffer = NULL;
    char* cmd_copy = NULL;
    if (huart->Instance == USART1) {
        
        // 立即重启接收
        HAL_UART_Receive_IT(huart, (uint8_t*)&rx_char, 1);
        
        // 获取缓冲区指针
        buffer = (char*)g_terminal.data;
        
        // 处理回车键
        if (rx_char == '\r' || rx_char == '\n') {
            if (g_terminal.length > 0) {
                // 确保字符串以NULL结尾
                buffer[g_terminal.length] = '\0';
                
                // 分配新内存并复制字符串到对齐内存
                cmd_copy = copy_string_aligned(buffer);
                if (cmd_copy != NULL) {
                    // 发送字符串指针到队列
                    if (xQueueSendFromISR(g_terminal.cmd_queue, &cmd_copy, &xHigherPriorityTaskWoken) != pdTRUE) {
                        // 发送失败，释放内存
                        free(cmd_copy);
                    }
                }
                
                // 重置终端状态
                g_terminal.length = 0;
                g_terminal.cursor_pos = 0;
                memset(buffer, 0, MAX_CMD_LENGTH);
            }
            
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            return;
        }
        
        // 处理退格键
        if (rx_char == 0x08 || rx_char == 0x7F) {
            if (g_terminal.cursor_pos > 0) {
                // 从缓冲区删除字符
                memmove(&buffer[g_terminal.cursor_pos - 1],
                        &buffer[g_terminal.cursor_pos],
                        g_terminal.length - g_terminal.cursor_pos);
                
                g_terminal.cursor_pos--;
                g_terminal.length--;
                buffer[g_terminal.length] = '\0';  // 更新结束符
                
                // 回显
                if (g_terminal.echo_enabled) {
                    safe_printf("\b \b");
                }
            }
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            return;
        }
        
        // 忽略控制字符（除了退格和回车）
        if (rx_char < 32 || rx_char == 127) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            return;
        }
        
        // 处理普通字符
        if (g_terminal.length < MAX_CMD_LENGTH - 1) {
            // 如果光标不在末尾，需要移动后面的字符
            if (g_terminal.cursor_pos < g_terminal.length) {
                memmove(&buffer[g_terminal.cursor_pos + 1],
                       &buffer[g_terminal.cursor_pos],
                       g_terminal.length - g_terminal.cursor_pos);
            }
            
            // 插入字符
            buffer[g_terminal.cursor_pos] = rx_char;
            g_terminal.cursor_pos++;
            g_terminal.length++;
            buffer[g_terminal.length] = '\0';  // 确保字符串终止
            
            // 回显字符
            if (g_terminal.echo_enabled) {
                uart_send_char(rx_char);
                
                // 如果光标不在末尾，需要重绘后面的字符
                if (g_terminal.cursor_pos < g_terminal.length) {
                    // 暂时保存光标位置
                    safe_printf(ANSI_SAVE_CURSOR);
                    
                    // 输出后面的字符
                    safe_printf("%s", &buffer[g_terminal.cursor_pos]);
                
                    // 恢复光标位置
                    safe_printf(ANSI_RESTORE_CURSOR);
                }
            }
        }
        
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}