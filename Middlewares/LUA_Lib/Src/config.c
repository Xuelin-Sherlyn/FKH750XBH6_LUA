// config.c
#include "config.h"
#include "ff.h"
#include "usart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 加载配置文件
bool config_load(LuaConfig *config, const char *filename)
{
    FIL file;
    FRESULT fr;
    char buffer[384];
    UINT bytes_read;
    
    // 使用默认配置
    LuaConfig default_config = DEFAULT_CONFIG;
    memcpy(config, &default_config, sizeof(LuaConfig));
    
    // 尝试打开配置文件
    fr = f_open(&file, filename, FA_READ);
    if (fr != FR_OK) {
        safe_printf("\rConfiguration file does not exist, using default settings\n");
        return false;
    }
    
    // 读取配置文件
    while (1) {
        fr = f_read(&file, buffer, sizeof(buffer) - 1, &bytes_read);
        if (fr != FR_OK || bytes_read == 0) {
            break;
        }
        
        buffer[bytes_read] = '\0';
        
        // 解析配置文件（简单键值对格式）
        char *line = strtok(buffer, "\n");
        while (line != NULL) {
            // 跳过注释和空行
            if (line[0] == '#' || line[0] == '\r' || line[0] == '\0') {
                line = strtok(NULL, "\n");
                continue;
            }
            
            // 去除回车符
            char *cr = strchr(line, '\r');
            if (cr) *cr = '\0';
            
            // 解析键值对
            char *key = line;
            char *value = strchr(line, '=');
            
            if (value) {
                *value = '\0';
                value++;
                
                // 去除空格
                while (*key == ' ') key++;
                char *key_end = key + strlen(key) - 1;
                while (key_end > key && *key_end == ' ') key_end--;
                *(key_end + 1) = '\0';
                
                while (*value == ' ') value++;
                char *value_end = value + strlen(value) - 1;
                while (value_end > value && *value_end == ' ') value_end--;
                *(value_end + 1) = '\0';
                
                // 处理配置项
                if (strcmp(key, "auto_start_lua") == 0) {
                    config->auto_start_lua = (strcmp(value, "true") == 0 || 
                                            strcmp(value, "1") == 0);
                }
                else if (strcmp(key, "enable_sandbox") == 0) {
                    config->enable_sandbox = (strcmp(value, "true") == 0 || 
                                            strcmp(value, "1") == 0);
                }
                else if (strcmp(key, "max_file_size") == 0) {
                    config->max_file_size = atoi(value);
                }
                else if (strcmp(key, "max_recursion_depth") == 0) {
                    config->max_recursion_depth = atoi(value);
                }
                else if (strcmp(key, "enable_debug") == 0) {
                    config->enable_debug = (strcmp(value, "true") == 0 || 
                                          strcmp(value, "1") == 0);
                }
                else if (strcmp(key, "startup_script") == 0) {
                    strncpy(config->startup_script, value, 
                            sizeof(config->startup_script) - 1);
                    config->startup_script[sizeof(config->startup_script) - 1] = '\0';
                }
                else if (strcmp(key, "lua_path") == 0) {
                    strncpy(config->lua_path, value, 
                            sizeof(config->lua_path) - 1);
                    config->lua_path[sizeof(config->lua_path) - 1] = '\0';
                }
            }
            
            line = strtok(NULL, "\n");
        }
    }
    
    f_close(&file);
    return true;
}

// 保存配置文件
bool config_save(const LuaConfig *config, const char *filename)
{
    FIL file;
    FRESULT fr;
    UINT bytes_written;
    char buffer[384];
    
    fr = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        safe_printf("\rUnable to create configuration file: %s\n", filename);
        return false;
    }
    
    // 写入配置项
    const char *header = "# Lua configuration file\n"
                         "# Reboot after Edit\n\n";
    f_write(&file, header, strlen(header), &bytes_written);
    
    // 写入各个配置项
    snprintf(buffer, sizeof(buffer), "auto_start_lua = %s\n", 
             config->auto_start_lua ? "true" : "false");
    f_write(&file, buffer, strlen(buffer), &bytes_written);
    
    snprintf(buffer, sizeof(buffer), "enable_sandbox = %s\n", 
             config->enable_sandbox ? "true" : "false");
    f_write(&file, buffer, strlen(buffer), &bytes_written);
    
    snprintf(buffer, sizeof(buffer), "max_file_size = %d\n", 
             config->max_file_size);
    f_write(&file, buffer, strlen(buffer), &bytes_written);
    
    snprintf(buffer, sizeof(buffer), "max_recursion_depth = %d\n", 
             config->max_recursion_depth);
    f_write(&file, buffer, strlen(buffer), &bytes_written);
    
    snprintf(buffer, sizeof(buffer), "enable_debug = %s\n", 
             config->enable_debug ? "true" : "false");
    f_write(&file, buffer, strlen(buffer), &bytes_written);
    
    snprintf(buffer, sizeof(buffer), "startup_script = %s\n", 
             config->startup_script);
    f_write(&file, buffer, strlen(buffer), &bytes_written);
    
    snprintf(buffer, sizeof(buffer), "lua_path = %s\n", 
             config->lua_path);
    f_write(&file, buffer, strlen(buffer), &bytes_written);
    
    f_close(&file);
    return true;
}

// 打印配置信息
void config_print(const LuaConfig *config)
{
    safe_printf("=== Lua configuration info ===\n");
    safe_printf("Auto Start Lua Script: %s\n", config->auto_start_lua ? "Yes" : "No");
    safe_printf("Enable SandBox Mode: %s\n", config->enable_sandbox ? "Yes" : "No");
    safe_printf("Max File Size: %d Byte (%.2f MB)\n", 
                config->max_file_size, 
                config->max_file_size / 1024.0 / 1024.0);
    safe_printf("Maximum recursion depth: %d\n", config->max_recursion_depth);
    safe_printf("Enable Debug Output: %s\n", config->enable_debug ? "是" : "否");
    safe_printf("Start Script: %s\n", config->startup_script);
    safe_printf("Lua Module Path: %s\n", config->lua_path);
    safe_printf("========================\n");
}