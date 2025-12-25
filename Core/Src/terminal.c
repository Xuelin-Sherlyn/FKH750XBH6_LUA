// terminal.c - 简化版本
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
    g_terminal.data = RxData;
    g_terminal.echo_enabled = true;
    g_terminal.state = TERM_STATE_NORMAL;
    g_terminal.history_display_index = 0;
    
    // 创建队列
    g_terminal.cmd_queue = xQueueCreate(5, sizeof(char*));
    
    if (g_terminal.cmd_queue == NULL) {
        safe_printf("\r\033[31mERROR: Failed to create command queue!\033[0m\r\n");
    }
    
    // 清零缓冲区
    memset(RxData, 0, MAX_CMD_LENGTH);
    
    // 启动字符中断接收
    HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_char, 1);
    
    // 显示提示符
    safe_printf("\033[36m[Lua Shell]>\033[0m ");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static char temp_buffer[MAX_CMD_LENGTH]; // 用于历史记录临时存储
    size_t len;
    
    if (huart->Instance == USART1) {
        // 立即重启接收
        HAL_UART_Receive_IT(huart, (uint8_t*)&rx_char, 1);
        
        char* buffer = (char*)g_terminal.data;
        
        // 处理状态机
        switch (g_terminal.state) {
            case TERM_STATE_NORMAL:
                if (rx_char == 0x1B) { // ESC键
                    g_terminal.state = TERM_STATE_ESC;
                } else {
                    // 处理回车键
                    if (rx_char == '\r' || rx_char == '\n') {
                        if (g_terminal.length > 0) {
                            buffer[g_terminal.length] = '\0';
                            
                            // 添加到历史记录
                            if (g_terminal.history_count > 0) {
                                if (strcmp(buffer, g_terminal.history[g_terminal.history_count - 1]) == 0) {
                                    goto reset_terminal;
                                }
                            }
                            
                            if (g_terminal.history_count == MAX_HISTORY) {
                                for (uint8_t i = 0; i < MAX_HISTORY - 1; i++) {
                                    strcpy(g_terminal.history[i], g_terminal.history[i + 1]);
                                }
                                g_terminal.history_count--;
                            }
                            
                            strncpy(g_terminal.history[g_terminal.history_count], buffer, MAX_CMD_LENGTH);
                            g_terminal.history[g_terminal.history_count][MAX_CMD_LENGTH - 1] = '\0';
                            g_terminal.history_count++;
                            
                            reset_terminal:
                            // 发送命令到队列
                            len = strlen(buffer) + 1;
                            char* cmd_copy = (char*)malloc(len);
                            if (cmd_copy != NULL) {
                                strcpy(cmd_copy, buffer);
                                if (xQueueSendFromISR(g_terminal.cmd_queue, &cmd_copy, &xHigherPriorityTaskWoken) != pdTRUE) {
                                    free(cmd_copy);
                                }
                            }
                            
                            // 重置终端
                            g_terminal.length = 0;
                            g_terminal.cursor_pos = 0;
                            g_terminal.history_display_index = g_terminal.history_count;
                            memset(buffer, 0, MAX_CMD_LENGTH);
                            
                            // safe_printf("\r\n\033[36m[Lua Shell]>\033[0m ");
                        } else {
                            safe_printf("\r\n\033[36m[Lua Shell]>\033[0m ");
                        }
                    }
                    // 处理退格键
                    else if (rx_char == 0x08 || rx_char == 0x7F) {
                        if (g_terminal.cursor_pos > 0) {
                            memmove(&buffer[g_terminal.cursor_pos - 1],
                                    &buffer[g_terminal.cursor_pos],
                                    g_terminal.length - g_terminal.cursor_pos);
                            
                            g_terminal.cursor_pos--;
                            g_terminal.length--;
                            buffer[g_terminal.length] = '\0';
                            
                            if (g_terminal.echo_enabled) {
                                safe_printf(ANSI_CURSOR_LEFT);
                                if (g_terminal.cursor_pos < g_terminal.length) {
                                    safe_printf(ANSI_SAVE_CURSOR);
                                    safe_printf("%s", &buffer[g_terminal.cursor_pos]);
                                    safe_printf(" ");
                                    safe_printf(ANSI_RESTORE_CURSOR);
                                } else {
                                    safe_printf(" \b");
                                }
                            }
                        }
                    }
                    // 处理普通字符
                    else if (rx_char >= 32 && rx_char != 127) {
                        if (g_terminal.length < MAX_CMD_LENGTH - 1) {
                            if (g_terminal.cursor_pos < g_terminal.length) {
                                memmove(&buffer[g_terminal.cursor_pos + 1],
                                       &buffer[g_terminal.cursor_pos],
                                       g_terminal.length - g_terminal.cursor_pos);
                            }
                            
                            buffer[g_terminal.cursor_pos] = rx_char;
                            g_terminal.cursor_pos++;
                            g_terminal.length++;
                            buffer[g_terminal.length] = '\0';
                            
                            if (g_terminal.echo_enabled) {
                                uart_send_char(rx_char);
                                if (g_terminal.cursor_pos < g_terminal.length) {
                                    safe_printf(ANSI_SAVE_CURSOR);
                                    safe_printf("%s", &buffer[g_terminal.cursor_pos]);
                                    safe_printf(ANSI_RESTORE_CURSOR);
                                }
                            }
                        }
                    }
                }
                break;
                
            case TERM_STATE_ESC:
                if (rx_char == '[') {
                    g_terminal.state = TERM_STATE_ESC_BRACKET;
                } else if (rx_char == 'O') {
                    g_terminal.state = TERM_STATE_ESC_O;
                } else {
                    g_terminal.state = TERM_STATE_NORMAL;
                }
                break;
                
            case TERM_STATE_ESC_BRACKET:
                if (rx_char >= '0' && rx_char <= '9') {
                    g_terminal.state = TERM_STATE_ESC_BRACKET_1;
                } else {
                    // 处理方向键
                    switch (rx_char) {
                        case 'A': // 上箭头
                            if (g_terminal.history_display_index > 0) {
                                if (g_terminal.history_display_index == g_terminal.history_count) {
                                    if (g_terminal.length > 0) {
                                        strncpy(temp_buffer, buffer, MAX_CMD_LENGTH);
                                        temp_buffer[MAX_CMD_LENGTH - 1] = '\0';
                                    } else {
                                        temp_buffer[0] = '\0';
                                    }
                                }
                                g_terminal.history_display_index--;
                                if (g_terminal.history_display_index < g_terminal.history_count) {
                                    strncpy(buffer, g_terminal.history[g_terminal.history_display_index], MAX_CMD_LENGTH);
                                    g_terminal.length = strlen(buffer);
                                    g_terminal.cursor_pos = g_terminal.length;
                                } else if (temp_buffer[0] != '\0') {
                                    strncpy(buffer, temp_buffer, MAX_CMD_LENGTH);
                                    g_terminal.length = strlen(buffer);
                                    g_terminal.cursor_pos = g_terminal.length;
                                } else {
                                    buffer[0] = '\0';
                                    g_terminal.length = 0;
                                    g_terminal.cursor_pos = 0;
                                }
                                buffer[MAX_CMD_LENGTH - 1] = '\0';
                                safe_printf(ANSI_CLEAR_LINE ANSI_CURSOR_HOME);
                                safe_printf("\033[36m[Lua Shell]>\033[0m %s", buffer);
                            }
                            break;
                            
                        case 'B': // 下箭头
                            if (g_terminal.history_display_index < g_terminal.history_count) {
                                g_terminal.history_display_index++;
                                if (g_terminal.history_display_index < g_terminal.history_count) {
                                    strncpy(buffer, g_terminal.history[g_terminal.history_display_index], MAX_CMD_LENGTH);
                                    g_terminal.length = strlen(buffer);
                                    g_terminal.cursor_pos = g_terminal.length;
                                } else if (temp_buffer[0] != '\0') {
                                    strncpy(buffer, temp_buffer, MAX_CMD_LENGTH);
                                    g_terminal.length = strlen(buffer);
                                    g_terminal.cursor_pos = g_terminal.length;
                                } else {
                                    buffer[0] = '\0';
                                    g_terminal.length = 0;
                                    g_terminal.cursor_pos = 0;
                                }
                                buffer[MAX_CMD_LENGTH - 1] = '\0';
                                safe_printf(ANSI_CLEAR_LINE ANSI_CURSOR_HOME);
                                safe_printf("\033[36m[Lua Shell]>\033[0m %s", buffer);
                            }
                            break;
                            
                        case 'C': // 右箭头
                            if (g_terminal.cursor_pos < g_terminal.length) {
                                g_terminal.cursor_pos++;
                                safe_printf(ANSI_CURSOR_RIGHT);
                            }
                            break;
                            
                        case 'D': // 左箭头
                            if (g_terminal.cursor_pos > 0) {
                                g_terminal.cursor_pos--;
                                safe_printf(ANSI_CURSOR_LEFT);
                            }
                            break;
                            
                        case 'H': // Home键
                            if (g_terminal.cursor_pos > 0) {
                                uint16_t steps = g_terminal.cursor_pos;
                                g_terminal.cursor_pos = 0;
                                safe_printf("\033[%uD", steps);
                            }
                            break;
                            
                        case 'F': // End键
                            if (g_terminal.cursor_pos < g_terminal.length) {
                                uint16_t steps = g_terminal.length - g_terminal.cursor_pos;
                                g_terminal.cursor_pos = g_terminal.length;
                                safe_printf("\033[%uC", steps);
                            }
                            break;
                    }
                    g_terminal.state = TERM_STATE_NORMAL;
                }
                break;
                
            case TERM_STATE_ESC_BRACKET_1:
                // 处理Delete键 (ESC [ 3 ~)
                if (rx_char == '~') {
                    if (g_terminal.cursor_pos < g_terminal.length) {
                        memmove(&buffer[g_terminal.cursor_pos],
                                &buffer[g_terminal.cursor_pos + 1],
                                g_terminal.length - g_terminal.cursor_pos - 1);
                        g_terminal.length--;
                        buffer[g_terminal.length] = '\0';
                        safe_printf(ANSI_SAVE_CURSOR);
                        safe_printf("%s", &buffer[g_terminal.cursor_pos]);
                        safe_printf(" ");
                        safe_printf(ANSI_RESTORE_CURSOR);
                    }
                }
                g_terminal.state = TERM_STATE_NORMAL;
                break;
                
            case TERM_STATE_ESC_O:
                // 功能键暂不处理
                g_terminal.state = TERM_STATE_NORMAL;
                break;
                
            default:
                g_terminal.state = TERM_STATE_NORMAL;
                break;
        }
        
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// 获取命令
char* Terminal_GetCommand(uint32_t timeout) {
    char* cmd = NULL;
    if (xQueueReceive(g_terminal.cmd_queue, &cmd, timeout) == pdTRUE) {
        return cmd;
    }
    return NULL;
}

// 释放命令内存
void Terminal_FreeCommand(char* cmd) {
    if (cmd != NULL) {
        free(cmd);
    }
}